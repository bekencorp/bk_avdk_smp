## Encoder Sample Project

- [中文](./README_CN.md)

## 1. Project Overview

This project demonstrates how to encode an **internal** YUV422 (YUYV) test buffer into **MJPEG** on BK7258.
It provides CLI test commands to run the encoder controller flow via UART (e.g., **open/encode/get_compress/set_compress/close**).

### 1.1 Test Environment

- **Hardware**:
  - Core board: BK7258_QFN88_9X9_V3.2
  - PSRAM 8M/16M
- **Input image**:
  - Internal YUV422 (YUYV) test buffer: `projects/encoder_example/ap/yuv_test_buf.c`
  - Default resolution: 640x480 (see `projects/encoder_example/ap/encoder_cli.h`)

## 2. Directory Structure

```
encoder_example/
├── .it.csv
├── CMakeLists.txt
├── Makefile
├── README.md
├── README_CN.md
├── ap/
│   ├── CMakeLists.txt
│   ├── ap_main.c         # Register CLI command: jpeg_encode
│   ├── encoder_cli.h
│   ├── yuv_test_buf.c    # get_yuv_test_buf()
│   ├── jpeg_encoder_test.c
├── cp/
├── partitions/
└── pj_config.mk
```

## 3. CLI Usage

The following commands are registered in `ap/ap_main.c`:

- **jpeg_encode**: `open | encode | set_compress | get_compress | close`

Notes:

- **open**: create and open the encoder controller (default resolution/fps/yuv_format)
- **encode**: encode the internal YUV test buffer; output is printed via `stack_mem_dump()`
- **set_compress**: update compression parameters during encoding
- **get_compress**: get current compression parameters (min/max expected output size)
- **close**: close and destroy the controller handle

All test commands must be prefixed with `ap_cmd`, for example:

```bash
ap_cmd jpeg_encode open
ap_cmd jpeg_encode get_compress
ap_cmd jpeg_encode encode
ap_cmd jpeg_encode set_compress 10240 40960
ap_cmd jpeg_encode get_compress
ap_cmd jpeg_encode close
```

On success: `CMDRSP:OK`
On failure: `CMDRSP:ERROR`

## 4. Build and Run

### 4.1 Build

```bash
make bk7258 PROJECT=encoder_example
```

### 4.2 Run

Flash the firmware and run the CLI commands above via UART to test the encoding flow.

## 5. Notes

1. The input image format is YUV422. The default format is YUYV (big-endian byte order).
2. The encode command may take longer to run; for automated test cases, the suggested timeout is 20 seconds.

