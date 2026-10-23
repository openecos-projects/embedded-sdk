import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from io import StringIO
from pathlib import Path
from unittest.mock import patch


SOURCE_ROOT = Path(__file__).parents[1] / "src"
SDK_ROOT = Path(__file__).parents[3]
sys.path.insert(0, str(SOURCE_ROOT))

from ecos_cli import installer  # noqa: E402
from ecos_cli.cli import main as cli_main  # noqa: E402
from ecos_cli.sdk_registry import SdkRegistry  # noqa: E402

FIXTURE_SDK_DIRECTORIES = (
    "components", "drivers", "hal", "environments",
    "third_party", "boards", "examples", "docs", "devices",
)
FIXTURE_RELEASE_PATHS = (
    "LICENSE",
    *FIXTURE_SDK_DIRECTORIES,
    "tools/ecos_cli/src",
    "tools/ecos_cli/pyproject.toml",
    "tools/ecos.py",
    "tools/install.py",
    "tools/sdk-manifest.json",
)


def create_sdk_fixture(
    root: Path, version: str = "3.0.0", *, channel: str = "release"
) -> None:
    for relative in FIXTURE_SDK_DIRECTORIES:
        directory = root / relative
        directory.mkdir(parents=True)
        (directory / "source.txt").write_text(relative, encoding="utf-8")

    source_user_bsp = root / "boards" / "UserBSP"
    source_user_bsp.mkdir()
    (source_user_bsp / "sdk-default.txt").write_text("default", encoding="utf-8")

    for relative in ("fixdep", "kconfig", "scripts", "toolchains", "ecos_cli"):
        directory = root / "tools" / relative
        directory.mkdir(parents=True)
        (directory / "source.txt").write_text(relative, encoding="utf-8")
    (root / "tools" / "ecos_cli" / "build").mkdir()
    (root / "tools" / "ecos_cli" / "build" / "generated.txt").write_text(
        "generated", encoding="utf-8"
    )
    (root / "tools" / "ecos.py").write_text("# fixture\n", encoding="utf-8")
    (root / "tools" / "install.py").write_text("# fixture\n", encoding="utf-8")
    (root / "tools" / "ecos_cli" / "pyproject.toml").write_text(
        "# fixture\n", encoding="utf-8"
    )
    (root / "LICENSE").write_text("fixture\n", encoding="utf-8")
    (root / "tools" / "ecos_cli" / "src").mkdir()
    fake_cli = root / "tools" / "ecos_cli" / "src" / "ecos_cli"
    fake_cli.mkdir()
    (fake_cli / "__init__.py").write_text('__version__ = "3.0.0.dev0"\n', encoding="utf-8")
    (fake_cli / "cli.py").write_text("def main():\n    return 0\n", encoding="utf-8")
    manifest = {
        "schema_version": 1,
        "sdk_id": "ecos-embedded-sdk",
        "sdk_version": version,
        "channel": channel,
        "cli_compatibility": {"major": 3, "schema_version": "1.0"},
        "layout": {
        "boards": "boards",
            "components": "components",
            "drivers": "drivers",
            "devices": "devices",
            "docs": "docs",
            "examples": "examples",
            "hal": "hal",
            "cli": "tools/ecos_cli/src",
        },
        "toolchain": {
            "id": "xpack-riscv-none-elf-gcc",
            "release": "15.2.0-1",
        },
        "release": {
            "paths": list(FIXTURE_RELEASE_PATHS),
            "exclude": ["boards/UserBSP"],
        },
    }
    (root / "tools" / "sdk-manifest.json").write_text(
        json.dumps(manifest), encoding="utf-8"
    )


def fake_python_dependency_install(prefix: Path, **_: object) -> dict[str, object]:
    return {
        "state": "installed",
        "root": str(prefix / installer.PYTHON_DEPENDENCY_RELATIVE_ROOT),
        "packages": list(installer.PYTHON_DEPENDENCIES),
    }


def fake_host_dependency_install(prefix: Path, **_: object) -> dict[str, object]:
    root = prefix / installer.HOST_DEPENDENCY_RELATIVE_ROOT
    host_bin = installer.dependencies.host_dependency_bin(prefix)
    ninja_name = "ninja.exe" if sys.platform == "win32" else "ninja"
    return {
        "state": "installed",
        "root": str(root),
        "bin": str(host_bin),
        "paths": [
            str(path) for path in installer.dependencies.host_dependency_paths(prefix)
        ],
        "packages": list(installer.HOST_TOOL_PACKAGES),
        "tools": {
            item["name"]: str(
                root / "cmake/data/bin/cmake"
                if item["name"] == "cmake"
                else host_bin / ninja_name
            )
            for item in installer.HOST_TOOL_DEPENDENCIES
        },
    }


class SdkInstallerTest(unittest.TestCase):
    def test_dry_run_bootstraps_without_third_party_python_packages(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            environment = os.environ.copy()
            environment.pop("PYTHONPATH", None)
            environment.pop("PYTHONHOME", None)
            result = subprocess.run(
                [
                    sys.executable, "-S", str(SDK_ROOT / "tools/install.py"),
                    "--dry-run", "--skip-toolchain", "--skip-wlink", "--shell", "none",
                    "--host-prefix", str(root / "host"),
                    "--registry", str(root / "registry.json"),
                    "--format", "json",
                ],
                env=environment,
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(json.loads(result.stdout)["status"], "ok")
            self.assertFalse((root / "host").exists())
            self.assertFalse((root / "registry.json").exists())

    def test_development_mode_registers_checkout_without_copying_sdk(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "source"
            install_base = root / "SDKs"
            host_prefix = root / "host"
            registry_path = root / "registry.json"
            create_sdk_fixture(source, channel="development")
            manifest_path = source / "tools/sdk-manifest.json"
            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
            del manifest["release"]
            manifest_path.write_text(json.dumps(manifest), encoding="utf-8")

            with patch.object(
                installer,
                "install_python_dependencies",
                side_effect=fake_python_dependency_install,
            ), patch.object(
                installer,
                "install_host_dependencies",
                side_effect=fake_host_dependency_install,
            ), redirect_stdout(StringIO()), redirect_stderr(StringIO()):
                result = installer.main(
                    [
                        "--prefix", str(install_base),
                        "--host-prefix", str(host_prefix),
                        "--skip-toolchain", "--skip-wlink", "--shell", "none",
                        "--registry", str(registry_path),
                    ],
                    sdk_root=source,
                )

            self.assertEqual(result, 0)
            self.assertFalse(install_base.exists())
            self.assertTrue((host_prefix / "bin/ecos").is_file())
            entry = SdkRegistry(registry_path).registrations()["3.0.0"]
            self.assertEqual(entry["kind"], "checkout")
            self.assertEqual(entry["root"], str(source.resolve()))

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

    def test_host_dependency_script_directory_is_platform_specific(self):
        prefix = Path("C:/Users/test/AppData/Local/ECOS/SDKs/3.0.0")

        self.assertEqual(
            installer.dependencies.host_dependency_bin(
                prefix, platform_name="win32"
            ),
            prefix / installer.HOST_DEPENDENCY_RELATIVE_ROOT / "Scripts",
        )
        self.assertEqual(
            installer.dependencies.host_dependency_bin(
                prefix, platform_name="linux"
            ),
            prefix / installer.HOST_DEPENDENCY_RELATIVE_ROOT / "bin",
        )

    def test_managed_host_tools_accept_windows_executable_layout(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            cmake = root / "cmake/data/bin/cmake.exe"
            ninja = root / "Scripts/ninja.exe"
            cmake.parent.mkdir(parents=True)
            ninja.parent.mkdir(parents=True)
            cmake.write_bytes(b"")
            ninja.write_bytes(b"")

            self.assertEqual(
                installer.dependencies.managed_host_tool(root, "cmake"),
                cmake.resolve(),
            )
            self.assertEqual(
                installer.dependencies.managed_host_tool(root, "ninja"),
                ninja.resolve(),
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
                    if shell == "fish":
                        expected_prefix = str(prefix.resolve()).replace("\\", "\\\\").replace("'", "\\'")
                    else:
                        expected_prefix = str(prefix.resolve())
                    self.assertIn(expected_prefix, content)
                    self.assertNotIn(f"ECOS_SDK_HOME={prefix}", content)
                    self.assertEqual(first["config_file"], str(profile.resolve()))

    @unittest.skipIf(
        os.name == "nt" or sys.platform == "darwin",
        "POSIX bash configuration test requires a supported Bash runtime",
    )
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
                f"{old_sdk_bin.resolve()}:{(prefix / 'bin').resolve()}:/usr/bin:/bin"
            )
            environment["ECOS_SDK_HOME"] = str(
                installer.toolchain.legacy_prefix().resolve()
            )

            environment.pop("BASH_ENV", None)
            environment.pop("ENV", None)
            result = subprocess.run(
                [
                    "/bin/bash",
                    "-c",
                    f"{block}\n{block}\nprintf '%s\\n%s' \"$PATH\" \"${{ECOS_SDK_HOME-unset}}\"",
                ],
                check=True,
                capture_output=True,
                text=True,
                env=environment,
            )

            configured_path, sdk_home = result.stdout.splitlines()
            self.assertEqual(
                configured_path.split(":"),
                [
                    str((prefix / "bin").resolve()),
                    str(
                        (
                            prefix
                            / installer.HOST_DEPENDENCY_RELATIVE_ROOT
                            / "cmake/data/bin"
                        ).resolve()
                    ),
                    str(
                        (
                            prefix / installer.HOST_DEPENDENCY_RELATIVE_ROOT / "bin"
                        ).resolve()
                    ),
                    str(old_sdk_bin.resolve()),
                    "/usr/bin",
                    "/bin",
                ],
            )
            self.assertEqual(sdk_home, "unset")

    def test_powershell_configuration_never_rebuilds_process_path(self):
        block = installer.shell_configuration_block(
            Path("C:/Users/test/AppData/Local/ECOS/SDKs/3.0.0"),
            "powershell",
        )

        self.assertNotIn("GetEnvironmentVariable", block)
        self.assertIn("$ecosManagedPaths + $ecosCurrentPaths", block)
        self.assertIn("$ecosHostBin", block)

    @unittest.skipUnless(
        os.name == "nt" and shutil.which("powershell"),
        "Windows PowerShell is required",
    )
    def test_powershell_configuration_preserves_process_path_idempotently(self):
        powershell = shutil.which("powershell")
        assert powershell is not None
        with tempfile.TemporaryDirectory() as directory:
            prefix = Path(directory) / "sdk"
            completion_file = (
                prefix
                / installer.COMPLETION_RELATIVE_ROOT
                / installer.completion.COMPLETION_FILENAMES["powershell"]
            )
            completion_file.parent.mkdir(parents=True)
            completion_file.write_text("", encoding="utf-8")
            block = installer.shell_configuration_block(prefix, "powershell")
            sdk_bin = str((prefix / "bin").resolve())
            host_bin = str(installer.dependencies.host_dependency_bin(prefix).resolve())
            cmake_bin = str(
                (
                    prefix
                    / installer.HOST_DEPENDENCY_RELATIVE_ROOT
                    / "cmake/data/bin"
                ).resolve()
            )

            for original_path in (
                r"C:\host-injected\python-codex",
                r"C:\Python;C:\Users\test\AppData\Roaming\npm;C:\Windows\System32",
            ):
                with self.subTest(original_path=original_path):
                    command = "\n".join(
                        [
                            f"$env:PATH = {installer._powershell_quote(original_path)}",
                            block,
                            block,
                            "[Console]::Write($env:PATH)",
                        ]
                    )
                    result = subprocess.run(
                        [powershell, "-NoProfile", "-Command", command],
                        check=True,
                        capture_output=True,
                        text=True,
                    )

                    self.assertEqual(
                        result.stdout,
                        f"{sdk_bin};{cmake_bin};{host_bin};{original_path}",
                    )

    def test_powershell_environment_commands_preserve_process_path(self):
        commands = installer.environment_commands(
            Path("C:/Users/test/AppData/Local/ECOS/SDKs/3.0.0"),
            "powershell",
        )
        rendered = "\n".join(commands)

        self.assertIn("[IO.Path]::PathSeparator", rendered)
        self.assertIn("$ecosManagedPaths + $ecosCurrentPaths", rendered)
        self.assertNotIn("GetEnvironmentVariable", rendered)

    def test_full_install_configures_requested_shell_profile(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk_root = root / "source"
            install_base = root / "installed"
            prefix = install_base / "3.0.0"
            profile = root / "profile" / ".bashrc"
            create_sdk_fixture(sdk_root)
            output = StringIO()

            with patch.object(
                installer,
                "install_python_dependencies",
                side_effect=fake_python_dependency_install,
            ), patch.object(
                installer,
                "install_host_dependencies",
                side_effect=fake_host_dependency_install,
            ), redirect_stdout(output):
                result = installer.main(
                    [
                        "--prefix",
                        str(install_base),
                        "--host-prefix",
                        str(root / "host"),
                        "--skip-toolchain", "--skip-wlink",
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
            self.assertEqual(
                payload["data"]["python_dependencies"]["installation"]["state"],
                "installed",
            )
            self.assertEqual(
                payload["data"]["host_dependencies"]["installation"]["state"],
                "installed",
            )
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
                    (root / "host").joinpath("share/ecos/completions", filename).is_file()
                )
            self.assertFalse((prefix / "bin").exists())
            self.assertTrue((root / "host/bin/ecos").is_file())

    def test_two_versions_install_side_by_side_under_one_base(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            install_base = root / "SDKs"
            registry_path = root / "sdks.json"
            first_source = root / "source-3.0.0"
            second_source = root / "source-3.1.0"
            create_sdk_fixture(first_source, "3.0.0")
            create_sdk_fixture(second_source, "3.1.0")

            with patch.object(
                installer,
                "install_python_dependencies",
                side_effect=fake_python_dependency_install,
            ), patch.object(
                installer,
                "install_host_dependencies",
                side_effect=fake_host_dependency_install,
            ):
                for source in (first_source, second_source):
                    with redirect_stdout(StringIO()), redirect_stderr(StringIO()):
                        result = installer.main(
                            [
                                "--prefix",
                                str(install_base),
                                "--host-prefix",
                                str(root / "host"),
                                "--skip-toolchain", "--skip-wlink",
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
            self.assertEqual(
                registry["sdks"]["3.0.0"]["root"],
                str((install_base / "3.0.0").resolve()),
            )
            self.assertEqual(
                registry["sdks"]["3.1.0"]["root"],
                str((install_base / "3.1.0").resolve()),
            )
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
                "--host-prefix",
                str(root / "host"),
                "--skip-toolchain", "--skip-wlink",
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
            blocked_payload = json.loads(blocked_output.getvalue())
            self.assertEqual(
                blocked_payload["diagnostics"][0]["code"],
                "ECOS_SDK_REGISTRATION_CONFLICT",
            )
            self.assertIn(
                "--replace-registration",
                blocked_payload["diagnostics"][0]["suggestion"],
            )

            output = StringIO()
            with patch.object(
                installer,
                "install_python_dependencies",
                side_effect=fake_python_dependency_install,
            ), patch.object(
                installer,
                "install_host_dependencies",
                side_effect=fake_host_dependency_install,
            ), redirect_stdout(output):
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
                        "--host-prefix",
                        str(root / "host"),
                        "--skip-toolchain", "--skip-wlink",
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
                        "--host-prefix",
                        str(root / "host"),
                        "--skip-toolchain", "--skip-wlink",
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
                        "--host-prefix",
                        str(root / "host"),
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
                payload["data"]["wlink"]["selection"]["release"],
                "0.1.3",
            )
            self.assertEqual(
                payload["data"]["prefix"], str((prefix / "3.0.0").resolve())
            )
            self.assertFalse(prefix.exists())

    def test_core_install_is_staged_and_keeps_host_cli_separate(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk_root = root / "source"
            prefix = root / "installed"
            host_prefix = root / "host"
            create_sdk_fixture(sdk_root)

            result = installer.install_sdk_core(
                sdk_root, prefix, host_prefix=host_prefix
            )
            installer.install_host_cli(sdk_root, host_prefix)

            self.assertEqual(result["prefix"], str(prefix.resolve()))
            self.assertEqual(result["sdk_installation"]["state"], "installed")
            self.assertEqual(
                (prefix / "components" / "source.txt").read_text(encoding="utf-8"),
                "components",
            )
            self.assertFalse(prefix.joinpath("boards/UserBSP").exists())
            self.assertFalse(prefix.joinpath("tools/ecos_cli/build").exists())
            self.assertFalse(prefix.joinpath("tools/fixdep").exists())
            self.assertFalse(prefix.joinpath("tools/kconfig").exists())
            self.assertFalse(prefix.joinpath("tools/scripts").exists())
            self.assertTrue(host_prefix.joinpath("bin/ecos").is_file())
            self.assertTrue(host_prefix.joinpath("bin/ecos.cmd").is_file())
            launch = json.loads(
                host_prefix.joinpath(installer.HOST_LAUNCH_RELATIVE_PATH).read_text(
                    encoding="utf-8"
                )
            )
            self.assertEqual(launch["schema_version"], 1)
            self.assertEqual(launch["script"], str((host_prefix / "bin/ecos").resolve()))
            self.assertTrue(prefix.joinpath("tools/sdk-manifest.json").is_file())
            self.assertTrue(prefix.joinpath(installer.INSTALL_MARKER).is_file())
            self.assertFalse(prefix.joinpath("bin").exists())
            self.assertTrue(
                host_prefix.joinpath("share/ecos/completions/ecos.bash").is_file()
            )
            self.assertIn(
                "Installed ECOS Python CLI launcher",
                host_prefix.joinpath("bin/ecos").read_text(encoding="utf-8"),
            )
            self.assertIn(
                'DEPENDENCY_ROOT = HOST_ROOT / "lib" / "ecos" / "python"',
                host_prefix.joinpath("bin/ecos").read_text(encoding="utf-8"),
            )

            repeated = installer.install_sdk_core(
                sdk_root, prefix, host_prefix=host_prefix
            )
            self.assertEqual(repeated["sdk_installation"]["state"], "reused")
            (sdk_root / "components/source.txt").write_text("changed", encoding="utf-8")
            with self.assertRaisesRegex(installer.InstallerError, "different content"):
                installer.install_sdk_core(sdk_root, prefix, host_prefix=host_prefix)
            self.assertEqual(
                (prefix / "components/source.txt").read_text(encoding="utf-8"),
                "components",
            )

    def test_old_user_bsp_is_migrated_before_legacy_install_conflict(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk_root = root / "source"
            prefix = root / "installed"
            host_prefix = root / "host"
            create_sdk_fixture(sdk_root)
            user_bsp = prefix / "boards" / "UserBSP"
            shared_user_bsp = root / "user-boards"
            user_bsp.mkdir(parents=True)
            (user_bsp / "user-board.txt").write_text("user", encoding="utf-8")

            with patch.dict(os.environ, {"ECOS_USER_BSP_ROOT": str(shared_user_bsp)}):
                with self.assertRaisesRegex(installer.InstallerError, "no valid installation marker"):
                    installer.install_sdk_core(sdk_root, prefix, host_prefix=host_prefix)

            migrated = shared_user_bsp / "user-board.txt"
            self.assertEqual(migrated.read_text(encoding="utf-8"), "user")
            self.assertEqual(
                (user_bsp / "user-board.txt").read_text(encoding="utf-8"), "user"
            )

    def test_release_uninstall_previews_and_removes_only_managed_version(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "source"
            installed = root / "SDKs" / "3.0.0"
            other_version = root / "SDKs" / "3.1.0"
            create_sdk_fixture(source)
            installation = installer.install_sdk_core(
                source, installed, host_prefix=root / "host"
            )
            other_version.mkdir()
            (other_version / "keep.txt").write_text("keep", encoding="utf-8")
            registry = SdkRegistry(root / "sdks.json")
            registry.register(installed, kind="release", activate=True)
            self.assertEqual(
                registry.registrations()["3.0.0"]["digest"],
                installation["sdk_installation"]["digest"],
            )

            with patch.dict(os.environ, {"ECOS_SDK_REGISTRY": str(registry.path)}):
                preview_output = StringIO()
                with redirect_stdout(preview_output):
                    preview_code = cli_main(
                        ["sdk", "uninstall", "3.0.0", "--dry-run", "--format", "json"]
                    )
                self.assertEqual(preview_code, 0)
                preview = json.loads(preview_output.getvalue())["data"]
                self.assertTrue(preview["dry_run"])
                self.assertFalse(preview["changed"])
                self.assertEqual(preview["root"], str(installed.resolve()))
                self.assertTrue(installed.is_dir())
                self.assertEqual(registry.active_name(), "3.0.0")

                output = StringIO()
                with redirect_stdout(output):
                    code = cli_main(["sdk", "uninstall", "3.0.0", "--format", "json"])

            self.assertEqual(code, 0, output.getvalue())
            self.assertFalse(installed.exists())
            self.assertTrue((other_version / "keep.txt").is_file())
            self.assertEqual(registry.registrations(), {})
            self.assertIsNone(registry.active_name())

    def test_release_uninstall_rejects_modified_content_and_checkout(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "source"
            installed = root / "SDKs" / "3.0.0"
            create_sdk_fixture(source)
            installer.install_sdk_core(source, installed, host_prefix=root / "host")
            registry = SdkRegistry(root / "sdks.json")
            registry.register(installed, kind="release", name="release")
            registry.register(source, kind="checkout", name="dev")
            (installed / "components/source.txt").write_text("modified", encoding="utf-8")

            with self.assertRaisesRegex(installer.InstallerError, "content has changed"):
                installer.uninstall_sdk_release(registry, "release")
            with self.assertRaisesRegex(installer.InstallerError, "not a release"):
                installer.uninstall_sdk_release(registry, "dev")
            self.assertTrue(installed.is_dir())
            self.assertEqual(set(registry.registrations()), {"release", "dev"})

    def test_registry_rejects_same_release_version_with_different_digest(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            first_source = root / "first-source"
            second_source = root / "second-source"
            create_sdk_fixture(first_source)
            create_sdk_fixture(second_source)
            (second_source / "components/source.txt").write_text(
                "different", encoding="utf-8"
            )
            first = root / "first" / "3.0.0"
            second = root / "second" / "3.0.0"
            installer.install_sdk_core(first_source, first, host_prefix=root / "host")
            installer.install_sdk_core(second_source, second, host_prefix=root / "host")
            registry = SdkRegistry(root / "sdks.json")
            registry.register(first, name="first", kind="release")

            with self.assertRaisesRegex(
                installer.SdkRegistrationConflict, "different digest"
            ):
                registry.register(second, name="second", kind="release")
            self.assertEqual(set(registry.registrations()), {"first"})

    def test_installs_private_python_dependencies_atomically_and_reuses_them(self):
        with tempfile.TemporaryDirectory() as directory:
            prefix = Path(directory) / "sdk"

            def install_fake_yaml(destination: Path) -> None:
                package = destination / "yaml"
                package.mkdir(parents=True)
                (package / "__init__.py").write_text(
                    "__version__ = '6.0.3'\n", encoding="utf-8"
                )
                (destination / "kconfiglib.py").write_text("", encoding="utf-8")
                (destination / "menuconfig.py").write_text("", encoding="utf-8")
                (destination / "jsonschema.py").write_text("", encoding="utf-8")
                serial_package = destination / "serial"
                serial_package.mkdir()
                (serial_package / "__init__.py").write_text(
                    "__version__ = '3.5'\n", encoding="utf-8"
                )

            with patch.object(
                installer,
                "_install_python_packages",
                side_effect=install_fake_yaml,
            ) as install_packages:
                first = installer.install_python_dependencies(prefix)
                second = installer.install_python_dependencies(prefix)

            dependency_root = prefix / installer.PYTHON_DEPENDENCY_RELATIVE_ROOT
            self.assertEqual(first["state"], "installed")
            self.assertEqual(second["state"], "reused")
            self.assertEqual(install_packages.call_count, 1)
            self.assertTrue(dependency_root.joinpath("yaml/__init__.py").is_file())
            self.assertTrue(
                dependency_root.joinpath(
                    installer.PYTHON_DEPENDENCY_MARKER
                ).is_file()
            )

    def test_installs_private_host_dependencies_atomically_and_reuses_them(self):
        with tempfile.TemporaryDirectory() as directory:
            prefix = Path(directory) / "sdk"

            def install_fake_host_tools(destination: Path) -> None:
                cmake = destination / "cmake/data/bin/cmake"
                script_dir = "Scripts" if sys.platform == "win32" else "bin"
                ninja_name = "ninja.exe" if sys.platform == "win32" else "ninja"
                ninja = destination / script_dir / ninja_name
                cmake.parent.mkdir(parents=True)
                ninja.parent.mkdir(parents=True)
                cmake.write_text("cmake version 3.31.10\n", encoding="utf-8")
                ninja.write_text("1.11.1\n", encoding="utf-8")

            with patch.object(
                installer,
                "_install_host_packages",
                side_effect=install_fake_host_tools,
            ) as install_packages, patch.object(
                installer.dependencies,
                "probe_host_tool",
                return_value={"valid": True},
            ):
                first = installer.install_host_dependencies(prefix)
                second = installer.install_host_dependencies(prefix)

            dependency_root = prefix / installer.HOST_DEPENDENCY_RELATIVE_ROOT
            self.assertEqual(first["state"], "installed")
            self.assertEqual(second["state"], "reused")
            self.assertEqual(install_packages.call_count, 1)
            self.assertTrue(
                dependency_root.joinpath("cmake/data/bin/cmake").is_file()
            )
            self.assertTrue(
                dependency_root.joinpath(installer.HOST_DEPENDENCY_MARKER).is_file()
            )
            self.assertEqual(set(first["tools"]), {"cmake", "ninja"})

    def test_rejects_overlapping_install_prefix(self):
        with tempfile.TemporaryDirectory() as directory:
            sdk_root = Path(directory) / "source"
            create_sdk_fixture(sdk_root)

            with self.assertRaises(installer.InstallerError):
                installer.validate_layout(sdk_root, sdk_root / "installed")


if __name__ == "__main__":
    unittest.main()
