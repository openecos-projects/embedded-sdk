#!/usr/bin/env python3
"""Install the ECOS Embedded SDK from a source checkout."""

import sys
from pathlib import Path


TOOLS_ROOT = Path(__file__).resolve().parent
SDK_ROOT = TOOLS_ROOT.parent
SOURCE_ROOT = TOOLS_ROOT / "ecos_cli" / "src"
if str(SOURCE_ROOT) not in sys.path:
    sys.path.insert(0, str(SOURCE_ROOT))

from ecos_cli.installer import main  # noqa: E402


if __name__ == "__main__":
    raise SystemExit(main(sdk_root=SDK_ROOT))
