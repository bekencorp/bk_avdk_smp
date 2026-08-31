# Animation → BAF v1 container converter

*English | [中文](README_CN.md)*

Convert **APNG / animated WebP / GIF / MP4 / MOV / MKV / WEBM** into a **BAF v1
container** (Beken Animation Format; normative spec `BAF_SPEC_CN.md` at the repo
root, on-device layout in `bk_baf/include/bk_baf_container.h`). The container holds
one H.264 stream (RGB) plus an optional **grayscale H.264 alpha stream**, forced to
`bframes=0 refs=1`, non-4:4:4 — aligned with the BK7259 VPU's hardware decode.

## Quick start

```bash
python3 to_baf.py --input <animation>
```

Auto-detects the input and, by default, emits **two byte-identical** forms of the
same container image (into the input's directory):

| Output | Purpose |
|---|---|
| `<name>.baf` | Binary container file — ship on filesystem/flash, load at runtime |
| `<name>_baf.c` | The **same bytes** as a C array: `const unsigned char <name>_baf[]` + `const unsigned int <name>_baf_size`, compiled into firmware |

Both are the same container image; the device parses either one with the **same**
`baf_parse_inplace()` (see `bk_baf_container.h`) — no conversion needed.

### Arguments

| Argument | Default | Meaning |
|---|---|---|
| `--input <path>` | (required) | Source animation (APNG/WebP/GIF/MP4/MOV/MKV/WEBM…) |
| `--outdir <dir>` | input's dir | Output directory |
| `--name <name>` | input stem | Output base name → `<name>.baf` / `<name>_baf.c` |
| `--emit {file,array,both}` | `both` | `.baf` only / C array only / both |
| `--symbol <sym>` | `<name>_baf` | C array symbol name |
| `--force-opaque-alpha` | off | Emit a full-resolution all-opaque alpha stream when the input has no alpha |

```bash
# .baf file only
python3 to_baf.py --input clip.mp4 --emit file --outdir out --name hello
# C array only, custom symbol
python3 to_baf.py --input hello.gif --emit array --symbol hello_baf
```

## Consuming it (both parsed inside bk_baf)

**① Compiled-in C array:**

```c
extern const unsigned char hello_baf[];        /* from <name>_baf.c */
extern const unsigned int  hello_baf_size;

/* LVGL widget */
lv_baf_set_src_data(anim, hello_baf, hello_baf_size);

/* or the bk_baf player directly */
bk_baf_decoder_t *d = bk_baf_open(&(bk_baf_config_t){
    .data = hello_baf, .data_len = hello_baf_size });
```

Add `<name>_baf.c` to your project's `CMakeLists.txt` srcs.

**② `.baf` file on filesystem/flash:**

```c
lv_baf_set_src_file(anim, "S:/baf/hello.baf");   /* read via lv_fs, parsed by bk_baf */
```

> Note: `bk_baf` **aliases the container bytes in place** (zero-copy) while parsing,
> so the array/buffer you pass must stay valid for the whole playback (a `const`
> flash array always does; for the file path `lv_baf` owns the buffer and frees it
> on close).

## Supported inputs

- **PIL path** (per-frame + duration + alpha): `.png/.apng`, `.gif`, `.webp`
- **ffmpeg path** (video containers): `.mp4/.mov/.mkv/.webm/.m4v/.avi`; if the pixel
  format carries alpha (rgba/yuva) it is `alphaextract`-ed automatically, otherwise
  treated as opaque
- Requires: `python3 + Pillow`, `ffmpeg / ffprobe`

## Pipeline

```
source -> extract per-frame RGB + Alpha(grayscale) + durations(ms)
       -> pad geometry to multiples of 16 (VPU macroblock requirement)
       -> H.264 encode RGB (no B-frames, refs=1) + H.264 encode Alpha as gray (optional)
       -> Annex-B + per-frame access-unit (AU) split + AUD strip
       -> pack into a BAF v1 container (64B FileHeader + ChunkDirectory + 64B-aligned IDX/DUR/DATA)
```

Alpha is grayscale-encoded (its Y plane is the A8 mask), matching the BK7259 BAF
device-side decode.

## Container format

Normative reference is **`BAF_SPEC_CN.md`** (repo root); the on-device C structs are
in **`bk_baf/include/bk_baf_container.h`**. In short: a 64-byte fixed `FileHeader`
(magic `"BAFANIM1"`, geometry, frame count, per-chunk directory indices,
`header_crc32`) + a flat `ChunkDirectory` (16B/entry `{type, offset, size, crc32}`) +
64B-aligned `'IDX '` (per-frame AU offset/size) / `'DUR '` (per-frame ms) / `'DATA'`
(H.264 Annex-B).

## Scripts here

| Script | Role |
|---|---|
| **`to_baf.py`** | Converter entry point (APNG/WebP/GIF/MP4/MOV… → BAF v1 `.baf` + `_baf.c`) |
| `mp4_to_bk_baf_asset.py` | Packing core (ffmpeg calls / Annex-B / AU split / AUD strip), reused by `to_baf.py` |
