import json
import os
import sys
import tempfile
import types
import unittest
from contextlib import redirect_stderr, redirect_stdout
from io import StringIO
from pathlib import Path
from typing import Optional
from unittest import mock


SOURCE_ROOT = Path(__file__).parents[1] / "src"
sys.path.insert(0, str(SOURCE_ROOT))

from ecos_cli import artifacts, configuration, flashing, monitoring  # noqa: E402
from ecos_cli.cli import ExitCode, main  # noqa: E402
from ecos_cli.sdk_context import SdkContext  # noqa: E402
from ecos_cli.sdk_manifest import context_from_root  # noqa: E402
from test_project import create_sdk  # noqa: E402


def sdk_context(sdk: Path) -> SdkContext:
    return context_from_root(sdk, kind="checkout", source="test")


def add_board_io_contract(sdk: Path) -> None:
    path = sdk / "board" / "StarrySkyL4" / "ecos-board.yml"
    path.write_text(
        path.read_text(encoding="utf-8")
        + "flash:\n"
        + "  provider: mass-storage\n"
        + "  artifact: bin\n"
        + "  volume_label: ECOS-TEST\n"
        + "monitor:\n"
        + "  baudrate: 115200\n",
        encoding="utf-8",
    )


def create_project(
    sdk: Path, workspace: Path, *, profile: Optional[str] = None
) -> Path:
    arguments = [
        "--sdk",
        str(sdk),
        "project",
        "create",
        "hello_world",
        "--path",
        str(workspace),
        "--board",
        "starrysky-l4",
        "--format",
        "json",
    ]
    if profile is not None:
        arguments.extend(["--profile", profile])
    with redirect_stdout(StringIO()), redirect_stderr(StringIO()):
        result = main(arguments)
    if result != ExitCode.OK:
        raise AssertionError(f"project creation failed with exit code {result}")
    return workspace / "hello_world"


def write_fake_outputs(root: Path) -> None:
    build = root / "build"
    build.mkdir()
    for suffix in ("elf", "bin", "hex", "txt", "map", "size"):
        (build / f"retrosoc_fw.{suffix}").write_bytes(f"{suffix}-output".encode())
    (build / "compile_commands.json").write_text(
        '[{"directory":"build","command":"cc -c main.c","file":"main.c"}]\n',
        encoding="utf-8",
    )


def fake_elf() -> dict:
    return {
        "class": "ELF32",
        "endianness": "little",
        "type": 2,
        "machine": "RISC-V",
        "machine_id": artifacts.EM_RISCV,
        "entry": 0x1000,
        "flags": artifacts.EF_RISCV_RVE,
        "rve": True,
        "sections": [],
        "segments": [],
        "symbols": {"_start": 0x1000, "main": 0x1010},
    }


class ConfigurationWorkflowTest(unittest.TestCase):
    def test_menuconfig_rejects_non_interactive_streams(self):
        with (
            mock.patch.object(configuration.sys, "stdin", StringIO()),
            mock.patch.object(configuration.sys, "stdout", StringIO()),
            self.assertRaises(configuration.ConfigurationTerminalError),
        ):
            configuration._require_interactive_terminal()

    def test_menuconfig_persists_user_config_and_regenerates_outputs(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            target_root = sdk / "components" / "soc" / "ysyx-2512"
            manifest = target_root / "ecos-soc.yml"
            manifest.write_text(
                manifest.read_text(encoding="utf-8") + "  kconfig: Kconfig\n",
                encoding="utf-8",
            )
            (target_root / "Kconfig").write_text(
                'config FEATURE\n    bool "Feature"\n    default n\n',
                encoding="utf-8",
            )
            workspace = root / "workspace"
            workspace.mkdir()
            project = create_project(sdk, workspace)

            class FakeKconfigError(Exception):
                pass

            class FakeKconfig:
                def __init__(self, *_: object, **__: object) -> None:
                    self.config_content = "# CONFIG_FEATURE is not set\n"
                    self.unique_defined_syms = [
                        types.SimpleNamespace(name="FEATURE", str_value="y")
                    ]

                def load_config(self, path: str, replace: bool = True) -> None:
                    self.config_content = Path(path).read_text(encoding="utf-8")

                def write_config(self, path: str) -> None:
                    Path(path).write_text(self.config_content, encoding="utf-8")

                def write_autoconf(self, path: str) -> None:
                    Path(path).write_text(
                        "#define CONFIG_FEATURE 1\n", encoding="utf-8"
                    )

            fake_kconfiglib = types.ModuleType("kconfiglib")
            fake_kconfiglib.Kconfig = FakeKconfig
            fake_kconfiglib.KconfigError = FakeKconfigError
            fake_menuconfig = types.ModuleType("menuconfig")

            def save_menuconfig(_: FakeKconfig) -> None:
                Path(os.environ["KCONFIG_CONFIG"]).write_text(
                    "CONFIG_FEATURE=y\n", encoding="utf-8"
                )

            fake_menuconfig.menuconfig = save_menuconfig
            output = StringIO()
            errors = StringIO()
            with (
                mock.patch.dict(
                    sys.modules,
                    {"kconfiglib": fake_kconfiglib, "menuconfig": fake_menuconfig},
                ),
                mock.patch.object(configuration, "_require_interactive_terminal"),
                mock.patch.dict(os.environ, {"KCONFIG_CONFIG": "outer.config"}),
                redirect_stdout(output),
                redirect_stderr(errors),
            ):
                result = main(
                    [
                        "--sdk",
                        str(sdk),
                        "menuconfig",
                        "--project",
                        str(project),
                    ]
                )
                self.assertEqual(os.environ["KCONFIG_CONFIG"], "outer.config")

            self.assertEqual(result, ExitCode.OK)
            self.assertEqual(
                (project / ".ecos/project.config").read_text(encoding="utf-8"),
                "CONFIG_FEATURE=y\n",
            )
            generated = project / ".ecos/generated"
            self.assertEqual(
                (generated / ".config").read_text(encoding="utf-8"),
                "CONFIG_FEATURE=y\n",
            )
            self.assertEqual(
                (generated / "sdkconfig.h").read_text(encoding="utf-8"),
                "#define CONFIG_FEATURE 1\n",
            )
            self.assertIn("User configuration:", errors.getvalue())

    def test_menuconfig_requires_kconfig_options(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            workspace = root / "workspace"
            workspace.mkdir()
            project = create_project(sdk, workspace)

            with self.assertRaisesRegex(
                configuration.ConfigurationError,
                "no Kconfig options",
            ):
                configuration.menuconfig_project(
                    sdk_context(sdk), project_root=project
                )

    def test_project_create_rejects_invalid_component_without_installing_project(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            manifest = sdk / "example" / "hello_world" / "ecos-example.yml"
            manifest.write_text(
                manifest.read_text(encoding="utf-8")
                + "components:\n  - missing-component\n",
                encoding="utf-8",
            )
            workspace = root / "workspace"
            workspace.mkdir()
            output = StringIO()
            with redirect_stdout(output), redirect_stderr(StringIO()):
                result = main(
                    [
                        "--sdk",
                        str(sdk),
                        "project",
                        "create",
                        "hello_world",
                        "--path",
                        str(workspace),
                        "--board",
                        "starrysky-l4",
                        "--format",
                        "json",
                    ]
                )
            payload = json.loads(output.getvalue())
            self.assertEqual(result, ExitCode.CONFIG)
            self.assertEqual(
                payload["diagnostics"][0]["code"],
                "ECOS_PROJECT_COMPONENT_INVALID",
            )
            self.assertFalse((workspace / "hello_world").exists())

    def test_invalid_arguments_preserve_json_contract(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            output = StringIO()
            with redirect_stdout(output), redirect_stderr(StringIO()):
                result = main(
                    [
                        "--sdk",
                        str(sdk),
                        "validate",
                        "--unknown",
                        "--format=json",
                    ]
                )
            payload = json.loads(output.getvalue())
            self.assertEqual(result, ExitCode.USAGE)
            self.assertEqual(payload["status"], "error")
            self.assertEqual(payload["command"], "validate")

    def test_validate_is_read_only_and_configure_is_idempotent(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            workspace = root / "workspace"
            workspace.mkdir()
            project = create_project(sdk, workspace)

            output = StringIO()
            with redirect_stdout(output), redirect_stderr(StringIO()):
                result = main(
                    [
                        "--sdk",
                        str(sdk),
                        "validate",
                        "--project",
                        str(project),
                        "--format",
                        "json",
                    ]
                )
            self.assertEqual(result, ExitCode.OK)
            self.assertTrue(json.loads(output.getvalue())["data"]["valid"])
            self.assertFalse((project / ".ecos" / "generated").exists())

            first = configuration.configure_project(
                sdk_context(sdk), project_root=project
            )
            second = configuration.configure_project(
                sdk_context(sdk), project_root=project
            )
            self.assertTrue(first.data["changed"])
            self.assertFalse(second.data["changed"])
            generated = project / ".ecos" / "generated"
            self.assertEqual(
                {path.name for path in generated.iterdir()},
                {
                    ".config",
                    "Kconfig",
                    "configuration.fingerprint",
                    "resolved-project.cmake",
                    "resolved-project.json",
                    "sdkconfig.cmake",
                    "sdkconfig.h",
                },
            )
            loaded = configuration.load_resolved_project(project)
            self.assertEqual(
                (generated / "configuration.fingerprint")
                .read_text(encoding="ascii")
                .strip(),
                loaded["configuration"]["fingerprint"],
            )

            (project / "main.h").write_text("#define APP_VALUE 1\n", encoding="utf-8")
            third = configuration.configure_project(
                sdk_context(sdk), project_root=project
            )
            self.assertTrue(third.data["changed"])
            self.assertNotEqual(
                first.data["source_fingerprint"], third.data["source_fingerprint"]
            )

    def test_resolves_component_dependencies_and_capabilities(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            target_manifest = sdk / "components" / "soc" / "ysyx-2512" / "ecos-soc.yml"
            target_manifest.write_text(
                target_manifest.read_text(encoding="utf-8")
                + "capabilities:\n  - console\n",
                encoding="utf-8",
            )
            for component_id, dependency in (("base", None), ("console-app", "base")):
                component = sdk / "components" / component_id
                component.mkdir()
                (component / f"{component_id}.c").write_text(
                    "void component(void) {}\n", encoding="utf-8"
                )
                manifest = (
                    f"schema: 1\nid: {component_id}\n"
                    f"sources:\n  - {component_id}.c\n"
                )
                if dependency:
                    manifest += f"dependencies:\n  - {dependency}\nrequires:\n  - console\n"
                (component / "ecos-component.yml").write_text(manifest, encoding="utf-8")
            example_manifest = sdk / "example" / "hello_world" / "ecos-example.yml"
            example_manifest.write_text(
                example_manifest.read_text(encoding="utf-8")
                + "components:\n  - console-app\n",
                encoding="utf-8",
            )
            workspace = root / "workspace"
            workspace.mkdir()
            project = create_project(sdk, workspace)

            resolved = configuration.configure_project(
                sdk_context(sdk), project_root=project
            ).resolved

            self.assertEqual(
                [item["id"] for item in resolved["components"]],
                ["base", "console-app"],
            )
            self.assertEqual(resolved["requirements"]["requested"], ["console"])

    def test_board_selects_bsp_with_private_driver_and_hal_dependencies(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            target_manifest = sdk / "components" / "soc" / "ysyx-2512" / "ecos-soc.yml"
            target_manifest.write_text(
                target_manifest.read_text(encoding="utf-8")
                + "capabilities:\n  - uart\n",
                encoding="utf-8",
            )
            board_root = sdk / "board" / "StarrySkyL4"
            board_manifest = board_root / "ecos-board.yml"
            board_manifest.write_text(
                board_manifest.read_text(encoding="utf-8")
                + "build:\n"
                + "  components:\n"
                + "    - bsp-starrysky-l4\n"
                + "resources:\n"
                + "  console:\n"
                + "    driver: driver-uart\n"
                + "    instance: 0\n",
                encoding="utf-8",
            )
            component_definitions = (
                (sdk / "hal" / "uart", "hal-uart", None, "uart"),
                (sdk / "drivers" / "uart", "driver-uart", "hal-uart", None),
                (board_root, "bsp-starrysky-l4", "driver-uart", None),
            )
            for component_root, component_id, dependency, requirement in component_definitions:
                component_root.mkdir(parents=True, exist_ok=True)
                manifest = f"schema: 1\nid: {component_id}\n"
                if dependency is not None:
                    manifest += f"private_dependencies:\n  - {dependency}\n"
                if requirement is not None:
                    manifest += f"requires:\n  - {requirement}\n"
                (component_root / "ecos-component.yml").write_text(
                    manifest, encoding="utf-8"
                )
            example_manifest = sdk / "example" / "hello_world" / "ecos-example.yml"
            example_manifest.write_text(
                example_manifest.read_text(encoding="utf-8")
                + "requires:\n  - console\n",
                encoding="utf-8",
            )
            workspace = root / "workspace"
            workspace.mkdir()
            project = create_project(sdk, workspace)

            configured = configuration.configure_project(
                sdk_context(sdk), project_root=project
            )
            resolved = configured.resolved

            self.assertEqual(resolved["component_roots"], ["bsp-starrysky-l4"])
            self.assertEqual(
                [item["id"] for item in resolved["components"]],
                ["hal-uart", "driver-uart", "bsp-starrysky-l4"],
            )
            self.assertEqual(
                resolved["components"][1]["private_dependencies"], ["hal-uart"]
            )
            self.assertEqual(
                resolved["components"][2]["private_dependencies"], ["driver-uart"]
            )
            self.assertEqual(
                resolved["requirements"]["requested"], ["console", "uart"]
            )
            generated_cmake = (
                project / ".ecos/generated/resolved-project.cmake"
            ).read_text(encoding="utf-8")
            self.assertIn("ECOS_COMPONENT_ROOT_KEYS", generated_cmake)
            self.assertIn(
                "ECOS_COMPONENT_BSP_STARRYSKY_L4_PRIVATE_DEPENDENCIES",
                generated_cmake,
            )

    def test_rejects_duplicate_component_ids_across_resource_roots(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            for component_root in (
                sdk / "components" / "duplicate",
                sdk / "drivers" / "duplicate",
            ):
                component_root.mkdir(parents=True)
                (component_root / "ecos-component.yml").write_text(
                    "schema: 1\nid: duplicate\n", encoding="utf-8"
                )
            example_manifest = sdk / "example" / "hello_world" / "ecos-example.yml"
            example_manifest.write_text(
                example_manifest.read_text(encoding="utf-8")
                + "components:\n  - duplicate\n",
                encoding="utf-8",
            )
            workspace = root / "workspace"
            workspace.mkdir()
            output = StringIO()

            with redirect_stdout(output), redirect_stderr(StringIO()):
                result = main(
                    [
                        "--sdk",
                        str(sdk),
                        "project",
                        "create",
                        "hello_world",
                        "--path",
                        str(workspace),
                        "--board",
                        "starrysky-l4",
                        "--format",
                        "json",
                    ]
                )

            payload = json.loads(output.getvalue())
            self.assertEqual(result, ExitCode.CONFIG)
            self.assertEqual(
                payload["diagnostics"][0]["code"],
                "ECOS_PROJECT_COMPONENT_INVALID",
            )
            self.assertIn("ambiguous", payload["diagnostics"][0]["message"])

    def test_rejects_unsupported_board_profile(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            board = sdk / "board" / "StarrySkyL4" / "ecos-board.yml"
            board.write_text(
                board.read_text(encoding="utf-8")
                + "build:\n"
                + "  default_profile: release\n"
                + "  profiles:\n"
                + "    - release\n",
                encoding="utf-8",
            )
            workspace = root / "workspace"
            workspace.mkdir()
            output = StringIO()
            with redirect_stdout(output), redirect_stderr(StringIO()):
                result = main(
                    [
                        "--sdk",
                        str(sdk),
                        "project",
                        "create",
                        "hello_world",
                        "--path",
                        str(workspace),
                        "--board",
                        "starrysky-l4",
                        "--profile",
                        "debug",
                        "--format",
                        "json",
                    ]
                )
            payload = json.loads(output.getvalue())
            self.assertEqual(result, ExitCode.CONFIG)
            self.assertEqual(
                payload["diagnostics"][0]["code"], "ECOS_PROJECT_MODEL_INVALID"
            )
            self.assertFalse((workspace / "hello_world").exists())


class ArtifactContractTest(unittest.TestCase):
    def test_manifest_records_contract_and_detects_tampering(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            write_fake_outputs(root)
            resolved = {
                "project": {
                    "name": "demo",
                    "board": "test-board",
                    "target": "test-target",
                    "profile": "release",
                },
                "sdk": {"id": "test-sdk", "version": "1.0.0"},
                "toolchain": {"id": "test-toolchain", "release": "1.0.0"},
                "target": {
                    "arch": "riscv",
                    "cpu": {"march": "rv32e", "abi": "ilp32e"},
                    "memory": {"ram": {"origin": 0x1000, "size": "64K"}},
                },
                "build": {
                    "outputs": [
                        "elf",
                        "bin",
                        "hex",
                        "txt",
                        "map",
                        "size",
                        "compile_commands",
                    ]
                },
                "source_fingerprint": "1" * 64,
                "configuration": {"fingerprint": "2" * 64},
            }
            with mock.patch("ecos_cli.artifacts.inspect_elf", return_value=fake_elf()):
                manifest = artifacts.create_manifest(root, resolved)

            self.assertEqual(manifest["firmware"]["architecture"], "riscv")
            self.assertEqual(manifest["firmware"]["isa"], "rv32e")
            self.assertEqual(manifest["firmware"]["abi"], "ilp32e")
            self.assertEqual(manifest["firmware"]["entry"], 0x1000)
            self.assertEqual(
                manifest["firmware"]["sha256"], manifest["files"]["bin"]["sha256"]
            )
            artifacts.load_manifest(root)
            (root / "build" / "retrosoc_fw.bin").write_bytes(b"tampered")
            with self.assertRaises(artifacts.ArtifactManifestError):
                artifacts.load_manifest(root)


class DeviceWorkflowTest(unittest.TestCase):
    def test_flash_refuses_to_overwrite_source_artifact(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "firmware.bin"
            source.write_bytes(b"firmware")
            with self.assertRaises(flashing.FlashWriteError):
                flashing._copy_and_sync(source, source)
            self.assertEqual(source.read_bytes(), b"firmware")

    def test_flash_uses_configured_board_and_artifact_manifest(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            add_board_io_contract(sdk)
            workspace = root / "workspace"
            workspace.mkdir()
            project = create_project(sdk, workspace)
            configured = configuration.configure_project(
                sdk_context(sdk), project_root=project
            )
            write_fake_outputs(project)
            with mock.patch("ecos_cli.artifacts.inspect_elf", return_value=fake_elf()):
                artifacts.create_manifest(project, configured.resolved)
            device = root / "device"
            device.mkdir()

            output = StringIO()
            with redirect_stdout(output), redirect_stderr(StringIO()):
                result = main(
                    [
                        "--sdk",
                        str(sdk),
                        "flash",
                        "--project",
                        str(project),
                        "--device",
                        str(device),
                        "--format",
                        "json",
                    ]
                )
            payload = json.loads(output.getvalue())
            self.assertEqual(result, ExitCode.OK)
            self.assertEqual(payload["data"]["board"], "starrysky-l4")
            self.assertEqual(
                (device / "retrosoc_fw.bin").read_bytes(), b"bin-output"
            )

            (project / "main.c").write_text("void main(void) { }\n", encoding="utf-8")
            output = StringIO()
            with redirect_stdout(output), redirect_stderr(StringIO()):
                stale_result = main(
                    [
                        "--sdk",
                        str(sdk),
                        "flash",
                        "--project",
                        str(project),
                        "--device",
                        str(device),
                        "--format",
                        "json",
                    ]
                )
            self.assertEqual(stale_result, ExitCode.CONFIG)
            self.assertEqual(
                json.loads(output.getvalue())["diagnostics"][0]["code"],
                "ECOS_FLASH_ARTIFACT_INVALID",
            )

    def test_monitor_uses_board_serial_contract(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            add_board_io_contract(sdk)
            workspace = root / "workspace"
            workspace.mkdir()
            project = create_project(sdk, workspace)

            connection = mock.Mock()
            connection.read.side_effect = [b"boot ready\n"]
            serial_module = mock.Mock()
            serial_module.SerialException = RuntimeError
            serial_module.Serial.return_value = connection
            output = StringIO()
            with mock.patch(
                "ecos_cli.monitoring._serial_modules",
                return_value=(serial_module, mock.Mock()),
            ), redirect_stdout(output), redirect_stderr(StringIO()):
                result = main(
                    [
                        "--sdk",
                        str(sdk),
                        "monitor",
                        "--project",
                        str(project),
                        "--port",
                        "TEST0",
                        "--expect",
                        "ready",
                        "--format",
                        "json",
                    ]
                )
            payload = json.loads(output.getvalue())
            self.assertEqual(result, ExitCode.OK)
            self.assertEqual(payload["data"]["port"], "TEST0")
            self.assertEqual(payload["data"]["baudrate"], 115200)
            self.assertTrue(payload["data"]["matched"])
            connection.close.assert_called_once_with()

            with self.assertRaises(monitoring.MonitorConfigurationError):
                monitoring.monitor_project(
                    sdk_context(sdk),
                    project_root=project,
                    port="TEST0",
                    baudrate=0,
                    timeout=1,
                    stream=None,
                )

    def test_mass_storage_discovery_supports_posix_and_windows(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            volume = root / "ECOS-TEST"
            volume.mkdir()
            self.assertEqual(
                flashing.discover_mass_storage(
                    "ecos-test", platform_name="linux", roots=[root]
                ),
                [volume.resolve()],
            )
            windows_volume = Path("Z:/")
            with mock.patch(
                "ecos_cli.flashing._windows_volumes",
                return_value=[(windows_volume, "ECOS-TEST")],
            ):
                self.assertEqual(
                    flashing.discover_mass_storage(
                        "ecos-test", platform_name="win32"
                    ),
                    [windows_volume.resolve()],
                )


if __name__ == "__main__":
    unittest.main()
