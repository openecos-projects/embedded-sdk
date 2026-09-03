import json
import os
import sys
import tempfile
import unittest
from contextlib import redirect_stdout
from io import StringIO
from pathlib import Path
from unittest import mock


SOURCE_ROOT = Path(__file__).parents[1] / "src"
sys.path.insert(0, str(SOURCE_ROOT))

from ecos_cli.cli import ExitCode, main  # noqa: E402
from ecos_cli.sdk_manifest import load_manifest  # noqa: E402
from ecos_cli.sdk_registry import (  # noqa: E402
    SdkRegistrationConflict,
    SdkRegistry,
    default_registry_path,
)
from ecos_cli.sdk_resolver import (  # noqa: E402
    SdkResolutionError,
    SdkResolver,
    write_project_pin,
)


REPOSITORY_ROOT = Path(__file__).parents[3]


def create_sdk(root: Path, version: str) -> Path:
    layout = {
        "boards": "board",
        "components": "components",
        "drivers": "drivers",
        "devices": "devices",
        "docs": "docs",
        "examples": "example",
        "hal": "hal",
        "templates": "templates",
        "cli": "tools/ecos_cli/src",
    }
    for relative in layout.values():
        (root / relative).mkdir(parents=True, exist_ok=True)
    manifest = {
        "schema_version": 1,
        "sdk_id": "ecos-embedded-sdk",
        "sdk_version": version,
        "channel": "development",
        "cli_compatibility": {"major": 3, "schema_version": "1.0"},
        "layout": layout,
        "toolchain": {
            "id": "xpack-riscv-none-elf-gcc",
            "release": "15.2.0-1",
        },
    }
    manifest_path = root / "tools" / "sdk-manifest.json"
    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
    return root


class SdkRegistryTest(unittest.TestCase):
    def test_repository_manifest_identifies_sdk_3_0_0(self):
        manifest = load_manifest(REPOSITORY_ROOT)
        self.assertEqual(manifest["sdk_version"], "3.0.0")
        self.assertEqual(manifest["sdk_id"], "ecos-embedded-sdk")

    def test_registry_default_paths_are_platform_specific(self):
        home = Path("/users/test")
        self.assertEqual(
            default_registry_path(environ={}, home=home, platform_name="linux"),
            home / ".config/ecos/sdks.json",
        )
        self.assertEqual(
            default_registry_path(environ={}, home=home, platform_name="darwin"),
            home / "Library/Application Support/ECOS/sdks.json",
        )
        self.assertEqual(
            default_registry_path(
                environ={"APPDATA": "C:/Users/test/AppData/Roaming"},
                home=home,
                platform_name="win32",
            ),
            Path("C:/Users/test/AppData/Roaming/ECOS/sdks.json"),
        )

    def test_register_switch_and_unregister_are_persistent(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            first = create_sdk(root / "SDK one", "3.0.0")
            second = create_sdk(root / "SDK-测试", "3.1.0")
            registry = SdkRegistry(root / "config/sdks.json")

            initial = registry.register(first, name="stable", activate=True)
            repeated = registry.register(first, name="stable", activate=True)
            registry.register(second, name="next")
            switched = registry.use("3.1.0")

            self.assertTrue(initial["changed"])
            self.assertFalse(repeated["changed"])
            self.assertEqual(switched["active"], "next")
            self.assertEqual(SdkRegistry(registry.path).active_name(), "next")

            removed = registry.unregister("stable")
            self.assertEqual(removed["entry"]["root"], str(first.resolve()))
            self.assertTrue(first.is_dir())

    def test_conflicting_name_requires_replace(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            first = create_sdk(root / "first", "3.0.0")
            second = create_sdk(root / "second", "3.1.0")
            registry = SdkRegistry(root / "sdks.json")
            registry.register(first, name="default")

            with self.assertRaises(SdkRegistrationConflict):
                registry.register(second, name="default")
            replaced = registry.register(second, name="default", replace=True)
            self.assertEqual(replaced["entry"]["sdk_version"], "3.1.0")

    def test_register_without_activate_preserves_empty_active_selection(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk", "3.0.0")
            registry = SdkRegistry(root / "sdks.json")

            result = registry.register(sdk, name="available")

            self.assertIsNone(result["active"])
            self.assertIsNone(registry.active_name())

    def test_resolver_uses_fixed_priority(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            explicit = create_sdk(root / "explicit", "3.4.0")
            project_sdk = create_sdk(root / "project-sdk", "3.3.0")
            environment = create_sdk(root / "environment", "3.2.0")
            active = create_sdk(root / "active", "3.1.0")
            checkout = create_sdk(root / "checkout", "3.0.0")
            project = root / "project"
            project.mkdir()
            registry = SdkRegistry(root / "sdks.json")
            registry.register(active, name="active", activate=True)
            project_entry = registry.register(project_sdk, name="project")
            registry.register(explicit, name="explicit")
            project_context = SdkResolver(registry, environ={}).resolve(explicit="project")
            write_project_pin(project, project_context)

            resolver = SdkResolver(
                registry,
                environ={"ECOS_SDK_HOME": str(environment)},
                checkout_hint=checkout,
            )
            self.assertEqual(
                resolver.resolve(explicit="explicit", project=project).version, "3.4.0"
            )
            self.assertEqual(resolver.resolve(project=project).version, "3.3.0")

            (project / ".ecos/sdk.json").unlink()
            self.assertEqual(resolver.resolve(project=project).version, "3.2.0")
            self.assertEqual(
                SdkResolver(registry, environ={}, checkout_hint=checkout).resolve(
                    project=project
                ).version,
                "3.1.0",
            )

            empty = SdkRegistry(root / "empty.json")
            self.assertEqual(
                SdkResolver(empty, environ={}, checkout_hint=checkout).resolve(
                    project=project
                ).version,
                "3.0.0",
            )
            self.assertTrue(project_entry["entry"]["root"].endswith("project-sdk"))

    def test_explicit_registered_release_path_preserves_release_context(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            release = create_sdk(root / "release", "3.0.0")
            registry = SdkRegistry(root / "sdks.json")
            registry.register(release, name="release", kind="release")

            context = SdkResolver(registry, environ={}).resolve(
                explicit=str(release)
            )

            self.assertEqual(context.kind, "release")
            self.assertEqual(context.registration_name, "release")
            self.assertEqual(context.root, release.resolve())

    def test_resolver_prefers_enclosing_checkout_to_legacy_environment(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            checkout = create_sdk(root / "checkout", "3.1.0")
            environment = create_sdk(root / "legacy-environment", "2.0.0")
            project = checkout / "work" / "project"
            project.mkdir(parents=True)

            context = SdkResolver(
                SdkRegistry(root / "sdks.json"),
                environ={"ECOS_SDK_HOME": str(environment)},
            ).resolve(project=project)

            self.assertEqual(context.root, checkout.resolve())
            self.assertEqual(context.source, "workspace-checkout")

    def test_invalid_high_priority_environment_does_not_fall_back(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            active = create_sdk(root / "active", "3.0.0")
            registry = SdkRegistry(root / "sdks.json")
            registry.register(active, activate=True)
            resolver = SdkResolver(
                registry,
                environ={"ECOS_SDK_HOME": str(root / "missing")},
                checkout_hint=active,
            )
            with self.assertRaisesRegex(SdkResolutionError, "ECOS_SDK_HOME"):
                resolver.resolve(project=root)

    def test_doctor_reports_moved_sdk(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk", "3.0.0")
            registry = SdkRegistry(root / "sdks.json")
            registry.register(sdk)
            sdk.rename(root / "moved")

            status = registry.doctor()
            self.assertFalse(status["valid"])
            self.assertEqual(status["entries"][0]["state"], "invalid")

    def test_cli_sdk_commands_have_json_contract(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk", "3.0.0")
            registry = root / "sdks.json"
            output = StringIO()
            with redirect_stdout(output):
                result = main(
                    [
                        "sdk",
                        "--registry",
                        str(registry),
                        "register",
                        str(sdk),
                        "--name",
                        "dev",
                        "--activate",
                        "--format",
                        "json",
                    ]
                )

            payload = json.loads(output.getvalue())
            self.assertEqual(result, ExitCode.OK)
            self.assertEqual(payload["command"], "sdk.register")
            self.assertEqual(payload["data"]["name"], "dev")
            self.assertEqual(payload["data"]["entry"]["sdk_version"], "3.0.0")

    def test_release_sdk_is_default_toolchain_prefix(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk", "3.0.0")
            registry_path = root / "sdks.json"
            SdkRegistry(registry_path).register(
                sdk, name="release", kind="release", activate=True
            )
            toolchain_manifest = (
                REPOSITORY_ROOT
                / "tools/ecos_cli/src/ecos_cli/resources/toolchains"
                / "xpack-riscv-none-elf-gcc.json"
            )
            output = StringIO()
            with mock.patch.dict(
                os.environ,
                {
                    "ECOS_SDK_HOME": "",
                    "ECOS_SDK_REGISTRY": str(registry_path),
                },
            ), mock.patch(
                "ecos_cli.sdk_resolver.Path.cwd", return_value=root
            ), redirect_stdout(output):
                result = main(
                    [
                        "toolchain",
                        "install",
                        "--manifest",
                        str(toolchain_manifest),
                        "--host",
                        "linux-x86_64",
                        "--dry-run",
                        "--format",
                        "json",
                    ]
                )

            payload = json.loads(output.getvalue())
            self.assertEqual(result, ExitCode.OK)
            self.assertEqual(payload["data"]["prefix"], str(sdk.resolve()))


if __name__ == "__main__":
    unittest.main()
