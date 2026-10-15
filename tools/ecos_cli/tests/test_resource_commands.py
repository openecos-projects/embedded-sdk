import json
import sys
import unittest
from contextlib import redirect_stdout
from io import StringIO
from pathlib import Path


SOURCE_ROOT = Path(__file__).parents[1] / "src"
SDK_ROOT = Path(__file__).parents[3]
sys.path.insert(0, str(SOURCE_ROOT))

from ecos_cli.cli import ExitCode, main  # noqa: E402


class ResourceCommandTest(unittest.TestCase):
    def run_json(self, *args: str) -> tuple[int, dict]:
        output = StringIO()
        with redirect_stdout(output):
            result = main(["--sdk", str(SDK_ROOT), *args, "--format", "json"])
        return result, json.loads(output.getvalue())

    def test_lists_validated_boards_targets_and_components(self):
        expectations = (
            ("board", "starrysky-l3-1"),
            ("target", "ysyx-l3-1"),
            ("component", "driver-ps2"),
            ("driver", "driver-ps2"),
            ("device", "device-st7735"),
        )
        for resource, expected_id in expectations:
            with self.subTest(resource=resource):
                result, payload = self.run_json(resource, "list")
                self.assertEqual(result, ExitCode.OK, payload)
                self.assertEqual(payload["command"], f"{resource}.list")
                self.assertEqual(payload["diagnostics"], [])
                items = payload["data"][f"{resource}s"]
                self.assertIn(expected_id, {item["id"] for item in items})

    def test_describes_board_alias_and_target_abi(self):
        result, payload = self.run_json("board", "describe", "l3-1")
        self.assertEqual(result, ExitCode.OK, payload)
        board = payload["data"]
        self.assertEqual(board["id"], "starrysky-l3-1")
        self.assertEqual(board["target"], "ysyx-l3-1")
        self.assertIn("mem", board["profiles"])
        self.assertIn("display", board["resources"])

        result, payload = self.run_json("target", "describe", "ysyx-l3-1")
        self.assertEqual(result, ExitCode.OK, payload)
        self.assertEqual(payload["data"]["cpu"]["abi"], "lp64")
        self.assertIn("ps2", payload["data"]["capabilities"])

    def test_describes_component_dependencies_and_rejects_missing_id(self):
        result, payload = self.run_json("component", "describe", "driver-ps2")
        self.assertEqual(result, ExitCode.OK, payload)
        self.assertIn("hal-ps2", payload["data"]["private_dependencies"])
        self.assertTrue(payload["data"]["include_dirs"])

        result, payload = self.run_json("driver", "describe", "driver-ps2")
        self.assertEqual(result, ExitCode.OK, payload)
        self.assertEqual(payload["data"]["cmake_target"], "ecos::driver::ps2")

        result, payload = self.run_json("device", "describe", "device-st7735")
        self.assertEqual(result, ExitCode.OK, payload)
        self.assertIn("driver-qspi", payload["data"]["dependencies"])

        result, payload = self.run_json("driver", "describe", "device-st7735")
        self.assertEqual(result, ExitCode.CONFIG)

        result, payload = self.run_json("board", "describe", "missing-board")
        self.assertEqual(result, ExitCode.CONFIG)
        self.assertEqual(payload["status"], "error")
        self.assertTrue(payload["diagnostics"][0]["code"])


if __name__ == "__main__":
    unittest.main()
