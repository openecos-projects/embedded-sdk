import importlib.util
import json
import subprocess
import sys
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from io import StringIO
from pathlib import Path


SOURCE_ROOT = Path(__file__).parents[1] / "src"
SDK_ROOT = Path(__file__).parents[3]
sys.path.insert(0, str(SOURCE_ROOT))

from ecos_cli.cli import ExitCode, main  # noqa: E402
from test_l4_build import host_tool_is_ready, sdk_toolchain_is_ready  # noqa: E402


@unittest.skipUnless(
    host_tool_is_ready("cmake")
    and host_tool_is_ready("ninja")
    and sdk_toolchain_is_ready()
    and importlib.util.find_spec("kconfiglib") is not None,
    "SDK Python/CMake/Ninja dependencies or the SDK toolchain is not installed",
)
class PhaseBBuildTest(unittest.TestCase):
    def test_json_build_keeps_cmake_output_off_stdout(self):
        with tempfile.TemporaryDirectory() as directory:
            output = StringIO()
            with redirect_stdout(output), redirect_stderr(output):
                created = main([
                    "--sdk", str(SDK_ROOT), "project", "create", "hello",
                    "--name", "json-build", "--path", directory,
                    "--board", "starrysky-l4-c1",
                ])
            self.assertEqual(created, ExitCode.OK, output.getvalue())

            result = subprocess.run(
                [
                    sys.executable, str(SDK_ROOT / "tools/ecos.py"),
                    "--sdk", str(SDK_ROOT), "build", "--project",
                    str(Path(directory) / "json-build"), "--format", "json",
                ],
                capture_output=True, text=True, check=False, timeout=120,
            )
            self.assertEqual(result.returncode, ExitCode.OK, result.stderr)
            payload = json.loads(result.stdout)
            self.assertEqual(payload["command"], "build")
            self.assertEqual(payload["status"], "ok")
            self.assertIn("elf", payload["data"]["outputs"])
            self.assertIn("-- Configuring done", result.stderr)
            output = StringIO()
            with redirect_stdout(output):
                info_result = main([
                    "--sdk", str(SDK_ROOT), "project", "info", "--project",
                    str(Path(directory) / "json-build"), "--format", "json",
                ])
            self.assertEqual(info_result, ExitCode.OK, output.getvalue())
            info = json.loads(output.getvalue())["data"]
            self.assertTrue(info["built"])
            self.assertEqual(info["outputs"]["elf"], payload["data"]["outputs"]["elf"])

    def test_hello_blink_and_donut_on_l3_1_and_l4_c1(self):
        boards = {
            "starrysky-l3-1": ("ELF64", "lp64"),
            "starrysky-l4-c1": ("ELF32", "ilp32e"),
        }
        examples = ("hello", "blink", "display-donut")
        with tempfile.TemporaryDirectory() as directory:
            for board, (elf_class, abi) in boards.items():
                for example in examples:
                    with self.subTest(board=board, example=example):
                        name = f"{board}-{example}"
                        output = StringIO()
                        with redirect_stdout(output), redirect_stderr(output):
                            created = main([
                                "--sdk", str(SDK_ROOT), "project", "create", example,
                                "--name", name, "--path", directory, "--board", board,
                            ])
                            built = (
                                main([
                                    "--sdk", str(SDK_ROOT), "build", "--project",
                                    str(Path(directory) / name),
                                ])
                                if created == ExitCode.OK else None
                            )
                        self.assertEqual(created, ExitCode.OK, output.getvalue())
                        self.assertEqual(built, ExitCode.OK, output.getvalue())
                        root = Path(directory) / name / "build"
                        artifacts = json.loads(
                            (root / "artifacts.json").read_text(encoding="utf-8")
                        )
                        self.assertEqual(artifacts["elf"]["class"], elf_class)
                        self.assertEqual(artifacts["firmware"]["abi"], abi)
                        if example == "display-donut":
                            symbols = artifacts["elf"]["symbols"]
                            for symbol in (
                                "bsp_display_init", "ecos_st7735_init",
                                "ecos_qspi_init",
                            ):
                                self.assertIn(symbol, symbols)


if __name__ == "__main__":
    unittest.main()
