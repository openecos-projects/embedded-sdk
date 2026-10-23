"""Pinned managed tools used by the ECOS SDK (currently wlink)."""

from __future__ import annotations

import json
import os
import re
import shutil
import tempfile
from importlib import resources
from pathlib import Path
from typing import Any, Callable, Optional

from . import dependencies
from . import toolchain


WLINK_MANIFEST_NAME = "wlink.json"
WLINK_RELATIVE_ROOT = Path("lib") / "ecos" / "tools" / "wlink"
WLINK_MARKER_NAME = ".ecos-wlink.json"
ProgressCallback = Callable[[str], None]
DownloadProgressCallback = toolchain.DownloadProgressCallback


class ManagedToolError(RuntimeError):
    """A managed tool could not be installed or validated."""


class WlinkManifestError(ManagedToolError):
    """The packaged wlink manifest is missing or invalid."""


class WlinkIntegrityError(ManagedToolError):
    """A downloaded or imported wlink archive failed validation."""


def _parse_manifest(content: str, source: str) -> dict[str, Any]:
    try:
        manifest = json.loads(content)
    except json.JSONDecodeError as exc:
        raise WlinkManifestError(
            f"cannot parse wlink manifest {source}: {exc}"
        ) from exc
    required = {
        "schema_version",
        "id",
        "name",
        "release",
        "base_url",
        "executable",
        "hosts",
    }
    missing = sorted(required - manifest.keys())
    if missing:
        raise WlinkManifestError(
            f"wlink manifest is missing: {', '.join(missing)}"
        )
    if manifest["schema_version"] != 1:
        raise WlinkManifestError(
            f"unsupported wlink manifest schema: {manifest['schema_version']}"
        )
    if not isinstance(manifest["hosts"], dict) or not manifest["hosts"]:
        raise WlinkManifestError("wlink manifest does not define any host assets")
    for field in ("id", "release", "executable"):
        value = manifest[field]
        if not isinstance(value, str) or not re.fullmatch(
            r"[A-Za-z0-9][A-Za-z0-9._-]*", value
        ):
            raise WlinkManifestError(f"unsafe wlink {field}: {value!r}")
    for host, asset in manifest["hosts"].items():
        if not isinstance(host, str) or not re.fullmatch(
            r"[A-Za-z0-9][A-Za-z0-9._-]*", host
        ):
            raise WlinkManifestError(f"unsafe wlink host: {host!r}")
        if not isinstance(asset, dict):
            raise WlinkManifestError(f"wlink host asset {host} must be a mapping")
        missing = sorted(
            {"file", "archive", "archive_root", "sha256"} - asset.keys()
        )
        if missing:
            raise WlinkManifestError(
                f"wlink host asset {host} is missing: {', '.join(missing)}"
            )
        if asset["archive"] not in {"tar.gz", "zip"}:
            raise WlinkManifestError(
                f"unsupported wlink archive type for {host}: {asset['archive']!r}"
            )
        if not re.fullmatch(r"[0-9a-fA-F]{64}", str(asset["sha256"])):
            raise WlinkManifestError(f"invalid wlink SHA-256 for {host}")
    return manifest


def load_wlink_manifest(path: Optional[Path] = None) -> dict[str, Any]:
    if path is not None:
        try:
            return _parse_manifest(path.read_text(encoding="utf-8"), str(path))
        except OSError as exc:
            raise WlinkManifestError(
                f"cannot read wlink manifest {path}: {exc}"
            ) from exc
    resource = resources.files("ecos_cli.resources.tools").joinpath(
        WLINK_MANIFEST_NAME
    )
    try:
        return _parse_manifest(resource.read_text(encoding="utf-8"), WLINK_MANIFEST_NAME)
    except OSError as exc:
        raise WlinkManifestError(
            f"cannot read packaged wlink manifest: {exc}"
        ) from exc


def default_prefix() -> Path:
    override = os.environ.get("ECOS_HOST_PREFIX")
    if override:
        return Path(override).expanduser()
    return dependencies.default_host_prefix()


def wlink_install_root(
    prefix: Path, manifest: dict[str, Any], host: str
) -> Path:
    return (
        prefix.expanduser().resolve()
        / WLINK_RELATIVE_ROOT
        / manifest["release"]
        / host
    )


def wlink_executable(
    root: Path, manifest: dict[str, Any], host: str
) -> Path:
    suffix = ".exe" if host.startswith("windows-") else ""
    return root / f"{manifest['executable']}{suffix}"


def _read_marker(path: Path) -> Optional[dict[str, Any]]:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return None


def wlink_status(
    prefix: Optional[Path] = None,
    manifest: Optional[dict[str, Any]] = None,
    host: Optional[str] = None,
) -> dict[str, Any]:
    manifest = manifest or load_wlink_manifest()
    host = host or toolchain.detect_host()
    root_prefix = (prefix or default_prefix()).expanduser().resolve()
    try:
        asset = toolchain.select_asset(manifest, host)
    except toolchain.UnsupportedHostError as exc:
        return {
            "provider": "wlink",
            "state": "unsupported",
            "id": manifest["id"],
            "name": manifest["name"],
            "release": manifest["release"],
            "host": host,
            "root": None,
            "executable": None,
            "error": str(exc),
        }
    root = wlink_install_root(root_prefix, manifest, host)
    executable = wlink_executable(root, manifest, host)
    marker = _read_marker(root / WLINK_MARKER_NAME)
    marker_valid = bool(
        marker
        and marker.get("id") == manifest["id"]
        and marker.get("release") == manifest["release"]
        and marker.get("host") == host
        and marker.get("sha256") == asset["sha256"]
        and marker.get("executable") == executable.name
    )
    if executable.is_file() and marker_valid:
        state = "installed"
    elif root.exists():
        state = "invalid"
    else:
        state = "missing"
    return {
        "provider": "wlink",
        "state": state,
        "id": manifest["id"],
        "name": manifest["name"],
        "release": manifest["release"],
        "host": host,
        "root": str(root),
        "executable": str(executable),
        "marker": marker,
    }


def resolve_wlink(
    prefix: Optional[Path] = None,
    *,
    host: Optional[str] = None,
    manifest: Optional[dict[str, Any]] = None,
) -> Optional[Path]:
    try:
        status = wlink_status(prefix=prefix, manifest=manifest, host=host)
    except (ManagedToolError, toolchain.ToolchainError):
        return None
    if status["state"] != "installed":
        return None
    return Path(status["executable"])


def selection_data(manifest: dict[str, Any], host: str) -> dict[str, Any]:
    try:
        asset = toolchain.select_asset(manifest, host)
    except toolchain.UnsupportedHostError as exc:
        return {
            "state": "unsupported",
            "id": manifest["id"],
            "name": manifest["name"],
            "release": manifest["release"],
            "host": host,
            "error": str(exc),
        }
    return {
        "state": "available",
        "id": manifest["id"],
        "name": manifest["name"],
        "release": manifest["release"],
        "host": host,
        "asset": {
            "file": asset["file"],
            "archive": asset["archive"],
            "sha256": asset["sha256"],
            "url": f"{manifest['base_url'].rstrip('/')}/{asset['file']}",
        },
    }


def _replace_managed_path(path: Path) -> None:
    if path.is_symlink() or path.is_file():
        path.unlink()
    elif path.exists():
        shutil.rmtree(path)


def install_wlink(
    prefix: Optional[Path] = None,
    cache: Optional[Path] = None,
    archive_override: Optional[Path] = None,
    force: bool = False,
    progress: Optional[ProgressCallback] = None,
    download_progress: Optional[DownloadProgressCallback] = None,
    *,
    manifest: Optional[dict[str, Any]] = None,
    host: Optional[str] = None,
) -> dict[str, Any]:
    manifest = manifest or load_wlink_manifest()
    host = host or toolchain.detect_host()
    root_prefix = (prefix or default_prefix()).expanduser().resolve()
    cache_root = (cache or toolchain.default_cache_root()).expanduser().resolve()
    try:
        asset = toolchain.select_asset(manifest, host)
    except toolchain.UnsupportedHostError as exc:
        return {
            "provider": "wlink",
            "state": "unsupported",
            "id": manifest["id"],
            "name": manifest["name"],
            "release": manifest["release"],
            "host": host,
            "root": None,
            "executable": None,
            "error": str(exc),
            "changed": False,
        }

    current = wlink_status(root_prefix, manifest, host)
    if current["state"] == "installed" and not force:
        current["changed"] = False
        return current
    if current["state"] == "invalid" and not force:
        raise ManagedToolError(
            f"invalid wlink installation exists at {current['root']}; "
            "rerun with --force"
        )

    archive = (
        archive_override.expanduser().resolve()
        if archive_override is not None
        else cache_root / manifest["id"] / manifest["release"] / asset["file"]
    )
    try:
        if archive_override is None:
            url = f"{manifest['base_url'].rstrip('/')}/{asset['file']}"
            if archive.exists():
                try:
                    toolchain.verify_archive(archive, asset["sha256"])
                except toolchain.IntegrityError:
                    archive.unlink()
            if not archive.exists():
                if progress:
                    progress(f"Downloading {url}")
                toolchain.download(url, archive, progress=download_progress)
        elif not archive.is_file():
            raise WlinkIntegrityError(f"offline wlink archive does not exist: {archive}")
        if progress:
            progress(f"Verifying SHA-256 for {archive}")
        toolchain.verify_archive(archive, asset["sha256"])
    except (toolchain.IntegrityError, toolchain.DownloadError) as exc:
        raise WlinkIntegrityError(str(exc)) from exc
    except OSError as exc:
        raise ManagedToolError(f"cannot prepare wlink archive: {exc}") from exc

    target = wlink_install_root(root_prefix, manifest, host)
    target.parent.mkdir(parents=True, exist_ok=True)
    try:
        with tempfile.TemporaryDirectory(
            prefix=".ecos-wlink-", dir=target.parent
        ) as temporary:
            staging = Path(temporary)
            if progress:
                progress(f"Extracting {asset['file']}")
            toolchain.extract_archive(
                archive, staging, asset["archive"], asset["archive_root"]
            )
            executable = wlink_executable(staging, manifest, host)
            if not executable.is_file():
                raise WlinkIntegrityError(
                    f"archive does not contain {executable.name} for {host}"
                )
            if os.name != "nt":
                executable.chmod(executable.stat().st_mode | 0o111)
            marker = {
                "schema_version": 1,
                "id": manifest["id"],
                "release": manifest["release"],
                "host": host,
                "sha256": asset["sha256"],
                "executable": executable.name,
            }
            (staging / WLINK_MARKER_NAME).write_text(
                json.dumps(marker, indent=2) + "\n", encoding="utf-8"
            )
            _replace_managed_path(target)
            staging.replace(target)
    except (toolchain.IntegrityError, toolchain.ManifestError) as exc:
        raise WlinkIntegrityError(str(exc)) from exc
    except OSError as exc:
        raise ManagedToolError(f"cannot install wlink: {exc}") from exc

    result = wlink_status(root_prefix, manifest, host)
    result["changed"] = True
    result["archive"] = str(archive)
    return result
