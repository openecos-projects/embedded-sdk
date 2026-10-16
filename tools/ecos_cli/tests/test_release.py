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

FIXTURE_RELEASE_PATHS = (
    "LICENSE",
    "README.md",
    "boards",
    "tools/ecos_cli/src",
    "tools/install.py",
    release.MANIFEST_PATH,
)
FIXTURE_RELEASE_EXCLUDES = (
    "boards/UserBSP",
    "boards/StarrySkyC1/Makefile",
)


class ReleaseBuilderTest(unittest.TestCase):
    def create_repository(
        self, root: Path, *, version: str = "1.2.3", channel: str = "release",
        build_kconfig: str = "",
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
        for relative in FIXTURE_RELEASE_PATHS:
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
        (root / "boards/StarrySkyC1").mkdir(parents=True, exist_ok=True)
        (root / "boards/StarrySkyC1/Makefile").write_text(
            "legacy make entry\n", encoding="utf-8"
        )
        manifest = {
            "schema_version": 1,
            "sdk_id": "ecos-embedded-sdk",
            "sdk_version": version,
            "channel": channel,
            "toolchain": {"id": "test-toolchain", "release": "9.9.9"},
            "release": {
                "paths": list(FIXTURE_RELEASE_PATHS),
                "exclude": list(FIXTURE_RELEASE_EXCLUDES),
            },
        }
        if build_kconfig:
            manifest["build"] = {"kconfig": build_kconfig}
        (root / release.MANIFEST_PATH).write_text(
            json.dumps(manifest), encoding="utf-8"
        )
        dependencies = root / "tools/ecos_cli/src/ecos_cli/dependencies.py"
        dependencies.parent.mkdir(parents=True, exist_ok=True)
        dependencies.write_text(
            'PYTHON_DEPENDENCIES = ("PyYAML==6.0.3", '
            '"windows-curses==2.4.2; sys_platform == \'win32\'")\n'
            'HOST_TOOL_DEPENDENCIES = ({"requirement": "cmake==3.31.10"},)\n',
            encoding="utf-8",
        )
        toolchain = root / (
            "tools/ecos_cli/src/ecos_cli/resources/toolchains/test-toolchain.json"
        )
        toolchain.parent.mkdir(parents=True, exist_ok=True)
        toolchain.write_text(
            json.dumps({"id": "test-toolchain", "release": "9.9.9"}),
            encoding="utf-8",
        )
        (root / "bin").mkdir()
        (root / "bin" / "legacy-command").write_text("legacy\n", encoding="utf-8")
        (root / "tools/fixdep").mkdir()
        (root / "tools/fixdep/legacy").write_text("legacy\n", encoding="utf-8")
        (root / "tools/ecos_cli/tests").mkdir(parents=True)
        (root / "tools/ecos_cli/tests/test_legacy.py").write_text(
            "# test only\n", encoding="utf-8"
        )
        (root / "boards/UserBSP").mkdir()
        (root / "boards/UserBSP/user-board.txt").write_text(
            "user data\n", encoding="utf-8"
        )
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
            sbom_path = output / f"{prefix}.spdx.json"
            self.assertEqual(result["version"], "1.2.3")
            self.assertTrue(tar_path.is_file())
            self.assertTrue(zip_path.is_file())
            self.assertTrue(sbom_path.is_file())
            with zipfile.ZipFile(zip_path) as bundle:
                names = set(bundle.namelist())
            self.assertIn(f"{prefix}/tools/install.py", names)
            self.assertFalse(any(name.startswith(f"{prefix}/bin/") for name in names))
            self.assertFalse(any(name.startswith(f"{prefix}/tools/fixdep/") for name in names))
            self.assertFalse(
                any(name.startswith(f"{prefix}/tools/ecos_cli/tests/") for name in names)
            )
            self.assertFalse(any(name.startswith(f"{prefix}/boards/UserBSP") for name in names))
            self.assertFalse(
                any(
                    name.startswith(f"{prefix}/boards/StarrySkyC1/Makefile")
                    for name in names
                )
            )

            sbom = json.loads(sbom_path.read_text(encoding="utf-8"))
            self.assertEqual(sbom["spdxVersion"], "SPDX-2.3")
            self.assertEqual(sbom["packages"][0]["name"], "ecos-embedded-sdk")
            self.assertEqual(
                {(item["name"], item["versionInfo"]) for item in sbom["packages"][1:]},
                {
                    ("PyYAML", "6.0.3"), ("windows-curses", "2.4.2"),
                    ("cmake", "3.31.10"), ("test-toolchain", "9.9.9"),
                },
            )
            windows_package = next(
                item for item in sbom["packages"] if item["name"] == "windows-curses"
            )
            self.assertIn("sys_platform", windows_package["packageComment"])
            self.assertEqual(
                {item["fileName"] for item in sbom["files"]},
                {
                    "./" + name[len(prefix) + 1:]
                    for name in names
                    if name.startswith(f"{prefix}/") and not name.endswith("/")
                },
            )
            self.assertEqual(
                set(sbom["packages"][0]["hasFiles"]),
                {item["SPDXID"] for item in sbom["files"]},
            )
            self.assertEqual(
                sbom["packages"][0]["licenseDeclared"], "NOASSERTION"
            )

            checksum_lines = (output / "SHA256SUMS").read_text(
                encoding="ascii"
            ).splitlines()
            expected = {
                f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.name}"
                for path in (tar_path, zip_path, sbom_path)
            }
            self.assertEqual(set(checksum_lines), expected)

            original_sbom = sbom_path.read_bytes()
            release.build_release(
                root, tag="v1.2.3", ref="HEAD", output_dir=output
            )
            self.assertEqual(sbom_path.read_bytes(), original_sbom)
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

    def test_rejects_archive_missing_declared_build_kconfig(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.create_repository(
                root,
                build_kconfig="tools/ecos_cli/src/ecos_cli/resources/Kconfig.build",
            )

            with self.assertRaisesRegex(release.ReleaseError, "incomplete"):
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
