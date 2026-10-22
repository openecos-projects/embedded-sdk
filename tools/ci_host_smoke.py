#!/usr/bin/env python3
"""Exercise the installed SDK and optionally build/package the full example matrix."""

from __future__ import annotations

import argparse
import json
import os
import platform
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any, Callable

try:
    from ci_firmware import FirmwarePackage
except ModuleNotFoundError:  # Support importing this script as tools.ci_host_smoke.
    from tools.ci_firmware import FirmwarePackage


SDK_ROOT = Path(__file__).resolve().parent.parent
CLI_SOURCE = SDK_ROOT / "tools/ecos.py"
PYTHON_SOURCE = SDK_ROOT / "tools/ecos_cli/src"


def _json_command(
    cli: Path, sdk_root: Path, args: list[str], environment: dict[str, str]
) -> dict[str, Any]:
    if cli.suffix.lower() == ".cmd":
        descriptor_path = cli.parent.parent / "share" / "ecos" / "cli-launch.json"
        descriptor = json.loads(descriptor_path.read_text(encoding="utf-8"))
        command = [
            descriptor["python_executable"], descriptor["script"],
            "--sdk", str(sdk_root), *args, "--format", "json",
        ]
    else:
        command = [
            sys.executable, str(cli), "--sdk", str(sdk_root),
            *args, "--format", "json",
        ]
    result = subprocess.run(
        command, cwd=SDK_ROOT, env=environment, capture_output=True,
        text=True, check=False,
    )
    try:
        payload = json.loads(result.stdout)
    except json.JSONDecodeError as exc:
        raise RuntimeError(
            f"{' '.join(args)} returned invalid JSON:\n{result.stdout}\n{result.stderr}"
        ) from exc
    if result.returncode or payload.get("status") != "ok":
        raise RuntimeError(
            f"{' '.join(args)} failed ({result.returncode}):\n"
            f"{result.stdout}\n{result.stderr}"
        )
    data = payload.get("data")
    if not isinstance(data, dict):
        raise RuntimeError(f"{' '.join(args)} returned no data")
    return data


def _install(host_prefix: Path, workspace: Path) -> Path:
    command = [
        sys.executable, str(SDK_ROOT / "tools/install.py"),
        "--shell", "none", "--prefix", str(workspace / "SDKs"),
        "--host-prefix", str(host_prefix),
        "--cache-dir", str(workspace / "cache"),
        "--registry", str(workspace / "sdks.json"),
        "--format", "json",
    ]
    result = subprocess.run(
        command, cwd=SDK_ROOT, capture_output=True, text=True, check=False
    )
    if result.returncode:
        raise RuntimeError(
            f"SDK installation failed ({result.returncode}):\n"
            f"{result.stdout}\n{result.stderr}"
        )
    payload = json.loads(result.stdout)
    if payload.get("status") != "ok":
        raise RuntimeError(f"SDK installation returned {payload!r}")
    return Path(payload["data"]["prefix"])


def _environment(host_prefix: Path) -> dict[str, str]:
    environment = os.environ.copy()
    python_paths = [
        str(PYTHON_SOURCE), str(host_prefix / "lib/ecos/python"),
    ]
    if environment.get("PYTHONPATH"):
        python_paths.append(environment["PYTHONPATH"])
    environment["PYTHONPATH"] = os.pathsep.join(python_paths)
    host_paths = [
        str(host_prefix / "lib/ecos/host/cmake/data/bin"),
        str(host_prefix / "lib/ecos/host/bin"),
    ]
    environment["PATH"] = os.pathsep.join([*host_paths, environment.get("PATH", "")])
    return environment


def _tests(environment: dict[str, str]) -> None:
    subprocess.run(
        [sys.executable, "-m", "unittest", "discover", "-s", "tools/ecos_cli/tests", "-q"],
        cwd=SDK_ROOT, env=environment, check=True,
    )


def _smoke(
    cli: Path, sdk_root: Path, environment: dict[str, str], workspace: Path
) -> dict[str, Any]:
    project_parent = workspace / "ECOS 工程 with spaces"
    project_parent.mkdir(parents=True)
    name = "hello-smoke"
    project = project_parent / name
    _json_command(cli, sdk_root, [
        "project", "create", "hello", "--name", name,
        "--path", str(project_parent), "--board", "starrysky-l4-c1",
    ], environment)
    _json_command(cli, sdk_root, ["configure", "--project", str(project)], environment)
    _json_command(cli, sdk_root, ["build", "--project", str(project)], environment)
    info = _json_command(
        cli, sdk_root, ["project", "info", "--project", str(project)], environment
    )
    if not info.get("built") or not Path(info["outputs"]["elf"]).is_file():
        raise RuntimeError("Unicode-path smoke project did not produce a verified ELF")
    print(f"Host smoke build passed: {ascii(str(project))}", flush=True)
    return _build_record(project, "hello")


def _build_record(project: Path, example: str) -> dict[str, Any]:
    artifact = json.loads(
        (project / "build/artifacts.json").read_text(encoding="utf-8")
    )
    return {
        "board": artifact["project"]["board"],
        "example": example,
        "target": artifact["project"]["target"],
        "profile": artifact["project"]["profile"],
        "isa": artifact["firmware"]["isa"],
        "abi": artifact["firmware"]["abi"],
        "image_size_limit": artifact["firmware"].get("size_limit"),
        "sdk": artifact["sdk"],
        "toolchain": artifact["toolchain"],
        "source_fingerprint": artifact["source_fingerprint"],
        "configuration_fingerprint": artifact.get("configuration_fingerprint"),
        "elf": artifact["files"]["elf"],
        "bin": artifact["files"]["bin"],
    }


def _build_matrix(
    cli: Path, sdk_root: Path, environment: dict[str, str], workspace: Path,
    selected_boards: list[str] | None,
    artifact_callback: Callable[[Path, dict[str, Any]], None] | None = None,
) -> list[dict[str, Any]]:
    boards = _json_command(cli, sdk_root, ["board", "list"], environment)["boards"]
    examples = _json_command(cli, sdk_root, ["project", "list"], environment)["examples"]
    board_ids = sorted(item["id"] for item in boards)
    if selected_boards:
        unknown = sorted(set(selected_boards) - set(board_ids))
        if unknown:
            raise RuntimeError(f"unknown Board in CI matrix: {', '.join(unknown)}")
        board_ids = sorted(set(selected_boards))
    if not examples or any(not item["supported_boards"] for item in examples):
        raise RuntimeError("an SDK Example has no compatible Board")

    records: list[dict[str, Any]] = []
    parent = workspace / "matrix"
    parent.mkdir(parents=True)
    for board_id in board_ids:
        board = _json_command(cli, sdk_root, ["board", "describe", board_id], environment)
        for example in examples:
            if board_id not in example["supported_boards"]:
                continue
            name = f"{board_id}-{example['name']}"
            project = parent / name
            _json_command(cli, sdk_root, [
                "project", "create", example["name"], "--name", name,
                "--path", str(parent), "--board", board_id,
            ], environment)
            if example["name"] == "llm-chat":
                # The external model is runtime data; one byte tests .incbin and linking.
                for filename in ("stories260k_q8.bin", "tok512.bin"):
                    (project / filename).write_bytes(b"\0")
            _json_command(cli, sdk_root, ["build", "--project", str(project)], environment)
            artifact = json.loads(
                (project / "build/artifacts.json").read_text(encoding="utf-8")
            )
            cpu = board["cpu"]
            expected_class = "ELF32" if cpu["march"].startswith("rv32") else "ELF64"
            if (
                artifact["project"]["board"] != board_id
                or artifact["firmware"]["isa"] != cpu["march"]
                or artifact["firmware"]["abi"] != cpu["abi"]
                or artifact["elf"]["class"] != expected_class
            ):
                raise RuntimeError(f"artifact target contract mismatched: {name}")
            record = _build_record(project, example["name"])
            if artifact_callback is not None:
                artifact_callback(project, record)
            records.append(record)
            print(f"[{len(records)}] {board_id} / {example['name']} ({cpu['abi']})", flush=True)
    if not records:
        raise RuntimeError("Board/Example matrix contained no builds")
    print(f"Board/Example matrix passed: {len(records)} builds", flush=True)
    return records


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--skip-install", action="store_true")
    parser.add_argument("--skip-tests", action="store_true")
    parser.add_argument("--report", type=Path, help="write a JSON build record outside the temporary workspace")
    parser.add_argument(
        "--firmware-dir", type=Path,
        help="package matrix .bin files and metadata under this directory",
    )
    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        "--all-examples", action="store_true",
        default=os.environ.get("ECOS_CI_ALL_EXAMPLES") == "true",
    )
    group.add_argument(
        "--board", action="append", dest="boards", metavar="BOARD_ID",
        help="build all compatible examples for this Board (repeat for multiple Boards)",
    )
    args = parser.parse_args()
    if args.firmware_dir is not None and not (args.all_examples or args.boards):
        parser.error("--firmware-dir requires --all-examples or --board")

    with tempfile.TemporaryDirectory(prefix="ecos-ci-") as directory:
        workspace = Path(directory)
        host_prefix = workspace / "host"
        sdk_root = SDK_ROOT if args.skip_install else _install(host_prefix, workspace)
        environment = _environment(host_prefix)
        cli = CLI_SOURCE if args.skip_install else (
            host_prefix / "bin/ecos.cmd" if os.name == "nt" else host_prefix / "bin/ecos"
        )
        if not args.skip_tests:
            _tests(environment)
        smoke = _smoke(cli, sdk_root, environment, workspace)
        matrix: list[dict[str, Any]] = []
        firmware: FirmwarePackage | None = None
        firmware_summary: dict[str, Any] | None = None
        if args.all_examples or args.boards:
            if args.firmware_dir is not None:
                firmware_dir = args.firmware_dir
                if not firmware_dir.is_absolute():
                    firmware_dir = SDK_ROOT / firmware_dir
                firmware = FirmwarePackage(firmware_dir)
            matrix = _build_matrix(
                cli, sdk_root, environment, workspace, args.boards,
                firmware.add if firmware is not None else None,
            )
            if firmware is not None:
                firmware_summary = firmware.finalize()
        if args.report is not None:
            report = {
                "schema_version": 1,
                "host": {"system": platform.system(), "machine": platform.machine()},
                "smoke": smoke,
                "matrix": matrix,
            }
            if firmware_summary is not None:
                report["firmware"] = firmware_summary
            args.report.parent.mkdir(parents=True, exist_ok=True)
            args.report.write_text(
                json.dumps(report, indent=2, ensure_ascii=False, sort_keys=True) + "\n",
                encoding="utf-8",
            )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, subprocess.CalledProcessError) as exc:
        print(f"ECOS CI smoke failed: {exc}", file=sys.stderr)
        raise SystemExit(1) from exc
