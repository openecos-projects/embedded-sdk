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
from ecos_cli.artifacts import inspect_elf
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
        for example in ("adc-oneshot", "gpio-basic", "gpio-interrupt", "i2c-scan", "pwm-basic", "rtc-counter", "spi-loopback", "timer-polling", "timer-interrupt", "wdg-reset"):
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

    def test_ch32_does_not_claim_plic_support(self):
        output = StringIO()
        with tempfile.TemporaryDirectory() as directory, redirect_stdout(output):
            result = main([
                "--sdk", str(SDK_ROOT), "project", "create", "plic",
                "--board", "ch32v103c8t6", "--path", directory,
                "--dry-run", "--format", "json",
            ])
        self.assertEqual(result, 3, output.getvalue())
        payload = json.loads(output.getvalue())
        self.assertEqual(payload["diagnostics"][0]["code"], "ECOS_PROJECT_CAPABILITY_MISMATCH")
        self.assertIn("plic", payload["diagnostics"][0]["message"])

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
            self.assertFalse(any("hal/interrupt" in str(path) for path in compiled))
            self.assertNotIn("ecos_ch32v103_vector_table", inspect_elf(build / "retrosoc_fw.elf")["symbols"])
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

    def test_interrupt_builds_have_fixed_vectors_and_full_software_context(self):
        workspace = SDK_ROOT / "testdir"
        workspace.mkdir(exist_ok=True)
        for example, peripheral_source in (
            ("timer-interrupt", "hal/timer/timer_interrupt.c"),
            ("gpio-interrupt", "hal/gpio/gpio_interrupt.c"),
        ):
            with self.subTest(example=example), tempfile.TemporaryDirectory(
                prefix="ch32-irq-build-", dir=workspace
            ) as directory:
                project = Path(directory) / example
                output = StringIO()
                with redirect_stdout(output), redirect_stderr(output):
                    created = main([
                        "--sdk", str(SDK_ROOT), "project", "create", example,
                        "--board", "ch32v103c8t6", "--path", directory,
                    ])
                    self.assertEqual(created, 0, output.getvalue())
                    built = main(["--sdk", str(SDK_ROOT), "build", "--project", str(project)])
                self.assertEqual(built, 0, output.getvalue())
                build = project / "build"
                compiled = {Path(item["file"]).resolve() for item in json.loads(
                    (build / "compile_commands.json").read_text()
                )}
                for source in (peripheral_source, "hal/interrupt/interrupt.c", "hal/interrupt/trap.S"):
                    self.assertIn((SDK_ROOT / "components/soc/ch32v103" / source).resolve(), compiled)
                elf_path = build / "retrosoc_fw.elf"
                elf = inspect_elf(elf_path)
                self.assertEqual(elf["entry"], 0)
                symbols = elf["symbols"]
                start = symbols["ecos_ch32v103_vector_table"]
                end = symbols["ecos_ch32v103_vector_table_end"]
                trap = symbols["ecos_ch32v103_trap_entry"]
                trap_end = symbols["ecos_ch32v103_trap_entry_end"]
                self.assertEqual(start % 256, 0)
                self.assertEqual(end - start, 60 * 4)

                def at(address, size):
                    section = next(item for item in elf["sections"] if
                                   item["name"] == ".text")
                    offset = section["offset"] + address - section["address"]
                    return elf_path.read_bytes()[offset:offset + size]

                # Every PFIC slot must be a 32-bit JAL x0 to the common entry,
                # never a compressed instruction, address word or relaxed call.
                vectors = at(start, end - start)
                for slot in range(60):
                    word = int.from_bytes(vectors[slot * 4:slot * 4 + 4], "little")
                    self.assertEqual(word & 0xFFF, 0x06F)
                    imm = (((word >> 31) & 1) << 20 | ((word >> 21) & 0x3FF) << 1 |
                           ((word >> 20) & 1) << 11 | ((word >> 12) & 0xFF) << 12)
                    if imm & (1 << 20):
                        imm -= 1 << 21
                    self.assertEqual(start + slot * 4 + imm, trap)

                body = at(trap, trap_end - trap)
                words = [int.from_bytes(body[i:i + 4], "little") for i in range(0, len(body), 4)]
                stores = {}
                loads = {}
                for word in words:
                    opcode = word & 0x7F
                    base = (word >> 15) & 31
                    funct3 = (word >> 12) & 7
                    if base == 2 and funct3 == 2:
                        if opcode == 0x23:
                            offset = ((word >> 25) << 5) | ((word >> 7) & 31)
                            stores.setdefault((word >> 20) & 31, []).append(offset)
                        elif opcode == 0x03:
                            loads[(word >> 7) & 31] = word >> 20
                registers = [1, 3, 4, *range(5, 32)]
                for offset, register in enumerate(registers):
                    self.assertIn(offset * 4, stores[register])
                    self.assertEqual(loads[register], offset * 4)
                self.assertEqual(words[0], 0xF7010113)  # addi sp,sp,-144
                self.assertEqual(words[-2], 0x09010113)  # addi sp,sp,144
                self.assertEqual(words[-1], 0x30200073)  # mret
                self.assertIn(0x34129073, words)  # csrw mepc,t0
                self.assertIn(0x30029073, words)  # csrw mstatus,t0
                binary = (build / "retrosoc_fw.bin").read_bytes()
                self.assertEqual(intel_hex_image((build / "retrosoc_fw.hex").read_text()), (0, binary))

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
            (
                "adc-oneshot",
                ("components/soc/ch32v103/hal/adc/adc.c", "drivers/adc/src/adc.c"),
                ("<ecos_adc_read>", "<hal_adc_read>"),
            ),
            (
                "i2c-scan",
                ("components/soc/ch32v103/hal/i2c/i2c.c", "drivers/i2c/src/i2c.c"),
                ("<ecos_i2c_probe>", "<hal_i2c_probe>"),
            ),
            (
                "pwm-basic",
                ("components/soc/ch32v103/hal/pwm/pwm.c", "drivers/pwm/src/pwm.c"),
                ("<ecos_pwm_start>", "<hal_pwm_set_duty_cycle>"),
            ),
            (
                "rtc-counter",
                ("components/soc/ch32v103/hal/rtc/rtc.c", "drivers/rtc/src/rtc.c"),
                ("<ecos_rtc_get_counter>", "<hal_rtc_set_alarm>"),
            ),
            (
                "spi-loopback",
                ("components/soc/ch32v103/hal/spi/spi.c", "drivers/spi/src/spi.c"),
                ("<ecos_spi_transfer>", "<hal_spi_transfer_byte>"),
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
