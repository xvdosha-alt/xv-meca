#!/usr/bin/env python3
"""Strip GCC-incompatible enum sentinel values from Dumper-7 SDK headers."""

from __future__ import annotations

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
SDK_DIR = ROOT / "MecchaCheatV" / "main" / "sdk" / "SDK"

PATTERNS = [
    re.compile(r"^\s+\w+_MAX\s*=\s*256,\s*\n", re.MULTILINE),
    re.compile(r"^\s+\w+_MAX\s*=\s*4294967296,\s*\n", re.MULTILINE),
    re.compile(r"^\s+Invalid\s*=\s*18446744073709551615,\s*\n", re.MULTILINE),
    re.compile(r"^\s+None\s*=\s*18446744073709551615,\s*\n", re.MULTILINE),
]

def patch_file(path: pathlib.Path) -> int:
    original = path.read_text(encoding="utf-8")
    updated = original
    removed = 0

    for pattern in PATTERNS:
        updated, count = pattern.subn("", updated)
        removed += count

    if removed and updated != original:
        path.write_text(updated, encoding="utf-8")

    return removed

def main() -> int:
    if not SDK_DIR.is_dir():
        print(f"error: SDK directory not found: {SDK_DIR}", file=sys.stderr)
        return 1

    total = 0
    for header in sorted(SDK_DIR.glob("*_structs.hpp")):
        total += patch_file(header)

    print(f"patched {total} enum sentinel lines in {SDK_DIR}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
