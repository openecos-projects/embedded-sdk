"""Portable skill content, opt-in installation and read-only helper contracts."""

import importlib.util
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import yaml

SDK_ROOT = Path(__file__).resolve().parents[3]
SKILL_ROOT = SDK_ROOT / ".agents/skills/ecos-sdk"


def load_script(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


install = load_script("ecos_install_skill", SDK_ROOT / "tools/install_skill.py")
check = load_script("ecos_check_project", SKILL_ROOT / "scripts/check_project.py")


class SkillTestBase(unittest.TestCase):
    def setUp(self):
        workspace = SDK_ROOT / "testdir"
        workspace.mkdir(exist_ok=True)
        self.temporary = tempfile.TemporaryDirectory(prefix="agent-skill-", dir=workspace)
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)

    def fixture(self):
        sdk = self.root / "SDK with spaces"
        manifest = sdk / "tools/sdk-manifest.json"
        manifest.parent.mkdir(parents=True)
        manifest.write_text(json.dumps({
            "schema_version": 1, "sdk_id": "ecos-embedded-sdk", "sdk_version": "3.0.1",
        }), encoding="utf-8")
        source = sdk / install.SKILL_RELATIVE
        (source / "references").mkdir(parents=True)
        (source / "SKILL.md").write_text("---\nname: ecos-sdk\ndescription: ECOS tests\n---\n", encoding="utf-8")
        (source / "references/workflow.md").write_text("Read the selected SDK.\n", encoding="utf-8")
        return sdk, source


class SkillContentTest(SkillTestBase):
    def test_portable_metadata_and_all_bundled_links_exist(self):
        text = (SKILL_ROOT / "SKILL.md").read_text(encoding="utf-8")
        self.assertTrue(text.startswith("---\n"))
        frontmatter = yaml.safe_load(text.split("---", 2)[1])
        self.assertEqual(frontmatter["name"], SKILL_ROOT.name)
        self.assertRegex(frontmatter["name"], r"^[a-z0-9]+(?:-[a-z0-9]+)*$")
        self.assertLessEqual(len(frontmatter["name"]), 64)
        self.assertTrue(frontmatter["description"])
        self.assertLessEqual(len(frontmatter["description"]), 1024)
        self.assertEqual(frontmatter["metadata"]["sdk-major"], "3")
        self.assertEqual(frontmatter["license"], "Apache-2.0")
        self.assertEqual((SKILL_ROOT / "LICENSE").read_bytes(), (SDK_ROOT / "LICENSE").read_bytes())
        self.assertNotIn("allowed-tools", frontmatter)  # Not an authorization grant.
        for path in SKILL_ROOT.rglob("*.md"):
            content = path.read_text(encoding="utf-8")
            self.assertNotIn("/home/timmo", content)
            for target in re.findall(r"\]\(([^)]+)\)", content):
                self.assertFalse(Path(target).is_absolute())
                resolved = (path.parent / target).resolve()
                self.assertTrue(resolved.is_relative_to(SKILL_ROOT))
                self.assertTrue(resolved.is_file(), (path, target))
        for reference in (SKILL_ROOT / "references").glob("*.md"):
            self.assertIn(f"references/{reference.name}", text)

    def test_sdk_release_copy_contains_skill_and_opt_in_installer(self):
        sys.path.insert(0, str(SDK_ROOT / "tools/ecos_cli/src"))
        from ecos_cli import installer, sdk_manifest

        manifest = sdk_manifest.load_manifest(SDK_ROOT)
        release_paths = sdk_manifest.release_paths(manifest)
        self.assertIn(".agents/skills/ecos-sdk", release_paths)
        self.assertIn("tools/install_skill.py", release_paths)
        self.assertNotIn(".agents", release_paths)  # Do not ship unrelated Agent state.
        destination = self.root / "release copy"
        destination.mkdir()
        installer._copy_sdk_files(
            SDK_ROOT, destination, (".agents/skills/ecos-sdk", "tools/install_skill.py")
        )
        self.assertEqual((destination / install.SKILL_RELATIVE / "SKILL.md").read_bytes(),
                         (SKILL_ROOT / "SKILL.md").read_bytes())
        self.assertTrue((destination / "tools/install_skill.py").is_file())
        self.assertFalse(any(destination.rglob("*.pyc")))
        self.assertFalse(any(destination.rglob("__pycache__")))


class SkillInstallerTest(SkillTestBase):
    def test_dry_run_creates_nothing_and_install_is_idempotent(self):
        sdk, source = self.fixture()
        parent = self.root / "new project/.agents/skills"
        preview = install.install_skill(sdk, parent, dry_run=True)
        self.assertTrue(preview["would_change"])
        self.assertFalse(preview["changed"])
        self.assertFalse((self.root / "new project").exists())
        result = install.install_skill(sdk, parent)
        self.assertEqual(result["action"], "install")
        destination = parent / "ecos-sdk"
        self.assertEqual((destination / "SKILL.md").read_bytes(), (source / "SKILL.md").read_bytes())
        marker = destination / install.MARKER
        before = marker.stat().st_mtime_ns
        again = install.install_skill(sdk, parent)
        self.assertEqual(again["action"], "reused")
        self.assertFalse(again["changed"])
        self.assertEqual(marker.stat().st_mtime_ns, before)
        self.assertFalse((self.root / "new project/.pi").exists())

    def test_update_requires_opt_in_and_replaces_only_clean_managed_content(self):
        sdk, source = self.fixture()
        parent = self.root / "skills"
        install.install_skill(sdk, parent)
        (source / "references/workflow.md").write_text("updated\n", encoding="utf-8")
        with self.assertRaisesRegex(install.SkillInstallError, "--update"):
            install.install_skill(sdk, parent)
        preview = install.install_skill(sdk, parent, update=True, dry_run=True)
        self.assertEqual(preview["action"], "update")
        self.assertNotEqual((parent / "ecos-sdk/references/workflow.md").read_text(), "updated\n")
        result = install.install_skill(sdk, parent, update=True)
        self.assertEqual(result["action"], "update")
        self.assertEqual((parent / "ecos-sdk/references/workflow.md").read_text(), "updated\n")
        self.assertFalse(any(parent.glob(".ecos-sdk-install-*")))

    def test_rejects_unmanaged_changed_added_or_deleted_content_even_with_update(self):
        sdk, _ = self.fixture()
        for kind in ("unmanaged", "changed", "added", "deleted", "empty-directory"):
            with self.subTest(kind=kind):
                parent = self.root / kind
                destination = parent / "ecos-sdk"
                if kind == "unmanaged":
                    destination.mkdir(parents=True)
                    (destination / "SKILL.md").write_text("user-owned", encoding="utf-8")
                else:
                    install.install_skill(sdk, parent)
                    if kind == "changed":
                        (destination / "SKILL.md").write_text("user change", encoding="utf-8")
                    elif kind == "added":
                        (destination / "notes.txt").write_text("user notes", encoding="utf-8")
                    elif kind == "deleted":
                        (destination / "SKILL.md").unlink()
                    else:
                        (destination / "user-directory").mkdir()
                before = sorted(str(p.relative_to(destination)) for p in destination.rglob("*"))
                with self.assertRaises(install.SkillInstallError):
                    install.install_skill(sdk, parent, update=True)
                self.assertEqual(sorted(str(p.relative_to(destination)) for p in destination.rglob("*")), before)

    def test_rejects_source_overlap_missing_skill_and_wrong_sdk(self):
        sdk, source = self.fixture()
        for parent in (source.parent, source / "nested"):
            with self.subTest(parent=parent), self.assertRaises(install.SkillInstallError):
                install.install_skill(sdk, parent)
        (source / "SKILL.md").unlink()
        with self.assertRaisesRegex(install.SkillInstallError, "missing"):
            install.install_skill(sdk, self.root / "skills")
        (sdk / "tools/sdk-manifest.json").write_text('{"sdk_id":"other"}', encoding="utf-8")
        with self.assertRaisesRegex(install.SkillInstallError, "major 3"):
            install.install_skill(sdk, self.root / "skills")

    def test_ignores_source_bytecode(self):
        sdk, source = self.fixture()
        (source / "scripts/__pycache__").mkdir(parents=True)
        (source / "scripts/__pycache__/test.pyc").write_bytes(b"bytecode")
        parent = self.root / "skills"
        install.install_skill(sdk, parent)
        self.assertFalse(any(parent.rglob("*.pyc")))
        self.assertFalse(any(parent.rglob("__pycache__")))

    def test_rejects_symlinked_source_and_destination(self):
        sdk, source = self.fixture()
        outside = self.root / "outside"
        outside.mkdir()
        link = self.root / "link"
        try:
            link.symlink_to(outside, target_is_directory=True)
        except (OSError, NotImplementedError):
            self.skipTest("symlinks are unavailable")
        with self.assertRaisesRegex(install.SkillInstallError, "symlink/junction"):
            install.install_skill(sdk, link / "skills")
        (source / "linked-file").symlink_to(sdk / "tools/sdk-manifest.json")
        with self.assertRaisesRegex(install.SkillInstallError, "symlink/junction"):
            install.install_skill(sdk, self.root / "skills")
        self.assertEqual(list(outside.iterdir()), [])

    def test_update_restores_previous_tree_on_replacement_failure(self):
        sdk, source = self.fixture()
        parent = self.root / "skills"
        install.install_skill(sdk, parent)
        old = (parent / "ecos-sdk/SKILL.md").read_bytes()
        (source / "SKILL.md").write_text("new version", encoding="utf-8")
        replace = Path.replace

        def fail_stage(path, destination):
            if path.name == "staged":
                raise OSError("simulated rename failure")
            return replace(path, destination)

        with mock.patch.object(Path, "replace", fail_stage):
            with self.assertRaises(OSError):
                install.install_skill(sdk, parent, update=True)
        self.assertEqual((parent / "ecos-sdk/SKILL.md").read_bytes(), old)
        self.assertFalse(any(parent.glob(".ecos-sdk-install-*")))

    def test_keeps_backup_when_rollback_also_fails(self):
        sdk, source = self.fixture()
        parent = self.root / "skills"
        install.install_skill(sdk, parent)
        old = (parent / "ecos-sdk/SKILL.md").read_bytes()
        (source / "SKILL.md").write_text("new version", encoding="utf-8")
        replace = Path.replace

        def fail_stage_and_restore(path, destination):
            if path.name in {"staged", "previous"}:
                raise OSError("simulated filesystem failure")
            return replace(path, destination)

        with mock.patch.object(Path, "replace", fail_stage_and_restore):
            with self.assertRaisesRegex(install.SkillInstallError, "recover it from"):
                install.install_skill(sdk, parent, update=True)
        backups = list(parent.glob(".ecos-sdk-install-*/previous/SKILL.md"))
        self.assertEqual(len(backups), 1)
        self.assertEqual(backups[0].read_bytes(), old)

    def test_does_not_replace_when_source_or_destination_changes_during_copy(self):
        sdk, source = self.fixture()
        parent = self.root / "skills"
        install.install_skill(sdk, parent)
        old = (parent / "ecos-sdk/SKILL.md").read_bytes()
        changed = source / "references/workflow.md"
        changed.write_text("new content", encoding="utf-8")
        copy = shutil.copy2

        def edit_source_after_copy(src, dst):
            result = copy(src, dst)
            if Path(src).name == "workflow.md":
                Path(dst).write_text("concurrent source change", encoding="utf-8")
            return result

        with mock.patch.object(install.shutil, "copy2", side_effect=edit_source_after_copy):
            with self.assertRaisesRegex(install.SkillInstallError, "changed while copying"):
                install.install_skill(sdk, parent, update=True)
        self.assertEqual((parent / "ecos-sdk/SKILL.md").read_bytes(), old)

        def edit_destination_after_copy(src, dst):
            result = copy(src, dst)
            if Path(src).name == "workflow.md":
                (parent / "ecos-sdk/user-notes.txt").write_text("preserve me", encoding="utf-8")
            return result

        with mock.patch.object(install.shutil, "copy2", side_effect=edit_destination_after_copy):
            with self.assertRaisesRegex(install.SkillInstallError, "was modified"):
                install.install_skill(sdk, parent, update=True)
        self.assertEqual((parent / "ecos-sdk/user-notes.txt").read_text(), "preserve me")
        self.assertEqual((parent / "ecos-sdk/SKILL.md").read_bytes(), old)
        self.assertFalse(any(parent.glob(".ecos-sdk-install-*")))

    def test_cli_reports_conflict_as_json_without_force_overwrite(self):
        sdk, _ = self.fixture()
        parent = self.root / "skills"
        (parent / "ecos-sdk").mkdir(parents=True)
        (parent / "ecos-sdk/SKILL.md").write_text("user-owned", encoding="utf-8")
        result = subprocess.run([
            sys.executable, "-S", str(SDK_ROOT / "tools/install_skill.py"), "--sdk-root", str(sdk),
            "--skills-dir", str(parent), "--update", "--format", "json",
        ], capture_output=True, text=True, check=False)
        self.assertEqual(result.returncode, 3, result.stderr)
        payload = json.loads(result.stdout)
        self.assertEqual(payload["status"], "error")
        self.assertIn("unmanaged", payload["diagnostics"][0])
        self.assertEqual((parent / "ecos-sdk/SKILL.md").read_text(), "user-owned")

    def test_cli_has_no_default_scope_and_runs_without_dependencies(self):
        sdk, _ = self.fixture()
        script = str(SDK_ROOT / "tools/install_skill.py")
        environment = os.environ.copy()
        environment.pop("PYTHONPATH", None)
        missing = subprocess.run([sys.executable, "-S", script], capture_output=True, text=True, env=environment)
        self.assertEqual(missing.returncode, 2)
        parent = self.root / "custom 中文 path"
        result = subprocess.run([
            sys.executable, "-S", script, "--sdk-root", str(sdk), "--skills-dir", str(parent),
            "--dry-run", "--format", "json",
        ], capture_output=True, text=True, env=environment)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(result.stdout)["status"], "ok")
        self.assertFalse(parent.exists())

    def test_project_and_user_scopes_do_not_edit_agent_settings(self):
        sdk, _ = self.fixture()
        project = self.root / "project"
        home = self.root / "home"
        project.mkdir()
        settings = project / ".pi/settings.json"
        settings.parent.mkdir()
        settings.write_text('{"theme":"custom"}', encoding="utf-8")
        with mock.patch("builtins.print"):
            self.assertEqual(install.main(["--sdk-root", str(sdk), "--project", str(project)]), 0)
            with mock.patch.object(Path, "home", return_value=home):
                self.assertEqual(install.main(["--sdk-root", str(sdk), "--user"]), 0)
        self.assertTrue((project / ".agents/skills/ecos-sdk/SKILL.md").is_file())
        self.assertTrue((home / ".agents/skills/ecos-sdk/SKILL.md").is_file())
        self.assertEqual(settings.read_text(), '{"theme":"custom"}')
        self.assertFalse((home / ".pi").exists())


class SkillProjectCheckTest(SkillTestBase):
    def selection(self, sdk):
        return {
            "root": str(sdk), "sdk_id": "ecos-embedded-sdk", "sdk_version": "3.0.0",
            "channel": "development", "source": "project:pin",
        }

    def envelope(self, data=None, status="ok"):
        return {"cli_version": "3.0.0.dev0", "schema_version": "1.0", "status": status, "data": data, "diagnostics": []}

    def mock_cli(self, sdk, toolchain="installed"):
        def run(argv, **kwargs):
            if "current" in argv:
                data = self.selection(sdk)
            elif "toolchain" in argv:
                data = {"installation": {"state": toolchain}}
            else:
                data = {}
            return subprocess.CompletedProcess(argv, 0, json.dumps(self.envelope(data)), "")
        return run

    def test_uses_confirmed_sdk_root_and_only_read_only_commands(self):
        project = self.root / "external app"
        project.mkdir()
        cli = self.root / "cli.py"
        cli.write_text("# not executed", encoding="utf-8")
        with mock.patch.object(check.subprocess, "run", side_effect=self.mock_cli(SDK_ROOT)) as run:
            report = check.collect(str(cli), None, project, 5)
        self.assertEqual(report["status"], "ok")
        self.assertFalse(report["hardware_tested"])
        self.assertEqual(run.call_count, 4)
        first = run.call_args_list[0].args[0]
        self.assertNotIn("--sdk", first)
        for call in run.call_args_list:
            self.assertFalse(call.kwargs["shell"])
            self.assertEqual(call.kwargs["cwd"], project)
            self.assertEqual(call.kwargs["env"]["PYTHONDONTWRITEBYTECODE"], "1")
            self.assertFalse(set(call.args[0]) & {"build", "configure", "flash", "monitor", "install"})
        for call in run.call_args_list[1:]:
            self.assertEqual(call.args[0][2:4], ["--sdk", str(SDK_ROOT)])
        self.assertEqual(list(project.iterdir()), [])

    def test_explicit_selector_is_not_replaced_by_skill_location(self):
        cli = self.root / "cli.py"
        cli.touch()
        with mock.patch.object(check.subprocess, "run", side_effect=self.mock_cli(SDK_ROOT)) as run:
            report = check.collect(str(cli), "chosen-sdk", self.root, 5)
        self.assertEqual(report["status"], "ok")
        self.assertEqual(run.call_args_list[0].args[0][2:4], ["--sdk", "chosen-sdk"])

    def test_missing_toolchain_is_an_error_even_when_cli_exit_is_zero(self):
        cli = self.root / "cli.py"
        cli.touch()
        with mock.patch.object(check.subprocess, "run", side_effect=self.mock_cli(SDK_ROOT, "missing")):
            report = check.collect(str(cli), None, self.root, 5)
        self.assertEqual(report["status"], "error")
        self.assertIn("not ready", report["diagnostics"][0])

    def test_invalid_sdk_selection_does_not_fall_back(self):
        cli = self.root / "cli.py"
        cli.touch()
        result = subprocess.CompletedProcess([], 3, json.dumps(self.envelope(status="error")), "bad pin")
        with mock.patch.object(check.subprocess, "run", return_value=result) as run:
            report = check.collect(str(cli), None, self.root, 5)
        self.assertEqual(report["status"], "error")
        self.assertEqual(run.call_count, 1)
        self.assertEqual(report["queries"][0]["stderr"], "bad pin")

    def test_malformed_json_and_timeout_are_reported(self):
        cli = self.root / "cli.py"
        cli.touch()
        wrong_cli = {**self.envelope(), "cli_version": "4.0.0"}
        cases = (subprocess.CompletedProcess([], 0, "not JSON", ""),
                 subprocess.CompletedProcess([], 0, '{"schema_version":"9"}', ""),
                 subprocess.CompletedProcess([], 0, json.dumps(wrong_cli), ""))
        for result in cases:
            with self.subTest(result=result), mock.patch.object(check.subprocess, "run", return_value=result):
                self.assertEqual(check.collect(str(cli), None, self.root, 1)["status"], "error")
        with mock.patch.object(check.subprocess, "run", side_effect=subprocess.TimeoutExpired([], 1)):
            report = check.collect(str(cli), None, self.root, 1)
        self.assertIn("timed out", report["queries"][0]["error"])

    def test_rejects_sdk_major_or_identity_mismatch(self):
        sdk, _ = self.fixture()
        cli = self.root / "cli.py"
        cli.touch()
        manifest_path = sdk / "tools/sdk-manifest.json"
        for manifest in (
            {"schema_version": 1, "sdk_id": "ecos-embedded-sdk", "sdk_version": "4.0.0"},
            {"schema_version": 1, "sdk_id": "other", "sdk_version": "3.0.0"},
            ["not an object"],
        ):
            manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
            with self.subTest(manifest=manifest), mock.patch.object(
                check.subprocess, "run", side_effect=self.mock_cli(sdk)
            ) as run:
                report = check.collect(str(cli), None, self.root, 1)
            self.assertEqual(report["status"], "error")
            self.assertEqual(run.call_count, 1)
        manifest_path.write_text(json.dumps({
            "schema_version": 1, "sdk_id": "ecos-embedded-sdk", "sdk_version": "3.0.1",
            "cli_compatibility": {"major": 3, "schema_version": "1.0"},
        }), encoding="utf-8")
        with mock.patch.object(check.subprocess, "run", side_effect=self.mock_cli(sdk)) as run:
            report = check.collect(str(cli), None, self.root, 1)
        self.assertEqual(report["status"], "error")
        self.assertIn("identities disagree", report["diagnostics"][0])
        self.assertEqual(run.call_count, 1)

    def test_missing_cli_and_bad_project_are_structured_errors(self):
        with mock.patch.object(check.subprocess, "run") as run:
            report = check.collect("ecos", "", self.root, 1)
            self.assertEqual(report["status"], "error")
            self.assertIn("must not be empty", report["diagnostics"][0])
            run.assert_not_called()
        report = check.collect(str(self.root / "missing-cli.py"), None, self.root, 1)
        self.assertEqual(report["status"], "error")
        self.assertEqual(report["queries"], [])
        self.assertIn("CLI not found", report["diagnostics"][0])
        report = check.collect("ecos", None, self.root / "missing-project", 1)
        self.assertEqual(report["status"], "error")
        self.assertIn("does not exist", report["diagnostics"][0])

    def test_windows_cmd_uses_launch_descriptor_not_a_shell(self):
        cli = self.root / "host/bin/ecos.cmd"
        cli.parent.mkdir(parents=True)
        cli.touch()
        script = cli.with_suffix(".py")
        script.touch()
        descriptor = self.root / "host/share/ecos/cli-launch.json"
        descriptor.parent.mkdir(parents=True)
        descriptor.write_text(json.dumps({"python_executable": sys.executable, "script": str(script)}), encoding="utf-8")
        self.assertEqual(check.cli_prefix(str(cli)), [sys.executable, str(script)])
        descriptor.unlink()
        with self.assertRaisesRegex(check.CheckError, "descriptor"):
            check.cli_prefix(str(cli))

    def test_installed_skill_checks_project_pin_without_sdk_sibling_or_build(self):
        # An actual CLI integration test: only create metadata, never configure/build.
        sys.path.insert(0, str(SDK_ROOT / "tools/ecos_cli/src"))
        from ecos_cli.cli import main
        from ecos_cli.sdk_manifest import context_from_root
        from ecos_cli.sdk_registry import SdkRegistry
        from ecos_cli.sdk_resolver import write_project_pin
        from contextlib import redirect_stdout, redirect_stderr
        from io import StringIO

        with redirect_stdout(StringIO()), redirect_stderr(StringIO()):
            self.assertEqual(main([
                "--sdk", str(SDK_ROOT), "project", "create", "hello", "--board", "ch32v103c8t6",
                "--path", str(self.root),
            ]), 0)
        parent = self.root / "standalone skills"
        install.install_skill(SDK_ROOT, parent)
        copied = parent / "ecos-sdk"
        project = self.root / "hello"
        registry = self.root / "registry.json"
        SdkRegistry(registry).register(SDK_ROOT, name="skill-test", activate=False)
        context = context_from_root(SDK_ROOT, kind="checkout", source="test", registration_name="skill-test")
        write_project_pin(project, context)
        before = {p.relative_to(project).as_posix(): p.read_bytes() for p in project.rglob("*") if p.is_file()}
        environment = os.environ.copy()
        environment["ECOS_SDK_REGISTRY"] = str(registry)
        result = subprocess.run([
            sys.executable, str(copied / "scripts/check_project.py"),
            "--cli", str(SDK_ROOT / "tools/ecos.py"), "--project", str(project),
        ], capture_output=True, text=True, check=False, env=environment)
        # Toolchain readiness is host-dependent; the report must still be well-formed
        # and prove it selected the project's pin, not a skill-relative directory.
        payload = json.loads(result.stdout)
        self.assertIn(result.returncode, (0, 3), result.stderr)
        self.assertEqual(payload["sdk"]["root"], str(SDK_ROOT))
        self.assertTrue(payload["sdk"]["selected_by"].startswith("project:"), payload)
        self.assertFalse(payload["hardware_tested"])
        self.assertTrue(all(record["ok"] for record in payload["queries"][:3]), payload)
        after = {p.relative_to(project).as_posix(): p.read_bytes() for p in project.rglob("*") if p.is_file()}
        self.assertEqual(before, after)
        self.assertFalse((project / "build").exists())
        self.assertFalse((project / ".ecos/generated").exists())


if __name__ == "__main__":
    unittest.main()
