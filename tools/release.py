#!/usr/bin/env python3
"""Build versioned ECOS SDK release archives from a Git tag."""

from __future__ import annotations

import argparse
import ast
import gzip
import hashlib
import json
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
import zipfile
from datetime import datetime, timezone
from pathlib import Path, PurePosixPath, PureWindowsPath
from typing import Any, Optional, Sequence


TAG_PATTERN = re.compile(
    r"^v(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)"
    r"(?:[-+][0-9A-Za-z.-]+)?$"
)
ARTIFACT_ID_PATTERN = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]*$")
MANIFEST_PATH = "tools/sdk-manifest.json"

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


def _release_selection(
    manifest: dict[str, Any]
) -> tuple[tuple[str, ...], tuple[str, ...]]:
    release = manifest.get("release")
    if (
        not isinstance(release, dict)
        or "paths" not in release
        or set(release) - {"paths", "exclude"}
    ):
        raise ReleaseError(f"{MANIFEST_PATH} release must define paths and optional exclude")
    declared = release["paths"]
    if not isinstance(declared, list) or not declared:
        raise ReleaseError(f"{MANIFEST_PATH} release.paths must be a non-empty list")
    paths: list[str] = []
    for value in declared:
        if not isinstance(value, str) or not value or "\\" in value:
            raise ReleaseError(f"invalid SDK release path: {value!r}")
        path = PurePosixPath(value)
        if (
            path.is_absolute()
            or PureWindowsPath(value).drive
            or any(part in {"", ".", ".."} for part in value.split("/"))
        ):
            raise ReleaseError(f"unsafe SDK release path: {value!r}")
        normalized = path.as_posix()
        if (
            normalized in {"bin", "tools", "tools/ecos_cli"}
            or normalized.startswith("bin/")
            or any(
                normalized == legacy or normalized.startswith(f"{legacy}/")
                for legacy in (
                    "tools/fixdep", "tools/kconfig", "tools/scripts", "tools/ecos_cli/tests"
                )
            )
        ):
            raise ReleaseError(f"legacy path cannot enter an SDK release: {normalized}")
        if normalized in paths or any(
            normalized.startswith(f"{previous}/")
            or previous.startswith(f"{normalized}/")
            for previous in paths
        ):
            raise ReleaseError(f"duplicate or overlapping SDK release path: {normalized}")
        paths.append(normalized)
    for required in ("LICENSE", "tools/install.py", MANIFEST_PATH):
        if required not in paths:
            raise ReleaseError(f"SDK release paths must include {required}")
    layout = manifest.get("layout", {})
    if isinstance(layout, dict):
        for name, relative in layout.items():
            if not isinstance(relative, str) or not any(
                relative == path or relative.startswith(f"{path}/") for path in paths
            ):
                raise ReleaseError(
                    f"SDK release paths do not include layout resource {name}: {relative}"
                )
    declared_excludes = release.get("exclude", [])
    if not isinstance(declared_excludes, list):
        raise ReleaseError("SDK release.exclude must be a list")
    excludes: list[str] = []
    for value in declared_excludes:
        if not isinstance(value, str) or not value or "\\" in value:
            raise ReleaseError(f"invalid SDK release exclusion: {value!r}")
        excluded = PurePosixPath(value)
        if (
            excluded.is_absolute()
            or PureWindowsPath(value).drive
            or any(part in {"", ".", ".."} for part in value.split("/"))
        ):
            raise ReleaseError(f"unsafe SDK release exclusion: {value!r}")
        normalized = excluded.as_posix()
        if normalized in excludes or not any(
            normalized.startswith(f"{path}/") for path in paths
        ):
            raise ReleaseError(f"invalid SDK release exclusion: {normalized}")
        if any(
            relative == normalized or relative.startswith(f"{normalized}/")
            for relative in layout.values()
        ):
            raise ReleaseError(f"SDK release exclusion removes a layout resource: {normalized}")
        excludes.append(normalized)
    build = manifest.get("build", {})
    if not isinstance(build, dict):
        raise ReleaseError("SDK manifest build must be an object")
    if "kconfig" in build:
        kconfig = build["kconfig"]
        if (
            not isinstance(kconfig, str)
            or not kconfig
            or "\\" in kconfig
            or PurePosixPath(kconfig).is_absolute()
            or PureWindowsPath(kconfig).drive
            or any(part in {"", ".", ".."} for part in kconfig.split("/"))
            or not any(kconfig == path or kconfig.startswith(f"{path}/") for path in paths)
            or any(kconfig == path or kconfig.startswith(f"{path}/") for path in excludes)
        ):
            raise ReleaseError(f"invalid or excluded build.kconfig: {kconfig!r}")
    return tuple(paths), tuple(excludes)


def _archive_command(
    commit: str, archive_format: str, prefix: str,
    paths: tuple[str, ...], excludes: tuple[str, ...],
) -> list[str]:
    return [
        "archive",
        f"--format={archive_format}",
        f"--prefix={prefix}/",
        commit,
        "--",
        *paths,
        *(f":(exclude){path}" for path in excludes),
    ]


def _write_archives(
    repository: Path, commit: str, prefix: str, destination: Path,
    paths: tuple[str, ...], excludes: tuple[str, ...],
) -> tuple[Path, Path]:
    raw_tar = destination / f".{prefix}.tar"
    _git_to_file(repository, _archive_command(commit, "tar", prefix, paths, excludes), raw_tar)
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
    _git_to_file(repository, _archive_command(commit, "zip", prefix, paths, excludes), zip_path)
    return tar_path, zip_path


def _validate_archive(
    archive: Path, prefix: str,
    paths: tuple[str, ...], excludes: tuple[str, ...],
    build_kconfig: Optional[str] = None,
) -> None:
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
    if build_kconfig is not None:
        required.add(f"{prefix}/{build_kconfig}")
    missing = sorted(required - names)
    if missing:
        raise ReleaseError(
            f"archive {archive.name} is incomplete: {', '.join(missing)}"
        )
    for name in names:
        if name.rstrip("/") == prefix:
            continue
        relative = name.removeprefix(f"{prefix}/").rstrip("/")
        if not relative:
            continue
        if any(
            relative == excluded or relative.startswith(f"{excluded}/")
            for excluded in excludes
        ):
            raise ReleaseError(f"archive {archive.name} contains excluded path: {name}")
        if not any(
            relative == path
            or relative.startswith(f"{path}/")
            or path.startswith(f"{relative}/")
            for path in paths
        ):
            raise ReleaseError(f"archive {archive.name} contains undeclared path: {name}")
    for path in paths:
        if not any(
            name.rstrip("/") == f"{prefix}/{path}"
            or name.startswith(f"{prefix}/{path}/")
            for name in names
        ):
            raise ReleaseError(f"archive {archive.name} is missing declared path: {path}")


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def _pinned_dependencies(source: bytes) -> list[tuple[str, str, str]]:
    try:
        tree = ast.parse(source.decode("utf-8"))
        values: dict[str, Any] = {}
        for node in tree.body:
            if isinstance(node, ast.Assign):
                targets = node.targets
            elif isinstance(node, ast.AnnAssign):
                targets = [node.target]
            else:
                continue
            for target in targets:
                if isinstance(target, ast.Name) and target.id in {
                    "PYTHON_DEPENDENCIES", "HOST_TOOL_DEPENDENCIES"
                }:
                    values[target.id] = ast.literal_eval(node.value)
        requirements = [*values["PYTHON_DEPENDENCIES"]]
        requirements.extend(
            item["requirement"] for item in values["HOST_TOOL_DEPENDENCIES"]
        )
    except (KeyError, TypeError, ValueError, SyntaxError, UnicodeError) as exc:
        raise ReleaseError("cannot read pinned host dependencies from SDK archive") from exc
    parsed: list[tuple[str, str, str]] = []
    for requirement in requirements:
        if not isinstance(requirement, str):
            raise ReleaseError("SDK host dependency is not a pinned requirement")
        name, separator, rest = requirement.partition("==")
        version, _, marker = rest.partition(";")
        if not separator or not name or not version:
            raise ReleaseError(f"SDK host dependency is not pinned: {requirement}")
        parsed.append((name, version.strip(), marker.strip()))
    return parsed


def _build_sbom(
    repository: Path, commit: str, archive: Path,
    sdk_id: str, version: str, prefix: str, destination: Path,
) -> None:
    timestamp = _git(
        repository, ["show", "-s", "--format=%cI", commit], text=True
    ).stdout.strip()
    created = datetime.fromisoformat(timestamp).astimezone(timezone.utc).strftime(
        "%Y-%m-%dT%H:%M:%SZ"
    )
    files: list[dict[str, Any]] = []
    sha1_values: list[str] = []
    metadata: dict[str, bytes] = {}
    with tarfile.open(archive, "r:gz") as bundle:
        for member in sorted(bundle.getmembers(), key=lambda item: item.name):
            if member.isdir():
                continue
            if not member.isfile() or not member.name.startswith(f"{prefix}/"):
                raise ReleaseError(f"cannot inventory release member: {member.name}")
            source = bundle.extractfile(member)
            if source is None:
                raise ReleaseError(f"cannot read release member: {member.name}")
            with source:
                content = source.read()
            relative = member.name[len(prefix) + 1:]
            if relative in {MANIFEST_PATH, "tools/ecos_cli/src/ecos_cli/dependencies.py"} or (
                relative.startswith("tools/ecos_cli/src/ecos_cli/resources/toolchains/")
                and relative.endswith(".json")
            ):
                metadata[relative] = content
            sha1 = hashlib.sha1(content).hexdigest()
            sha1_values.append(sha1)
            files.append({
                "SPDXID": "SPDXRef-File-" + hashlib.sha256(
                    relative.encode("utf-8")
                ).hexdigest(),
                "fileName": f"./{relative}",
                "checksums": [
                    {"algorithm": "SHA1", "checksumValue": sha1},
                    {"algorithm": "SHA256", "checksumValue": hashlib.sha256(content).hexdigest()},
                ],
                "licenseConcluded": "NOASSERTION",
                "licenseInfoInFiles": ["NOASSERTION"],
                "copyrightText": "NOASSERTION",
            })
    verification = hashlib.sha1("".join(sorted(sha1_values)).encode("ascii")).hexdigest()
    try:
        sdk_manifest = json.loads(metadata[MANIFEST_PATH])
        toolchain_id = sdk_manifest["toolchain"]["id"]
        toolchain_release = sdk_manifest["toolchain"]["release"]
        toolchain_path = (
            "tools/ecos_cli/src/ecos_cli/resources/toolchains/"
            f"{toolchain_id}.json"
        )
        toolchain = json.loads(metadata[toolchain_path])
        if (toolchain["id"], toolchain["release"]) != (
            toolchain_id, toolchain_release
        ):
            raise ValueError("SDK and toolchain releases differ")
        dependencies = _pinned_dependencies(
            metadata["tools/ecos_cli/src/ecos_cli/dependencies.py"]
        )
    except (KeyError, TypeError, ValueError, json.JSONDecodeError) as exc:
        raise ReleaseError("cannot inventory locked SDK dependencies") from exc
    package_id = "SPDXRef-Package-SDK"
    package = {
        "SPDXID": package_id,
        "name": sdk_id,
        "versionInfo": version,
        "downloadLocation": "NOASSERTION",
        "filesAnalyzed": True,
        "licenseConcluded": "NOASSERTION",
        "licenseDeclared": "NOASSERTION",
        "licenseInfoFromFiles": ["NOASSERTION"],
        "copyrightText": "NOASSERTION",
        "packageFileName": archive.name,
        "checksums": [{"algorithm": "SHA256", "checksumValue": _sha256(archive)}],
        "packageVerificationCode": {"packageVerificationCodeValue": verification},
        "sourceInfo": f"Git commit {commit}",
        "hasFiles": [item["SPDXID"] for item in files],
    }
    external_packages: list[dict[str, Any]] = []
    relationships = [{
        "spdxElementId": "SPDXRef-DOCUMENT",
        "relationshipType": "DESCRIBES",
        "relatedSpdxElement": package_id,
    }]
    for name, locked_version, marker in [
        *dependencies, (toolchain_id, toolchain_release, "")
    ]:
        external_id = "SPDXRef-Package-External-" + hashlib.sha256(
            f"{name}=={locked_version}".encode("utf-8")
        ).hexdigest()
        external_package = {
            "SPDXID": external_id,
            "name": name,
            "versionInfo": locked_version,
            "downloadLocation": "NOASSERTION",
            "filesAnalyzed": False,
            "licenseConcluded": "NOASSERTION",
            "licenseDeclared": "NOASSERTION",
            "copyrightText": "NOASSERTION",
        }
        if marker:
            external_package["packageComment"] = f"Install condition: {marker}"
        external_packages.append(external_package)
        relationship = {
            "spdxElementId": package_id,
            "relationshipType": "DEPENDS_ON",
            "relatedSpdxElement": external_id,
        }
        if marker:
            relationship["comment"] = f"Applies when {marker}"
        relationships.append(relationship)
    sbom = {
        "spdxVersion": "SPDX-2.3",
        "dataLicense": "CC0-1.0",
        "SPDXID": "SPDXRef-DOCUMENT",
        "name": f"{sdk_id}-{version}",
        "documentNamespace": (
            f"https://spdx.org/spdxdocs/{sdk_id}-{version}-{_sha256(archive)}"
        ),
        "creationInfo": {
            "created": created,
            "creators": ["Tool: ECOS SDK release.py"],
        },
        "documentComment": (
            "File inventory of the SDK release archive and pinned host tool "
            "dependencies. License conclusions require separate review."
        ),
        "documentDescribes": [package_id],
        "packages": [package, *external_packages],
        "files": files,
        "relationships": relationships,
    }
    destination.write_text(
        json.dumps(sbom, sort_keys=True, indent=2, ensure_ascii=True) + "\n",
        encoding="utf-8",
    )


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
    paths, excludes = _release_selection(manifest)
    prefix = f"{sdk_id}-{version}"
    output_dir = output_dir.resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory(
        prefix=".ecos-release-", dir=output_dir
    ) as temporary:
        staging = Path(temporary)
        archives = _write_archives(repository, commit, prefix, staging, paths, excludes)
        for archive in archives:
            _validate_archive(
                archive, prefix, paths, excludes,
                manifest.get("build", {}).get("kconfig"),
            )
        sbom = staging / f"{prefix}.spdx.json"
        _build_sbom(repository, commit, archives[0], sdk_id, version, prefix, sbom)
        checksums = staging / "SHA256SUMS"
        checksums.write_text(
            "".join(f"{_sha256(path)}  {path.name}\n" for path in (*archives, sbom)),
            encoding="ascii",
        )
        outputs = [*archives, sbom, checksums]
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
