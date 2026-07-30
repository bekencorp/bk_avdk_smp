# H264 Encoding Example Project

* [中文](./README_CN.md)

## 1. Project Overview

This project demonstrates H264 encoding on the Beken platform. The current test path is based on the `bk_h264_encode_*` controller APIs and the VCENC H264 backend.

The project provides:

- Frame-mode and software FLEXA VCENC H264 encode tests (pure encode, no OSD)
- Interactive OSD test command: `h264_encode osd` (8 channels + ARGB8888 / NV12 / Bitmap, single frame)
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
h264_encode osd
```

- `vcenc_h264e` runs the frame-mode encoder test.
- `vcenc_h264e_flexa` runs the software FLEXA encoder test.
- `osd` runs the OSD test (8 channels + ARGB8888, NV12, Bitmap; one encoded frame).
- Both encode commands use the built-in 256x128 NV12 frame for 30 frames with GOP 15.
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

### 5.3 OSD Test (8 Channels + 3 Formats)

```text
h264_encode osd
```

Expected final log:

```text
[RESULT][PASS] h264_encode_osd_test success, slots=8/8 formats=3/3 frames=1/1 encoded_size=... frame_type=...
```

All 8 OSD slots are enabled in one frame: slot1 NV12, slot2 Bitmap, the rest ARGB8888. Each region occupies one CTB cell (4 columns × 2 rows).

Tests slot0 ARGB8888, slot1 NV12, and slot2 Bitmap on separate CTB columns.

### 5.4 Integration Test Commands

`.it.csv` contains:

```text
ap_cmd h264_encode vcenc_h264e
ap_cmd h264_encode vcenc_h264e_flexa
ap_cmd h264_encode osd
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

### 6.2 H264 OSD Usage Guidelines

This project overlays OSD via `bk_h264_encode_set_osd()`. Supported formats: ARGB8888 (0), NV12 (1), Bitmap (2).

| Item | Requirement |
|------|-------------|
| Slot count | Hardware supports up to **8** overlays, index **0..7** |
| Pixel format | **0=ARGB8888** (per-pixel alpha; `alpha` ignored); **1=NV12** (global `alpha`); **2=Bitmap** (1bpp; color via `bitmap_y/u/v`) |
| Position alignment | **x/y must be 2-pixel aligned** |
| Size alignment | ARGB8888/NV12: **2-pixel aligned**; Bitmap: **8-pixel aligned** |
| Stride | ARGB8888: `width×4`; NV12: Y/UV stride = `width`; Bitmap: `width/8` |
| CTB overlap | H.264 CTB is **64×16**; multiple OSD regions must not share the same CTB |
| Cache | **Flush D-Cache** after CPU writes, before `bk_h264_encode_set_osd()` |
| Buffer lifetime | Release OSD buffers in the `buffer_free` callback |
| Disable a slot | Submit the slot index with `buffer = NULL` |

**`h264_encode osd` test layout (CTB-safe, 4 columns × 2 rows)**

| Slot | Format | Label | x | y | Size |
|------|--------|-------|---|---|------|
| 0 | ARGB8888 | 00:00:00 | 4 | 0 | 48×16 |
| 1 | NV12 | 01 | 68 | 0 | 32×16 |
| 2 | Bitmap | 02 | 132 | 0 | 32×16 |
| 3 | ARGB8888 | 03 | 196 | 0 | 32×16 |
| 4 | ARGB8888 | 04 | 4 | 16 | 32×16 |
| 5 | ARGB8888 | 05 | 68 | 16 | 32×16 |
| 6 | ARGB8888 | 06 | 132 | 16 | 32×16 |
| 7 | ARGB8888 | 07 | 196 | 16 | 32×16 |

**Frame / Flexa encode tests**: no OSD overlay; they validate the pure H.264 encode path only.

**API call order**

1. Create and `open` the encoder
2. Allocate buffer for the chosen format, draw content, flush cache
3. Fill `bk_h264_encode_osd_t` (index, format, position, size, alpha/bitmap color, `buffer_free`)
4. Call `bk_h264_encode_set_osd()`
5. Call `bk_h264_encode_start()`

**Common mistakes**

- Overlapping OSD regions in the same CTB → pipeline hang or encode timeout
- Misaligned or out-of-bounds geometry → `bk_h264_encode_set_osd()` failure
- Missing cache flush → corrupted OSD content

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
