import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


SDK_ROOT = Path(__file__).parents[3]
HOST_CC = shutil.which("cc")


@unittest.skipUnless(HOST_CC, "a host C compiler is required")
class I2CDriverTest(unittest.TestCase):
    def test_public_api_validates_and_forwards_transactions(self):
        with tempfile.TemporaryDirectory() as directory:
            executable = Path(directory) / "i2c-driver-test"
            command = [
                HOST_CC,
                "-std=c11",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-pedantic",
                "-I",
                str(SDK_ROOT / "components/core/include"),
                "-I",
                str(SDK_ROOT / "hal/i2c/include"),
                "-I",
                str(SDK_ROOT / "drivers/i2c/include"),
                str(SDK_ROOT / "components/core/src/error.c"),
                str(SDK_ROOT / "drivers/i2c/src/i2c.c"),
                str(SDK_ROOT / "drivers/i2c/tests/test_i2c_driver.c"),
                "-o",
                str(executable),
            ]
            subprocess.run(command, check=True, capture_output=True, text=True)
            subprocess.run(
                [str(executable)], check=True, capture_output=True, text=True
            )


if __name__ == "__main__":
    unittest.main()
