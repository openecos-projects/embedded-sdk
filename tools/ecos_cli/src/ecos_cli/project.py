"""Project creation from SDK examples."""

from __future__ import annotations

import json
import os
import re
import shutil
import tempfile
import uuid
from pathlib import Path
from typing import Any, Optional

import yaml

from .sdk_context import SdkContext
from .sdk_resolver import SdkResolutionError, write_project_pin


PROJECT_SCHEMA_VERSION = 2
EXAMPLE_SEGMENT_PATTERN = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]*$")
PROJECT_NAME_PATTERN = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]*$")
SELECTION_PATTERN = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]*$")


class ProjectError(RuntimeError):
    """Base error for project creation."""


class InvalidProjectArgument(ProjectError):
    """A project creation argument is invalid."""


class ExampleNotFound(ProjectError):
    """The requested SDK example does not exist."""


class ExampleAmbiguous(ProjectError):
    """More than one SDK example uses the requested name."""


class ProjectConflict(ProjectError):
    """The target project path already exists."""


class ProjectWriteError(ProjectError):
    """The project could not be written completely."""


class ProjectNotFound(ProjectError):
    """The selected directory is not an ECOS project."""


class ProjectMetadataError(ProjectError):
    """The project metadata cannot be read or updated."""


class BoardNotFound(ProjectError):
    """The requested Board is not available in the SDK."""


class BoardManifestError(ProjectError):
    """A Board manifest cannot provide a valid Target."""


class TargetNotFound(ProjectError):
    """The requested Target/SoC is not available in the SDK."""


def normalize_example_name(value: str) -> str:
    if not value or "/" in value or "\\" in value:
        raise InvalidProjectArgument(
            "example name must not include a parent directory"
        )
    if value in {".", ".."} or not EXAMPLE_SEGMENT_PATTERN.fullmatch(value):
        raise InvalidProjectArgument(
            "example name must contain only ASCII letters, digits, '.', '_', or '-'"
        )
    return value


def validate_project_name(value: str) -> str:
    if not PROJECT_NAME_PATTERN.fullmatch(value) or value in {".", ".."}:
        raise InvalidProjectArgument(
            "project name must be one ASCII path segment containing only "
            "letters, digits, '.', '_', or '-'"
        )
    return value


def validate_selection(name: str, value: Optional[str]) -> Optional[str]:
    if value is None:
        return None
    if not SELECTION_PATTERN.fullmatch(value) or value in {".", ".."}:
        raise InvalidProjectArgument(
            f"{name} must contain only ASCII letters, digits, '.', '_', or '-'"
        )
    return value


def resolve_target(context: SdkContext, target: str) -> str:
    target_id = validate_selection("target ID", target)
    assert target_id is not None
    target_path = context.resource("components") / "soc" / target_id
    if not target_path.is_dir():
        raise TargetNotFound(
            f"SDK Target does not exist: {target_id} "
            f"(expected {target_path})"
        )
    manifest_path = target_path / "ecos-soc.yml"
    try:
        manifest = yaml.safe_load(manifest_path.read_text(encoding="utf-8"))
    except OSError as exc:
        raise TargetNotFound(
            f"SDK Target manifest does not exist or cannot be read: {manifest_path}"
        ) from exc
    except yaml.YAMLError as exc:
        raise TargetNotFound(
            f"SDK Target manifest is invalid YAML: {manifest_path}: {exc}"
        ) from exc
    if (
        not isinstance(manifest, dict)
        or manifest.get("schema") != 1
        or manifest.get("id") != target_id
    ):
        raise TargetNotFound(
            f"SDK Target manifest schema or ID is invalid: {manifest_path}"
        )
    return target_id


def _read_board_identity(path: Path) -> tuple[str, tuple[str, ...], str]:
    try:
        manifest = yaml.safe_load(path.read_text(encoding="utf-8"))
    except OSError as exc:
        raise BoardManifestError(f"cannot read Board manifest {path}: {exc}") from exc
    except yaml.YAMLError as exc:
        raise BoardManifestError(f"invalid Board manifest YAML {path}: {exc}") from exc
    if not isinstance(manifest, dict):
        raise BoardManifestError(f"Board manifest must be a mapping: {path}")
    board_id = manifest.get("id")
    target = manifest.get("target")
    aliases = manifest.get("aliases", [])
    if not isinstance(board_id, str) or not board_id:
        raise BoardManifestError(f"Board manifest has no top-level id: {path}")
    if target is not None and not isinstance(target, str):
        raise BoardManifestError(f"Board target must be a string: {path}")
    if not isinstance(aliases, list) or not all(
        isinstance(alias, str) for alias in aliases
    ):
        raise BoardManifestError(f"Board aliases must be a string list: {path}")
    validate_selection("board ID", board_id)
    if target:
        validate_selection("target ID", target)
    for alias in aliases:
        validate_selection("board alias", alias)
    return board_id, tuple(aliases), target or ""


def resolve_board(context: SdkContext, board: str) -> tuple[str, str]:
    requested = validate_selection("board ID", board)
    assert requested is not None
    matches: list[tuple[str, str]] = []
    boards_root = context.resource("boards")
    for manifest_path in sorted(boards_root.glob("*/ecos-board.yml")):
        board_id, aliases, target = _read_board_identity(manifest_path)
        if requested == board_id or requested in aliases:
            matches.append((board_id, target))
    if not matches:
        raise BoardNotFound(f"SDK Board does not exist: {requested}")
    if len(matches) > 1:
        raise BoardManifestError(
            f"Board ID or alias is ambiguous in the SDK: {requested}"
        )
    board_id, target = matches[0]
    if not target:
        raise BoardManifestError(
            f"Board {board_id!r} has no top-level target mapping"
        )
    return board_id, resolve_target(context, target)


def resolve_example(context: SdkContext, example_name: str) -> Path:
    examples_root = context.resource("examples").resolve()
    candidates: set[Path] = set()

    def add_candidate(path: Path) -> None:
        resolved = path.resolve()
        try:
            resolved.relative_to(examples_root)
        except ValueError as exc:
            raise InvalidProjectArgument(
                f"Example resolves outside the SDK examples directory: {path}"
            ) from exc
        candidates.add(resolved)

    direct = (examples_root / example_name).resolve()
    if direct.is_dir():
        add_candidate(direct)

    for manifest_path in examples_root.rglob("ecos-example.yml"):
        try:
            manifest = yaml.safe_load(manifest_path.read_text(encoding="utf-8"))
        except (OSError, yaml.YAMLError) as exc:
            raise ProjectMetadataError(
                f"cannot parse Example manifest {manifest_path}: {exc}"
            ) from exc
        if isinstance(manifest, dict) and manifest.get("name") == example_name:
            add_candidate(manifest_path.parent)

    if not candidates:
        raise ExampleNotFound(
            f"SDK Example does not exist: {example_name}"
        )
    if len(candidates) > 1:
        locations = ", ".join(str(path) for path in sorted(candidates))
        raise ExampleAmbiguous(
            f"SDK Example name is ambiguous: {example_name} ({locations})"
        )
    return candidates.pop()


def project_metadata(
    context: SdkContext,
    *,
    example_name: str,
    project_name: str,
    board: Optional[str],
    target: Optional[str],
    profile: Optional[str],
) -> dict[str, Any]:
    return {
        "schema": PROJECT_SCHEMA_VERSION,
        "name": project_name,
        "example": example_name,
        "board": board,
        "target": target,
        "profile": profile,
        "sdk": {
            "id": context.sdk_id,
            "version": context.version,
            "registration": context.registration_name,
        },
    }


def _yaml_scalar(value: Any) -> str:
    if value is None:
        return "null"
    if isinstance(value, int):
        return str(value)
    if isinstance(value, str):
        return json.dumps(value, ensure_ascii=False)
    raise TypeError(f"unsupported project metadata value: {type(value).__name__}")


def serialize_project_metadata(metadata: dict[str, Any]) -> str:
    sdk = metadata["sdk"]
    return "\n".join(
        (
            f"schema: {_yaml_scalar(metadata['schema'])}",
            f"name: {_yaml_scalar(metadata['name'])}",
            f"example: {_yaml_scalar(metadata['example'])}",
            f"board: {_yaml_scalar(metadata['board'])}",
            f"target: {_yaml_scalar(metadata['target'])}",
            f"profile: {_yaml_scalar(metadata['profile'])}",
            "sdk:",
            f"  id: {_yaml_scalar(sdk['id'])}",
            f"  version: {_yaml_scalar(sdk['version'])}",
            f"  registration: {_yaml_scalar(sdk['registration'])}",
            "",
        )
    )


def load_project_metadata(project_root: Path) -> tuple[Path, dict[str, Any]]:
    root = project_root.expanduser().resolve()
    metadata_path = root / ".ecos" / "project.yml"
    if not root.is_dir() or not metadata_path.is_file():
        raise ProjectNotFound(
            f"ECOS project metadata does not exist: {metadata_path}"
        )
    try:
        metadata = yaml.safe_load(metadata_path.read_text(encoding="utf-8"))
    except OSError as exc:
        raise ProjectMetadataError(
            f"cannot read project metadata {metadata_path}: {exc}"
        ) from exc
    except yaml.YAMLError as exc:
        raise ProjectMetadataError(
            f"invalid project metadata YAML {metadata_path}: {exc}"
        ) from exc
    if not isinstance(metadata, dict):
        raise ProjectMetadataError(
            f"project metadata must be a mapping: {metadata_path}"
        )
    schema = metadata.get("schema")
    if schema not in {1, PROJECT_SCHEMA_VERSION}:
        raise ProjectMetadataError(
            f"unsupported project metadata schema {schema!r}: {metadata_path}"
        )
    if schema == 1:
        metadata.setdefault("target", None)
    required = {"schema", "name", "example", "board", "target", "profile"}
    missing = sorted(required.difference(metadata))
    sdk = metadata.get("sdk")
    if (
        missing
        or not isinstance(sdk, dict)
        or not {"id", "version", "registration"}.issubset(sdk)
    ):
        details = ", ".join(missing) if missing else "sdk"
        raise ProjectMetadataError(
            f"project metadata is missing required fields: {details}"
        )
    allowed = required | {"target", "sdk"}
    unknown = sorted(set(metadata).difference(allowed))
    if unknown:
        raise ProjectMetadataError(
            f"project metadata contains unsupported fields: {', '.join(unknown)}"
        )
    return metadata_path, metadata


def _write_metadata_atomic(path: Path, metadata: dict[str, Any]) -> None:
    temporary_path: Optional[Path] = None
    try:
        with tempfile.NamedTemporaryFile(
            "w",
            encoding="utf-8",
            newline="\n",
            dir=path.parent,
            prefix=f".{path.name}.",
            suffix=".tmp",
            delete=False,
        ) as temporary:
            temporary.write(serialize_project_metadata(metadata))
            temporary_path = Path(temporary.name)
        temporary_path.replace(path)
    except OSError as exc:
        if temporary_path is not None:
            temporary_path.unlink(missing_ok=True)
        raise ProjectMetadataError(
            f"cannot update project metadata {path}: {exc}"
        ) from exc


def set_board(
    context: SdkContext, board: str, *, project_root: Optional[Path] = None
) -> dict[str, Any]:
    board_id, target = resolve_board(context, board)
    metadata_path, metadata = load_project_metadata(project_root or Path.cwd())
    previous = {"board": metadata["board"], "target": metadata["target"]}
    metadata["schema"] = PROJECT_SCHEMA_VERSION
    metadata["board"] = board_id
    metadata["target"] = target
    _write_metadata_atomic(metadata_path, metadata)
    return {
        "path": str(metadata_path.parent.parent),
        "metadata": str(metadata_path),
        "board": board_id,
        "target": target,
        "previous": previous,
        "changed": previous != {"board": board_id, "target": target},
    }


def set_target(
    context: SdkContext, target: str, *, project_root: Optional[Path] = None
) -> dict[str, Any]:
    target_id = resolve_target(context, target)
    metadata_path, metadata = load_project_metadata(project_root or Path.cwd())
    previous = {"board": metadata["board"], "target": metadata["target"]}
    metadata["schema"] = PROJECT_SCHEMA_VERSION
    metadata["board"] = None
    metadata["target"] = target_id
    _write_metadata_atomic(metadata_path, metadata)
    return {
        "path": str(metadata_path.parent.parent),
        "metadata": str(metadata_path),
        "board": None,
        "target": target_id,
        "previous": previous,
        "changed": previous != {"board": None, "target": target_id},
    }


def _path_exists(path: Path) -> bool:
    return os.path.lexists(path)


def _remove_path(path: Path) -> None:
    if path.is_dir() and not path.is_symlink():
        shutil.rmtree(path)
    else:
        path.unlink(missing_ok=True)


def create_project(
    context: SdkContext,
    example: str,
    *,
    name: Optional[str] = None,
    parent: Optional[Path] = None,
    board: Optional[str] = None,
    target: Optional[str] = None,
    profile: Optional[str] = None,
    dry_run: bool = False,
    force: bool = False,
) -> dict[str, Any]:
    example_name = normalize_example_name(example)
    project_name = validate_project_name(
        name if name is not None else example_name
    )
    if board is not None and target is not None:
        raise InvalidProjectArgument("--board and --target are mutually exclusive")
    selected_board: Optional[str] = None
    selected_target: Optional[str] = None
    if board is not None:
        selected_board, selected_target = resolve_board(context, board)
    elif target is not None:
        selected_target = resolve_target(context, target)
    selected_profile = validate_selection("profile", profile)
    source = resolve_example(context, example_name)
    project_parent = (parent or Path.cwd()).expanduser().resolve()
    if not project_parent.is_dir():
        raise InvalidProjectArgument(
            f"project parent directory does not exist: {project_parent}"
        )
    destination = project_parent / project_name
    destination_exists = _path_exists(destination)
    if destination_exists and not force:
        raise ProjectConflict(f"project path already exists: {destination}")

    metadata = project_metadata(
        context,
        example_name=example_name,
        project_name=project_name,
        board=selected_board,
        target=selected_target,
        profile=selected_profile,
    )
    data: dict[str, Any] = {
        "name": project_name,
        "example": example_name,
        "source": str(source),
        "path": str(destination),
        "metadata": str(destination / ".ecos" / "project.yml"),
        "board": selected_board,
        "target": selected_target,
        "profile": selected_profile,
        "sdk": {
            "sdk_id": context.sdk_id,
            "sdk_version": context.version,
            "registration_name": context.registration_name,
        },
        "replaced": destination_exists,
        "force": force,
        "dry_run": dry_run,
        "changed": False,
    }
    if dry_run:
        return data

    try:
        temporary = Path(
            tempfile.mkdtemp(prefix=f".{project_name}.ecos-create-", dir=project_parent)
        )
    except OSError as exc:
        raise ProjectWriteError(
            f"cannot create temporary project in {project_parent}: {exc}"
        ) from exc
    backup: Optional[Path] = None
    installed = False
    try:
        shutil.copytree(
            source, temporary, dirs_exist_ok=True, copy_function=shutil.copy2
        )
        metadata_path = temporary / ".ecos" / "project.yml"
        metadata_path.parent.mkdir(parents=True, exist_ok=True)
        with metadata_path.open("w", encoding="utf-8", newline="\n") as metadata_file:
            metadata_file.write(serialize_project_metadata(metadata))
        if context.registration_name:
            write_project_pin(temporary, context)

        if destination_exists:
            backup = project_parent / (
                f".{project_name}.ecos-backup-{uuid.uuid4().hex}"
            )
            destination.replace(backup)
        try:
            temporary.replace(destination)
            installed = True
        except OSError:
            if (
                backup is not None
                and _path_exists(backup)
                and not _path_exists(destination)
            ):
                backup.replace(destination)
                backup = None
            raise
        if backup is not None:
            _remove_path(backup)
            backup = None
    except (OSError, shutil.Error, SdkResolutionError) as exc:
        raise ProjectWriteError(f"cannot create project {destination}: {exc}") from exc
    finally:
        if not installed and _path_exists(temporary):
            _remove_path(temporary)
        if (
            backup is not None
            and _path_exists(backup)
            and not _path_exists(destination)
        ):
            backup.replace(destination)

    data["changed"] = True
    return data
