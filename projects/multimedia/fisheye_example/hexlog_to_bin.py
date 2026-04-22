#!/usr/bin/env python3
"""Extract raw bytes from UART-style hex dumps.

Only lines that contain nothing but hex digits and whitespace are decoded
(fisheye/$, timestamps, etc. are skipped). Glued pairs like ``d2d0`` (missing
space) are handled by concatenating the line and splitting into byte pairs.
"""
from __future__ import annotations

import sys
from pathlib import Path


def line_to_bytes(line: str):
    s = "".join(line.split())
    if not s:
        return None
    if not all(c in "0123456789abcdefABCDEF" for c in s):
        return None
    if len(s) % 2:
        return None
    return bytes(int(s[i : i + 2], 16) for i in range(0, len(s), 2))


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
