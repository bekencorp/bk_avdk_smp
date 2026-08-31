#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
to_baf.py - convert common animations to the BAF v1 container (see BAF_SPEC_CN.md).

Input (auto-detected):  APNG / animated PNG, animated WebP, GIF, MP4 / MOV / MKV / WEBM
Output (user-selectable via --emit, byte-identical container image):
  1. <name>.baf     binary container file (ship on filesystem/flash), OR
  2. <name>_baf.c   the SAME bytes as a C array (const unsigned char <name>_baf[]),
                    compiled into firmware.
Both forms are the same container image; the device parses either with the same
baf_parse_inplace() (see bk_baf/include/bk_baf_container.h).

Pipeline:
  source
    -> extract per-frame RGB + Alpha (grayscale) + durations
    -> H.264 encode RGB (no B-frames, refs=1)  +  H.264 encode Alpha as gray (optional)
    -> Annex-B + per-frame access-unit (AU) split + AUD strip
    -> pack into the BAF v1 container

--------------------------------------------------------------------------------
BAF v1 container layout (little-endian). See BAF_SPEC_CN.md for the normative spec.

  FileHeader (64 bytes):
    magic char[8] "BAFANIM1"; u16 ver_major=1, ver_minor=0; u32 flags=0;
    u32 file_size; u32 dir_offset; u16 dir_count; u8 has_alpha; u8 reserved0;
    u32 frame_count; u16 width,height,alpha_width,alpha_height;
    u32 rgb_idx, rgb_data, alpha_idx, alpha_data, dur;   (directory indices)
    u32 header_crc32   (CRC32 of the first 60 bytes)
  ChunkDirectory (16 bytes/entry): u32 type(FourCC), offset, size, crc32
  Chunks (each 64B aligned): 'IDX ' (u32 offset,size per frame) / 'DUR ' (u32 ms
    per frame) / 'DATA' (H.264 Annex-B). alpha_idx/alpha_data = 0xFFFFFFFF if none.
--------------------------------------------------------------------------------
"""

import argparse
import json
import struct
import subprocess
import tempfile
import zlib
from pathlib import Path

from PIL import Image, ImageSequence

# Reuse the proven Annex-B / AU-split / AUD-strip / C-asset packing helpers.
import mp4_to_bk_baf_asset as bk_baf

try:
    from imageio_ffmpeg import get_ffmpeg_exe
    FFMPEG_BIN = get_ffmpeg_exe()
except ImportError:
    FFMPEG_BIN = "ffmpeg"

bk_baf.FFMPEG_BIN = FFMPEG_BIN


# ------------------------------- config ------------------------------------
DEFAULT_DURATION_MS = 40
# H.264 encode: intra-friendly for the HW decoder (no B-frames, single ref).
CRF = 24
# Alpha is a mask that must round-trip cleanly: an "opaque" pixel has to stay 255
# so a stacked layer fully occludes the ones below (see clean_alpha_frames). Use
# a high-quality (low-CRF) alpha encode so the snapped-flat 0/255 regions survive
# the codec. Still a normal 4:2:0/monochrome profile (NOT lossless -crf 0, which
# switches to High 4:4:4 that the HW decoder can't handle).
ALPHA_CRF = 12
# Alpha values >= HI snap to 255 (fully opaque), <= LO snap to 0 (fully
# transparent); the mid-range is kept for smooth anti-aliased edges.
ALPHA_SNAP_HI = 248
ALPHA_SNAP_LO = 7
PRESET = "veryfast"
GOP = 30
REFS = 1
# Sources handled by PIL (frame + per-frame duration + alpha channel).
PIL_EXTS = {".png", ".apng", ".gif", ".webp"}
# Sources handled by ffmpeg (video containers).
FFMPEG_EXTS = {".mp4", ".mov", ".mkv", ".webm", ".m4v", ".avi"}

BAF_MAGIC = b"BAFANIM1"
BAF_VER_MAJOR = 1
BAF_VER_MINOR = 0
BAF_HEADER_SIZE = 64
BAF_IDX_NONE = 0xFFFFFFFF

# FourCC as little-endian u32 (matches bk_baf_container.h / spec appendix A).
def _cc(s: str) -> int:
    b = s.encode("ascii")
    return b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24)

CC_IDX = _cc("IDX ")
CC_DUR = _cc("DUR ")
CC_DATA = _cc("DATA")


# --------------------------- frame extraction ------------------------------
def extract_frames_pil(src: Path, rgb_dir: Path, alpha_dir: Path):
    """APNG / GIF / WebP / animated PNG -> rgb_%04d.png + alpha_%04d.png (L)."""
    image = Image.open(src)
    width, height = image.size
    durations = []
    has_alpha = False
    n = 0
    for frame in ImageSequence.Iterator(image):
        rgba = frame.convert("RGBA")
        dur = int(frame.info.get("duration", image.info.get("duration", DEFAULT_DURATION_MS))
                  or DEFAULT_DURATION_MS)
        durations.append(dur)
        rgba.convert("RGB").save(rgb_dir / f"rgb_{n:04d}.png")
        a = rgba.getchannel("A")
        if a.getextrema()[0] < 255:
            has_alpha = True
        a.save(alpha_dir / f"alpha_{n:04d}.png")
        n += 1
    if n == 0:
        raise RuntimeError(f"No frames decoded from {src}")
    return width, height, durations, has_alpha


def _ffprobe_stream(src: Path) -> dict:
    out = subprocess.check_output([
        "ffprobe", "-v", "error", "-select_streams", "v:0",
        "-show_entries", "stream=width,height,nb_frames,avg_frame_rate,pix_fmt",
        "-of", "json", str(src),
    ])
    return json.loads(out)["streams"][0]


def extract_frames_ffmpeg(src: Path, rgb_dir: Path, alpha_dir: Path):
    """Video container -> rgb_%04d.png (+ alpha_%04d.png if the stream has alpha)."""
    info = _ffprobe_stream(src)
    width, height = int(info["width"]), int(info["height"])
    # fps from avg_frame_rate "num/den"
    num, _, den = info.get("avg_frame_rate", "0/1").partition("/")
    fps = (float(num) / float(den)) if den and float(den) != 0 else 25.0
    if fps <= 0:
        fps = 25.0
    has_alpha = "a" in (info.get("pix_fmt") or "")  # rgba/yuva... contain 'a'

    subprocess.run([
        FFMPEG_BIN, "-y", "-loglevel", "error", "-i", str(src),
        "-vf", "format=rgb24", str(rgb_dir / "rgb_%04d.png"),
    ], check=True)
    if has_alpha:
        subprocess.run([
            FFMPEG_BIN, "-y", "-loglevel", "error", "-i", str(src),
            "-vf", "alphaextract,format=gray", str(alpha_dir / "alpha_%04d.png"),
        ], check=True)

    n = len(list(rgb_dir.glob("rgb_*.png")))
    if n == 0:
        raise RuntimeError(f"ffmpeg extracted no frames from {src}")
    # Distribute integer-millisecond durations without accumulating rounding
    # error (for example, 24 FPS alternates 42/41 ms instead of using 42 ms
    # for every frame and slowing playback to 23.81 FPS).
    durations = [
        int(round((index + 1) * 1000.0 / fps) - round(index * 1000.0 / fps))
        for index in range(n)
    ]
    return width, height, durations, has_alpha


def extract_frames(src: Path, rgb_dir: Path, alpha_dir: Path):
    ext = src.suffix.lower()
    if ext in PIL_EXTS:
        return extract_frames_pil(src, rgb_dir, alpha_dir)
    if ext in FFMPEG_EXTS:
        return extract_frames_ffmpeg(src, rgb_dir, alpha_dir)
    # Fallback: let PIL try (covers odd extensions of still/animated images).
    try:
        return extract_frames_pil(src, rgb_dir, alpha_dir)
    except Exception:
        return extract_frames_ffmpeg(src, rgb_dir, alpha_dir)


# ------------------------------- encoding ----------------------------------
def clean_alpha_frames(alpha_dir: Path) -> None:
    """Snap near-extreme alpha to exactly 0/255 so flat opaque/transparent regions
    are constant and survive the lossy H.264 alpha stream: an opaque pixel stays
    255, so a stacked layer fully occludes the ones below (no faint bleed-through)
    and transparent regions stay clean (no halo). Mid-range edge alpha is left
    untouched for smooth anti-aliasing."""
    lut = bytes(0 if v <= ALPHA_SNAP_LO else 255 if v >= ALPHA_SNAP_HI else v
                for v in range(256))
    for png in sorted(alpha_dir.glob("alpha_*.png")):
        Image.open(png).convert("L").point(lut).save(png)


def _align16(n: int) -> int:
    return (n + 15) & ~15


def pad_frames_to(frame_dir: Path, glob: str, pw: int, ph: int, mode: str, fill) -> None:
    """Pad each extracted frame onto a pw x ph canvas (top-left), so width/height
    are multiples of 16 as the VPU requires. No-op for frames already that size."""
    for png in sorted(frame_dir.glob(glob)):
        im = Image.open(png).convert(mode)
        if im.size == (pw, ph):
            continue
        canvas = Image.new(mode, (pw, ph), fill)
        canvas.paste(im, (0, 0))
        canvas.save(png)


def encode_h264(frame_dir: Path, pattern: str, out_mp4: Path, fps: float,
                pixel_format: str = "yuv420p", crf: int = CRF) -> None:
    subprocess.run([
        FFMPEG_BIN, "-y", "-loglevel", "error",
        "-framerate", f"{fps:.6f}",
        "-i", str(frame_dir / pattern),
        "-an", "-c:v", "libx264", "-preset", PRESET, "-crf", str(crf),
        "-pix_fmt", pixel_format, "-bf", "0", "-x264-params", "bframes=0",
        "-refs", str(REFS), "-g", str(GOP), "-movflags", "+faststart",
        str(out_mp4),
    ], check=True)


def mp4_to_annexb_aus(mp4: Path, tmp_dir: Path, tag: str):
    """mp4 -> (annexb bytes, per-frame AU list) with AUD stripped."""
    h264 = tmp_dir / f"{tag}.h264"
    bk_baf.run_ffmpeg_annexb(mp4, h264)
    data = h264.read_bytes()
    aus = bk_baf.split_access_units(data)
    return bk_baf.remove_aud_nals(data, aus)  # -> (clean_data, clean_aus)


# ---------------------- BAF v1 container packing ---------------------------
def _align64(n: int) -> int:
    return (n + 63) & ~63


def pack_baf_container(width, height, alpha_width, alpha_height,
                       durations, rgb_data, rgb_aus, alpha_data, alpha_aus) -> bytes:
    """Pack the BAF v1 container image (bytes). Same image is emitted as .baf file
    and/or C array. Layout: 64B FileHeader + ChunkDirectory + 64B-aligned chunks."""
    has_alpha = alpha_data is not None
    frame_count = len(durations)

    idx_rgb = b"".join(struct.pack("<II", off, size) for off, size in rgb_aus)
    dur_blob = b"".join(struct.pack("<I", d) for d in durations)

    # entries: (fourcc, blob); their positions become the header directory indices.
    entries = [(CC_IDX, idx_rgb), (CC_DUR, dur_blob), (CC_DATA, rgb_data)]
    rgb_idx_i, dur_i, rgb_data_i = 0, 1, 2
    alpha_idx_i = alpha_data_i = BAF_IDX_NONE
    if has_alpha:
        idx_alpha = b"".join(struct.pack("<II", off, size) for off, size in alpha_aus)
        alpha_idx_i = len(entries); entries.append((CC_IDX, idx_alpha))
        alpha_data_i = len(entries); entries.append((CC_DATA, alpha_data))

    dir_count = len(entries)
    dir_size = dir_count * 16

    # Reserve header + directory, then append each blob 64B-aligned.
    buf = bytearray(BAF_HEADER_SIZE + dir_size)
    dir_records = []
    for cc, blob in entries:
        if len(buf) % 64:
            buf += b"\x00" * (64 - len(buf) % 64)
        off = len(buf)
        dir_records.append((cc, off, len(blob), zlib.crc32(blob) & 0xFFFFFFFF))
        buf += blob
    file_size = len(buf)

    # Write the directory at dir_offset (== header size).
    dir_bytes = b"".join(struct.pack("<IIII", cc, off, sz, crc)
                         for cc, off, sz, crc in dir_records)
    buf[BAF_HEADER_SIZE:BAF_HEADER_SIZE + dir_size] = dir_bytes

    # Header: pack first 60 bytes, CRC them, then append header_crc32.
    hdr = struct.pack(
        "<8sHHIIIHBBIHHHHIIIII",
        BAF_MAGIC, BAF_VER_MAJOR, BAF_VER_MINOR, 0,
        file_size, BAF_HEADER_SIZE, dir_count, 1 if has_alpha else 0, 0,
        frame_count, width, height, alpha_width, alpha_height,
        rgb_idx_i, rgb_data_i, alpha_idx_i, alpha_data_i, dur_i,
    )
    assert len(hdr) == BAF_HEADER_SIZE - 4, len(hdr)
    buf[0:BAF_HEADER_SIZE - 4] = hdr
    buf[BAF_HEADER_SIZE - 4:BAF_HEADER_SIZE] = struct.pack("<I", zlib.crc32(hdr) & 0xFFFFFFFF)
    return bytes(buf)


def write_carray(path: Path, symbol: str, data: bytes) -> None:
    """Emit the container bytes as a C array (identical to the .baf file bytes)."""
    lines = ["#include <stdint.h>\n\n",
             f"const unsigned char {symbol}[] = {{\n"]
    for i in range(0, len(data), 12):
        row = ", ".join(f"0x{b:02x}" for b in data[i:i + 12])
        lines.append(f"    {row},\n")
    lines.append("};\n")
    lines.append(f"const unsigned int {symbol}_size = sizeof({symbol});\n")
    path.write_text("".join(lines), encoding="utf-8")


# --------------------------------- main ------------------------------------
def main() -> None:
    ap = argparse.ArgumentParser(
        description="Convert APNG/WebP/GIF/MP4/MOV to a BAF v1 container (file and/or C array).")
    ap.add_argument("--input", type=Path, required=True)
    ap.add_argument("--outdir", type=Path, default=None,
                    help="Output directory (default: input's directory).")
    ap.add_argument("--name", default=None,
                    help="Base name for outputs (default: input stem).")
    ap.add_argument("--emit", choices=("file", "array", "both"), default="both",
                    help="Output form: .baf file / C array / both (default: both). "
                         "Both are byte-identical container images.")
    ap.add_argument("--symbol", default=None,
                    help="C array symbol name (default: <name>_baf).")
    ap.add_argument("--force-opaque-alpha", action="store_true",
                    help="Generate a full-resolution all-opaque alpha stream "
                         "when the input has no alpha channel.")
    args = ap.parse_args()

    src = args.input
    outdir = args.outdir or src.parent
    outdir.mkdir(parents=True, exist_ok=True)
    name = args.name or src.stem
    symbol = args.symbol or f"{name}_baf"
    baf_path = outdir / f"{name}.baf"
    c_path = outdir / f"{name}_baf.c"

    with tempfile.TemporaryDirectory(prefix="to_baf_", dir=outdir) as tmp:
        tmp_dir = Path(tmp)
        rgb_dir = tmp_dir / "rgb"; rgb_dir.mkdir()
        alpha_dir = tmp_dir / "alpha"; alpha_dir.mkdir()

        width, height, durations, has_alpha = extract_frames(src, rgb_dir, alpha_dir)
        frame_count = len(durations)
        fps = 1000.0 / (sum(durations) / frame_count)

        # VPU decodes 16x16 macroblocks -> width/height must be multiples of 16.
        pw, ph = _align16(width), _align16(height)

        if args.force_opaque_alpha and not has_alpha:
            opaque_alpha = Image.new("L", (pw, ph), 255)
            for index in range(frame_count):
                opaque_alpha.save(alpha_dir / f"alpha_{index:04d}.png")
            has_alpha = True

        pad_frames_to(rgb_dir, "rgb_*.png", pw, ph, "RGB", (0, 0, 0))
        rgb_mp4 = tmp_dir / "rgb.mp4"
        encode_h264(rgb_dir, "rgb_%04d.png", rgb_mp4, fps)
        rgb_data, rgb_aus = mp4_to_annexb_aus(rgb_mp4, tmp_dir, "rgb")

        alpha_data = alpha_aus = None
        if has_alpha:
            clean_alpha_frames(alpha_dir)   # snap extremes so opaque stays 255
            pad_frames_to(alpha_dir, "alpha_*.png", pw, ph, "L", 0)
            alpha_mp4 = tmp_dir / "alpha.mp4"
            encode_h264(alpha_dir, "alpha_%04d.png", alpha_mp4, fps, "gray", ALPHA_CRF)
            alpha_data, alpha_aus = mp4_to_annexb_aus(alpha_mp4, tmp_dir, "alpha")

        # sanity: one AU per frame
        if len(rgb_aus) != frame_count or (alpha_aus is not None and len(alpha_aus) != frame_count):
            raise RuntimeError(
                f"Frame/AU mismatch: frames={frame_count} rgb_aus={len(rgb_aus)} "
                f"alpha_aus={len(alpha_aus) if alpha_aus is not None else 0}")

        width, height = pw, ph          # container carries the padded (16-aligned) size
        alpha_w = alpha_h = 0           # alpha full-res (0 => same as RGB)
        container = pack_baf_container(width, height, alpha_w, alpha_h,
                                       durations, rgb_data, rgb_aus, alpha_data, alpha_aus)

    if args.emit in ("file", "both"):
        baf_path.write_bytes(container)
    if args.emit in ("array", "both"):
        write_carray(c_path, symbol, container)

    print(f"input:       {src}")
    print(f"size:        {width}x{height}  frames: {frame_count}  fps: {fps:.2f}  alpha: {has_alpha}")
    print(f"rgb annexb:  {len(rgb_data)} bytes")
    if has_alpha:
        print(f"alpha annexb:{len(alpha_data)} bytes")
    print(f"container:   {len(container)} bytes")
    if args.emit in ("file", "both"):
        print(f"baf file:    {baf_path}")
    if args.emit in ("array", "both"):
        print(f"c array:     {c_path}  (symbol: {symbol})")


if __name__ == "__main__":
    main()
