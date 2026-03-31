#!/usr/bin/env python3
"""
Parse stack_mem_dump MJPEG logs and export JPEG images.

This script extracts hex bytes printed by stack_mem_dump between:
  ">>>>stack mem dump begin" and "<<<<stack mem dump end"

It then searches for JPEG frames by SOI/EOI markers (FFD8 .. FFD9) and writes
each frame to an output .jpg file for easy viewing.
"""

from __future__ import annotations

import argparse
import os
import re
import sys
from dataclasses import dataclass
from typing import List, Optional, Tuple


HEX_BYTE_RE = re.compile(r"\b[0-9a-fA-F]{2}\b")
BEGIN_RE = re.compile(r">>>>stack mem dump begin", re.IGNORECASE)
END_RE = re.compile(r"<<<<stack mem dump end", re.IGNORECASE)


@dataclass
class DumpBlock:
    index: int
    start_line: int
    end_line: int
    data: bytes


def iter_dump_blocks(lines: List[str]) -> List[DumpBlock]:
    blocks: List[DumpBlock] = []
    in_dump = False
    cur_tokens: List[int] = []
    start_line = -1
    block_idx = 0

    def flush(end_line: int) -> None:
        nonlocal block_idx, cur_tokens, start_line
        if not cur_tokens:
            return
        blocks.append(
            DumpBlock(
                index=block_idx,
                start_line=start_line,
                end_line=end_line,
                data=bytes(cur_tokens),
            )
        )
        block_idx += 1
        cur_tokens = []
        start_line = -1

    for i, line in enumerate(lines, start=1):
        if not in_dump and BEGIN_RE.search(line):
            in_dump = True
            cur_tokens = []
            start_line = i
            continue

        if in_dump and END_RE.search(line):
            in_dump = False
            flush(i)
            continue

        if not in_dump:
            continue

        # Extract all 2-hex-digit tokens on the line.
        for m in HEX_BYTE_RE.finditer(line):
            cur_tokens.append(int(m.group(0), 16))

    # Handle truncated logs without an END marker.
    if in_dump:
        flush(len(lines))

    return blocks


def find_jpeg_frames(buf: bytes) -> List[Tuple[int, int]]:
    """
    Return list of (start, end_exclusive) ranges for each JPEG frame.
    Uses SOI (FFD8) and EOI (FFD9).
    """
    frames: List[Tuple[int, int]] = []
    i = 0
    n = len(buf)
    while i + 1 < n:
        soi = buf.find(b"\xFF\xD8", i)
        if soi < 0:
            break
        eoi = buf.find(b"\xFF\xD9", soi + 2)
        if eoi < 0:
            # No end marker; stop to avoid emitting broken frame.
            break
        frames.append((soi, eoi + 2))
        i = eoi + 2
    return frames


def ensure_dir(path: str) -> None:
    os.makedirs(path, exist_ok=True)


def ceil_align(v: int, align: int) -> int:
    if align <= 0:
        return v
    return ((v + align - 1) // align) * align


def patch_jpeg_sof_height(jpeg: bytes, new_height: int) -> bytes:
    """
    Patch JPEG SOF height field so image viewers display padded lines.

    This only modifies the first SOF marker found (SOF0/SOF2/...).
    If no SOF marker is found, returns original bytes.
    """
    if len(jpeg) < 4 or jpeg[0:2] != b"\xFF\xD8":
        return jpeg

    b = bytearray(jpeg)
    i = 2
    while i + 4 <= len(b):
        if b[i] != 0xFF:
            i += 1
            continue
        j = i + 1
        while j < len(b) and b[j] == 0xFF:
            j += 1
        if j >= len(b):
            break
        marker = b[j]
        i = j + 1

        # Standalone markers
        if marker in (0xD8, 0xD9) or (0xD0 <= marker <= 0xD7) or marker == 0x01:
            continue

        if i + 2 > len(b):
            break
        seglen = (b[i] << 8) | b[i + 1]
        segstart = i + 2
        segend = segstart + seglen - 2
        if seglen < 2 or segend > len(b):
            break

        is_sof = marker in (
            0xC0, 0xC1, 0xC2, 0xC3,
            0xC5, 0xC6, 0xC7,
            0xC9, 0xCA, 0xCB,
            0xCD, 0xCE, 0xCF,
        )
        if is_sof and seglen >= 7:
            # SOF payload: P(1) + Y(2) + X(2) + ...
            if not (0 < new_height <= 0xFFFF):
                return jpeg
            b[segstart + 1] = (new_height >> 8) & 0xFF
            b[segstart + 2] = new_height & 0xFF
            return bytes(b)

        i = segend

    return jpeg


def write_frames(frames: List[bytes], out_dir: str, prefix: str, force_height: Optional[int]) -> List[str]:
    ensure_dir(out_dir)
    out_paths: List[str] = []
    for idx, data in enumerate(frames):
        out_data = data
        if force_height is not None:
            out_data = patch_jpeg_sof_height(out_data, force_height)

        out_path = os.path.join(out_dir, f"{prefix}_{idx:03d}.jpg")
        with open(out_path, "wb") as f:
            f.write(out_data)
        out_paths.append(out_path)
    return out_paths


def main(argv: Optional[List[str]] = None) -> int:
    ap = argparse.ArgumentParser(description="Parse stack_mem_dump MJPEG logs to .jpg files.")
    ap.add_argument("input", help="Input log file (e.g. ReceivedTofile-*.DAT)")
    ap.add_argument("-o", "--out-dir", default="mjpeg_out", help="Output directory")
    ap.add_argument("--prefix", default="frame", help="Output filename prefix")
    ap.add_argument("--dump-index", type=int, default=None, help="Only parse the Nth dump block (0-based)")
    ap.add_argument(
        "--force-height",
        type=int,
        default=None,
        help="Patch JPEG SOF height to this value (e.g. 1088) for display",
    )
    ap.add_argument(
        "--align-height-16",
        action="store_true",
        help="Patch JPEG SOF height to ceil(original_height/16)*16 for display",
    )
    args = ap.parse_args(argv)

    try:
        with open(args.input, "r", encoding="utf-8", errors="ignore") as f:
            lines = f.read().splitlines()
    except OSError as e:
        print(f"ERROR: failed to read input file: {e}", file=sys.stderr)
        return 2

    blocks = iter_dump_blocks(lines)
    if not blocks:
        print("ERROR: no dump blocks found. Expected '>>>>stack mem dump begin' markers.", file=sys.stderr)
        return 3

    selected: List[DumpBlock]
    if args.dump_index is None:
        selected = blocks
    else:
        if args.dump_index < 0 or args.dump_index >= len(blocks):
            print(f"ERROR: dump-index out of range: {args.dump_index} (have {len(blocks)} blocks)", file=sys.stderr)
            return 4
        selected = [blocks[args.dump_index]]

    total_frames = 0
    for b in selected:
        ranges = find_jpeg_frames(b.data)
        if not ranges:
            print(
                f"WARNING: no JPEG SOI/EOI found in dump block {b.index} "
                f"(lines {b.start_line}-{b.end_line}, bytes={len(b.data)})",
                file=sys.stderr,
            )
            continue

        frames = [b.data[s:e] for (s, e) in ranges]

        force_h = args.force_height
        if args.align_height_16:
            # Try to infer original height from SOF; if not found, skip patching.
            # We reuse patcher logic by scanning for SOF and reading current height.
            cur_h = None
            jf = frames[0]
            if len(jf) >= 4 and jf[0:2] == b"\xFF\xD8":
                i = 2
                while i + 4 <= len(jf):
                    if jf[i] != 0xFF:
                        i += 1
                        continue
                    j = i + 1
                    while j < len(jf) and jf[j] == 0xFF:
                        j += 1
                    if j >= len(jf):
                        break
                    marker = jf[j]
                    i = j + 1
                    if marker in (0xD8, 0xD9) or (0xD0 <= marker <= 0xD7) or marker == 0x01:
                        continue
                    if i + 2 > len(jf):
                        break
                    seglen = (jf[i] << 8) | jf[i + 1]
                    segstart = i + 2
                    segend = segstart + seglen - 2
                    if seglen < 2 or segend > len(jf):
                        break
                    is_sof = marker in (
                        0xC0, 0xC1, 0xC2, 0xC3,
                        0xC5, 0xC6, 0xC7,
                        0xC9, 0xCA, 0xCB,
                        0xCD, 0xCE, 0xCF,
                    )
                    if is_sof and seglen >= 7:
                        cur_h = (jf[segstart + 1] << 8) | jf[segstart + 2]
                        break
                    i = segend
            if cur_h is not None:
                force_h = ceil_align(cur_h, 16)

        out_paths = write_frames(frames, args.out_dir, f"{args.prefix}_dump{b.index}", force_h)
        total_frames += len(out_paths)
        print(
            f"dump{b.index}: extracted {len(frames)} frame(s), "
            f"lines {b.start_line}-{b.end_line}, bytes={len(b.data)}"
        )
        for p in out_paths:
            print(p)

    if total_frames == 0:
        print("ERROR: no frames written.", file=sys.stderr)
        return 5

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

