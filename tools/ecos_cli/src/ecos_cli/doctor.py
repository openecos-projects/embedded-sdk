"""Read-only checks for a selected SDK and project environment."""

from __future__ import annotations

import os
import shutil
from pathlib import Path
from typing import Any, Optional

from . import dependencies, flashing, monitoring, project, toolchain
from .project_model import ProjectModelError, resolve_project
from .sdk_context import SdkContext


def _issue(
    code: str, severity: str, message: str, *, resource: str, suggestion: str
) -> dict[str, str]:
    return {
        "code": code,
        "severity": severity,
        "message": message,
        "resource": resource,
        "suggestion": suggestion,
    }


def inspect_environment(
    context: SdkContext, *, project_root: Optional[Path] = None
) -> dict[str, Any]:
    root = (project_root or Path.cwd()).expanduser().resolve()
    diagnostics: list[dict[str, str]] = []
    checks: dict[str, Any] = {
        "sdk": {"state": "valid", **context.as_dict()},
        "project": {"state": "invalid", "path": str(root)},
        "host_tools": {},
        "toolchain": {"state": "unknown"},
        "flash": {"state": "not_applicable"},
        "monitor": {"state": "not_applicable"},
    }

    resolved: Optional[dict[str, Any]] = None
    try:
        resolved = resolve_project(context, project_root=root)
        checks["project"] = {
            "state": "valid",
            "path": str(root),
            "board": resolved["project"]["board"],
            "target": resolved["project"]["target"],
            "profile": resolved["project"]["profile"],
        }
    except (ProjectModelError, project.ProjectError) as exc:
        diagnostics.append(
            _issue(
                "ECOS_DOCTOR_PROJECT_INVALID",
                "error",
                str(exc),
                resource=str(root),
                suggestion="Run 'ecos validate --project PATH' for details.",
            )
        )

    managed_root = dependencies.host_dependency_root(
        dependencies.default_host_prefix()
    )
    for item in dependencies.HOST_TOOL_DEPENDENCIES:
        name = item["name"]
        selected = dependencies.managed_host_tool(managed_root, name)
        if selected is None:
            found = shutil.which(item["executable"]) or shutil.which(
                f"{item['executable']}.exe"
            )
            selected = Path(found) if found else None
        if selected is None:
            checks["host_tools"][name] = {"state": "missing"}
            diagnostics.append(
                _issue(
                    "ECOS_DOCTOR_HOST_TOOL_MISSING",
                    "error",
                    f"required host tool {name} was not found",
                    resource=name,
                    suggestion="Run 'tools/install.py' to install SDK host dependencies.",
                )
            )
            continue
        probe = dependencies.probe_host_tool(selected, item["minimum_version"])
        state = "valid" if probe["valid"] else "invalid"
        checks["host_tools"][name] = {"state": state, **probe}
        if not probe["valid"]:
            diagnostics.append(
                _issue(
                    "ECOS_DOCTOR_HOST_TOOL_INVALID",
                    "error",
                    f"host tool {name} does not meet the SDK minimum version",
                    resource=str(selected),
                    suggestion="Run 'tools/install.py' to install SDK host dependencies.",
                )
            )

    try:
        manifest = toolchain.load_manifest()
        host = toolchain.detect_host()
        toolchain.select_asset(manifest, host)
        requirement = (
            resolved["toolchain"]
            if resolved is not None
            else context.manifest["toolchain"]
        )
        if (
            requirement["id"] != manifest["id"]
            or requirement["release"] != manifest["release"]
        ):
            raise toolchain.ManifestError(
                "selected SDK requires a different toolchain than this CLI provides"
            )
        prefix = toolchain.default_prefix()
        legacy_roots = (toolchain.legacy_prefix(),)
        if context.kind == "release":
            legacy_roots += (context.root,)
        status = toolchain.compatible_installation_status(
            manifest, prefix, host, legacy_roots=legacy_roots
        )
        checks["toolchain"] = {"host": host, **status}
        if status["state"] != "installed":
            diagnostics.append(
                _issue(
                    "ECOS_DOCTOR_TOOLCHAIN_NOT_READY",
                    "error",
                    f"SDK toolchain state is {status['state']}",
                    resource=str(status["root"]),
                    suggestion=(
                        "Run 'ecos toolchain install' or inspect "
                        "'ecos toolchain status'."
                    ),
                )
            )
    except toolchain.ToolchainError as exc:
        checks["toolchain"] = {"state": "invalid", "error": str(exc)}
        diagnostics.append(
            _issue(
                "ECOS_DOCTOR_TOOLCHAIN_INVALID",
                "error",
                str(exc),
                resource="toolchain",
                suggestion="Run 'ecos toolchain detect' and 'ecos toolchain status'.",
            )
        )

    board = resolved.get("board") if resolved is not None else None
    if isinstance(board, dict):
        flash = board.get("flash")
        if isinstance(flash, dict):
            provider = flash.get("provider")
            label = flash.get("volume_label")
            if provider == "mass-storage" and isinstance(label, str):
                devices = flashing.discover_mass_storage(label)
                checks["flash"] = {
                    "state": "ready" if devices else "disconnected",
                    "provider": "mass-storage",
                    "volume_label": label,
                    "devices": [str(device) for device in devices],
                }
                if not devices:
                    diagnostics.append(
                        _issue(
                            "ECOS_DOCTOR_PROGRAMMER_DISCONNECTED",
                            "warning",
                            f"programmer volume {label!r} is not mounted",
                            resource=board["id"],
                            suggestion="Connect the programmer before running 'ecos flash'.",
                        )
                    )
            elif provider == "wlink":
                command = os.environ.get("ECOS_WLINK") or flash.get("tool")
                try:
                    executable = flashing.resolve_wlink_command(command)
                except flashing.FlashConfigurationError as exc:
                    checks["flash"] = {
                        "state": "unavailable",
                        "provider": "wlink",
                        "tool": str(command),
                        "error": str(exc),
                    }
                    diagnostics.append(
                        _issue(
                            "ECOS_DOCTOR_FLASH_TOOL_MISSING",
                            "warning",
                            str(exc),
                            resource=board["id"],
                            suggestion=(
                                "Install wlink, or set Board.flash.tool / ECOS_WLINK."
                            ),
                        )
                    )
                else:
                    checks["flash"] = {
                        "state": "ready",
                        "provider": "wlink",
                        "tool": str(executable),
                        "chip": flash.get("chip"),
                    }
        monitor = board.get("monitor")
        if isinstance(monitor, dict):
            try:
                ports = monitoring.discover_ports(monitor)
                checks["monitor"] = {
                    "state": "ready" if ports else "disconnected",
                    "ports": ports,
                    "baudrate": monitor.get("baudrate", 115200),
                }
                if not ports:
                    diagnostics.append(
                        _issue(
                            "ECOS_DOCTOR_SERIAL_DISCONNECTED",
                            "warning",
                            "no matching serial port is connected",
                            resource=board["id"],
                            suggestion="Connect the board or pass --port when monitoring.",
                        )
                    )
            except (monitoring.MonitorDependencyError, OSError) as exc:
                checks["monitor"] = {"state": "unavailable", "error": str(exc)}
                diagnostics.append(
                    _issue(
                        "ECOS_DOCTOR_SERIAL_UNAVAILABLE",
                        "warning",
                        str(exc),
                        resource=board["id"],
                        suggestion="Install the SDK Python dependencies before monitoring.",
                    )
                )

    return {
        "path": str(root),
        "valid": not any(item["severity"] == "error" for item in diagnostics),
        "checks": checks,
        "diagnostics": diagnostics,
    }
