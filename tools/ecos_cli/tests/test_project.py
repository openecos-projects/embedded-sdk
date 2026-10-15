import json
import os
import sys
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from io import StringIO
from pathlib import Path
from unittest import mock


SOURCE_ROOT = Path(__file__).parents[1] / "src"
sys.path.insert(0, str(SOURCE_ROOT))

from ecos_cli import build, dependencies  # noqa: E402
from ecos_cli.cli import ExitCode, main  # noqa: E402
from ecos_cli.sdk_registry import SdkRegistry  # noqa: E402


def create_sdk(root: Path) -> Path:
    layout = {
        "boards": "boards",
        "components": "components",
        "drivers": "drivers",
        "devices": "devices",
        "docs": "docs",
        "examples": "examples",
        "hal": "hal",
        "templates": "templates",
        "cli": "tools/ecos_cli/src",
    }
    for relative in layout.values():
        (root / relative).mkdir(parents=True, exist_ok=True)
    manifest = {
        "schema_version": 1,
        "sdk_id": "ecos-embedded-sdk",
        "sdk_version": "3.0.0",
        "channel": "development",
        "cli_compatibility": {"major": 3, "schema_version": "1.0"},
        "layout": layout,
        "toolchain": {
            "id": "xpack-riscv-none-elf-gcc",
            "release": "15.2.0-1",
        },
    }
    manifest_path = root / "tools" / "sdk-manifest.json"
    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
    hello_world = root / "examples" / "hello_world"
    hello_world.mkdir()
    (hello_world / "main.c").write_text(
        '#include "main.h"\n\nvoid main(void) {}\n', encoding="utf-8"
    )
    (hello_world / "ecos-example.yml").write_text(
        "schema: 1\nname: hello_world\nsources:\n  - main.c\n",
        encoding="utf-8",
    )
    (hello_world / "retrosoc_fw.bin").write_bytes(b"firmware")
    nested = root / "examples" / "get-started" / "hello"
    nested.mkdir(parents=True)
    (nested / "main.c").write_text("void main(void) {}\n", encoding="utf-8")
    (nested / "ecos-example.yml").write_text(
        "schema: 1\nname: hello\nsources:\n  - main.c\n",
        encoding="utf-8",
    )
    for target in ("ysyx-l3", "ysyx-2512"):
        target_root = root / "components" / "soc" / target
        target_root.mkdir(parents=True)
        (target_root / "ecos-soc.yml").write_text(
            f"schema: 1\nid: {target}\narch: riscv\n"
            "cpu:\n  march: rv32e\n  abi: ilp32e\n"
            "build:\n  cmake: CMakeLists.txt\n"
            "  toolchain_file: toolchain.cmake\n",
            encoding="utf-8",
        )
        (target_root / "CMakeLists.txt").write_text(
            "cmake_minimum_required(VERSION 3.20)\nproject(test C)\n",
            encoding="utf-8",
        )
        (target_root / "toolchain.cmake").write_text("", encoding="utf-8")
    l3_board = root / "boards" / "StarrySkyL3_1" / "ecos-board.yml"
    l3_board.parent.mkdir()
    l3_board.write_text(
        "schema: 2\n"
        "id: starrysky-l3-1\n"
        "aliases:\n"
        "  - l3_1\n"
        "target: ysyx-l3\n",
        encoding="utf-8",
    )
    l4_board = root / "boards" / "StarrySkyL4" / "ecos-board.yml"
    l4_board.parent.mkdir()
    l4_board.write_text(
        "schema: 2\n"
        "id: starrysky-l4\n"
        "aliases:\n"
        "  - l4\n"
        "target: ysyx-2512\n",
        encoding="utf-8",
    )
    return root


class ProjectCreateTest(unittest.TestCase):
    def run_json(self, sdk: Path, *arguments: str) -> tuple[int, dict]:
        output = StringIO()
        with redirect_stdout(output):
            result = main(
                ["--sdk", str(sdk), "project", "create", *arguments, "--format", "json"]
            )
        return result, json.loads(output.getvalue())

    def run_list_json(self, sdk: Path) -> tuple[int, dict]:
        output = StringIO()
        with redirect_stdout(output):
            result = main(
                [
                    "--sdk",
                    str(sdk),
                    "project",
                    "list",
                    "--format",
                    "json",
                ]
            )
        return result, json.loads(output.getvalue())

    def test_lists_examples_and_compatible_boards(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            pwm = sdk / "examples" / "peripherals" / "pwm" / "basic"
            pwm.mkdir(parents=True)
            (pwm / "main.c").write_text("void main(void) {}\n", encoding="utf-8")
            (pwm / "ecos-example.yml").write_text(
                "schema: 1\n"
                "name: pwm-basic\n"
                "sources:\n  - main.c\n"
                "requires:\n  - pwm-output\n",
                encoding="utf-8",
            )
            l4_manifest = sdk / "boards" / "StarrySkyL4" / "ecos-board.yml"
            l4_manifest.write_text(
                l4_manifest.read_text(encoding="utf-8")
                + "resources:\n  pwm-output:\n    driver: driver-pwm\n",
                encoding="utf-8",
            )
            legacy_board = sdk / "boards" / "Legacy" / "ecos-board.yml"
            legacy_board.parent.mkdir()
            legacy_board.write_text(
                "schema: 1\nid: legacy\nname: Legacy\n", encoding="utf-8"
            )
            pending_board = sdk / "boards" / "Pending" / "ecos-board.yml"
            pending_board.parent.mkdir()
            pending_board.write_text(
                "schema: 2\nid: pending\ntarget: pending-soc\n",
                encoding="utf-8",
            )

            result, payload = self.run_list_json(sdk)

            self.assertEqual(result, ExitCode.OK)
            self.assertEqual(payload["command"], "project.list")
            data = payload["data"]
            self.assertEqual(data["sdk"]["id"], "ecos-embedded-sdk")
            self.assertEqual(
                [board["id"] for board in data["boards"]],
                ["starrysky-l3-1", "starrysky-l4"],
            )
            self.assertEqual(
                [example["name"] for example in data["examples"]],
                ["hello", "hello_world", "pwm-basic"],
            )
            by_name = {item["name"]: item for item in data["examples"]}
            self.assertEqual(by_name["hello"]["path"], "get-started/hello")
            self.assertEqual(
                by_name["hello"]["supported_boards"],
                ["starrysky-l3-1", "starrysky-l4"],
            )
            self.assertEqual(
                by_name["pwm-basic"]["supported_boards"], ["starrysky-l4"]
            )
            self.assertEqual(by_name["pwm-basic"]["requires"], ["pwm-output"])

    def test_list_rejects_invalid_current_board_schema(self):
        with tempfile.TemporaryDirectory() as directory:
            sdk = create_sdk(Path(directory) / "sdk")
            broken = sdk / "boards" / "Broken"
            broken.mkdir()
            (broken / "ecos-board.yml").write_text(
                "schema: 2\nid: broken\n", encoding="utf-8"
            )

            result, payload = self.run_list_json(sdk)
            self.assertEqual(result, ExitCode.CONFIG)
            self.assertIn("'target' is a required property", payload["diagnostics"][0]["message"])

    def test_board_debug_configuration_overrides_target_default(self):
        with tempfile.TemporaryDirectory() as directory:
            sdk = create_sdk(Path(directory) / "sdk")
            target = sdk / "components/soc/ysyx-l3/ecos-soc.yml"
            target.write_text(
                target.read_text(encoding="utf-8") +
                "debug:\n  adapter: cppdbg\n  server_address: localhost:3333\n",
                encoding="utf-8",
            )
            board = sdk / "boards/StarrySkyL3_1/ecos-board.yml"

            def describe(kind: str, name: str) -> dict:
                output = StringIO()
                with redirect_stdout(output):
                    result = main([
                        "--sdk", str(sdk), kind, "describe", name,
                        "--format", "json",
                    ])
                self.assertEqual(result, ExitCode.OK, output.getvalue())
                return json.loads(output.getvalue())["data"]

            self.assertEqual(
                describe("target", "ysyx-l3")["debug"]["server_address"],
                "localhost:3333",
            )
            self.assertEqual(
                describe("board", "starrysky-l3-1")["debug"]["server_address"],
                "localhost:3333",
            )
            board.write_text(
                board.read_text(encoding="utf-8") +
                "debug:\n  adapter: cppdbg\n  server_address: localhost:4444\n",
                encoding="utf-8",
            )
            self.assertEqual(
                describe("board", "starrysky-l3-1")["debug"]["server_address"],
                "localhost:4444",
            )

    def test_user_bsp_is_discovered_and_validated_outside_sdk(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            user_bsp = root / "user-boards"
            board_root = user_bsp / "CustomL4"
            board_root.mkdir(parents=True)
            (board_root / "ecos-board.yml").write_text(
                "schema: 2\nid: custom-l4\ntarget: ysyx-2512\n"
                "build:\n  components:\n    - custom-bsp\n",
                encoding="utf-8",
            )
            (board_root / "ecos-component.yml").write_text(
                "schema: 1\nid: custom-bsp\n"
                "cmake_target: ecos::bsp::custom_l4\n"
                "sources:\n  - bsp.c\n",
                encoding="utf-8",
            )
            (board_root / "bsp.c").write_text("void custom_bsp(void) {}\n", encoding="utf-8")
            workspace = root / "workspace"
            workspace.mkdir()

            with mock.patch.dict(os.environ, {"ECOS_USER_BSP_ROOT": str(user_bsp)}):
                result, listing = self.run_list_json(sdk)
                self.assertEqual(result, ExitCode.OK)
                self.assertIn(
                    "custom-l4", [board["id"] for board in listing["data"]["boards"]]
                )
                result, created = self.run_json(
                    sdk, "hello", "--path", str(workspace), "--board", "custom-l4"
                )
                self.assertEqual(result, ExitCode.OK, created)
                output = StringIO()
                with redirect_stdout(output):
                    validated = main(
                        [
                            "--sdk", str(sdk), "validate", "--project",
                            str(workspace / "hello"), "--format", "json",
                        ]
                    )
                payload = json.loads(output.getvalue())
                self.assertEqual(validated, ExitCode.OK, payload)
                self.assertIn("custom-bsp", payload["data"]["components"])

                (board_root / "bsp.c").write_text(
                    "void custom_bsp(void) { int changed = 1; (void)changed; }\n",
                    encoding="utf-8",
                )
                updated_output = StringIO()
                with redirect_stdout(updated_output):
                    updated = main(
                        [
                            "--sdk", str(sdk), "validate", "--project",
                            str(workspace / "hello"), "--format", "json",
                        ]
                    )
                updated_payload = json.loads(updated_output.getvalue())

            self.assertEqual(updated, ExitCode.OK, updated_payload)
            self.assertNotEqual(
                payload["data"]["source_fingerprint"],
                updated_payload["data"]["source_fingerprint"],
            )

    def test_project_list_text_prints_compatibility_table(self):
        with tempfile.TemporaryDirectory() as directory:
            sdk = create_sdk(Path(directory) / "sdk")
            output = StringIO()

            with redirect_stderr(output):
                result = main(["--sdk", str(sdk), "project", "list"])

            text = output.getvalue()
            self.assertEqual(result, ExitCode.OK)
            self.assertIn("SDK Examples: ecos-embedded-sdk 3.0.0", text)
            self.assertIn("EXAMPLE", text)
            self.assertIn("BOARDS", text)
            self.assertIn("hello_world", text)
            self.assertIn("starrysky-l3-1, starrysky-l4", text)

    def test_project_list_rejects_ambiguous_example_names(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            duplicate = sdk / "examples" / "other" / "hello"
            duplicate.mkdir(parents=True)
            (duplicate / "ecos-example.yml").write_text(
                "schema: 1\nname: hello\nsources:\n  - main.c\n",
                encoding="utf-8",
            )
            (duplicate / "main.c").write_text(
                "void main(void) {}\n", encoding="utf-8"
            )

            result, payload = self.run_list_json(sdk)

            self.assertEqual(result, ExitCode.CONFIG)
            self.assertEqual(
                payload["diagnostics"][0]["code"],
                "ECOS_PROJECT_EXAMPLE_AMBIGUOUS",
            )

    def test_creates_hello_world_with_metadata(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            workspace = root / "workspace"
            workspace.mkdir()

            result, payload = self.run_json(
                sdk, "hello_world", "--path", str(workspace)
            )

            project = workspace / "hello_world"
            self.assertEqual(result, ExitCode.OK)
            self.assertEqual(payload["command"], "project.create")
            self.assertEqual(payload["data"]["path"], str(project))
            self.assertEqual(
                (project / "main.c").read_text(encoding="utf-8"),
                '#include "main.h"\n\nvoid main(void) {}\n',
            )
            self.assertEqual((project / "retrosoc_fw.bin").read_bytes(), b"firmware")
            metadata = (project / ".ecos/project.yml").read_text(encoding="utf-8")
            self.assertIn("schema: 2", metadata)
            self.assertIn('name: "hello_world"', metadata)
            self.assertIn('example: "hello_world"', metadata)
            self.assertIn('id: "ecos-embedded-sdk"', metadata)
            self.assertNotIn(str(sdk.resolve()), metadata)

    def test_custom_name_board_profile_and_nested_example(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            workspace = root / "workspace"
            workspace.mkdir()

            result, payload = self.run_json(
                sdk,
                "hello",
                "--name",
                "demo-app",
                "--path",
                str(workspace),
                "--board",
                "starrysky-l3-1",
                "--profile",
                "debug",
            )

            self.assertEqual(result, ExitCode.OK)
            self.assertEqual(payload["data"]["name"], "demo-app")
            metadata = (workspace / "demo-app/.ecos/project.yml").read_text(
                encoding="utf-8"
            )
            self.assertIn('example: "hello"', metadata)
            self.assertIn('board: "starrysky-l3-1"', metadata)
            self.assertIn('target: "ysyx-l3"', metadata)
            self.assertIn('profile: "debug"', metadata)

    def test_create_rejects_board_and_target_together(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            workspace = root / "workspace"
            workspace.mkdir()

            result, payload = self.run_json(
                sdk,
                "hello_world",
                "--path",
                str(workspace),
                "--board",
                "starrysky-l4",
                "--target",
                "ysyx-2512",
            )

            self.assertEqual(result, ExitCode.USAGE)
            self.assertEqual(
                payload["diagnostics"][0]["code"],
                "ECOS_USAGE_INVALID_ARGUMENTS",
            )
            self.assertFalse((workspace / "hello_world").exists())

    def test_unsupported_boards_are_filtered_and_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            workspace = root / "workspace"
            workspace.mkdir()
            manifest = sdk / "examples" / "hello_world" / "ecos-example.yml"
            manifest.write_text(
                manifest.read_text(encoding="utf-8")
                + "unsupported_boards:\n  - starrysky-l4\n",
                encoding="utf-8",
            )

            result, payload = self.run_list_json(sdk)
            self.assertEqual(result, ExitCode.OK)
            by_name = {item["name"]: item for item in payload["data"]["examples"]}
            self.assertEqual(
                by_name["hello_world"]["supported_boards"], ["starrysky-l3-1"]
            )

            result, payload = self.run_json(
                sdk,
                "hello_world",
                "--path",
                str(workspace),
                "--board",
                "l4",
            )
            self.assertEqual(result, ExitCode.CONFIG)
            self.assertEqual(
                payload["diagnostics"][0]["code"],
                "ECOS_PROJECT_CAPABILITY_MISMATCH",
            )
            self.assertFalse((workspace / "hello_world").exists())

            result, payload = self.run_json(
                sdk,
                "hello_world",
                "--path",
                str(workspace),
                "--board",
                "l4",
                "--dry-run",
            )
            self.assertEqual(result, ExitCode.CONFIG)
            self.assertEqual(
                payload["diagnostics"][0]["code"],
                "ECOS_PROJECT_CAPABILITY_MISMATCH",
            )
            self.assertFalse((workspace / "hello_world").exists())

            result, payload = self.run_json(
                sdk,
                "hello_world",
                "--path",
                str(workspace),
                "--board",
                "starrysky-l3-1",
            )
            self.assertEqual(result, ExitCode.OK)
            self.assertTrue((workspace / "hello_world").exists())

    def test_project_info_and_profile_selection(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            board_manifest = sdk / "boards/StarrySkyL3_1/ecos-board.yml"
            board_manifest.write_text(
                board_manifest.read_text(encoding="utf-8")
                + "build:\n  default_profile: memory\n  profiles:\n"
                + "    - memory\n    - psram\n",
                encoding="utf-8",
            )
            workspace = root / "workspace"
            workspace.mkdir()
            created, payload = self.run_json(
                sdk, "hello_world", "--path", str(workspace),
                "--board", "starrysky-l3-1",
            )
            self.assertEqual(created, ExitCode.OK, payload)
            project_root = workspace / "hello_world"
            metadata_path = project_root / ".ecos/project.yml"

            def command(*args: str) -> tuple[int, dict]:
                output = StringIO()
                with redirect_stdout(output):
                    result = main(
                        ["--sdk", str(sdk), "project", *args,
                         "--project", str(project_root), "--format", "json"]
                    )
                return result, json.loads(output.getvalue())

            result, payload = command("info")
            self.assertEqual(result, ExitCode.OK, payload)
            self.assertEqual(payload["data"]["profile"], "memory")
            self.assertFalse(payload["data"]["configured"])

            original_metadata = metadata_path.read_bytes()
            result, payload = command("set-profile", "psram", "--dry-run")
            self.assertEqual(result, ExitCode.OK, payload)
            self.assertTrue(payload["data"]["dry_run"])
            self.assertTrue(payload["data"]["changed"])
            self.assertEqual(metadata_path.read_bytes(), original_metadata)

            result, payload = command("set-profile", "psram")
            self.assertEqual(result, ExitCode.OK, payload)
            self.assertTrue(payload["data"]["changed"])
            metadata = metadata_path.read_bytes()
            self.assertIn(b'profile: "psram"', metadata)

            result, payload = command("set-profile", "psram")
            self.assertEqual(result, ExitCode.OK, payload)
            self.assertFalse(payload["data"]["changed"])
            self.assertEqual(metadata_path.read_bytes(), metadata)

            result, payload = command("set-profile", "unknown")
            self.assertEqual(result, ExitCode.CONFIG)
            self.assertEqual(metadata_path.read_bytes(), metadata)

    def test_set_board_and_set_target_replace_each_other(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            workspace = root / "workspace"
            workspace.mkdir()
            create_result, _ = self.run_json(
                sdk,
                "hello_world",
                "--path",
                str(workspace),
                "--target",
                "ysyx-l3",
            )
            self.assertEqual(create_result, ExitCode.OK)
            project = workspace / "hello_world"
            metadata_path = project / ".ecos/project.yml"
            original_metadata = metadata_path.read_bytes()

            for selection, value in (("set-board", "l4"), ("set-target", "ysyx-2512")):
                with self.subTest(selection=selection):
                    output = StringIO()
                    with redirect_stdout(output):
                        preview = main([
                            "--sdk", str(sdk), "project", selection, value,
                            "--project", str(project), "--dry-run", "--format", "json",
                        ])
                    self.assertEqual(preview, ExitCode.OK, output.getvalue())
                    self.assertTrue(json.loads(output.getvalue())["data"]["dry_run"])
                    self.assertEqual(metadata_path.read_bytes(), original_metadata)

            output = StringIO()
            with redirect_stdout(output):
                set_board_result = main(
                    [
                        "--sdk",
                        str(sdk),
                        "project",
                        "set-board",
                        "l4",
                        "--project",
                        str(project),
                        "--format",
                        "json",
                    ]
                )
            set_board_payload = json.loads(output.getvalue())
            self.assertEqual(set_board_result, ExitCode.OK)
            self.assertEqual(set_board_payload["data"]["board"], "starrysky-l4")
            self.assertEqual(set_board_payload["data"]["target"], "ysyx-2512")
            metadata = (project / ".ecos/project.yml").read_text(encoding="utf-8")
            self.assertIn('board: "starrysky-l4"', metadata)
            self.assertIn('target: "ysyx-2512"', metadata)

            output = StringIO()
            with redirect_stdout(output):
                set_target_result = main(
                    [
                        "--sdk",
                        str(sdk),
                        "project",
                        "set-target",
                        "ysyx-l3",
                        "--project",
                        str(project),
                        "--format",
                        "json",
                    ]
                )
            set_target_payload = json.loads(output.getvalue())
            self.assertEqual(set_target_result, ExitCode.OK)
            self.assertIsNone(set_target_payload["data"]["board"])
            self.assertEqual(set_target_payload["data"]["target"], "ysyx-l3")
            metadata = (project / ".ecos/project.yml").read_text(encoding="utf-8")
            self.assertIn("board: null", metadata)
            self.assertIn('target: "ysyx-l3"', metadata)

    def test_failed_target_selection_keeps_project_metadata(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            workspace = root / "workspace"
            workspace.mkdir()
            create_result, _ = self.run_json(
                sdk,
                "hello_world",
                "--path",
                str(workspace),
                "--board",
                "starrysky-l4",
            )
            self.assertEqual(create_result, ExitCode.OK)
            project = workspace / "hello_world"
            metadata_path = project / ".ecos/project.yml"
            original = metadata_path.read_text(encoding="utf-8")

            output = StringIO()
            with redirect_stdout(output):
                result = main(
                    [
                        "--sdk",
                        str(sdk),
                        "project",
                        "set-target",
                        "missing-soc",
                        "--project",
                        str(project),
                        "--format",
                        "json",
                    ]
                )

            payload = json.loads(output.getvalue())
            self.assertEqual(result, ExitCode.CONFIG)
            self.assertEqual(
                payload["diagnostics"][0]["code"],
                "ECOS_PROJECT_TARGET_NOT_FOUND",
            )
            self.assertEqual(metadata_path.read_text(encoding="utf-8"), original)

    def test_build_dispatches_board_to_mapped_soc(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            workspace = root / "workspace"
            workspace.mkdir()
            create_result, _ = self.run_json(
                sdk,
                "hello_world",
                "--path",
                str(workspace),
                "--board",
                "starrysky-l4",
            )
            self.assertEqual(create_result, ExitCode.OK)
            project = workspace / "hello_world"

            def fake_cmake(command, *, cwd, env, check):
                build_dir = Path(cwd) / "build"
                build_dir.mkdir(exist_ok=True)
                for suffix in ("elf", "bin", "txt", "hex", "map", "size"):
                    (build_dir / f"retrosoc_fw.{suffix}").write_bytes(b"output")
                (build_dir / "compile_commands.json").write_text("[]", encoding="utf-8")
                return mock.Mock(returncode=0)

            active_toolchain = root / "toolchain"
            (active_toolchain / "bin").mkdir(parents=True)
            compiler = (
                "riscv-none-elf-gcc.exe"
                if os.name == "nt"
                else "riscv-none-elf-gcc"
            )
            (active_toolchain / "bin" / compiler).write_bytes(b"")
            output = StringIO()
            with mock.patch(
                "ecos_cli.build.shutil.which",
                side_effect=lambda name: name if name in {"cmake", "ninja"} else None,
            ), mock.patch(
                "ecos_cli.build.toolchain.default_prefix",
                return_value=active_toolchain,
            ), mock.patch(
                "ecos_cli.build.toolchain.installation_status",
                return_value={"state": "installed", "active_root": str(active_toolchain)},
            ) as installation_status, mock.patch(
                "ecos_cli.build.subprocess.run", side_effect=fake_cmake
            ) as run, mock.patch(
                "ecos_cli.build.artifacts.create_manifest",
                return_value={
                    "files": {},
                    "manifest": str(project / "build/artifacts.json"),
                },
            ), redirect_stdout(output), redirect_stderr(output):
                result = main(
                    ["--sdk", str(sdk), "build", "--project", str(project)]
                )

            self.assertEqual(result, ExitCode.OK)
            command = run.call_args_list[0].args[0]
            self.assertEqual(command[1], "-S")
            self.assertEqual(
                Path(command[2]),
                sdk / "components/soc/ysyx-2512",
            )
            self.assertIn(
                "-DECOS_PROJECT_CONFIG="
                f"{(project / '.ecos/generated/resolved-project.cmake').as_posix()}",
                command,
            )
            self.assertEqual(installation_status.call_args.args[1], active_toolchain)

    def test_cmake_path_uses_forward_slashes_for_windows_paths(self):
        self.assertEqual(
            build._cmake_path(r"C:\Users\developer\project"),
            "C:/Users/developer/project",
        )

    def test_build_prefers_host_managed_cmake_and_ninja(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            workspace = root / "workspace"
            workspace.mkdir()
            create_result, _ = self.run_json(
                sdk,
                "hello_world",
                "--path",
                str(workspace),
                "--board",
                "starrysky-l4",
            )
            self.assertEqual(create_result, ExitCode.OK)
            project = workspace / "hello_world"

            host_prefix = root / "host"
            host_root = host_prefix / dependencies.HOST_DEPENDENCY_RELATIVE_ROOT
            cmake = host_root / "cmake/data/bin/cmake"
            ninja = host_root / "bin/ninja"
            cmake.parent.mkdir(parents=True)
            ninja.parent.mkdir(parents=True)
            cmake.write_bytes(b"")
            ninja.write_bytes(b"")

            def fake_cmake(command, *, cwd, env, check):
                build_dir = Path(cwd) / "build"
                build_dir.mkdir(exist_ok=True)
                for suffix in ("elf", "bin", "txt", "hex", "map", "size"):
                    (build_dir / f"retrosoc_fw.{suffix}").write_bytes(b"output")
                (build_dir / "compile_commands.json").write_text("[]", encoding="utf-8")
                return mock.Mock(returncode=0)

            active_toolchain = root / "toolchain"
            (active_toolchain / "bin").mkdir(parents=True)
            compiler = (
                "riscv-none-elf-gcc.exe"
                if os.name == "nt"
                else "riscv-none-elf-gcc"
            )
            (active_toolchain / "bin" / compiler).write_bytes(b"")
            with mock.patch(
                "ecos_cli.build.shutil.which", return_value=None
            ), mock.patch(
                "ecos_cli.build.dependencies.default_host_prefix",
                return_value=host_prefix,
            ), mock.patch(
                "ecos_cli.build.toolchain.default_prefix",
                return_value=active_toolchain,
            ), mock.patch(
                "ecos_cli.build.toolchain.installation_status",
                return_value={"state": "installed", "active_root": str(active_toolchain)},
            ), mock.patch(
                "ecos_cli.build.subprocess.run", side_effect=fake_cmake
            ) as run, mock.patch(
                "ecos_cli.build.artifacts.create_manifest",
                return_value={
                    "files": {},
                    "manifest": str(project / "build/artifacts.json"),
                },
            ), redirect_stdout(StringIO()), redirect_stderr(StringIO()):
                result = main(
                    ["--sdk", str(sdk), "build", "--project", str(project)]
                )

            self.assertEqual(result, ExitCode.OK)
            configure = run.call_args_list[0].args[0]
            self.assertEqual(configure[0], str(cmake.resolve()))
            self.assertIn(
                f"-DCMAKE_MAKE_PROGRAM={ninja.resolve().as_posix()}", configure
            )

    def test_clean_does_not_require_host_build_tools(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            workspace = root / "workspace"
            workspace.mkdir()
            create_result, _ = self.run_json(
                sdk,
                "hello_world",
                "--path",
                str(workspace),
                "--board",
                "starrysky-l4",
            )
            self.assertEqual(create_result, ExitCode.OK)
            project = workspace / "hello_world"
            build_dir = project / "build"
            build_dir.mkdir()
            (build_dir / "stale").write_text("old", encoding="utf-8")
            (build_dir / "CMakeCache.txt").write_text(
                f"CMAKE_HOME_DIRECTORY:INTERNAL={project}\n"
                f"ECOS_PROJECT_CONFIG:UNINITIALIZED="
                f"{project / '.ecos/generated/resolved-project.cmake'}\n",
                encoding="utf-8",
            )

            output = StringIO()
            with mock.patch(
                "ecos_cli.build.shutil.which", return_value=None
            ), redirect_stdout(output), redirect_stderr(output):
                result = main(
                    ["--sdk", str(sdk), "build", "--project", str(project), "--clean"]
                )

            self.assertEqual(result, ExitCode.OK, output.getvalue())
            self.assertFalse(build_dir.exists())

    def test_build_rejects_board_target_mismatch(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            workspace = root / "workspace"
            workspace.mkdir()
            create_result, _ = self.run_json(
                sdk,
                "hello_world",
                "--path",
                str(workspace),
                "--board",
                "starrysky-l4",
            )
            self.assertEqual(create_result, ExitCode.OK)
            project = workspace / "hello_world"
            metadata_path = project / ".ecos/project.yml"
            metadata_path.write_text(
                metadata_path.read_text(encoding="utf-8").replace(
                    'target: "ysyx-2512"', 'target: "ysyx-l3"'
                ),
                encoding="utf-8",
            )

            output = StringIO()
            with mock.patch("ecos_cli.build.subprocess.run") as run, redirect_stdout(
                output
            ), redirect_stderr(output):
                result = main(
                    ["--sdk", str(sdk), "build", "--project", str(project)]
                )

            self.assertEqual(result, ExitCode.CONFIG)
            self.assertFalse(run.called)

    def test_example_name_resolves_nested_template(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            workspace = root / "workspace"
            workspace.mkdir()

            result, payload = self.run_json(sdk, "hello", "--path", str(workspace))

            self.assertEqual(result, ExitCode.OK)
            self.assertEqual(payload["data"]["name"], "hello")
            self.assertTrue((workspace / "hello/main.c").is_file())

    def test_rejects_ambiguous_example_names(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            duplicate = sdk / "examples" / "other" / "hello"
            duplicate.mkdir(parents=True)
            (duplicate / "ecos-example.yml").write_text(
                "schema: 1\nname: hello\nsources:\n  - main.c\n",
                encoding="utf-8",
            )
            (duplicate / "main.c").write_text("int main(void) {}\n", encoding="utf-8")
            workspace = root / "workspace"
            workspace.mkdir()

            result, payload = self.run_json(
                sdk, "hello", "--path", str(workspace)
            )

            self.assertEqual(result, ExitCode.CONFIG)
            self.assertEqual(
                payload["diagnostics"][0]["code"],
                "ECOS_PROJECT_EXAMPLE_AMBIGUOUS",
            )
            self.assertFalse((workspace / "hello").exists())

    def test_dry_run_does_not_write(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            workspace = root / "workspace"
            workspace.mkdir()

            result, payload = self.run_json(
                sdk, "hello_world", "--path", str(workspace), "--dry-run"
            )

            self.assertEqual(result, ExitCode.OK)
            self.assertTrue(payload["data"]["dry_run"])
            self.assertFalse(payload["data"]["changed"])
            self.assertFalse((workspace / "hello_world").exists())
            self.assertEqual(list(workspace.iterdir()), [])

    def test_registered_sdk_writes_portable_project_pin(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            workspace = root / "workspace"
            workspace.mkdir()
            registry_path = root / "sdks.json"
            SdkRegistry(registry_path).register(sdk, name="dev", activate=True)
            output = StringIO()

            with mock.patch.dict(
                os.environ,
                {
                    "ECOS_SDK_HOME": "",
                    "ECOS_SDK_REGISTRY": str(registry_path),
                },
            ), mock.patch(
                "ecos_cli.sdk_resolver.Path.cwd", return_value=workspace
            ), redirect_stdout(output):
                result = main(
                    [
                        "project",
                        "create",
                        "hello_world",
                        "--path",
                        str(workspace),
                        "--format",
                        "json",
                    ]
                )

            payload = json.loads(output.getvalue())
            pin = json.loads(
                (workspace / "hello_world/.ecos/sdk.json").read_text(encoding="utf-8")
            )
            self.assertEqual(result, ExitCode.OK)
            self.assertEqual(payload["data"]["sdk"]["registration_name"], "dev")
            self.assertEqual(pin["registration"], "dev")
            self.assertEqual(pin["sdk_id"], "ecos-embedded-sdk")
            self.assertNotIn(str(sdk.resolve()), json.dumps(pin))

    def test_existing_path_requires_force_and_force_replaces_it(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            workspace = root / "workspace"
            destination = workspace / "hello_world"
            destination.mkdir(parents=True)
            stale = destination / "stale.txt"
            stale.write_text("keep", encoding="utf-8")

            result, payload = self.run_json(
                sdk, "hello_world", "--path", str(workspace)
            )

            self.assertEqual(result, ExitCode.CONFIG)
            self.assertEqual(
                payload["diagnostics"][0]["code"], "ECOS_PROJECT_PATH_EXISTS"
            )
            self.assertEqual(stale.read_text(encoding="utf-8"), "keep")

            result, payload = self.run_json(
                sdk, "hello_world", "--path", str(workspace), "--force"
            )
            self.assertEqual(result, ExitCode.OK)
            self.assertTrue(payload["data"]["replaced"])
            self.assertFalse(stale.exists())
            self.assertTrue((destination / "main.c").is_file())
            self.assertEqual(list(workspace.glob(".hello_world.ecos-*")), [])

    def test_rejects_missing_and_unsafe_examples(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = create_sdk(root / "sdk")
            workspace = root / "workspace"
            workspace.mkdir()

            result, payload = self.run_json(
                sdk, "missing", "--path", str(workspace)
            )
            self.assertEqual(result, ExitCode.CONFIG)
            self.assertEqual(
                payload["diagnostics"][0]["code"], "ECOS_PROJECT_EXAMPLE_NOT_FOUND"
            )

            result, payload = self.run_json(
                sdk, "../hello_world", "--path", str(workspace)
            )
            self.assertEqual(result, ExitCode.USAGE)
            self.assertEqual(
                payload["diagnostics"][0]["code"], "ECOS_PROJECT_INVALID_ARGUMENT"
            )
            self.assertEqual(list(workspace.iterdir()), [])


if __name__ == "__main__":
    unittest.main()
