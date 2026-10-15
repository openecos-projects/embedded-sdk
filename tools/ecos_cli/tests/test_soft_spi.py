import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


SDK_ROOT = Path(__file__).parents[3]
HOST_CC = shutil.which("cc")


@unittest.skipUnless(HOST_CC, "a host C compiler is required")
class SoftSpiTest(unittest.TestCase):
    def test_modes_validation_and_transfer(self):
        with tempfile.TemporaryDirectory() as directory:
            executable = Path(directory) / "soft-spi-test"
            component = SDK_ROOT / "components" / "soft_spi"
            command = [
                HOST_CC,
                "-std=c11",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-pedantic",
                "-I",
                str(component / "include"),
                "-I",
                str(SDK_ROOT / "drivers" / "gpio" / "include"),
                "-I",
                str(SDK_ROOT / "components" / "core" / "include"),
                str(component / "src" / "soft_spi.c"),
                str(component / "tests" / "test_soft_spi.c"),
                "-o",
                str(executable),
            ]
            subprocess.run(command, check=True, capture_output=True, text=True)
            subprocess.run([str(executable)], check=True, capture_output=True, text=True)


if __name__ == "__main__":
    unittest.main()
