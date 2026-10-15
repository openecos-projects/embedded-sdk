import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


SDK_ROOT = Path(__file__).parents[3]
HOST_CC = shutil.which("cc")


@unittest.skipUnless(HOST_CC, "a host C compiler is required")
class LegacySimpleUartTest(unittest.TestCase):
    def test_idle_and_received_bytes(self):
        with tempfile.TemporaryDirectory() as directory:
            executable = Path(directory) / "legacy-simple-uart-test"
            source_root = SDK_ROOT / "components" / "soc" / "legacy-common"
            command = [
                HOST_CC,
                "-std=c11",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-pedantic",
                "-I",
                str(source_root / "tests"),
                "-I",
                str(SDK_ROOT / "hal" / "uart" / "include"),
                "-I",
                str(SDK_ROOT / "components" / "core" / "include"),
                str(source_root / "hal" / "uart.c"),
                str(source_root / "tests" / "test_simple_uart.c"),
                "-o",
                str(executable),
            ]
            subprocess.run(command, check=True, capture_output=True, text=True)
            subprocess.run(
                [str(executable)], check=True, capture_output=True, text=True
            )


if __name__ == "__main__":
    unittest.main()
