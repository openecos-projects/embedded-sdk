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

from ecos_cli import artifacts, configuration, doctor, flashing, monitoring  # noqa: E402
from ecos_cli.cli import ExitCode, main  # noqa: E402
from ecos_cli.project_model import ManifestValidationError, _read_manifest  # noqa: E402
from ecos_cli.sdk_context import SdkContext  # noqa: E402
from ecos_cli.sdk_manifest import context_from_root  # noqa: E402
from test_project import create_sdk  # noqa: E402


def sdk_context(sdk: Path) -> SdkContext:
    return context_from_root(sdk, kind="checkout", source="test")


def add_board_io_contract(sdk: Path) -> None:
    path = sdk / "boards" / "StarrySkyL4" / "ecos-board.yml"
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


def add_wlink_board_contract(
    sdk: Path, tool: Path, *, unprotect: bool = True
) -> None:
    path = sdk / "boards" / "StarrySkyL4" / "ecos-board.yml"
    path.write_text(
        path.read_text(encoding="utf-8")
        + "flash:\n"
        + "  provider: wlink\n"
        + "  artifact: hex\n"
        + "  chip: CH32V103\n"
        + f"  tool: {json.dumps(str(tool))}\n"
        + f"  unprotect: {'true' if unprotect else 'false'}\n"
        + "  erase: true\n",
        encoding="utf-8",
    )


def prepare_wlink_project(
    root: Path, *, unprotect: bool = True
) -> tuple[Path, Path, Path]:
    sdk = create_sdk(root / "sdk")
    tool = root / "wlink"
    tool.write_text("", encoding="utf-8")
    tool.chmod(0o755)
    add_wlink_board_contract(sdk, tool, unprotect=unprotect)
    workspace = root / "workspace"
    workspace.mkdir()
    project = create_project(sdk, workspace)
    configured = configuration.configure_project(
        sdk_context(sdk), project_root=project
    )
    write_fake_outputs(project)
    with mock.patch("ecos_cli.artifacts.inspect_elf", return_value=fake_elf()):
        artifacts.create_manifest(project, configured.resolved)
    return sdk, project, tool


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
    def test_doctor_reports_build_errors_and_connection_warnings(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            add_board_io_contract(sdk)
            workspace = root / "workspace"
            workspace.mkdir()
            selected = create_project(sdk, workspace)

            def invoke(toolchain_state: str) -> tuple[int, dict]:
                output = StringIO()
                with (
                    mock.patch.object(doctor.dependencies, "managed_host_tool", return_value=None),
                    mock.patch.object(doctor.shutil, "which", side_effect=lambda name: f"/fake/{name}"),
                    mock.patch.object(doctor.dependencies, "probe_host_tool", return_value={"valid": True, "version": "3.31"}),
                    mock.patch.object(doctor.toolchain, "detect_host", return_value="linux-x86_64"),
                    mock.patch.object(doctor.toolchain, "installation_status", return_value={"state": toolchain_state, "root": "/fake/toolchain"}),
                    mock.patch.object(doctor.flashing, "discover_mass_storage", return_value=[]),
                    mock.patch.object(doctor.monitoring, "discover_ports", return_value=[]),
                    redirect_stdout(output),
                    redirect_stderr(StringIO()),
                ):
                    code = main([
                        "--sdk", str(sdk), "doctor", "--project", str(selected),
                        "--format", "json",
                    ])
                return code, json.loads(output.getvalue())

            code, payload = invoke("installed")
            self.assertEqual(code, ExitCode.OK)
            self.assertTrue(payload["data"]["valid"])
            self.assertEqual(payload["data"]["checks"]["project"]["state"], "valid")
            self.assertEqual(payload["data"]["checks"]["flash"]["state"], "disconnected")
            self.assertEqual(payload["data"]["checks"]["monitor"]["state"], "disconnected")
            self.assertTrue(all(item["severity"] == "warning" for item in payload["diagnostics"]))

            code, payload = invoke("missing")
            self.assertEqual(code, ExitCode.CONFIG)
            self.assertEqual(payload["status"], "error")
            self.assertIn(
                "ECOS_DOCTOR_TOOLCHAIN_NOT_READY",
                {item["code"] for item in payload["diagnostics"]},
            )

    def test_clean_only_removes_recognized_build_outputs(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            workspace = root / "workspace"
            workspace.mkdir()
            selected = create_project(sdk, workspace)
            build_dir = selected / "build"
            build_dir.mkdir()
            marker = build_dir / "user-notes.txt"
            marker.write_text("keep this\n", encoding="utf-8")

            def invoke(*options: str) -> tuple[int, dict]:
                output = StringIO()
                with redirect_stdout(output), redirect_stderr(StringIO()):
                    code = main([
                        "--sdk", str(sdk), "clean", "--project", str(selected),
                        *options, "--format", "json",
                    ])
                return code, json.loads(output.getvalue())

            code, payload = invoke()
            self.assertEqual(code, ExitCode.CONFIG)
            self.assertEqual(
                payload["diagnostics"][0]["code"],
                "ECOS_CLEAN_UNRECOGNIZED_OUTPUT",
            )
            self.assertTrue(marker.is_file())

            (build_dir / "CMakeCache.txt").write_text(
                f"CMAKE_HOME_DIRECTORY:INTERNAL={selected}\n"
                f"ECOS_PROJECT_CONFIG:UNINITIALIZED="
                f"{selected / '.ecos/generated/resolved-project.cmake'}\n",
                encoding="utf-8",
            )
            code, payload = invoke("--dry-run")
            self.assertEqual(code, ExitCode.OK)
            self.assertEqual(payload["data"]["paths"], [str(build_dir.resolve())])
            self.assertFalse(payload["data"]["removed"])
            self.assertTrue(marker.is_file())

            code, payload = invoke()
            self.assertEqual(code, ExitCode.OK)
            self.assertTrue(payload["data"]["removed"])
            self.assertFalse(build_dir.exists())
            code, payload = invoke()
            self.assertEqual(code, ExitCode.OK)
            self.assertEqual(payload["data"]["paths"], [])

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
            manifest = sdk / "examples" / "hello_world" / "ecos-example.yml"
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
                    "include",
                    "resolved-project.cmake",
                    "resolved-project.json",
                    "sdkconfig.cmake",
                    "sdkconfig.h",
                },
            )
            self.assertIn(
                "#define ECOS_BOARD_HAS_GPIO_DEMO 0",
                (generated / "include/ecos/board_resources.h").read_text(
                    encoding="utf-8"
                ),
            )
            loaded = configuration.load_resolved_project(project)
            self.assertEqual(
                (generated / "configuration.fingerprint")
                .read_text(encoding="ascii")
                .strip(),
                loaded["configuration"]["fingerprint"],
            )

            stale_directory = generated / "stale"
            stale_directory.mkdir()
            cleaned = configuration.configure_project(
                sdk_context(sdk), project_root=project
            )
            self.assertTrue(cleaned.data["changed"])
            self.assertFalse(stale_directory.exists())

            (project / "main.h").write_text("#define APP_VALUE 1\n", encoding="utf-8")
            third = configuration.configure_project(
                sdk_context(sdk), project_root=project
            )
            self.assertTrue(third.data["changed"])
            self.assertNotEqual(
                first.data["source_fingerprint"], third.data["source_fingerprint"]
            )

    def test_validate_exposes_the_resolved_build_graph(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            workspace = root / "workspace"
            workspace.mkdir()
            project = create_project(sdk, workspace)

            output = StringIO()
            with redirect_stdout(output), redirect_stderr(StringIO()):
                result = main([
                    "--sdk", str(sdk), "validate", "--project", str(project),
                    "--format", "json",
                ])
            payload = json.loads(output.getvalue())
            graph = payload["data"]["graph"]
            self.assertEqual(result, ExitCode.OK)
            self.assertEqual(graph["application"]["example"], "hello_world")
            self.assertEqual(graph["target"]["id"], "ysyx-2512")
            self.assertEqual(graph["components"], [])
            self.assertIn("sources", graph["target"])
            self.assertIn("elf", graph["outputs"])

    def test_generates_typed_gpio_demo_board_resources(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            board_manifest = sdk / "boards/StarrySkyL4/ecos-board.yml"
            board_manifest.write_text(
                board_manifest.read_text(encoding="utf-8")
                + "resources:\n"
                + "  gpio-demo:\n"
                + "    driver: driver-gpio\n"
                + "    input:\n"
                + "      controller: 1\n"
                + "      pin: 7\n"
                + "      label: GPIO1[7]\n"
                + "      idle_level: high\n"
                + "    output:\n"
                + "      controller: 1\n"
                + "      pin: 5\n"
                + "      label: GPIO1[5]\n"
                + "      initial_level: high\n",
                encoding="utf-8",
            )
            example_manifest = sdk / "examples" / "hello_world" / "ecos-example.yml"
            example_manifest.write_text(
                example_manifest.read_text(encoding="utf-8")
                + "requires:\n  - gpio-demo\n",
                encoding="utf-8",
            )
            workspace = root / "workspace"
            workspace.mkdir()
            project = create_project(sdk, workspace)

            configured = configuration.configure_project(
                sdk_context(sdk), project_root=project
            )
            generated = project / ".ecos/generated"
            header = (generated / "include/ecos/board_resources.h").read_text(
                encoding="utf-8"
            )
            cmake = (generated / "resolved-project.cmake").read_text(
                encoding="utf-8"
            )

            self.assertEqual(
                configured.data["generated"]["board_resources"],
                str((generated / "include/ecos/board_resources.h").resolve()),
            )
            self.assertIn("#define ECOS_BOARD_HAS_GPIO_DEMO 1", header)
            self.assertIn("{ ECOS_GPIO_PORT_1, 7u }", header)
            self.assertIn("{ ECOS_GPIO_PORT_1, 5u }", header)
            self.assertIn(
                '#define ECOS_BOARD_GPIO_DEMO_INPUT_LABEL "GPIO1[7]"', header
            )
            self.assertIn(
                "#define ECOS_BOARD_GPIO_DEMO_OUTPUT_INITIAL_LEVEL "
                "ECOS_GPIO_LEVEL_HIGH",
                header,
            )
            self.assertIn("set(ECOS_GENERATED_INCLUDE_DIR", cmake)

    def test_rejects_invalid_gpio_demo_board_resources(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            workspace = root / "workspace"
            workspace.mkdir()
            project = create_project(sdk, workspace)
            board_manifest = sdk / "boards/StarrySkyL4/ecos-board.yml"
            board_manifest.write_text(
                board_manifest.read_text(encoding="utf-8")
                + "resources:\n"
                + "  gpio-demo:\n"
                + "    driver: driver-gpio\n"
                + "    input:\n"
                + "      controller: 1\n"
                + "      pin: 256\n"
                + "      label: GPIO1[256]\n"
                + "      idle_level: high\n"
                + "    output:\n"
                + "      controller: 1\n"
                + "      pin: 5\n"
                + "      label: GPIO1[5]\n"
                + "      initial_level: high\n",
                encoding="utf-8",
            )

            with self.assertRaisesRegex(
                configuration.ProjectModelError,
                r"/resources/gpio-demo/input/pin: 256 is greater than the maximum of 255",
            ):
                configuration.configure_project(
                    sdk_context(sdk), project_root=project
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
            example_manifest = sdk / "examples" / "hello_world" / "ecos-example.yml"
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

    def test_sdk_core_components_are_implicit_project_roots(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            sdk_manifest = sdk / "tools" / "sdk-manifest.json"
            manifest = json.loads(sdk_manifest.read_text(encoding="utf-8"))
            manifest["build"] = {"core_components": ["core-runtime"]}
            sdk_manifest.write_text(json.dumps(manifest), encoding="utf-8")
            core = sdk / "components" / "core"
            core.mkdir()
            (core / "core.c").write_text(
                "void ecos_core(void) {}\n", encoding="utf-8"
            )
            (core / "ecos-component.yml").write_text(
                "schema: 1\n"
                "id: core-runtime\n"
                "sources:\n"
                "  - core.c\n",
                encoding="utf-8",
            )
            workspace = root / "workspace"
            workspace.mkdir()
            project = create_project(sdk, workspace)

            resolved = configuration.configure_project(
                sdk_context(sdk), project_root=project
            ).resolved

            self.assertEqual(resolved["component_roots"], ["core-runtime"])
            self.assertEqual(
                [item["id"] for item in resolved["components"]],
                ["core-runtime"],
            )

            target_workspace = root / "target-workspace"
            target_workspace.mkdir()
            with redirect_stdout(StringIO()), redirect_stderr(StringIO()):
                create_result = main(
                    [
                        "--sdk",
                        str(sdk),
                        "project",
                        "create",
                        "hello_world",
                        "--path",
                        str(target_workspace),
                        "--target",
                        "ysyx-2512",
                    ]
                )
            self.assertEqual(create_result, ExitCode.OK)
            target_resolved = configuration.configure_project(
                sdk_context(sdk),
                project_root=target_workspace / "hello_world",
            ).resolved
            self.assertIsNone(target_resolved["project"]["board"])
            self.assertEqual(
                target_resolved["component_roots"], ["core-runtime"]
            )

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
            board_root = sdk / "boards" / "StarrySkyL4"
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
            example_manifest = sdk / "examples" / "hello_world" / "ecos-example.yml"
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
            example_manifest = sdk / "examples" / "hello_world" / "ecos-example.yml"
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
            board = sdk / "boards" / "StarrySkyL4" / "ecos-board.yml"
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

    def test_board_profile_applies_kconfig_preset(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            board = sdk / "boards" / "StarrySkyL3_1" / "ecos-board.yml"
            board.write_text(
                "schema: 2\n"
                "id: starrysky-l3-1\n"
                "target: ysyx-l3\n"
                "build:\n"
                "  default_profile: mem\n"
                "  profiles:\n"
                "    mem:\n"
                "      kconfig:\n"
                "        PROFILE_MEM: y\n"
                "kconfig:\n"
                "  board: board.kconfig\n",
                encoding="utf-8",
            )
            (board.parent / "board.kconfig").write_text(
                'config PROFILE_MEM\n'
                '    bool "MEM profile"\n'
                "    default n\n",
                encoding="utf-8",
            )
            target = sdk / "components" / "soc" / "ysyx-l3" / "Kconfig"
            target.write_text(
                'config TARGET_OPTION\n'
                '    bool "Target option"\n'
                "    default n\n",
                encoding="utf-8",
            )
            manifest = sdk / "components" / "soc" / "ysyx-l3" / "ecos-soc.yml"
            manifest.write_text(
                manifest.read_text(encoding="utf-8") + "  kconfig: Kconfig\n",
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
                        "hello",
                        "--path",
                        str(workspace),
                        "--board",
                        "starrysky-l3-1",
                        "--format",
                        "json",
                    ]
                )
            self.assertEqual(result, ExitCode.OK)
            project_root = workspace / "hello"
            configuration.configure_project(
                sdk_context(sdk), project_root=project_root
            )
            generated_header = (
                project_root / ".ecos" / "generated" / "sdkconfig.h"
            ).read_text(encoding="utf-8")
            self.assertIn("#define CONFIG_PROFILE_MEM 1", generated_header)


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
            image_size = len(b"bin-output")
            resolved["target"]["memory"]["flash"] = {
                "origin": 0x1000,
                "size": image_size,
            }
            with mock.patch("ecos_cli.artifacts.inspect_elf", return_value=fake_elf()):
                limited = artifacts.create_manifest(root, resolved)
            self.assertEqual(limited["firmware"]["size_limit"], image_size)
            self.assertEqual(
                artifacts.load_manifest(root)["firmware"]["size_limit"], image_size
            )
            resolved["target"]["memory"]["flash"]["size"] = image_size - 1
            with mock.patch("ecos_cli.artifacts.inspect_elf", return_value=fake_elf()):
                with self.assertRaisesRegex(
                    artifacts.ArtifactValidationError, "exceeds Target.memory.flash.size"
                ):
                    artifacts.create_manifest(root, resolved)
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


class WlinkFlashTest(unittest.TestCase):
    @staticmethod
    def status_output(protected: bool) -> str:
        state = "true" if protected else "false"
        return (
            "[INFO] Connected to WCH-Link v3.2(v38) (WCH-LinkE-CH32V305)\n"
            "[INFO] Attached chip: CH32V103\n"
            "[INFO] Chip ESIG: FlashSize(64KB) UID(01-02-03)\n"
            f"[INFO] Flash protected: {state}\n"
        )

    def test_wlink_unprotects_and_flashes(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk, project, tool = prepare_wlink_project(root)
            run = mock.Mock()
            run.side_effect = [
                mock.Mock(returncode=0, stdout=self.status_output(True), stderr=None),
                mock.Mock(
                    returncode=0,
                    stdout="[INFO] Unprotect Flash\n[INFO] Read protected: false\n",
                    stderr=None,
                ),
                mock.Mock(
                    returncode=0,
                    stdout="[INFO] Flashing 4 bytes to 0x08000000\n[INFO] Flash done\n",
                    stderr=None,
                ),
            ]
            with mock.patch("ecos_cli.flashing.subprocess.run", run):
                data = flashing.flash_project(
                    sdk_context(sdk), project_root=project
                )

            self.assertEqual(data["provider"], "wlink")
            self.assertEqual(data["chip"], "CH32V103")
            self.assertEqual(data["attached_chip"], "CH32V103")
            self.assertIn("WCH-LinkE", data["probe"])
            self.assertTrue(data["read_protected"])
            self.assertTrue(data["unprotected"])
            self.assertTrue(data["erase"])
            self.assertTrue(data["artifact"].endswith("retrosoc_fw.hex"))
            self.assertIn("[INFO] Flash done", data["tool_output"])
            self.assertEqual(
                [call.args[0] for call in run.call_args_list],
                [
                    [str(tool), "--chip", "CH32V103", "status"],
                    [str(tool), "--chip", "CH32V103", "unprotect"],
                    [
                        str(tool),
                        "--chip",
                        "CH32V103",
                        "flash",
                        "-e",
                        str(project / "build" / "retrosoc_fw.hex"),
                    ],
                ],
            )

    def test_wlink_requires_opt_in_for_protected_chip(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk, project, _ = prepare_wlink_project(root, unprotect=False)
            run = mock.Mock(
                return_value=mock.Mock(
                    returncode=0, stdout=self.status_output(True), stderr=None
                )
            )
            with mock.patch("ecos_cli.flashing.subprocess.run", run):
                with self.assertRaisesRegex(
                    flashing.FlashConfigurationError, "read-protected"
                ):
                    flashing.flash_project(
                        sdk_context(sdk), project_root=project
                    )
            self.assertEqual(run.call_count, 1)

    def test_wlink_flash_failure_reports_tool_output(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk, project, _ = prepare_wlink_project(root)
            run = mock.Mock()
            run.side_effect = [
                mock.Mock(returncode=0, stdout=self.status_output(False), stderr=None),
                mock.Mock(
                    returncode=1,
                    stdout="[ERROR] failed to write flash\n",
                    stderr=None,
                ),
            ]
            with mock.patch("ecos_cli.flashing.subprocess.run", run):
                with self.assertRaisesRegex(
                    flashing.FlashWriteError, "failed to write flash"
                ):
                    flashing.flash_project(
                        sdk_context(sdk), project_root=project
                    )

    def test_wlink_board_requires_chip(self):
        with tempfile.TemporaryDirectory() as directory:
            sdk = create_sdk(Path(directory) / "sdk")
            path = sdk / "boards" / "StarrySkyL4" / "ecos-board.yml"
            path.write_text(
                path.read_text(encoding="utf-8")
                + "flash:\n"
                + "  provider: wlink\n"
                + "  artifact: hex\n",
                encoding="utf-8",
            )
            with self.assertRaisesRegex(ManifestValidationError, "chip"):
                _read_manifest(path, "Board")

    def test_wlink_missing_tool_is_configuration_error(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk, project, tool = prepare_wlink_project(root)
            tool.unlink()
            with self.assertRaisesRegex(
                flashing.FlashConfigurationError, "does not exist"
            ):
                flashing.flash_project(sdk_context(sdk), project_root=project)

    def test_cli_wlink_flash_json_and_text(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk, project, _ = prepare_wlink_project(root)
            for output_format in ("json", "text"):
                run = mock.Mock()
                run.side_effect = [
                    mock.Mock(
                        returncode=0,
                        stdout=self.status_output(False),
                        stderr=None,
                    ),
                    mock.Mock(
                        returncode=0,
                        stdout="[INFO] Flash done\n",
                        stderr=None,
                    ),
                ]
                output = StringIO()
                log_output = StringIO()
                with (
                    mock.patch("ecos_cli.flashing.subprocess.run", run),
                    redirect_stdout(output),
                    redirect_stderr(log_output),
                ):
                    result = main(
                        [
                            "--sdk",
                            str(sdk),
                            "flash",
                            "--project",
                            str(project),
                            "--format",
                            output_format,
                        ]
                    )
                self.assertEqual(result, ExitCode.OK)
                if output_format == "json":
                    payload = json.loads(output.getvalue())
                    self.assertEqual(payload["data"]["provider"], "wlink")
                    self.assertEqual(payload["data"]["chip"], "CH32V103")
                    self.assertFalse(payload["data"]["unprotected"])
                else:
                    text = log_output.getvalue()
                    self.assertIn("Probe: WCH-Link", text)
                    self.assertIn("Chip: CH32V103", text)
                    self.assertIn("SHA-256:", text)



if __name__ == "__main__":
    unittest.main()
