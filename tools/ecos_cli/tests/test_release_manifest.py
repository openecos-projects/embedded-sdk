import sys
import unittest
from pathlib import Path


SOURCE_ROOT = Path(__file__).parents[1] / "src"
SDK_ROOT = Path(__file__).parents[3]
sys.path.insert(0, str(SOURCE_ROOT))

from ecos_cli import sdk_manifest  # noqa: E402


def _covers(relative: str, paths: tuple[str, ...]) -> bool:
    return any(
        relative == path or relative.startswith(f"{path}/") for path in paths
    )


class ReleaseManifestTest(unittest.TestCase):
    """The 3.0 release must ship the public component and example trees."""

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
                    f"{relative} is excluded from the 3.0 release",
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
                    f"{relative} must be part of the 3.0 release",
                )


if __name__ == "__main__":
    unittest.main()
