# Animation → .baf Converter (BAF asset tools)

*English | [中文](README_CN.md)*

Convert **APNG / animated WebP / GIF / MP4 / MOV** in one step into **`.baf`**
(Beken Animation Format) — a dual H.264 stream (RGB + grayscale Alpha) animation
format with real alpha, consumed by the BK7259 BAF (`lv_baf`) widget.

## One-click usage

```bash
python3 to_baf.py --input <animation-file>
```

Auto-detects the input type and produces two outputs (in the input directory by default):

| Output | Description |
|---|---|
| `<name>.baf` | **Self-contained binary container** (header + RGB H.264 + Alpha H.264 + AU tables + per-frame durations); ship on filesystem/flash |
| `<name>_baf_asset.c` | **C asset** (`bk_baf_source_t`), compiled into firmware; usable on-device today |

Common options:
```bash
python3 to_baf.py --input my_anim.apng \
    --outdir out --name my_anim \
    --symbol my_anim_bk_baf_source   # C symbol name in the asset
    # --no-c    only emit .baf
    # --no-baf  only emit the C asset
```

### Using the C asset in a project

```c
extern const bk_baf_source_t my_anim_bk_baf_source;   /* from <name>_baf_asset.c */

lv_obj_t *anim = lv_baf_create(scr);
lv_baf_set_gpu_overlay(anim, true);              /* optional: GPU overlay */
lv_baf_set_src(anim, &my_anim_bk_baf_source);
```
Add `<name>_baf_asset.c` to your project `CMakeLists.txt` srcs.

> Note: the device currently consumes the **C asset** (compiled-in). The `.baf`
> binary container is also produced, but a **device-side runtime `.baf` loader is
> not yet implemented** (can be added later to load from filesystem/flash without
> rebuilding firmware).

## Supported inputs

- **PIL path** (per-frame + per-frame duration + alpha channel): `.png/.apng`, `.gif`, `.webp`
- **ffmpeg path** (video containers): `.mp4/.mov/.mkv/.webm/.m4v/.avi`; if the pixel
  format carries alpha (e.g. rgba/yuva) it is extracted via `alphaextract`, otherwise
  treated as opaque
- Dependencies: `python3 + Pillow`, `ffmpeg/ffprobe`

## Pipeline

```
source → extract per-frame RGB + Alpha(gray) + durations
       → H.264 encode RGB (no B-frames, refs=1) + H.264 encode Alpha as gray (optional)
       → Annex-B + per-frame AU split + AUD strip
       → pack .baf (binary) and _baf_asset.c (C array)
```
Alpha is a grayscale stream, matching the BK7259 BAF device decoder.

## .baf binary format (little-endian)

```
magic        char[8]  "BAFANIM1"
version      u32      = 1
width        u16      RGB width
height       u16      RGB height
alpha_width  u16      0 => same as width (full-res alpha)
alpha_height u16      0 => same as height
frame_count  u32
flags        u32      bit0 = HAS_ALPHA
rgb_size     u32
alpha_size   u32      0 if no alpha
reserved     u32[4]   = 0
--- variable sections ---
durations    u32[frame_count]                    per-frame duration (ms)
rgb_aus      (u32 offset,u32 size)[frame_count]  AU table into the rgb blob
alpha_aus    (u32 offset,u32 size)[frame_count]  only if HAS_ALPHA
rgb_data     u8[rgb_size]                         RGB H.264 Annex-B
alpha_data   u8[alpha_size]                       Alpha H.264 Annex-B (if any)
```

## Scripts here

| Script | Role |
|---|---|
| **`to_baf.py`** | ⭐ One-click entry (APNG/WebP/GIF/MP4/MOV → .baf + C asset) |
| `mp4_to_bk_baf_asset.py` | Packing core (Annex-B / AU split / AUD strip / C-asset generation), reused by `to_baf.py`; can also pack directly from RGB/Alpha mp4 files |
