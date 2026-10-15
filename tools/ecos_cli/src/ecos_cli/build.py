"""Project build dispatch for Target/SoC implementations."""

from __future__ import annotations

import os
import shutil
import subprocess
from pathlib import Path
from typing import Any, Optional, Union

from . import artifacts
from . import configuration
from . import dependencies
from . import project
from . import toolchain
from .sdk_context import SdkContext


class BuildError(RuntimeError):
    """Base error for project builds."""


class BuildConfigurationError(BuildError):
    """The project does not describe a buildable hardware selection."""


class BuildToolNotFound(BuildError):
    """A required host build tool is unavailable."""


class BuildCommandError(BuildError):
    """The Target build command failed."""

    def __init__(self, returncode: int) -> None:
        super().__init__(f"Target build failed with exit code {returncode}")
        self.returncode = returncode


class BuildOutputError(BuildError):
    """The Target build did not produce its required firmware outputs."""


def clean_project(
    *, project_root: Optional[Path] = None, dry_run: bool = False
) -> dict[str, Any]:
    root = (project_root or Path.cwd()).expanduser().resolve()
    _, metadata = project.load_project_metadata(root)
    build_dir = root / "build"
    exists = build_dir.exists() or build_dir.is_symlink()
    if exists:
        if build_dir.is_symlink() or not build_dir.is_dir():
            raise BuildConfigurationError(
                f"build path is not a regular directory: {build_dir}"
            )
        cache = build_dir / "CMakeCache.txt"
        try:
            lines = cache.read_text(encoding="utf-8").splitlines()
        except OSError as exc:
            raise BuildConfigurationError(
                f"cannot verify that {build_dir} belongs to this ECOS project: {exc}"
            ) from exc
        values: dict[str, str] = {}
        for line in lines:
            if line.startswith(("CMAKE_HOME_DIRECTORY:", "ECOS_PROJECT_CONFIG:")):
                key, value = line.split("=", 1)
                values[key.split(":", 1)[0]] = value
        expected_config = (
            root
            / configuration.GENERATED_DIRECTORY
            / configuration.RESOLVED_CMAKE_FILE
        ).resolve()
        if (
            not values.get("CMAKE_HOME_DIRECTORY")
            or Path(values["CMAKE_HOME_DIRECTORY"]).resolve() != root
            or not values.get("ECOS_PROJECT_CONFIG")
            or Path(values["ECOS_PROJECT_CONFIG"]).resolve() != expected_config
        ):
            raise BuildConfigurationError(
                f"refusing to remove an unrecognized build directory: {build_dir}"
            )
        if not dry_run:
            try:
                shutil.rmtree(build_dir)
            except OSError as exc:
                raise BuildOutputError(f"cannot remove {build_dir}: {exc}") from exc
    return {
        "path": str(root),
        "board": metadata.get("board"),
        "target": metadata.get("target"),
        "clean": True,
        "dry_run": dry_run,
        "removed": exists and not dry_run,
        "paths": [str(build_dir)] if exists else [],
        "outputs": {},
    }


def _cmake_path(value: Union[str, Path]) -> str:
    """Return a path safe for use in a CMake cache definition."""
    return str(value).replace("\\", "/")


def _resolve_host_tool(context: SdkContext, item: dict[str, str]) -> str:
    managed = dependencies.managed_host_tool(
        dependencies.host_dependency_root(dependencies.default_host_prefix()),
        item["name"],
    )
    if managed is not None:
        return str(managed)
    found = shutil.which(item["executable"]) or shutil.which(
        f"{item['executable']}.exe"
    )
    if found is None:
        raise BuildToolNotFound(
            f"required build dependency {item['name']} is unavailable; "
            "run 'tools/install.py' to install the SDK dependencies"
        )
    return found


def build_project(
    context: SdkContext,
    *,
    project_root: Optional[Path] = None,
    clean: bool = False,
    json_output: bool = False,
) -> dict[str, Any]:
    root = (project_root or Path.cwd()).expanduser().resolve()
    _, metadata = project.load_project_metadata(root)
    target = metadata.get("target")
    board = metadata.get("board")

    build_dir = root / "build"
    if clean:
        return clean_project(project_root=root)

    try:
        configured = configuration.configure_project(context, project_root=root)
        resolved = configuration.load_resolved_project(root)
    except (configuration.ConfigurationError, configuration.ProjectModelError) as exc:
        raise BuildConfigurationError(str(exc)) from exc
    target = resolved["project"]["target"]
    board = resolved["project"]["board"]

    host_tools = {
        item["name"]: _resolve_host_tool(context, item)
        for item in dependencies.HOST_TOOL_DEPENDENCIES
    }
    cmake = host_tools["cmake"]
    ninja = host_tools["ninja"]

    target_root = Path(resolved["target"]["root"])
    build_file = Path(resolved["build"]["cmake"])
    toolchain_file = Path(resolved["build"]["toolchain_file"])
    if not build_file.is_file() or not toolchain_file.is_file():
        raise BuildConfigurationError(
            f"Target does not provide CMake build rules: {build_file}"
        )
    environment = os.environ.copy()
    environment["ECOS_SDK_HOME"] = str(context.root)
    host_paths = [
        path
        for path in dependencies.host_dependency_paths(dependencies.default_host_prefix())
        if path.is_dir()
    ]
    if host_paths:
        current_path = environment.get("PATH", "")
        environment["PATH"] = os.pathsep.join(
            value
            for value in ([str(path) for path in host_paths] + [current_path])
            if value
        )

    toolchain_manifest = toolchain.load_manifest()
    requirement = resolved["toolchain"]
    if (
        toolchain_manifest["id"] != requirement["id"]
        or toolchain_manifest["release"] != requirement["release"]
        or (
            requirement["prefix"] is not None
            and toolchain_manifest["tool_prefix"] != requirement["prefix"]
        )
        or (
            requirement["triple"] is not None
            and toolchain_manifest["target_triplet"] != requirement["triple"]
        )
    ):
        raise BuildConfigurationError(
            "the CLI toolchain manifest does not match the resolved project: "
            f"expected {requirement['id']} {requirement['release']}"
        )
    host = toolchain.detect_host()
    toolchain_prefix = toolchain.default_prefix()
    legacy_roots = (toolchain.legacy_prefix(),)
    if context.kind == "release":
        legacy_roots += (context.root,)
    status = toolchain.compatible_installation_status(
        toolchain_manifest, toolchain_prefix, host, legacy_roots=legacy_roots
    )
    if status["state"] != "installed":
        raise BuildConfigurationError(
            "the SDK toolchain is not installed or is invalid "
            f"(state: {status['state']}, path: {status.get('root', toolchain_prefix)}); "
            "run 'ecos toolchain install' first"
        )
    toolchain_root = Path(status["active_root"])
    compiler = toolchain.compiler_path(toolchain_root, toolchain_manifest, host)
    if not compiler.is_file():
        raise BuildConfigurationError(
            f"active SDK toolchain compiler does not exist: {compiler}"
        )

    project_config = (
        root
        / configuration.GENERATED_DIRECTORY
        / configuration.RESOLVED_CMAKE_FILE
    )
    configure_command = [
        cmake,
        "-S",
        str(build_file.parent),
        "-B",
        str(build_dir),
        "-G",
        "Ninja",
        f"-DCMAKE_MAKE_PROGRAM={_cmake_path(ninja)}",
        f"-DCMAKE_TOOLCHAIN_FILE={_cmake_path(toolchain_file)}",
        f"-DECOS_TOOLCHAIN_ROOT={_cmake_path(toolchain_root)}",
        f"-DECOS_TARGET_DIR={_cmake_path(target_root)}",
        f"-DECOS_PROJECT_CONFIG={_cmake_path(project_config)}",
        "-DFIRMWARE_NAME=retrosoc_fw",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
    ]
    result = subprocess.run(
        configure_command, cwd=root, env=environment, check=False,
        **({"stdout": 2} if json_output else {}),
    )
    if result.returncode != 0:
        raise BuildCommandError(result.returncode)

    result = subprocess.run(
        [cmake, "--build", str(build_dir), "--parallel"],
        cwd=root,
        env=environment,
        check=False,
        **({"stdout": 2} if json_output else {}),
    )
    if result.returncode != 0:
        raise BuildCommandError(result.returncode)

    try:
        artifact_manifest = artifacts.create_manifest(root, resolved)
    except artifacts.ArtifactError as exc:
        raise BuildOutputError(str(exc)) from exc
    outputs = {
        name: str(root / item["path"])
        for name, item in artifact_manifest["files"].items()
    }
    outputs["artifacts"] = artifact_manifest["manifest"]
    return {
        "path": str(root),
        "board": board,
        "target": target,
        "clean": clean,
        "host_tools": host_tools,
        "outputs": outputs,
        "source_fingerprint": resolved["source_fingerprint"],
        "configuration_fingerprint": resolved["configuration"]["fingerprint"],
        "reconfigured": configured.data["changed"],
    }
