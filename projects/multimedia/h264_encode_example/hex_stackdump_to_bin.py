#!/usr/bin/env python3
import argparse
import re
from pathlib import Path


#
# stack_mem_dump prints hex bytes like:
#   4c 4c 4c ... (32 bytes per line)
# Bytes are separated by a single space. Some .DAT logs may contain
# merged tokens like "44c" (missing space), which should be treated as
# format errors instead of silently “best-effort” parsing.
#
BYTE_TOKEN_RE = re.compile(r"\b[0-9a-fA-F]{2}\b")


def parse_hex_range(line: str):
    # Example:
    # >>>>stack mem dump begin, stack_top=60d089c0, stack end=60ffffc0
    m = re.search(
        r"stack_top=([0-9a-fA-F]+)\s*,\s*stack end=([0-9a-fA-F]+)",
        line,
    )
    if not m:
        return None
    top = int(m.group(1), 16)
    end = int(m.group(2), 16)
    if end < top:
        return None
    return top, end, (end - top)


def main():
    parser = argparse.ArgumentParser(
        description="Convert stack_mem_dump hex tokens in .DAT to raw .bin bytes."
    )
    parser.add_argument("input_dat", help="Input .DAT file path")
    parser.add_argument("output_bin", help="Output .bin file path")
    parser.add_argument(
        "--strict-size",
        action="store_true",
        help="Fail if extracted size != expected size from stack_top/stack end.",
    )
    parser.add_argument(
        "--bytes-per-line",
        type=int,
        default=32,
        help="Expected hex bytes count per dump line (default: 32).",
    )
    args = parser.parse_args()

    in_path = Path(args.input_dat)
    out_path = Path(args.output_bin)
    if not in_path.exists():
        raise FileNotFoundError(str(in_path))

    expected_size = None
    started = False
    in_dump = False
    have_data_line = False
    byte_list = []
    bytes_per_line = int(args.bytes_per_line)

    with in_path.open("r", encoding="utf-8", errors="ignore") as f:
        for line_no, line in enumerate(f, start=1):
            if not in_dump:
                if ">>>>stack mem dump begin" in line:
                    started = True
                    in_dump = True
                    parsed = parse_hex_range(line)
                    if parsed is not None:
                        _, _, expected_size = parsed
                continue

            if "<<<<stack mem dump end" in line:
                break

            tokens = BYTE_TOKEN_RE.findall(line)

            # Leading empty lines after "begin" are possible due to \r\n output.
            if len(tokens) == 0:
                if have_data_line:
                    raise RuntimeError(
                        f"Parse error: expected {bytes_per_line} bytes but got 0 at file line {line_no}.\n"
                        f"Line preview: {line[:160]!r}"
                    )
                continue

            if len(tokens) != bytes_per_line:
                raise RuntimeError(
                    f"Parse error: expected {bytes_per_line} bytes but got {len(tokens)} at file line {line_no}.\n"
                    f"Line preview: {line[:160]!r}"
                )

            for tok in tokens:
                byte_list.append(int(tok, 16))

            have_data_line = True

            if expected_size is not None and len(byte_list) >= expected_size:
                # Stop early once we have enough bytes.
                break

    if not started:
        raise RuntimeError("Could not find '>>>>stack mem dump begin' in input file.")

    if expected_size is None:
        expected_size = len(byte_list)
    else:
        # If output contains extra tokens, we truncate.
        byte_list = byte_list[:expected_size]

    if expected_size is not None and len(byte_list) != expected_size:
        msg = (
            f"Error: Extracted byte count mismatch: got={len(byte_list)} expected={expected_size}. "
            "The .DAT dump output may be truncated."
        )
        if args.strict_size:
            raise RuntimeError(msg)
        print("Warning: " + msg)

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_bytes(bytes(byte_list))
    print(f"Wrote {len(byte_list)} bytes to {out_path}")


if __name__ == "__main__":
    main()

