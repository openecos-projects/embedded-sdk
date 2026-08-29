"""Project build dispatch for Target/SoC implementations."""

from __future__ import annotations

import os
import shutil
import subprocess
from pathlib import Path
from typing import Any, Optional

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


def _resolve_host_tool(context: SdkContext, item: dict[str, str]) -> str:
    managed = dependencies.managed_host_tool(
        dependencies.host_dependency_root(context.root), item["name"]
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
) -> dict[str, Any]:
    root = (project_root or Path.cwd()).expanduser().resolve()
    _, metadata = project.load_project_metadata(root)
    target = metadata.get("target")
    board = metadata.get("board")
    if not isinstance(target, str) or not target:
        raise BuildConfigurationError(
            "project has no Target; run 'ecos project set-board BOARD' or "
            "'ecos project set-target TARGET'"
        )
    target = project.resolve_target(context, target)
    if board is not None:
        if not isinstance(board, str):
            raise BuildConfigurationError("project Board must be a string or null")
        board_id, board_target = project.resolve_board(context, board)
        if board_id != board or board_target != target:
            raise BuildConfigurationError(
                f"project Board {board!r} maps to {board_target!r}, not {target!r}"
            )

    build_dir = root / "build"
    if clean:
        if build_dir.exists():
            shutil.rmtree(build_dir)
        return {
            "path": str(root),
            "board": board,
            "target": target,
            "clean": True,
            "outputs": {},
        }

    host_tools = {
        item["name"]: _resolve_host_tool(context, item)
        for item in dependencies.HOST_TOOL_DEPENDENCIES
    }
    cmake = host_tools["cmake"]
    ninja = host_tools["ninja"]

    target_root = context.resource("components") / "soc" / target
    project_build_file = root / "CMakeLists.txt"
    build_file = (
        project_build_file
        if project_build_file.is_file()
        else target_root / "CMakeLists.txt"
    )
    toolchain_file = target_root / "toolchain.cmake"
    if not build_file.is_file() or not toolchain_file.is_file():
        raise BuildConfigurationError(
            f"Target does not provide CMake build rules: {build_file}"
        )
    source = root / "main.c"
    if not source.is_file():
        raise BuildConfigurationError(
            f"project application source does not exist: {source}"
        )

    environment = os.environ.copy()
    environment["ECOS_SDK_HOME"] = str(context.root)
    host_paths = [
        path
        for path in dependencies.host_dependency_paths(context.root)
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
    host = toolchain.detect_host()
    # A source checkout keeps downloaded toolchains in the user-level prefix;
    # an installed SDK release keeps them beside its own manifest and assets.
    toolchain_prefix = (
        context.root
        if context.kind == "release"
        else toolchain.default_prefix()
    )
    status = toolchain.installation_status(
        toolchain_manifest, toolchain_prefix, host
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

    configure_command = [
        cmake,
        "-S",
        str(build_file.parent),
        "-B",
        str(build_dir),
        "-G",
        "Ninja",
        f"-DCMAKE_MAKE_PROGRAM={ninja}",
        f"-DCMAKE_TOOLCHAIN_FILE={toolchain_file}",
        f"-DECOS_TOOLCHAIN_ROOT={toolchain_root}",
        f"-DECOS_TARGET_DIR={target_root}",
        f"-DPROJECT_DIR={root}",
        "-DFIRMWARE_NAME=retrosoc_fw",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
    ]
    result = subprocess.run(
        configure_command, cwd=root, env=environment, check=False
    )
    if result.returncode != 0:
        raise BuildCommandError(result.returncode)

    result = subprocess.run(
        [cmake, "--build", str(build_dir), "--parallel"],
        cwd=root,
        env=environment,
        check=False,
    )
    if result.returncode != 0:
        raise BuildCommandError(result.returncode)

    outputs = {
        suffix: str(build_dir / f"retrosoc_fw.{suffix}")
        for suffix in ("elf", "bin", "txt", "hex", "map", "size")
        if (build_dir / f"retrosoc_fw.{suffix}").is_file()
    }
    compile_commands = build_dir / "compile_commands.json"
    if compile_commands.is_file():
        outputs["compile_commands"] = str(compile_commands)
    if not clean:
        missing = sorted(
            {"elf", "bin", "txt", "map", "size", "compile_commands"}.difference(outputs)
        )
        if missing:
            raise BuildOutputError(
                f"Target build did not produce required outputs: {', '.join(missing)}"
            )
    return {
        "path": str(root),
        "board": board,
        "target": target,
        "clean": clean,
        "host_tools": host_tools,
        "outputs": outputs,
    }
