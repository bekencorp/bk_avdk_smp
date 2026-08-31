import argparse
import json
import re
import subprocess
import tempfile
from pathlib import Path


FFMPEG_BIN = "ffmpeg"


def run_ffmpeg_annexb(src: Path, dst: Path) -> None:
    subprocess.run(
        [
            FFMPEG_BIN, "-y", "-loglevel", "error", "-i", str(src),
            "-map", "0:v:0", "-an", "-c:v", "copy",
            "-bsf:v", "h264_mp4toannexb,h264_metadata=aud=insert",
            "-f", "h264", str(dst),
        ],
        check=True,
    )


def find_start_codes(data: bytes) -> list[tuple[int, int]]:
    starts = []
    i = 0
    while i + 3 < len(data):
        if data[i:i + 3] == b"\x00\x00\x01":
            starts.append((i, 3))
            i += 3
        elif data[i:i + 4] == b"\x00\x00\x00\x01":
            starts.append((i, 4))
            i += 4
        else:
            i += 1
    return starts


def split_access_units(data: bytes) -> list[tuple[int, int]]:
    aud_offsets = []
    for offset, prefix_len in find_start_codes(data):
        nal_pos = offset + prefix_len
        if nal_pos < len(data) and (data[nal_pos] & 0x1F) == 9:
            aud_offsets.append(offset)

    if not aud_offsets:
        raise RuntimeError("No H.264 AUD NAL units found")

    if aud_offsets[0] != 0:
        aud_offsets[0] = 0

    access_units = []
    for index, offset in enumerate(aud_offsets):
        end = aud_offsets[index + 1] if index + 1 < len(aud_offsets) else len(data)
        access_units.append((offset, end - offset))
    return access_units


def remove_aud_nals(data: bytes, access_units: list[tuple[int, int]]) -> tuple[bytes, list[tuple[int, int]]]:
    clean_data = bytearray()
    clean_access_units = []

    for offset, size in access_units:
        access_unit = data[offset:offset + size]
        starts = find_start_codes(access_unit)
        if not starts:
            raise RuntimeError("Access unit has no H.264 start code")

        first_offset, first_prefix = starts[0]
        first_nal = first_offset + first_prefix
        if first_nal >= len(access_unit) or (access_unit[first_nal] & 0x1F) != 9:
            raise RuntimeError("Access unit does not start with AUD")

        nal_chunks = []
        for index, (start, prefix) in enumerate(starts[1:]):
            end = starts[index + 2][0] if index + 2 < len(starts) else len(access_unit)
            nal_type = access_unit[start + prefix] & 0x1F
            nal_chunks.append((nal_type, access_unit[start:end]))

        if any(nal_type == 7 for nal_type, _ in nal_chunks):
            ordered = [chunk for nal_type, chunk in nal_chunks if nal_type == 7]
            ordered += [chunk for nal_type, chunk in nal_chunks if nal_type == 8]
            ordered += [chunk for nal_type, chunk in nal_chunks if nal_type not in (7, 8)]
            payload = b"".join(ordered)
        else:
            payload = b"".join(chunk for _, chunk in nal_chunks)
        clean_access_units.append((len(clean_data), len(payload)))
        clean_data.extend(payload)

    return bytes(clean_data), clean_access_units


def format_bytes(data: bytes, columns: int = 12) -> str:
    rows = []
    for offset in range(0, len(data), columns):
        chunk = data[offset:offset + columns]
        rows.append("    " + ", ".join(f"0x{value:02x}" for value in chunk) + ",")
    return "\n".join(rows)


def format_access_units(access_units: list[tuple[int, int]]) -> str:
    return "\n".join(f"    {{ .offset = {offset}U, .size = {size}U }}," for offset, size in access_units)


def format_u32(values: list[int], columns: int = 12) -> str:
    rows = []
    for offset in range(0, len(values), columns):
        chunk = values[offset:offset + columns]
        rows.append("    " + ", ".join(f"{value}U" for value in chunk) + ",")
    return "\n".join(rows)


def probe_video(src: Path) -> dict:
    output = subprocess.check_output(
        [
            "ffprobe", "-v", "error", "-select_streams", "v:0",
            "-show_entries", "stream=width,height,nb_frames",
            "-of", "json", str(src),
        ]
    )
    return json.loads(output)["streams"][0]


def write_asset(
    output: Path,
    symbol: str,
    width: int,
    height: int,
    rgb_data: bytes,
    alpha_data: bytes | None,
    rgb_aus: list[tuple[int, int]],
    alpha_aus: list[tuple[int, int]] | None,
    durations: list[int],
    alpha_width: int = 0,
    alpha_height: int = 0,
) -> None:
    if re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", symbol) is None:
        raise ValueError(f"Invalid C symbol: {symbol}")

    alpha_declarations = ""
    alpha_initializer = "{ 0 }"
    if alpha_data is not None and alpha_aus is not None:
        alpha_declarations = f"""
static const uint8_t {symbol}_alpha_data[] = {{
{format_bytes(alpha_data)}
}};

static const bk_baf_au_t {symbol}_alpha_aus[] = {{
{format_access_units(alpha_aus)}
}};
"""
        alpha_initializer = f"""{{
        .data = {symbol}_alpha_data,
        .data_size = sizeof({symbol}_alpha_data),
        .aus = {symbol}_alpha_aus,
        .au_count = {len(alpha_aus)}U,
    }}"""

    source = f"""#include "bk_baf_types.h"

static const uint8_t {symbol}_rgb_data[] = {{
{format_bytes(rgb_data)}
}};

static const bk_baf_au_t {symbol}_rgb_aus[] = {{
{format_access_units(rgb_aus)}
}};

{alpha_declarations}

static const uint32_t {symbol}_durations_ms[] = {{
{format_u32(durations)}
}};

static const bk_baf_media_t {symbol}_media = {{
    .width = {width}U,
    .height = {height}U,
    .alpha_width = {alpha_width}U,
    .alpha_height = {alpha_height}U,
    .frame_count = {len(durations)}U,
    .rgb = {{
        .data = {symbol}_rgb_data,
        .data_size = sizeof({symbol}_rgb_data),
        .aus = {symbol}_rgb_aus,
        .au_count = {len(rgb_aus)}U,
    }},
    .alpha = {alpha_initializer},
    .durations_ms = {symbol}_durations_ms,
}};

const bk_baf_source_t {symbol} = {{
    .magic = BK_BAF_SOURCE_MAGIC,
    .ops = &bk_baf_decoder_ops,
    .data = &{symbol}_media,
}};
"""
    output.write_text(source, encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate a BK_BAF C asset from RGB and alpha MP4 files.")
    parser.add_argument("--rgb", type=Path, required=True)
    parser.add_argument("--alpha", type=Path)
    parser.add_argument("--meta", type=Path, required=True)
    parser.add_argument("--output-c", type=Path, required=True)
    parser.add_argument("--symbol", default="my_anim_bk_baf_source")
    args = parser.parse_args()

    rgb_info = probe_video(args.rgb)
    alpha_width = 0
    alpha_height = 0
    if args.alpha is not None:
        alpha_info = probe_video(args.alpha)
        # Alpha may be authored at a lower resolution than RGB. Only record the
        # alpha dimensions explicitly when they differ; equal dims stay 0 so the
        # device treats alpha as full resolution (backward compatible).
        if (rgb_info["width"], rgb_info["height"]) != (alpha_info["width"], alpha_info["height"]):
            alpha_width = int(alpha_info["width"])
            alpha_height = int(alpha_info["height"])

    metadata = json.loads(args.meta.read_text(encoding="utf-8"))
    durations = [int(value) for value in metadata["durations_ms"]]

    with tempfile.TemporaryDirectory(prefix="bk_baf_annexb_", dir=args.output_c.parent) as tmp:
        tmp_dir = Path(tmp)
        rgb_path = tmp_dir / "rgb.h264"
        run_ffmpeg_annexb(args.rgb, rgb_path)
        rgb_data = rgb_path.read_bytes()
        alpha_data = None
        if args.alpha is not None:
            alpha_path = tmp_dir / "alpha.h264"
            run_ffmpeg_annexb(args.alpha, alpha_path)
            alpha_data = alpha_path.read_bytes()

    rgb_aus = split_access_units(rgb_data)
    alpha_aus = split_access_units(alpha_data) if alpha_data is not None else None
    expected = len(durations)
    if len(rgb_aus) != expected or (alpha_aus is not None and len(alpha_aus) != expected):
        raise RuntimeError(
            f"Frame count mismatch: meta={expected}, rgb={len(rgb_aus)}, "
            f"alpha={len(alpha_aus) if alpha_aus is not None else 0}"
        )

    rgb_data, rgb_aus = remove_aud_nals(rgb_data, rgb_aus)
    if alpha_data is not None and alpha_aus is not None:
        alpha_data, alpha_aus = remove_aud_nals(alpha_data, alpha_aus)

    write_asset(
        args.output_c,
        args.symbol,
        int(rgb_info["width"]),
        int(rgb_info["height"]),
        rgb_data,
        alpha_data,
        rgb_aus,
        alpha_aus,
        durations,
        alpha_width,
        alpha_height,
    )
    print(f"frames: {expected}")
    print(f"size: {rgb_info['width']}x{rgb_info['height']}")
    if alpha_width:
        print(f"alpha size: {alpha_width}x{alpha_height}")
    print(f"rgb_annexb: {len(rgb_data)} bytes")
    if alpha_data is not None:
        print(f"alpha_annexb: {len(alpha_data)} bytes")
    print(f"output_c: {args.output_c}")


if __name__ == "__main__":
    main()
