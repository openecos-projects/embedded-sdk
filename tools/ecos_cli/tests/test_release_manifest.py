import re
import sys
import unittest
from pathlib import Path


SOURCE_ROOT = Path(__file__).parents[1] / "src"
SDK_ROOT = Path(__file__).parents[3]
sys.path.insert(0, str(SOURCE_ROOT))

from ecos_cli import __schema_version__, __version__, sdk_manifest  # noqa: E402


def _covers(relative: str, paths: tuple[str, ...]) -> bool:
    return any(
        relative == path or relative.startswith(f"{path}/") for path in paths
    )


class ReleaseManifestTest(unittest.TestCase):
    """SDK releases must ship the public component and example trees."""

    def test_cli_package_version_matches_runtime_and_sdk_compatibility(self):
        metadata = (SDK_ROOT / "tools/ecos_cli/pyproject.toml").read_text(encoding="utf-8")
        versions = re.findall(r'^version = "([^"]+)"$', metadata, re.MULTILINE)
        self.assertEqual(versions, [__version__])
        manifest = sdk_manifest.load_manifest(SDK_ROOT)
        self.assertEqual(manifest["cli_compatibility"], {
            "major": int(__version__.split(".", 1)[0]),
            "schema_version": __schema_version__,
        })

    def test_release_ships_ch32_and_portable_skill(self):
        manifest = sdk_manifest.load_manifest(SDK_ROOT)
        paths = sdk_manifest.release_paths(manifest)
        excludes = sdk_manifest.release_excludes(manifest)
        for relative in (
            "components/soc/ch32v103/ecos-soc.yml",
            "boards/CH32V103C8T6/ecos-board.yml",
            "hal/spi/include/ecos/hal/spi.h",
            "drivers/spi/include/ecos/driver/spi.h",
            "examples/peripherals/rtc/counter/ecos-example.yml",
            ".agents/skills/ecos-sdk/SKILL.md",
            "tools/install_skill.py",
            "docs/versioning.md",
        ):
            with self.subTest(resource=relative):
                self.assertTrue((SDK_ROOT / relative).is_file())
                self.assertTrue(_covers(relative, paths))
                self.assertFalse(_covers(relative, excludes))

    def test_release_ships_every_component_and_example(self):
        manifest = sdk_manifest.load_manifest(SDK_ROOT)
        paths = sdk_manifest.release_paths(manifest)
        excludes = sdk_manifest.release_excludes(manifest)

        manifests = [
            *sorted((SDK_ROOT / "components").rglob("ecos-component.yml")),
            *sorted((SDK_ROOT / "examples").rglob("ecos-example.yml")),
        ]
        self.assertTrue(manifests)

        for manifest_path in manifests:
            relative = manifest_path.parent.relative_to(SDK_ROOT).as_posix()
            with self.subTest(unit=relative):
                self.assertTrue(
                    _covers(relative, paths),
                    f"{relative} is outside release.paths",
                )
                self.assertFalse(
                    _covers(relative, excludes),
                    f"{relative} is excluded from the SDK release",
                )

    def test_public_components_previously_deferred_are_released(self):
        manifest = sdk_manifest.load_manifest(SDK_ROOT)
        excludes = sdk_manifest.release_excludes(manifest)
        for relative in (
            "components/fatfs",
            "components/fcc",
            "components/letter-shell",
            "components/letter-shell-fs",
            "components/text_editor",
            "examples/concurrency/letter-shell-smoke",
            "examples/storage/fcc",
            "examples/storage/flash-fatfs",
            "examples/storage/text-editor",
        ):
            with self.subTest(unit=relative):
                self.assertFalse(
                    _covers(relative, excludes),
                    f"{relative} must be part of the SDK release",
                )


if __name__ == "__main__":
    unittest.main()
