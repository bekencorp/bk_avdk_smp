# JPEG Encoding Example Project

* [中文](./README_CN.md)

## 1. Project Overview

Demonstrates **JPEG hardware encoding** on Beken via `bk_jpeg_encode_*` controllers (VCENC JPEG, `bk_encoder`), including Frame mode and software Flexa low-latency mode.

* Developer guide:

  - [JPEG Encode (VPU)](../../../developer-guide/vpu/jpeg_encode.html)
  - [Frame vs Flexa Mode](../../../developer-guide/vpu/flexa_frame.html)

The project provides:

- Frame mode: `jpeg_encode frame`
- Software Flexa: `jpeg_encode flexa`
- Boot self-test when `CONFIG_BK_ENCODER` is enabled
- `.it.csv` integration tests

### 1.1 Test Environment

- Hardware: **BK7259_QF128_12.3X12.3_V4.0**, 32M PSRAM
- Input: built-in 256×128 NV12, tiled into **1920×1080** NV12 for encoding
- Output: JPEG bitstream; **10 frames** per run, quality level **5**

.. warning::

    Use the reference board for evaluation. Different memory or resolution settings may require `defconfig` and test parameter changes.

## 2. Directory Layout

```text
jpeg_encode_example/
├── .it.csv
├── ap/jpeg_encode/
│   ├── common/jpeg_encode_nv12_256x128.*
│   └── src/jpeg_encode_cli.c, bk_jpeg_encode_test.c
└── ...
```

## 3. Features

### 3.1 CLI

```text
jpeg_encode help
jpeg_encode frame          # bk_jpeg_encode_frame_new
jpeg_encode flexa          # bk_jpeg_encode_sw_flexa_new
```

`CMDRSP:OK` means the test thread was created — check `[RESULT]` for pass/fail.

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
```

## 5. Configuration

| Item | Value |
|------|-------|
| Encode size | 1920×1080 |
| Source tile | 256×128 NV12 |
| Frames / quality | 10 / level 5 |

Hardware Flexa (`bk_jpeg_encode_hw_flexa_new`) is not covered here — see VPU docs for ISP-bound pipelines.
