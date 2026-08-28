import json
import os
import subprocess
import sys
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from io import StringIO
from pathlib import Path


SOURCE_ROOT = Path(__file__).parents[1] / "src"
sys.path.insert(0, str(SOURCE_ROOT))

from ecos_cli import installer  # noqa: E402
from ecos_cli.sdk_registry import SdkRegistry  # noqa: E402


def create_sdk_fixture(root: Path, version: str = "3.0.0") -> None:
    for relative in installer.SDK_DIRECTORIES:
        directory = root / relative
        directory.mkdir(parents=True)
        (directory / "source.txt").write_text(relative, encoding="utf-8")

    source_user_bsp = root / "board" / "UserBSP"
    source_user_bsp.mkdir()
    (source_user_bsp / "sdk-default.txt").write_text("default", encoding="utf-8")

    for relative in installer.TOOL_DIRECTORIES:
        directory = root / "tools" / relative
        directory.mkdir(parents=True)
        (directory / "source.txt").write_text(relative, encoding="utf-8")
    (root / "tools" / "ecos_cli" / "build").mkdir()
    (root / "tools" / "ecos_cli" / "build" / "generated.txt").write_text(
        "generated", encoding="utf-8"
    )
    (root / "tools" / "ecos.py").write_text("# fixture\n", encoding="utf-8")
    (root / "tools" / "ecos_cli" / "src").mkdir()
    manifest = {
        "schema_version": 1,
        "sdk_id": "ecos-embedded-sdk",
        "sdk_version": version,
        "channel": "development",
        "cli_compatibility": {"major": 3, "schema_version": "1.0"},
        "layout": {
            "boards": "board",
            "components": "components",
            "devices": "devices",
            "docs": "docs",
            "examples": "example",
            "hal": "hal",
            "templates": "templates",
            "cli": "tools/ecos_cli/src",
        },
        "toolchain": {
            "id": "xpack-riscv-none-elf-gcc",
            "release": "15.2.0-1",
        },
    }
    (root / "tools" / "sdk-manifest.json").write_text(
        json.dumps(manifest), encoding="utf-8"
    )


class SdkInstallerTest(unittest.TestCase):
    def test_default_install_base_is_platform_specific(self):
        home = Path("/users/test")
        self.assertEqual(
            installer.default_install_base(
                environ={}, home=home, platform_name="linux"
            ),
            home / ".local/share/ecos/sdk",
        )
        self.assertEqual(
            installer.default_install_base(
                environ={}, home=home, platform_name="darwin"
            ),
            home / "Library/Application Support/ECOS/SDKs",
        )
        self.assertEqual(
            installer.default_install_base(
                environ={"LOCALAPPDATA": "C:/Users/test/AppData/Local"},
                home=home,
                platform_name="win32",
            ),
            Path("C:/Users/test/AppData/Local/ECOS/SDKs"),
        )

    def test_versioned_install_path_appends_manifest_version(self):
        with tempfile.TemporaryDirectory() as directory:
            install_base = Path(directory) / "SDKs"
            self.assertEqual(
                installer.versioned_install_path(install_base, "3.0.0"),
                install_base.resolve() / "3.0.0",
            )

    def test_configures_all_supported_shells_idempotently(self):
        expected_paths = {
            "bash": Path(".bashrc"),
            "zsh": Path(".zshrc"),
            "fish": Path(".config/fish/config.fish"),
            "powershell": Path(
                "Documents/PowerShell/Microsoft.PowerShell_profile.ps1"
            ),
        }
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            home = root / "home"
            prefix = root / "sdk"
            for shell, relative in expected_paths.items():
                with self.subTest(shell=shell):
                    profile = home / relative
                    profile.parent.mkdir(parents=True, exist_ok=True)
                    profile.write_text("# user setting\n", encoding="utf-8")

                    first = installer.configure_shell(prefix, shell, home=home)
                    second = installer.configure_shell(prefix, shell, home=home)
                    content = profile.read_text(encoding="utf-8")

                    self.assertTrue(first["changed"])
                    self.assertFalse(second["changed"])
                    self.assertEqual(content.count(installer.SHELL_CONFIG_BEGIN), 1)
                    self.assertEqual(content.count(installer.SHELL_CONFIG_END), 1)
                    self.assertIn("# user setting", content)
                    self.assertIn(str(prefix), content)
                    self.assertNotIn(f"ECOS_SDK_HOME={prefix}", content)
                    self.assertEqual(first["config_file"], str(profile.resolve()))

    def test_bash_configuration_places_installed_sdk_first(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            prefix = root / "sdk"
            completion_file = (
                prefix
                / installer.COMPLETION_RELATIVE_ROOT
                / "ecos.bash"
            )
            completion_file.parent.mkdir(parents=True)
            completion_file.write_text("", encoding="utf-8")
            block = installer.shell_configuration_block(prefix, "bash")
            old_sdk_bin = root / "old-sdk" / "bin"
            environment = dict(os.environ)
            environment["PATH"] = (
                f"{old_sdk_bin}:{prefix / 'bin'}:/usr/bin:/bin"
            )
            environment["ECOS_SDK_HOME"] = str(
                installer.toolchain.default_prefix().resolve()
            )

            result = subprocess.run(
                [
                    "/bin/bash",
                    "-c",
                    f"{block}\nprintf '%s\\n%s' \"$PATH\" \"${{ECOS_SDK_HOME-unset}}\"",
                ],
                check=True,
                capture_output=True,
                text=True,
                env=environment,
            )

            configured_path, sdk_home = result.stdout.splitlines()
            self.assertEqual(configured_path.split(":", 1)[0], str(prefix / "bin"))
            self.assertEqual(sdk_home, "unset")

    def test_full_install_configures_requested_shell_profile(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk_root = root / "source"
            install_base = root / "installed"
            prefix = install_base / "3.0.0"
            profile = root / "profile" / ".bashrc"
            create_sdk_fixture(sdk_root)
            output = StringIO()

            with redirect_stdout(output):
                result = installer.main(
                    [
                        "--prefix",
                        str(install_base),
                        "--skip-toolchain",
                        "--shell",
                        "bash",
                        "--shell-profile",
                        str(profile),
                        "--format",
                        "json",
                        "--registry",
                        str(root / "registry.json"),
                    ],
                    sdk_root=sdk_root,
                )

            payload = json.loads(output.getvalue())
            configuration = payload["data"]["environment"]["configuration"]
            self.assertEqual(result, 0)
            self.assertEqual(payload["data"]["install_base"], str(install_base.resolve()))
            self.assertEqual(payload["data"]["prefix"], str(prefix.resolve()))
            self.assertEqual(configuration["state"], "configured")
            self.assertEqual(configuration["config_file"], str(profile.resolve()))
            self.assertTrue(profile.is_file())
            self.assertIn(installer.SHELL_CONFIG_BEGIN, profile.read_text(encoding="utf-8"))
            registry = json.loads(
                (root / "registry.json").read_text(encoding="utf-8")
            )
            self.assertEqual(registry["active"], "3.0.0")
            self.assertEqual(registry["sdks"]["3.0.0"]["kind"], "release")
            self.assertEqual(registry["sdks"]["3.0.0"]["root"], str(prefix.resolve()))
            for filename in ("ecos.bash", "_ecos", "ecos.fish", "ecos.ps1"):
                self.assertTrue(
                    prefix.joinpath("share/ecos/completions", filename).is_file()
                )

    def test_two_versions_install_side_by_side_under_one_base(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            install_base = root / "SDKs"
            registry_path = root / "sdks.json"
            first_source = root / "source-3.0.0"
            second_source = root / "source-3.1.0"
            create_sdk_fixture(first_source, "3.0.0")
            create_sdk_fixture(second_source, "3.1.0")

            for source in (first_source, second_source):
                with redirect_stdout(StringIO()), redirect_stderr(StringIO()):
                    result = installer.main(
                        [
                            "--prefix",
                            str(install_base),
                            "--skip-toolchain",
                            "--shell",
                            "none",
                            "--registry",
                            str(registry_path),
                        ],
                        sdk_root=source,
                    )
                self.assertEqual(result, 0)

            registry = json.loads(registry_path.read_text(encoding="utf-8"))
            self.assertTrue((install_base / "3.0.0/tools/sdk-manifest.json").is_file())
            self.assertTrue((install_base / "3.1.0/tools/sdk-manifest.json").is_file())
            self.assertEqual(registry["sdks"]["3.0.0"]["root"], str(install_base / "3.0.0"))
            self.assertEqual(registry["sdks"]["3.1.0"]["root"], str(install_base / "3.1.0"))
            self.assertEqual(registry["active"], "3.1.0")

    def test_force_replaces_registration_from_legacy_path(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "source"
            legacy = root / "legacy-sdk"
            install_base = root / "SDKs"
            registry_path = root / "sdks.json"
            create_sdk_fixture(source)
            create_sdk_fixture(legacy)

            SdkRegistry(registry_path).register(
                legacy, name="3.0.0", kind="release", activate=True
            )
            arguments = [
                "--prefix",
                str(install_base),
                "--skip-toolchain",
                "--shell",
                "none",
                "--registry",
                str(registry_path),
                "--format",
                "json",
            ]
            dry_run_output = StringIO()
            with redirect_stdout(dry_run_output):
                dry_run = installer.main(
                    [*arguments, "--dry-run"],
                    sdk_root=source,
                )

            dry_run_payload = json.loads(dry_run_output.getvalue())
            self.assertEqual(dry_run, 0)
            self.assertTrue(
                dry_run_payload["data"]["registration"]["requires_force"]
            )
            self.assertEqual(
                dry_run_payload["diagnostics"][0]["code"],
                "ECOS_SDK_REGISTRATION_CONFLICT",
            )
            self.assertFalse(install_base.exists())
            self.assertEqual(
                SdkRegistry(registry_path).registrations()["3.0.0"]["root"],
                str(legacy.resolve()),
            )

            blocked_output = StringIO()
            with redirect_stdout(blocked_output):
                blocked = installer.main(arguments, sdk_root=source)

            self.assertEqual(blocked, 3)
            self.assertEqual(
                json.loads(blocked_output.getvalue())["diagnostics"][0]["code"],
                "ECOS_SDK_REGISTRY_INVALID",
            )

            output = StringIO()
            with redirect_stdout(output):
                result = installer.main(
                    [*arguments, "--force"],
                    sdk_root=source,
                )

            payload = json.loads(output.getvalue())
            registry = json.loads(registry_path.read_text(encoding="utf-8"))
            expected = (install_base / "3.0.0").resolve()
            self.assertEqual(result, 0)
            self.assertTrue(payload["data"]["force"])
            self.assertEqual(registry["sdks"]["3.0.0"]["root"], str(expected))
            self.assertTrue((expected / "tools/sdk-manifest.json").is_file())

    def test_rejects_incomplete_managed_shell_block(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            profile = root / ".bashrc"
            profile.write_text(installer.SHELL_CONFIG_BEGIN + "\n", encoding="utf-8")

            with self.assertRaises(installer.InstallerError):
                installer.configure_shell(root / "sdk", "bash", profile=profile)

    def test_migrates_legacy_installer_shell_lines(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            prefix = root / "sdk"
            profile = root / ".bashrc"
            profile.write_text(
                "# user setting\n"
                f"export PATH={prefix}/bin:$PATH\n"
                f"export PATH={prefix}/toolchain/riscv_unknown/bin:$PATH\n"
                f"export ECOS_SDK_HOME={prefix}\n"
                "# ECOS command completion\n"
                f"source {prefix}/bin/ecos-completion.zsh\n",
                encoding="utf-8",
            )

            result = installer.configure_shell(prefix, "bash", profile=profile)
            content = profile.read_text(encoding="utf-8")

            self.assertTrue(result["changed"])
            self.assertIn("# user setting", content)
            self.assertNotIn("riscv_unknown", content)
            self.assertNotIn("ecos-completion.zsh", content)
            self.assertNotIn(f"export ECOS_SDK_HOME={prefix}", content)

    def test_text_dry_run_uses_ecos_info_prefix(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk_root = root / "source"
            prefix = root / "installed"
            create_sdk_fixture(sdk_root)
            errors = StringIO()

            with redirect_stderr(errors):
                result = installer.main(
                    [
                        "--prefix",
                        str(prefix),
                        "--skip-toolchain",
                        "--dry-run",
                        "--shell",
                        "none",
                        "--registry",
                        str(root / "registry.json"),
                    ],
                    sdk_root=sdk_root,
                )

            self.assertEqual(result, 0)
            self.assertIn("【ECOS-INFO】 Dry run", errors.getvalue())
            self.assertNotIn("\033[", errors.getvalue())

    def test_dry_run_is_structured_and_has_no_side_effects(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk_root = root / "source"
            prefix = root / "installed"
            create_sdk_fixture(sdk_root)
            output = StringIO()

            with redirect_stdout(output):
                result = installer.main(
                    [
                        "--prefix",
                        str(prefix),
                        "--skip-toolchain",
                        "--dry-run",
                        "--format",
                        "json",
                        "--shell",
                        "none",
                        "--registry",
                        str(root / "registry.json"),
                    ],
                    sdk_root=sdk_root,
                )

            payload = json.loads(output.getvalue())
            self.assertEqual(result, 0)
            self.assertEqual(payload["command"], "sdk.install")
            self.assertTrue(payload["data"]["dry_run"])
            self.assertIsNone(payload["data"]["toolchain"])
            self.assertFalse(prefix.exists())
            self.assertFalse((root / "registry.json").exists())

    def test_dry_run_includes_pinned_toolchain_selection(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk_root = root / "source"
            prefix = root / "installed"
            create_sdk_fixture(sdk_root)
            output = StringIO()

            with redirect_stdout(output):
                result = installer.main(
                    [
                        "--prefix",
                        str(prefix),
                        "--dry-run",
                        "--format",
                        "json",
                        "--shell",
                        "none",
                        "--registry",
                        str(root / "registry.json"),
                    ],
                    sdk_root=sdk_root,
                )

            payload = json.loads(output.getvalue())
            self.assertEqual(result, 0)
            self.assertEqual(
                payload["data"]["toolchain"]["selection"]["release"],
                "15.2.0-1",
            )
            self.assertEqual(
                payload["data"]["prefix"], str((prefix / "3.0.0").resolve())
            )
            self.assertFalse(prefix.exists())

    def test_core_install_preserves_user_bsp_and_generates_launchers(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk_root = root / "source"
            prefix = root / "installed"
            create_sdk_fixture(sdk_root)
            user_bsp = prefix / "board" / "UserBSP"
            user_bsp.mkdir(parents=True)
            (user_bsp / "user-board.txt").write_text("user", encoding="utf-8")
            legacy_bin = prefix / "bin"
            legacy_bin.mkdir(parents=True)
            (legacy_bin / "ecos-board").write_text("#!/bin/sh\n", encoding="utf-8")

            result = installer.install_sdk_core(sdk_root, prefix)

            self.assertEqual(result["prefix"], str(prefix.resolve()))
            self.assertEqual(
                (prefix / "components" / "source.txt").read_text(encoding="utf-8"),
                "components",
            )
            self.assertEqual(
                (prefix / "board" / "UserBSP" / "user-board.txt").read_text(
                    encoding="utf-8"
                ),
                "user",
            )
            self.assertTrue(prefix.joinpath("board/UserBSP/sdk-default.txt").is_file())
            self.assertFalse(prefix.joinpath("tools/ecos_cli/build").exists())
            self.assertTrue(prefix.joinpath("bin/ecos").is_file())
            self.assertTrue(prefix.joinpath("bin/ecos.cmd").is_file())
            self.assertTrue(prefix.joinpath("tools/sdk-manifest.json").is_file())
            self.assertFalse(prefix.joinpath("bin/ecos-board").exists())
            self.assertEqual(
                {path.name for path in prefix.joinpath("bin").iterdir()},
                {"ecos", "ecos.cmd"},
            )
            self.assertTrue(
                prefix.joinpath("share/ecos/completions/ecos.bash").is_file()
            )
            self.assertIn(
                "Installed ECOS Python CLI launcher",
                prefix.joinpath("bin/ecos").read_text(encoding="utf-8"),
            )

    def test_rejects_overlapping_install_prefix(self):
        with tempfile.TemporaryDirectory() as directory:
            sdk_root = Path(directory) / "source"
            create_sdk_fixture(sdk_root)

            with self.assertRaises(installer.InstallerError):
                installer.validate_layout(sdk_root, sdk_root / "installed")


if __name__ == "__main__":
    unittest.main()
