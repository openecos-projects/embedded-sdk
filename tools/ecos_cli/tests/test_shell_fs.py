import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from host_compile import compiler_flags


SDK_ROOT = Path(__file__).parents[3]
HOST_CC = shutil.which("cc")


@unittest.skipUnless(HOST_CC, "a host C compiler is required")
class ShellFsTest(unittest.TestCase):
    def test_file_commands(self):
        with tempfile.TemporaryDirectory() as directory:
            executable = Path(directory) / "shell-fs-test"
            component = SDK_ROOT / "components" / "letter-shell-fs"
            command = [
                HOST_CC,
                *compiler_flags("c11", strict=True),
                "-I",
                str(component / "include"),
                "-I",
                str(SDK_ROOT / "components" / "fatfs" / "include"),
                "-I",
                str(SDK_ROOT / "components" / "letter-shell" / "include"),
                "-I",
                str(SDK_ROOT / "components" / "core" / "include"),
                "-I",
                str(SDK_ROOT / "boards" / "include"),
                str(component / "src" / "shell_fs.c"),
                str(component / "tests" / "test_shell_fs.c"),
                "-o",
                str(executable),
            ]
            subprocess.run(command, check=True, capture_output=True, text=True)
            subprocess.run(
                [str(executable)], check=True, capture_output=True, text=True
            )


if __name__ == "__main__":
    unittest.main()
