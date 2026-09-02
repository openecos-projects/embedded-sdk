"""Pinned Python and host-tool dependencies used by the ECOS SDK."""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path
from typing import Any, Optional


PYTHON_DEPENDENCY_RELATIVE_ROOT = Path("lib") / "ecos" / "python"
PYTHON_DEPENDENCY_MARKER = ".ecos-python-dependencies.json"
HOST_DEPENDENCY_RELATIVE_ROOT = Path("lib") / "ecos" / "host"
HOST_DEPENDENCY_MARKER = ".ecos-host-dependencies.json"

# Keep these versions fixed so an SDK installation is reproducible.
PYTHON_DEPENDENCIES = (
    "PyYAML==6.0.3",
    "kconfiglib==14.1.0",
    "pyserial==3.5",
)
HOST_TOOL_DEPENDENCIES: tuple[dict[str, str], ...] = (
    {
        "name": "cmake",
        "requirement": "cmake==3.31.10",
        "executable": "cmake",
        "minimum_version": "3.20",
    },
    {
        "name": "ninja",
        "requirement": "ninja==1.11.1.4",
        "executable": "ninja",
        "minimum_version": "1.10",
    },
)
HOST_TOOL_PACKAGES = tuple(item["requirement"] for item in HOST_TOOL_DEPENDENCIES)


def host_dependency_root(prefix: Path) -> Path:
    """Return the private host dependency root for an SDK prefix."""

    return prefix / HOST_DEPENDENCY_RELATIVE_ROOT


def host_dependency_bin(
    prefix: Path, *, platform_name: Optional[str] = None
) -> Path:
    """Return the directory containing installed host-tool executables."""

    host = sys.platform if platform_name is None else platform_name
    directory = "Scripts" if host == "win32" else "bin"
    return host_dependency_root(prefix) / directory


def host_dependency_paths(
    prefix: Path, *, platform_name: Optional[str] = None
) -> list[Path]:
    """Return SDK-local directories that contain runnable host tools."""

    root = host_dependency_root(prefix)
    # The cmake wheel's bin/cmake is a Python entry point that expects its
    # package on sys.path. Use its bundled native binary directly instead.
    return [
        root / "cmake" / "data" / "bin",
        host_dependency_bin(prefix, platform_name=platform_name),
    ]


def managed_host_tool(root: Path, name: str) -> Optional[Path]:
    """Find a host tool below an SDK-local host dependency root."""

    spec = next(
        (item for item in HOST_TOOL_DEPENDENCIES if item["name"] == name),
        None,
    )
    if spec is None:
        raise ValueError(f"unknown ECOS host tool: {name}")
    executable = spec["executable"]
    # pip creates .exe launchers on Windows. Checking both spellings also keeps
    # status inspection deterministic when tests emulate another host.
    candidates: list[Path] = []
    if name == "cmake":
        candidates.extend(
            [
                root / "cmake" / "data" / "bin" / executable,
                root / "cmake" / "data" / "bin" / f"{executable}.exe",
            ]
        )
    script_directories = (
        ("Scripts", "bin") if sys.platform == "win32" else ("bin", "Scripts")
    )
    for directory in script_directories:
        candidates.extend(
            [
                root / directory / executable,
                root / directory / f"{executable}.exe",
                root / directory / f"{executable}.cmd",
            ]
        )
    for candidate in candidates:
        if candidate.is_file():
            return candidate.resolve()
    return None


def _version_tuple(value: str) -> tuple[int, ...]:
    return tuple(int(part) for part in re.findall(r"\d+", value)[:4])


def probe_host_tool(path: Path, minimum_version: str) -> dict[str, Any]:
    """Run a host tool's version command and validate its minimum version."""

    result: dict[str, Any] = {
        "path": str(path),
        "minimum_version": minimum_version,
        "valid": False,
    }
    try:
        completed = subprocess.run(
            [str(path), "--version"],
            check=False,
            capture_output=True,
            text=True,
            timeout=15,
        )
    except (OSError, subprocess.SubprocessError) as exc:
        result["error"] = str(exc)
        return result
    output = "\n".join(
        value for value in (completed.stdout, completed.stderr) if value
    )
    match = re.search(r"\b(\d+(?:\.\d+){1,3})\b", output)
    if match:
        result["version"] = match.group(1)
        result["valid"] = (
            completed.returncode == 0
            and _version_tuple(match.group(1)) >= _version_tuple(minimum_version)
        )
    else:
        result["error"] = "version output did not contain a semantic version"
    if completed.returncode != 0 and "error" not in result:
        result["error"] = f"exited with status {completed.returncode}"
    return result
