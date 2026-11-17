# JPEG Decoding Example Project

* [中文](./README_CN.md)

## 1. Project Overview

This project demonstrates JPEG decoding on the Beken platform with two API layers:

| Layer | Functions / CLI | Description |
|-------|-----------------|-------------|
| Platform JPEG | `jpeg_decode jpegd` / `jpegd_flexa` | Platform JPEG decode demos |
| `bk_decoder` | `jpeg_decode vcdec_jpegd` / `vcdec_jpegd_frame_rgb` / `vcdec_jpegd_flexa` | JPEG decode via `bk_decoder` |

When `CONFIG_BK_DECODER` is enabled, `main()` runs a boot self-test: `vcdec_jpeg_flexa_test()` then `vcdec_jpeg_test()`.

* Developer guides:

  - [JPEG Decode (VPU)](../../../developer-guide/vpu/jpeg_decode.html)
  - [Frame vs Flexa Mode](../../../developer-guide/vpu/flexa_frame.html)

### 1.1 Test Environment

- Hardware: **BK7259_QF128_12.3X12.3_V4.0**, PSRAM 32M
- Input: built-in JPEG sample image
- Output: decode logs; `bk_decoder` paths print `[RESULT][PASS]` / `[RESULT][FAIL]`

.. warning::
    Use reference peripherals. Different specs may require code/config changes.

## 2. Directory Structure

```text
jpeg_decode_example/
├── ap/
│   ├── ap_main.c
│   └── jpeg_decode/
│       ├── include/
│       └── src/
│           ├── jpeg_decode_cli.c
│           └── vcdec_jpeg_test.c
├── cp/
├── partitions/
└── pj_config.mk
```

## 3. Features

- Boot-time `vcdec` self-test when `CONFIG_BK_DECODER=y`
- Single-task guard: only one decode task at a time
- Crop/scale cases report per-frame decode time (`[RESULT][SPEED]`)

Cases available under `CONFIG_BK_DECODER`:

| CLI subcommand | Mode | Content | Output |
|----------------|------|---------|--------|
| `vcdec_jpegd` | frame | full-frame decode | 176x144 NV12 |
| `vcdec_jpegd_flexa` | flexa | segmented decode + CPU stitching | 176x144 NV12 |
| `vcdec_jpegd_frame_rgb` | frame | PP output RGB565 / RGB888 | 176x144 RGB |
| `vcdec_jpegd_frame_scale` | frame | full-frame downscale (full / 3/4 / 1/2 / 1/4) | 1920x1080 / 1440x810 / 960x540 / 480x270 |
| `vcdec_jpegd_frame_crop` | frame | full-frame crop (full / 3/4 / 1/2 / 1/4) | 1920x1080 / 1440x810 / 960x540 / 480x270 |
| `vcdec_jpegd_frame_scale_crop` | frame | crop 3/4 x scale(1/1,1/2,1/4), crop 1/2 x scale(1/1,3/4,1/4), crop 1/4 x scale(1/1,3/4,1/2) | crop region x scale ratio |
| `vcdec_jpegd_flexa_crop` | flexa | crop (full / 3/4 / 1/2 / 1/4) | 1920x1080 / 1440x810 / 960x540 / 480x270 |
| `jpegd_uvc_gpu_display [crop]` | flexa | UVC MJPEG -> PP -> GPU -> LCD | 1080p / 1280x720 |

> `jpegd` / `jpegd_flexa` (platform JPEG) are legacy cases, disabled by default (`USE_LEGACY_JPEGD 0`).

## 4. Build And Run

### 4.1 Build

```bash
make bk7259 PROJECT=multimedia/jpeg_decode_example
```

### 4.2 CLI Commands

```text
jpeg_decode help
jpeg_decode vcdec_jpegd          # requires CONFIG_BK_DECODER
jpeg_decode vcdec_jpegd_frame_rgb # requires CONFIG_BK_DECODER
jpeg_decode vcdec_jpegd_flexa    # requires CONFIG_BK_DECODER
jpeg_decode vcdec_jpegd_frame_scale      # frame scale + timing
jpeg_decode vcdec_jpegd_frame_crop       # frame crop + timing
jpeg_decode vcdec_jpegd_frame_scale_crop # frame crop+scale + timing
jpeg_decode vcdec_jpegd_flexa_crop       # flexa crop + timing
jpeg_decode jpegd_uvc_gpu_display [crop]  # UVC MJPEG -> PP -> GPU -> LCD
jpeg_decode jpegd_uvc_gpu_display close           # stop the display pipeline
```

### 4.3 Pass Criteria

`bk_decoder` paths — search for `RESULT`:

```text
[RESULT][PASS] vcdec_jpeg_test success, rounds=5/5
[RESULT][PASS] vcdec_jpeg_frame_rgb_test success
[RESULT][PASS] vcdec_jpeg_flexa_test success, rounds=5/5
[RESULT][SPEED] vcdec_jpeg_frame_scale rounds=5 avg=xxxx us fps=xx.xx
```

`[RESULT][SPEED]` is the timing output of the crop/scale cases, see 4.5.

Platform `jpegd` paths have no unified `[RESULT]` line; absence of init/malloc errors is the usual pass indicator.

### 4.4 Integration Tests (`.it.csv`)

```text
ap_cmd jpeg_decode vcdec_jpegd
ap_cmd jpeg_decode vcdec_jpegd_frame_rgb
ap_cmd jpeg_decode vcdec_jpegd_flexa
ap_cmd jpeg_decode vcdec_jpegd_frame_scale
ap_cmd jpeg_decode vcdec_jpegd_frame_crop
ap_cmd jpeg_decode vcdec_jpegd_frame_scale_crop
ap_cmd jpeg_decode vcdec_jpegd_flexa_crop
```

### 4.5 Crop/Scale Timing

The following cases time each decode round with AON RTC (about ±30us accuracy, covering only
`bk_jpeg_decode_frame()`, excluding buffer clearing) and print one summary line per case. All of
them use the built-in **1920x1080 JPEG (215105 bytes, > 200KB, complex texture)** so that a single
frame takes long enough to be resolved by the 32K AON RTC; small-image cases such as
`vcdec_jpegd_frame_rgb` still use the 176x144 input.

| CLI subcommand | Mode | PP operation | Output | Timed |
|----------------|------|--------------|--------|-------|
| `vcdec_jpegd_frame_scale` | frame | scale full / 3/4 / 1/2 / 1/4 | 1920x1080 / 1440x810 / 960x540 / 480x270 | yes |
| `vcdec_jpegd_frame_crop` | frame | crop full / 3/4 / 1/2 / 1/4 | 1920x1080 / 1440x810 / 960x540 / 480x270 | yes |
| `vcdec_jpegd_frame_scale_crop` | frame | crop x scale combos (9 sets) | source size x {1/1, 3/4, 1/2, 1/4} | yes |
| `vcdec_jpegd_flexa_crop` | flexa | crop full / 3/4 / 1/2 / 1/4 | 1920x1080 / 1440x810 / 960x540 / 480x270 | yes |

The `full` set performs no PP scale/crop (straight 1920x1080 frame decode) and serves as the speed
baseline; the other three sets use 3/4, 1/2 and 1/4 of the source resolution. In the crop+scale
cases both the crop and the scale ratio are relative to the **source frame**, so the output size is
source size x scale ratio (larger crops combined with 1/1 therefore upscale): crop 3/4 x
{1/1, 1/2, 1/4}, crop 1/2 x {1/1, 3/4, 1/4},
crop 1/4 x {1/1, 3/4, 1/2}; output width/height are rounded down to even. The
crop 1/4 x 1/1 combination needs a 4x upscale, which exceeds the PP 3x scale
limit and is skipped at runtime; all other frame combinations run normally. That is
4 scale, 4 crop and 9 crop+scale sets, plus 4 flexa crop sets, 21 sub-cases
(20 executed) in total, each printing its own `[RESULT][SPEED]` line.

Log format:

```text
[RESULT][SPEED] <case_name> rounds=<n> avg=<us> us fps=<xx.xx>
```

- `avg`: average per-frame decode time in microseconds
- `fps`: theoretical frame rate derived from `avg`
- `rounds`: measured rounds, fixed to `VCDEC_JPEG_DECODE_CNT` (5); the first round includes
  decoder warm-up, so the average is slightly high
- Each sub-case prints its own line; `case_name` identifies the crop/scale combination

Actual numbers vary with chip revision, PSRAM bandwidth and cache configuration. Use the
board log as the reference.

## 5. Notes

1. `vcdec` commands and boot self-test require `CONFIG_BK_DECODER`.
2. Wait for boot self-test to finish before starting another `vcdec` command.
3. RGB565/RGB888 output conversion is covered by `vcdec_jpegd_frame_rgb` only; flexa mode
   does not support RGB output.
4. Flexa mode supports PP crop only.
5. Timing covers `bk_jpeg_decode_frame()` only; buffer clearing, buffer allocation and decoder
   open are excluded.
