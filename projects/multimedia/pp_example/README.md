# PP Post-Processor Example Project

* [中文](./README_CN.md)

## 1. Project Overview

This project demonstrates the **memory-in / memory-out** usage of the **PP
(Post-Processor)** module on the Beken platform: an external NV12 buffer is fed into
the PP hardware to validate its **scaling** and **format-conversion** capability
(NV12 → NV12 / RGB565 / RGB888).

All capabilities are exposed through the single `pp` CLI (8 cases in total); each case
runs **3 rounds** by default (`PP_EXAMPLE_ROUNDS`) and all rounds must succeed to pass.

PP shares the same VCDec hardware block with the H.264 / JPEG decoders, but this
project tests the **PP module itself**, not the decode flow. The difference:

| | Decode examples (e.g. `h264_decode_example`) | This project `pp_example` |
| --- | --- | --- |
| Under test | Decoder (PP on the decode side path) | **PP module** |
| How PP runs | Automatically in the decode pipeline | `bk_pp_process()` standalone call |
| Input | Bitstream through decoder core | External NV12 buffer (H.264 decode here generates it) |
| Typical API | `bk_h264_decode_frame()` + `out_width/out_format` | `bk_pp_process()` + `in/out_width/format` |
| Use case | End-to-end decode + scale / format | PP on an existing NV12 frame |

`bk_h264_decode` in this project **only prepares the 1280×720 test NV12**; it is not
part of the code under test. In a real product the NV12 can come from a decoder, camera
ISP, shared memory, etc., as long as it satisfies the PP bus addressing rules. The
PP module and the H.264 decode module share the same VCDec hardware, so the two
**cannot be used at the same time**.

### 1.1 Test Environment

- Hardware: **BK7259_QF128_12.3X12.3_V4.0**, PSRAM 32M, HX8399C 1080×1920 MIPI panel
- Input: built-in 1280×720 H.264 (1I30P) stream; the first decoded frame is the test NV12
- Output: PP logs and `[RESULT][PASS]` lines; flushed to the MIPI panel when display is enabled

.. warning::
    Use reference peripherals. Different specs may require code/config changes.

## 2. Directory Structure

```text
pp_example/
├── ap/
│   ├── ap_main.c                    # AP entry, registers the pp CLI and starts the boot demo
│   ├── config/bk7259_ap/            # AP-side defconfig / GPIO config
│   ├── Kconfig.projbuild            # Project config (PP_EXAMPLE_ENABLE_MIPI_DISPLAY)
│   └── pp/
│       ├── include/
│       │   ├── pp_config.h          # Compile-time config (display resolution / rotation)
│       │   ├── pp_test.h            # Case entries / source-size declarations
│       │   ├── pp_dpu.h             # DPU flush wrapper declarations
│       │   └── pp_gpu_blit.h        # GPU strip-blit wrapper declarations
│       └── src/
│           ├── pp_cli.c             # pp CLI dispatch
│           ├── pp_test.c            # PP cases (decode input prep + bk_pp_process + display)
│           ├── pp_dpu.c             # DPU decompress + flush to MIPI panel
│           └── pp_gpu_blit.c        # GPU scale / rotate
├── cp/
├── partitions/
└── .it.csv
```

## 3. Features

### 3.1 Data Path

1. Decode the first frame of the embedded **1280×720 H.264 (1I30P)** stream to NV12
   (`bk_h264_decode`, input preparation only).
2. Feed that NV12 buffer into the PP controller for scaling and/or RGB output.
3. When display is enabled, the PP output is scaled / rotated by the GPU and flushed to
   the MIPI panel via the DPU (see [3.3](#33-display-path)).

Public API (`components/bk_decode/bk_pp_ctlr.h`, a handle-based controller like `h264d`):

- Lifecycle: `bk_pp_ctlr_new()` → `bk_pp_init()` → `bk_pp_open()` →
  `bk_pp_process()` (repeatable) → `bk_pp_close()` → `bk_pp_deinit()` → `bk_pp_delete()`.
- `bk_pp_ctlr_new()` takes a `bk_pp_config_t` (`timeout_ms` / callback); `bk_pp_process()`
  takes a `bk_pp_process_req_t` (input / output buffers and sizes, with `out_width/out_height`
  = 0 meaning same as input).
- The controller brings up H26D power / IRQ and calls the vcdec `vcdec_pp_process()`
  underneath; the application never touches the PP registers directly.

### 3.2 Test Cases

| Case (CLI sub-command) | Input | Output | Description |
| --- | --- | --- | --- |
| `nv12_rgb565` | 1280×720 NV12 | 1280×720 RGB565 | format conversion |
| `nv12_rgb888` | 1280×720 NV12 | 1280×720 RGB888 | format conversion |
| `nv12_scale_down` | 1280×720 NV12 | 640×360 NV12 | down-scale only (1/2) |
| `nv12_scale_up` | 1280×720 NV12 | 1920×1080 NV12 | up-scale only (1.5×) |
| `nv12_rgb565_down` | 1280×720 NV12 | 640×360 RGB565 | scale + format |
| `nv12_rgb888_down` | 1280×720 NV12 | 640×360 RGB888 | scale + format |
| `nv12_rgb565_up` | 1280×720 NV12 | 1920×1080 RGB565 | scale + format |
| `nv12_rgb888_up` | 1280×720 NV12 | 1920×1080 RGB888 | scale + format |

### 3.3 Display Path

The PP output (NV12 / RGB565 / RGB888) is scaled to 1088×1920 and rotated 90° by a GPU
strip blit, then decompressed by the DPU and flushed to the HX8399C MIPI panel — the same
display path as `start_dec_scale_cvt` in `h264d_gpu_display_example`. Disable
`PP_EXAMPLE_ENABLE_MIPI_DISPLAY` (`CONFIG_PP_EXAMPLE_ENABLE_MIPI_DISPLAY`) via `menuconfig`
to run PP processing only, without any display.

## 4. Build And Run

### 4.1 Build

```bash
CCACHE_DISABLE=1 make bk7259 PROJECT=multimedia/pp_example -j32
```

### 4.2 CLI Commands

Type `pp <case>` on the AP console:

```text
pp help
pp nv12_rgb565
pp nv12_rgb888
pp nv12_scale_down
pp nv12_scale_up
pp nv12_rgb565_down
pp nv12_rgb888_down
pp nv12_rgb565_up
pp nv12_rgb888_up
```

Success / failure of command submission: `CMDRSP:OK` / `CMDRSP:ERROR`.

1s after boot (when `CONFIG_BK_DECODER` is enabled) the `nv12_rgb565_down` boot demo runs
once and, after the PP conversion, is flushed to the MIPI panel via the GPU / DPU. The boot
demo and manual CLI share the PP hardware, so wait for the boot demo to finish before
triggering cases manually.

### 4.3 How To Judge Pass Or Fail

`CMDRSP:OK` only means the CLI created the worker thread successfully; it does not mean the
test passed. At the end of a run, check the result line (the logged case name carries a
`pp_` prefix):

```text
[RESULT][PASS] pp_<case> success, rounds=3/3      # e.g. pp_nv12_rgb565_down
```

On failure the matching `[RESULT][FAIL] pp_<case> failed at <stage>, ret=...` line is
printed, where `<stage>` may be `decode_nv12`, `alloc_out_buf`, `pp_process`, `display`, etc.

## 5. Notes

1. All cases and the boot demo run in dedicated worker threads to avoid blocking the CLI
   thread; only one PP test thread is allowed at a time.
2. Each case runs 3 rounds by default (`PP_EXAMPLE_ROUNDS`); all must succeed to be marked PASS.
3. For RGB output the row height is aligned to 16 (NV12 to 2); the output buffer size is
   computed accordingly, so do not size it from the raw dimensions.
4. All buffers must be in PP-bus-accessible memory (PSRAM, allocated with
   `bk_frame_buffer_malloc`); PP reads NV12 from memory via `HWIF_PP_E` and `HWIF_PP_IN_*`,
   and writes back the scaled / converted result.
5. The default `bk7259_ap` config enables `CONFIG_BK_DECODER=y`, `CONFIG_FRAME_BUFFER=y`,
   and the display-related `CONFIG_DPU_DRIVER` / `CONFIG_MIPI_DSI` / `CONFIG_VG_LITE_GPU`, etc.
6. Disabling `CONFIG_PP_EXAMPLE_ENABLE_MIPI_DISPLAY` lets you validate PP processing on a
   board with no panel.
7. `bk_h264_decode` is only used to generate the test NV12 and is not part of the PP path
   under test; a real product can replace it with any NV12 source that meets the PP bus
   addressing rules.
