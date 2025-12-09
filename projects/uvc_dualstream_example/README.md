# UVC Dual Stream Sample Project

* [中文](./README_CN.md)

## 1. Project Overview

This project is a UVC dual stream sample project for implementing dual-stream output functionality of USB Video Class (UVC) devices. The module provides CLI test commands that allow users to open and close UVC devices, configure main stream and sub stream parameters, and retrieve real-time UVC output image data.

* For detailed information about UVC usage, please refer to:

  - [UVC Usage](../../../developer-guide/camera/uvc.html)

* For detailed information about UVC API and data structures, please refer to:

  - [UVC API](../../../api-reference/multimedia/bk_camera.html)

### 1.1 Test Environment

   * Hardware Configuration:
      * Core Board, **BK7258_QFN88_9X9_V3.2**
      * PSRAM 8M/16M
   * Supports USB2.0/UVC1.5, Output Formats: MJPEG/H264/H265

   * Supported UVC Devices, refer to: [Supported UVC Peripherals](../../../support_peripherals/index.html#uvc-camera)

.. note::

    Please use the reference peripherals for familiarizing yourself with and learning the demo project. If the peripheral specifications differ, the code may need to be reconfigured.

## 2. Project Directory Structure

The project adopts an AP-CP dual-core architecture, with main source code located in the AP directory. The project structure is as follows:

```
uvc_dualstream_example
├── .ci                       # CI configuration directory
├── .gitignore                # Git ignore file configuration
├── .it.csv                   # Test case configuration file
├── CMakeLists.txt            # CMake build script
├── Makefile                  # Make build script
├── README.md                 # English README
├── README_CN.md              # Chinese README
├── ap/                       # AP core code directory
│   ├── CMakeLists.txt        # AP CMake build script
│   ├── ap_main.c             # AP main entry file
│   ├── config/               # AP configuration directory
│   │   └── bk7258_ap/        # BK7258 AP configuration
│   └── uvc_test/             # UVC test code directory
│       ├── include/          # Header files
│       │   ├── uvc_cli.h     # UVC CLI command header file
│       │   └── uvc_common.h  # Common UVC definitions and functions header
│       └── src/              # Source files
│           ├── uvc_common.c  # Common UVC implementation
│           └── uvc_main.c    # Main UVC test implementation
├── app.rst                   # Application description file
├── cp/                       # CP core code directory
│   ├── CMakeLists.txt        # CP CMake build script
│   ├── config/               # CP configuration directory
│   │   └── bk7258/           # BK7258 CP configuration
│   └── cp_main.c             # CP main entry file
├── it.yaml                   # Test configuration file
├── partitions/               # Partition configuration directory
│   └── bk7258/               # BK7258 partition configuration
│       ├── auto_partitions.csv  # Auto partition configuration
│       └── ram_regions.csv      # RAM regions configuration
└── pj_config.mk              # Project configuration file
```

## 3. Functionality Description

### 3.1 Main Functions

- Supports UVC power up operations
- Supports UVC port enumeration and query
- Supports UVC device opening and closing
- Supports UVC video format configuration and query
- Supports UVC video data acquisition and parsing
- Supports simultaneous output of main stream and sub stream
- Supports dual stream configuration with different resolutions and formats

### 3.2 Dual Stream Functionality Details

This project implements UVC dual stream technology, allowing simultaneous transmission of two different formats or resolutions of video streams over the same USB connection, meeting the needs of different scenarios:

#### 3.2.1 Stream Type Definitions
- **Main Stream**: Uses MJPEG format, providing high-resolution, high-quality video, suitable for scenarios requiring detailed image information
- **Sub Stream**: Uses H264 or H265 format, providing lower resolution video, suitable for network transmission scenarios

#### 3.2.2 Supported Stream Combination Methods
- Open/close MJPEG main stream separately
- Open/close H264 sub stream separately
- Open/close H265 sub stream separately
- Open MJPEG main stream and H264 sub stream simultaneously, can close both or close MJPEG main stream or H264 sub stream separately
- Open MJPEG main stream and H265 sub stream simultaneously, can close both or close MJPEG main stream or H265 sub stream separately

#### 3.2.3 Dual Stream Implementation Mechanism
- Uses independent stream information structures to manage main stream and sub stream separately
- Handles different types of streams through unified stream operation interfaces (uvc_open_stream_helper/uvc_close_stream_helper)
- Supports closing specific types of streams separately, improving flexibility

### 3.3 Main Code

- `uvc_test_info_t`: Test information structure, containing decode buffer and camera handle array
- `uvc_common.c`: UVC power up and down basic operations, port information parsing and check
- `uvc_main.c`: CLI command processing function, supporting API testing and full functionality testing
- `g_uvc_main_stream_info`: Main stream information structure
- `g_uvc_sub_stream_info`: Sub stream information structure

### 3.4 CLI Command Usage
```bash
uvc open_single port img_format width height   # Open single stream UVC device
uvc open_dual port [options]                  # Open dual stream UVC device
uvc close port [stream_type]                  # Close UVC device
```

#### 3.4.1 open_single Command Parameter Description

- `port`: UVC port number, default is 1, range [1-4]
- `width`: Image width, default is 864
- `height`: Image height, default is 480
- `img_format`: Image format, default is MJPEG, supports MJPEG, H264, H265

#### 3.4.2 open_dual Command Parameter Description

- `port`: UVC port number, default is 1, range [1-4]
- `options`: Optional parameters for stream configuration

  Supported parameter combination examples:
  ```
  uvc open_dual 1 MJPEG 864 480                     # Open only MJPEG main stream
  uvc open_dual 1 H264 1280 720                     # Open only H264 sub stream
  uvc open_dual 1 H265 1280 720                     # Open only H265 sub stream
  uvc open_dual 1 MJPEG 864 480 H264 1280 720       # Open MJPEG main stream and H264 sub stream simultaneously
  uvc open_dual 1 MJPEG 864 480 H265 1280 720       # Open MJPEG main stream and H265 sub stream simultaneously
  uvc close 1 all                                   # Close all streams on UVC port 1
  uvc close 1 H26X                                  # Close H264/H265 sub stream on UVC port 1
  uvc close 1 MJPEG                                 # Close MJPEG main stream on UVC port 1
  ```

#### 3.4.3 close Command Parameter Description

- `port`: UVC port number
- `stream_type`: Optional, specifies closing a specific stream, can be 'H26X' or 'MJPEG'

### 3.5 User Reference Files
- `projects/uvc_dualstream_example/ap/uvc_test/src/uvc_main.c` : UVC test main function, including dual stream processing logic
- `projects/uvc_dualstream_example/ap/uvc_test/src/uvc_common.c` : UVC power management and port enumeration information check functionality

## 4. Compilation and Operation

### 4.1 Compilation Method

Use the following command to compile the project:

```
make bk7258 PROJECT=uvc_dualstream_example
```

### 4.2 Operation Method

After compilation, burn the generated firmware to the development board, then use the following commands to test UVC functionality through the serial port terminal:

Command execution success prints: "CMDRSP:OK"

Command execution failure prints: "CMDRSP:ERROR"

### 4.3 Configuration Parameters

- `BK_UVC_864X480_30FPS_MJPEG_CONFIG`: Default MJPEG resolution configuration

- `BK_UVC_1920X1080_30FPS_H26X_CONFIG`: Default H264/H265 resolution configuration

### 4.4 Test Examples

Test commands need to be prefixed with "ap_cmd", such as `ap_cmd uvc open_single 1 MJPEG 864 480`.

#### 4.4.1 UVC Open 864X480 MJPEG Single Stream Test
- CASE command:

```bash
uvc open_single 1 MJPEG 864 480
```

- CASE expected result:

```
Success
```
- CASE success standard:

```
CMDRSP:OK
```
- CASE success log:

```
uvc_id1:30[463 23KB], uvc_id2:0[0 0KB], uvc_id3:0[0 0KB], uvc_id4:0[0 0KB], packets[all:127896, err:8]
If it is jpeg format, it will print mjpeg format frame information, seq will gradually increase, when it reaches the maximum value and becomes 0, length will dynamically change, generally greater than 10KB at least
uvc_frame_complete: seq:0, length:8303, format:mjpeg, h264_type:0 prints mjpeg format frame information
```
- CASE failure standard:

```
CMDRSP:ERROR
```
- CASE failure log:

```
No frame information printed
```
#### 4.4.2 UVC Open 864X480 MJPEG 1280X720 H264 Dual Stream Test
- CASE command:

```bash
uvc open_dual 1 MJPEG 864 480 H264 1280 720
```

- CASE expected result:

```
Success
```
- CASE success standard:

```
CMDRSP:OK
```
- CASE success log:

```
uvc_id1:19[77 23KB], uvc_id2:19[77 4KB], uvc_id3:0[0 0KB], uvc_id4:0[0 0KB], packets[all:4432, err:0]

uvc_frame_complete: port:1, seq:150, length:23676, format:mjpeg, h264_type:0, result:0
uvc_frame_complete: port:1, seq:150, length:36084, format:h264, h264_type:0, result:0
```
- CASE failure standard:

```
CMDRSP:ERROR
```
- CASE failure log:

```
No frame information printed
```

#### 4.4.3 UVC Close Test
- CASE command:

```bash
uvc close 1 all
```

- CASE expected result:

```
Success
```
- CASE success standard:

```
CMDRSP:OK
```

- CASE failure standard:

```
CMDRSP:ERROR
```

### 4.5 Notes

1. Port number must be within the range of 1-4
2. Resolution must match hardware support
3. Format type must match device capabilities
4. **Dual stream functionality is built on the basis of normal single stream functionality. If there is an exception when testing dual streams, you can first test whether the single stream functionality is normal**

### 4.6 Normal Print Logs

- CMDRSP:OK prints command execution success
- CMDRSP:ERROR prints command execution failure
- uvc_id1:19[77 23KB], uvc_id2:19[77 4KB], uvc_id3:0[0 0KB], uvc_id4:0[0 0KB], packets[all:4432, err:0], where uvc_id1 represents the frame rate of the first opened stream, and uvc_id2 represents the frame rate of the second opened stream
- uvc_frame_complete: port:1, seq:150, length:23676, format:mjpeg, h264_type:0, result:0 prints mjpeg format frame information
- uvc_frame_complete: port:1, seq:150, length:36084, format:h264, h264_type:0, result:0 prints h264/h265 format frame information
- In dual stream mode, the main stream (MJPEG) and sub stream (H264/H265) will display their respective frame rates and image information separately, and output frame information in different formats