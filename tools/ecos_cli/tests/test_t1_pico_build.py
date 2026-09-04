import importlib.util
import json
import shutil
import sys
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from io import StringIO
from pathlib import Path


SOURCE_ROOT = Path(__file__).parents[1] / "src"
SDK_ROOT = Path(__file__).parents[3]
sys.path.insert(0, str(SOURCE_ROOT))

from ecos_cli import dependencies, toolchain  # noqa: E402
from ecos_cli.cli import ExitCode, main  # noqa: E402


REQUIRED_TOOLS = ("cmake", "ninja")


def host_tool_is_ready(name: str) -> bool:
    spec = next(
        item
        for item in dependencies.HOST_TOOL_DEPENDENCIES
        if item["name"] == name
    )
    return bool(
        dependencies.managed_host_tool(
            dependencies.host_dependency_root(SDK_ROOT), name
        )
        or shutil.which(spec["executable"])
    )


def sdk_toolchain_is_ready() -> bool:
    try:
        manifest = toolchain.load_manifest()
        host = toolchain.detect_host()
        status = toolchain.installation_status(
            manifest, toolchain.default_prefix(), host
        )
    except toolchain.ToolchainError:
        return False
    return status["state"] == "installed"


class StartySkyT1PicoCapabilityTest(unittest.TestCase):
    def test_t1_name_is_available_for_a_future_board(self):
        with tempfile.TemporaryDirectory() as directory:
            output = StringIO()
            with redirect_stdout(output), redirect_stderr(output):
                result = main(
                    [
                        "--sdk",
                        str(SDK_ROOT),
                        "project",
                        "create",
                        "hello",
                        "--path",
                        directory,
                        "--board",
                        "t1",
                        "--format",
                        "json",
                    ]
                )

            payload = json.loads(output.getvalue())
            self.assertEqual(result, ExitCode.CONFIG)
            self.assertEqual(
                payload["diagnostics"][0]["code"],
                "ECOS_PROJECT_BOARD_NOT_FOUND",
            )
            self.assertFalse((Path(directory) / "hello").exists())

    def test_board_bound_examples_remain_unavailable(self):
        expected_capabilities = {
            "pwm-basic": "pwm",
        }

        for example, capability in expected_capabilities.items():
            with self.subTest(example=example), tempfile.TemporaryDirectory() as directory:
                output = StringIO()
                with redirect_stdout(output), redirect_stderr(output):
                    result = main(
                        [
                            "--sdk",
                            str(SDK_ROOT),
                            "project",
                            "create",
                            example,
                            "--path",
                            directory,
                            "--board",
                            "t1-pico",
                            "--format",
                            "json",
                        ]
                    )

                payload = json.loads(output.getvalue())
                self.assertEqual(result, ExitCode.CONFIG)
                self.assertEqual(
                    payload["diagnostics"][0]["code"],
                    "ECOS_PROJECT_CAPABILITY_MISMATCH",
                )
                self.assertIn(capability, payload["diagnostics"][0]["message"])
                self.assertFalse((Path(directory) / example).exists())


@unittest.skipUnless(
    all(host_tool_is_ready(tool) for tool in REQUIRED_TOOLS)
    and sdk_toolchain_is_ready()
    and importlib.util.find_spec("kconfiglib") is not None,
    "SDK Python/CMake/Ninja dependencies or the SDK toolchain is not installed",
)
class StartySkyT1PicoBuildTest(unittest.TestCase):
    def create_and_build(self, example: str) -> tuple[Path, StringIO, tempfile.TemporaryDirectory]:
        temporary = tempfile.TemporaryDirectory()
        output = StringIO()
        with redirect_stdout(output), redirect_stderr(output):
            create_result = main(
                [
                    "--sdk",
                    str(SDK_ROOT),
                    "project",
                    "create",
                    example,
                    "--path",
                    temporary.name,
                    "--board",
                    "t1-pico",
                ]
            )
            project_root = Path(temporary.name) / example
            build_result = main(
                [
                    "--sdk",
                    str(SDK_ROOT),
                    "build",
                    "--project",
                    str(project_root),
                ]
            )

        self.assertEqual(create_result, ExitCode.OK, output.getvalue())
        self.assertEqual(build_result, ExitCode.OK, output.getvalue())
        return project_root, output, temporary

    def test_hello_build_uses_cl1_uart_and_t1_pico_console(self):
        project_root, _, temporary = self.create_and_build("hello")
        self.addCleanup(temporary.cleanup)
        firmware = project_root / "build" / "retrosoc_fw"
        self.assertEqual(firmware.with_suffix(".elf").read_bytes()[:4], b"\x7fELF")
        self.assertGreater(firmware.with_suffix(".bin").stat().st_size, 0)

        compile_commands = project_root / "build" / "compile_commands.json"
        compiled_sources = {
            Path(item["file"]).resolve()
            for item in json.loads(compile_commands.read_text(encoding="utf-8"))
        }
        for source in (
            "components/soc/cl1-2512/startup/start.S",
            "components/soc/cl1-2512/hal/uart/uart.c",
            "board/StartySkyT1Pico/bsp/console.c",
            "drivers/uart/src/uart.c",
        ):
            self.assertIn((SDK_ROOT / source).resolve(), compiled_sources)

        text = firmware.with_suffix(".txt").read_text(encoding="utf-8")
        for symbol in (
            "<_start>",
            "<main>",
            "<bsp_console_init>",
            "<hal_uart_init>",
        ):
            self.assertIn(symbol, text)

    def test_blink_build_uses_t1_pico_led_and_cl1_gpio_timer(self):
        project_root, _, temporary = self.create_and_build("blink")
        self.addCleanup(temporary.cleanup)
        firmware = project_root / "build" / "retrosoc_fw"
        compile_commands = project_root / "build" / "compile_commands.json"
        compiled_sources = {
            Path(item["file"]).resolve()
            for item in json.loads(compile_commands.read_text(encoding="utf-8"))
        }
        for source in (
            "components/soc/cl1-2512/hal/gpio/gpio.c",
            "components/soc/cl1-2512/hal/timer/timer.c",
            "board/StartySkyT1Pico/bsp/led.c",
            "drivers/gpio/src/gpio.c",
            "drivers/timer/src/timer.c",
        ):
            self.assertIn((SDK_ROOT / source).resolve(), compiled_sources)

        text = firmware.with_suffix(".txt").read_text(encoding="utf-8")
        for symbol in (
            "<bsp_led_init>",
            "<bsp_led_set_state>",
            "<ecos_timer_delay_ms>",
            "<hal_gpio_set_level>",
            "<hal_timer_delay_us>",
        ):
            self.assertIn(symbol, text)

    def test_gpio_basic_build_uses_t1_pico_gpio_demo_pins(self):
        project_root, _, temporary = self.create_and_build("gpio-basic")
        self.addCleanup(temporary.cleanup)
        firmware = project_root / "build" / "retrosoc_fw"
        compile_commands = project_root / "build" / "compile_commands.json"
        compiled_sources = {
            Path(item["file"]).resolve()
            for item in json.loads(compile_commands.read_text(encoding="utf-8"))
        }
        self.assertIn((project_root / "main.c").resolve(), compiled_sources)
        for source in (
            "components/soc/cl1-2512/hal/gpio/gpio.c",
            "drivers/gpio/src/gpio.c",
        ):
            self.assertIn((SDK_ROOT / source).resolve(), compiled_sources)

        text = firmware.with_suffix(".txt").read_text(encoding="utf-8")
        for symbol in (
            "<ecos_gpio_configure>",
            "<ecos_gpio_get_level>",
            "<ecos_gpio_set_level>",
            "<hal_gpio_configure>",
            "<hal_gpio_get_level>",
            "<hal_gpio_set_level>",
        ):
            self.assertIn(symbol, text)

    def test_i2c_scan_build_uses_cl1_i2c_stack(self):
        project_root, _, temporary = self.create_and_build("i2c-scan")
        self.addCleanup(temporary.cleanup)
        firmware = project_root / "build" / "retrosoc_fw"
        compile_commands = project_root / "build" / "compile_commands.json"
        compiled_sources = {
            Path(item["file"]).resolve()
            for item in json.loads(compile_commands.read_text(encoding="utf-8"))
        }
        for source in (
            "components/soc/cl1-2512/hal/i2c/i2c.c",
            "drivers/i2c/src/i2c.c",
        ):
            self.assertIn((SDK_ROOT / source).resolve(), compiled_sources)

        text = firmware.with_suffix(".txt").read_text(encoding="utf-8")
        for symbol in (
            "<ecos_i2c_init>",
            "<ecos_i2c_probe>",
            "<hal_i2c_init>",
            "<hal_i2c_probe>",
        ):
            self.assertIn(symbol, text)


if __name__ == "__main__":
    unittest.main()
