#!/usr/bin/env python3
"""Collect an ECOS 3.x project report using read-only CLI commands only.

The skill's installation location is never used to select an SDK. All subprocesses
use argument arrays (no shell); this helper does not install, configure or build.
"""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any, Optional


class CheckError(RuntimeError):
    pass


def cli_prefix(value: str) -> list[str]:
    """Use the installed Windows launcher descriptor instead of cmd.exe."""
    candidate = Path(value).expanduser()
    executable = candidate if candidate.is_file() else None
    if executable is None:
        found = shutil.which(value)
        if found:
            executable = Path(found)
    if executable is None:
        raise CheckError(f"ecos CLI not found: {value}; install it explicitly or pass --cli PATH")
    executable = executable.resolve()
    suffix = executable.suffix.lower()
    if suffix == ".cmd":
        descriptor_path = executable.parent.parent / "share/ecos/cli-launch.json"
        try:
            descriptor = json.loads(descriptor_path.read_text(encoding="utf-8"))
        except (OSError, ValueError) as exc:
            raise CheckError(f"cannot read CLI launch descriptor {descriptor_path}: {exc}") from exc
        if not isinstance(descriptor, dict):
            raise CheckError(f"invalid CLI launch descriptor: {descriptor_path}")
        prefix = [descriptor.get("python_executable"), descriptor.get("script")]
        if not all(isinstance(item, str) and item and Path(item).is_file() for item in prefix):
            raise CheckError(f"CLI descriptor must name existing Python/script files: {descriptor_path}")
        return prefix
    if suffix == ".bat":
        raise CheckError(".bat launchers are unsupported; pass the installed ecos.cmd or a Python entry")
    if suffix == ".py":
        return [sys.executable, str(executable)]
    return [str(executable)]


def query(prefix: list[str], arguments: list[str], project: Path, timeout: int) -> dict[str, Any]:
    argv = [*prefix, *arguments, "--format", "json"]
    environment = os.environ.copy()
    environment["PYTHONDONTWRITEBYTECODE"] = "1"
    environment["PYTHONUTF8"] = "1"
    try:
        result = subprocess.run(
            argv, cwd=project, env=environment, shell=False,
            capture_output=True, text=True, encoding="utf-8", errors="replace",
            timeout=timeout, check=False,
        )
    except subprocess.TimeoutExpired:
        return {"argv": argv, "ok": False, "error": f"CLI query timed out after {timeout}s"}
    except OSError as exc:
        return {"argv": argv, "ok": False, "error": str(exc)}
    record: dict[str, Any] = {
        "argv": argv, "exit_code": result.returncode, "stderr": result.stderr,
    }
    try:
        payload = json.loads(result.stdout)
    except ValueError:
        record.update(ok=False, error="CLI did not return JSON", stdout=result.stdout)
        return record
    if not isinstance(payload, dict) or payload.get("schema_version") != "1.0":
        record.update(ok=False, error="unsupported CLI JSON envelope", payload=payload)
        return record
    cli_version = payload.get("cli_version")
    if not isinstance(cli_version, str) or cli_version.split(".", 1)[0] != "3":
        record.update(ok=False, error="this skill requires CLI major 3", payload=payload)
        return record
    record.update(payload=payload, ok=result.returncode == 0 and payload.get("status") == "ok")
    return record


def selected_sdk(record: dict[str, Any]) -> tuple[Path, dict[str, Any]]:
    if not record["ok"]:
        raise CheckError("sdk current failed; fix the selection/CLI diagnostics, no fallback was attempted")
    data = record["payload"].get("data")
    if not isinstance(data, dict) or not isinstance(data.get("root"), str) or not data["root"]:
        raise CheckError("sdk current did not identify an SDK root")
    root = Path(data["root"]).expanduser().resolve()
    manifest_path = root / "tools/sdk-manifest.json"
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    except (OSError, ValueError) as exc:
        raise CheckError(f"cannot read selected SDK manifest {manifest_path}: {exc}") from exc
    if not isinstance(manifest, dict):
        raise CheckError(f"invalid selected SDK manifest: {manifest_path}")
    version = manifest.get("sdk_version")
    compatibility = manifest.get("cli_compatibility")
    if (
        manifest.get("schema_version") != 1
        or manifest.get("sdk_id") != "ecos-embedded-sdk"
        or not isinstance(version, str) or version.split(".", 1)[0] != "3"
        or not isinstance(compatibility, dict) or compatibility.get("major") != 3
        or compatibility.get("schema_version") != "1.0"
    ):
        raise CheckError("this skill requires ecos-embedded-sdk major 3 / SDK schema 1 / CLI schema 1.0")
    if data.get("sdk_id") != manifest["sdk_id"] or data.get("sdk_version") != version:
        raise CheckError("CLI selection and SDK manifest identities disagree")
    return root, manifest


def collect(cli: str, sdk: Optional[str], project: Path, timeout: int) -> dict[str, Any]:
    report: dict[str, Any] = {
        "schema_version": 1, "command": "skill.check-project", "status": "error",
        "project": str(project), "queries": [], "diagnostics": [],
        "hardware_tested": False,
    }
    try:
        if sdk is not None and not sdk.strip():
            raise CheckError("an explicit --sdk selector must not be empty; no fallback was attempted")
        if timeout <= 0:
            raise CheckError("--timeout must be positive")
        project = project.expanduser().resolve()
        report["project"] = str(project)
        if not project.is_dir():
            raise CheckError(f"project directory does not exist: {project}")
        prefix = cli_prefix(cli)
        selection = ["--sdk", sdk] if sdk is not None else []
        record = query(prefix, [*selection, "sdk", "current", "--project", str(project)], project, timeout)
        report["queries"].append(record)
        root, manifest = selected_sdk(record)
        report["sdk"] = {
            "root": str(root), "id": manifest["sdk_id"], "version": manifest["sdk_version"],
            "channel": manifest.get("channel"), "selected_by": record["payload"]["data"].get("source"),
        }
        # Pin every subsequent query to the CLI-confirmed root, not to this skill.
        for arguments in (
            ["project", "info", "--project", str(project)],
            ["validate", "--project", str(project)],
            ["toolchain", "status"],
        ):
            report["queries"].append(query(prefix, ["--sdk", str(root), *arguments], project, timeout))
        if not all(item["ok"] for item in report["queries"]):
            raise CheckError("one or more read-only CLI checks failed; inspect their diagnostics")
        toolchain = report["queries"][-1]["payload"].get("data")
        installation = toolchain.get("installation") if isinstance(toolchain, dict) else None
        if not isinstance(installation, dict) or installation.get("state") != "installed":
            raise CheckError("toolchain status succeeded but the compiler installation is not ready")
        report["status"] = "ok"
    except (CheckError, OSError, ValueError) as exc:
        report["diagnostics"].append(str(exc))
    return report


def main(argv: Optional[list[str]] = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cli", default="ecos", help="ecos executable, installed ecos.cmd or tools/ecos.py")
    parser.add_argument("--sdk", help="explicit SDK selector/path; otherwise use CLI project-pin resolution")
    parser.add_argument("--project", type=Path, default=Path.cwd())
    parser.add_argument("--timeout", type=int, default=60, help="maximum seconds per read-only query")
    args = parser.parse_args(argv)
    if args.timeout <= 0:
        parser.error("--timeout must be positive")
    report = collect(args.cli, args.sdk, args.project, args.timeout)
    print(json.dumps(report, indent=2, ensure_ascii=True))
    return 0 if report["status"] == "ok" else 3


if __name__ == "__main__":
    raise SystemExit(main())
