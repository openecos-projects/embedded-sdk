import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


SDK_ROOT = Path(__file__).parents[3]
HOST_CC = shutil.which("cc")


@unittest.skipUnless(HOST_CC, "a host C compiler is required")
class FccTest(unittest.TestCase):
    def test_compiler_bounds_and_programs(self):
        with tempfile.TemporaryDirectory() as directory:
            executable = Path(directory) / "fcc-test"
            component = SDK_ROOT / "components" / "fcc"
            command = [
                HOST_CC,
                "-std=c11",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-pedantic",
                "-I",
                str(component / "include"),
                str(component / "src" / "fcc_lexer.c"),
                str(component / "src" / "fcc_parser.c"),
                str(component / "src" / "fcc_codegen.c"),
                str(component / "tests" / "test_fcc.c"),
                "-o",
                str(executable),
            ]
            subprocess.run(command, check=True, capture_output=True, text=True)
            subprocess.run(
                [str(executable)], check=True, capture_output=True, text=True
            )


if __name__ == "__main__":
    unittest.main()
