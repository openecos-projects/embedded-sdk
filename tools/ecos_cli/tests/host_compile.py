"""Compiler flags shared by host-side C integration tests."""

from __future__ import annotations

import sys


def compiler_flags(standard: str, *, strict: bool = False) -> list[str]:
    flags = [f"-std={standard}"]
    if sys.platform == "win32":
        flags.append("-mno-ms-bitfields")
    if strict:
        flags.extend(["-Wall", "-Wextra"])
        if sys.platform.startswith("linux"):
            flags.extend(["-Werror", "-pedantic"])
    return flags


def linker_flags() -> list[str]:
    if sys.platform.startswith("linux"):
        return ["-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections"]
    if sys.platform == "darwin":
        return ["-ffunction-sections", "-fdata-sections", "-Wl,-dead_strip"]
    if sys.platform == "win32":
        return ["-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections"]
    return []
