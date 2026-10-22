"""Board selection for the CI firmware build matrix."""

import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from tools import ci_host_smoke


class CiBoardSelectionTest(unittest.TestCase):
    def test_builds_only_selected_boards_and_compatible_examples(self):
        boards = ["starrysky-l4-c1", "starrysky-t1-pico", "starrysky-c2"]
        builds = []

        def command(cli, sdk, args, environment):
            if args == ["board", "list"]:
                return {"boards": [{"id": board} for board in [*boards, "starrysky-l3"]]}
            if args == ["project", "list"]:
                return {"examples": [
                    {"name": "hello", "supported_boards": [*boards, "starrysky-l3"]},
                    {"name": "c2-only", "supported_boards": ["starrysky-c2"]},
                ]}
            if args[:2] == ["board", "describe"]:
                return {"cpu": {"march": "rv32im", "abi": "ilp32"}}
            if args[:2] == ["project", "create"]:
                project = Path(args[args.index("--path") + 1]) / args[args.index("--name") + 1]
                board = args[args.index("--board") + 1]
                builds.append((board, args[2]))
                (project / "build").mkdir(parents=True)
                (project / "build/artifacts.json").write_text(json.dumps({
                    "project": {"board": board},
                    "firmware": {"isa": "rv32im", "abi": "ilp32"},
                    "elf": {"class": "ELF32"},
                }))
            return {}

        with tempfile.TemporaryDirectory() as directory, patch.object(
            ci_host_smoke, "_json_command", side_effect=command
        ), patch.object(ci_host_smoke, "_build_record", return_value={"built": True}):
            root = Path(directory)
            records = ci_host_smoke._build_matrix(
                root / "ecos", root, {}, root, [*boards, boards[0]]
            )
        self.assertEqual(len(records), 4)
        self.assertCountEqual(builds, [
            ("starrysky-l4-c1", "hello"),
            ("starrysky-t1-pico", "hello"),
            ("starrysky-c2", "hello"),
            ("starrysky-c2", "c2-only"),
        ])

    def test_rejects_unknown_board_before_building(self):
        with patch.object(ci_host_smoke, "_json_command", side_effect=[
            {"boards": [{"id": "starrysky-c2"}]}, {"examples": []},
        ]):
            with self.assertRaisesRegex(RuntimeError, "unknown Board.*missing"):
                ci_host_smoke._build_matrix(
                    Path("ecos"), Path("."), {}, Path("."), ["missing"]
                )


if __name__ == "__main__":
    unittest.main()
