# JPEG Decoding Example Project

* [中文](./README_CN.md)

## 1. Project Overview

This project demonstrates JPEG decoding on the Beken platform with two API layers:

| Layer | Functions / CLI | Description |
|-------|-----------------|-------------|
| Platform JPEG | `jpeg_decode jpegd` / `jpegd_flexa` | Platform JPEG decode demos |
| VCDEC (`bk_decoder`) | `jpeg_decode vcdec_jpegd` / `vcdec_jpegd_frame_rgb` / `vcdec_jpegd_flexa` | JPEG decode via `bk_decoder` |

When `CONFIG_BK_DECODER` is enabled, `main()` runs a boot self-test: `vcdec_jpeg_flexa_test()` then `vcdec_jpeg_test()`.

Legacy `jpeg_decode_stress` was removed from the active tree. Archived sources: `verisilicon_nano/legacy/projects/jpeg_decode_example/` (see `verisilicon_nano/legacy/README.md`).

* Developer guides:

  - [JPEG Decode (VPU)](../../../developer-guide/vpu/jpeg_decode.html)
  - [Frame vs Flexa Mode](../../../developer-guide/vpu/flexa_frame.html)

### 1.1 Test Environment

- Hardware: **BK7259_QF128_12.3X12.3_V4.0**, PSRAM 32M
- Input: built-in JPEG sample image
- Output: decode logs; VCDEC paths print `[RESULT][PASS]` / `[RESULT][FAIL]`

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

- `jpeg_decode jpegd` / `jpegd_flexa` — platform JPEG decode
- `jpeg_decode vcdec_jpegd` / `vcdec_jpegd_flexa` — when `CONFIG_BK_DECODER` is enabled
- `jpeg_decode vcdec_jpegd_frame_rgb` — frame-mode PP RGB565/RGB888 output when `CONFIG_BK_DECODER` is enabled
- Boot-time `vcdec` self-test when `CONFIG_BK_DECODER=y`
- Single-task guard: only one decode task at a time

## 4. Build And Run

### 4.1 Build

```bash
make bk7259 PROJECT=multimedia/jpeg_decode_example
```

### 4.2 CLI Commands

```text
jpeg_decode help
jpeg_decode jpegd
jpeg_decode jpegd_flexa
jpeg_decode vcdec_jpegd          # requires CONFIG_BK_DECODER
jpeg_decode vcdec_jpegd_frame_rgb # requires CONFIG_BK_DECODER
jpeg_decode vcdec_jpegd_flexa    # requires CONFIG_BK_DECODER
```

### 4.3 Pass Criteria

VCDEC paths — search for `RESULT`:

```text
[RESULT][PASS] vcdec_jpeg_test success, rounds=5/5
[RESULT][PASS] vcdec_jpeg_frame_rgb_test success
[RESULT][PASS] vcdec_jpeg_flexa_test success, rounds=5/5
```

Platform `jpegd` paths have no unified `[RESULT]` line; absence of init/malloc errors is the usual pass indicator.

### 4.4 Integration Tests (`.it.csv`)

```text
ap_cmd jpeg_decode vcdec_jpegd
ap_cmd jpeg_decode vcdec_jpegd_frame_rgb
ap_cmd jpeg_decode vcdec_jpegd_flexa
```

## 5. Notes

1. `vcdec` commands and boot self-test require `CONFIG_BK_DECODER`.
2. Wait for boot self-test to finish before starting another `vcdec` command.
3. RGB565/RGB888 output conversion is covered by `vcdec_jpegd_frame_rgb` only; flexa mode
   does not support RGB output.
4. Legacy `jpeg_decode_stress` is archived under `verisilicon_nano/legacy/`.
