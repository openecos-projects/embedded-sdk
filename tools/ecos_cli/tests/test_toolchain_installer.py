import hashlib
import io
import json
import os
import sys
import tarfile
import tempfile
import unittest
import zipfile
from contextlib import redirect_stderr, redirect_stdout
from io import StringIO
from pathlib import Path
from unittest import mock


SOURCE_ROOT = Path(__file__).parents[1] / "src"
sys.path.insert(0, str(SOURCE_ROOT))

from ecos_cli import toolchain as installer  # noqa: E402
from ecos_cli.cli import ExitCode, main  # noqa: E402
from ecos_cli.progress import ConsoleProgress, DownloadStatus  # noqa: E402


class ToolchainInstallerTest(unittest.TestCase):
    def setUp(self):
        environment = mock.patch.dict(os.environ, {"ECOS_SDK_HOME": ""})
        environment.start()
        self.addCleanup(environment.stop)

    def test_download_reports_size_and_completes_atomically(self):
        payload = b"x" * (2 * 1024 * 1024 + 17)
        response = io.BytesIO(payload)
        response.headers = {"Content-Length": str(len(payload))}
        updates = []

        with tempfile.TemporaryDirectory() as directory, mock.patch(
            "urllib.request.urlopen", return_value=response
        ):
            destination = Path(directory) / "archive.tar.gz"
            installer.download(
                "https://example.invalid/archive.tar.gz",
                destination,
                progress=updates.append,
            )

            self.assertEqual(destination.read_bytes(), payload)
            self.assertFalse(destination.with_name(".archive.tar.gz.part").exists())

        self.assertEqual(updates[0].downloaded, 0)
        self.assertEqual(updates[-1].downloaded, len(payload))
        self.assertEqual(updates[-1].total, len(payload))
        self.assertTrue(updates[-1].done)
        self.assertFalse(updates[-1].failed)

    def test_console_download_progress_shows_percent_speed_and_eta(self):
        output = StringIO()
        console = ConsoleProgress(output)

        console.download(DownloadStatus(5 * 1024 * 1024, 10 * 1024 * 1024, 2.0))
        console.download(
            DownloadStatus(10 * 1024 * 1024, 10 * 1024 * 1024, 4.0, done=True)
        )

        rendered = output.getvalue()
        self.assertIn("【ECOS-INFO】", rendered)
        self.assertIn("50.0%", rendered)
        self.assertIn("MiB/s", rendered)
        self.assertIn("ETA", rendered)
        self.assertIn("100.0%", rendered)

    def test_console_log_levels_keep_prefixes_without_terminal_colors(self):
        output = StringIO()
        console = ConsoleProgress(output)

        console.info("ready")
        console.warning("careful")
        console.error("failed")

        rendered = output.getvalue()
        self.assertIn("【ECOS-INFO】 ready", rendered)
        self.assertIn("【ECOS-WARN】 careful", rendered)
        self.assertIn("【ECOS-ERR】 failed", rendered)
        self.assertNotIn("\033[", rendered)

    def test_console_log_levels_use_colors_on_supported_terminal(self):
        class TerminalOutput(StringIO):
            def isatty(self):
                return True

        output = TerminalOutput()
        with mock.patch.dict(os.environ, {"TERM": "xterm-256color"}):
            os.environ.pop("NO_COLOR", None)
            console = ConsoleProgress(output)
            console.info("ready")
            console.warning("careful")
            console.error("failed")

        rendered = output.getvalue()
        self.assertIn("\033[1;32m【ECOS-INFO】\033[0m", rendered)
        self.assertIn("\033[1;33m【ECOS-WARN】\033[0m", rendered)
        self.assertIn("\033[1;31m【ECOS-ERR】\033[0m", rendered)

    def test_detect_host_aliases(self):
        self.assertEqual(installer.detect_host("Linux", "x86_64"), "linux-x86_64")
        self.assertEqual(installer.detect_host("Linux", "aarch64"), "linux-arm64")
        self.assertEqual(installer.detect_host("Darwin", "arm64"), "macos-arm64")
        self.assertEqual(installer.detect_host("Windows", "AMD64"), "windows-x86_64")

    def test_release_manifest_has_complete_assets(self):
        manifest = installer.load_manifest()
        self.assertEqual(manifest["release"], "15.2.0-1")
        self.assertEqual(manifest["tool_prefix"], "riscv-none-elf-")
        self.assertEqual(
            set(manifest["hosts"]),
            {
                "linux-x86_64",
                "linux-arm64",
                "macos-x86_64",
                "macos-arm64",
                "windows-x86_64",
            },
        )
        for asset in manifest["hosts"].values():
            self.assertEqual(len(asset["sha256"]), 64)

    def test_rejects_archive_path_traversal(self):
        with tempfile.TemporaryDirectory() as directory:
            archive = Path(directory) / "bad.tar.gz"
            payload = b"bad"
            with tarfile.open(archive, "w:gz") as bundle:
                member = tarfile.TarInfo("root/../outside")
                member.size = len(payload)
                bundle.addfile(member, io.BytesIO(payload))
            with self.assertRaises(installer.ToolchainError):
                installer.extract_tar(archive, Path(directory) / "out", "root")

    def test_extracts_zip_with_top_directory_removed(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            archive = root / "fixture.zip"
            with zipfile.ZipFile(archive, "w") as bundle:
                bundle.writestr("fixture/bin/riscv-none-elf-gcc.exe", b"compiler")

            output = root / "out"
            installer.extract_zip(archive, output, "fixture")

            self.assertEqual(
                (output / "bin" / "riscv-none-elf-gcc.exe").read_bytes(),
                b"compiler",
            )

    @unittest.skipIf(os.name == "nt", "fixture uses a POSIX shell script")
    def test_installs_local_archive_and_legacy_alias(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            archive = root / "fixture.tar.gz"
            compiler = (
                b"#!/bin/sh\n"
                b"if [ \"$1\" = \"-dumpmachine\" ]; then\n"
                b"  echo riscv-none-elf\n"
                b"else\n"
                b"  echo 'fixture gcc 1.0.0'\n"
                b"fi\n"
            )
            with tarfile.open(archive, "w:gz") as bundle:
                member = tarfile.TarInfo("fixture/bin/riscv-none-elf-gcc")
                member.mode = 0o755
                member.size = len(compiler)
                bundle.addfile(member, io.BytesIO(compiler))

            digest = hashlib.sha256(archive.read_bytes()).hexdigest()
            manifest = {
                "schema_version": 1,
                "id": "fixture",
                "name": "Fixture",
                "release": "1.0.0",
                "base_url": "https://invalid.example",
                "tool_prefix": "riscv-none-elf-",
                "legacy_tool_prefix": "riscv64-unknown-elf-",
                "compiler": "riscv-none-elf-gcc",
                "target_triplet": "riscv-none-elf",
                "install_path": "toolchain/versions/fixture/1.0.0",
                "active_path": "toolchain/riscv",
                "archive_root": "fixture",
                "hosts": {
                    "linux-x86_64": {
                        "file": archive.name,
                        "archive": "tar.gz",
                        "sha256": digest,
                    }
                },
            }
            manifest_path = root / "manifest.json"
            manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
            loaded = installer.load_manifest(manifest_path)

            result = installer.install_toolchain(
                loaded,
                root / "prefix",
                "linux-x86_64",
                root / "cache",
                archive_override=archive,
            )
            target = Path(result["root"])

            self.assertEqual(result["state"], "installed")
            self.assertTrue(result["changed"])
            self.assertTrue((target / "bin" / "riscv-none-elf-gcc").is_file())
            self.assertTrue((target / "bin" / "riscv64-unknown-elf-gcc").is_symlink())
            self.assertEqual((root / "prefix" / "toolchain" / "riscv").resolve(), target)

            custom_manifest = dict(loaded)
            custom_manifest["release"] = "2.0.0"
            custom = installer.installation_status(
                custom_manifest,
                root / "unused-prefix",
                "linux-x86_64",
                custom=target,
            )
            self.assertEqual(custom["state"], "installed")
            self.assertFalse(custom["compiler"]["matches_release"])

            marker_path = target / ".ecos-toolchain.json"
            marker = json.loads(marker_path.read_text(encoding="utf-8"))
            marker["sha256"] = "0" * 64
            marker_path.write_text(json.dumps(marker), encoding="utf-8")
            invalid = installer.installation_status(
                loaded, root / "prefix", "linux-x86_64"
            )
            self.assertEqual(invalid["state"], "invalid")
            with self.assertRaises(installer.ToolchainError):
                installer.install_toolchain(
                    loaded,
                    root / "prefix",
                    "linux-x86_64",
                    root / "cache",
                    archive_override=archive,
                )
            repaired = installer.install_toolchain(
                loaded,
                root / "prefix",
                "linux-x86_64",
                root / "cache",
                archive_override=archive,
                force=True,
            )
            self.assertEqual(repaired["state"], "installed")

    def test_detect_json_contract(self):
        output = StringIO()
        with redirect_stdout(output):
            result = main(
                [
                    "toolchain",
                    "detect",
                    "--host",
                    "linux-x86_64",
                    "--format",
                    "json",
                ]
            )

        payload = json.loads(output.getvalue())
        self.assertEqual(result, ExitCode.OK)
        self.assertEqual(payload["schema_version"], "1.0")
        self.assertEqual(payload["command"], "toolchain.detect")
        self.assertEqual(payload["status"], "ok")
        self.assertEqual(payload["data"]["release"], "15.2.0-1")
        self.assertEqual(payload["data"]["sdk"]["sdk_version"], "3.0.0")
        self.assertEqual(payload["diagnostics"], [])

    def test_unsupported_host_has_stable_json_diagnostic(self):
        output = StringIO()
        with redirect_stdout(output):
            result = main(
                [
                    "toolchain",
                    "detect",
                    "--host",
                    "linux-riscv64",
                    "--format",
                    "json",
                ]
            )

        payload = json.loads(output.getvalue())
        self.assertEqual(result, ExitCode.UNSUPPORTED)
        self.assertEqual(payload["status"], "error")
        self.assertEqual(
            payload["diagnostics"][0]["code"],
            "ECOS_TOOLCHAIN_UNSUPPORTED_HOST",
        )

    def test_invalid_arguments_preserve_json_contract(self):
        output = StringIO()
        with redirect_stdout(output):
            result = main(
                [
                    "toolchain",
                    "detect",
                    "--format",
                    "json",
                    "--unknown-option",
                ]
            )

        payload = json.loads(output.getvalue())
        self.assertEqual(result, ExitCode.USAGE)
        self.assertEqual(payload["status"], "error")
        self.assertEqual(
            payload["diagnostics"][0]["code"],
            "ECOS_USAGE_INVALID_ARGUMENTS",
        )

    def test_install_dry_run_has_no_side_effects(self):
        with tempfile.TemporaryDirectory() as directory:
            prefix = Path(directory) / "not-created"
            output = StringIO()
            errors = StringIO()
            with redirect_stdout(output), redirect_stderr(errors):
                result = main(
                    [
                        "toolchain",
                        "install",
                        "--host",
                        "linux-x86_64",
                        "--prefix",
                        str(prefix),
                        "--dry-run",
                        "--format",
                        "json",
                    ]
                )

            payload = json.loads(output.getvalue())
            self.assertEqual(result, ExitCode.OK)
            self.assertTrue(payload["data"]["dry_run"])
            self.assertFalse(payload["data"]["changed"])
            self.assertFalse(prefix.exists())
            self.assertEqual(errors.getvalue(), "")


if __name__ == "__main__":
    unittest.main()
