import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


SDK_ROOT = Path(__file__).parents[3]
HOST_CC = shutil.which("cc")


@unittest.skipUnless(HOST_CC, "a host C compiler is required")
class TextEditorTest(unittest.TestCase):
    def test_edit_save_and_capacity(self):
        with tempfile.TemporaryDirectory() as directory:
            executable = Path(directory) / "text-editor-test"
            component = SDK_ROOT / "components" / "text_editor"
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
                str(SDK_ROOT / "components" / "fatfs" / "include"),
                "-I",
                str(SDK_ROOT / "components" / "letter-shell" / "include"),
                "-I",
                str(SDK_ROOT / "components" / "core" / "include"),
                "-I",
                str(SDK_ROOT / "boards" / "include"),
                str(component / "src" / "text_editor.c"),
                str(component / "src" / "editor_render.c"),
                str(component / "tests" / "test_text_editor.c"),
                "-o",
                str(executable),
            ]
            subprocess.run(command, check=True, capture_output=True, text=True)
            subprocess.run(
                [str(executable)], check=True, capture_output=True, text=True
            )


if __name__ == "__main__":
    unittest.main()
