import hashlib
import importlib.util
import json
import subprocess
import tempfile
import unittest
import zipfile
from pathlib import Path


REPOSITORY_ROOT = Path(__file__).parents[3]
SPEC = importlib.util.spec_from_file_location(
    "ecos_release", REPOSITORY_ROOT / "tools" / "release.py"
)
assert SPEC is not None and SPEC.loader is not None
release = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(release)


class ReleaseBuilderTest(unittest.TestCase):
    def create_repository(
        self, root: Path, *, version: str = "1.2.3", channel: str = "release"
    ) -> None:
        subprocess.run(["git", "init", "-q", str(root)], check=True)
        subprocess.run(
            ["git", "-C", str(root), "config", "user.name", "Release Test"],
            check=True,
        )
        subprocess.run(
            ["git", "-C", str(root), "config", "user.email", "release@example.com"],
            check=True,
        )
        for relative in release.RELEASE_PATHS:
            path = root / relative
            if Path(relative).suffix or relative in {
                "LICENSE",
                "README.md",
                "README_EN.md",
                "CODE_OF_CONDUCT.md",
            }:
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(f"fixture for {relative}\n", encoding="utf-8")
            else:
                path.mkdir(parents=True, exist_ok=True)
                (path / ".keep").write_text("fixture\n", encoding="utf-8")
        manifest = {
            "schema_version": 1,
            "sdk_id": "ecos-embedded-sdk",
            "sdk_version": version,
            "channel": channel,
        }
        (root / release.MANIFEST_PATH).write_text(
            json.dumps(manifest), encoding="utf-8"
        )
        (root / "bin").mkdir()
        (root / "bin" / "legacy-command").write_text("legacy\n", encoding="utf-8")
        subprocess.run(["git", "-C", str(root), "add", "."], check=True)
        subprocess.run(
            ["git", "-C", str(root), "commit", "-qm", "release fixture"],
            check=True,
        )
        subprocess.run(
            ["git", "-C", str(root), "tag", "-a", f"v{version}", "-m", "release"],
            check=True,
        )

    def test_builds_installable_archives_and_checksums(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / "repository"
            root.mkdir()
            self.create_repository(root)
            output = root / "dist"

            result = release.build_release(
                root, tag="v1.2.3", ref="HEAD", output_dir=output
            )

            prefix = "ecos-embedded-sdk-1.2.3"
            tar_path = output / f"{prefix}.tar.gz"
            zip_path = output / f"{prefix}.zip"
            self.assertEqual(result["version"], "1.2.3")
            self.assertTrue(tar_path.is_file())
            self.assertTrue(zip_path.is_file())
            with zipfile.ZipFile(zip_path) as bundle:
                names = set(bundle.namelist())
            self.assertIn(f"{prefix}/tools/install.py", names)
            self.assertFalse(any(name.startswith(f"{prefix}/bin/") for name in names))

            checksum_lines = (output / "SHA256SUMS").read_text(
                encoding="ascii"
            ).splitlines()
            expected = {
                f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.name}"
                for path in (tar_path, zip_path)
            }
            self.assertEqual(set(checksum_lines), expected)

            release.build_release(
                root, tag="v1.2.3", ref="HEAD", output_dir=output
            )
            self.assertEqual(
                (output / "SHA256SUMS").read_text(encoding="ascii").splitlines(),
                checksum_lines,
            )

    def test_rejects_tag_that_does_not_match_manifest(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.create_repository(root, version="1.2.4")
            subprocess.run(
                ["git", "-C", str(root), "tag", "v1.2.3"], check=True
            )

            with self.assertRaisesRegex(release.ReleaseError, "does not match"):
                release.build_release(
                    root,
                    tag="v1.2.3",
                    ref="HEAD",
                    output_dir=root / "dist",
                )

    def test_rejects_development_channel(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.create_repository(root, channel="development")

            with self.assertRaisesRegex(release.ReleaseError, "channel 'release'"):
                release.build_release(
                    root,
                    tag="v1.2.3",
                    ref="HEAD",
                    output_dir=root / "dist",
                )

    def test_rejects_ref_that_is_not_the_tagged_commit(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.create_repository(root)
            (root / "README.md").write_text("next commit\n", encoding="utf-8")
            subprocess.run(["git", "-C", str(root), "add", "README.md"], check=True)
            subprocess.run(
                ["git", "-C", str(root), "commit", "-qm", "next commit"], check=True
            )

            with self.assertRaisesRegex(release.ReleaseError, "but tag"):
                release.build_release(
                    root,
                    tag="v1.2.3",
                    ref="HEAD",
                    output_dir=root / "dist",
                )


if __name__ == "__main__":
    unittest.main()
