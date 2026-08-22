#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
to_baf.py - one-click converter to the .baf (Beken Animation Format) format.

Input (auto-detected):  APNG / animated PNG, animated WebP, GIF, MP4 / MOV / MKV / WEBM
Output (both):
  1. <name>.baf         self-contained binary container (ship on filesystem/flash)
  2. <name>_baf_asset.c LVGL/BAF C asset (bk_baf_source_t) to compile into firmware

Pipeline:
  source
    -> extract per-frame RGB + Alpha (grayscale) + durations
    -> H.264 encode RGB (no B-frames, refs=1)  +  H.264 encode Alpha as gray (optional)
    -> Annex-B + per-frame access-unit (AU) split + AUD strip
    -> pack into .baf (binary) and _baf_asset.c (C array)

The alpha is encoded as a grayscale stream, matching the BK7259 BAF device
decoder.

--------------------------------------------------------------------------------
.baf binary layout (little-endian). All offsets/sizes in bytes.

  magic        char[8]  "BAFANIM1"
  version      u32      = 1
  width        u16      RGB width
  height       u16      RGB height
  alpha_width  u16      0 => same as width (full-res alpha)
  alpha_height u16      0 => same as height
  frame_count  u32
  flags        u32      bit0 = HAS_ALPHA
  rgb_size     u32      bytes of RGB Annex-B blob
  alpha_size   u32      bytes of Alpha Annex-B blob (0 if no alpha)
  reserved     u32[4]   = 0
  --- variable sections, in this order ---
  durations    u32[frame_count]                       per-frame duration (ms)
  rgb_aus      (u32 offset, u32 size)[frame_count]     AU table into rgb blob
  alpha_aus    (u32 offset, u32 size)[frame_count]     only if HAS_ALPHA
  rgb_data     u8[rgb_size]                            RGB H.264 Annex-B
  alpha_data   u8[alpha_size]                          Alpha H.264 Annex-B (if any)
--------------------------------------------------------------------------------
"""

import argparse
import json
import struct
import subprocess
import tempfile
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
BAF_VERSION = 1
BAF_FLAG_HAS_ALPHA = 1 << 0


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


# ------------------------------- .baf pack ---------------------------------
def write_baf(path: Path, width, height, alpha_width, alpha_height,
              durations, rgb_data, rgb_aus, alpha_data, alpha_aus):
    has_alpha = alpha_data is not None
    flags = BAF_FLAG_HAS_ALPHA if has_alpha else 0
    frame_count = len(durations)

    header = struct.pack(
        "<8sIHHHHIIII16x",
        BAF_MAGIC, BAF_VERSION,
        width, height, alpha_width, alpha_height,
        frame_count, flags,
        len(rgb_data), len(alpha_data) if has_alpha else 0,
    )
    body = bytearray()
    for d in durations:
        body += struct.pack("<I", d)
    for off, size in rgb_aus:
        body += struct.pack("<II", off, size)
    if has_alpha:
        for off, size in alpha_aus:
            body += struct.pack("<II", off, size)
    body += rgb_data
    if has_alpha:
        body += alpha_data
    path.write_bytes(header + bytes(body))


# --------------------------------- main ------------------------------------
def main() -> None:
    ap = argparse.ArgumentParser(
        description="One-click convert APNG/WebP/GIF/MP4/MOV to .baf (+ C asset).")
    ap.add_argument("--input", type=Path, required=True)
    ap.add_argument("--outdir", type=Path, default=None,
                    help="Output directory (default: input's directory).")
    ap.add_argument("--name", default=None,
                    help="Base name for outputs (default: input stem).")
    ap.add_argument("--symbol", default=None,
                    help="C symbol name (default: <name>_baf_source).")
    ap.add_argument("--no-c", action="store_true", help="Skip the C asset output.")
    ap.add_argument("--no-baf", action="store_true", help="Skip the .baf binary output.")
    ap.add_argument("--force-opaque-alpha", action="store_true",
                    help="Generate a full-resolution all-opaque alpha stream "
                         "when the input has no alpha channel.")
    args = ap.parse_args()

    src = args.input
    outdir = args.outdir or src.parent
    outdir.mkdir(parents=True, exist_ok=True)
    name = args.name or src.stem
    symbol = args.symbol or f"{name}_baf_source"
    baf_path = outdir / f"{name}.baf"
    c_path = outdir / f"{name}_baf_asset.c"

    with tempfile.TemporaryDirectory(prefix="to_baf_", dir=outdir) as tmp:
        tmp_dir = Path(tmp)
        rgb_dir = tmp_dir / "rgb"; rgb_dir.mkdir()
        alpha_dir = tmp_dir / "alpha"; alpha_dir.mkdir()

        width, height, durations, has_alpha = extract_frames(src, rgb_dir, alpha_dir)
        frame_count = len(durations)
        fps = 1000.0 / (sum(durations) / frame_count)
        if args.force_opaque_alpha and not has_alpha:
            opaque_alpha = Image.new("L", (width, height), 255)
            for index in range(frame_count):
                opaque_alpha.save(alpha_dir / f"alpha_{index:04d}.png")
            has_alpha = True

        rgb_mp4 = tmp_dir / "rgb.mp4"
        encode_h264(rgb_dir, "rgb_%04d.png", rgb_mp4, fps)
        rgb_data, rgb_aus = mp4_to_annexb_aus(rgb_mp4, tmp_dir, "rgb")

        alpha_data = alpha_aus = None
        if has_alpha:
            clean_alpha_frames(alpha_dir)   # snap extremes so opaque stays 255
            alpha_mp4 = tmp_dir / "alpha.mp4"
            encode_h264(alpha_dir, "alpha_%04d.png", alpha_mp4, fps, "gray", ALPHA_CRF)
            alpha_data, alpha_aus = mp4_to_annexb_aus(alpha_mp4, tmp_dir, "alpha")

        # sanity: one AU per frame
        if len(rgb_aus) != frame_count or (alpha_aus is not None and len(alpha_aus) != frame_count):
            raise RuntimeError(
                f"Frame/AU mismatch: frames={frame_count} rgb_aus={len(rgb_aus)} "
                f"alpha_aus={len(alpha_aus) if alpha_aus is not None else 0}")

        # alpha is full-res here (0 => same as RGB in both .baf and C asset)
        alpha_w = alpha_h = 0

        if not args.no_baf:
            write_baf(baf_path, width, height, alpha_w, alpha_h,
                      durations, rgb_data, rgb_aus, alpha_data, alpha_aus)
        if not args.no_c:
            bk_baf.write_asset(c_path, symbol, width, height,
                             rgb_data, alpha_data, rgb_aus, alpha_aus,
                             durations, alpha_w, alpha_h)

    print(f"input:       {src}")
    print(f"size:        {width}x{height}  frames: {frame_count}  fps: {fps:.2f}  alpha: {has_alpha}")
    print(f"rgb annexb:  {len(rgb_data)} bytes")
    if has_alpha:
        print(f"alpha annexb:{len(alpha_data)} bytes")
    if not args.no_baf:
        print(f"baf:         {baf_path}  ({baf_path.stat().st_size} bytes)")
    if not args.no_c:
        print(f"c asset:     {c_path}  (symbol: {symbol})")


if __name__ == "__main__":
    main()
