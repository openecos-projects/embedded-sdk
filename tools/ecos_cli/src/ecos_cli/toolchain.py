"""Cross-platform discovery and installation for ECOS toolchains."""

from __future__ import annotations

import hashlib
import json
import os
import platform
import shutil
import subprocess
import sys
import tarfile
import tempfile
import time
import urllib.request
import zipfile
from importlib import resources
from pathlib import Path, PurePosixPath
from typing import Any, Callable, Optional

from .progress import DownloadStatus


MANIFEST_NAME = "xpack-riscv-none-elf-gcc.json"
ProgressCallback = Callable[[str], None]
DownloadProgressCallback = Callable[[DownloadStatus], None]


class ToolchainError(RuntimeError):
    """Base class for toolchain management failures."""


class ManifestError(ToolchainError):
    """The toolchain manifest is missing or invalid."""


class UnsupportedHostError(ToolchainError):
    """No pinned asset exists for the current host."""


class IntegrityError(ToolchainError):
    """A downloaded or imported archive failed validation."""


class DownloadError(ToolchainError):
    """A remote toolchain asset could not be downloaded."""


def _parse_manifest(content: str, source: str) -> dict[str, Any]:
    try:
        manifest = json.loads(content)
    except json.JSONDecodeError as exc:
        raise ManifestError(f"cannot parse toolchain manifest {source}: {exc}") from exc

    required = {
        "schema_version",
        "id",
        "name",
        "release",
        "base_url",
        "tool_prefix",
        "compiler",
        "target_triplet",
        "install_path",
        "active_path",
        "archive_root",
        "hosts",
    }
    missing = sorted(required - manifest.keys())
    if missing:
        raise ManifestError(f"toolchain manifest is missing: {', '.join(missing)}")
    if manifest["schema_version"] != 1:
        raise ManifestError(
            f"unsupported toolchain manifest schema: {manifest['schema_version']}"
        )
    if not isinstance(manifest["hosts"], dict) or not manifest["hosts"]:
        raise ManifestError("toolchain manifest does not define any host assets")
    return manifest


def load_manifest(path: Optional[Path] = None) -> dict[str, Any]:
    if path is not None:
        try:
            return _parse_manifest(path.read_text(encoding="utf-8"), str(path))
        except OSError as exc:
            raise ManifestError(f"cannot read toolchain manifest {path}: {exc}") from exc

    resource = resources.files("ecos_cli.resources.toolchains").joinpath(MANIFEST_NAME)
    try:
        return _parse_manifest(resource.read_text(encoding="utf-8"), MANIFEST_NAME)
    except OSError as exc:
        raise ManifestError(f"cannot read packaged toolchain manifest: {exc}") from exc


def detect_host(system: Optional[str] = None, machine: Optional[str] = None) -> str:
    system_name = (system or platform.system()).lower()
    machine_name = (machine or platform.machine()).lower()
    systems = {
        "linux": "linux",
        "darwin": "macos",
        "windows": "windows",
        "win32": "windows",
        "msys": "windows",
        "cygwin": "windows",
    }
    architectures = {
        "x86_64": "x86_64",
        "amd64": "x86_64",
        "arm64": "arm64",
        "aarch64": "arm64",
    }
    host_system = systems.get(system_name)
    host_arch = architectures.get(machine_name)
    if not host_system or not host_arch:
        raise UnsupportedHostError(
            f"unsupported host platform: system={system_name}, architecture={machine_name}"
        )
    return f"{host_system}-{host_arch}"


def select_asset(manifest: dict[str, Any], host: str) -> dict[str, Any]:
    try:
        asset = manifest["hosts"][host]
    except KeyError as exc:
        supported = ", ".join(sorted(manifest["hosts"]))
        raise UnsupportedHostError(
            f"unsupported host {host}; supported hosts: {supported}"
        ) from exc
    required = {"file", "archive", "sha256"}
    missing = sorted(required - asset.keys())
    if missing:
        raise ManifestError(
            f"toolchain asset {host} is missing: {', '.join(missing)}"
        )
    return asset


def selection_data(manifest: dict[str, Any], host: str) -> dict[str, Any]:
    asset = select_asset(manifest, host)
    return {
        "provider": manifest.get("provider", "unknown"),
        "id": manifest["id"],
        "name": manifest["name"],
        "release": manifest["release"],
        "package_version": manifest.get("package_version"),
        "host": host,
        "tool_prefix": manifest["tool_prefix"],
        "target_triplet": manifest["target_triplet"],
        "asset": {
            "file": asset["file"],
            "archive": asset["archive"],
            "sha256": asset["sha256"],
            "url": f"{manifest['base_url'].rstrip('/')}/{asset['file']}",
        },
        "targets": manifest.get("targets", []),
    }


def default_prefix() -> Path:
    return Path.home() / ".local" / "ecos-sdk"


def default_cache_root() -> Path:
    if sys.platform == "win32":
        base = Path(os.environ.get("LOCALAPPDATA", Path.home() / "AppData" / "Local"))
        return base / "ECOS" / "Cache" / "toolchains"
    if sys.platform == "darwin":
        return Path.home() / "Library" / "Caches" / "ecos" / "toolchains"
    return Path(os.environ.get("XDG_CACHE_HOME", Path.home() / ".cache")) / "ecos" / "toolchains"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def verify_archive(path: Path, expected: str) -> None:
    actual = sha256(path)
    if actual != expected:
        raise IntegrityError(
            f"SHA-256 mismatch for {path}: expected {expected}, got {actual}"
        )


def download(
    url: str,
    destination: Path,
    progress: Optional[DownloadProgressCallback] = None,
) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    temporary = destination.with_name(f".{destination.name}.part")
    request = urllib.request.Request(
        url, headers={"User-Agent": "ECOS-SDK-toolchain-installer"}
    )
    started = time.monotonic()
    downloaded = 0
    total: Optional[int] = None
    last_update = started
    try:
        with urllib.request.urlopen(request, timeout=30) as response, temporary.open(
            "wb"
        ) as output:
            content_length = response.headers.get("Content-Length")
            try:
                total = int(content_length) if content_length else None
            except (TypeError, ValueError):
                total = None
            if total is not None and total <= 0:
                total = None
            if progress:
                progress(DownloadStatus(0, total, 0.0))
            while True:
                chunk = response.read(1024 * 1024)
                if not chunk:
                    break
                output.write(chunk)
                downloaded += len(chunk)
                now = time.monotonic()
                if progress and now - last_update >= 0.2:
                    progress(DownloadStatus(downloaded, total, now - started))
                    last_update = now
            if progress:
                progress(
                    DownloadStatus(downloaded, total, time.monotonic() - started, done=True)
                )
        temporary.replace(destination)
    except Exception as exc:
        if progress:
            progress(
                DownloadStatus(
                    downloaded,
                    total,
                    time.monotonic() - started,
                    done=True,
                    failed=True,
                )
            )
        temporary.unlink(missing_ok=True)
        raise DownloadError(f"cannot download {url}: {exc}") from exc


def _stripped_path(name: str, archive_root: str) -> Optional[Path]:
    path = PurePosixPath(name.replace("\\", "/"))
    if path.is_absolute() or ".." in path.parts:
        raise IntegrityError(f"unsafe archive member: {name}")
    if not path.parts or path.parts[0] != archive_root:
        raise IntegrityError(f"unexpected archive root for member: {name}")
    relative = Path(*path.parts[1:])
    return relative if relative.parts else None


def _safe_destination(root: Path, relative: Path) -> Path:
    destination = (root / relative).resolve(strict=False)
    if destination != root and root not in destination.parents:
        raise IntegrityError(f"archive member escapes extraction directory: {relative}")
    return destination


def extract_tar(archive: Path, destination: Path, archive_root: str) -> None:
    pending_hardlinks: list[tuple[Path, Path]] = []
    with tarfile.open(archive, "r:gz") as bundle:
        for member in bundle.getmembers():
            relative = _stripped_path(member.name, archive_root)
            if relative is None:
                continue
            target = _safe_destination(destination, relative)
            target.parent.mkdir(parents=True, exist_ok=True)
            if member.isdir():
                target.mkdir(exist_ok=True)
            elif member.isfile():
                source = bundle.extractfile(member)
                if source is None:
                    raise IntegrityError(f"cannot extract archive member: {member.name}")
                with source, target.open("wb") as output:
                    shutil.copyfileobj(source, output)
                target.chmod(member.mode & 0o777)
            elif member.issym():
                link_target = (target.parent / member.linkname).resolve(strict=False)
                if link_target != destination and destination not in link_target.parents:
                    raise IntegrityError(f"unsafe symbolic link: {member.name}")
                target.symlink_to(member.linkname)
            elif member.islnk():
                source_relative = _stripped_path(member.linkname, archive_root)
                if source_relative is None:
                    raise IntegrityError(f"invalid hard link: {member.name}")
                pending_hardlinks.append(
                    (target, _safe_destination(destination, source_relative))
                )
            else:
                raise IntegrityError(f"unsupported archive member type: {member.name}")
    for target, source in pending_hardlinks:
        if not source.is_file():
            raise IntegrityError(f"hard link target is missing: {source}")
        os.link(source, target)


def extract_zip(archive: Path, destination: Path, archive_root: str) -> None:
    with zipfile.ZipFile(archive) as bundle:
        for member in bundle.infolist():
            relative = _stripped_path(member.filename, archive_root)
            if relative is None:
                continue
            target = _safe_destination(destination, relative)
            if member.is_dir():
                target.mkdir(parents=True, exist_ok=True)
                continue
            target.parent.mkdir(parents=True, exist_ok=True)
            with bundle.open(member) as source, target.open("wb") as output:
                shutil.copyfileobj(source, output)
            mode = (member.external_attr >> 16) & 0o777
            if mode:
                target.chmod(mode)


def extract_archive(
    archive: Path, destination: Path, archive_type: str, archive_root: str
) -> None:
    if archive_type == "tar.gz":
        extract_tar(archive, destination, archive_root)
    elif archive_type == "zip":
        extract_zip(archive, destination, archive_root)
    else:
        raise ManifestError(f"unsupported archive type: {archive_type}")


def compiler_path(root: Path, manifest: dict[str, Any], host: str) -> Path:
    suffix = ".exe" if host.startswith("windows-") else ""
    return root / "bin" / f"{manifest['compiler']}{suffix}"


def probe_compiler(
    root: Path,
    manifest: dict[str, Any],
    host: str,
    require_release: bool = True,
) -> dict[str, Any]:
    compiler = compiler_path(root, manifest, host)
    result: dict[str, Any] = {
        "path": str(compiler),
        "exists": compiler.is_file(),
        "valid": False,
        "version": None,
        "target_triplet": None,
        "matches_release": False,
    }
    if not compiler.is_file():
        return result
    try:
        machine = subprocess.run(
            [compiler, "-dumpmachine"],
            check=True,
            capture_output=True,
            text=True,
        ).stdout.strip()
        version = subprocess.run(
            [compiler, "--version"],
            check=True,
            capture_output=True,
            text=True,
        ).stdout.splitlines()[0]
    except (OSError, subprocess.SubprocessError, IndexError) as exc:
        result["error"] = str(exc)
        return result
    gcc_version = manifest["release"].split("-", 1)[0]
    matches_release = gcc_version in version
    result.update(
        {
            "version": version,
            "target_triplet": machine,
            "matches_release": matches_release,
            "valid": machine == manifest["target_triplet"]
            and (matches_release or not require_release),
        }
    )
    return result


def installation_status(
    manifest: dict[str, Any], prefix: Path, host: str, custom: Optional[Path] = None
) -> dict[str, Any]:
    if custom is not None:
        root = custom.expanduser().resolve()
        probe = probe_compiler(root, manifest, host, require_release=False)
        return {
            "provider": "custom",
            "pinned": False,
            "state": "installed" if probe["valid"] else "invalid",
            "root": str(root),
            "compiler": probe,
        }

    root = prefix / manifest["install_path"]
    marker_path = root / ".ecos-toolchain.json"
    marker: Optional[dict[str, Any]] = None
    if marker_path.is_file():
        try:
            marker = json.loads(marker_path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError):
            marker = None
    probe = probe_compiler(root, manifest, host)
    marker_valid = bool(
        marker
        and marker.get("id") == manifest["id"]
        and marker.get("release") == manifest["release"]
        and marker.get("host") == host
        and marker.get("sha256") == select_asset(manifest, host)["sha256"]
    )
    if probe["valid"] and marker_valid:
        state = "installed"
    elif root.exists():
        state = "invalid"
    else:
        state = "missing"
    return {
        "provider": manifest.get("provider", "unknown"),
        "pinned": True,
        "state": state,
        "root": str(root),
        "active_root": str(prefix / manifest["active_path"]),
        "marker": marker,
        "compiler": probe,
    }


def add_legacy_aliases(root: Path, manifest: dict[str, Any]) -> None:
    old_prefix = manifest.get("legacy_tool_prefix")
    new_prefix = manifest["tool_prefix"]
    if not old_prefix:
        return
    for source in (root / "bin").glob(f"{new_prefix}*"):
        if not source.is_file():
            continue
        alias = source.with_name(old_prefix + source.name[len(new_prefix) :])
        if alias.exists() or alias.is_symlink():
            continue
        try:
            if os.name == "nt":
                os.link(source, alias)
            else:
                alias.symlink_to(source.name)
        except OSError as exc:
            raise ToolchainError(f"cannot create compatibility command {alias}: {exc}") from exc


def activate_install(target: Path, active: Path, force: bool = False) -> None:
    active.parent.mkdir(parents=True, exist_ok=True)
    if active.exists():
        try:
            if active.resolve() == target.resolve():
                return
        except OSError:
            pass
    if active.is_symlink():
        active.unlink()
    elif active.exists():
        legacy = active.parent / "legacy-riscv64-unknown-elf"
        if not legacy.exists():
            active.replace(legacy)
        elif force:
            shutil.rmtree(active)
        else:
            raise ToolchainError(
                f"cannot replace existing toolchain path {active}; rerun with --force"
            )
    try:
        active.symlink_to(os.path.relpath(target, active.parent), target_is_directory=True)
    except OSError as exc:
        if os.name != "nt":
            raise ToolchainError(f"cannot activate toolchain at {active}: {exc}") from exc
        result = subprocess.run(
            ["cmd", "/c", "mklink", "/J", str(active), str(target)],
            check=False,
            capture_output=True,
            text=True,
        )
        if result.returncode != 0:
            raise ToolchainError(
                f"cannot create Windows toolchain junction at {active}: "
                f"{result.stderr.strip()}"
            )


def install_toolchain(
    manifest: dict[str, Any],
    prefix: Path,
    host: str,
    cache: Path,
    archive_override: Optional[Path] = None,
    force: bool = False,
    progress: Optional[ProgressCallback] = None,
    download_progress: Optional[DownloadProgressCallback] = None,
) -> dict[str, Any]:
    asset = select_asset(manifest, host)
    prefix = prefix.expanduser().resolve()
    cache = cache.expanduser().resolve()
    target = prefix / manifest["install_path"]
    active = prefix / manifest["active_path"]
    current = installation_status(manifest, prefix, host)
    if current["state"] == "installed" and not force:
        activate_install(target, active)
        current["changed"] = False
        return current
    if current["state"] == "invalid" and not force:
        raise ToolchainError(
            f"invalid toolchain installation exists at {target}; rerun with --force"
        )

    archive = (
        archive_override.expanduser().resolve()
        if archive_override is not None
        else cache / manifest["id"] / manifest["release"] / asset["file"]
    )
    if archive_override is None:
        url = f"{manifest['base_url'].rstrip('/')}/{asset['file']}"
        if archive.exists():
            try:
                verify_archive(archive, asset["sha256"])
            except IntegrityError:
                archive.unlink()
        if not archive.exists():
            if progress:
                progress(f"Downloading {url}")
            download(url, archive, progress=download_progress)
    elif not archive.is_file():
        raise IntegrityError(f"offline archive does not exist: {archive}")

    if progress:
        progress(f"Verifying SHA-256 for {archive}")
    verify_archive(archive, asset["sha256"])
    target.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".ecos-toolchain-", dir=target.parent) as temporary:
        staging = Path(temporary)
        if progress:
            progress(f"Extracting {asset['file']}")
        extract_archive(archive, staging, asset["archive"], manifest["archive_root"])
        probe = probe_compiler(staging, manifest, host)
        if not probe["valid"]:
            raise IntegrityError(
                f"archive does not contain a usable {manifest['compiler']} "
                f"{manifest['release']} compiler for {manifest['target_triplet']}"
            )
        marker = {
            "id": manifest["id"],
            "release": manifest["release"],
            "host": host,
            "sha256": asset["sha256"],
        }
        (staging / ".ecos-toolchain.json").write_text(
            json.dumps(marker, indent=2) + "\n", encoding="utf-8"
        )
        add_legacy_aliases(staging, manifest)
        if target.exists() or target.is_symlink():
            if target.is_symlink() or target.is_file():
                target.unlink()
            else:
                shutil.rmtree(target)
        staging.replace(target)
    activate_install(target, active, force=force)
    result = installation_status(manifest, prefix, host)
    result["changed"] = True
    result["archive"] = str(archive)
    return result
