"""Cross-platform installer for the ECOS Embedded SDK."""

from __future__ import annotations

import argparse
import json
import os
import shlex
import shutil
import stat
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any, Callable, Mapping, Optional, Sequence

from . import __version__
from . import completion
from . import sdk_manifest
from . import toolchain
from .progress import ConsoleProgress
from .sdk_registry import SdkRegistrationConflict, SdkRegistry, SdkRegistryError


INSTALL_SCHEMA_VERSION = "1.0"
ProgressCallback = Callable[[str], None]
SHELL_CONFIG_BEGIN = "# >>> ECOS SDK >>>"
SHELL_CONFIG_END = "# <<< ECOS SDK <<<"
COMPLETION_RELATIVE_ROOT = Path("share") / "ecos" / "completions"

SDK_DIRECTORIES = (
    "components",
    "hal",
    "templates",
    "environments",
    "third_party",
    "board",
    "example",
    "docs",
    "devices",
)

TOOL_DIRECTORIES = (
    "fixdep",
    "kconfig",
    "scripts",
    "toolchains",
    "ecos_cli",
)

PYTHON_PACKAGE_IGNORE = shutil.ignore_patterns(
    "__pycache__", "*.pyc", "*.pyo", "build", "dist", "*.egg-info"
)

INSTALLED_ECOS_LAUNCHER = '''#!/usr/bin/env python3
"""Installed ECOS Python CLI launcher."""

import sys
from pathlib import Path


SDK_ROOT = Path(__file__).resolve().parent.parent
SOURCE_ROOT = SDK_ROOT / "tools" / "ecos_cli" / "src"
if str(SOURCE_ROOT) not in sys.path:
    sys.path.insert(0, str(SOURCE_ROOT))

from ecos_cli.cli import main


if __name__ == "__main__":
    raise SystemExit(main())
'''


class InstallerError(RuntimeError):
    """The SDK installation could not be completed."""


def default_install_base(
    *,
    environ: Optional[Mapping[str, str]] = None,
    home: Optional[Path] = None,
    platform_name: Optional[str] = None,
) -> Path:
    env = os.environ if environ is None else environ
    home_root = (home or Path.home()).expanduser().resolve()
    host = platform_name or sys.platform
    if host == "win32":
        base = Path(env.get("LOCALAPPDATA", home_root / "AppData" / "Local"))
        return base / "ECOS" / "SDKs"
    if host == "darwin":
        return home_root / "Library" / "Application Support" / "ECOS" / "SDKs"
    base = Path(env.get("XDG_DATA_HOME", home_root / ".local" / "share"))
    return base / "ecos" / "sdk"


def versioned_install_path(install_base: Path, sdk_version: str) -> Path:
    base = install_base.expanduser().resolve()
    if base == Path(base.anchor):
        raise InstallerError(f"refusing to use filesystem root as install prefix: {base}")
    if not sdk_manifest.SDK_VERSION_PATTERN.fullmatch(sdk_version):
        raise InstallerError(f"invalid SDK version for installation path: {sdk_version!r}")
    return base / sdk_version


def result_envelope(
    status: str,
    data: Any,
    diagnostics: Optional[list[dict[str, Any]]] = None,
) -> dict[str, Any]:
    return {
        "cli_version": __version__,
        "schema_version": INSTALL_SCHEMA_VERSION,
        "command": "sdk.install",
        "status": status,
        "data": data,
        "diagnostics": diagnostics or [],
    }


def validate_layout(sdk_root: Path, prefix: Path) -> None:
    sdk_root = sdk_root.resolve()
    prefix = prefix.expanduser().resolve()
    filesystem_root = Path(prefix.anchor)
    if prefix == filesystem_root:
        raise InstallerError(f"refusing to install into filesystem root: {prefix}")
    if prefix == sdk_root or sdk_root in prefix.parents or prefix in sdk_root.parents:
        raise InstallerError(
            f"installation prefix must not overlap the SDK source tree: {prefix}"
        )

    try:
        sdk_manifest.validate_sdk_root(sdk_root)
    except sdk_manifest.SdkManifestError as exc:
        raise InstallerError(str(exc)) from exc

    missing: list[str] = []
    for relative in SDK_DIRECTORIES:
        if not (sdk_root / relative).is_dir():
            missing.append(relative)
    for relative in TOOL_DIRECTORIES:
        if not (sdk_root / "tools" / relative).is_dir():
            missing.append(f"tools/{relative}")
    if not (sdk_root / "tools" / "ecos.py").is_file():
        missing.append("tools/ecos.py")
    if missing:
        raise InstallerError(
            f"SDK source tree is incomplete; missing: {', '.join(sorted(missing))}"
        )


def installation_plan(sdk_root: Path, prefix: Path) -> dict[str, Any]:
    copies = [
        {"source": str(sdk_root / relative), "destination": str(prefix / relative)}
        for relative in SDK_DIRECTORIES
    ]
    copies.extend(
        {
            "source": str(sdk_root / "tools" / relative),
            "destination": str(prefix / "tools" / relative),
        }
        for relative in TOOL_DIRECTORIES
    )
    copies.append(
        {
            "source": str(sdk_root / "tools" / "ecos.py"),
            "destination": str(prefix / "tools" / "ecos.py"),
        }
    )
    copies.append(
        {
            "source": str(sdk_root / sdk_manifest.MANIFEST_RELATIVE_PATH),
            "destination": str(prefix / sdk_manifest.MANIFEST_RELATIVE_PATH),
        }
    )
    return {
        "sdk_root": str(sdk_root),
        "prefix": str(prefix),
        "copies": copies,
        "preserved": [str(prefix / "board" / "UserBSP")],
        "launchers": [str(prefix / "bin" / "ecos"), str(prefix / "bin" / "ecos.cmd")],
        "completions": [
            str(prefix / COMPLETION_RELATIVE_ROOT / filename)
            for filename in completion.COMPLETION_FILENAMES.values()
        ],
    }


def _remove_managed_path(path: Path) -> None:
    if path.is_symlink() or path.is_file():
        path.unlink()
    elif path.exists():
        shutil.rmtree(path)


def replace_tree(
    source: Path,
    destination: Path,
    *,
    ignore: Optional[Callable[[str, list[str]], set[str]]] = None,
) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(
        prefix=f".{destination.name}.install-", dir=destination.parent
    ) as temporary:
        staged = Path(temporary) / destination.name
        shutil.copytree(source, staged, symlinks=True, ignore=ignore)
        _remove_managed_path(destination)
        staged.replace(destination)


def replace_file(source: Path, destination: Path) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(
        prefix=f".{destination.name}.install-",
        dir=destination.parent,
        delete=False,
    ) as temporary:
        temporary_path = Path(temporary.name)
    try:
        shutil.copy2(source, temporary_path)
        _remove_managed_path(destination)
        temporary_path.replace(destination)
    finally:
        temporary_path.unlink(missing_ok=True)


def _copy_user_bsp(source: Path, destination: Path) -> None:
    if source.is_dir():
        shutil.copytree(source, destination, dirs_exist_ok=True, symlinks=True)


def install_sdk_core(
    sdk_root: Path, prefix: Path, progress: Optional[ProgressCallback] = None
) -> dict[str, Any]:
    sdk_root = sdk_root.resolve()
    prefix = prefix.expanduser().resolve()
    validate_layout(sdk_root, prefix)
    prefix.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory(prefix="ecos-user-bsp-") as temporary:
        user_bsp_backup = Path(temporary) / "UserBSP"
        _copy_user_bsp(prefix / "board" / "UserBSP", user_bsp_backup)

        for relative in SDK_DIRECTORIES:
            if progress:
                progress(f"Installing {relative}")
            replace_tree(sdk_root / relative, prefix / relative)

        if user_bsp_backup.is_dir():
            _copy_user_bsp(user_bsp_backup, prefix / "board" / "UserBSP")

    for relative in TOOL_DIRECTORIES:
        if progress:
            progress(f"Installing tools/{relative}")
        ignore = (
            PYTHON_PACKAGE_IGNORE
            if relative in {"ecos_cli", "toolchains"}
            else None
        )
        replace_tree(
            sdk_root / "tools" / relative,
            prefix / "tools" / relative,
            ignore=ignore,
        )

    replace_file(sdk_root / "tools" / "ecos.py", prefix / "tools" / "ecos.py")
    replace_file(
        sdk_root / sdk_manifest.MANIFEST_RELATIVE_PATH,
        prefix / sdk_manifest.MANIFEST_RELATIVE_PATH,
    )
    install_launchers(prefix)
    install_completion_files(prefix)
    return installation_plan(sdk_root, prefix)


def install_launchers(prefix: Path) -> None:
    bin_dir = prefix / "bin"
    _remove_managed_path(bin_dir)
    bin_dir.mkdir(parents=True, exist_ok=True)
    launcher = bin_dir / "ecos"
    launcher.write_text(INSTALLED_ECOS_LAUNCHER, encoding="utf-8", newline="\n")
    launcher.chmod(
        launcher.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH
    )
    python_executable = str(Path(sys.executable).resolve())
    cmd_launcher = bin_dir / "ecos.cmd"
    cmd_launcher.write_text(
        f'@echo off\r\n"{python_executable}" "%~dp0ecos" %*\r\n',
        encoding="utf-8",
        newline="",
    )


def install_completion_files(prefix: Path) -> dict[str, str]:
    destination = prefix / COMPLETION_RELATIVE_ROOT
    destination.mkdir(parents=True, exist_ok=True)
    installed: dict[str, str] = {}
    for shell, filename in completion.COMPLETION_FILENAMES.items():
        path = destination / filename
        path.write_text(completion.generate(shell), encoding="utf-8", newline="\n")
        installed[shell] = str(path)
    return installed


def detect_shell(requested: str) -> str:
    if requested != "auto":
        return requested
    if os.name == "nt":
        return "powershell"
    shell_name = Path(os.environ.get("SHELL", "bash")).name.lower()
    aliases = {
        "bash": "bash",
        "zsh": "zsh",
        "fish": "fish",
        "pwsh": "powershell",
        "powershell": "powershell",
        "powershell.exe": "powershell",
    }
    return aliases.get(shell_name, "bash")


def _powershell_profile(home: Path, query_runtime: bool) -> Path:
    if query_runtime:
        executable = shutil.which("pwsh") or shutil.which("powershell")
        if executable:
            try:
                result = subprocess.run(
                    [executable, "-NoProfile", "-Command", "$PROFILE.CurrentUserCurrentHost"],
                    check=True,
                    capture_output=True,
                    text=True,
                    timeout=5,
                )
                configured = result.stdout.strip()
                if configured:
                    return Path(configured).expanduser()
            except (OSError, subprocess.SubprocessError):
                pass
    return home / "Documents" / "PowerShell" / "Microsoft.PowerShell_profile.ps1"


def shell_config_path(shell: str, home: Optional[Path] = None) -> Optional[Path]:
    if shell == "none":
        return None
    home_root = (home or Path.home()).expanduser().resolve()
    paths = {
        "bash": home_root / ".bashrc",
        "zsh": home_root / ".zshrc",
        "fish": home_root / ".config" / "fish" / "config.fish",
    }
    if shell == "powershell":
        return _powershell_profile(home_root, query_runtime=home is None)
    try:
        return paths[shell]
    except KeyError as exc:
        raise InstallerError(f"unsupported shell: {shell}") from exc


def _fish_quote(value: str) -> str:
    return "'" + value.replace("\\", "\\\\").replace("'", "\\'") + "'"


def _powershell_quote(value: str) -> str:
    return "'" + value.replace("'", "''") + "'"


def shell_configuration_block(prefix: Path, shell: str) -> str:
    completion_file = prefix / COMPLETION_RELATIVE_ROOT / completion.COMPLETION_FILENAMES[shell]
    bin_dir = str(prefix / "bin")
    legacy_sdk_home = str(toolchain.default_prefix().expanduser().resolve())
    migrate_legacy_environment = prefix.expanduser().resolve() != Path(legacy_sdk_home)
    if shell in {"bash", "zsh"}:
        lines = [
            f"_ecos_sdk_bin={shlex.quote(bin_dir)}",
            'export PATH="$_ecos_sdk_bin:${PATH#"$_ecos_sdk_bin:"}"',
            "unset _ecos_sdk_bin",
        ]
        if migrate_legacy_environment:
            lines.extend(
                [
                    f'if [ "${{ECOS_SDK_HOME-}}" = {shlex.quote(legacy_sdk_home)} ]; then',
                    "    unset ECOS_SDK_HOME",
                    "fi",
                ]
            )
        if shell == "zsh":
            lines.extend(
                [
                    "autoload -Uz compinit",
                    "(( $+functions[compdef] )) || compinit",
                ]
            )
        lines.append(f"source {shlex.quote(str(completion_file))}")
        return "\n".join(lines)
    if shell == "fish":
        lines = [f"fish_add_path {_fish_quote(bin_dir)}"]
        if migrate_legacy_environment:
            lines.extend(
                [
                    f"if test \"$ECOS_SDK_HOME\" = {_fish_quote(legacy_sdk_home)}",
                    "    set -e ECOS_SDK_HOME",
                    "end",
                ]
            )
        lines.append(f"source {_fish_quote(str(completion_file))}")
        return "\n".join(lines)
    if shell == "powershell":
        lines = [
            f"$ecosSdkPaths = @({_powershell_quote(bin_dir)})",
            "$ecosPathValue = $env:PATH",
            "if ($ecosPathValue -and $ecosPathValue -notlike '*;*') {",
            "    $ecosPathValue = @(",
            "        [Environment]::GetEnvironmentVariable('Path', 'Machine')",
            "        [Environment]::GetEnvironmentVariable('Path', 'User')",
            "    ) -join [IO.Path]::PathSeparator",
            "}",
            "$ecosCurrentPaths = @($ecosPathValue -split [IO.Path]::PathSeparator | Where-Object { $_ })",
            "$env:PATH = @(($ecosSdkPaths | Where-Object { $_ -notin $ecosCurrentPaths }) + $ecosCurrentPaths) -join [IO.Path]::PathSeparator",
        ]
        if migrate_legacy_environment:
            lines.extend(
                [
                    f"if ($env:ECOS_SDK_HOME -eq {_powershell_quote(legacy_sdk_home)}) {{",
                    "    Remove-Item Env:ECOS_SDK_HOME",
                    "}",
                ]
            )
        lines.extend(
            [
                f". {_powershell_quote(str(completion_file))}",
                "Remove-Variable ecosSdkPaths, ecosPathValue, ecosCurrentPaths -ErrorAction SilentlyContinue",
            ]
        )
        return "\n".join(lines)
    raise InstallerError(f"unsupported shell: {shell}")


def _updated_config_content(existing: str, block: str, path: Path) -> str:
    start = existing.find(SHELL_CONFIG_BEGIN)
    end = existing.find(SHELL_CONFIG_END)
    if (start < 0) != (end < 0) or (start >= 0 and end < start):
        raise InstallerError(f"incomplete ECOS configuration block in {path}")
    managed = f"{SHELL_CONFIG_BEGIN}\n{block.rstrip()}\n{SHELL_CONFIG_END}"
    if start >= 0:
        end += len(SHELL_CONFIG_END)
        return existing[:start] + managed + existing[end:]
    separator = "" if not existing else ("\n" if existing.endswith("\n") else "\n\n")
    return f"{existing}{separator}{managed}\n"


def _remove_legacy_shell_configuration(existing: str, prefix: Path) -> str:
    sdk = str(prefix)
    legacy_lines = {
        f"export PATH={sdk}/bin:$PATH",
        f"export PATH={sdk}/toolchain/riscv/bin:$PATH",
        f"export PATH={sdk}/toolchain/riscv_unknown/bin:$PATH",
        f"export ECOS_SDK_HOME={sdk}",
        f"source {sdk}/bin/ecos-completion.zsh",
    }
    lines = existing.splitlines(keepends=True)
    removed_completion = any(
        line.strip() == f"source {sdk}/bin/ecos-completion.zsh" for line in lines
    )
    filtered = [line for line in lines if line.strip() not in legacy_lines]
    if removed_completion:
        filtered = [
            line for line in filtered if line.strip() != "# ECOS command completion"
        ]
    return "".join(filtered)


def _write_config_atomic(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    previous_mode = stat.S_IMODE(path.stat().st_mode) if path.exists() else 0o600
    with tempfile.NamedTemporaryFile(
        prefix=f".{path.name}.ecos-", dir=path.parent, delete=False
    ) as temporary:
        temporary_path = Path(temporary.name)
        temporary.write(content.encode("utf-8"))
    try:
        temporary_path.chmod(previous_mode)
        temporary_path.replace(path)
    finally:
        temporary_path.unlink(missing_ok=True)


def shell_reload_command(path: Path, shell: str) -> str:
    if shell in {"bash", "zsh", "fish"}:
        return f"source {shlex.quote(str(path))}"
    if shell == "powershell":
        return f". {_powershell_quote(str(path))}"
    raise InstallerError(f"unsupported shell: {shell}")


def configure_shell(
    prefix: Path,
    shell: str,
    *,
    home: Optional[Path] = None,
    profile: Optional[Path] = None,
    dry_run: bool = False,
) -> dict[str, Any]:
    if shell == "none":
        return {
            "shell": "none",
            "state": "disabled",
            "changed": False,
            "config_file": None,
            "completion_file": None,
            "reload_command": None,
        }
    config_path = (profile or shell_config_path(shell, home)).expanduser().resolve()
    completion_file = prefix / COMPLETION_RELATIVE_ROOT / completion.COMPLETION_FILENAMES[shell]
    try:
        existing = config_path.read_text(encoding="utf-8") if config_path.is_file() else ""
    except UnicodeDecodeError as exc:
        raise InstallerError(f"shell configuration is not UTF-8: {config_path}") from exc
    migrated = (
        _remove_legacy_shell_configuration(existing, prefix)
        if shell in {"bash", "zsh"}
        else existing
    )
    legacy_prefix = toolchain.default_prefix().expanduser().resolve()
    if shell in {"bash", "zsh"} and legacy_prefix != prefix:
        migrated = _remove_legacy_shell_configuration(migrated, legacy_prefix)
    updated = _updated_config_content(
        migrated, shell_configuration_block(prefix, shell), config_path
    )
    changed = updated != existing
    if changed and not dry_run:
        _write_config_atomic(config_path, updated)
    return {
        "shell": shell,
        "state": "planned" if dry_run else "configured",
        "changed": changed,
        "config_file": str(config_path),
        "completion_file": str(completion_file),
        "reload_command": shell_reload_command(config_path, shell),
    }


def environment_commands(prefix: Path, shell: str) -> list[str]:
    if shell == "none":
        return []
    bin_dir = str(prefix / "bin")
    if shell in {"bash", "zsh"}:
        return [
            f"export PATH={shlex.quote(bin_dir)}:$PATH",
            f"source {shlex.quote(str(prefix / COMPLETION_RELATIVE_ROOT / completion.COMPLETION_FILENAMES[shell]))}",
        ]
    if shell == "fish":
        return [
            f"fish_add_path {_fish_quote(bin_dir)}",
            f"source {_fish_quote(str(prefix / COMPLETION_RELATIVE_ROOT / completion.COMPLETION_FILENAMES[shell]))}",
        ]
    if shell == "powershell":
        escaped_paths = bin_dir.replace("'", "''")
        return [
            f"$env:PATH = '{escaped_paths};' + $env:PATH",
            f". {_powershell_quote(str(prefix / COMPLETION_RELATIVE_ROOT / completion.COMPLETION_FILENAMES[shell]))}",
        ]
    raise InstallerError(f"unsupported shell format: {shell}")


def create_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--prefix",
        type=Path,
        default=default_install_base(),
        help="parent directory for versioned SDK installations",
    )
    parser.add_argument("--cache-dir", type=Path, default=toolchain.default_cache_root())
    parser.add_argument("--archive", type=Path, help="install the toolchain from an offline archive")
    parser.add_argument(
        "--skip-toolchain", action="store_true", help="install SDK files without the toolchain"
    )
    parser.add_argument(
        "--force-toolchain", action="store_true", help="replace an invalid toolchain installation"
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="reinstall SDK files, replace its registration, and reinstall the toolchain",
    )
    parser.add_argument(
        "--dry-run", action="store_true", help="show the plan without writing or downloading"
    )
    parser.add_argument("--format", choices=("text", "json"), default="text", dest="output_format")
    parser.add_argument(
        "--shell",
        choices=("auto", *completion.SUPPORTED_SHELLS, "none"),
        default="auto",
        help="shell profile and completion to configure during installation",
    )
    parser.add_argument(
        "--shell-profile",
        type=Path,
        help="override the shell startup file selected for configuration",
    )
    parser.add_argument(
        "--registry",
        type=Path,
        help="override the user SDK registry path",
    )
    parser.add_argument(
        "--registration-name",
        help="registration name (defaults to the SDK version)",
    )
    parser.add_argument(
        "--no-activate",
        action="store_true",
        help="register the installed SDK without making it globally active",
    )
    parser.add_argument(
        "--replace-registration",
        action="store_true",
        help="replace a conflicting registration with the same name",
    )
    return parser


def _emit_error(output_format: str, code: str, message: str, exit_code: int) -> int:
    item = {"code": code, "severity": "error", "message": message}
    if output_format == "json":
        print(json.dumps(result_envelope("error", None, [item]), indent=2, ensure_ascii=False))
    else:
        ConsoleProgress().error(f"[{code}] {message}")
    return exit_code


def _print_text(data: dict[str, Any], console: ConsoleProgress) -> None:
    console.info(f"SDK install base: {data['install_base']}")
    console.info(f"SDK installed at: {data['prefix']}")
    registration = data["registration"]
    console.info(f"SDK version: {registration['entry']['sdk_version']}")
    console.info(f"SDK registration: {registration['name']}")
    console.info(f"Active SDK: {registration['active'] or 'unchanged'}")
    toolchain_result = data.get("toolchain")
    if toolchain_result is None:
        console.info("Toolchain: skipped")
    else:
        installation = toolchain_result["installation"]
        console.info(f"Toolchain: {installation['state']}")
        console.info(f"Compiler: {installation['compiler']['path']}")
    configuration = data["environment"]["configuration"]
    if configuration["state"] == "disabled":
        console.warning("Shell environment and completion configuration was disabled.")
    else:
        action = "updated" if configuration["changed"] else "already current"
        console.info(f"Shell configuration {action}: {configuration['config_file']}")
        console.info(f"Reload the current shell: {configuration['reload_command']}")


def main(
    argv: Optional[Sequence[str]] = None, *, sdk_root: Optional[Path] = None
) -> int:
    args = create_parser().parse_args(list(argv) if argv is not None else None)
    source_root = (
        sdk_root.resolve()
        if sdk_root is not None
        else Path(__file__).resolve().parents[4]
    )
    install_base = args.prefix.expanduser().resolve()
    shell = detect_shell(args.shell)
    try:
        if args.skip_toolchain and (args.archive or args.force_toolchain):
            raise InstallerError(
                "--archive and --force-toolchain cannot be used with --skip-toolchain"
            )
        if shell == "none" and args.shell_profile:
            raise InstallerError("--shell-profile cannot be used with --shell none")
        _, sdk_identity = sdk_manifest.validate_sdk_root(source_root)
        prefix = versioned_install_path(install_base, sdk_identity["sdk_version"])
        validate_layout(source_root, prefix)
        registry = SdkRegistry(args.registry)
        plan_diagnostics: list[dict[str, Any]] = []
        try:
            registration_plan = registry.preview_registration(
                prefix,
                sdk_identity,
                name=args.registration_name,
                kind="release",
                activate=not args.no_activate,
                replace=args.force or args.replace_registration,
            )
            registration_plan["requires_force"] = False
            registration_plan["conflict"] = None
        except SdkRegistrationConflict as exc:
            if not args.dry_run:
                raise
            registration_plan = registry.preview_registration(
                prefix,
                sdk_identity,
                name=args.registration_name,
                kind="release",
                activate=not args.no_activate,
                replace=True,
            )
            registration_plan["requires_force"] = True
            registration_plan["conflict"] = str(exc)
            plan_diagnostics.append(
                {
                    "code": "ECOS_SDK_REGISTRATION_CONFLICT",
                    "severity": "warning",
                    "message": str(exc),
                    "suggestion": (
                        "Rerun with --force to reinstall everything, or "
                        "--replace-registration to replace only the registration."
                    ),
                }
            )
        manifest = None
        host = None
        selection = None
        if not args.skip_toolchain:
            manifest = toolchain.load_manifest()
            expected_toolchain = sdk_identity["toolchain"]
            if (
                manifest["id"] != expected_toolchain["id"]
                or manifest["release"] != expected_toolchain["release"]
            ):
                raise InstallerError(
                    "toolchain package does not match SDK manifest requirement"
                )
            host = toolchain.detect_host()
            selection = toolchain.selection_data(manifest, host)
        configuration_plan = configure_shell(
            prefix,
            shell,
            profile=args.shell_profile,
            dry_run=True,
        )
        plan = installation_plan(source_root, prefix)
        plan.update(
            {
                "install_base": str(install_base),
                "toolchain": None
                if args.skip_toolchain
                else {
                    "selection": selection,
                    "archive": str(args.archive.expanduser().resolve())
                    if args.archive
                    else None,
                    "installation": None,
                },
                "dry_run": args.dry_run,
                "force": args.force,
                "registration": registration_plan,
                "environment": {
                    "shell": shell,
                    "commands": environment_commands(prefix, shell),
                    "configuration": configuration_plan,
                },
            }
        )
        console = ConsoleProgress(enabled=args.output_format == "text")
        if args.dry_run:
            if args.output_format == "json":
                print(
                    json.dumps(
                        result_envelope("ok", plan, plan_diagnostics),
                        indent=2,
                        ensure_ascii=False,
                    )
                )
            else:
                console.info("Dry run; no files were written and no network request was made.")
                console.info(f"SDK source: {plan['sdk_root']}")
                console.info(f"Install base: {plan['install_base']}")
                console.info(f"Versioned install path: {plan['prefix']}")
                if plan["toolchain"] is None:
                    console.info("Toolchain: skip")
                else:
                    console.info(
                        "Toolchain: "
                        f"{plan['toolchain']['selection']['name']} "
                        f"{plan['toolchain']['selection']['release']}"
                    )
                if configuration_plan["state"] == "disabled":
                    console.info("Shell configuration: disabled")
                else:
                    console.info(
                        f"Shell configuration: {configuration_plan['config_file']}"
                    )
                for item in plan_diagnostics:
                    console.warning(f"[{item['code']}] {item['message']}")
                    console.info(f"Suggestion: {item['suggestion']}")
            return 0

        core_result = install_sdk_core(source_root, prefix, progress=console.message)
        toolchain_result = None
        if not args.skip_toolchain:
            assert manifest is not None and host is not None and selection is not None
            installation = toolchain.install_toolchain(
                manifest,
                prefix,
                host,
                args.cache_dir,
                archive_override=args.archive,
                force=args.force or args.force_toolchain,
                progress=console.message,
                download_progress=console.download,
            )
            toolchain_result = {
                "selection": selection,
                "installation": installation,
            }
        registration = registry.register(
            prefix,
            name=args.registration_name,
            kind="release",
            activate=not args.no_activate,
            replace=args.force or args.replace_registration,
        )
        shell_configuration = configure_shell(
            prefix,
            shell,
            profile=args.shell_profile,
        )
        data = {
            **core_result,
            "install_base": str(install_base),
            "force": args.force,
            "toolchain": toolchain_result,
            "registration": registration,
            "environment": {
                "shell": shell,
                "commands": environment_commands(prefix, shell),
                "configuration": shell_configuration,
            },
        }
        if args.output_format == "json":
            print(json.dumps(result_envelope("ok", data), indent=2, ensure_ascii=False))
        else:
            _print_text(data, console)
        return 0
    except InstallerError as exc:
        return _emit_error(args.output_format, "ECOS_INSTALL_CONFIG_INVALID", str(exc), 3)
    except SdkRegistryError as exc:
        return _emit_error(args.output_format, "ECOS_SDK_REGISTRY_INVALID", str(exc), 3)
    except sdk_manifest.SdkManifestError as exc:
        return _emit_error(args.output_format, "ECOS_SDK_MANIFEST_INVALID", str(exc), 3)
    except toolchain.DownloadError as exc:
        return _emit_error(args.output_format, "ECOS_TOOLCHAIN_DOWNLOAD_FAILED", str(exc), 6)
    except toolchain.ToolchainError as exc:
        return _emit_error(args.output_format, "ECOS_TOOLCHAIN_INSTALL_FAILED", str(exc), 5)
    except OSError as exc:
        return _emit_error(args.output_format, "ECOS_INSTALL_IO_FAILED", str(exc), 5)
