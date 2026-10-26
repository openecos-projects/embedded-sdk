"""CH32V103 compile/link validation for hello and basic GPIO/timer IP tests."""

import importlib.util
import json
import shutil
import subprocess
import sys
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from io import StringIO
from pathlib import Path
from unittest import mock

SDK_ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(SDK_ROOT / "tools/ecos_cli/src"))

from ecos_cli import dependencies, installer, toolchain
from ecos_cli.cli import main


def build_dependencies_ready():
    try:
        status = toolchain.compatible_installation_status(
            toolchain.load_manifest(), toolchain.default_prefix(), toolchain.detect_host(),
            legacy_roots=(toolchain.legacy_prefix(),),
        )
        host_root = dependencies.host_dependency_root(SDK_ROOT)
        return (
            status["state"] == "installed"
            and importlib.util.find_spec("kconfiglib") is not None
            and all(dependencies.managed_host_tool(host_root, name) or shutil.which(name)
                    for name in ("cmake", "ninja"))
        )
    except toolchain.ToolchainError:
        return False


def intel_hex_image(text):
    """Decode an Intel HEX file into (base_address, contiguous image bytes)."""
    upper = 0
    base = None
    image = bytearray()
    saw_eof = False

    for raw_line in text.splitlines():
        line = raw_line.strip()
        if not line:
            continue
        if not line.startswith(":"):
            raise AssertionError(f"not an Intel HEX record: {line!r}")
        record = bytes.fromhex(line[1:])
        count = record[0]
        address = int.from_bytes(record[1:3], "big")
        kind = record[3]
        payload = record[4:4 + count]
        if len(record) != count + 5 or sum(record) & 0xFF:
            raise AssertionError(f"invalid Intel HEX record: {line!r}")
        if kind == 0x00:
            absolute = upper + address
            if base is None:
                base = absolute
            if absolute != base + len(image):
                raise AssertionError("Intel HEX data is not contiguous")
            image.extend(payload)
        elif kind == 0x01:
            saw_eof = True
            break
        elif kind == 0x02:
            upper = int.from_bytes(payload, "big") << 4
        elif kind == 0x04:
            upper = int.from_bytes(payload, "big") << 16

    if base is None or not saw_eof:
        raise AssertionError("Intel HEX image is empty or has no EOF record")
    return base, bytes(image)


class CH32V103SelectionTest(unittest.TestCase):
    def test_hello_creation_does_not_require_installed_compiler(self):
        output = StringIO()
        with (
            tempfile.TemporaryDirectory() as directory,
            mock.patch.object(toolchain, "compatible_installation_status") as status,
            redirect_stdout(output),
            redirect_stderr(output),
        ):
            result = main([
                "--sdk", str(SDK_ROOT), "project", "create", "hello",
                "--board", "ch32v103c8t6", "--path", directory, "--format", "json",
            ])
            self.assertEqual(result, 0, output.getvalue())
            self.assertTrue((Path(directory) / "hello/.ecos/project.yml").is_file())
            status.assert_not_called()
        payload = json.loads(output.getvalue())
        self.assertEqual(payload["data"]["board"], "ch32v103c8t6")
        self.assertEqual(payload["data"]["target"], "ch32v103")
        self.assertIsNone(payload["data"]["profile"])  # Inherit the Board default.

    def test_installed_cli_includes_ch32v103_toolchain_support(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            host = root / "host"
            installer.install_host_cli(SDK_ROOT, host)
            result = subprocess.run([
                sys.executable, str(host / "bin/ecos"),
                "--sdk", str(SDK_ROOT), "project", "create", "hello",
                "--board", "ch32v103c8t6", "--path", str(root),
                "--dry-run", "--format", "json",
            ], capture_output=True, text=True, check=False)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            payload = json.loads(result.stdout)
            self.assertEqual(payload["data"]["board"], "ch32v103c8t6")
            self.assertFalse((root / "hello").exists())

    def test_basic_ip_examples_are_supported(self):
        for example in ("gpio-basic", "timer-polling", "wdg-reset"):
            with self.subTest(example=example), tempfile.TemporaryDirectory() as directory:
                output = StringIO()
                with redirect_stdout(output), redirect_stderr(output):
                    result = main([
                        "--sdk", str(SDK_ROOT), "project", "create", example,
                        "--board", "ch32v103c8t6", "--path", directory,
                        "--dry-run", "--format", "json",
                    ])
                self.assertEqual(result, 0, output.getvalue())
                payload = json.loads(output.getvalue())
                self.assertEqual(payload["data"]["board"], "ch32v103c8t6")
                self.assertEqual(payload["data"]["target"], "ch32v103")

    def test_unsupported_blink_is_rejected(self):
        output = StringIO()
        with tempfile.TemporaryDirectory() as directory, redirect_stdout(output), redirect_stderr(output):
            result = main([
                "--sdk", str(SDK_ROOT), "project", "create", "blink",
                "--board", "ch32v103c8t6", "--path", directory, "--format", "json",
            ])
            self.assertEqual(result, 3, output.getvalue())
            self.assertFalse((Path(directory) / "blink").exists())
        self.assertEqual(json.loads(output.getvalue())["diagnostics"][0]["code"],
                         "ECOS_PROJECT_CAPABILITY_MISMATCH")


@unittest.skipUnless(build_dependencies_ready(), "SDK build dependencies are unavailable")
class CH32V103BuildTest(unittest.TestCase):
    def test_hello_build_and_artifact_contract(self):
        workspace = SDK_ROOT / "testdir"
        workspace.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="ch32v103-test-", dir=workspace) as directory:
            project = Path(directory) / "hello"
            output = StringIO()
            with redirect_stdout(output), redirect_stderr(output):
                self.assertEqual(main([
                    "--sdk", str(SDK_ROOT), "project", "create", "hello",
                    "--board", "ch32v103c8t6", "--path", directory,
                ]), 0, output.getvalue())
                self.assertEqual(main([
                    "--sdk", str(SDK_ROOT), "build", "--project", str(project),
                ]), 0, output.getvalue())
            build = project / "build"
            artifact = json.loads((build / "artifacts.json").read_text())
            self.assertEqual(artifact["project"]["board"], "ch32v103c8t6")
            self.assertEqual(artifact["project"]["target"], "ch32v103")
            self.assertEqual(artifact["project"]["profile"], "flash")
            self.assertEqual(artifact["elf"]["class"], "ELF32")
            self.assertEqual(artifact["firmware"]["isa"], "rv32imac_zicsr_zifencei")
            self.assertEqual(artifact["firmware"]["abi"], "ilp32")
            binary = build / "retrosoc_fw.bin"
            self.assertGreater(binary.stat().st_size, 0)
            self.assertLessEqual(binary.stat().st_size, 64 * 1024)
            hex_base, hex_image = intel_hex_image(
                (build / "retrosoc_fw.hex").read_text()
            )
            self.assertEqual(hex_base, 0)
            self.assertEqual(hex_image, binary.read_bytes())
            compiled = {Path(item["file"]).resolve() for item in
                        json.loads((build / "compile_commands.json").read_text())}
            for source in (
                "components/soc/ch32v103/startup/start.S",
                "components/soc/ch32v103/hal/uart/uart.c",
                "boards/CH32V103C8T6/bsp/console.c",
                "drivers/uart/src/uart.c",
            ):
                self.assertIn(SDK_ROOT / source, compiled)
            self.assertFalse(any("cl1-2512" in str(path) for path in compiled))
            disassembly = (build / "retrosoc_fw.txt").read_text()
            for symbol in ("<_start>", "<main>", "<hal_uart_init>", "<bsp_console_init>"):
                self.assertIn(symbol, disassembly)

    def test_gpio_demo_resources_map_to_pa0_pa1(self):
        with tempfile.TemporaryDirectory() as directory:
            output = StringIO()
            with redirect_stdout(output), redirect_stderr(output):
                result = main([
                    "--sdk", str(SDK_ROOT), "project", "create", "gpio-basic",
                    "--board", "ch32v103c8t6", "--path", directory,
                ])
                self.assertEqual(result, 0, output.getvalue())
                project = Path(directory) / "gpio-basic"
                result = main([
                    "--sdk", str(SDK_ROOT), "configure", "--project", str(project),
                ])
            self.assertEqual(result, 0, output.getvalue())
            header = (
                Path(directory) / "gpio-basic/.ecos/generated/include/ecos/board_resources.h"
            ).read_text()
            self.assertIn('#define ECOS_BOARD_GPIO_DEMO_INPUT_LABEL "PA0"', header)
            self.assertIn('#define ECOS_BOARD_GPIO_DEMO_OUTPUT_LABEL "PA1"', header)
            self.assertIn("{ ECOS_GPIO_PORT_0, 0u }", header)
            self.assertIn("{ ECOS_GPIO_PORT_0, 1u }", header)

    def test_gpio_and_timer_builds_link_ch32_hal_sources(self):
        for example, sources, symbols in (
            (
                "gpio-basic",
                ("components/soc/ch32v103/hal/gpio/gpio.c", "drivers/gpio/src/gpio.c"),
                ("<ecos_gpio_configure>", "<hal_gpio_set_level>"),
            ),
            (
                "timer-polling",
                (
                    "components/soc/ch32v103/hal/timer/timer.c",
                    "components/soc/ch32v103/hal/gpio/gpio.c",
                    "drivers/timer/src/timer.c",
                    "drivers/gpio/src/gpio.c",
                ),
                ("<ecos_timer_start>", "<hal_timer_is_expired>", "<hal_gpio_set_level>"),
            ),
            (
                "wdg-reset",
                ("components/soc/ch32v103/hal/wdg/wdg.c", "drivers/wdg/src/wdg.c"),
                ("<ecos_wdg_init>", "<hal_wdg_feed>"),
            ),
        ):
            with self.subTest(example=example):
                workspace = SDK_ROOT / "testdir"
                workspace.mkdir(exist_ok=True)
                with tempfile.TemporaryDirectory(prefix=f"{example}-test-", dir=workspace) as directory:
                    project = Path(directory) / example
                    output = StringIO()
                    with redirect_stdout(output), redirect_stderr(output):
                        create_result = main([
                            "--sdk", str(SDK_ROOT), "project", "create", example,
                            "--board", "ch32v103c8t6", "--path", directory,
                        ])
                        build_result = main([
                            "--sdk", str(SDK_ROOT), "build", "--project", str(project),
                        ])
                    self.assertEqual(create_result, 0, output.getvalue())
                    self.assertEqual(build_result, 0, output.getvalue())
                    build = project / "build"
                    compiled = {
                        Path(item["file"]).resolve()
                        for item in json.loads((build / "compile_commands.json").read_text())
                    }
                    for source in sources:
                        self.assertIn((SDK_ROOT / source).resolve(), compiled)
                    disassembly = (build / "retrosoc_fw.txt").read_text()
                    for symbol in symbols:
                        self.assertIn(symbol, disassembly)


if __name__ == "__main__":
    unittest.main()
