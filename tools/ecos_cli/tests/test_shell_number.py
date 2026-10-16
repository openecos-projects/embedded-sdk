import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from host_compile import compiler_flags, linker_flags


SDK_ROOT = Path(__file__).parents[3]
HOST_CC = shutil.which("cc")


@unittest.skipUnless(HOST_CC, "a host C compiler is required")
class ShellNumberTest(unittest.TestCase):
    def test_integer_arguments_and_invalid_literals(self):
        with tempfile.TemporaryDirectory() as directory:
            executable = Path(directory) / "shell-number-test"
            command = [
                HOST_CC,
                *compiler_flags("gnu11"),
                *linker_flags(),
                "-I",
                str(SDK_ROOT / "components" / "letter-shell" / "include"),
                "-I",
                str(SDK_ROOT / "components" / "core" / "include"),
                "-I",
                str(SDK_ROOT / "boards" / "include"),
                str(SDK_ROOT / "components" / "letter-shell" / "src" / "shell_ext.c"),
                str(SDK_ROOT / "components" / "letter-shell" / "tests" / "test_shell_number.c"),
                "-o",
                str(executable),
            ]
            subprocess.run(command, check=True, capture_output=True, text=True)
            subprocess.run([str(executable)], check=True, capture_output=True, text=True)

    def test_variable_addresses_keep_host_pointer_width(self):
        with tempfile.TemporaryDirectory() as directory:
            executable = Path(directory) / "shell-var-test"
            command = [
                HOST_CC,
                *compiler_flags("gnu11"),
                *linker_flags(),
                "-I",
                str(SDK_ROOT / "components" / "letter-shell" / "include"),
                "-I",
                str(SDK_ROOT / "components" / "core" / "include"),
                "-I",
                str(SDK_ROOT / "boards" / "include"),
                str(SDK_ROOT / "components" / "letter-shell" / "src" / "shell.c"),
                str(SDK_ROOT / "components" / "letter-shell" / "tests" / "test_shell_var.c"),
                "-o",
                str(executable),
            ]
            subprocess.run(command, check=True, capture_output=True, text=True)
            subprocess.run([str(executable)], check=True, capture_output=True, text=True)


if __name__ == "__main__":
    unittest.main()
