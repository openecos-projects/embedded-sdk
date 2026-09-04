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


REQUIRED_TOOLS = (
    "cmake",
    "ninja",
)


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


@unittest.skipUnless(
    all(host_tool_is_ready(tool) for tool in REQUIRED_TOOLS)
    and sdk_toolchain_is_ready()
    and importlib.util.find_spec("kconfiglib") is not None,
    "SDK Python/CMake/Ninja dependencies or the SDK toolchain is not installed",
)
class StarrySkyL4BuildTest(unittest.TestCase):
    def test_gpio_basic_build_links_gpio_input_and_output_stack(self):
        with tempfile.TemporaryDirectory() as directory:
            output = StringIO()
            with redirect_stdout(output), redirect_stderr(output):
                create_result = main(
                    [
                        "--sdk",
                        str(SDK_ROOT),
                        "project",
                        "create",
                        "gpio-basic",
                        "--path",
                        directory,
                        "--board",
                        "starrysky-l4",
                    ]
                )
                project_root = Path(directory) / "gpio-basic"
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
            firmware = project_root / "build" / "retrosoc_fw"
            self.assertEqual(
                firmware.with_suffix(".elf").read_bytes()[:4], b"\x7fELF"
            )
            text = firmware.with_suffix(".txt").read_text(encoding="utf-8")
            for symbol in (
                "<main>",
                "<bsp_console_init>",
                "<ecos_gpio_configure>",
                "<ecos_gpio_get_level>",
                "<ecos_gpio_set_level>",
                "<hal_gpio_configure>",
                "<hal_gpio_get_level>",
                "<hal_gpio_set_level>",
            ):
                self.assertIn(symbol, text)

    def test_blink_build_links_bsp_gpio_and_timer_stack(self):
        with tempfile.TemporaryDirectory() as directory:
            output = StringIO()
            with redirect_stdout(output), redirect_stderr(output):
                create_result = main(
                    [
                        "--sdk",
                        str(SDK_ROOT),
                        "project",
                        "create",
                        "blink",
                        "--path",
                        directory,
                        "--board",
                        "starrysky-l4",
                    ]
                )
                project_root = Path(directory) / "blink"
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
            firmware = project_root / "build" / "retrosoc_fw"
            self.assertEqual(firmware.with_suffix(".elf").read_bytes()[:4], b"\x7fELF")
            self.assertGreater(firmware.with_suffix(".bin").stat().st_size, 0)
            compile_commands = project_root / "build" / "compile_commands.json"
            compiled_sources = {
                Path(item["file"]).resolve()
                for item in json.loads(compile_commands.read_text(encoding="utf-8"))
            }
            self.assertIn(
                (SDK_ROOT / "drivers/timer/src/timer.c").resolve(), compiled_sources
            )
            self.assertIn(
                (SDK_ROOT / "components/soc/ysyx-2512/hal/timer/timer.c").resolve(),
                compiled_sources,
            )
            self.assertIn(
                (SDK_ROOT / "components/core/src/error.c").resolve(),
                compiled_sources,
            )
            self.assertIn(
                (SDK_ROOT / "components/core/src/log.c").resolve(),
                compiled_sources,
            )
            text = firmware.with_suffix(".txt").read_text(encoding="utf-8")
            for symbol in (
                "<main>",
                "<bsp_console_init>",
                "<bsp_led_init>",
                "<bsp_led_set_state>",
                "<ecos_log_set_writer>",
                "<ecos_log_write>",
                "<ecos_log_error>",
                "<ecos_gpio_configure>",
                "<ecos_gpio_set_level>",
                "<ecos_timer_get_instance_count>",
                "<ecos_timer_delay_ms>",
                "<hal_gpio_configure>",
                "<hal_gpio_set_level>",
                "<hal_timer_get_instance_count>",
                "<hal_timer_delay_us>",
            ):
                self.assertIn(symbol, text)

    def test_hello_build_produces_executable_bin_and_disassembly(self):
        with tempfile.TemporaryDirectory() as directory:
            output = StringIO()
            with redirect_stdout(output), redirect_stderr(output):
                create_result = main(
                    [
                        "--sdk",
                        str(SDK_ROOT),
                        "project",
                        "create",
                        "hello",
                        "--path",
                        directory,
                        "--board",
                        "starrysky-l4",
                    ]
                )
                project_root = Path(directory) / "hello"
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
            firmware = project_root / "build" / "retrosoc_fw"
            elf = firmware.with_suffix(".elf")
            binary = firmware.with_suffix(".bin")
            disassembly = firmware.with_suffix(".txt")
            memory_map = firmware.with_suffix(".map")
            size_report = firmware.with_suffix(".size")
            self.assertEqual(elf.read_bytes()[:4], b"\x7fELF")
            self.assertGreater(binary.stat().st_size, 0)
            self.assertTrue(memory_map.is_file())
            self.assertTrue(size_report.is_file())
            compile_commands = project_root / "build" / "compile_commands.json"
            self.assertTrue(compile_commands.is_file())
            compiled_sources = {
                Path(item["file"]).resolve()
                for item in json.loads(compile_commands.read_text(encoding="utf-8"))
            }
            self.assertIn(
                (SDK_ROOT / "drivers/gpio/src/gpio.c").resolve(), compiled_sources
            )
            self.assertIn(
                (SDK_ROOT / "components/soc/ysyx-2512/hal/gpio/gpio.c").resolve(),
                compiled_sources,
            )
            self.assertIn(
                (SDK_ROOT / "board/StarrySkyL4/bsp/button.c").resolve(),
                compiled_sources,
            )
            self.assertIn(
                (SDK_ROOT / "board/StarrySkyL4/bsp/led.c").resolve(),
                compiled_sources,
            )
            self.assertIn(
                (SDK_ROOT / "components/core/src/error.c").resolve(),
                compiled_sources,
            )
            self.assertIn(
                (SDK_ROOT / "components/core/src/log.c").resolve(),
                compiled_sources,
            )
            text = disassembly.read_text(encoding="utf-8")
            for symbol in (
                "<_start>",
                "<main>",
                "<bsp_console_init>",
                "<ecos_log_set_writer>",
                "<ecos_log_write>",
                "<ecos_result_succeeded>",
                "<hal_uart_init>",
            ):
                self.assertIn(symbol, text)


if __name__ == "__main__":
    unittest.main()
