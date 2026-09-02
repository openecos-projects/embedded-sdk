"""Cross-platform firmware flashing through Board-declared providers."""

from __future__ import annotations

import ctypes
import os
import shutil
import string
import sys
import time
from pathlib import Path
from typing import Any, Iterable, Mapping, Optional

from . import artifacts
from . import configuration
from .project_model import ProjectModelError, resolve_project
from .sdk_context import SdkContext


class FlashError(RuntimeError):
    """Base error for firmware flashing."""


class FlashConfigurationError(FlashError):
    """The selected Board has no usable flash configuration."""


class FlashDeviceNotFound(FlashError):
    """The configured programmer volume cannot be found."""


class FlashArtifactError(FlashError):
    """The build artifact is absent, stale, or damaged."""


class FlashWriteError(FlashError):
    """Firmware could not be copied to the programmer."""


def _windows_volumes() -> list[tuple[Path, str]]:
    if os.name != "nt":
        return []
    kernel32 = ctypes.windll.kernel32  # type: ignore[attr-defined]
    mask = kernel32.GetLogicalDrives()
    volumes: list[tuple[Path, str]] = []
    for index, letter in enumerate(string.ascii_uppercase):
        if not mask & (1 << index):
            continue
        root = f"{letter}:\\"
        label = ctypes.create_unicode_buffer(261)
        filesystem = ctypes.create_unicode_buffer(261)
        serial_number = ctypes.c_ulong()
        maximum_component = ctypes.c_ulong()
        flags = ctypes.c_ulong()
        ok = kernel32.GetVolumeInformationW(
            ctypes.c_wchar_p(root),
            label,
            len(label),
            ctypes.byref(serial_number),
            ctypes.byref(maximum_component),
            ctypes.byref(flags),
            filesystem,
            len(filesystem),
        )
        if ok:
            volumes.append((Path(root), label.value))
    return volumes


def _posix_volume_candidates(
    label: str,
    *,
    roots: Optional[Iterable[Path]] = None,
    environ: Optional[Mapping[str, str]] = None,
) -> list[Path]:
    env = os.environ if environ is None else environ
    if roots is None:
        user = env.get("USER") or env.get("USERNAME")
        defaults = [Path("/Volumes"), Path("/run/media"), Path("/media"), Path("/mnt")]
        roots = defaults
        direct = []
        if user:
            direct.extend([Path("/run/media") / user, Path("/media") / user])
        roots = [*direct, *roots]
    candidates: set[Path] = set()
    for root in roots:
        direct = root / label
        if direct.is_dir():
            candidates.add(direct.resolve())
        if not root.is_dir():
            continue
        try:
            children = list(root.iterdir())
        except OSError:
            continue
        for child in children:
            if child.is_dir() and child.name.casefold() == label.casefold():
                candidates.add(child.resolve())
            nested = child / label
            if nested.is_dir():
                candidates.add(nested.resolve())
    return sorted(candidates)


def discover_mass_storage(
    volume_label: str,
    *,
    platform_name: Optional[str] = None,
    roots: Optional[Iterable[Path]] = None,
) -> list[Path]:
    platform = sys.platform if platform_name is None else platform_name
    if platform == "win32":
        return sorted(
            path.resolve()
            for path, label in _windows_volumes()
            if label.casefold() == volume_label.casefold()
        )
    return _posix_volume_candidates(volume_label, roots=roots)


def _select_device(config: dict[str, Any], explicit: Optional[Path]) -> Path:
    if explicit is not None:
        device = explicit.expanduser().resolve()
        if not device.is_dir():
            raise FlashDeviceNotFound(f"flash device directory does not exist: {device}")
        return device
    label = config.get("volume_label")
    if not isinstance(label, str) or not label:
        raise FlashConfigurationError(
            "mass-storage flash provider requires Board.flash.volume_label"
        )
    devices = discover_mass_storage(label)
    if not devices:
        raise FlashDeviceNotFound(
            f"no mounted programmer volume has label {label!r}; connect the board "
            "or pass --device DIRECTORY"
        )
    if len(devices) > 1:
        locations = ", ".join(str(item) for item in devices)
        raise FlashDeviceNotFound(
            f"multiple programmer volumes have label {label!r}: {locations}; "
            "pass --device DIRECTORY"
        )
    return devices[0]


def _copy_and_sync(source: Path, destination: Path) -> None:
    if source.resolve() == destination.resolve():
        raise FlashWriteError(
            f"flash destination would overwrite the source artifact: {source}"
        )
    try:
        with source.open("rb") as input_stream, destination.open("wb") as output_stream:
            shutil.copyfileobj(input_stream, output_stream, length=1024 * 1024)
            output_stream.flush()
            os.fsync(output_stream.fileno())
    except OSError as exc:
        raise FlashWriteError(
            f"cannot copy firmware {source} to programmer {destination}: {exc}"
        ) from exc


def flash_project(
    context: SdkContext,
    *,
    project_root: Optional[Path] = None,
    device: Optional[Path] = None,
) -> dict[str, Any]:
    root = (project_root or Path.cwd()).expanduser().resolve()
    try:
        current = resolve_project(context, project_root=root)
    except ProjectModelError as exc:
        raise FlashConfigurationError(str(exc)) from exc
    try:
        resolved = configuration.load_resolved_project(root)
    except configuration.ConfigurationError as exc:
        raise FlashArtifactError(str(exc)) from exc
    if resolved.get("source_fingerprint") != current["source_fingerprint"]:
        raise FlashArtifactError(
            "generated project configuration is stale; run 'ecos build'"
        )
    board = resolved.get("board")
    if not isinstance(board, dict):
        raise FlashConfigurationError(
            "flashing requires a selected Board; Target-only projects have no flash provider"
        )
    config = board.get("flash")
    if not isinstance(config, dict):
        raise FlashConfigurationError(
            f"Board {board['id']!r} does not declare a flash provider"
        )
    provider = config.get("provider")
    if provider != "mass-storage":
        raise FlashConfigurationError(
            f"Board {board['id']!r} uses unsupported flash provider {provider!r}"
        )
    try:
        manifest = artifacts.load_manifest(root, verify=True)
    except artifacts.ArtifactError as exc:
        raise FlashArtifactError(str(exc)) from exc
    if manifest.get("source_fingerprint") != resolved["source_fingerprint"]:
        raise FlashArtifactError(
            "build artifacts are stale for the current project inputs; run 'ecos build'"
        )
    if manifest.get("configuration_fingerprint") != resolved["configuration"]["fingerprint"]:
        raise FlashArtifactError(
            "build artifacts are stale for the current configuration; run 'ecos build'"
        )
    if manifest.get("project", {}).get("board") != board["id"]:
        raise FlashArtifactError(
            "build artifacts were produced for a different Board; run 'ecos build'"
        )
    artifact_name = config.get("artifact", "bin")
    item = manifest["files"].get(artifact_name)
    if not isinstance(item, dict):
        raise FlashArtifactError(
            f"artifact manifest does not contain required {artifact_name!r} output"
        )
    source = (root / item["path"]).resolve()
    destination_root = _select_device(config, device)
    destination = destination_root / source.name
    copy_count = config.get("copy_count", 1)
    settle_seconds = float(config.get("settle_seconds", 0))
    completed = 0
    for attempt in range(copy_count):
        if attempt and not destination_root.is_dir():
            break
        _copy_and_sync(source, destination)
        completed += 1
        if attempt + 1 < copy_count and settle_seconds:
            time.sleep(settle_seconds)
    if completed < 1:
        raise FlashWriteError(
            f"programmer disappeared before firmware was copied: {destination_root}"
        )
    return {
        "path": str(root),
        "board": board["id"],
        "target": resolved["project"]["target"],
        "provider": provider,
        "artifact": str(source),
        "sha256": item["sha256"],
        "configuration_fingerprint": resolved["configuration"]["fingerprint"],
        "device": str(destination_root),
        "destination": str(destination),
        "copies": completed,
    }
