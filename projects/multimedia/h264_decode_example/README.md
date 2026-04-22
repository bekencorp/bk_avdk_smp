# H264 Decoding Example Project

* [中文](./README_CN.md)

## 1. Project Overview

This project demonstrates H264 decoding on the Beken platform. The current implementation provides three main usage paths:

- Basic H264 decode CLI: `h264_decode`
- Standalone FLEXA decode demo: `h264_decode_flexa_test`
- Loop decode stress test with optional DMA pressure: `h264_decode_stress`

Unlike `jpeg_decode_example`, the current `h264_decode_example` `main()` only performs initialization and CLI registration. It does not start any decode test automatically at boot. All tests are triggered manually from the serial console.

* For more details about H264 decoding, refer to:

  - [H264 Decoding (SW) Overview](../../../developer-guide/video_codec/h264_decoding_sw.html)

### 1.1 Test Environment

- Hardware
  - Core board: `BK7258_QFN88_9X9_V3.2`
  - PSRAM: `8M/16M`
- Input
  - Built-in H264 demo stream
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
│   │   ├── include/
│   │   └── src/h264_decode_cli.c          # h264_decode CLI implementation
│   └── h264_decode_stress/
│       ├── include/
│       └── src/
│           ├── h264_decode_flexa_test.c   # Standalone FLEXA example
│           ├── h264_decode_stress.c       # H264 decode stress test
│           └── h264_decode_stress_stream.c
├── cp/
├── partitions/
└── pj_config.mk
```

## 3. Features

### 3.1 Currently Implemented Features

- `h264_decode h264d` for normal H264 decode demo
- `h264_decode h264d_flexa` for H264 FLEXA decode demo
- `h264_decode_flexa_test start` for a dedicated FLEXA demo thread
- `h264_decode_stress` for loop decode stress with optional DMA pressure

## 4. Build And Run

### 4.1 Build

```bash
make bk7259 PROJECT=multimedia/h264_decode_example
```

### 4.2 Run

After flashing the firmware, trigger tests manually from the serial console.

#### 4.2.1 CLI Command List

Basic decode commands:

```text
h264_decode help
h264_decode h264d
h264_decode h264d_flexa
```

Standalone FLEXA demo command:

```text
h264_decode_flexa_test start
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

`CMDRSP:OK` only means the CLI created the worker task successfully. The current H264 demos do not print a unified `[RESULT][PASS]` line, so the outcome must be judged from the task logs.

Typical guidance:

- `h264_decode h264d`
  - No immediate CLI error, and the decode flow keeps running normally
- `h264_decode h264d_flexa` / `h264_decode_flexa_test start`
  - No error such as `h264_decoder_init failed` or `h264_decoder_decode failed`
  - The task reaches `exit h264_decode_flexa_test`
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
failed to allocate ... buffer
psram_dma_stress_start failed, ret=...
```

## 5. Notes

1. The current `h264_decode` CLI parser only accepts `h264d` and `h264d_flexa`. Use the actual parser behavior in source code as the reference.
2. `h264_decode`, `h264_decode_flexa_test`, and `h264_decode_stress` all run tests in dedicated worker threads to avoid blocking the CLI thread.
3. The optional argument of `h264_decode_stress start [dma_copy_size_kb]` is in KB.
4. The stress path allocates extra output, stream, and DMA buffers, so sufficient memory is required.
5. If you need a demo with unified `[RESULT][PASS/FAIL]` logs, refer to the `vcdec` JPEG demos in `jpeg_decode_example`.
