import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


SDK_ROOT = Path(__file__).parents[3]
HOST_CC = shutil.which("cc")


@unittest.skipUnless(HOST_CC, "a host C compiler is required")
class QSPIDriverTest(unittest.TestCase):
    def test_public_api_validates_and_forwards_transactions(self):
        with tempfile.TemporaryDirectory() as directory:
            executable = Path(directory) / "qspi-driver-test"
            command = [
                HOST_CC,
                "-std=c11",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-pedantic",
                "-ffunction-sections",
                "-fdata-sections",
                "-Wl,--gc-sections",
                "-I",
                str(SDK_ROOT / "components/core/include"),
                "-I",
                str(SDK_ROOT / "hal/qspi/include"),
                "-I",
                str(SDK_ROOT / "drivers/qspi/include"),
                str(SDK_ROOT / "components/core/src/error.c"),
                str(SDK_ROOT / "drivers/qspi/src/qspi.c"),
                str(SDK_ROOT / "drivers/qspi/tests/test_qspi_driver.c"),
                "-o",
                str(executable),
            ]
            subprocess.run(command, check=True, capture_output=True, text=True)
            subprocess.run(
                [str(executable)], check=True, capture_output=True, text=True
            )


if __name__ == "__main__":
    unittest.main()
