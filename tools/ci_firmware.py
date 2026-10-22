#!/usr/bin/env python3
"""Package verified firmware binaries produced by the CI build matrix."""

from __future__ import annotations

import hashlib
import json
import shutil
import tarfile
from collections.abc import Mapping
from pathlib import Path
from typing import Any


class FirmwarePackageError(RuntimeError):
    """Raised when a matrix artifact cannot be packaged safely."""


def _digest(path: Path) -> tuple[int, str]:
    digest = hashlib.sha256()
    size = 0
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            size += len(chunk)
            digest.update(chunk)
    return size, digest.hexdigest()


def _component(value: Any, label: str) -> str:
    if not isinstance(value, str) or not value:
        raise FirmwarePackageError(f"artifact has invalid {label}: {value!r}")
    if value in {".", ".."} or Path(value).name != value or "/" in value or "\\" in value:
        raise FirmwarePackageError(f"artifact has unsafe {label}: {value!r}")
    return value


class FirmwarePackage:
    """Collect matrix build records and write a machine-readable firmware bundle."""

    def __init__(self, output_dir: Path) -> None:
        self.output_dir = Path(output_dir)
        self.stage_dir = self.output_dir / "firmware"
        self.archive_path = self.output_dir / "ecos-firmware-matrix.tar.gz"
        self._entries: dict[str, dict[str, Any]] = {}

        self.output_dir.mkdir(parents=True, exist_ok=True)
        if self.stage_dir.exists():
            if not self.stage_dir.is_dir():
                raise FirmwarePackageError(f"firmware output is not a directory: {self.stage_dir}")
            shutil.rmtree(self.stage_dir)
        if self.archive_path.exists():
            if self.archive_path.is_dir():
                raise FirmwarePackageError(f"firmware archive is a directory: {self.archive_path}")
            self.archive_path.unlink()
        self.stage_dir.mkdir()

    def add(self, project: Path, record: Mapping[str, Any]) -> None:
        """Copy one verified build artifact into the package staging directory."""
        example = _component(record.get("example"), "example")
        board = _component(record.get("board"), "board")
        filename = f"{example}-{board}.bin"
        if filename in self._entries:
            raise FirmwarePackageError(f"duplicate firmware output: {filename}")

        binary = record.get("bin")
        if not isinstance(binary, Mapping):
            raise FirmwarePackageError(f"{example}/{board} has no binary artifact metadata")
        source_value = binary.get("path")
        if not isinstance(source_value, str) or not source_value:
            raise FirmwarePackageError(f"{example}/{board} has no binary artifact path")
        source = Path(source_value)
        if not source.is_absolute():
            source = project / source
        if not source.is_file():
            raise FirmwarePackageError(f"binary artifact does not exist: {source}")

        size, sha256 = _digest(source)
        expected_size = binary.get("size")
        expected_sha256 = binary.get("sha256")
        if expected_size is not None and size != expected_size:
            raise FirmwarePackageError(
                f"binary size mismatch for {example}/{board}: {size} != {expected_size}"
            )
        if expected_sha256 is not None and sha256 != expected_sha256:
            raise FirmwarePackageError(
                f"binary checksum mismatch for {example}/{board}: {sha256} != {expected_sha256}"
            )

        destination = self.stage_dir / filename
        shutil.copy2(source, destination)
        readme_name = f"{example}-{board}.README.md"
        readme_destination = self.stage_dir / readme_name
        readme_source = project / "README.md"
        readme_missing = not readme_source.is_file()
        if readme_missing:
            readme_destination.write_text(
                f"# {example} — {board}\n\n"
                f"对应固件：`{filename}`\n\n"
                "> 注意：该例程未提供原始 README.md。本文件由 CI 自动生成，"
                "不是测试说明或实机验收通过记录。\n\n"
                "测试前请联系维护者补充接线、操作步骤及预期结果；"
                "仅构建成功不能证明实机功能正常。\n",
                encoding="utf-8",
            )
        else:
            shutil.copy2(readme_source, readme_destination)
        readme_size, readme_sha256 = _digest(readme_destination)
        self._entries[filename] = {
            "name": filename.removesuffix(".bin"),
            "file": filename,
            "example": example,
            "board": board,
            "target": record.get("target"),
            "profile": record.get("profile"),
            "isa": record.get("isa"),
            "abi": record.get("abi"),
            "size": size,
            "sha256": sha256,
            "readme": {
                "file": readme_name,
                "size": readme_size,
                "sha256": readme_sha256,
                "generated": readme_missing,
            },
            "image_size_limit": record.get("image_size_limit"),
            "sdk": record.get("sdk"),
            "toolchain": record.get("toolchain"),
            "source_fingerprint": record.get("source_fingerprint"),
            "configuration_fingerprint": record.get("configuration_fingerprint"),
        }

    def finalize(self) -> dict[str, Any]:
        """Write the manifest, checksums, and archive, returning their paths."""
        if not self._entries:
            raise FirmwarePackageError("cannot create an empty firmware package")

        artifacts = [self._entries[name] for name in sorted(self._entries)]
        manifest = {
            "schema_version": 1,
            "format": "ecos-firmware-matrix",
            "count": len(artifacts),
            "artifacts": artifacts,
        }
        manifest_path = self.stage_dir / "manifest.json"
        manifest_path.write_text(
            json.dumps(manifest, indent=2, ensure_ascii=False, sort_keys=True) + "\n",
            encoding="utf-8",
        )
        checksums_path = self.stage_dir / "SHA256SUMS"
        checksums_path.write_text(
            "".join(
                f"{entry['sha256']}  {entry['file']}\n"
                f"{entry['readme']['sha256']}  {entry['readme']['file']}\n"
                for entry in artifacts
            ),
            encoding="ascii",
        )

        with tarfile.open(self.archive_path, "w:gz") as archive:
            archive.add(self.stage_dir, arcname="firmware")
        return {
            "count": len(artifacts),
            "directory": str(self.stage_dir),
            "archive": str(self.archive_path),
            "manifest": str(manifest_path),
            "checksums": str(checksums_path),
        }
