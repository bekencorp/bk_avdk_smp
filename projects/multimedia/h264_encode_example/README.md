# H264 Encoding Example Project

* [中文](./README_CN.md)

## 1. Project Overview

This project demonstrates H264 encoding on the Beken platform. The current test path is based on the `bk_h264_encode_*` controller APIs and the VCENC H264 backend.

The project provides:

- Frame-mode VCENC H264 encode test: `h264_encode vcenc_h264e`
- Software FLEXA VCENC H264 encode test: `h264_encode vcenc_h264e_flexa`
- Boot-time VCENC H264 self-test when `CONFIG_BK_ENCODER` is enabled
- Integration-test entries in `.it.csv`

* For detailed information about H264 encoding, please refer to:

  - [H.264 Encode (VPU)](../../../developer-guide/vpu/h264_encode.html)
  - [Frame vs Flexa Mode](../../../developer-guide/vpu/flexa_frame.html)

* For API reference, please refer to:

  - [VPU API](../../../api-reference/multimedia/bk_vpu.html)

### 1.1 Test Environment

   * Hardware configuration:
      * Core board, **BK7259_QF128_12.3X12.3_V4.0**
      * PSRAM 32M
   * Supports H264 hardware encoding
      * Test input: built-in NV12 frame
      * Output: H264 encoded stream
      * Test setting: GOP 15, total 30 encoded frames per run

.. warning::
    Please use reference peripherals for familiarization and learning of the demo project. If peripheral specifications are different, the code may need to be reconfigured.

## 2. Directory Structure

The project adopts an AP-CP dual-core architecture, with the main source code located in the AP directory. The project structure is as follows:

```
h264_encode_example/
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
│   └── h264_encode/      # H264 encode implementation
│       ├── common/       # Built-in 256x128 NV12 test image
│       ├── include/      # Header files
│       └── src/          # CLI and VCENC H264 tests
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

- Supports VCENC H264 frame-mode encoding test
- Supports VCENC H264 software FLEXA encoding test
- Uses built-in `h264_encode_stream_256x128` NV12 input
- Encodes 30 frames with GOP set to 15
- Provides unified `[RESULT][PASS]` / `[RESULT][FAIL]` logs
- Runs CLI-triggered tests in worker threads
- Starts a boot-time encode demo when `CONFIG_BK_ENCODER` is enabled

### 3.2 Frame Buffer Management

The project uses frame buffer heaps for input and output buffers during H264 encoding:

- Input NV12 frame is copied to `MEM_SLAB_HEAP_UNCODED`
- Encoded output buffers are allocated from `MEM_SLAB_HEAP_CODED`
- Output buffers are released in the encode completion callback

### 3.3 H264 Encoding Process

1. Allocate and fill the 256x128 NV12 input frame
2. Create the H264 controller
   - Frame mode: `bk_h264_encode_frame_new()`
   - FLEXA mode: `bk_h264_encode_sw_flexa_new()`
3. Initialize and open the encoder
4. Encode 30 frames with GOP set to 15
5. Wait for completion callback for each frame
6. Print final `[RESULT]` log and release resources

## 4. Compilation and Execution

### 4.1 Compilation Method

Compile the project using the following command:

```bash
make bk7259 PROJECT=multimedia/h264_encode_example
```

### 4.2 Execution Method

After successful compilation, flash the generated firmware to the development board. If `CONFIG_BK_ENCODER` is enabled, `main()` starts a boot-time VCENC H264 demo task automatically. Manual tests can also be started from the serial terminal:

Command execution success prints: "CMDRSP:OK"
Command execution failure prints: "CMDRSP:ERROR"

#### 4.2.1 Current VCENC H264 Commands

```text
h264_encode help
h264_encode vcenc_h264e
h264_encode vcenc_h264e_flexa
```

- `vcenc_h264e` runs the frame-mode encoder test.
- `vcenc_h264e_flexa` runs the software FLEXA encoder test.
- Both commands encode the built-in 256x128 NV12 frame for 30 frames with GOP 15.
- `CMDRSP:OK` means the test task was created successfully; check `[RESULT]` logs for the final pass/fail status.

## 5. Test Examples

### 5.1 Frame-Mode VCENC H264 Test

```text
h264_encode vcenc_h264e
```

Expected final log:

```text
[RESULT][PASS] vcenc_h264_frame_test success, frames=30/30 encoded_size=... frame_type=...
```

### 5.2 Software FLEXA VCENC H264 Test

```text
h264_encode vcenc_h264e_flexa
```

Expected final log:

```text
[RESULT][PASS] vcenc_h264_flexa_test success, frames=30/30 encoded_size=... frame_type=...
```

### 5.3 Integration Test Commands

`.it.csv` contains:

```text
ap_cmd h264_encode vcenc_h264e
ap_cmd h264_encode vcenc_h264e_flexa
```

The expected result strings are the corresponding `[RESULT][PASS]` logs.

## 6. Configuration Options

### 6.1 Encoder Configuration

The current VCENC H264 test uses the following fixed test configuration:

- Width: `256`
- Height: `128`
- Input format: `BK_PIXEL_FORMAT_NV12`
- GOP: `15`
- Frames per test: `30`
- Frame-mode controller: `bk_h264_encode_frame_new()`
- FLEXA controller: `bk_h264_encode_sw_flexa_new()`

## 7. Notes

1. `h264_encode` only creates the test task. The final result is shown by `[RESULT][PASS]` or `[RESULT][FAIL]`.
2. The boot demo and manual CLI tests both use encoder hardware. Avoid starting a manual test before the boot demo finishes.
3. Frame buffer resources are limited; avoid occupying too many buffers simultaneously.
4. Encoding is asynchronous; results are returned in the output complete callback.
5. FLEXA mode feeds 16-line blocks. The test advances the write pointer from the FLEXA done callback until all 8 blocks of the 256x128 frame are provided.
6. **Callback Function Usage Notes**:
   - Blocking operations (such as long waits, sleep, etc.) are not recommended in callback functions to avoid impacting encoding performance and system responsiveness
   - It is recommended to perform only lightweight operations in callback functions, such as setting flags, sending messages/semaphores, etc., and move time-consuming operations to other tasks
7. **Buffer Management**:
   - Input buffers are allocated from `MEM_SLAB_HEAP_UNCODED`
   - Output buffers are allocated from `MEM_SLAB_HEAP_CODED`
   - Both input and output buffers should be released after use
