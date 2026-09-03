import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


SDK_ROOT = Path(__file__).parents[3]
CORE_ROOT = SDK_ROOT / "components" / "core"
HOST_CC = shutil.which("cc")


@unittest.skipUnless(HOST_CC, "a host C compiler is required")
class CoreRuntimeTest(unittest.TestCase):
    def compile_and_run(
        self, descriptions: int, source_location: int = 0, log_level: int = 1
    ) -> None:
        with tempfile.TemporaryDirectory() as directory:
            executable = Path(directory) / "core-runtime-test"
            command = [
                HOST_CC,
                "-std=c11",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-pedantic",
                f"-DCONFIG_ECOS_LOG_LEVEL={log_level}",
                "-DCONFIG_ECOS_LOG_BUFFER_SIZE=256",
                "-DCONFIG_ECOS_LOG_COLOR=0",
                f"-DCONFIG_ECOS_LOG_SOURCE_LOCATION={source_location}",
                f"-DCONFIG_ECOS_ERROR_DESCRIPTIONS={descriptions}",
                "-I",
                str(CORE_ROOT / "include"),
                str(CORE_ROOT / "src" / "error.c"),
                str(CORE_ROOT / "src" / "log.c"),
                str(CORE_ROOT / "tests" / "test_core_runtime.c"),
                "-o",
                str(executable),
            ]
            subprocess.run(command, check=True, capture_output=True, text=True)
            subprocess.run(
                [str(executable)], check=True, capture_output=True, text=True
            )

    def test_error_descriptions_enabled(self):
        self.compile_and_run(1)

    def test_error_descriptions_disabled(self):
        self.compile_and_run(0)

    def test_source_location_enabled(self):
        self.compile_and_run(1, source_location=1, log_level=0)

    def test_bsp_console_installs_log_writer(self):
        with tempfile.TemporaryDirectory() as directory:
            executable = Path(directory) / "console-integration-test"
            command = [
                HOST_CC,
                "-std=c11",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-pedantic",
                "-DCONFIG_ECOS_LOG_LEVEL=0",
                "-DCONFIG_ECOS_LOG_SOURCE_LOCATION=0",
                "-I",
                str(CORE_ROOT / "include"),
                "-I",
                str(SDK_ROOT / "board" / "include"),
                "-I",
                str(SDK_ROOT / "drivers" / "uart" / "include"),
                str(CORE_ROOT / "src" / "error.c"),
                str(CORE_ROOT / "src" / "log.c"),
                str(SDK_ROOT / "board" / "StarrySkyL4" / "bsp" / "console.c"),
                str(CORE_ROOT / "tests" / "test_console_integration.c"),
                "-o",
                str(executable),
            ]
            subprocess.run(command, check=True, capture_output=True, text=True)
            subprocess.run(
                [str(executable)], check=True, capture_output=True, text=True
            )


if __name__ == "__main__":
    unittest.main()
