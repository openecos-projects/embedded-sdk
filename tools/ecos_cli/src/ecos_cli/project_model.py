"""Resolve an ECOS project into one validated, deterministic build model."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
from typing import Any, Iterable, Optional

import yaml

from . import project
from . import toolchain as toolchain_support
from .sdk_context import SdkContext


MODEL_SCHEMA_VERSION = 1
SUPPORTED_BUILD_OUTPUTS = {
    "elf",
    "bin",
    "hex",
    "txt",
    "map",
    "size",
    "compile_commands",
}
REQUIRED_BUILD_OUTPUTS = {"elf", "bin", "hex", "map", "size", "compile_commands"}


class ProjectModelError(RuntimeError):
    """Base error for project model resolution."""


class ManifestValidationError(ProjectModelError):
    """A project resource manifest is malformed or unsafe."""


class CapabilityMismatchError(ProjectModelError):
    """The selected hardware cannot satisfy an application requirement."""


class ComponentResolutionError(ProjectModelError):
    """A component dependency cannot be resolved."""


class ToolchainResolutionError(ProjectModelError):
    """The Target toolchain declaration conflicts with the selected SDK."""


def _read_manifest(path: Path, kind: str) -> dict[str, Any]:
    try:
        value = yaml.safe_load(path.read_text(encoding="utf-8"))
    except OSError as exc:
        raise ManifestValidationError(f"cannot read {kind} manifest {path}: {exc}") from exc
    except yaml.YAMLError as exc:
        raise ManifestValidationError(f"invalid {kind} manifest YAML {path}: {exc}") from exc
    if not isinstance(value, dict):
        raise ManifestValidationError(f"{kind} manifest must be a mapping: {path}")
    return value


def _check_keys(
    value: dict[str, Any], allowed: set[str], *, kind: str, path: Path
) -> None:
    unknown = sorted(set(value).difference(allowed))
    if unknown:
        raise ManifestValidationError(
            f"{kind} manifest contains unsupported fields at {path}: "
            + ", ".join(unknown)
        )


def _string(value: Any, field: str, path: Path, *, optional: bool = False) -> Optional[str]:
    if value is None and optional:
        return None
    if not isinstance(value, str) or not value:
        raise ManifestValidationError(
            f"{field} must be a non-empty string in {path}"
        )
    return value


def _string_list(value: Any, field: str, path: Path) -> list[str]:
    if value is None:
        return []
    if not isinstance(value, list) or not all(
        isinstance(item, str) and item for item in value
    ):
        raise ManifestValidationError(f"{field} must be a string list in {path}")
    if len(value) != len(set(value)):
        raise ManifestValidationError(f"{field} contains duplicate values in {path}")
    return list(value)


def _mapping(value: Any, field: str, path: Path) -> dict[str, Any]:
    if value is None:
        return {}
    if not isinstance(value, dict) or not all(isinstance(key, str) for key in value):
        raise ManifestValidationError(f"{field} must be a mapping in {path}")
    return dict(value)


def _integer(value: Any, field: str, path: Path) -> int:
    if isinstance(value, bool):
        raise ManifestValidationError(f"{field} must be an integer in {path}")
    if isinstance(value, int):
        return value
    if isinstance(value, str):
        text = value.strip().upper()
        multiplier = 1
        if text.endswith("K"):
            multiplier, text = 1024, text[:-1]
        elif text.endswith("M"):
            multiplier, text = 1024 * 1024, text[:-1]
        try:
            return int(text, 0) * multiplier
        except ValueError as exc:
            raise ManifestValidationError(
                f"{field} must be an integer in {path}: {value!r}"
            ) from exc
    raise ManifestValidationError(f"{field} must be an integer in {path}")


def _resolved_path(
    root: Path,
    value: str,
    field: str,
    manifest_path: Path,
    *,
    allowed_root: Optional[Path] = None,
    must_be_file: bool = True,
) -> Path:
    declared = Path(value)
    candidate = declared if declared.is_absolute() else root / declared
    resolved = candidate.resolve()
    boundary = (allowed_root or root).resolve()
    try:
        resolved.relative_to(boundary)
    except ValueError as exc:
        raise ManifestValidationError(
            f"{field} resolves outside {boundary}: {value!r} in {manifest_path}"
        ) from exc
    if must_be_file and not resolved.is_file():
        raise ManifestValidationError(
            f"{field} does not name an existing file: {resolved}"
        )
    if not must_be_file and not resolved.is_dir():
        raise ManifestValidationError(
            f"{field} does not name an existing directory: {resolved}"
        )
    return resolved


def _path_list(
    root: Path,
    values: Any,
    field: str,
    manifest_path: Path,
    *,
    allowed_root: Optional[Path] = None,
    directories: bool = False,
) -> list[Path]:
    declared = _string_list(values, field, manifest_path)
    return [
        _resolved_path(
            root,
            item,
            field,
            manifest_path,
            allowed_root=allowed_root,
            must_be_file=not directories,
        )
        for item in declared
    ]


class _Inputs:
    def __init__(self, project_root: Path, sdk_root: Path) -> None:
        self.project_root = project_root.resolve()
        self.sdk_root = sdk_root.resolve()
        self._items: dict[str, dict[str, Any]] = {}

    def add(self, path: Path) -> None:
        resolved = path.resolve()
        try:
            relative = resolved.relative_to(self.project_root)
            identity = f"project:{relative.as_posix()}"
        except ValueError:
            try:
                relative = resolved.relative_to(self.sdk_root)
                identity = f"sdk:{relative.as_posix()}"
            except ValueError as exc:
                raise ManifestValidationError(
                    f"resolved input is outside the project and SDK: {resolved}"
                ) from exc
        try:
            digest = hashlib.sha256(resolved.read_bytes()).hexdigest()
        except OSError as exc:
            raise ManifestValidationError(f"cannot hash project input {resolved}: {exc}") from exc
        self._items[identity] = {
            "identity": identity,
            "path": str(resolved),
            "sha256": digest,
        }

    def add_include_directory(self, path: Path) -> None:
        """Track headers that can affect compilation without hashing derived state."""

        directory = path.resolve()
        for candidate in sorted(directory.rglob("*")):
            if not candidate.is_file():
                continue
            try:
                relative = candidate.relative_to(self.project_root)
            except ValueError:
                relative = None
            if relative is not None and (
                relative.parts[:1] in {("build",), (".git",)}
                or relative.parts[:2] == (".ecos", "generated")
            ):
                continue
            if candidate.suffix.lower() in {".h", ".hh", ".hpp", ".hxx", ".inc"}:
                self.add(candidate)

    def values(self) -> list[dict[str, Any]]:
        return [self._items[key] for key in sorted(self._items)]

    def fingerprint(self) -> str:
        canonical = [
            {"identity": item["identity"], "sha256": item["sha256"]}
            for item in self.values()
        ]
        encoded = json.dumps(
            canonical, sort_keys=True, separators=(",", ":"), ensure_ascii=False
        ).encode("utf-8")
        return hashlib.sha256(encoded).hexdigest()


def _find_board_manifest(context: SdkContext, board_id: str) -> Path:
    matches: list[Path] = []
    for path in sorted(context.resource("boards").glob("*/ecos-board.yml")):
        value = _read_manifest(path, "Board")
        aliases = value.get("aliases", [])
        if value.get("id") == board_id or (
            isinstance(aliases, list) and board_id in aliases
        ):
            matches.append(path.resolve())
    if not matches:
        raise project.BoardNotFound(f"SDK Board does not exist: {board_id}")
    if len(matches) > 1:
        raise project.BoardManifestError(
            f"Board ID or alias is ambiguous in the SDK: {board_id}"
        )
    return matches[0]


def _resolve_board(
    context: SdkContext, requested: str, expected_target: str, inputs: _Inputs
) -> dict[str, Any]:
    path = _find_board_manifest(context, requested)
    value = _read_manifest(path, "Board")
    _check_keys(
        value,
        {
            "schema",
            "id",
            "name",
            "aliases",
            "category",
            "arch",
            "target",
            "build",
            "resources",
            "files",
            "kconfig",
            "paths",
            "abstract_machine",
            "flash",
            "monitor",
        },
        kind="Board",
        path=path,
    )
    if value.get("schema") != 2:
        raise ManifestValidationError(f"unsupported Board schema in {path}")
    board_id = _string(value.get("id"), "Board.id", path)
    assert board_id is not None
    project.validate_selection("board ID", board_id)
    aliases = _string_list(value.get("aliases"), "Board.aliases", path)
    for alias in aliases:
        project.validate_selection("board alias", alias)
    if requested not in {board_id, *aliases}:
        raise ManifestValidationError(
            f"Board manifest {path} does not identify requested Board {requested!r}"
        )
    name = _string(value.get("name", board_id), "Board.name", path)
    arch = _string(value.get("arch"), "Board.arch", path, optional=True)
    target = _string(value.get("target"), "Board.target", path)
    if target != expected_target:
        raise CapabilityMismatchError(
            f"project Board {board_id!r} maps to {target!r}, not {expected_target!r}"
        )
    resources = _mapping(value.get("resources"), "Board.resources", path)
    for resource_id, resource in resources.items():
        project.validate_selection("Board resource", resource_id)
        if not isinstance(resource, dict):
            raise ManifestValidationError(
                f"Board resource {resource_id!r} must be a mapping in {path}"
            )
    build = _mapping(value.get("build"), "Board.build", path)
    _check_keys(
        build,
        {"default_profile", "profiles"},
        kind="Board build",
        path=path,
    )
    default_profile = build.get("default_profile")
    if default_profile is not None:
        default_profile = _string(default_profile, "Board.build.default_profile", path)
        project.validate_selection("profile", default_profile)
    profiles = build.get("profiles")
    if profiles is not None and not isinstance(profiles, (dict, list)):
        raise ManifestValidationError(
            f"Board.build.profiles must be a mapping or list in {path}"
        )
    profile_names: list[str] = []
    if isinstance(profiles, list):
        profile_names = _string_list(profiles, "Board.build.profiles", path)
    elif isinstance(profiles, dict):
        for profile_name, profile_value in profiles.items():
            project.validate_selection("profile", profile_name)
            if not isinstance(profile_value, dict):
                raise ManifestValidationError(
                    f"Board.build.profiles.{profile_name} must be a mapping in {path}"
                )
        profile_names = list(profiles)
    if default_profile is not None:
        if profile_names and default_profile not in profile_names:
            raise ManifestValidationError(
                f"Board.build.default_profile is not declared in profiles: {path}"
            )
        if not profile_names:
            profile_names = [default_profile]

    kconfig = _mapping(value.get("kconfig"), "Board.kconfig", path)
    _check_keys(kconfig, {"board", "drivers"}, kind="Board Kconfig", path=path)
    kconfig_paths: list[Path] = []
    legacy_kconfig_paths: list[Path] = []
    board_kconfig = kconfig.get("board")
    if board_kconfig is not None:
        board_kconfig = _string(board_kconfig, "Board.kconfig.board", path)
        assert board_kconfig is not None
        kconfig_paths.append(
            _resolved_path(path.parent, board_kconfig, "Board.kconfig.board", path)
        )
    driver_kconfig = kconfig.get("drivers")
    if driver_kconfig is not None:
        driver_kconfig = _string(driver_kconfig, "Board.kconfig.drivers", path)
        assert driver_kconfig is not None
        legacy_kconfig_paths.append(
            _resolved_path(
                path.parent, driver_kconfig, "Board.kconfig.drivers", path
            )
        )

    flash = _mapping(value.get("flash"), "Board.flash", path)
    if flash:
        _check_keys(
            flash,
            {
                "provider",
                "artifact",
                "volume_label",
                "copy_count",
                "settle_seconds",
            },
            kind="Board flash",
            path=path,
        )
        _string(flash.get("provider"), "Board.flash.provider", path)
        flash_artifact = _string(
            flash.get("artifact", "bin"), "Board.flash.artifact", path
        )
        project.validate_selection("Board flash artifact", flash_artifact)
        if "volume_label" in flash:
            _string(flash["volume_label"], "Board.flash.volume_label", path)
        copy_count = flash.get("copy_count", 1)
        settle_seconds = flash.get("settle_seconds", 0)
        if not isinstance(copy_count, int) or isinstance(copy_count, bool) or copy_count < 1:
            raise ManifestValidationError(
                f"Board.flash.copy_count must be a positive integer in {path}"
            )
        if (
            not isinstance(settle_seconds, (int, float))
            or isinstance(settle_seconds, bool)
            or settle_seconds < 0
        ):
            raise ManifestValidationError(
                f"Board.flash.settle_seconds must be non-negative in {path}"
            )

    monitor = _mapping(value.get("monitor"), "Board.monitor", path)
    if monitor:
        _check_keys(
            monitor,
            {"baudrate", "vid", "pid", "serial_number"},
            kind="Board monitor",
            path=path,
        )
        baudrate = monitor.get("baudrate", 115200)
        if not isinstance(baudrate, int) or isinstance(baudrate, bool) or baudrate < 1:
            raise ManifestValidationError(
                f"Board.monitor.baudrate must be a positive integer in {path}"
            )
        for field in ("vid", "pid"):
            item = monitor.get(field)
            if item is not None and (
                not isinstance(item, int) or isinstance(item, bool) or not 0 <= item <= 0xFFFF
            ):
                raise ManifestValidationError(
                    f"Board.monitor.{field} must be a USB 16-bit integer in {path}"
                )
        if "serial_number" in monitor:
            _string(monitor["serial_number"], "Board.monitor.serial_number", path)

    inputs.add(path)
    for item in [*kconfig_paths, *legacy_kconfig_paths]:
        inputs.add(item)
    return {
        "id": board_id,
        "name": name,
        "manifest": str(path),
        "target": target,
        "arch": arch,
        "default_profile": default_profile,
        "profiles": profiles,
        "profile_names": profile_names,
        "resources": resources,
        "kconfig": [str(item) for item in kconfig_paths],
        "legacy_kconfig": [str(item) for item in legacy_kconfig_paths],
        "flash": flash or None,
        "monitor": monitor or None,
    }


def _resolve_target(
    context: SdkContext, target_id: str, inputs: _Inputs
) -> dict[str, Any]:
    project.resolve_target(context, target_id)
    root = (context.resource("components") / "soc" / target_id).resolve()
    path = root / "ecos-soc.yml"
    value = _read_manifest(path, "Target")
    _check_keys(
        value,
        {
            "schema",
            "id",
            "name",
            "arch",
            "description",
            "cpu",
            "toolchain",
            "memory",
            "build",
            "capabilities",
        },
        kind="Target",
        path=path,
    )
    if value.get("schema") != 1 or value.get("id") != target_id:
        raise ManifestValidationError(f"Target schema or ID is invalid: {path}")
    name = _string(value.get("name", target_id), "Target.name", path)
    arch = _string(value.get("arch"), "Target.arch", path)
    cpu = _mapping(value.get("cpu"), "Target.cpu", path)
    _check_keys(cpu, {"march", "abi"}, kind="Target CPU", path=path)
    march = _string(cpu.get("march"), "Target.cpu.march", path)
    abi = _string(cpu.get("abi"), "Target.cpu.abi", path)
    toolchain = _mapping(value.get("toolchain"), "Target.toolchain", path)
    _check_keys(
        toolchain,
        {"id", "release", "prefix", "triple"},
        kind="Target toolchain",
        path=path,
    )
    for field, item in toolchain.items():
        _string(item, f"Target.toolchain.{field}", path)
    capabilities = _string_list(
        value.get("capabilities"), "Target.capabilities", path
    )
    for capability in capabilities:
        project.validate_selection("Target capability", capability)
    build = _mapping(value.get("build"), "Target.build", path)
    _check_keys(
        build,
        {
            "cmake",
            "toolchain_file",
            "kconfig",
            "linker_script",
            "startup",
            "loader",
            "sources",
            "include_dirs",
            "capability_sources",
            "capability_include_dirs",
            "outputs",
        },
        kind="Target build",
        path=path,
    )
    sdk_root = context.root.resolve()
    cmake = _resolved_path(
        root,
        _string(build.get("cmake", "CMakeLists.txt"), "Target.build.cmake", path) or "",
        "Target.build.cmake",
        path,
    )
    toolchain_file = _resolved_path(
        root,
        _string(
            build.get("toolchain_file", "toolchain.cmake"),
            "Target.build.toolchain_file",
            path,
        )
        or "",
        "Target.build.toolchain_file",
        path,
    )
    linker_value = build.get("linker_script")
    linker_script = None
    if linker_value is not None:
        linker = _string(linker_value, "Target.build.linker_script", path)
        assert linker is not None
        linker_script = _resolved_path(
            root, linker, "Target.build.linker_script", path
        )

    sources = _path_list(
        root,
        build.get("sources"),
        "Target.build.sources",
        path,
        allowed_root=sdk_root,
    )
    fallback_sources: list[Path] = []
    for field in ("startup", "loader"):
        declared = build.get(field)
        if declared is not None:
            item = _string(declared, f"Target.build.{field}", path)
            assert item is not None
            fallback_sources.append(
                _resolved_path(root, item, f"Target.build.{field}", path)
            )
    if not sources:
        sources.extend(fallback_sources)
    include_dirs = _path_list(
        root,
        build.get("include_dirs"),
        "Target.build.include_dirs",
        path,
        allowed_root=sdk_root,
        directories=True,
    )
    capability_sources = _mapping(
        build.get("capability_sources"), "Target.build.capability_sources", path
    )
    capability_include_dirs = _mapping(
        build.get("capability_include_dirs"),
        "Target.build.capability_include_dirs",
        path,
    )
    resolved_capability_sources: dict[str, list[str]] = {}
    resolved_capability_includes: dict[str, list[str]] = {}
    for capability, declared in capability_sources.items():
        if capability not in capabilities:
            raise ManifestValidationError(
                f"Target build maps undeclared capability {capability!r} in {path}"
            )
        resolved_capability_sources[capability] = [
            str(item)
            for item in _path_list(
                root,
                declared,
                f"Target.build.capability_sources.{capability}",
                path,
                allowed_root=sdk_root,
            )
        ]
    for capability, declared in capability_include_dirs.items():
        if capability not in capabilities:
            raise ManifestValidationError(
                f"Target build maps undeclared capability {capability!r} in {path}"
            )
        resolved_capability_includes[capability] = [
            str(item)
            for item in _path_list(
                root,
                declared,
                f"Target.build.capability_include_dirs.{capability}",
                path,
                allowed_root=sdk_root,
                directories=True,
            )
        ]

    kconfig_paths: list[Path] = []
    if build.get("kconfig") is not None:
        kconfig_value = _string(build["kconfig"], "Target.build.kconfig", path)
        assert kconfig_value is not None
        kconfig_paths.append(
            _resolved_path(root, kconfig_value, "Target.build.kconfig", path)
        )
    outputs = _string_list(build.get("outputs"), "Target.build.outputs", path)
    if not outputs:
        outputs = ["elf", "bin", "txt", "hex", "map", "size", "compile_commands"]
    unsupported_outputs = sorted(set(outputs).difference(SUPPORTED_BUILD_OUTPUTS))
    if unsupported_outputs:
        raise ManifestValidationError(
            f"Target.build.outputs contains unsupported outputs in {path}: "
            + ", ".join(unsupported_outputs)
        )
    missing_outputs = sorted(REQUIRED_BUILD_OUTPUTS.difference(outputs))
    if missing_outputs:
        raise ManifestValidationError(
            f"Target.build.outputs omits required outputs in {path}: "
            + ", ".join(missing_outputs)
        )

    memory = _mapping(value.get("memory"), "Target.memory", path)
    for region, definition in memory.items():
        if not isinstance(definition, dict):
            raise ManifestValidationError(
                f"Target.memory.{region} must be a mapping in {path}"
            )
        if "origin" not in definition or "size" not in definition:
            raise ManifestValidationError(
                f"Target.memory.{region} must define origin and size in {path}"
            )
        unknown = sorted(set(definition).difference({"origin", "size", "attributes"}))
        if unknown:
            raise ManifestValidationError(
                f"Target.memory.{region} contains unsupported fields in {path}: "
                + ", ".join(unknown)
            )
        if "attributes" in definition:
            _string(definition["attributes"], f"Target.memory.{region}.attributes", path)
        origin = _integer(definition["origin"], f"Target.memory.{region}.origin", path)
        size = _integer(definition["size"], f"Target.memory.{region}.size", path)
        if origin < 0 or size <= 0:
            raise ManifestValidationError(
                f"Target.memory.{region} origin must be non-negative and size positive in {path}"
            )

    inputs.add(path)
    for item in [cmake, toolchain_file, linker_script, *sources, *include_dirs, *kconfig_paths]:
        if isinstance(item, Path) and item.is_file():
            inputs.add(item)
    for items in resolved_capability_sources.values():
        for item in items:
            inputs.add(Path(item))
    for item in include_dirs:
        inputs.add_include_directory(item)
    for items in resolved_capability_includes.values():
        for item in items:
            inputs.add_include_directory(Path(item))
    return {
        "id": target_id,
        "name": name,
        "root": str(root),
        "manifest": str(path),
        "arch": arch,
        "cpu": {"march": march, "abi": abi},
        "toolchain": toolchain,
        "memory": memory,
        "capabilities": capabilities,
        "build": {
            "cmake": str(cmake),
            "toolchain_file": str(toolchain_file),
            "linker_script": str(linker_script) if linker_script else None,
            "sources": [str(item) for item in sources],
            "include_dirs": [str(item) for item in include_dirs],
            "capability_sources": resolved_capability_sources,
            "capability_include_dirs": resolved_capability_includes,
            "kconfig": [str(item) for item in kconfig_paths],
            "outputs": outputs,
        },
    }


def _resolve_example(root: Path, expected_name: str, inputs: _Inputs) -> dict[str, Any]:
    path = root / "ecos-example.yml"
    value = _read_manifest(path, "Example")
    _check_keys(
        value,
        {"schema", "name", "sources", "include_dirs", "defines", "requires", "components"},
        kind="Example",
        path=path,
    )
    if value.get("schema") != 1:
        raise ManifestValidationError(f"unsupported Example schema in {path}")
    name = _string(value.get("name"), "Example.name", path)
    if name != expected_name:
        raise ManifestValidationError(
            f"project Example is {expected_name!r}, but {path} declares {name!r}"
        )
    sources = _path_list(root, value.get("sources"), "Example.sources", path)
    if not sources:
        raise ManifestValidationError(f"Example.sources must not be empty in {path}")
    include_dirs = _path_list(
        root,
        value.get("include_dirs", ["."]),
        "Example.include_dirs",
        path,
        directories=True,
    )
    requires = _string_list(value.get("requires"), "Example.requires", path)
    components = _string_list(value.get("components"), "Example.components", path)
    defines = _string_list(value.get("defines"), "Example.defines", path)
    for item in [*requires, *components]:
        project.validate_selection("Example dependency", item)
    inputs.add(path)
    for item in sources:
        inputs.add(item)
    for item in include_dirs:
        inputs.add_include_directory(item)
    return {
        "name": name,
        "manifest": str(path),
        "sources": [str(item) for item in sources],
        "include_dirs": [str(item) for item in include_dirs],
        "defines": defines,
        "requires": requires,
        "components": components,
    }


def _resolve_toolchain_requirement(
    context: SdkContext, target: dict[str, Any]
) -> dict[str, Any]:
    sdk_toolchain = context.manifest["toolchain"]
    target_toolchain = target["toolchain"]
    try:
        manifest = toolchain_support.load_manifest()
    except toolchain_support.ManifestError as exc:
        raise ToolchainResolutionError(str(exc)) from exc
    if (
        manifest["id"] != sdk_toolchain["id"]
        or manifest["release"] != sdk_toolchain["release"]
    ):
        raise ToolchainResolutionError(
            "SDK toolchain pin does not match the CLI toolchain manifest: "
            f"expected {sdk_toolchain['id']} {sdk_toolchain['release']}"
        )
    declared_id = target_toolchain.get("id")
    declared_release = target_toolchain.get("release")
    if declared_id is not None and declared_id != sdk_toolchain["id"]:
        raise ToolchainResolutionError(
            f"Target {target['id']!r} requires toolchain {declared_id!r}, "
            f"but SDK {context.version} pins {sdk_toolchain['id']!r}"
        )
    if declared_release is not None and declared_release != sdk_toolchain["release"]:
        raise ToolchainResolutionError(
            f"Target {target['id']!r} requires toolchain release "
            f"{declared_release!r}, but SDK {context.version} pins "
            f"{sdk_toolchain['release']!r}"
        )
    declared_prefix = target_toolchain.get("prefix")
    if declared_prefix is not None and declared_prefix != manifest["tool_prefix"]:
        raise ToolchainResolutionError(
            f"Target {target['id']!r} requires tool prefix {declared_prefix!r}, "
            f"but the SDK toolchain provides {manifest['tool_prefix']!r}"
        )
    declared_triple = target_toolchain.get("triple")
    if declared_triple is not None and declared_triple != manifest["target_triplet"]:
        raise ToolchainResolutionError(
            f"Target {target['id']!r} requires target triple {declared_triple!r}, "
            f"but the SDK toolchain provides {manifest['target_triplet']!r}"
        )
    cpu = target["cpu"]
    supported = any(
        item.get("isa") == cpu["march"] and item.get("abi") == cpu["abi"]
        for item in manifest.get("targets", [])
        if isinstance(item, dict)
    )
    if not supported:
        raise ToolchainResolutionError(
            f"SDK toolchain {manifest['id']!r} does not declare support for "
            f"{cpu['march']}/{cpu['abi']}"
        )
    return {
        "id": sdk_toolchain["id"],
        "release": sdk_toolchain["release"],
        "provider": manifest.get("provider"),
        "package_version": manifest.get("package_version"),
        "prefix": manifest["tool_prefix"],
        "triple": manifest["target_triplet"],
        "compiler": manifest["compiler"],
        "cmake_file": target["build"]["toolchain_file"],
    }


def _component_candidates(context: SdkContext) -> dict[str, list[tuple[Path, dict[str, Any]]]]:
    candidates: dict[str, list[tuple[Path, dict[str, Any]]]] = {}
    for path in sorted(context.resource("components").rglob("ecos-component.yml")):
        value = _read_manifest(path, "Component")
        component_id = value.get("id")
        if isinstance(component_id, str) and component_id:
            candidates.setdefault(component_id, []).append((path.resolve(), value))
    return candidates


def _resolve_components(
    context: SdkContext, requested: Iterable[str], inputs: _Inputs
) -> list[dict[str, Any]]:
    requested_list = list(requested)
    if not requested_list:
        return []
    candidates = _component_candidates(context)
    resolved: dict[str, dict[str, Any]] = {}
    visiting: list[str] = []
    sdk_root = context.root.resolve()

    def visit(component_id: str) -> None:
        if component_id in resolved:
            return
        if component_id in visiting:
            cycle = " -> ".join([*visiting, component_id])
            raise ComponentResolutionError(f"Component dependency cycle: {cycle}")
        matches = candidates.get(component_id, [])
        if not matches:
            raise ComponentResolutionError(f"SDK Component does not exist: {component_id}")
        if len(matches) > 1:
            locations = ", ".join(str(path) for path, _ in matches)
            raise ComponentResolutionError(
                f"SDK Component ID is ambiguous: {component_id} ({locations})"
            )
        path, value = matches[0]
        _check_keys(
            value,
            {
                "schema",
                "id",
                "name",
                "sources",
                "include_dirs",
                "defines",
                "dependencies",
                "requires",
            },
            kind="Component",
            path=path,
        )
        if value.get("schema") != 1 or value.get("id") != component_id:
            raise ManifestValidationError(f"Component schema or ID is invalid: {path}")
        name = _string(value.get("name", component_id), "Component.name", path)
        dependencies = _string_list(
            value.get("dependencies"), "Component.dependencies", path
        )
        requirements = _string_list(value.get("requires"), "Component.requires", path)
        for item in [*dependencies, *requirements]:
            project.validate_selection("Component dependency", item)
        visiting.append(component_id)
        for dependency in dependencies:
            visit(dependency)
        visiting.pop()
        root = path.parent
        sources = _path_list(
            root,
            value.get("sources"),
            "Component.sources",
            path,
            allowed_root=sdk_root,
        )
        include_dirs = _path_list(
            root,
            value.get("include_dirs"),
            "Component.include_dirs",
            path,
            allowed_root=sdk_root,
            directories=True,
        )
        inputs.add(path)
        for source in sources:
            inputs.add(source)
        for include_dir in include_dirs:
            inputs.add_include_directory(include_dir)
        resolved[component_id] = {
            "id": component_id,
            "name": name,
            "manifest": str(path),
            "sources": [str(item) for item in sources],
            "include_dirs": [str(item) for item in include_dirs],
            "defines": _string_list(value.get("defines"), "Component.defines", path),
            "dependencies": dependencies,
            "requires": requirements,
        }

    for component_id in requested_list:
        visit(component_id)
    return list(resolved.values())


def validate_example_source(
    context: SdkContext, example_root: Path, expected_name: str
) -> dict[str, Any]:
    """Validate a source Example and its Component graph before project creation."""

    root = example_root.resolve()
    inputs = _Inputs(root, context.root)
    example = _resolve_example(root, expected_name, inputs)
    components = _resolve_components(context, example["components"], inputs)
    return {
        "example": example,
        "components": components,
        "inputs": inputs.values(),
    }


def resolve_project(
    context: SdkContext,
    *,
    project_root: Optional[Path] = None,
    metadata_override: Optional[dict[str, Any]] = None,
) -> dict[str, Any]:
    """Resolve all project inputs without writing or invoking external tools."""

    root = (project_root or Path.cwd()).expanduser().resolve()
    metadata_path, current_metadata = project.load_project_metadata(root)
    metadata = dict(metadata_override) if metadata_override is not None else current_metadata
    project_name = metadata.get("name")
    if not isinstance(project_name, str):
        raise ManifestValidationError("project name must be a string")
    project.validate_project_name(project_name)
    inputs = _Inputs(root, context.root)
    inputs.add(metadata_path)
    sdk_manifest_path = context.root / "tools" / "sdk-manifest.json"
    if sdk_manifest_path.is_file():
        inputs.add(sdk_manifest_path)
    toolchain_manifest_path = (
        context.resource("cli")
        / "ecos_cli"
        / "resources"
        / "toolchains"
        / toolchain_support.MANIFEST_NAME
    )
    if toolchain_manifest_path.is_file():
        inputs.add(toolchain_manifest_path)

    sdk = metadata["sdk"]
    if sdk.get("id") != context.sdk_id or sdk.get("version") != context.version:
        raise ManifestValidationError(
            "project SDK identity does not match the selected SDK: "
            f"expected {sdk.get('id')} {sdk.get('version')}, "
            f"got {context.sdk_id} {context.version}"
        )
    target_id = metadata.get("target")
    if not isinstance(target_id, str) or not target_id:
        raise ManifestValidationError(
            "project has no Target; run 'ecos project set-board BOARD' or "
            "'ecos project set-target TARGET'"
        )
    target_id = project.resolve_target(context, target_id)
    target = _resolve_target(context, target_id, inputs)
    toolchain_requirement = _resolve_toolchain_requirement(context, target)

    board_value = metadata.get("board")
    board = None
    if board_value is not None:
        if not isinstance(board_value, str) or not board_value:
            raise ManifestValidationError("project Board must be a non-empty string or null")
        board = _resolve_board(context, board_value, target_id, inputs)
        board_arch = board.get("arch")
        if board_arch is not None and board_arch != target["arch"]:
            raise CapabilityMismatchError(
                f"Board {board['id']!r} uses architecture {board_arch!r}, "
                f"but Target {target_id!r} uses {target['arch']!r}"
            )

    example_name = metadata.get("example")
    if not isinstance(example_name, str) or not example_name:
        raise ManifestValidationError("project Example must be a non-empty string")
    example = _resolve_example(root, example_name, inputs)
    components = _resolve_components(context, example["components"], inputs)

    requested_capabilities = set(example["requires"])
    for component in components:
        requested_capabilities.update(component["requires"])
    available_capabilities = set(target["capabilities"])
    if board is not None:
        available_capabilities.update(board["resources"])
    missing = sorted(requested_capabilities.difference(available_capabilities))
    if missing:
        selected = board["id"] if board else target_id
        raise CapabilityMismatchError(
            f"{selected} cannot satisfy required capabilities: {', '.join(missing)}"
        )

    target_core_sources = list(target["build"]["sources"])
    target_capability_sources: list[str] = []
    target_includes = list(target["build"]["include_dirs"])
    for capability in sorted(requested_capabilities):
        target_capability_sources.extend(
            target["build"]["capability_sources"].get(capability, [])
        )
        target_includes.extend(
            target["build"]["capability_include_dirs"].get(capability, [])
        )
    target_core_sources = list(dict.fromkeys(target_core_sources))
    target_capability_sources = list(dict.fromkeys(target_capability_sources))
    target_sources = [*target_core_sources, *target_capability_sources]
    target_includes = list(dict.fromkeys(target_includes))
    for source in target_sources:
        inputs.add(Path(source))

    profile = metadata.get("profile")
    if profile is not None:
        if not isinstance(profile, str):
            raise ManifestValidationError("project profile must be a string or null")
        project.validate_selection("profile", profile)
    elif board is not None:
        profile = board["default_profile"]
    if board is not None and profile is not None:
        profile_names = board["profile_names"]
        if profile_names and profile not in profile_names:
            raise ManifestValidationError(
                f"profile {profile!r} is not supported by Board {board['id']!r}; "
                f"choose one of: {', '.join(profile_names)}"
            )

    selected_cmake = root / "CMakeLists.txt"
    if selected_cmake.is_file():
        inputs.add(selected_cmake)
    else:
        selected_cmake = Path(target["build"]["cmake"])

    if board is not None and board.get("flash") is not None:
        flash_artifact = board["flash"].get("artifact", "bin")
        if flash_artifact not in target["build"]["outputs"]:
            raise ManifestValidationError(
                f"Board {board['id']!r} requires flash artifact {flash_artifact!r}, "
                f"but Target {target_id!r} does not produce it"
            )

    kconfig_paths: list[str] = []
    if board is not None:
        kconfig_paths.extend(board["kconfig"])
    kconfig_paths.extend(target["build"]["kconfig"])
    common_kconfig = context.root / "tools" / "kconfig" / "Kconfig.build"
    if common_kconfig.is_file():
        kconfig_paths.append(str(common_kconfig.resolve()))
    for path_value in kconfig_paths:
        inputs.add(Path(path_value))
    user_config = root / ".ecos" / "project.config"
    if user_config.is_file():
        inputs.add(user_config)

    resolved_inputs = inputs.values()
    source_fingerprint = inputs.fingerprint()
    return {
        "schema_version": MODEL_SCHEMA_VERSION,
        "source_fingerprint": source_fingerprint,
        "project": {
            "root": str(root),
            "metadata": str(metadata_path),
            "name": project_name,
            "example": example_name,
            "board": board["id"] if board else None,
            "target": target_id,
            "profile": profile,
        },
        "sdk": {
            "root": str(context.root.resolve()),
            "id": context.sdk_id,
            "version": context.version,
            "kind": context.kind,
        },
        "example": example,
        "board": board,
        "target": target,
        "toolchain": toolchain_requirement,
        "components": components,
        "requirements": {
            "requested": sorted(requested_capabilities),
            "available": sorted(available_capabilities),
        },
        "build": {
            "application_sources": example["sources"],
            "application_include_dirs": example["include_dirs"],
            "application_defines": example["defines"],
            "target_sources": target_core_sources,
            "target_capability_sources": target_capability_sources,
            "target_include_dirs": target_includes,
            "linker_script": target["build"]["linker_script"],
            "cmake": str(selected_cmake.resolve()),
            "toolchain_file": target["build"]["toolchain_file"],
            "outputs": target["build"]["outputs"],
        },
        "kconfig": {
            "sources": kconfig_paths,
            "user_config": str(user_config) if user_config.is_file() else None,
        },
        "inputs": resolved_inputs,
    }
