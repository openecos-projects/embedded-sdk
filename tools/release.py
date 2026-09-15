#!/usr/bin/env python3
"""Build versioned ECOS SDK release archives from a Git tag."""

from __future__ import annotations

import argparse
import gzip
import hashlib
import json
import re
import shutil
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path
from typing import Any, Sequence


TAG_PATTERN = re.compile(
    r"^v(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)"
    r"(?:[-+][0-9A-Za-z.-]+)?$"
)
ARTIFACT_ID_PATTERN = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]*$")
MANIFEST_PATH = "tools/sdk-manifest.json"

# Keep this list aligned with the source inputs consumed by ecos_cli.installer.
# Legacy bin/ commands and repository-only metadata are intentionally excluded.
RELEASE_PATHS = (
    "LICENSE",
    "README.md",
    "README_EN.md",
    "CODE_OF_CONDUCT.md",
    "components",
    "drivers",
    "hal",
    "templates",
    "environments",
    "third_party",
    "board",
    "example",
    "docs",
    "devices",
    "tools/fixdep",
    "tools/kconfig",
    "tools/scripts",
    "tools/toolchains",
    "tools/ecos_cli",
    "tools/ecos.py",
    "tools/install.py",
    MANIFEST_PATH,
)


class ReleaseError(RuntimeError):
    """The requested release cannot be built safely."""


def _git(
    repository: Path, arguments: Sequence[str], *, text: bool = False
) -> subprocess.CompletedProcess[Any]:
    try:
        return subprocess.run(
            ["git", "-C", str(repository), *arguments],
            check=True,
            capture_output=True,
            text=text,
        )
    except FileNotFoundError as exc:
        raise ReleaseError("git is required to build an SDK release") from exc
    except subprocess.CalledProcessError as exc:
        detail = (
            exc.stderr.strip()
            if isinstance(exc.stderr, str)
            else exc.stderr.decode("utf-8", errors="replace").strip()
        )
        command = " ".join(arguments)
        raise ReleaseError(
            f"git {command} failed: {detail or 'unknown error'}"
        ) from exc


def _resolve_commit(repository: Path, ref: str) -> str:
    result = _git(
        repository,
        ["rev-parse", "--verify", "--end-of-options", f"{ref}^{{commit}}"],
        text=True,
    )
    return result.stdout.strip()


def _git_to_file(repository: Path, arguments: Sequence[str], destination: Path) -> None:
    try:
        output = destination.open("wb")
    except OSError as exc:
        raise ReleaseError(
            f"cannot write release archive {destination}: {exc}"
        ) from exc
    try:
        with output:
            result = subprocess.run(
                ["git", "-C", str(repository), *arguments],
                check=False,
                stdout=output,
                stderr=subprocess.PIPE,
            )
    except FileNotFoundError as exc:
        raise ReleaseError("git is required to build an SDK release") from exc
    if result.returncode != 0:
        detail = result.stderr.decode("utf-8", errors="replace").strip()
        command = " ".join(arguments)
        raise ReleaseError(f"git {command} failed: {detail or 'unknown error'}")


def _manifest_at(repository: Path, commit: str) -> dict[str, Any]:
    result = _git(repository, ["show", f"{commit}:{MANIFEST_PATH}"], text=True)
    try:
        manifest = json.loads(result.stdout)
    except json.JSONDecodeError as exc:
        raise ReleaseError(f"cannot parse {MANIFEST_PATH}: {exc}") from exc
    if not isinstance(manifest, dict):
        raise ReleaseError(f"{MANIFEST_PATH} must contain a JSON object")
    return manifest


def _validate_identity(manifest: dict[str, Any], tag: str) -> tuple[str, str]:
    if not TAG_PATTERN.fullmatch(tag):
        raise ReleaseError(
            f"release tag {tag!r} is invalid; expected v<major>.<minor>.<patch>"
        )
    sdk_id = manifest.get("sdk_id")
    version = manifest.get("sdk_version")
    channel = manifest.get("channel")
    if not isinstance(sdk_id, str) or not ARTIFACT_ID_PATTERN.fullmatch(sdk_id):
        raise ReleaseError(f"invalid sdk_id in {MANIFEST_PATH}: {sdk_id!r}")
    if not isinstance(version, str) or tag != f"v{version}":
        raise ReleaseError(
            f"release tag {tag!r} does not match sdk_version {version!r}"
        )
    if channel != "release":
        raise ReleaseError(
            f"release tag requires channel 'release', found {channel!r}"
        )
    return sdk_id, version


def _archive_command(commit: str, archive_format: str, prefix: str) -> list[str]:
    return [
        "archive",
        f"--format={archive_format}",
        f"--prefix={prefix}/",
        commit,
        "--",
        *RELEASE_PATHS,
    ]


def _write_archives(
    repository: Path, commit: str, prefix: str, destination: Path
) -> tuple[Path, Path]:
    raw_tar = destination / f".{prefix}.tar"
    _git_to_file(repository, _archive_command(commit, "tar", prefix), raw_tar)
    tar_path = destination / f"{prefix}.tar.gz"
    try:
        with raw_tar.open("rb") as source, tar_path.open("wb") as output:
            with gzip.GzipFile(
                fileobj=output, mode="wb", filename="", mtime=0
            ) as bundle:
                shutil.copyfileobj(source, bundle, length=1024 * 1024)
    finally:
        raw_tar.unlink(missing_ok=True)

    zip_path = destination / f"{prefix}.zip"
    _git_to_file(repository, _archive_command(commit, "zip", prefix), zip_path)
    return tar_path, zip_path


def _validate_archive(archive: Path, prefix: str) -> None:
    if archive.suffix == ".zip":
        with zipfile.ZipFile(archive) as bundle:
            names = set(bundle.namelist())
    else:
        import tarfile

        with tarfile.open(archive, "r:gz") as bundle:
            names = set(bundle.getnames())
    required = {
        f"{prefix}/{MANIFEST_PATH}",
        f"{prefix}/tools/install.py",
        f"{prefix}/LICENSE",
    }
    missing = sorted(required - names)
    if missing:
        raise ReleaseError(
            f"archive {archive.name} is incomplete: {', '.join(missing)}"
        )
    contains_legacy_bin = any(
        name == f"{prefix}/bin" or name.startswith(f"{prefix}/bin/")
        for name in names
    )
    if contains_legacy_bin:
        raise ReleaseError(f"archive {archive.name} contains legacy bin/ commands")


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def build_release(
    repository: Path, *, tag: str, ref: str, output_dir: Path
) -> dict[str, Any]:
    repository = repository.resolve()
    if not (repository / ".git").exists():
        raise ReleaseError(f"not a Git repository: {repository}")
    if not TAG_PATTERN.fullmatch(tag):
        raise ReleaseError(
            f"release tag {tag!r} is invalid; expected v<major>.<minor>.<patch>"
        )

    commit = _resolve_commit(repository, ref)
    tag_commit = _resolve_commit(repository, f"refs/tags/{tag}")
    if commit != tag_commit:
        raise ReleaseError(
            f"ref {ref!r} resolves to {commit}, but tag {tag!r} resolves to "
            f"{tag_commit}"
        )

    manifest = _manifest_at(repository, commit)
    sdk_id, version = _validate_identity(manifest, tag)
    prefix = f"{sdk_id}-{version}"
    output_dir = output_dir.resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory(
        prefix=".ecos-release-", dir=output_dir
    ) as temporary:
        staging = Path(temporary)
        archives = _write_archives(repository, commit, prefix, staging)
        for archive in archives:
            _validate_archive(archive, prefix)
        checksums = staging / "SHA256SUMS"
        checksums.write_text(
            "".join(f"{_sha256(path)}  {path.name}\n" for path in archives),
            encoding="ascii",
        )
        outputs = [*archives, checksums]
        for path in outputs:
            path.replace(output_dir / path.name)

    return {
        "sdk_id": sdk_id,
        "version": version,
        "tag": tag,
        "revision": commit,
        "artifacts": [str(output_dir / path.name) for path in outputs],
    }


def create_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--tag",
        required=True,
        help="existing release tag, for example v3.0.0",
    )
    parser.add_argument(
        "--ref",
        default="HEAD",
        help="Git ref to package (default: HEAD)",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("dist"),
        help="artifact output directory",
    )
    parser.add_argument(
        "--repository-root",
        type=Path,
        default=Path(__file__).resolve().parent.parent,
        help=argparse.SUPPRESS,
    )
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = create_parser().parse_args(argv)
    try:
        result = build_release(
            args.repository_root,
            tag=args.tag,
            ref=args.ref,
            output_dir=args.output_dir,
        )
    except ReleaseError as exc:
        print(f"release error: {exc}", file=sys.stderr)
        return 1
    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
