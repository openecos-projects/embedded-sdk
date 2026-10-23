import hashlib
import io
import os
import sys
import tarfile
import tempfile
import unittest
from pathlib import Path


SOURCE_ROOT = Path(__file__).parents[1] / "src"
sys.path.insert(0, str(SOURCE_ROOT))

from ecos_cli import managed_tools  # noqa: E402


def write_wlink_archive(path: Path, *, host_root: str = "wlink-linux-x64") -> str:
    payload = b"#!/bin/sh\nexit 0\n"
    with tarfile.open(path, "w:gz") as archive:
        member = tarfile.TarInfo(f"{host_root}/wlink")
        member.size = len(payload)
        member.mode = 0o755
        archive.addfile(member, io.BytesIO(payload))
    return hashlib.sha256(path.read_bytes()).hexdigest()


def wlink_manifest(digest: str) -> dict:
    return {
        "schema_version": 1,
        "id": "wlink",
        "name": "wlink test",
        "release": "0.1.3",
        "base_url": "file:///nonexistent",
        "executable": "wlink",
        "hosts": {
            "linux-x86_64": {
                "file": "wlink.tar.gz",
                "archive": "tar.gz",
                "archive_root": "wlink-linux-x64",
                "sha256": digest,
            }
        },
    }


class WlinkManagedToolTest(unittest.TestCase):
    def test_windows_default_asset_uses_wdm_compatible_x86_build(self):
        manifest = managed_tools.load_wlink_manifest()
        asset = manifest["hosts"]["windows-x86_64"]
        self.assertEqual(asset["file"], "wlink-v0.1.3-win-x86.zip")
        self.assertEqual(asset["archive_root"], "wlink-win-x86")
        self.assertEqual(
            asset["sha256"],
            "c51e2f060cf71abfb773ac5efa0003377e4cb4256f7bd18869b5f9c28c3b683c",
        )

    def test_install_reuse_and_resolve(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            archive = root / "wlink.tar.gz"
            digest = write_wlink_archive(archive)
            manifest = wlink_manifest(digest)
            prefix = root / "host"

            installed = managed_tools.install_wlink(
                prefix=prefix,
                cache=root / "cache",
                archive_override=archive,
                manifest=manifest,
                host="linux-x86_64",
            )
            self.assertEqual(installed["state"], "installed")
            executable = Path(installed["executable"])
            self.assertTrue(executable.is_file())
            if os.name != "nt":
                self.assertTrue(executable.stat().st_mode & 0o111)
            self.assertTrue((Path(installed["root"]) / managed_tools.WLINK_MARKER_NAME).is_file())
            self.assertEqual(
                managed_tools.resolve_wlink(prefix, host="linux-x86_64", manifest=manifest),
                executable,
            )

            reused = managed_tools.install_wlink(
                prefix=prefix,
                cache=root / "cache",
                archive_override=archive,
                manifest=manifest,
                host="linux-x86_64",
            )
            self.assertEqual(reused["state"], "installed")
            self.assertFalse(reused["changed"])

    def test_bad_archive_digest_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            archive = root / "wlink.tar.gz"
            digest = write_wlink_archive(archive)
            manifest = wlink_manifest(digest)
            manifest["hosts"]["linux-x86_64"]["sha256"] = "0" * 64
            with self.assertRaises(managed_tools.WlinkIntegrityError):
                managed_tools.install_wlink(
                    prefix=root / "host",
                    cache=root / "cache",
                    archive_override=archive,
                    manifest=manifest,
                    host="linux-x86_64",
                )

    def test_unsupported_host_is_not_an_error(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            archive = root / "wlink.tar.gz"
            digest = write_wlink_archive(archive)
            result = managed_tools.install_wlink(
                prefix=root / "host",
                cache=root / "cache",
                archive_override=archive,
                manifest=wlink_manifest(digest),
                host="macos-x86_64",
            )
            self.assertEqual(result["state"], "unsupported")
            self.assertFalse(result["changed"])

    def test_tampered_marker_requires_force(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            archive = root / "wlink.tar.gz"
            digest = write_wlink_archive(archive)
            manifest = wlink_manifest(digest)
            prefix = root / "host"
            installed = managed_tools.install_wlink(
                prefix=prefix,
                cache=root / "cache",
                archive_override=archive,
                manifest=manifest,
                host="linux-x86_64",
            )
            marker = Path(installed["root"]) / managed_tools.WLINK_MARKER_NAME
            marker.write_text("{}\n", encoding="utf-8")
            with self.assertRaises(managed_tools.ManagedToolError):
                managed_tools.install_wlink(
                    prefix=prefix,
                    cache=root / "cache",
                    archive_override=archive,
                    manifest=manifest,
                    host="linux-x86_64",
                )
            forced = managed_tools.install_wlink(
                prefix=prefix,
                cache=root / "cache",
                archive_override=archive,
                manifest=manifest,
                host="linux-x86_64",
                force=True,
            )
            self.assertTrue(forced["changed"])


if __name__ == "__main__":
    unittest.main()
