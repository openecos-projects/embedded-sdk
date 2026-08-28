#!/usr/bin/env python3
"""Compatibility entry point for the Python ECOS toolchain installer."""

import sys
from pathlib import Path


SDK_ROOT = Path(__file__).resolve().parents[2]
SOURCE_ROOT = SDK_ROOT / "tools" / "ecos_cli" / "src"
if str(SOURCE_ROOT) not in sys.path:
    sys.path.insert(0, str(SOURCE_ROOT))

from ecos_cli.cli import main  # noqa: E402


if __name__ == "__main__":
    raise SystemExit(main(["toolchain", "install", *sys.argv[1:]]))
