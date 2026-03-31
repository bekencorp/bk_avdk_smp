#!/usr/bin/env python3
import argparse
import re
from pathlib import Path


# Marker line emitted by h264_encode_stress:
# h264_frame_dump tag=h264_h p_index=0 type=3 size=28 buf=0xXXXXXXXX aligned=28
MARKER_RE = re.compile(
    r"h264_frame_dump\s+tag=(?P<tag>\S+)\s+"
    r"p_index=(?P<p_index>\d+)\s+"
    r"type=(?P<type>\d+)\s+"
    r"size=(?P<size>\d+)\s+"
    r"buf=(?P<buf>(?:0x)?[0-9a-fA-F]+)\s+"
    r"aligned=(?P<aligned>\d+)"
)

# Example begin line:
# >>>>stack mem dump begin, stack_top=64c05bc0, stack end=64c05d74
#
# Some platform logs may add extra spaces or punctuation, so only capture stack_top
# as long as the line is a stack_mem_dump begin line.
BEGIN_RE = re.compile(r">{4}stack mem dump begin", re.IGNORECASE)
STACK_TOP_RE = re.compile(r"stack_top=(?P<stack_top>(?:0x)?[0-9a-fA-F]+)", re.IGNORECASE)
END_RE = re.compile(r"<<<<stack mem dump end")

# stack_mem_dump prints %02x tokens, but some logs may occasionally lose spaces.
# Use a tolerant regex that extracts any 2-hex-digit sequences.
HEX_BYTE_RE = re.compile(r"[0-9a-fA-F]{2}")


def parse_stack_top_from_begin_line(line: str):
    # Extract only stack_top=... to avoid being sensitive to log formatting.
    m = STACK_TOP_RE.search(line)
    if not m:
        return None
    return int(m.group("stack_top"), 16)


def parse_dump_block(lines, begin_idx: int):
    """
    Parse one stack_mem_dump block.
    Returns: (end_idx_inclusive, stack_top_int, dump_bytes_list)
    """
    stack_top = parse_stack_top_from_begin_line(lines[begin_idx])
    if stack_top is None:
        raise RuntimeError(f"dump begin parse failed at line {begin_idx + 1}")

    bytes_out = []
    i = begin_idx + 1  # skip the "begin" line itself
    while i < len(lines):
        if END_RE.search(lines[i]):
            return i, stack_top, bytes_out
        bytes_out.extend(int(x, 16) for x in HEX_BYTE_RE.findall(lines[i]))
        i += 1
    raise RuntimeError(f"dump end not found after begin line {begin_idx + 1}")


def main():
    parser = argparse.ArgumentParser(
        description="Parse h264 stack_mem_dump into a single H264 stream .bin file."
    )
    parser.add_argument("input_log", help="Input log file (e.g. ReceivedTofile-xxxx.DAT)")
    parser.add_argument(
        "--out-stream",
        default="h264_stream_from_dump.h264",
        help="Output H264 stream .h264 file path (default: h264_stream_from_dump.h264)",
    )
    parser.add_argument(
        "--p-count",
        type=int,
        default=29,
        help="How many P frames to append (default: 29)",
    )
    parser.add_argument(
        "--strict-size",
        action="store_true",
        help="Fail if extracted dump bytes are smaller than marker's expected size.",
    )
    args = parser.parse_args()

    in_path = Path(args.input_log)
    if not in_path.exists():
        raise FileNotFoundError(str(in_path))

    lines = in_path.read_text("utf-8", errors="ignore").splitlines()

    # Collect markers and dump blocks first.
    markers = []
    dump_blocks = []  # list of dicts: begin_idx, end_idx, stack_top, bytes

    i = 0
    while i < len(lines):
        mm = MARKER_RE.search(lines[i])
        if mm:
            markers.append(
                {
                    "line_idx": i,
                    "tag": mm.group("tag"),
                    "p_index": int(mm.group("p_index")),
                    "type": int(mm.group("type")),
                    "size": int(mm.group("size")),
                    "buf": int(mm.group("buf"), 16),
                }
            )
            i += 1
            continue

        if ">>>>stack mem dump begin" in lines[i]:
            end_idx, stack_top, dump_bytes = parse_dump_block(lines, i)
            dump_blocks.append(
                {
                    "begin_idx": i,
                    "end_idx": end_idx,
                    "stack_top": stack_top,
                    "bytes": dump_bytes,
                }
            )
            i = end_idx + 1
            continue

        i += 1

    if not markers:
        raise RuntimeError("No h264_frame_dump markers found in log.")
    if not dump_blocks:
        raise RuntimeError("No stack_mem_dump blocks found in log.")

    blocks_by_stack_top = {}
    for b in dump_blocks:
        blocks_by_stack_top.setdefault(b["stack_top"], []).append(b)

    def extract_frame_bytes(mk):
        frame_type = mk["type"]
        size = mk["size"]
        buf_addr = mk["buf"]

        candidates = blocks_by_stack_top.get(buf_addr)
        if not candidates:
            raise RuntimeError(
                f"No dump block with stack_top==buf (buf=0x{buf_addr:x}) "
                f"for marker at line {mk['line_idx'] + 1}: type={frame_type} size={size}"
            )

        # The log output may interleave markers and stack_mem_dump blocks.
        # For correctness, prefer:
        # 1) dump blocks that end before the marker line (marker printed after dump)
        # 2) otherwise dump blocks that begin after the marker line (marker printed before dump)
        # 3) otherwise fall back to the closest begin line.
        marker_line_idx = mk["line_idx"]
        prev = [b for b in candidates if b["end_idx"] < marker_line_idx]
        if prev:
            best = max(prev, key=lambda b: b["end_idx"])
        else:
            nxt = [b for b in candidates if b["begin_idx"] > marker_line_idx]
            if nxt:
                best = min(nxt, key=lambda b: b["begin_idx"])
            else:
                best = min(candidates, key=lambda b: abs(b["begin_idx"] - marker_line_idx))
        dump_bytes = best["bytes"]
        if len(dump_bytes) < size:
            preview = "\n".join(lines[best["begin_idx"] : min(best["begin_idx"] + 4, len(lines))])
            if args.strict_size:
                raise RuntimeError(
                    f"dump bytes not enough: buf=0x{buf_addr:x} type={frame_type} size={size} got={len(dump_bytes)} "
                    f"(marker line {mk['line_idx'] + 1}, dump begin line {best['begin_idx'] + 1})\n"
                    f"Dump preview:\n{preview}"
                )
            print(
                f"Warning: dump bytes truncated: buf=0x{buf_addr:x} type={frame_type} size={size} got={len(dump_bytes)} "
                f"(marker line {mk['line_idx'] + 1}, dump begin line {best['begin_idx'] + 1})"
            )

        return dump_bytes[:size]

    # Select one marker for each frame category:
    # type==3 => header, type==0 => I frame, type==1 => P frame
    header_mk = None
    i_mk = None
    p_mks_by_index = {}

    for mk in markers:
        if mk["type"] == 3 and header_mk is None:
            header_mk = mk
        elif mk["type"] == 0 and i_mk is None:
            i_mk = mk
        elif mk["type"] == 1:
            idx = mk["p_index"]
            if idx < 1:
                raise RuntimeError(f"Invalid p_index={idx} for P frame at marker line {mk['line_idx'] + 1}")
            # If duplicated, keep the earlier one in the log.
            if idx not in p_mks_by_index or mk["line_idx"] < p_mks_by_index[idx]["line_idx"]:
                p_mks_by_index[idx] = mk

    if header_mk is None:
        raise RuntimeError("Missing header marker (type=3) in log.")
    if i_mk is None:
        raise RuntimeError("Missing I frame marker (type=0) in log.")

    stream_bytes = []

    header_bytes = extract_frame_bytes(header_mk)
    stream_bytes.extend(header_bytes)
    print(f"Appended header: {len(header_bytes)} bytes (marker line {header_mk['line_idx'] + 1})")

    i_bytes = extract_frame_bytes(i_mk)
    stream_bytes.extend(i_bytes)
    print(f"Appended I frame: {len(i_bytes)} bytes (marker line {i_mk['line_idx'] + 1})")

    missing_p = []
    for p_idx in range(1, args.p_count + 1):
        mk = p_mks_by_index.get(p_idx)
        if mk is None:
            missing_p.append(p_idx)
            continue
        p_bytes = extract_frame_bytes(mk)
        stream_bytes.extend(p_bytes)
        print(f"Appended P frame {p_idx:02d}: {len(p_bytes)} bytes (marker line {mk['line_idx'] + 1})")

    if missing_p:
        print(f"Warning: missing P frames: {missing_p[:10]}{'...' if len(missing_p) > 10 else ''}")

    out_stream = Path(args.out_stream)
    if not out_stream.is_absolute():
        out_stream = in_path.parent / out_stream
    out_stream.parent.mkdir(parents=True, exist_ok=True)
    out_stream.write_bytes(bytes(stream_bytes))
    print(f"Wrote full H264 stream: {len(stream_bytes)} bytes -> {out_stream}")


if __name__ == "__main__":
    main()

