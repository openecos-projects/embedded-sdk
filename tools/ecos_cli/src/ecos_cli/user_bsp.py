"""User-owned BSP storage independent of SDK version directories."""

from __future__ import annotations

import os
import sys
from pathlib import Path
from typing import Mapping, Optional


def default_root(
    *,
    environ: Optional[Mapping[str, str]] = None,
    home: Optional[Path] = None,
    platform_name: Optional[str] = None,
) -> Path:
    env = os.environ if environ is None else environ
    override = env.get("ECOS_USER_BSP_ROOT")
    if override:
        return Path(override).expanduser().resolve()
    home_root = (home or Path.home()).expanduser().resolve()
    host = platform_name or sys.platform
    if host == "win32":
        base = Path(env.get("LOCALAPPDATA", home_root / "AppData" / "Local"))
        return base / "ECOS" / "boards" / "UserBSP"
    if host == "darwin":
        return home_root / "Library" / "Application Support" / "ECOS" / "boards" / "UserBSP"
    base = Path(env.get("XDG_DATA_HOME", home_root / ".local" / "share"))
    return base / "ecos" / "boards" / "UserBSP"
