# H264 Decoding Example Project

* [中文](./README_CN.md)

## 1. Project Overview

This project demonstrates H264 decoding on the Beken platform. The current implementation provides the following usage paths:

- VCDEC CLI based on `bk_decoder`: `h264_decode vcdec_h264d` / `h264_decode vcdec_h264d_flexa`
- Legacy H264D FLEXA demo CLI: `h264_decode_flexa start`
- Direct-register VCDEC CLI: `vcdec_h264_driver`
- Loop decode stress test with optional DMA pressure: `h264_decode_stress`
- Shared Annex-B H264 stream parser used by the VCDEC cases

In addition to initialization and CLI registration, the current `main()` also starts a boot demo thread when `CONFIG_BK_DECODER` is enabled. That boot demo runs `vcdec_h264_flexa_test` once and then `vcdec_h264_test` once. The default project `defconfig` enables `CONFIG_BK_DECODER`.

* For more details about H264 decoding, refer to:

  - [H264 Decoding (SW) Overview](../../../developer-guide/video_codec/h264_decoding_sw.html)

### 1.1 Test Environment

- Hardware
  - Core board: **BK7259_QF128_12.3X12.3_V4.0**
  - PSRAM: 32M
- Input
  - Built-in H264 demo streams
- Output
  - Decode flow logs
  - FLEXA callback logs
  - Stress-test round/exit logs

.. warning::
    Please use the reference peripherals when evaluating this demo. If the peripheral specification is different, corresponding code and configuration changes may be required.

## 2. Directory Structure

The project uses an AP-CP dual-core layout, with the main logic on the AP side:

```text
h264_decode_example/
├── README.md
├── README_CN.md
├── ap/
│   ├── ap_main.c                           # AP entry, registers H264-related CLI commands
│   ├── h264_decode/
│   │   ├── common/
│   │   │   ├── h264_decode_h264_parser.c   # Shared Annex-B AU / frame type parser
│   │   │   ├── h264_decode_h264_parser.h
│   │   │   ├── h264_decode_stream_1280x720.c
│   │   │   └── h264_decode_stream_256x128.c
│   │   ├── include/
│   │   └── src/
│   │       ├── h264_decode_cli.c           # h264_decode CLI implementation
│   │       ├── h264_decode_flexa_test.c    # Legacy FLEXA CLI example
│   │       ├── vcdec_h264_driver_test.c    # Direct-register VCDEC CLI
│   │       └── vcdec_h264_test.c           # bk_decoder VCDEC examples and boot demo
│   └── h264_decode_stress/
│       ├── include/
│       └── src/
│           ├── h264_decode_stress.c        # H264 decode stress test
│           └── h264_decode_stress_stream.c
├── cp/
├── partitions/
└── .it.csv
```

## 3. Features

### 3.1 Currently Implemented Features

- `h264_decode vcdec_h264d [1280x720|256x128]` for VCDEC frame-mode decode based on `bk_decoder`
- `h264_decode vcdec_h264d_flexa [1280x720|256x128]` for VCDEC FLEXA decode based on `bk_decoder`
- `h264_decode_flexa start` for the legacy `h264_decoder_*` FLEXA demo
- `vcdec_h264_driver frame|flexa [1280x720|256x128]` for direct-register VCDEC verification
- `h264_decode_stress` for loop decode stress with optional DMA pressure
- Shared H264 parser for Annex-B access units, frame type detection, and frame table generation; it is reused by `vcdec_h264_test.c` and `vcdec_h264_driver_test.c`
- Automatic boot demo: when `CONFIG_BK_DECODER=y`, the firmware automatically runs `vcdec_h264_flexa_test(1280x720)` and `vcdec_h264_test(1280x720)` once after boot

> Note: the previously added `h264d_native_frame_test.c` and `h264d_native_flexa_test.c` cases have been removed, and the related `h264d_native*` CLI entries have been deleted.

## 4. Build And Run

### 4.1 Build

```bash
make bk7259 PROJECT=multimedia/h264_decode_example -j32
```

### 4.2 Run

After flashing the firmware, if `CONFIG_BK_DECODER=y`, the system automatically triggers one VCDEC boot demo at startup. Other tests can be triggered manually from the serial console.

#### 4.2.1 CLI Command List

Basic decode commands:

```text
h264_decode help
h264_decode vcdec_h264d
h264_decode vcdec_h264d 256x128
h264_decode vcdec_h264d_flexa
h264_decode vcdec_h264d_flexa 256x128
h264_decode_flexa start
```

Direct-register VCDEC commands:

```text
vcdec_h264_driver frame
vcdec_h264_driver frame 256x128
vcdec_h264_driver flexa
vcdec_h264_driver flexa 256x128
```

Stress-test commands:

```text
h264_decode_stress start
h264_decode_stress start 512
h264_decode_stress stop
h264_decode_stress dma_open
h264_decode_stress dma_close
```

Notes:

- `start` uses the default DMA copy size
- `start 512` is an example that sets `dma_copy_size_kb`
- `dma_open` / `dma_close` can be used to manage the DMA pressure source separately

Successful command submission returns:

```text
CMDRSP:OK
```

Failed command submission returns:

```text
CMDRSP:ERROR
```

#### 4.2.2 How To Judge Pass Or Fail

`CMDRSP:OK` only means the CLI created the worker task successfully. Pass criteria depend on the path being tested:

- `h264_decode vcdec_h264d` / `h264_decode vcdec_h264d_flexa`
  - The run ends with logs like:

```text
[RESULT][PASS] vcdec_h264_test success, decoded_aus=..., rounds=...
[RESULT][PASS] vcdec_h264_flexa_test success, decoded_aus=..., rounds=...
```

- `h264_decode_flexa start`
  - No error such as `h264_decoder_init failed` or `h264_decoder_decode failed`
  - The task reaches `exit h264_decode_flexa_test`

- `vcdec_h264_driver frame` / `vcdec_h264_driver flexa`
  - No error such as `vcdec_h264_* failed`, `unexpected output size`, or `all decoded outputs sampled as zero`
  - The run ends with logs like:

```text
vcdec h264 frame test PASS: stream=1280x720 aus=... frame_cb=... flexa_cb=... last_wr=...
vcdec_h264_test_thread exit, mode=frame stream=1280x720 ret=0
```

- `h264_decode_stress`
  - No allocation/init/decode failure log is printed while running
  - After `h264_decode_stress stop`, the task exits with a log like:

```text
h264 decode stress thread exit, rounds=123 stop=1
```

Common failure logs include:

```text
h264_decoder_init failed, ret=...
h264_decoder_decode failed at round=... ret=...
vcdec_h264_decode_frame failed, ret=...
vcdec_h264_get_info failed, au=...
unexpected output size ... expect ...
all decoded outputs sampled as zero
failed to allocate ... buffer
psram_dma_stress_start failed, ret=...
```

#### 4.2.3 Integration Test Commands

`.it.csv` currently covers both supported VCDEC H264 streams and both decode modes:

```text
ap_cmd h264_decode vcdec_h264d 1280x720
ap_cmd h264_decode vcdec_h264d_flexa 1280x720
ap_cmd h264_decode vcdec_h264d 256x128
ap_cmd h264_decode vcdec_h264d_flexa 256x128
```

The expected result strings are the corresponding `[RESULT][PASS] vcdec_h264_test success` or `[RESULT][PASS] vcdec_h264_flexa_test success` logs.

## 5. Notes

1. The current `h264_decode` CLI enables `vcdec_h264d` and `vcdec_h264d_flexa` by default. Both optionally accept `1280x720` or `256x128` as stream arguments.
2. `h264_decode_flexa start` is a separate CLI command for the legacy `h264_decoder_*` FLEXA demo.
3. `h264_decode`, `h264_decode_flexa`, `vcdec_h264_driver`, `h264_decode_stress`, and the boot demo all run in dedicated worker threads to avoid blocking the CLI thread.
4. The optional argument of `h264_decode_stress start [dma_copy_size_kb]` is in KB.
5. Both `vcdec_h264_driver` and `h264_decode vcdec_h264d*` include two built-in streams: `1280x720` and `256x128`. If no stream argument is given, they default to `1280x720`.
6. The stress path allocates extra output, stream, and DMA buffers, so sufficient memory is required.
7. The default `bk7259_ap` configuration enables `CONFIG_BK_DECODER=y`, so a VCDEC boot demo log is expected on first boot.
8. GPIO debug pulses used for performance analysis have been removed. Normal runs no longer drive P30-P39 or P34/P36 as decode-stage markers.
