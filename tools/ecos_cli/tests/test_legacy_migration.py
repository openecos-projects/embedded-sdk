import importlib.util
import sys
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from io import StringIO
from pathlib import Path


SOURCE_ROOT = Path(__file__).parents[1] / "src"
SDK_ROOT = Path(__file__).parents[3]
sys.path.insert(0, str(SOURCE_ROOT))

from ecos_cli.artifacts import inspect_elf  # noqa: E402
from ecos_cli.cli import ExitCode, main  # noqa: E402
from test_l4_build import host_tool_is_ready, sdk_toolchain_is_ready  # noqa: E402


@unittest.skipUnless(
    host_tool_is_ready("cmake")
    and host_tool_is_ready("ninja")
    and sdk_toolchain_is_ready()
    and importlib.util.find_spec("kconfiglib") is not None,
    "SDK Python/CMake/Ninja dependencies or the SDK toolchain is not installed",
)
class LegacyMigrationBuildTest(unittest.TestCase):
    def build_example(
        self, workspace: Path, example: str, board: str, profile: str = ""
    ) -> Path:
        output = StringIO()
        args = [
            "--sdk", str(SDK_ROOT), "project", "create", example,
            "--path", str(workspace), "--board", board,
        ]
        if profile:
            args.extend(["--profile", profile])
        with redirect_stdout(output), redirect_stderr(output):
            created = main(args)
            if created == ExitCode.OK:
                built = main(
                    ["--sdk", str(SDK_ROOT), "build", "--project", str(workspace / example)]
                )
            else:
                built = None
        self.assertEqual(created, ExitCode.OK, output.getvalue())
        self.assertEqual(built, ExitCode.OK, output.getvalue())
        return workspace / example

    def test_c2_psram_profile_preserves_gpio_state_and_links_data_in_psram(self):
        with tempfile.TemporaryDirectory() as directory:
            project = self.build_example(
                Path(directory), "spi-master-st7789", "starrysky-c2", "psram-data"
            )
            config = (project / ".ecos/generated/sdkconfig.h").read_text(
                encoding="utf-8"
            )
            self.assertIn("#define CONFIG_LINK_RAM_REGION_PSRAM 1", config)

            firmware = project / "build/retrosoc_fw.elf"
            elf = inspect_elf(firmware)
            sections = {item["name"]: item for item in elf["sections"]}
            self.assertEqual(elf["class"], "ELF32")
            self.assertEqual(elf["entry"], 0)
            self.assertLess(sections[".text"]["address"], 0x01000000)
            self.assertEqual(sections[".data"]["address"], 0x40000000)
            self.assertGreaterEqual(sections[".bss"]["address"], 0x40000000)
            self.assertEqual(elf["symbols"]["_stack_pointer"], 0x407FFFF0)

            output = elf["symbols"]["simple_output"]
            data = sections[".data"]
            output_offset = data["offset"] + output - data["address"]
            self.assertEqual(
                firmware.read_bytes()[output_offset:output_offset + 4],
                b"\x00\x80\x00\x00",
            )

    def test_l3_1_ps2_and_wdg_examples_link_public_stack(self):
        expected = {
            "ps2-keycodes": ("ecos_ps2_read", "hal_ps2_read"),
            "wdg-reset": ("ecos_wdg_feed", "hal_wdg_feed"),
        }
        with tempfile.TemporaryDirectory() as directory:
            for example, symbols in expected.items():
                with self.subTest(example=example):
                    project = self.build_example(
                        Path(directory), example, "starrysky-l3-1"
                    )
                    elf = inspect_elf(project / "build/retrosoc_fw.elf")
                    self.assertEqual(elf["class"], "ELF64")
                    for symbol in symbols:
                        self.assertIn(symbol, elf["symbols"])


if __name__ == "__main__":
    unittest.main()
