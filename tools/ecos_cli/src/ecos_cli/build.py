"""Project build dispatch for Target/SoC implementations."""

from __future__ import annotations

import os
import shutil
import subprocess
from pathlib import Path
from typing import Any, Optional

from . import project
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

    make = shutil.which("make")
    if make is None:
        raise BuildToolNotFound("required build tool is not available on PATH: make")
    build_file = context.resource("components") / "soc" / target / "build.mk"
    if not build_file.is_file():
        raise BuildConfigurationError(
            f"Target does not provide build rules: {build_file}"
        )
    source = root / "main.c"
    if not clean and not source.is_file():
        raise BuildConfigurationError(
            f"project application source does not exist: {source}"
        )

    command = [
        make,
        "-f",
        str(build_file),
        f"PROJECT_DIR={root}",
        f"ECOS_SDK_HOME={context.root}",
    ]
    if clean:
        command.append("clean")
    environment = os.environ.copy()
    environment["ECOS_SDK_HOME"] = str(context.root)
    result = subprocess.run(command, cwd=root, env=environment, check=False)
    if result.returncode != 0:
        raise BuildCommandError(result.returncode)

    build_dir = root / "build"
    outputs = {
        suffix: str(build_dir / f"retrosoc_fw.{suffix}")
        for suffix in ("elf", "bin", "txt", "hex")
        if (build_dir / f"retrosoc_fw.{suffix}").is_file()
    }
    if not clean:
        missing = sorted({"elf", "bin", "txt"}.difference(outputs))
        if missing:
            raise BuildOutputError(
                f"Target build did not produce required outputs: {', '.join(missing)}"
            )
    return {
        "path": str(root),
        "board": board,
        "target": target,
        "clean": clean,
        "outputs": outputs,
    }
