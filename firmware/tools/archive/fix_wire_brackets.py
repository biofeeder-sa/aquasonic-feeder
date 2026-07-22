#!/usr/bin/env python3
"""Fix VAR_WIRE_BYTE(row, idx] -> VAR_WIRE_BYTE(row, idx) after migrate_wire_vars.py."""

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SKIP = {"tools", "registry", "lib"}


def fix(text: str) -> str:
    return re.sub(
        r"VAR_WIRE_BYTE\((VAR_[A-Z0-9_]+),\s*([^\]\)]+)\]",
        r"VAR_WIRE_BYTE(\1, \2)",
        text,
    )


def main():
    for path in sorted(ROOT.rglob("*")):
        if path.suffix not in {".cpp", ".h", ".ino"}:
            continue
        if any(part in SKIP for part in path.parts):
            continue
        original = path.read_text(encoding="utf-8")
        updated = fix(original)
        if updated != original:
            path.write_text(updated, encoding="utf-8")
            print(f"fixed: {path.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
