import json
import sys
import tempfile
import unittest
from contextlib import redirect_stdout
from io import StringIO
from pathlib import Path


SOURCE_ROOT = Path(__file__).parents[1] / "src"
SDK_ROOT = Path(__file__).parents[3]
sys.path.insert(0, str(SOURCE_ROOT))

from ecos_cli import schema_contract  # noqa: E402
from ecos_cli.cli import ExitCode, main  # noqa: E402
from ecos_cli.project_model import ManifestValidationError, _read_manifest  # noqa: E402
from ecos_cli.sdk_manifest import load_manifest, release_paths  # noqa: E402


class SchemaContractTest(unittest.TestCase):
    def run_json(self, *args: str) -> tuple[int, dict]:
        output = StringIO()
        with redirect_stdout(output):
            result = main(["schema", *args, "--format", "json"])
        return result, json.loads(output.getvalue())

    def test_packaged_contracts_validate_all_current_manifests(self):
        roots = (
            ("board", SDK_ROOT / "boards", "ecos-board.yml"),
            ("target", SDK_ROOT / "components/soc", "ecos-soc.yml"),
            ("component", SDK_ROOT, "ecos-component.yml"),
            ("example", SDK_ROOT / "examples", "ecos-example.yml"),
        )
        for kind, root, filename in roots:
            paths = list(root.rglob(filename))
            self.assertTrue(paths, kind)
            for path in paths:
                with self.subTest(kind=kind, path=path):
                    self.assertTrue(schema_contract.validate_file(kind, path)["valid"])
        self.assertTrue(
            schema_contract.validate_file("sdk", SDK_ROOT / "tools/sdk-manifest.json")["valid"]
        )

    def test_sdk_example_root_and_release_selection_match(self):
        manifest = load_manifest(SDK_ROOT)
        self.assertEqual(manifest["layout"]["boards"], "boards")
        self.assertEqual(manifest["layout"]["examples"], "examples")
        self.assertIn("examples", release_paths(manifest))
        self.assertIn("boards", release_paths(manifest))
        self.assertTrue((SDK_ROOT / "boards").is_dir())
        self.assertFalse((SDK_ROOT / "board").exists())
        self.assertFalse((SDK_ROOT / "example").exists())

    def test_cli_lists_describes_and_validates_offline(self):
        result, payload = self.run_json("list")
        self.assertEqual(result, ExitCode.OK)
        self.assertEqual(
            {item["kind"] for item in payload["data"]["schemas"]},
            {"board", "target", "component", "example", "sdk"},
        )

        result, payload = self.run_json("describe", "board")
        self.assertEqual(result, ExitCode.OK)
        self.assertEqual(payload["data"]["schema"]["properties"]["schema"]["const"], 2)

        result, payload = self.run_json("describe", "sdk")
        self.assertEqual(result, ExitCode.OK)
        self.assertEqual(
            payload["data"]["schema"]["properties"]["schema_version"]["const"], 1
        )

        result, payload = self.run_json(
            "validate", "sdk", str(SDK_ROOT / "tools/sdk-manifest.json")
        )
        self.assertEqual(result, ExitCode.OK)
        self.assertTrue(payload["data"]["valid"])

        path = SDK_ROOT / "boards/StarrySkyL3_1/ecos-board.yml"
        result, payload = self.run_json("validate", "board", str(path))
        self.assertEqual(result, ExitCode.OK)
        self.assertEqual(payload["data"]["path"], str(path.resolve()))
        self.assertEqual(payload["diagnostics"], [])

    def test_invalid_manifest_reports_field_actual_and_expected(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "invalid example.yml"
            path.write_text(
                "schema: 1\nname: demo\nsources: main.c\n", encoding="utf-8"
            )
            result, payload = self.run_json("validate", "example", str(path))
            self.assertEqual(result, ExitCode.CONFIG)
            self.assertEqual(payload["status"], "error")
            item = payload["diagnostics"][0]
            self.assertEqual(item["code"], "ECOS_SCHEMA_INVALID")
            self.assertEqual(item["field"], "/sources")
            self.assertEqual(item["actual"], "main.c")
            self.assertEqual(item["expected"], "array")

            with self.assertRaisesRegex(ManifestValidationError, "/sources"):
                _read_manifest(path, "Example")

    def test_sdk_schema_rejects_invalid_compatibility_type(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "sdk-manifest.json"
            manifest = json.loads(
                (SDK_ROOT / "tools/sdk-manifest.json").read_text(encoding="utf-8")
            )
            manifest["cli_compatibility"]["major"] = True
            path.write_text(json.dumps(manifest), encoding="utf-8")

            result, payload = self.run_json("validate", "sdk", str(path))
            self.assertEqual(result, ExitCode.CONFIG)
            item = payload["diagnostics"][0]
            self.assertEqual(item["code"], "ECOS_SCHEMA_INVALID")
            self.assertEqual(item["field"], "/cli_compatibility/major")
            self.assertIs(item["actual"], True)
            self.assertEqual(item["expected"], "integer")


if __name__ == "__main__":
    unittest.main()
