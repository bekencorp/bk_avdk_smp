#!/usr/bin/env python3
"""Extract raw bytes from UART-style 0xNN hex dumps."""
from __future__ import annotations

import re
import sys
from pathlib import Path


TOKEN_RE = re.compile(r"0x([0-9a-fA-F]{2})\b")


def line_to_bytes(line: str):
    tokens = TOKEN_RE.findall(line)
    if not tokens:
        return None
    return bytes(int(token, 16) for token in tokens)


def extract_bytes_from_log(path: Path) -> bytes:
    out = bytearray()
    with path.open("r", encoding="utf-8", errors="replace") as f:
        for line in f:
            b = line_to_bytes(line)
            if b is not None:
                out.extend(b)
    return bytes(out)


def main() -> None:
    if len(sys.argv) < 3:
        print("Usage: hexlog_to_bin.py <input.log> <output.bin>", file=sys.stderr)
        sys.exit(1)
    inp = Path(sys.argv[1])
    outp = Path(sys.argv[2])
    data = extract_bytes_from_log(inp)
    outp.write_bytes(data)
    print(f"{inp.name} -> {outp.name}: {len(data)} bytes")


if __name__ == "__main__":
    main()
