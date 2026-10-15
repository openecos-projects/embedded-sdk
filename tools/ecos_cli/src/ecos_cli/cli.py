"""Command-line entry point for the ECOS Embedded SDK."""

from __future__ import annotations

import argparse
import json
import sys
from enum import IntEnum
from pathlib import Path
from typing import Any, Optional, Sequence

from . import __schema_version__, __version__
from . import artifacts
from . import build
from . import completion
from . import configuration
from . import doctor
from . import flashing
from . import installer
from . import monitoring
from . import project
from . import project_model
from . import schema_contract
from . import sdk_manifest
from . import toolchain
from .progress import ConsoleProgress
from .sdk_context import SdkContext
from .sdk_registry import SdkRegistry, SdkRegistryError
from .sdk_resolver import SdkResolutionError, SdkResolver, write_project_pin


CONTRACT_VERSION = __schema_version__


class ExitCode(IntEnum):
    OK = 0
    USAGE = 2
    CONFIG = 3
    UNSUPPORTED = 4
    EXTERNAL_TOOL = 5
    NETWORK = 6


class UsageError(ValueError):
    """Raised instead of exiting when command arguments are invalid."""


class EcosArgumentParser(argparse.ArgumentParser):
    def error(self, message: str) -> None:
        raise UsageError(message)


def diagnostic(
    code: str,
    severity: str,
    message: str,
    *,
    resource: Optional[str] = None,
    field: Optional[str] = None,
    actual: Any = None,
    expected: Any = None,
    suggestion: Optional[str] = None,
) -> dict[str, Any]:
    item: dict[str, Any] = {
        "code": code,
        "severity": severity,
        "message": message,
    }
    optional = {
        "resource": resource,
        "field": field,
        "actual": actual,
        "expected": expected,
        "suggestion": suggestion,
    }
    item.update({key: value for key, value in optional.items() if value is not None})
    return item


def envelope(
    command: str,
    status: str,
    data: Any,
    diagnostics: Optional[list[dict[str, Any]]] = None,
) -> dict[str, Any]:
    return {
        "cli_version": __version__,
        "schema_version": CONTRACT_VERSION,
        "command": command,
        "status": status,
        "data": data,
        "diagnostics": diagnostics or [],
    }


def emit_json(payload: dict[str, Any]) -> None:
    print(json.dumps(payload, indent=2, ensure_ascii=False))


def emit_error(
    command: str,
    output_format: str,
    code: str,
    message: str,
    exit_code: ExitCode,
    *,
    suggestion: Optional[str] = None,
) -> int:
    item = diagnostic(code, "error", message, suggestion=suggestion)
    if output_format == "json":
        emit_json(envelope(command, "error", None, [item]))
    else:
        console = ConsoleProgress()
        console.error(f"[{code}] {message}")
        if suggestion:
            console.info(f"Suggestion: {suggestion}")
    return int(exit_code)


def _requested_output_format(arguments: Sequence[str]) -> str:
    values = list(arguments)
    for index, value in enumerate(values):
        if value == "--format" and index + 1 < len(values):
            return "json" if values[index + 1] == "json" else "text"
        if value.startswith("--format="):
            return "json" if value.split("=", 1)[1] == "json" else "text"
    return "text"


def _add_selection_options(parser: argparse.ArgumentParser) -> None:
    parser.add_argument(
        "--format", choices=("text", "json"), default="text", dest="output_format"
    )
    parser.add_argument("--manifest", type=Path, help=argparse.SUPPRESS)
    parser.add_argument("--host", help=argparse.SUPPRESS)


def create_toolchain_parser(sdk_context: SdkContext) -> argparse.ArgumentParser:
    parser = EcosArgumentParser(
        prog="ecos toolchain",
        description="Detect, inspect, and install the SDK-pinned RISC-V toolchain.",
    )
    commands = parser.add_subparsers(dest="toolchain_command", required=True)
    default_toolchain_prefix = toolchain.default_prefix()

    detect = commands.add_parser("detect", help="detect the host and select its pinned asset")
    _add_selection_options(detect)

    status = commands.add_parser("status", help="inspect the selected toolchain installation")
    _add_selection_options(status)
    status.add_argument("--prefix", type=Path, default=default_toolchain_prefix)
    status.add_argument(
        "--custom",
        type=Path,
        metavar="DIRECTORY",
        help="inspect an explicitly supplied toolchain directory",
    )

    install = commands.add_parser("install", help="download and install the pinned toolchain")
    _add_selection_options(install)
    install.add_argument("--prefix", type=Path, default=default_toolchain_prefix)
    install.add_argument("--cache-dir", type=Path, default=toolchain.default_cache_root())
    install.add_argument("--archive", type=Path, help="install from a local archive")
    install.add_argument("--force", action="store_true", help="replace an invalid installation")
    install.add_argument(
        "--dry-run", action="store_true", help="show the install plan without writing or downloading"
    )
    return parser


def _load_selection(
    args: argparse.Namespace, sdk_context: SdkContext
) -> tuple[dict[str, Any], str]:
    manifest_path = args.manifest
    if manifest_path is None:
        manifest_path = (
            sdk_context.root
            / "tools"
            / "ecos_cli"
            / "src"
            / "ecos_cli"
            / "resources"
            / "toolchains"
            / toolchain.MANIFEST_NAME
        )
    manifest = toolchain.load_manifest(manifest_path)
    expected = sdk_context.manifest["toolchain"]
    if manifest["id"] != expected["id"] or manifest["release"] != expected["release"]:
        raise toolchain.ManifestError(
            "selected toolchain manifest does not match SDK requirement: "
            f"expected {expected['id']} {expected['release']}, "
            f"got {manifest['id']} {manifest['release']}"
        )
    host = args.host or toolchain.detect_host()
    toolchain.select_asset(manifest, host)
    return manifest, host


def _print_detection(data: dict[str, Any], console: Optional[ConsoleProgress] = None) -> None:
    emit = console.info if console else print
    asset = data["asset"]
    emit(f"Host: {data['host']}")
    emit(f"Toolchain: {data['name']} {data['release']}")
    emit(f"Tool prefix: {data['tool_prefix']}")
    emit(f"Asset: {asset['file']}")
    emit(f"SHA-256: {asset['sha256']}")


def _print_status(data: dict[str, Any], console: Optional[ConsoleProgress] = None) -> None:
    emit = console.info if console else print
    emit(f"State: {data['state']}")
    emit(f"Provider: {data['provider']}")
    emit(f"Root: {data['root']}")
    compiler = data["compiler"]
    emit(f"Compiler: {compiler['path']}")
    if compiler.get("version"):
        emit(f"Version: {compiler['version']}")
    if compiler.get("target_triplet"):
        emit(f"Target: {compiler['target_triplet']}")


def _toolchain_exception(
    command: str, output_format: str, exc: toolchain.ToolchainError
) -> int:
    if isinstance(exc, toolchain.UnsupportedHostError):
        return emit_error(
            command,
            output_format,
            "ECOS_TOOLCHAIN_UNSUPPORTED_HOST",
            str(exc),
            ExitCode.UNSUPPORTED,
            suggestion="Use a supported host or provide a validated custom toolchain.",
        )
    if isinstance(exc, toolchain.ManifestError):
        return emit_error(
            command,
            output_format,
            "ECOS_TOOLCHAIN_MANIFEST_INVALID",
            str(exc),
            ExitCode.CONFIG,
        )
    if isinstance(exc, toolchain.DownloadError):
        return emit_error(
            command,
            output_format,
            "ECOS_TOOLCHAIN_DOWNLOAD_FAILED",
            str(exc),
            ExitCode.NETWORK,
            suggestion="Check network access or retry with --archive PATH.",
        )
    if isinstance(exc, toolchain.IntegrityError):
        return emit_error(
            command,
            output_format,
            "ECOS_TOOLCHAIN_INTEGRITY_FAILED",
            str(exc),
            ExitCode.EXTERNAL_TOOL,
            suggestion="Use the archive named by 'ecos toolchain detect'.",
        )
    return emit_error(
        command,
        output_format,
        "ECOS_TOOLCHAIN_INSTALL_FAILED",
        str(exc),
        ExitCode.EXTERNAL_TOOL,
    )


def run_toolchain(argv: Sequence[str], sdk_context: SdkContext) -> int:
    parser = create_toolchain_parser(sdk_context)
    raw_arguments = list(argv)
    requested_format = "json" if "--format" in raw_arguments and "json" in raw_arguments else "text"
    requested_command = raw_arguments[0] if raw_arguments else "unknown"
    try:
        args = parser.parse_args(raw_arguments)
    except UsageError as exc:
        return emit_error(
            f"toolchain.{requested_command}",
            requested_format,
            "ECOS_USAGE_INVALID_ARGUMENTS",
            str(exc),
            ExitCode.USAGE,
            suggestion="Run 'ecos toolchain --help' for supported arguments.",
        )
    command = f"toolchain.{args.toolchain_command}"
    try:
        manifest, host = _load_selection(args, sdk_context)
        selection = toolchain.selection_data(manifest, host)
        selection["sdk"] = sdk_context.as_dict()
        if args.toolchain_command == "detect":
            payload = envelope(command, "ok", selection)
            if args.output_format == "json":
                emit_json(payload)
            else:
                _print_detection(selection, ConsoleProgress())
            return int(ExitCode.OK)

        prefix = args.prefix.expanduser().resolve()
        if args.toolchain_command == "status":
            if args.custom is not None:
                status = toolchain.installation_status(manifest, prefix, host, args.custom)
            elif prefix == toolchain.default_prefix().resolve():
                legacy_roots = (toolchain.legacy_prefix(),)
                if sdk_context.kind == "release":
                    legacy_roots += (sdk_context.root,)
                status = toolchain.compatible_installation_status(
                    manifest, prefix, host, legacy_roots=legacy_roots
                )
            else:
                status = toolchain.installation_status(manifest, prefix, host)
            diagnostics: list[dict[str, Any]] = []
            if status["provider"] == "custom":
                diagnostics.append(
                    diagnostic(
                        "ECOS_TOOLCHAIN_CUSTOM_PROVIDER",
                        "warning",
                        "the custom toolchain is not the SDK-pinned default",
                        resource=manifest["id"],
                        actual=status["root"],
                        expected=f"{manifest['name']} {manifest['release']}",
                        suggestion="Use the pinned provider for reproducible SDK builds.",
                    )
                )
            if status["state"] != "installed":
                suggestion = (
                    "Select a custom directory containing bin/riscv-none-elf-gcc."
                    if status["provider"] == "custom"
                    else "Run 'ecos toolchain install'."
                )
                diagnostics.append(
                    diagnostic(
                        "ECOS_TOOLCHAIN_NOT_READY",
                        "warning",
                        f"toolchain state is {status['state']}",
                        resource=manifest["id"],
                        actual=status["state"],
                        expected="installed",
                        suggestion=suggestion,
                    )
                )
            data = {"selection": selection, "installation": status}
            if status["state"] == "installed":
                debugger_name = manifest["tool_prefix"] + "gdb"
                if sys.platform == "win32":
                    debugger_name += ".exe"
                debugger = Path(status["active_root"]) / "bin" / debugger_name
                if debugger.is_file():
                    status["debugger"] = str(debugger.resolve())
            if args.output_format == "json":
                emit_json(envelope(command, "ok", data, diagnostics))
            else:
                console = ConsoleProgress()
                _print_detection(selection, console)
                _print_status(status, console)
                for item in diagnostics:
                    console.warning(item["message"])
            return int(ExitCode.OK)

        if args.dry_run:
            asset = selection["asset"]
            archive = (
                args.archive.expanduser().resolve()
                if args.archive
                else args.cache_dir.expanduser().resolve()
                / manifest["id"]
                / manifest["release"]
                / asset["file"]
            )
            plan = {
                "selection": selection,
                "prefix": str(prefix),
                "install_root": str(toolchain.shared_install_root(manifest, prefix, host)),
                "active_root": str(toolchain.shared_install_root(manifest, prefix, host)),
                "archive": str(archive),
                "download_required": args.archive is None and not archive.is_file(),
                "force": args.force,
                "changed": False,
                "dry_run": True,
            }
            if args.output_format == "json":
                emit_json(envelope(command, "ok", plan))
            else:
                console = ConsoleProgress()
                console.info("Dry run; no files were written and no network request was made.")
                _print_detection(selection, console)
                console.info(f"Install root: {plan['install_root']}")
                console.info(f"Active root: {plan['active_root']}")
                console.info(f"Archive: {plan['archive']}")
            return int(ExitCode.OK)

        console = ConsoleProgress(enabled=args.output_format == "text")
        result = toolchain.install_toolchain(
            manifest,
            prefix,
            host,
            args.cache_dir,
            archive_override=args.archive,
            force=args.force,
            progress=console.message,
            download_progress=console.download,
        )
        data = {"selection": selection, "installation": result}
        if args.output_format == "json":
            emit_json(envelope(command, "ok", data))
        else:
            _print_detection(selection, console)
            _print_status(result, console)
            console.info(
                "Installation completed." if result["changed"] else "Already installed."
            )
        return int(ExitCode.OK)
    except toolchain.ToolchainError as exc:
        return _toolchain_exception(command, args.output_format, exc)


def create_sdk_parser() -> argparse.ArgumentParser:
    parser = EcosArgumentParser(
        prog="ecos sdk",
        description="Register, select, and inspect ECOS SDK versions.",
    )
    parser.add_argument("--registry", type=Path, help=argparse.SUPPRESS)
    commands = parser.add_subparsers(dest="sdk_command", required=True)

    register = commands.add_parser("register", help="register an SDK checkout or installation")
    register.add_argument("path", type=Path)
    register.add_argument("--name")
    register.add_argument("--kind", choices=("checkout", "release"), default="checkout")
    register.add_argument("--activate", action="store_true")
    register.add_argument("--replace", action="store_true")

    commands.add_parser("list", help="list registered SDKs")

    current = commands.add_parser("current", help="resolve the SDK selected for a project")
    current.add_argument("--project", type=Path, default=Path.cwd())
    current.add_argument("--sdk", dest="explicit_sdk")

    use = commands.add_parser("use", help="set the globally active SDK")
    use.add_argument("selector")

    pin = commands.add_parser("pin", help="pin a registered SDK to a project")
    pin.add_argument("selector")
    pin.add_argument("--project", type=Path, default=Path.cwd())

    unregister = commands.add_parser("unregister", help="remove an SDK registration")
    unregister.add_argument("name")

    uninstall = commands.add_parser("uninstall", help="remove a verified release installation")
    uninstall.add_argument("name")
    uninstall.add_argument("--dry-run", action="store_true")

    commands.add_parser("doctor", help="validate all registered SDK paths")

    for command_parser in commands.choices.values():
        command_parser.add_argument(
            "--format", choices=("text", "json"), default="text", dest="output_format"
        )
    return parser


def _print_sdk_data(command: str, data: dict[str, Any], console: ConsoleProgress) -> None:
    if command in {"register", "use", "unregister"}:
        action = {
            "register": "Registered",
            "use": "Active SDK",
            "unregister": "Unregistered",
        }[command]
        console.info(f"{action}: {data['name']}")
        if command != "unregister":
            console.info(f"SDK: {data['entry']['sdk_id']} {data['entry']['sdk_version']}")
            console.info(f"Root: {data['entry']['root']}")
        return
    if command == "list":
        console.info(f"Registry: {data['registry']}")
        if not data["entries"]:
            console.info("No SDKs registered.")
        for entry in data["entries"]:
            marker = "*" if entry["active"] else " "
            console.info(
                f"{marker} {entry['name']}: {entry['sdk_version']} "
                f"({entry['kind']}) {entry['root']}"
            )
        return
    if command == "current":
        console.info(f"SDK: {data['sdk_id']} {data['sdk_version']}")
        console.info(f"Root: {data['root']}")
        console.info(f"Selected by: {data['source']}")
        return
    if command == "pin":
        console.info(f"Project SDK pinned: {data['pin']['registration']}")
        console.info(f"Pin file: {data['path']}")
        return
    if command == "uninstall":
        action = "Would remove" if data["dry_run"] else "Removed"
        console.info(f"{action} SDK release: {data['root']}")
        console.info(f"Registration: {data['name']}")
        return
    if command == "doctor":
        console.info(f"Registry: {data['registry']}")
        for entry in data["entries"]:
            console.info(f"{entry['name']}: {entry['state']} ({entry['root']})")
            if entry["error"]:
                console.warning(entry["error"])
        console.info("Registry state: valid" if data["valid"] else "Registry state: invalid")


def run_sdk(argv: Sequence[str], *, global_sdk: Optional[str] = None) -> int:
    parser = create_sdk_parser()
    raw_arguments = list(argv)
    requested_format = "json" if "--format" in raw_arguments and "json" in raw_arguments else "text"
    requested_command = next((value for value in raw_arguments if not value.startswith("-")), "unknown")
    try:
        args = parser.parse_args(raw_arguments)
    except UsageError as exc:
        return emit_error(
            f"sdk.{requested_command}",
            requested_format,
            "ECOS_USAGE_INVALID_ARGUMENTS",
            str(exc),
            ExitCode.USAGE,
            suggestion="Run 'ecos sdk --help' for supported arguments.",
        )

    command = f"sdk.{args.sdk_command}"
    registry = SdkRegistry(args.registry)
    try:
        if args.sdk_command == "register":
            data = registry.register(
                args.path,
                name=args.name,
                kind=args.kind,
                activate=args.activate,
                replace=args.replace,
            )
        elif args.sdk_command == "list":
            state = registry.load()
            data = {
                "registry": str(registry.path),
                "active": state["active"],
                "entries": [
                    {"name": name, "active": state["active"] == name, **entry}
                    for name, entry in state["sdks"].items()
                ],
            }
        elif args.sdk_command == "current":
            context = SdkResolver(
                registry,
                checkout_hint=Path(__file__).resolve().parents[4],
            ).resolve(explicit=args.explicit_sdk or global_sdk, project=args.project)
            data = context.as_dict()
        elif args.sdk_command == "use":
            data = registry.use(args.selector)
        elif args.sdk_command == "pin":
            name, entry = registry.find(args.selector)
            context = sdk_manifest.context_from_root(
                Path(entry["root"]),
                kind=entry["kind"],
                source="pin-command",
                registration_name=name,
            )
            data = write_project_pin(args.project, context)
        elif args.sdk_command == "unregister":
            data = registry.unregister(args.name)
        elif args.sdk_command == "uninstall":
            data = installer.uninstall_sdk_release(
                registry, args.name, dry_run=args.dry_run
            )
        else:
            data = registry.doctor()

        diagnostics: list[dict[str, Any]] = []
        if args.sdk_command == "doctor" and not data["valid"]:
            diagnostics.append(
                diagnostic(
                    "ECOS_SDK_REGISTRY_INVALID",
                    "warning",
                    "one or more registered SDK paths are invalid",
                    suggestion="Repair the path or unregister the stale entry.",
                )
            )
        if args.output_format == "json":
            emit_json(envelope(command, "ok", data, diagnostics))
        else:
            console = ConsoleProgress()
            _print_sdk_data(args.sdk_command, data, console)
        return int(ExitCode.OK)
    except (
        SdkRegistryError,
        sdk_manifest.SdkManifestError,
        SdkResolutionError,
        installer.InstallerError,
    ) as exc:
        return emit_error(
            command,
            args.output_format,
            "ECOS_SDK_CONFIG_INVALID",
            str(exc),
            ExitCode.CONFIG,
        )


def run_completion(argv: Sequence[str]) -> int:
    parser = EcosArgumentParser(
        prog="ecos completion",
        description="Generate shell completion for ECOS.",
    )
    parser.add_argument("shell", choices=completion.SUPPORTED_SHELLS)
    try:
        args = parser.parse_args(list(argv))
    except UsageError as exc:
        return emit_error(
            "completion",
            "text",
            "ECOS_USAGE_INVALID_ARGUMENTS",
            str(exc),
            ExitCode.USAGE,
            suggestion="Choose bash, zsh, fish, or powershell.",
        )
    print(completion.generate(args.shell), end="")
    return int(ExitCode.OK)


def _project_error(
    command: str, output_format: str, exc: Exception
) -> int:
    if isinstance(exc, project.ProjectNotFound):
        code = "ECOS_PROJECT_NOT_FOUND"
    elif isinstance(exc, project.ProjectMetadataError):
        code = "ECOS_PROJECT_METADATA_INVALID"
    elif isinstance(exc, (project.BoardNotFound, project.BoardManifestError)):
        code = "ECOS_PROJECT_BOARD_INVALID"
    elif isinstance(exc, project.TargetNotFound):
        code = "ECOS_PROJECT_TARGET_NOT_FOUND"
    elif isinstance(exc, project_model.CapabilityMismatchError):
        code = "ECOS_PROJECT_CAPABILITY_MISMATCH"
    elif isinstance(exc, project_model.ComponentResolutionError):
        code = "ECOS_PROJECT_COMPONENT_INVALID"
    elif isinstance(exc, project_model.ToolchainResolutionError):
        code = "ECOS_PROJECT_TOOLCHAIN_INVALID"
    elif isinstance(exc, configuration.ConfigurationDependencyError):
        code = "ECOS_CONFIG_DEPENDENCY_MISSING"
    elif isinstance(exc, configuration.ConfigurationTerminalError):
        code = "ECOS_CONFIG_TERMINAL_REQUIRED"
    else:
        code = "ECOS_PROJECT_MODEL_INVALID"
    return emit_error(command, output_format, code, str(exc), ExitCode.CONFIG)


def run_resource(
    argv: Sequence[str], sdk_context: SdkContext, resource: str
) -> int:
    parser = EcosArgumentParser(
        prog=f"ecos {resource}",
        description=f"Inspect SDK {resource} manifests and capabilities.",
    )
    commands = parser.add_subparsers(dest="resource_command", required=True)
    listing = commands.add_parser("list", help=f"list SDK {resource}s")
    description = commands.add_parser("describe", help=f"describe one SDK {resource}")
    description.add_argument("id", metavar="ID")
    for command in (listing, description):
        command.add_argument(
            "--format", choices=("text", "json"), default="text", dest="output_format"
        )
    try:
        args = parser.parse_args(list(argv))
    except UsageError as exc:
        return emit_error(
            resource, _requested_output_format(argv), "ECOS_USAGE_INVALID_ARGUMENTS",
            str(exc), ExitCode.USAGE,
        )

    command_name = f"{resource}.{args.resource_command}"
    operations = {
        "board": (project_model.list_boards, project_model.describe_board, "boards"),
        "target": (project_model.list_targets, project_model.describe_target, "targets"),
        "component": (
            project_model.list_components, project_model.describe_component, "components"
        ),
        "driver": (project_model.list_drivers, project_model.describe_driver, "drivers"),
        "device": (project_model.list_devices, project_model.describe_device, "devices"),
    }
    list_items, describe_item, collection_name = operations[resource]
    try:
        data = (
            list_items(sdk_context)
            if args.resource_command == "list"
            else describe_item(sdk_context, args.id)
        )
    except (project.ProjectError, project_model.ProjectModelError) as exc:
        return _project_error(command_name, args.output_format, exc)

    if args.output_format == "json":
        emit_json(envelope(command_name, "ok", data))
    elif args.resource_command == "list":
        console = ConsoleProgress()
        for item in data[collection_name]:
            console.info(f"{item['id']}  {item['name']}")
    else:
        print(json.dumps(data, indent=2, ensure_ascii=False))
    return int(ExitCode.OK)


def run_schema(argv: Sequence[str]) -> int:
    parser = EcosArgumentParser(
        prog="ecos schema", description="Inspect and validate packaged manifest schemas."
    )
    commands = parser.add_subparsers(dest="schema_command", required=True)
    listing = commands.add_parser("list", help="list available manifest schemas")
    description = commands.add_parser("describe", help="show one JSON Schema")
    description.add_argument("kind", choices=tuple(schema_contract.SCHEMA_FILES))
    validation = commands.add_parser("validate", help="validate one YAML or JSON manifest")
    validation.add_argument("kind", choices=tuple(schema_contract.SCHEMA_FILES))
    validation.add_argument("path", type=Path, metavar="PATH")
    for command in (listing, description, validation):
        command.add_argument(
            "--format", choices=("text", "json"), default="text", dest="output_format"
        )
    try:
        args = parser.parse_args(list(argv))
    except UsageError as exc:
        return emit_error(
            "schema", _requested_output_format(argv), "ECOS_USAGE_INVALID_ARGUMENTS",
            str(exc), ExitCode.USAGE,
        )

    command_name = f"schema.{args.schema_command}"
    try:
        if args.schema_command == "list":
            data = schema_contract.list_schemas()
        elif args.schema_command == "describe":
            data = schema_contract.describe_schema(args.kind)
        else:
            data = schema_contract.validate_file(args.kind, args.path)
    except schema_contract.SchemaValidationError as exc:
        item = diagnostic(
            "ECOS_SCHEMA_INVALID", "error", str(exc), resource=exc.kind,
            field=exc.field, actual=exc.actual, expected=exc.expected,
            suggestion="Check the manifest against 'ecos schema describe KIND'.",
        )
        if args.output_format == "json":
            emit_json(envelope(command_name, "error", None, [item]))
        else:
            ConsoleProgress().error(f"[{item['code']}] {item['message']}")
        return int(ExitCode.CONFIG)

    if args.output_format == "json":
        emit_json(envelope(command_name, "ok", data))
    elif args.schema_command == "list":
        for item in data["schemas"]:
            print(f"{item['kind']} v{item['version']}  {item['manifest']}")
    else:
        print(json.dumps(data, indent=2, ensure_ascii=False))
    return int(ExitCode.OK)


def run_validate(argv: Sequence[str], sdk_context: SdkContext) -> int:
    parser = EcosArgumentParser(
        prog="ecos validate",
        description="Validate and resolve an ECOS project without writing files.",
    )
    parser.add_argument("--project", type=Path, default=Path.cwd(), metavar="DIRECTORY")
    parser.add_argument(
        "--format", choices=("text", "json"), default="text", dest="output_format"
    )
    try:
        args = parser.parse_args(list(argv))
    except UsageError as exc:
        return emit_error(
            "validate",
            _requested_output_format(argv),
            "ECOS_USAGE_INVALID_ARGUMENTS",
            str(exc),
            ExitCode.USAGE,
        )
    try:
        data = configuration.validate_project(sdk_context, project_root=args.project)
        if args.output_format == "json":
            emit_json(envelope("validate", "ok", data))
        else:
            console = ConsoleProgress()
            console.info(f"Project is valid: {data['path']}")
            console.info(f"Board: {data['board'] or 'none'}")
            console.info(f"Target: {data['target']}")
            console.info(f"Fingerprint: {data['source_fingerprint']}")
        return int(ExitCode.OK)
    except (
        project.ProjectError,
        project_model.ProjectModelError,
        configuration.ConfigurationError,
    ) as exc:
        return _project_error("validate", args.output_format, exc)


def run_configure(argv: Sequence[str], sdk_context: SdkContext) -> int:
    parser = EcosArgumentParser(
        prog="ecos configure",
        description="Generate resolved CMake and Kconfig inputs for an ECOS project.",
    )
    parser.add_argument("--project", type=Path, default=Path.cwd(), metavar="DIRECTORY")
    parser.add_argument("--dry-run", action="store_true", help="resolve without writing files")
    parser.add_argument(
        "--format", choices=("text", "json"), default="text", dest="output_format"
    )
    try:
        args = parser.parse_args(list(argv))
    except UsageError as exc:
        return emit_error(
            "configure",
            _requested_output_format(argv),
            "ECOS_USAGE_INVALID_ARGUMENTS",
            str(exc),
            ExitCode.USAGE,
        )
    try:
        result = configuration.configure_project(
            sdk_context, project_root=args.project, dry_run=args.dry_run
        )
        data = result.data
        if args.output_format == "json":
            emit_json(envelope("configure", "ok", data))
        else:
            console = ConsoleProgress()
            action = "would update" if args.dry_run and data["changed"] else (
                "updated" if data["changed"] else "already current"
            )
            console.info(f"Project configuration {action}: {data['path']}")
            console.info(f"Generated directory: {data['generated']['directory']}")
            console.info(f"Fingerprint: {data['configuration_fingerprint']}")
        return int(ExitCode.OK)
    except (
        project.ProjectError,
        project_model.ProjectModelError,
        configuration.ConfigurationError,
    ) as exc:
        return _project_error("configure", args.output_format, exc)


def run_menuconfig(argv: Sequence[str], sdk_context: SdkContext) -> int:
    parser = EcosArgumentParser(
        prog="ecos menuconfig",
        description="Interactively configure an ECOS project.",
    )
    parser.add_argument("--project", type=Path, default=Path.cwd(), metavar="DIRECTORY")
    try:
        args = parser.parse_args(list(argv))
    except UsageError as exc:
        return emit_error(
            "menuconfig",
            "text",
            "ECOS_USAGE_INVALID_ARGUMENTS",
            str(exc),
            ExitCode.USAGE,
        )
    try:
        result = configuration.menuconfig_project(
            sdk_context, project_root=args.project
        )
        data = result.data
        console = ConsoleProgress()
        project_config = Path(data["path"]) / configuration.PROJECT_CONFIG_FILE
        if project_config.is_file():
            console.info(f"User configuration: {project_config}")
        console.info(f"Generated directory: {data['generated']['directory']}")
        console.info(f"Fingerprint: {data['configuration_fingerprint']}")
        return int(ExitCode.OK)
    except (
        project.ProjectError,
        project_model.ProjectModelError,
        configuration.ConfigurationError,
    ) as exc:
        return _project_error("menuconfig", "text", exc)


def run_clean(argv: Sequence[str], sdk_context: SdkContext) -> int:
    parser = EcosArgumentParser(
        prog="ecos clean",
        description="Remove recognized CMake build outputs from an ECOS project.",
    )
    parser.add_argument("--project", type=Path, default=Path.cwd(), metavar="DIRECTORY")
    parser.add_argument("--dry-run", action="store_true", help="show what would be removed")
    parser.add_argument(
        "--format", choices=("text", "json"), default="text", dest="output_format"
    )
    try:
        args = parser.parse_args(list(argv))
    except UsageError as exc:
        return emit_error(
            "clean", _requested_output_format(argv),
            "ECOS_USAGE_INVALID_ARGUMENTS", str(exc), ExitCode.USAGE,
        )
    try:
        data = build.clean_project(project_root=args.project, dry_run=args.dry_run)
    except project.ProjectError as exc:
        return _project_error("clean", args.output_format, exc)
    except build.BuildConfigurationError as exc:
        return emit_error(
            "clean", args.output_format,
            "ECOS_CLEAN_UNRECOGNIZED_OUTPUT", str(exc), ExitCode.CONFIG,
        )
    except build.BuildOutputError as exc:
        return emit_error(
            "clean", args.output_format,
            "ECOS_CLEAN_FAILED", str(exc), ExitCode.EXTERNAL_TOOL,
        )
    if args.output_format == "json":
        emit_json(envelope("clean", "ok", data))
    else:
        console = ConsoleProgress()
        if not data["paths"]:
            console.info(f"No build outputs found: {data['path']}")
        elif args.dry_run:
            console.info(f"Would remove: {data['paths'][0]}")
        else:
            console.info(f"Build outputs removed: {data['paths'][0]}")
    return int(ExitCode.OK)


def run_doctor(argv: Sequence[str], sdk_context: SdkContext) -> int:
    parser = EcosArgumentParser(
        prog="ecos doctor",
        description="Inspect project, build tools, toolchain, and Board connections.",
    )
    parser.add_argument("--project", type=Path, default=Path.cwd(), metavar="DIRECTORY")
    parser.add_argument(
        "--format", choices=("text", "json"), default="text", dest="output_format"
    )
    try:
        args = parser.parse_args(list(argv))
    except UsageError as exc:
        return emit_error(
            "doctor", _requested_output_format(argv),
            "ECOS_USAGE_INVALID_ARGUMENTS", str(exc), ExitCode.USAGE,
        )
    data = doctor.inspect_environment(sdk_context, project_root=args.project)
    if args.output_format == "json":
        emit_json(envelope(
            "doctor", "ok" if data["valid"] else "error",
            {key: value for key, value in data.items() if key != "diagnostics"},
            data["diagnostics"],
        ))
    else:
        console = ConsoleProgress()
        for name, check in data["checks"].items():
            if name == "host_tools":
                for tool_name, item in check.items():
                    console.info(f"{tool_name}: {item['state']}")
            else:
                console.info(f"{name}: {check['state']}")
        for item in data["diagnostics"]:
            if item["severity"] == "error":
                console.error(f"[{item['code']}] {item['message']}")
            else:
                console.warning(f"[{item['code']}] {item['message']}")
    return int(ExitCode.OK if data["valid"] else ExitCode.CONFIG)


def run_build(argv: Sequence[str], sdk_context: SdkContext) -> int:
    parser = EcosArgumentParser(
        prog="ecos build",
        description="Build an ECOS project with its selected Target/SoC.",
    )
    parser.add_argument(
        "--project",
        type=Path,
        default=Path.cwd(),
        metavar="DIRECTORY",
        help="project directory (default: current directory)",
    )
    parser.add_argument("--clean", action="store_true", help="remove build outputs")
    parser.add_argument(
        "--format", choices=("text", "json"), default="text", dest="output_format"
    )
    try:
        args = parser.parse_args(list(argv))
    except UsageError as exc:
        return emit_error(
            "build",
            _requested_output_format(argv),
            "ECOS_USAGE_INVALID_ARGUMENTS",
            str(exc),
            ExitCode.USAGE,
        )
    try:
        data = build.build_project(
            sdk_context, project_root=args.project, clean=args.clean,
            json_output=args.output_format == "json",
        )
        if args.output_format == "json":
            emit_json(envelope("build", "ok", data))
        else:
            console = ConsoleProgress()
            if data["clean"]:
                if data["removed"]:
                    console.info(f"Build outputs removed: {data['paths'][0]}")
                else:
                    console.info(f"No build outputs found: {data['path']}")
            else:
                console.info(f"Project built: {data['path']}")
                console.info(f"Target: {data['target']}")
                for output in data["outputs"].values():
                    console.info(f"Output: {output}")
        return int(ExitCode.OK)
    except project.ProjectNotFound as exc:
        return emit_error(
            "build",
            args.output_format,
            "ECOS_PROJECT_NOT_FOUND",
            str(exc),
            ExitCode.CONFIG,
        )
    except project.ProjectMetadataError as exc:
        return emit_error(
            "build",
            args.output_format,
            "ECOS_PROJECT_METADATA_INVALID",
            str(exc),
            ExitCode.CONFIG,
        )
    except (project.BoardNotFound, project.BoardManifestError) as exc:
        return emit_error(
            "build",
            args.output_format,
            "ECOS_PROJECT_BOARD_INVALID",
            str(exc),
            ExitCode.CONFIG,
        )
    except project.TargetNotFound as exc:
        return emit_error(
            "build",
            args.output_format,
            "ECOS_PROJECT_TARGET_NOT_FOUND",
            str(exc),
            ExitCode.CONFIG,
        )
    except build.BuildConfigurationError as exc:
        return emit_error(
            "build",
            args.output_format,
            "ECOS_BUILD_CONFIG_INVALID",
            str(exc),
            ExitCode.CONFIG,
        )
    except build.BuildToolNotFound as exc:
        return emit_error(
            "build",
            args.output_format,
            "ECOS_BUILD_TOOL_NOT_FOUND",
            str(exc),
            ExitCode.EXTERNAL_TOOL,
        )
    except build.BuildCommandError as exc:
        return emit_error(
            "build",
            args.output_format,
            "ECOS_BUILD_FAILED",
            str(exc),
            ExitCode.EXTERNAL_TOOL,
        )
    except build.BuildOutputError as exc:
        return emit_error(
            "build",
            args.output_format,
            "ECOS_BUILD_OUTPUT_INVALID",
            str(exc),
            ExitCode.EXTERNAL_TOOL,
        )


def run_flash(argv: Sequence[str], sdk_context: SdkContext) -> int:
    parser = EcosArgumentParser(
        prog="ecos flash",
        description="Flash the current firmware using the selected Board provider.",
    )
    parser.add_argument("--project", type=Path, default=Path.cwd(), metavar="DIRECTORY")
    parser.add_argument(
        "--device",
        dest="device",
        type=Path,
        metavar="DIRECTORY",
        help="programmer mount directory (auto-detected by volume label by default)",
    )
    parser.add_argument(
        "--format", choices=("text", "json"), default="text", dest="output_format"
    )
    try:
        args = parser.parse_args(list(argv))
    except UsageError as exc:
        return emit_error(
            "flash",
            _requested_output_format(argv),
            "ECOS_USAGE_INVALID_ARGUMENTS",
            str(exc),
            ExitCode.USAGE,
        )
    try:
        data = flashing.flash_project(
            sdk_context, project_root=args.project, device=args.device
        )
        if args.output_format == "json":
            emit_json(envelope("flash", "ok", data))
        else:
            console = ConsoleProgress()
            console.info(f"Firmware flashed: {data['artifact']}")
            console.info(f"Device: {data['device']}")
            console.info(f"SHA-256: {data['sha256']}")
        return int(ExitCode.OK)
    except (project.ProjectError, project_model.ProjectModelError) as exc:
        return _project_error("flash", args.output_format, exc)
    except flashing.FlashConfigurationError as exc:
        return emit_error(
            "flash",
            args.output_format,
            "ECOS_FLASH_CONFIG_INVALID",
            str(exc),
            ExitCode.CONFIG,
        )
    except flashing.FlashArtifactError as exc:
        return emit_error(
            "flash",
            args.output_format,
            "ECOS_FLASH_ARTIFACT_INVALID",
            str(exc),
            ExitCode.CONFIG,
        )
    except flashing.FlashDeviceNotFound as exc:
        return emit_error(
            "flash",
            args.output_format,
            "ECOS_FLASH_DEVICE_NOT_FOUND",
            str(exc),
            ExitCode.EXTERNAL_TOOL,
        )
    except flashing.FlashWriteError as exc:
        return emit_error(
            "flash",
            args.output_format,
            "ECOS_FLASH_FAILED",
            str(exc),
            ExitCode.EXTERNAL_TOOL,
        )


def run_monitor(argv: Sequence[str], sdk_context: SdkContext) -> int:
    parser = EcosArgumentParser(
        prog="ecos monitor",
        description="Monitor the selected Board console through a serial port.",
    )
    parser.add_argument("--project", type=Path, default=Path.cwd(), metavar="DIRECTORY")
    parser.add_argument("--port", help="serial port (auto-detected by default)")
    parser.add_argument("--baudrate", type=int, help="override the Board baudrate")
    parser.add_argument("--timeout", type=float, help="stop after this many seconds")
    parser.add_argument("--expect", help="succeed after observing this UTF-8 text")
    parser.add_argument(
        "--format", choices=("text", "json"), default="text", dest="output_format"
    )
    try:
        args = parser.parse_args(list(argv))
    except UsageError as exc:
        return emit_error(
            "monitor",
            _requested_output_format(argv),
            "ECOS_USAGE_INVALID_ARGUMENTS",
            str(exc),
            ExitCode.USAGE,
        )
    if args.output_format == "json" and args.timeout is None and args.expect is None:
        return emit_error(
            "monitor",
            args.output_format,
            "ECOS_USAGE_INVALID_ARGUMENTS",
            "JSON monitoring requires --timeout or --expect so a final result is produced",
            ExitCode.USAGE,
        )
    timeout = 10.0 if args.expect is not None and args.timeout is None else args.timeout
    try:
        if args.output_format == "text":
            ConsoleProgress().info(
                f"Opening serial monitor{f' on {args.port}' if args.port else ''}"
            )
        data = monitoring.monitor_project(
            sdk_context,
            project_root=args.project,
            port=args.port,
            baudrate=args.baudrate,
            timeout=timeout,
            expect=args.expect,
            stream=None if args.output_format == "json" else sys.stdout,
        )
        if args.output_format == "json":
            emit_json(envelope("monitor", "ok", data))
        else:
            console = ConsoleProgress()
            console.info(f"Serial monitor closed: {data['port']}")
            if args.expect is not None:
                console.info(f"Expected output observed: {args.expect}")
        return int(ExitCode.OK)
    except (project.ProjectError, project_model.ProjectModelError) as exc:
        return _project_error("monitor", args.output_format, exc)
    except monitoring.MonitorConfigurationError as exc:
        return emit_error(
            "monitor",
            args.output_format,
            "ECOS_MONITOR_CONFIG_INVALID",
            str(exc),
            ExitCode.CONFIG,
        )
    except monitoring.MonitorDependencyError as exc:
        return emit_error(
            "monitor",
            args.output_format,
            "ECOS_MONITOR_DEPENDENCY_MISSING",
            str(exc),
            ExitCode.EXTERNAL_TOOL,
        )
    except monitoring.MonitorPortError as exc:
        return emit_error(
            "monitor",
            args.output_format,
            "ECOS_MONITOR_PORT_FAILED",
            str(exc),
            ExitCode.EXTERNAL_TOOL,
        )
    except monitoring.MonitorExpectationError as exc:
        return emit_error(
            "monitor",
            args.output_format,
            "ECOS_MONITOR_EXPECTATION_FAILED",
            str(exc),
            ExitCode.EXTERNAL_TOOL,
        )


def create_project_parser() -> argparse.ArgumentParser:
    parser = EcosArgumentParser(
        prog="ecos project",
        description="Create and manage ECOS projects.",
    )
    commands = parser.add_subparsers(dest="project_command", required=True)
    listing = commands.add_parser(
        "list", help="list SDK examples and compatible Boards"
    )
    listing.add_argument(
        "--format",
        choices=("text", "json"),
        default="text",
        dest="output_format",
        help="output format",
    )
    info = commands.add_parser("info", help="inspect the selected project")
    info.add_argument("--project", type=Path, default=Path.cwd())
    info.add_argument(
        "--format", choices=("text", "json"), default="text", dest="output_format"
    )

    create = commands.add_parser("create", help="create a project from an SDK example")
    create.add_argument("example", metavar="EXAMPLE", help="SDK Example name")
    create.add_argument(
        "--name", help="project directory and CMake project name"
    )
    create.add_argument(
        "--path",
        type=Path,
        default=Path.cwd(),
        metavar="DIRECTORY",
        help="project parent directory (default: current directory)",
    )
    initial_hardware = create.add_mutually_exclusive_group()
    initial_hardware.add_argument("--board", help="initial Board ID")
    initial_hardware.add_argument("--target", help="initial Target/SoC ID")
    create.add_argument("--profile", help="initial configuration profile")
    create.add_argument(
        "--dry-run", action="store_true", help="show the plan without writing files"
    )
    create.add_argument(
        "--force", action="store_true", help="replace an existing project path"
    )
    create.add_argument(
        "--format",
        choices=("text", "json"),
        default="text",
        dest="output_format",
        help="output format",
    )

    for command_name, argument_name, help_text in (
        ("set-board", "board", "select a Board and its mapped Target"),
        ("set-target", "target", "select a Target/SoC and clear the Board"),
        ("set-profile", "profile", "select a project configuration profile"),
    ):
        selection = commands.add_parser(command_name, help=help_text)
        selection.add_argument(argument_name, metavar=argument_name.upper())
        selection.add_argument("--dry-run", action="store_true", help="validate without writing project metadata")
        selection.add_argument(
            "--project",
            type=Path,
            default=Path.cwd(),
            metavar="DIRECTORY",
            help="project directory (default: current directory)",
        )
        selection.add_argument(
            "--format",
            choices=("text", "json"),
            default="text",
            dest="output_format",
            help="output format",
        )
    return parser


def _project_info(sdk_context: SdkContext, root: Path) -> dict[str, Any]:
    root = root.expanduser().resolve()
    resolved = project_model.resolve_project(sdk_context, project_root=root)
    source_fingerprint = resolved["source_fingerprint"]

    def read_json(path: Path) -> Optional[dict[str, Any]]:
        if not path.is_file():
            return None
        try:
            value = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, ValueError) as exc:
            raise project.ProjectMetadataError(f"cannot read {path}: {exc}") from exc
        if not isinstance(value, dict):
            raise project.ProjectMetadataError(f"expected JSON object in {path}")
        return value

    generated_root = root / ".ecos" / "generated"
    generated = read_json(generated_root / "resolved-project.json")
    configured = (
        generated is not None
        and generated.get("source_fingerprint") == source_fingerprint
    )
    artifact_data = read_json(root / "build" / "artifacts.json")
    fingerprint_path = generated_root / "configuration.fingerprint"
    configuration_fingerprint = (
        fingerprint_path.read_text(encoding="utf-8").strip()
        if fingerprint_path.is_file() else None
    )
    built = bool(
        configured
        and artifact_data is not None
        and artifact_data.get("source_fingerprint") == source_fingerprint
        and artifact_data.get("configuration_fingerprint") == configuration_fingerprint
    )
    outputs: dict[str, str] = {}
    if built:
        try:
            verified = artifacts.load_manifest(root)
            outputs["elf"] = str((root / verified["files"]["elf"]["path"]).resolve())
        except artifacts.ArtifactError:
            built = False
    selected = resolved["project"]
    return {
        "path": str(root),
        "name": selected["name"],
        "example": selected["example"],
        "sdk": {key: resolved["sdk"][key] for key in ("id", "version", "kind")},
        "board": selected["board"],
        "target": selected["target"],
        "profile": selected["profile"],
        "configured": configured,
        "built": built,
        "outputs": outputs,
    }


def _print_project_data(data: dict[str, Any], console: ConsoleProgress) -> None:
    if data["dry_run"]:
        console.info("Dry run; no files were written.")
    else:
        console.info(f"Project created: {data['path']}")
    console.info(f"Example: {data['example']}")
    console.info(f"Project name: {data['name']}")
    if data["board"]:
        console.info(f"Board: {data['board']}")
    if data["target"]:
        console.info(f"Target: {data['target']}")
    if data["profile"]:
        console.info(f"Profile: {data['profile']}")
    console.info(f"Metadata: {data['metadata']}")
    if data["replaced"]:
        message = (
            "Existing project path will be replaced."
            if data["dry_run"]
            else "Existing project path was replaced."
        )
        console.warning(message)


def _print_project_list(data: dict[str, Any], console: ConsoleProgress) -> None:
    examples = data["examples"]
    sdk = data["sdk"]
    console.info(f"SDK Examples: {sdk['id']} {sdk['version']}")
    if not examples:
        console.info("No project examples are available.")
        return

    example_width = max(len("EXAMPLE"), *(len(item["name"]) for item in examples))
    board_values = [
        ", ".join(item["supported_boards"]) or "none" for item in examples
    ]
    board_width = max(len("BOARDS"), *(len(value) for value in board_values))
    console.info(
        f"{'EXAMPLE':<{example_width}}  "
        f"{'BOARDS':<{board_width}}  REQUIRES"
    )
    for item, boards in zip(examples, board_values):
        requirements = ", ".join(item["requires"]) or "none"
        console.info(
            f"{item['name']:<{example_width}}  "
            f"{boards:<{board_width}}  {requirements}"
        )


def _print_project_selection(data: dict[str, Any], console: ConsoleProgress) -> None:
    console.info(f"Project {'would update' if data['dry_run'] else 'updated'}: {data['path']}")
    console.info(f"Board: {data['board'] or 'none'}")
    console.info(f"Target: {data['target']}")


def run_project(argv: Sequence[str], sdk_context: SdkContext) -> int:
    parser = create_project_parser()
    raw_arguments = list(argv)
    requested_format = _requested_output_format(raw_arguments)
    requested_command = raw_arguments[0] if raw_arguments else "unknown"
    try:
        args = parser.parse_args(raw_arguments)
    except UsageError as exc:
        return emit_error(
            f"project.{requested_command}",
            requested_format,
            "ECOS_USAGE_INVALID_ARGUMENTS",
            str(exc),
            ExitCode.USAGE,
            suggestion="Run 'ecos project --help' for supported arguments.",
        )

    command = f"project.{args.project_command}"
    try:
        if args.project_command == "list":
            data = project_model.list_examples(sdk_context)
        elif args.project_command == "info":
            data = _project_info(sdk_context, args.project)
        elif args.project_command == "create":
            example_name = project.normalize_example_name(args.example)
            source = project.resolve_example(sdk_context, example_name)
            project_model.validate_example_source(
                sdk_context, source, example_name
            )
            validator = None
            plan_validator = None
            if args.board is not None or args.target is not None:
                validator = lambda root: project_model.resolve_project(
                    sdk_context, project_root=root
                )
                plan_validator = lambda source, metadata: project_model.resolve_project(
                    sdk_context, example_source=source, metadata_override=metadata
                )
            data = project.create_project(
                sdk_context,
                args.example,
                name=args.name,
                parent=args.path,
                board=args.board,
                target=args.target,
                profile=args.profile,
                dry_run=args.dry_run,
                force=args.force,
                validator=validator,
                plan_validator=plan_validator,
            )
        elif args.project_command == "set-board":
            data = project.set_board(
                sdk_context,
                args.board,
                project_root=args.project,
                dry_run=args.dry_run,
                validator=lambda metadata: project_model.resolve_project(
                    sdk_context,
                    project_root=args.project,
                    metadata_override=metadata,
                ),
            )
        elif args.project_command == "set-target":
            data = project.set_target(
                sdk_context,
                args.target,
                project_root=args.project,
                dry_run=args.dry_run,
                validator=lambda metadata: project_model.resolve_project(
                    sdk_context,
                    project_root=args.project,
                    metadata_override=metadata,
                ),
            )
        elif args.project_command == "set-profile":
            data = project.set_profile(
                sdk_context,
                args.profile,
                project_root=args.project,
                dry_run=args.dry_run,
                validator=lambda metadata: project_model.resolve_project(
                    sdk_context,
                    project_root=args.project,
                    metadata_override=metadata,
                ),
            )
        if args.output_format == "json":
            emit_json(envelope(command, "ok", data))
        elif args.project_command == "list":
            _print_project_list(data, ConsoleProgress())
        elif args.project_command == "create":
            _print_project_data(data, ConsoleProgress())
        elif args.project_command == "info":
            console = ConsoleProgress()
            for key in ("path", "name", "example", "board", "target", "profile"):
                console.info(f"{key}: {data[key]}")
            console.info(f"SDK: {data['sdk']['id']} {data['sdk']['version']}")
            console.info(f"Configured: {data['configured']}; built: {data['built']}")
        elif args.project_command == "set-profile":
            console = ConsoleProgress()
            console.info(f"Project updated: {data['path']}")
            console.info(f"Profile: {data['profile']}")
        else:
            _print_project_selection(data, ConsoleProgress())
        return int(ExitCode.OK)
    except project.InvalidProjectArgument as exc:
        return emit_error(
            command,
            args.output_format,
            "ECOS_PROJECT_INVALID_ARGUMENT",
            str(exc),
            ExitCode.USAGE,
        )
    except project.ExampleNotFound as exc:
        return emit_error(
            command,
            args.output_format,
            "ECOS_PROJECT_EXAMPLE_NOT_FOUND",
            str(exc),
            ExitCode.CONFIG,
            suggestion=(
                "Choose an Example name declared by the selected SDK."
            ),
        )
    except project.ExampleAmbiguous as exc:
        return emit_error(
            command,
            args.output_format,
            "ECOS_PROJECT_EXAMPLE_AMBIGUOUS",
            str(exc),
            ExitCode.CONFIG,
            suggestion="Give every SDK Example a globally unique manifest name.",
        )
    except project.BoardNotFound as exc:
        return emit_error(
            command,
            args.output_format,
            "ECOS_PROJECT_BOARD_NOT_FOUND",
            str(exc),
            ExitCode.CONFIG,
        )
    except project.BoardManifestError as exc:
        return emit_error(
            command,
            args.output_format,
            "ECOS_PROJECT_BOARD_INVALID",
            str(exc),
            ExitCode.CONFIG,
        )
    except project.TargetNotFound as exc:
        return emit_error(
            command,
            args.output_format,
            "ECOS_PROJECT_TARGET_NOT_FOUND",
            str(exc),
            ExitCode.CONFIG,
        )
    except project.ProjectNotFound as exc:
        return emit_error(
            command,
            args.output_format,
            "ECOS_PROJECT_NOT_FOUND",
            str(exc),
            ExitCode.CONFIG,
        )
    except project.ProjectMetadataError as exc:
        return emit_error(
            command,
            args.output_format,
            "ECOS_PROJECT_METADATA_INVALID",
            str(exc),
            ExitCode.CONFIG,
        )
    except project.ProjectConflict as exc:
        return emit_error(
            command,
            args.output_format,
            "ECOS_PROJECT_PATH_EXISTS",
            str(exc),
            ExitCode.CONFIG,
            suggestion="Choose another --name or pass --force to replace the path.",
        )
    except project.ProjectWriteError as exc:
        return emit_error(
            command,
            args.output_format,
            "ECOS_PROJECT_CREATE_FAILED",
            str(exc),
            ExitCode.CONFIG,
        )
    except project_model.ProjectModelError as exc:
        return _project_error(command, args.output_format, exc)


def print_help() -> None:
    print("ECOS Embedded SDK Command-Line Tool")
    print()
    print("Usage: ecos [--sdk NAME|VERSION|PATH] <command> [options]")
    print()
    print("Python commands:")
    print("  sdk register PATH                Register an SDK checkout or installation")
    print("  sdk list                         List registered SDK versions")
    print("  sdk current                      Show the SDK selected by fixed priority")
    print("  sdk use NAME                     Set the globally active SDK")
    print("  sdk pin NAME                     Pin an SDK to the current project")
    print("  sdk unregister NAME              Remove an SDK registration")
    print("  sdk uninstall NAME               Remove a verified release installation")
    print("  sdk doctor                       Validate registered SDK paths")
    print("  board list|describe              Inspect Board resources and profiles")
    print("  target list|describe             Inspect Target ISA and capabilities")
    print("  component list|describe          Inspect Component dependencies")
    print("  driver list|describe             Inspect public Driver dependencies")
    print("  device list|describe             Inspect Device Driver bus dependencies")
    print("  schema list|describe|validate    Inspect and check manifest schemas")
    print("  project list                     List SDK examples and compatible Boards")
    print("  project create EXAMPLE           Create a project from an SDK example")
    print("  project info                     Inspect the selected project state")
    print("  project set-board BOARD          Select a Board and its mapped Target")
    print("  project set-target TARGET        Select a Target and clear the Board")
    print("  project set-profile PROFILE      Select a Board configuration profile")
    print("  validate                          Validate project manifests and capabilities")
    print("  configure                         Generate CMake and Kconfig project state")
    print("  menuconfig                        Interactively configure the current project")
    print("  build                             Build the selected project Target")
    print("  clean                             Remove recognized project build outputs")
    print("  doctor                            Inspect project and host readiness")
    print("  flash                             Flash the validated firmware artifact")
    print("  monitor                           Monitor and assert Board serial output")
    print("  toolchain detect                  Detect host and select the pinned asset")
    print("  toolchain status                  Inspect the current toolchain installation")
    print("  toolchain install                 Download or import the pinned toolchain")
    print("  completion <shell>                Generate shell completion")
    print()
    print("Use 'ecos <command> --help' for command options.")


def _global_arguments(arguments: list[str]) -> tuple[Optional[str], list[str]]:
    sdk_selector: Optional[str] = None
    remaining = list(arguments)
    while remaining and (
        remaining[0] == "--sdk" or remaining[0].startswith("--sdk=")
    ):
        option = remaining.pop(0)
        if option == "--sdk":
            if not remaining:
                raise UsageError("argument --sdk: expected one argument")
            sdk_selector = remaining.pop(0)
        elif option.startswith("--sdk="):
            sdk_selector = option.split("=", 1)[1]
            if not sdk_selector:
                raise UsageError("argument --sdk: expected one argument")
    return sdk_selector, remaining


def _project_hint(arguments: Sequence[str]) -> Optional[Path]:
    values = list(arguments)
    for index, value in enumerate(values):
        if value == "--project" and index + 1 < len(values):
            return Path(values[index + 1])
        if value.startswith("--project="):
            selected = value.split("=", 1)[1]
            if selected:
                return Path(selected)
    return None


def main(argv: Optional[Sequence[str]] = None) -> int:
    arguments = list(sys.argv[1:] if argv is None else argv)
    try:
        sdk_selector, arguments = _global_arguments(arguments)
    except UsageError as exc:
        return emit_error(
            "global",
            "text",
            "ECOS_USAGE_INVALID_ARGUMENTS",
            str(exc),
            ExitCode.USAGE,
        )
    if not arguments or arguments[0] in {"help", "-h", "--help"}:
        print_help()
        return int(ExitCode.OK)
    if arguments[0] in {"-V", "--version"}:
        print(f"ecos {__version__}")
        return int(ExitCode.OK)
    command, command_args = arguments[0], arguments[1:]
    if command == "sdk":
        return run_sdk(command_args, global_sdk=sdk_selector)
    if command == "schema":
        if sdk_selector is not None:
            try:
                SdkResolver(
                    checkout_hint=Path(__file__).resolve().parents[4]
                ).resolve(explicit=sdk_selector)
            except SdkResolutionError as exc:
                return emit_error(
                    "schema", _requested_output_format(command_args),
                    "ECOS_SDK_RESOLUTION_FAILED", str(exc), ExitCode.CONFIG,
                )
        return run_schema(command_args)
    if command == "toolchain":
        try:
            context = SdkResolver(
                checkout_hint=Path(__file__).resolve().parents[4]
            ).resolve(explicit=sdk_selector)
        except SdkResolutionError as exc:
            return emit_error(
                "toolchain",
                _requested_output_format(command_args),
                "ECOS_SDK_RESOLUTION_FAILED",
                str(exc),
                ExitCode.CONFIG,
                suggestion="Run 'ecos sdk current' to inspect SDK selection.",
            )
        return run_toolchain(command_args, context)
    if command == "project":
        try:
            context = SdkResolver(
                checkout_hint=Path(__file__).resolve().parents[4]
            ).resolve(explicit=sdk_selector, project=_project_hint(command_args))
        except SdkResolutionError as exc:
            return emit_error(
                "project",
                _requested_output_format(command_args),
                "ECOS_SDK_RESOLUTION_FAILED",
                str(exc),
                ExitCode.CONFIG,
                suggestion="Run 'ecos sdk current' to inspect SDK selection.",
            )
        return run_project(command_args, context)
    if command in {"board", "target", "component", "driver", "device"}:
        try:
            context = SdkResolver(
                checkout_hint=Path(__file__).resolve().parents[4]
            ).resolve(explicit=sdk_selector)
        except SdkResolutionError as exc:
            return emit_error(
                command,
                _requested_output_format(command_args),
                "ECOS_SDK_RESOLUTION_FAILED",
                str(exc),
                ExitCode.CONFIG,
                suggestion="Run 'ecos sdk current' to inspect SDK selection.",
            )
        return run_resource(command_args, context, command)
    if command in {"validate", "configure", "menuconfig", "build", "clean", "doctor", "flash", "monitor"}:
        try:
            context = SdkResolver(
                checkout_hint=Path(__file__).resolve().parents[4]
            ).resolve(explicit=sdk_selector, project=_project_hint(command_args))
        except SdkResolutionError as exc:
            return emit_error(
                command,
                _requested_output_format(command_args),
                "ECOS_SDK_RESOLUTION_FAILED",
                str(exc),
                ExitCode.CONFIG,
                suggestion="Run 'ecos sdk current' to inspect SDK selection.",
            )
        handlers = {
            "validate": run_validate,
            "configure": run_configure,
            "menuconfig": run_menuconfig,
            "build": run_build,
            "clean": run_clean,
            "doctor": run_doctor,
            "flash": run_flash,
            "monitor": run_monitor,
        }
        return handlers[command](command_args, context)
    if command == "completion":
        return run_completion(command_args)
    return emit_error(
        command,
        "text",
        "ECOS_USAGE_UNKNOWN_COMMAND",
        f"unknown command: {command}",
        ExitCode.USAGE,
        suggestion="Run 'ecos --help' to list commands.",
    )
