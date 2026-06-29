# JPEG Decoding Example Project

* [中文](./README_CN.md)

## 1. Project Overview

This project demonstrates JPEG decoding on the Beken platform. It contains two kinds of JPEG demo paths:

- Platform JPEG decode demos: `jpeg_decoder_test()` / `jpeg_decoder_flexa_test()`
- `vcdec` JPEG decode demos: `vcdec_jpeg_test()` / `vcdec_jpeg_flexa_test()`

The project exposes serial CLI commands for normal decode, FLEXA decode, and stress testing. When `CONFIG_BK_DECODER` is enabled, the system also launches a boot-time `vcdec` self-test task that runs both FLEXA mode and full-frame mode automatically.

* Developer guide:

  - [JPEG Decode (VPU)](../../../developer-guide/vpu/jpeg_decode.html)
  - [Frame vs Flexa Mode](../../../developer-guide/vpu/flexa_frame.html)

### 1.1 Test Environment

- Hardware
  - Core board: **BK7259_QF128_12.3X12.3_V4.0**
  - PSRAM: 32M
- Input
  - Built-in JPEG sample image
- Output
  - Decode flow logs
  - `vcdec` result logs
  - Stress-test runtime logs

.. warning::
    Please use the reference peripherals when evaluating this demo. If the peripheral specification is different, corresponding code and configuration changes may be required.

## 2. Directory Structure

The project uses an AP-CP dual-core layout, with the main logic on the AP side:

```text
jpeg_decode_example/
├── README.md
├── README_CN.md
├── ap/
│   ├── ap_main.c                    # AP entry, CLI registration, optional boot-time vcdec self-test
│   ├── jpeg_decode/
│   │   ├── include/
│   │   └── src/
│   │       ├── jpeg_decode_cli.c    # jpeg_decode CLI implementation
│   │       └── vcdec_jpeg_test.c    # vcdec JPEG frame/FLEXA demos
│   └── jpeg_decode_stress/
│       ├── include/
│       └── src/jpeg_decode_stress.c # JPEG decode stress test
├── cp/
├── partitions/
└── pj_config.mk
```

## 3. Features

### 3.1 Currently Implemented Features

- `jpeg_decode` CLI for normal JPEG decode and FLEXA JPEG decode
- `jpeg_decode` CLI for `vcdec` JPEG frame/FLEXA decode when `CONFIG_BK_DECODER` is enabled
- `jpeg_decode_stress` CLI for JPEG decode stress testing in normal mode or FLEXA mode
- Boot-time `vcdec` self-test when `CONFIG_BK_DECODER` is enabled

## 4. Build And Run

### 4.1 Build

```bash
make bk7259 PROJECT=multimedia/jpeg_decode_example
```

### 4.2 Run

After flashing the firmware, use the serial console to observe boot logs or trigger commands manually.

#### 4.2.1 Boot-Time Auto Test

When `CONFIG_BK_DECODER` is enabled, `main()` calls `vcdec_jpeg_run_boot_demo()`. That function creates a dedicated worker task, and the task runs:

```text
vcdec_jpeg_flexa_test()
vcdec_jpeg_test()
```

There is a short delay between the two phases. It is recommended to wait until the boot-time self-test finishes before manually starting another `vcdec` JPEG command, so the single decoder instance is not used concurrently.

#### 4.2.2 Serial CLI Commands

`jpeg_decode` currently supports:

```text
jpeg_decode help
jpeg_decode jpegd
jpeg_decode jpegd_flexa
```

When `CONFIG_BK_DECODER` is enabled, the following commands are also available:

```text
jpeg_decode vcdec_jpegd
jpeg_decode vcdec_jpegd_flexa
```

`jpeg_decode_stress` currently supports:

```text
jpeg_decode_stress start
jpeg_decode_stress start none
jpeg_decode_stress start flexa
jpeg_decode_stress stop
```

Notes:

- `start` and `start none` run the stress test in normal mode
- `start flexa` runs the stress test in FLEXA mode
- The CLI only creates a background task; the real decode work runs in that task

Successful command submission returns:

```text
CMDRSP:OK
```

Failed command submission returns:

```text
CMDRSP:ERROR
```

#### 4.2.3 How To Judge Pass Or Fail

`CMDRSP:OK` only means the task was created successfully. It does not mean decoding has already passed.

The `vcdec` JPEG demos provide unified result lines. Search the serial log for `RESULT`, `PASS`, or `FAIL`.

Pass examples:

```text
[RESULT][PASS] vcdec_jpeg_test success, rounds=5/5
[RESULT][PASS] vcdec_jpeg_flexa_test success, rounds=5/5
```

Fail examples:

```text
[RESULT][FAIL] vcdec_jpeg_test failed at decode_frame, ret=-6, rounds=2/5
[RESULT][FAIL] vcdec_jpeg_flexa_test failed at register_bond, ret=-1, rounds=0/5
```

Field meanings:

- `failed at ...`: failure stage, such as `get_img_info`, `alloc_stream_buf`, `decoder_open`, or `decode_frame`
- `ret=...`: return code from the lower layer
- `rounds=x/y`: `x` successful rounds completed out of total `y`

The normal `jpegd` path and `jpeg_decode_stress` path do not currently print a unified `[RESULT][PASS]` line. In practice, they are typically considered successful when:

- No error log such as `malloc failed` or decoder init/open failure appears
- The decode flow continues normally without abnormal termination
- The stress task can be stopped and exits cleanly

#### 4.2.4 Integration Test Commands

`.it.csv` currently uses the unified `vcdec` result logs:

```text
ap_cmd jpeg_decode vcdec_jpegd
ap_cmd jpeg_decode vcdec_jpegd_flexa
```

The expected result strings are `[RESULT][PASS] vcdec_jpeg_test success` and `[RESULT][PASS] vcdec_jpeg_flexa_test success`.

## 5. Notes

1. `jpeg_decode` and `jpeg_decode_stress` are separate CLI commands.
2. `vcdec` commands and the boot-time self-test depend on `CONFIG_BK_DECODER`; if it is disabled, those features are not compiled in.
3. `jpeg_decode` has a single-task running guard, so only one decode task can run at a time.
4. `jpeg_decode_stress` allocates additional input, output, and DMA stress buffers, so sufficient memory is required.
5. The current `vcdec` demos cover both full-frame mode and FLEXA mode, which makes this project a useful reference for JPEG HAL/control-flow integration.
