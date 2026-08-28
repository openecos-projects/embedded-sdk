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

from ecos_cli.cli import ExitCode, main  # noqa: E402


REQUIRED_TOOLS = (
    "make",
    "riscv64-unknown-elf-gcc",
    "riscv64-unknown-elf-ld",
    "riscv64-unknown-elf-objdump",
    "riscv64-unknown-elf-objcopy",
)


@unittest.skipUnless(
    all(shutil.which(tool) for tool in REQUIRED_TOOLS),
    "RISC-V firmware build tools are not installed",
)
class StarrySkyL4BuildTest(unittest.TestCase):
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
            self.assertEqual(elf.read_bytes()[:4], b"\x7fELF")
            self.assertGreater(binary.stat().st_size, 0)
            text = disassembly.read_text(encoding="utf-8")
            for symbol in ("<_start>", "<main>", "<hal_sys_uart_init>"):
                self.assertIn(symbol, text)


if __name__ == "__main__":
    unittest.main()
