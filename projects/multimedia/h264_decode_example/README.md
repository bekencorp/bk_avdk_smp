# H264 Decoding Example Project

* [中文](./README_CN.md)

## 1. Project Overview

This project demonstrates H264 decoding on the Beken platform using the **`bk_decoder` / VCDEC** stack.

Supported paths:

| Path | CLI | Description |
|------|-----|-------------|
| VCDEC frame mode | `h264_decode vcdec_h264d [1280x720\|256x128]` | `bk_decoder` frame decode |
| VCDEC FLEXA mode | `h264_decode vcdec_h264d_flexa [1280x720\|256x128]` | `bk_decoder` FLEXA decode |
| Direct register | `vcdec_h264_driver frame\|flexa [1280x720\|256x128]` | Low-level VCDEC verification |

When `CONFIG_BK_DECODER` is enabled, `main()` runs a boot demo: `vcdec_h264_flexa_test(1280x720)` then `vcdec_h264_test(1280x720)`. Default `defconfig` enables `CONFIG_BK_DECODER`.

Legacy `h264_decode_flexa` and `h264_decode_stress` were removed from the active tree. Archived sources: `verisilicon_nano/legacy/projects/h264_decode_example/` (restore guide in `verisilicon_nano/legacy/README.md`).

* Developer guide: [H264 Decoding (SW) Overview](../../../developer-guide/video_codec/h264_decoding_sw.html)

### 1.1 Test Environment

- Hardware: **BK7259_QF128_12.3X12.3_V4.0**, PSRAM 32M
- Input: built-in H264 streams (`1280x720`, `256x128`)
- Output: decode logs and `[RESULT][PASS]` lines

.. warning::
    Use reference peripherals. Different specs may require code/config changes.

## 2. Directory Structure

```text
h264_decode_example/
├── ap/
│   ├── ap_main.c
│   └── h264_decode/
│       ├── common/          # Annex-B parser + built-in streams
│       └── src/
│           ├── h264_decode_cli.c
│           ├── vcdec_h264_driver_test.c
│           └── vcdec_h264_test.c   # VCDEC demos + boot demo
├── cp/
├── partitions/
└── .it.csv
```

## 3. Features

- `h264_decode vcdec_h264d` / `vcdec_h264d_flexa` with optional stream argument
- `vcdec_h264_driver frame` / `flexa` for register-level tests
- Shared Annex-B parser reused by VCDEC test cases
- Boot demo when `CONFIG_BK_DECODER=y`

## 4. Build And Run

### 4.1 Build

```bash
make bk7259 PROJECT=multimedia/h264_decode_example -j32
```

### 4.2 CLI Commands

```text
h264_decode help
h264_decode vcdec_h264d [1280x720|256x128]
h264_decode vcdec_h264d_flexa [1280x720|256x128]
vcdec_h264_driver frame [1280x720|256x128]
vcdec_h264_driver flexa [1280x720|256x128]
```

Success / failure of command submission: `CMDRSP:OK` / `CMDRSP:ERROR`.

### 4.3 Pass Criteria

- VCDEC paths: log contains `[RESULT][PASS] vcdec_h264_test success` or `vcdec_h264_flexa_test success`
- Driver path: ends with `vcdec h264 frame test PASS` or equivalent, `ret=0`

### 4.4 Integration Tests (`.it.csv`)

```text
ap_cmd h264_decode vcdec_h264d 1280x720
ap_cmd h264_decode vcdec_h264d_flexa 1280x720
ap_cmd h264_decode vcdec_h264d 256x128
ap_cmd h264_decode vcdec_h264d_flexa 256x128
```

## 5. Notes

1. `h264_decode` and `vcdec_h264_driver` run in worker threads; `CMDRSP:OK` only means the task was created.
2. Default stream is `1280x720` when no argument is given.
3. Boot demo and manual CLI share decoder hardware — wait for boot demo to finish before manual tests.
4. Legacy stress/flexa CLIs are archived; restore via `verisilicon_nano/legacy/README.md` if needed.
