#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

SKIP_PARTS = {
    "libs",
    "sdk",
    "fonts",
}

SKIP_FILES = {
    "embedded_payload.hpp",
    "embedded_payload.cpp",
}

EXTS = {".cpp", ".hpp", ".h", ".c", ".py", ".sh"}

def should_skip(path: Path) -> bool:
    if path.name in SKIP_FILES:
        return True
    return any(part in SKIP_PARTS for part in path.parts)

def strip_c_style(text: str) -> str:
    out: list[str] = []
    i = 0
    n = len(text)
    in_string = False
    string_char = ""
    while i < n:
        ch = text[i]
        nxt = text[i + 1] if i + 1 < n else ""

        if in_string:
            out.append(ch)
            if ch == "\\" and i + 1 < n:
                out.append(text[i + 1])
                i += 2
                continue
            if ch == string_char:
                in_string = False
            i += 1
            continue

        if ch in "\"'":
            in_string = True
            string_char = ch
            out.append(ch)
            i += 1
            continue

        if ch == "/" and nxt == "/":
            while i < n and text[i] != "\n":
                i += 1
            continue

        if ch == "/" and nxt == "*":
            i += 2
            while i < n - 1 and not (text[i] == "*" and text[i + 1] == "/"):
                i += 1
            i = min(i + 2, n)
            continue

        out.append(ch)
        i += 1

    return "".join(out)

def strip_file(path: Path) -> bool:
    original = path.read_text(encoding="utf-8", errors="replace")
    stripped = strip_c_style(original)
    stripped = re.sub(r"\n{3,}", "\n\n", stripped)
    if stripped != original:
        path.write_text(stripped, encoding="utf-8")
        return True
    return False

def main() -> int:
    changed = 0
    for path in ROOT.rglob("*"):
        if not path.is_file() or path.suffix not in EXTS:
            continue
        if should_skip(path):
            continue
        if strip_file(path):
            changed += 1
            print(path.relative_to(ROOT))
    print(f"stripped {changed} files")
    return 0

if __name__ == "__main__":
    sys.exit(main())
