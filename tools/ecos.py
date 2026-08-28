#!/usr/bin/env python3
"""Source-tree and installed-SDK launcher for the ECOS Python CLI."""

import sys
from pathlib import Path


TOOLS_ROOT = Path(__file__).resolve().parent
SOURCE_ROOT = TOOLS_ROOT / "ecos_cli" / "src"
if str(SOURCE_ROOT) not in sys.path:
    sys.path.insert(0, str(SOURCE_ROOT))

from ecos_cli.cli import main  # noqa: E402


if __name__ == "__main__":
    raise SystemExit(main())
