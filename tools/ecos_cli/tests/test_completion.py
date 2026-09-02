import shutil
import subprocess
import sys
import unittest
from contextlib import redirect_stdout
from io import StringIO
from pathlib import Path


SOURCE_ROOT = Path(__file__).parents[1] / "src"
sys.path.insert(0, str(SOURCE_ROOT))

from ecos_cli import completion  # noqa: E402
from ecos_cli.cli import ExitCode, main  # noqa: E402


class CompletionTest(unittest.TestCase):
    def test_generates_all_supported_shells(self):
        for shell in completion.SUPPORTED_SHELLS:
            with self.subTest(shell=shell):
                generated = completion.generate(shell)
                self.assertTrue(generated.endswith("\n"))
                self.assertIn("toolchain", generated)
                self.assertIn("project", generated)
                self.assertIn("build", generated)
                self.assertIn("menuconfig", generated)
                self.assertIn("create", generated)
                self.assertIn("set-board", generated)
                self.assertIn("set-target", generated)
                self.assertIn("-l name" if shell == "fish" else "--name", generated)
                self.assertIn("completion", generated)
                self.assertIn("powershell", generated)
                self.assertNotIn("init_project", generated)
                self.assertNotIn("set_board", generated)

    def test_cli_emits_sourceable_completion_without_log_prefix(self):
        output = StringIO()
        with redirect_stdout(output):
            result = main(["completion", "bash"])

        self.assertEqual(result, ExitCode.OK)
        self.assertIn("complete -F _ecos_complete ecos", output.getvalue())
        self.assertNotIn("ECOS-INFO", output.getvalue())

    @unittest.skipUnless(shutil.which("bash"), "bash is not installed")
    def test_bash_completion_has_valid_syntax(self):
        result = subprocess.run(
            ["bash", "-n"],
            input=completion.generate("bash"),
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(result.returncode, 0, result.stderr)

    @unittest.skipUnless(shutil.which("zsh"), "zsh is not installed")
    def test_zsh_completion_has_valid_syntax(self):
        result = subprocess.run(
            ["zsh", "-n"],
            input=completion.generate("zsh"),
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()
