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

from ecos_cli import toolchain  # noqa: E402
from ecos_cli.cli import ExitCode, main  # noqa: E402


REQUIRED_TOOLS = (
    "cmake",
    "ninja",
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
    all(shutil.which(tool) for tool in REQUIRED_TOOLS) and sdk_toolchain_is_ready(),
    "CMake/Ninja or the SDK toolchain is not installed",
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
            memory_map = firmware.with_suffix(".map")
            size_report = firmware.with_suffix(".size")
            self.assertEqual(elf.read_bytes()[:4], b"\x7fELF")
            self.assertGreater(binary.stat().st_size, 0)
            self.assertTrue(memory_map.is_file())
            self.assertTrue(size_report.is_file())
            self.assertTrue((project_root / "build" / "compile_commands.json").is_file())
            text = disassembly.read_text(encoding="utf-8")
            for symbol in ("<_start>", "<main>", "<hal_sys_uart_init>"):
                self.assertIn(symbol, text)


if __name__ == "__main__":
    unittest.main()
