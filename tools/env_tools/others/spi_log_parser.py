#!/usr/bin/env python3
"""
SPI CSV log parser.

Input CSV example columns:
  Time [s], Packet ID, MOSI, MISO
Each MOSI/MISO cell is typically a hex byte like "0x68". Empty cells are allowed.
"""

from __future__ import annotations

import argparse
import csv
import os
import sys
from dataclasses import dataclass, field
from typing import Dict, Iterable, List, Optional, Union


@dataclass
class PacketData:
    packet_id: int
    mosi: bytearray = field(default_factory=bytearray)
    miso: bytearray = field(default_factory=bytearray)


def _normalize_header(s: str) -> str:
    # Comments must be in English per user rule.
    return "".join(ch.lower() for ch in s.strip() if ch.isalnum())


def _parse_packet_id(s: str) -> Optional[int]:
    s = (s or "").strip()
    if not s:
        return None
    try:
        # Accept decimal, hex like "0x10".
        return int(s, 0)
    except ValueError:
        return None


def _parse_optional_hex_byte(s: str) -> Optional[int]:
    s = (s or "").strip()
    if not s:
        return None

    # Common forms: "0x68", "68", "0X68", "68h" (rare).
    s2 = s.lower()
    if s2.endswith("h") and all(c in "0123456789abcdef" for c in s2[:-1]):
        s2 = "0x" + s2[:-1]

    try:
        v = int(s2, 0) if s2.startswith("0x") else int(s2, 16)
    except ValueError:
        return None

    if 0 <= v <= 0xFF:
        return v
    return None


BytesLike = Union[bytes, bytearray]


def _bytes_to_ascii_safe(b: BytesLike) -> str:
    # Decode as UTF-8 but keep it robust; replace invalid sequences.
    s = bytes(b).decode("utf-8", errors="replace")
    return s


def _find_column(headers: Iterable[str], candidates: Iterable[str]) -> Optional[str]:
    normalized_to_original: Dict[str, str] = {_normalize_header(h): h for h in headers}
    for cand in candidates:
        key = _normalize_header(cand)
        if key in normalized_to_original:
            return normalized_to_original[key]
    return None


def parse_spi_csv(path: str) -> Dict[int, PacketData]:
    with open(path, "r", newline="", encoding="utf-8-sig") as f:
        reader = csv.DictReader(f)
        if not reader.fieldnames:
            raise ValueError("CSV has no header row.")

        pid_col = _find_column(reader.fieldnames, ["Packet ID", "PacketID", "Packet", "ID"])
        mosi_col = _find_column(reader.fieldnames, ["MOSI", "Mosi"])
        miso_col = _find_column(reader.fieldnames, ["MISO", "Miso"])

        if pid_col is None:
            raise ValueError("Missing required column: Packet ID")
        if mosi_col is None and miso_col is None:
            raise ValueError("Missing required column(s): MOSI and/or MISO")

        packets: Dict[int, PacketData] = {}

        for row in reader:
            pid = _parse_packet_id(row.get(pid_col, ""))
            if pid is None:
                # Skip completely empty lines or malformed rows.
                continue

            pkt = packets.get(pid)
            if pkt is None:
                pkt = PacketData(packet_id=pid)
                packets[pid] = pkt

            if mosi_col:
                mv = _parse_optional_hex_byte(row.get(mosi_col, ""))
                if mv is not None:
                    pkt.mosi.append(mv)

            if miso_col:
                rv = _parse_optional_hex_byte(row.get(miso_col, ""))
                if rv is not None:
                    pkt.miso.append(rv)

    return packets


def main() -> int:
    ap = argparse.ArgumentParser(description="Parse SPI CSV log and output decoded characters.")
    ap.add_argument("csv_path", help="Input CSV file path, e.g. 2026-01-22_17-58-56.csv")
    ap.add_argument(
        "--direction",
        choices=["mosi", "miso"],
        default="mosi",
        help="Which direction(s) to output (default: mosi).",
    )
    ap.add_argument(
        "--packet-id",
        type=int,
        default=None,
        help="Only print a specific Packet ID (decimal).",
    )
    ap.add_argument(
        "--print",
        action="store_true",
        help="Print decoded text to stdout. Default is not printing (save to .txt only).",
    )
    ap.add_argument(
        "--no-save",
        action="store_true",
        help="Do not save parsed text output to a .txt file (default behavior is saving).",
    )
    ap.add_argument(
        "--save-only",
        action="store_true",
        help="(Deprecated) Save output only (no stdout). This is the default now.",
    )
    ap.add_argument(
        "--out-txt",
        default=None,
        help="Override default output .txt path. Default is same name as CSV with .txt suffix.",
    )
    args = ap.parse_args()

    try:
        packets = parse_spi_csv(args.csv_path)
    except ValueError as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1

    pids = sorted(packets.keys())
    if args.packet_id is not None:
        if args.packet_id not in packets:
            print(f"Error: Packet ID {args.packet_id} not found.", file=sys.stderr)
            return 1
        pids = [args.packet_id]

    # Output only decoded characters without any labels.
    # If multiple packets exist, concatenate by Packet ID order.
    out_parts: List[str] = []
    for pid in pids:
        pkt = packets[pid]
        if args.direction == "mosi":
            out_parts.append(_bytes_to_ascii_safe(pkt.mosi))
        else:
            out_parts.append(_bytes_to_ascii_safe(pkt.miso))

    text = "".join(out_parts)
    # Default behavior: do not print decoded content to terminal.
    if args.print and not args.save_only:
        print(text, end="")

    if not args.no_save:
        out_txt = args.out_txt
        if not out_txt:
            base, _ext = os.path.splitext(args.csv_path)
            out_txt = base + ".txt"
        with open(out_txt, "w", encoding="utf-8", newline="\n") as f:
            f.write(text)
        # Keep stdout clean (chars-only). Status goes to stderr.
        print(f"Saved output to: {out_txt}", file=sys.stderr)

    return 0


# Usage:
#   This script parses an SPI CSV log and outputs decoded MOSI/MISO data as text.
#   Run with:
#       python spi_log_parser.py --csv PATH_TO_CSV [options]
#   Options:
#       --direction {mosi,miso}       Select data direction to decode (default: mosi)
#       --packet-id ID                Only output this Packet ID (optional)
#       --print                       Print decoded text to stdout (default: False)
#       --no-save                     Do not save the decoded text to file
#       --out-txt PATH                Specify output file name (default: CSV name with .txt suffix)
# Example:
#       python spi_log_parser.py sample.csv

if __name__ == "__main__":
    raise SystemExit(main())

