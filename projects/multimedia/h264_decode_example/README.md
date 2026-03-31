# H264 Decoding Example Project

* [中文](./README_CN.md)

## 1. Project Overview

This project is an H264 decoding test module designed to test H264 decoding functionality on the Beken platform. It provides a Command Line Interface (CLI) that supports H264 decoding.

* For detailed information about H264 decoding, please refer to:

  - [H264 Decoding (SW) Overview](../../../developer-guide/video_codec/h264_decoding_sw.html)

### 1.1 Test Environment

   * Hardware configuration:
      * Core board, **BK7258_QFN88_9X9_V3.2**
      * PSRAM 8M/16M
   * Supports H264 decoding
      * Input: H264 stream (built-in demo stream in `bk_test_h264d.c`)
      * Output: Decoded frame callback and basic information logs

.. warning::
    Please use reference peripherals for familiarization and learning of the demo project. If peripheral specifications are different, the code may need to be reconfigured.

## 2. Directory Structure

The project adopts an AP-CP dual-core architecture, with the main source code located in the AP directory. The project structure is as follows:

```
h264_decode_example/
├── .ci                   # CI configuration directory
├── .gitignore            # Git ignore file
├── CMakeLists.txt        # Project-level CMake build file
├── Makefile              # Make build file
├── README.md             # Project documentation (English)
├── README_CN.md          # Project documentation (Chinese)
├── ap/                   # AP-side code
│   ├── CMakeLists.txt    # AP-side CMake build file
│   ├── Kconfig.projbuild # Kconfig configuration
│   ├── ap_main.c         # AP main entry file
│   ├── config/           # AP configuration directory
│   └── h264_decode/      # H264 decode implementation
│       ├── include/      # Header files
│       └── src/          # Source code files
├── cp/                   # CP-side code
│   ├── CMakeLists.txt    # CP-side CMake build file
│   ├── cp_main.c         # CP main entry file
│   └── config/           # CP configuration directory
├── it.yaml               # Integration test configuration
├── partitions/           # Partition configuration
└── pj_config.mk          # Project configuration
```

## 3. Feature Description

### 3.1 Main Features

- Supports H264 decoding
- Provides CLI for decoding tests

## 4. Compilation and Execution

### 4.1 Compilation Method

Compile the project using the following command:

```
make bk7259 PROJECT=multimedia/h264_decode_example
```

### 4.2 Execution Method

After successful compilation, flash the generated firmware to the development board and use the following commands through the serial terminal to test the H264 decoding functionality:

Command execution success prints: "CMDRSP:OK"
Command execution failure prints: "CMDRSP:ERROR"

#### 4.2.1 Basic Decoding Commands

```
h264_decode h264d
h264_decode jpegd
```

## 7. Notes

1. Ensure `CONFIG_BK_H264D` is enabled before running.
2. Ensure `CONFIG_BK_H264D_DEMO` is enabled before running.
3. `h264_decode h264d` calls `h264_decoder_test()` implemented in `bk_test_h264d.c`.
4. The CLI command only creates a task; decode runs asynchronously in that task.
