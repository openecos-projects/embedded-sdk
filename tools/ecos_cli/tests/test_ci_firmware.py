import hashlib
import json
import sys
import tarfile
import tempfile
import unittest
from pathlib import Path


TOOLS_ROOT = Path(__file__).resolve().parents[2]
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from ci_firmware import FirmwarePackage, FirmwarePackageError


class FirmwarePackageTest(unittest.TestCase):
    def _record(self, project: Path, example: str, board: str, payload: bytes) -> dict:
        binary = project / "build/firmware.bin"
        binary.parent.mkdir(parents=True)
        binary.write_bytes(payload)
        return {
            "example": example,
            "board": board,
            "target": "firmware",
            "profile": "default",
            "isa": "rv32imac",
            "abi": "ilp32",
            "image_size_limit": 65536,
            "sdk": {"id": "embedded-sdk", "version": "3.0.0.dev0"},
            "toolchain": {"id": "riscv-none-elf"},
            "source_fingerprint": "source-hash",
            "configuration_fingerprint": "config-hash",
            "bin": {
                "path": "build/firmware.bin",
                "size": len(payload),
                "sha256": hashlib.sha256(payload).hexdigest(),
            },
        }

    def test_writes_named_binaries_manifest_checksums_and_archive(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            package = FirmwarePackage(root / "output")
            project_a = root / "project-a"
            project_b = root / "project-b"
            project_a.mkdir()
            (project_a / "README.md").write_text("# Hello\n测试步骤与预期输出\n", encoding="utf-8")
            package.add(
                project_a,
                self._record(project_a, "hello", "starrysky-c1", b"hello"),
            )
            package.add(
                project_b,
                self._record(project_b, "uart", "starrysky-c2", b"uart"),
            )

            summary = package.finalize()
            stage = root / "output/firmware"
            self.assertEqual(summary["count"], 2)
            self.assertEqual((stage / "hello-starrysky-c1.bin").read_bytes(), b"hello")
            self.assertEqual((stage / "uart-starrysky-c2.bin").read_bytes(), b"uart")

            manifest = json.loads((stage / "manifest.json").read_text(encoding="utf-8"))
            self.assertEqual(manifest["format"], "ecos-firmware-matrix")
            self.assertEqual(
                [entry["file"] for entry in manifest["artifacts"]],
                ["hello-starrysky-c1.bin", "uart-starrysky-c2.bin"],
            )
            self.assertEqual(
                manifest["artifacts"][0]["configuration_fingerprint"], "config-hash"
            )
            checksums = (stage / "SHA256SUMS").read_text(encoding="ascii").splitlines()
            self.assertEqual(len(checksums), 4)
            self.assertTrue(checksums[0].endswith("  hello-starrysky-c1.bin"))
            readme = stage / "hello-starrysky-c1.README.md"
            self.assertEqual(readme.read_bytes(), (project_a / "README.md").read_bytes())
            self.assertFalse(manifest["artifacts"][0]["readme"]["generated"])
            self.assertTrue(manifest["artifacts"][1]["readme"]["generated"])
            self.assertIn("不是测试说明", (stage / "uart-starrysky-c2.README.md").read_text(encoding="utf-8"))
            for line in checksums:
                digest, filename = line.split("  ", 1)
                self.assertEqual(hashlib.sha256((stage / filename).read_bytes()).hexdigest(), digest)
            with tarfile.open(root / "output/ecos-firmware-matrix.tar.gz", "r:gz") as archive:
                names = archive.getnames()
            self.assertIn("firmware/hello-starrysky-c1.bin", names)
            self.assertIn("firmware/manifest.json", names)
            self.assertIn("firmware/hello-starrysky-c1.README.md", names)
            self.assertIn("firmware/uart-starrysky-c2.README.md", names)

    def test_rejects_size_mismatch(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            package = FirmwarePackage(root / "output")
            project = root / "project"
            record = self._record(project, "hello", "starrysky-c1", b"hello")
            record["bin"]["size"] += 1
            with self.assertRaises(FirmwarePackageError):
                package.add(project, record)

    def test_rejects_checksum_mismatch(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            package = FirmwarePackage(root / "output")
            project = root / "project"
            record = self._record(project, "hello", "starrysky-c1", b"hello")
            record["bin"]["sha256"] = "0" * 64
            with self.assertRaises(FirmwarePackageError):
                package.add(project, record)

    def test_rejects_unsafe_output_name(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            package = FirmwarePackage(root / "output")
            project = root / "project"
            with self.assertRaises(FirmwarePackageError):
                package.add(
                    project,
                    self._record(project, "../hello", "starrysky-c1", b"hello"),
                )

    def test_rejects_duplicate_output_name(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            package = FirmwarePackage(root / "output")
            project_a = root / "project-a"
            project_b = root / "project-b"
            package.add(
                project_a,
                self._record(project_a, "hello", "starrysky-c1", b"one"),
            )
            with self.assertRaises(FirmwarePackageError):
                package.add(
                    project_b,
                    self._record(project_b, "hello", "starrysky-c1", b"two"),
                )


if __name__ == "__main__":
    unittest.main()
