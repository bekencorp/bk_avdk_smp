# UVC Sample Project

* [中文](./README_CN.md)

## 1. Project Overview

This project is a UVC sample project that implements the functionality of a USB Video Class (UVC) device. The module provides CLI test commands that can be used to open and close the UVC device, as well as retrieve real-time UVC output images.

* For detailed information about UVC usage, please refer to:

  - [UVC Usage](../../../developer-guide/camera/uvc.html)

* UVC API and Data Structures

  - [UVC API](../../../api-reference/multimedia/bk_camera.html)

### 1.1 Test Environment

   * Hardware Configuration:
      * Core Board, **BK7258_QFN88_9X9_V3.2**
      * PSRAM 8M/16M
   * Supported USB2.0/UVC1.5, Output Formats: MJPEG/H264/H265

   * Supported UVC Devices, refer to: [Supported UVC Peripherals](../../../support_peripherals/index.html#uvc-camera)

.. note::

    Please use the reference peripherals for familiarizing yourself with and learning the demo project. If the peripheral specifications differ, the code may need to be reconfigured.

## 2. Project Directory Structure

The project adopts an AP-CP dual-core architecture, with main source code located in the AP directory. The project structure is as follows:

```
uvc_example
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

## 3. Project Functionality

### 3.1 Main Functionality

- Supports UVC power up and down operations
- Supports UVC port enumeration and query
- Supports UVC device opening and closing
- Supports UVC video format configuration and query
- Supports UVC video data acquisition and parsing

### 3.2 Main Code

- `uvc_test_info_t`: Test information structure, containing decode buffer and camera handle array
- `uvc_common.c`: UVC power up and down basic operations, port information parsing and check
- `uvc_main.c`: CLI command processing function, supporting API testing and full functionality testing

### 3.3 CLI Command Usage
```bash
uvc open [port] [ppi] [type]  # Open UVC device
uvc close [port]              # Close UVC device
```

Parameter Description:
- `port`: Port number (1-4)
- `ppi`: Resolution (e.g., 640X480/1280X720)
- `type`: Video format (jpeg/h264/h265/yuv/dual)

### 3.4 User Reference Files
- `projects/uvc_example/ap/uvc_test/src/uvc_main.c` : UVC test main function
- `projects/uvc_example/ap/uvc_test/src/uvc_common.c` : UVC power up and down basic operations, port information parsing and check

## 4. Compilation and Operation

### 4.1 Compilation Method

Use the following command to compile the project:

```
make bk7258 PROJECT=uvc_example
```

### 4.2 Operation Method

After compilation, burn the generated firmware to the development board, and then use the following commands to test UVC functionality through the serial port terminal:

Command execution success prints: "CMDRSP:OK"

Command execution failure prints: "CMDRSP:ERROR"

### 4.3 Configuration Parameters

- `BK_UVC_864X480_30FPS_MJPEG_CONFIG`: Default resolution configuration

### 4.4 Test Examples

Test commands need to be prefixed with "ap_cmd", such as `ap_cmd uvc open 1`.

#### 4.4.1 UVC Open 864X480 MJPEG Test

- CASE command:

```bash
uvc open 1 864X480
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
```
#### 4.4.2 UVC Close Test

- CASE command:

```bash
uvc close 1
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
uvc_id1:0[0 0KB], uvc_id2:0[0 0KB], uvc_id3:0[0 0KB], uvc_id4:0[0 0KB], packets[all:0, err:0]
```

#### 4.4.3 UVC Open Port 2, 1280X720 MJPEG Test

.. note::
        When testing other ports, the USB should be connected to a HUB, and ensure that port 2 is connected to a UVC device.

- CASE command:

```bash
uvc open 2 1280X720
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
Regardless of the format, uvc will print the following log: uvc_idx represents the port number, default maximum support 3, followed by the frame rate, generally greater than or equal to 10;
uvc_id1:30[463 23KB], uvc_id2:0[0 0KB], uvc_id3:0[0 0KB], uvc_id4:0[0 0KB], packets[all:127896, err:8]
If it is jpeg format, it will print mjpeg format frame information, seq will gradually increase, when it reaches the maximum value and becomes 0, length will dynamically change, generally greater than 10KB at least
uvc_frame_complete: seq:0, length:8303, format:mjpeg, h264_type:0 打印mjpeg格式的帧信息
```
- CASE success standard:

```
CMDRSP:OK
```
- CASE success log:

```
uvc_id1:30[463 23KB], uvc_id2:0[0 0KB], uvc_id3:0[0 0KB], uvc_id4:0[0 0KB], packets[all:127896, err:8]
```

#### 4.4.4 UVC Open Unsupported Resolution Test
- CASE command:

```bash
uvc open 1 1024X600
```

- CASE expected result:

```
Failure
```
- CASE failure standard:

```
CMDRSP:ERROR
```
- CASE failure log:

```
uvc_camera_stream_check_config, not support this resolution:1024X600
uvc_camera_stream_rx_config, not support this solution, please retry...
```

#### 4.4.5 UVC Open Unsupported Image Format Test
- CASE command:

```bash
uvc open 1 864X480 h264
```

- CASE expected result:

```
Failure
```
- CASE failure standard:

```
CMDRSP:ERROR
```
- CASE failure log:

```
uvc_camera_stream_check_config, please check usb output format:h264
```

### 4.5 Notes

1. Port number must be within the range of 1-4
2. Resolution must match hardware support
3. Format type must match device capabilities

### 4.6 Normal Print Logs

- CMDRSP:OK prints command execution success
- CMDRSP:ERROR prints command execution failure
- uvc_id1:30[463 23KB], uvc_id2:0[0 0KB], uvc_id3:0[0 0KB], uvc_id4:0[0 0KB], packets[all:127896, err:8] prints frame rate and image size of each port
- uvc_frame_complete: seq:0, length:8303, format:mjpeg, h264_type:0 prints mjpeg format frame information