"""SDK identity manifest loading and validation."""

from __future__ import annotations

import json
import re
from pathlib import Path, PurePosixPath
from typing import Any, Optional

from . import __version__
from .sdk_context import SdkContext


MANIFEST_RELATIVE_PATH = Path("tools") / "sdk-manifest.json"
SDK_VERSION_PATTERN = re.compile(
    r"^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)"
    r"(?:[-+][0-9A-Za-z.-]+)?$"
)
REQUIRED_LAYOUT = {
    "boards",
    "components",
    "devices",
    "docs",
    "examples",
    "hal",
    "templates",
    "cli",
}


class SdkManifestError(RuntimeError):
    """The SDK identity manifest or resource layout is invalid."""


def _read_json(path: Path) -> dict[str, Any]:
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as exc:
        raise SdkManifestError(f"SDK manifest does not exist: {path}") from exc
    except OSError as exc:
        raise SdkManifestError(f"cannot read SDK manifest {path}: {exc}") from exc
    except json.JSONDecodeError as exc:
        raise SdkManifestError(f"cannot parse SDK manifest {path}: {exc}") from exc
    if not isinstance(payload, dict):
        raise SdkManifestError(f"SDK manifest must contain an object: {path}")
    return payload


def _validate_relative_path(name: str, value: Any) -> str:
    if not isinstance(value, str) or not value:
        raise SdkManifestError(f"SDK layout path must be a non-empty string: {name}")
    path = PurePosixPath(value.replace("\\", "/"))
    if path.is_absolute() or ".." in path.parts or "." in path.parts:
        raise SdkManifestError(f"unsafe SDK layout path for {name}: {value}")
    return path.as_posix()


def validate_manifest(manifest: dict[str, Any], source: Path) -> dict[str, Any]:
    required = {
        "schema_version",
        "sdk_id",
        "sdk_version",
        "channel",
        "cli_compatibility",
        "layout",
        "toolchain",
    }
    missing = sorted(required - manifest.keys())
    if missing:
        raise SdkManifestError(
            f"SDK manifest {source} is missing: {', '.join(missing)}"
        )
    if manifest["schema_version"] != 1:
        raise SdkManifestError(
            f"unsupported SDK manifest schema: {manifest['schema_version']}"
        )
    if not isinstance(manifest["sdk_id"], str) or not manifest["sdk_id"].strip():
        raise SdkManifestError("SDK manifest sdk_id must be a non-empty string")
    if not isinstance(manifest["sdk_version"], str) or not SDK_VERSION_PATTERN.fullmatch(
        manifest["sdk_version"]
    ):
        raise SdkManifestError(
            f"invalid SDK version in {source}: {manifest['sdk_version']!r}"
        )
    if manifest["channel"] not in {"development", "release"}:
        raise SdkManifestError(
            f"invalid SDK release channel: {manifest['channel']!r}"
        )
    compatibility = manifest["cli_compatibility"]
    if not isinstance(compatibility, dict) or not isinstance(
        compatibility.get("major"), int
    ):
        raise SdkManifestError("SDK manifest cli_compatibility.major must be an integer")
    try:
        cli_major = int(__version__.split(".", 1)[0])
    except ValueError as exc:
        raise SdkManifestError(f"invalid ECOS CLI version: {__version__}") from exc
    if compatibility["major"] != cli_major:
        raise SdkManifestError(
            f"SDK requires ECOS CLI major {compatibility['major']}, running {cli_major}"
        )
    layout = manifest["layout"]
    if not isinstance(layout, dict):
        raise SdkManifestError("SDK manifest layout must be an object")
    missing_layout = sorted(REQUIRED_LAYOUT - layout.keys())
    if missing_layout:
        raise SdkManifestError(
            f"SDK manifest layout is missing: {', '.join(missing_layout)}"
        )
    manifest["layout"] = {
        name: _validate_relative_path(name, value) for name, value in layout.items()
    }
    toolchain = manifest["toolchain"]
    if not isinstance(toolchain, dict) or not all(
        isinstance(toolchain.get(field), str) and toolchain[field]
        for field in ("id", "release")
    ):
        raise SdkManifestError("SDK manifest toolchain must define id and release")
    return manifest


def load_manifest(root: Path) -> dict[str, Any]:
    sdk_root = root.expanduser().resolve()
    path = sdk_root / MANIFEST_RELATIVE_PATH
    return validate_manifest(_read_json(path), path)


def validate_sdk_root(root: Path) -> tuple[Path, dict[str, Any]]:
    sdk_root = root.expanduser().resolve()
    manifest = load_manifest(sdk_root)
    missing = [
        name
        for name, relative in manifest["layout"].items()
        if not (sdk_root / Path(*relative.split("/"))).exists()
    ]
    if missing:
        raise SdkManifestError(
            f"SDK {sdk_root} is missing layout resources: {', '.join(sorted(missing))}"
        )
    return sdk_root, manifest


def context_from_root(
    root: Path,
    *,
    kind: str,
    source: str,
    registration_name: Optional[str] = None,
) -> SdkContext:
    sdk_root, manifest = validate_sdk_root(root)
    return SdkContext(
        root=sdk_root,
        sdk_id=manifest["sdk_id"],
        version=manifest["sdk_version"],
        channel=manifest["channel"],
        kind=kind,
        source=source,
        registration_name=registration_name,
        manifest=manifest,
    )
