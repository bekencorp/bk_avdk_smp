# JPEG Encoding Example Project

* [中文](./README_CN.md)

## 1. Project Overview

Demonstrates **JPEG hardware encoding** on Beken via `bk_jpeg_encode_*` controllers (VCENC JPEG, `bk_encoder`), including Frame mode, software Flexa low-latency mode, OSD overlay tests, and the full OSD-encode → decode → LCD display path.

* Developer guide:

  - [JPEG Encode (VPU)](../../../developer-guide/vpu/jpeg_encode.html)
  - [Frame vs Flexa Mode](../../../developer-guide/vpu/flexa_frame.html)

The project provides:

- Frame mode: `jpeg_encode frame`
- Software Flexa: `jpeg_encode flexa`
- OSD overlay test: `jpeg_encode osd` (runs frame, sw flexa and sw frame with 8 channels: ARGB8888 / NV12 / Bitmap)
- Software frame OSD is covered by `jpeg_encode osd` and `jpeg_encode osd_display` (no separate command)
- OSD encode → decode → display: `jpeg_encode osd_display [frame|flexa|sw_frame|all]` (one mode at a time; OSD slot 0 shows HW FRAME / SW FLEXA / SW FRAME)
- Boot self-test when `CONFIG_BK_ENCODER` is enabled
- `.it.csv` integration tests

### 1.1 Test Environment

- Hardware: **BK7259_QF128_12.3X12.3_V4.0**, 32M PSRAM, HX8399C MIPI panel (1080×1920, reset GPIO_60, backlight GPIO_7)
- Input: built-in 256×128 NV12, tiled into **1920×1080** NV12 for encoding
- Output: JPEG bitstream; **10 frames** per run, quality level **5** for the hardware encoder and **75** for the software whole-frame encoder

.. warning::

    Use the reference board for evaluation. Different memory or resolution settings may require `defconfig` and test parameter changes.

## 2. Directory Layout

```text
jpeg_encode_example/
├── .it.csv
├── ap/jpeg_encode/
│   ├── common/jpeg_encode_nv12_256x128.*
│   ├── include/jpeg_encode_test.h, jpeg_encode_osd.h, jpeg_encode_display.h
│   └── src/jpeg_encode_cli.c, jpeg_encode_osd.c, jpeg_encode_display.c, bk_jpeg_encode_test.c
└── ...
```

## 3. Features

### 3.1 CLI

```text
jpeg_encode help
jpeg_encode frame          # bk_jpeg_encode_frame_new
jpeg_encode flexa          # bk_jpeg_encode_sw_flexa_new
jpeg_encode osd            # frame + sw flexa + sw frame, 8-channel OSD overlay
jpeg_encode osd_display            # run frame, flexa and sw_frame one by one
jpeg_encode osd_display frame      # hardware frame mode only (OSD shows HW FRAME)
jpeg_encode osd_display flexa      # software flexa mode only (OSD shows SW FLEXA)
jpeg_encode osd_display sw_frame   # software whole-frame mode only (OSD shows SW FRAME)
jpeg_encode osd_bench              # software OSD per-frame time vs no-OSD baseline (ARGB8888 + BITMAP), default 640x360
jpeg_encode osd_bench 1920x1080    # same benchmark on a 1920x1080 input (slower)
jpeg_encode osd_bench 1080p        # shorthand for 1920x1080
```

`CMDRSP:OK` means the test thread was created — check `[RESULT]` for pass/fail.

The OSD test runs the frame, software Flexa and software frame paths in sequence. Each mode installs
the same 8 overlays via `bk_jpeg_encode_set_osd` after `open`, laid out as 4 columns × 2 rows:
row0 holds ARGB8888 mode text (HW FRAME / SW FLEXA / SW FRAME), NV12 text, Bitmap text and ARGB8888 text;
row1 holds 4 ARGB8888 labels.
Frame and sw flexa use NV12 input; sw frame uses packed YUYV422 converted from the built-in NV12 tile.

The display test runs 5 rounds per selected mode (15 rounds for `all`). Each round encodes
one OSD frame into a 512KB buffer, feeds the JPEG to the flexa decoder (HSRAM ring, 3 segments × 16 lines),
and pushes the decoded NV12 through the GPU (rotate 90°, ARGB8888 compressed) to the DPU/HX8399C panel.
The last frame stays on screen for 3 seconds.

Software OSD composition is selected by `JPEG_SW_OSD_IN_PLACE` in `jpeg_encode_test.h` (or
`bk_jpeg_encode_sw_frame_config_t::osd_in_place` in the SDK):
- `1` (default): the input is writable, OSD is composited straight into the input frame with no
  full-frame copy — only the OSD rectangles cost time;
- `0`: the input must stay untouched, the frame is copied into an internal staging buffer first.

The software OSD benchmark runs on the software whole-frame path only and uses its own YUYV input
(the other commands keep the 1920×1080 input). The size is selectable at run time, so switching
between 640×360 (default) and 1920×1080 does not need a rebuild: `jpeg_encode osd_bench 1920x1080`
(or the `1080p` shorthand). Both dimensions must be multiples of 8, which BITMAP requires anyway.
Each case runs 1 warm-up frame plus 4 timed frames, measured with the platform microsecond timestamp
`bk_aon_rtc_get_us()`. The timed window is the
outer `bk_jpeg_encode_frame()` call up to its return, so the dcache flush, the OSD composite (including
the full-frame copy in `copy` mode) and the whole software encode are counted. A single block is laid
down from (0,0) at 1/16, 1/4, 9/16 per axis and the whole frame (each dimension rounded down to 8 pixels); every
size runs once as **ARGB8888** and once as **BITMAP** (1 bpp) so the two formats can be compared
directly — both formats cover exactly the same area. With the default 640×360 input that is
40×16 / 160×88 / 360×200 / 640×360; with `1080p` it is 120×64 / 480×264 / 1080×600 / 1920×1080.
Each case then runs once in `copy` mode and once in `inplace` mode, and a final summary
compares both modes against the no-OSD baseline.

#### Encoder and OSD processing per mode

| Mode | Encoder | OSD handled by | Input format | Notes |
| --- | --- | --- | --- | --- |
| `frame` (`bk_jpeg_encode_frame_new`) | Hardware JPEG encoder, whole frame | The OSD unit inside the hardware encoder, `bk_jpeg_encode_set_osd` programs vcenc directly | NV12 | Both encode and blend run in hardware; the CPU only submits the channel configuration |
| `flexa` (`bk_jpeg_encode_sw_flexa_new`) | Hardware JPEG encoder, advanced in 16-line slices | Same hardware OSD unit | NV12, flexa-aligned (1080 lines need a 1088-line buffer) | "sw" here only means software feeds the slices and signals `SET_FLEXA_LINES_READY`; encoding and blending are still hardware |
| `sw_frame` (`bk_jpeg_encode_sw_frame_new`) | Software tjpeg_enc, quality factor 75 | Software composite (`jpeg_encode_osd.c`, 16-line HSRAM strip over pixel pairs, shared by ARGB8888 and BITMAP) | YUYV422 | The only fully CPU path, OSD cost grows linearly with the OSD area, about 0.13 µs/pixel for ARGB8888, see the measurements below |
| `hw flexa` | Hardware JPEG encoder fed straight from the ISP | Hardware | ISP output | Requires the ISP (camera-to-encoder) path, not provided by this example |

The three OSD test chains differ only in `sw_frame`: for `frame` and `flexa` the OSD is composed by the
hardware encoder, so there is no CPU-side composite time to compare and `osd_bench` measures the
software whole-frame path only.

#### Measured OSD overhead

1920×1080 input (`jpeg_encode osd_bench 1080p`, 1 warm-up + 4 timed frames):

| OSD size | OSD pixels | no-OSD baseline | ARGB copy | BITMAP copy | ARGB inplace | BITMAP inplace |
| --- | --- | --- | --- | --- | --- | --- |
| none | 0 | 2548.4 ms | — | — | — | — |
| 120×64 (1/16 per axis) | 7680 | 2548.4 ms | 2632.5 ms (+3.3%) | 2632.1 ms (+3.3%) | 2549.6 ms (+0.0%) | 2549.1 ms (+0.0%) |
| 480×264 (1/4 per axis) | 126720 | 2548.4 ms | 2648.1 ms (+3.9%) | 2644.7 ms (+3.8%) | 2565.0 ms (+0.7%) | 2561.8 ms (+0.5%) |
| 1080×600 (9/16 per axis) | 648000 | 2548.4 ms | 2715.1 ms (+6.5%) | 2697.5 ms (+5.8%) | 2632.4 ms (+3.3%) | 2615.0 ms (+2.6%) |
| 1920×1080 (full) | 2073600 | 2548.4 ms | 2899.9 ms (+13.8%) | 2842.9 ms (+11.6%) | 2816.7 ms (+10.5%) | 2761.5 ms (+8.4%) |

640×360 input (bench default):

| OSD size | OSD pixels | no-OSD baseline | ARGB copy | BITMAP copy | ARGB inplace | BITMAP inplace |
| --- | --- | --- | --- | --- | --- | --- |
| none | 0 | 284.5 ms | — | — | — | — |
| 40×16 (1/16 per axis) | 640 | 284.5 ms | 293.8 ms (+3.3%) | 293.7 ms (+3.2%) | 284.5 ms (+0.0%) | 284.5 ms (+0.0%) |
| 160×88 (1/4 per axis) | 14080 | 284.5 ms | 295.2 ms (+3.8%) | 294.9 ms (+3.7%) | 286.1 ms (+0.6%) | 285.7 ms (+0.4%) |
| 360×200 (9/16 per axis) | 72000 | 284.5 ms | 303.5 ms (+6.7%) | 301.5 ms (+6.0%) | 294.4 ms (+3.5%) | 292.4 ms (+2.8%) |
| 640×360 (full) | 230400 | 284.5 ms | 322.4 ms (+13.3%) | 316.2 ms (+11.1%) | 313.5 ms (+10.2%) | 307.2 ms (+8.0%) |

Both runs agree: an `inplace` composite costs about **0.130 µs/pixel for ARGB8888** and
**0.100 µs/pixel for BITMAP**, independent of the frame size (both scale linearly with the OSD
area). BITMAP is roughly 21% cheaper because its source is 1 bpp and a fully set pixel pair is
written without any blending. `copy` adds one full-frame copy on top, about 9.0 ms at 640×360 and
82.8 ms at 1920×1080, identical for both formats.

### 3.2 Boot Self-Test

With `CONFIG_BK_ENCODER`, `vcenc_jpeg_run_boot_demo()` runs Frame then Flexa tests after a short delay.

## 4. Build and Run

```bash
cd <SDK_ROOT>
make bk7259 PROJECT=multimedia/jpeg_encode_example -j$(nproc)
```

Expected pass logs:

```text
[RESULT][PASS] bk_jpeg_encode_frame_test success, frames=10/10 encoded_size=...
[RESULT][PASS] bk_jpeg_encode_sw_flexa_test success, frames=10/10 encoded_size=...
[RESULT][PASS] bk_jpeg_encode_osd_frame_test success, frames=10/10 encoded_size=...
[RESULT][PASS] bk_jpeg_encode_osd_flexa_test success, frames=10/10 encoded_size=...
[RESULT][PASS] bk_jpeg_encode_osd_sw_frame_test success, frames=3/3 encoded_size=...
[RESULT][PASS] bk_jpeg_encode_osd_test success, frame=1 flexa=1 sw_frame=1
[RESULT][PASS] bk_jpeg_encode_osd_display_test success, mode=frame frames=5/5
[OSD-BENCH] mode=copy no_osd fmt=argb8888 osd=0x0 px=0 frames=4 avg=xxx.xms fps=x.x
[OSD-BENCH] mode=copy argb_360x200 fmt=argb8888 osd=360x200 px=72000 frames=4 avg=xxx.xms fps=x.x
[OSD-BENCH] overhead mode=copy argb_360x200 fmt=argb8888 vs no_osd: +x.xms (+x.x%)
[OSD-BENCH] mode=copy bitmap_360x200 fmt=bitmap osd=360x200 px=72000 frames=4 avg=xxx.xms fps=x.x
[OSD-BENCH] overhead mode=copy bitmap_360x200 fmt=bitmap vs no_osd: +x.xms (+x.x%)
[OSD-BENCH] mode=inplace argb_360x200 fmt=argb8888 osd=360x200 px=72000 frames=4 avg=xxx.xms fps=x.x
[OSD-BENCH] overhead mode=inplace argb_360x200 fmt=argb8888 vs no_osd: +x.xms (+x.x%)
[OSD-BENCH] mode=inplace bitmap_360x200 fmt=bitmap osd=360x200 px=72000 frames=4 avg=xxx.xms fps=x.x
[OSD-BENCH] overhead mode=inplace bitmap_360x200 fmt=bitmap vs no_osd: +x.xms (+x.x%)
[OSD-BENCH] ===== summary: sw_frame 640x360, average of one bk_jpeg_encode_frame() call =====
[OSD-BENCH] baseline no_osd: xxx.xms
[OSD-BENCH] argb_360x200 fmt=argb8888 px=72000 copy=xxx.xms(xxx.x% of base) inplace=xxx.xms(xxx.x% of base) copy-inplace=+x.xms
[OSD-BENCH] bitmap_360x200 fmt=bitmap px=72000 copy=xxx.xms(xxx.x% of base) inplace=xxx.xms(xxx.x% of base) copy-inplace=+x.xms
[RESULT][PASS] bk_jpeg_encode_osd_bench_test success, baseline=xxx.xms
```

`.it.csv` checks:

```text
ap_cmd jpeg_encode frame        # expect [RESULT][PASS] bk_jpeg_encode_frame_test success
ap_cmd jpeg_encode osd          # expect [RESULT][PASS] bk_jpeg_encode_osd_test success
ap_cmd jpeg_encode osd_display  # expect [RESULT][PASS] bk_jpeg_encode_osd_display_test success (needs HX8399C panel)
ap_cmd jpeg_encode osd_bench    # expect [RESULT][PASS] bk_jpeg_encode_osd_bench_test success
```

## 5. Configuration

| Item | Value |
|------|-------|
| Encode size | 1920×1080 |
| Source tile | 256×128 NV12 |
| Frames / quality | Frame/Flexa 10, sw frame 3 / level 5 for hardware, 75 for software |
| OSD channels | 8 (ARGB8888 / NV12 / Bitmap) across frame / sw flexa / sw frame |
| sw frame input | YUYV422 (converted from the built-in NV12 tile) |
| Display path | JPEG flexa decode → GPU (NV12→ARGB8888, rotate 90°, compress) → DPU HX8399C 1080×1920 |
| Display rounds / hold | 5 rounds per mode (15 for `all`), last frame held 3s |
| OSD benchmark | sw frame path, input size from the CLI (default 640×360, `1080p` shortcut), 1 warm-up + 4 timed frames per case; ARGB8888 and BITMAP over 1/16, 1/4, 9/16 per axis and the full frame, copy and in-place each |
| Software OSD compose | `JPEG_SW_OSD_IN_PLACE`=1 in-place (default, no copy); =0 full-frame copy into a staging buffer |
| Display configs | `CONFIG_DPU_DRIVER`, `CONFIG_DSI_DRIVER`, `CONFIG_BK_DISPLAY`, `CONFIG_MIPI_DSI`, `CONFIG_LCD_DSI`, `CONFIG_LCD_HX8399C_MIPI_1080x1920`, `CONFIG_VG_LITE_GPU`, `CONFIG_APP_GPU`, `CONFIG_BK_VCDEC`, `CONFIG_BK_DECODER` |

Hardware Flexa (`bk_jpeg_encode_hw_flexa_new`) is not covered here — see VPU docs for ISP-bound pipelines.
`jpeg_encode osd_display` requires the HX8399C MIPI panel and its GPIOs (reset GPIO_60, backlight GPIO_7);
without the panel only this command fails, the other commands are unaffected.
