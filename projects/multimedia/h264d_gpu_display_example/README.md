# H264 Decode GPU Display Example

* [Chinese](./README_CN.md)

## 1. Project Overview

This project demonstrates the H264 decode, GPU image processing, and MIPI panel display flow on the Beken platform. The current test path is based on the `bk_h264_decode_*` decoder controller, the GPU blit/FLEXA processing pipeline, and the DPU/MIPI display backend.

This project provides:

- Decode a built-in H264 stream and output display frames through the GPU
- Optional MIPI panel display output
- Optional ISP picture-in-picture (PIP) overlay, composited onto the H264 main picture by GPU blit
- Power-on auto-run case when `CONFIG_BK_DECODER` is enabled
- `.it.csv` integration test entries

### 1.1 Test Environment

   * Hardware:
      * Core board: **BK7259_QF128_12.3X12.3_V4.0**
      * PSRAM 32M
      * MIPI DSI panel, default configuration: `LCD_HX8399C_MIPI_1080x1920`
      * CSI GC2053 sensor is used by default for PIP tests
   * Supported modules:
      * H264 hardware decoder
      * VG-Lite GPU
      * DPU/MIPI display
      * Optional ISP/CSI camera small-picture input
   * Default input:
      * Built-in `720x1280` H264 stream
      * Can be switched to the built-in `1280x720` H264 stream through Kconfig

.. warning::

    Use the reference peripherals to get familiar with this demo project. If the panel, sensor, or peripheral specifications are different, the code and Kconfig options may need to be adjusted.

## 2. Directory Structure

The project uses an AP-CP dual-core architecture. The main source code is under the AP directory. The project structure is as follows:

```
h264d_gpu_display_example/
├── .ci                         # CI configuration
├── .it.csv                     # Integration test commands
├── CMakeLists.txt              # Project-level CMake build file
├── Makefile                    # Make build file
├── README.md                   # Project README in English
├── README_CN.md                # Project README in Chinese
├── ap/                         # AP-side code
│   ├── CMakeLists.txt          # AP-side CMake build file
│   ├── Kconfig.projbuild       # Project configuration
│   ├── ap_main.c               # AP main entry
│   ├── config/                 # AP default configuration
│   ├── include/                # Header files
│   └── src/                    # H264 decode, GPU display, DPU, ISP PIP, and boot case
├── cp/                         # CP-side code
│   ├── CMakeLists.txt          # CP-side CMake build file
│   ├── cp_main.c               # CP main entry
│   └── config/                 # CP default configuration
├── main/                       # Dual-core entry build configuration
├── partitions/                 # Partition and memory region configuration
└── pj_config.mk                # Project configuration
```

## 3. Feature Description

### 3.1 Main Features

- Supports built-in H264 stream decode and display
- Supports `720x1280` and `1280x720` test stream configurations
- Supports GPU scaling, rotation, and format processing for decoded frames
- Supports sending GPU output frames to DPU/MIPI display
- Supports ISP small picture overlay onto the H264 main picture through GPU blit
- Supports specifying the loop count from CLI; `0` means loop continuously until `stop` is executed
- Provides unified `[RESULT][PASS]` / `[RESULT][FAIL]` result logs
- Automatically runs one default H264 decode GPU display case after power-on when `CONFIG_BK_DECODER` is enabled

### 3.2 H264 Decode Display Flow

1. Copy the built-in H264 stream to the coded frame buffer
2. Create and open the H264 decoder controller
3. Initialize the GPU output frame pool and GPU blit/FLEXA processing pipeline
4. Optionally initialize DPU/MIPI display output
5. Parse H264 NALUs and feed them to the decoder
6. Process decoded output frames through the GPU for the display size
7. Optionally composite the ISP small picture onto the GPU output frame
8. Optionally flush the frame to DPU/MIPI display
9. Release resources and print the `[RESULT]` log after the specified loop count is reached or a `stop` request is received

### 3.3 ISP PIP Overlay Flow

When `CONFIG_H264D_GPU_DISPLAY_ENABLE_ISP_PIP` is enabled, `isp_open` can be used to start the sensor, ISP channel, and PIP overlay task. The default configuration is:

- Sensor input: `1280x720@20fps`
- ISP PIP output: `640x360` NV12
- PIP rotation: 90 degrees by default
- PIP display position: `dst=(696,32)` by default, for a 1088x1920 portrait panel

`isp_close` closes the ISP channel, sensor, and bus, then releases PIP resources.

## 4. Build and Run

### 4.1 Build

Use the following command to build the project:

```bash
make bk7259 PROJECT=multimedia/h264d_gpu_display_example
```

### 4.2 Run

After the build is complete, flash the generated firmware to the development board. If `CONFIG_BK_DECODER` is enabled, `main()` automatically starts one H264 decode GPU display boot demo after power-on. You can also trigger the following commands manually from the serial terminal:

Command success prints: "CMDRSP:OK"

Command failure prints: "CMDRSP:ERROR"

#### 4.2.1 Current CLI Commands

```text
h264d_gpu_display help
h264d_gpu_display start [loops]
h264d_gpu_display stop
h264d_gpu_display isp_open
h264d_gpu_display isp_open <sensor_w> <sensor_h> <fps> <isp_w> <isp_h>
h264d_gpu_display isp_open <sensor_w> <sensor_h> <fps> <isp_w> <isp_h> <dst_x> <dst_y>
h264d_gpu_display isp_close
```

- `start [loops]`: starts the H264 decode GPU display task. If `loops` is omitted or is `0`, the task loops continuously. If `loops` is non-zero, the task exits automatically after the specified number of loops.
- `stop`: requests the current decode display task to exit after the current frame.
- `isp_open`: starts the ISP PIP overlay feature. When no parameters are passed, the default sensor/ISP/PIP configuration is used.
- `isp_close`: closes the ISP PIP overlay feature.
- `CMDRSP:OK` only means the command was accepted or the test thread was created successfully. Check the `[RESULT]` log for the final test result.

## 5. Test Examples

### 5.1 Decode and Display Once

```text
h264d_gpu_display start 1
```

Expected final log:

```text
[RESULT][PASS] decoded_frames=...
```

### 5.2 Continuous Decode Display and Stop

```text
h264d_gpu_display start
h264d_gpu_display stop
```

Expected final log:

```text
[RESULT][PASS] decoded_frames=...
```

### 5.3 Start ISP PIP Overlay

```text
h264d_gpu_display isp_open
h264d_gpu_display start
h264d_gpu_display isp_close
h264d_gpu_display stop
```

You can also specify the sensor size, ISP output size, and PIP position:

```text
h264d_gpu_display isp_open 1280 720 20 640 360 696 32
```

### 5.4 Integration Test Commands

`.it.csv` contains:

```text
ap_cmd h264d_gpu_display start 2
ap_cmd h264d_gpu_display start 20
ap_cmd h264d_gpu_display stop
ap_cmd h264d_gpu_display isp_open
ap_cmd h264d_gpu_display start
ap_cmd h264d_gpu_display isp_close
ap_cmd h264d_gpu_display stop
```

The expected results match `CMDRSP:OK`, `loop 1 start`, or `[RESULT][PASS]` logs.

## 6. Configuration Options

### 6.1 Display and Stream Configuration

- `CONFIG_H264D_GPU_DISPLAY_ENABLE_MIPI_DISPLAY`: enables MIPI panel display output
- `CONFIG_H264D_GPU_DISPLAY_TEST_STREAM_720X1280`: uses the built-in `720x1280` H264 stream
- `CONFIG_H264D_GPU_DISPLAY_TEST_STREAM_1280X720`: uses the built-in `1280x720` H264 stream

The default display output size is defined by the project configuration:

- GPU display width: `1088`
- GPU display height: `1920`
- The `720x1280` stream is not rotated by default
- The `1280x720` stream is rotated 90 degrees by default to fit the portrait panel

### 6.2 ISP PIP Configuration

- `CONFIG_H264D_GPU_DISPLAY_ENABLE_ISP_PIP`: enables ISP PIP overlay
- `CONFIG_H264D_GPU_DISPLAY_ENABLE_ISP_PIP_ROTATE`: enables ISP PIP rotation
- `CONFIG_H264D_GPU_DISPLAY_ISP_PIP_ROTATE_90`: rotates ISP PIP by 90 degrees
- `CONFIG_H264D_GPU_DISPLAY_ISP_PIP_ROTATE_270`: rotates ISP PIP by 270 degrees
- `CONFIG_H264D_GPU_DISPLAY_ISP_SENSOR_DEFAULT_WIDTH`: default sensor input width
- `CONFIG_H264D_GPU_DISPLAY_ISP_SENSOR_DEFAULT_HEIGHT`: default sensor input height
- `CONFIG_H264D_GPU_DISPLAY_ISP_SENSOR_DEFAULT_FPS`: default sensor frame rate
- `CONFIG_H264D_GPU_DISPLAY_ISP_PIP_DEFAULT_WIDTH`: default ISP PIP output width
- `CONFIG_H264D_GPU_DISPLAY_ISP_PIP_DEFAULT_HEIGHT`: default ISP PIP output height
- `CONFIG_H264D_GPU_DISPLAY_ISP_PIP_DEFAULT_DST_X`: default PIP X coordinate
- `CONFIG_H264D_GPU_DISPLAY_ISP_PIP_DEFAULT_DST_Y`: default PIP Y coordinate

## 7. Notes

1. `h264d_gpu_display start` only creates the test thread. The final result is reported by `[RESULT][PASS]` or `[RESULT][FAIL]`.
2. The power-on boot demo automatically runs one default stream. Wait for it to finish before manually triggering CLI tests.
3. `start` and `isp_open` use shared resources such as GPU, frame buffer, DPU, and ISP. Run them in the expected order and execute `stop` / `isp_close` when needed.
4. The default `isp_open` parameters are intended for GC2053 and a 1088x1920 portrait panel. When changing the sensor or panel, update Kconfig, GPIO, and display coordinates accordingly.
5. The PIP output format is NV12, so width and height must be even. It is recommended to keep the width aligned to 16 pixels for GPU/DMA access.
6. Do not perform blocking operations in callbacks or interrupt context. Time-consuming work should be handled in tasks.
7. Frame buffer resources are limited. If a test exits unexpectedly, use `memshow` / `memleak` to check whether resources were released completely.
