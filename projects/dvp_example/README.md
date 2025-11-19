# DVP Sample Project

* [中文](./README_CN.md)

## 1. Project Overview

This Project is a DVP Sample Project, which is used to implement DVP device function. This module provides CLI test commands, through which commands can be sent to open and close DVP, and real-time DVP output images can be obtained. The format of DVP output is YUV422, but this project not only supports YUV422 output, but also supports output of encoded data MJPEG or H.264, but cannot output both encoding data at the same time. In addition, it demonstrates how to adapt a new DVP sensor; refer to the integration approach shown in `dvp_gc0001_test.c`.

* For detailed instructions on how to use DVP, please refer to the following link:

  - [DVP use guide](../../../developer-guide/camera/dvp.html)

* For detailed descriptions of DVP API and data structures, please refer to the following link:

  - [DVP API](../../../api-reference/multimedia/bk_camera.html)

 * For guidance on adding support for a new DVP sensor, refer to this sample project or the following link:

 - [DVP Add New](../../../developer-guide/camera/dvp.html)

### 1.1 Test Environment

   * Hardware Configuration:
      * Core Board, **BK7258_QFN88_9X9_V3.2**
      * PSRAM 8M/16M
   * Supported DVP Device:
      * DVP GC2145, Maximum Resolution: 1280x720, Output Format: YUV422

   * For more supported DVP devices, please refer to: [Supported DVP Peripherals](../../../support_peripherals/index.html#dvp-camera)

.. note::

    Please use the reference peripherals to familiarize yourself with and learn the demo project. If the peripheral specifications are different, the code may need to be reconfigured.

## 2. Directory Structure

The project adopts the AP-CP dual-core architecture, with the main source code located in the AP directory. The project structure is as follows:

```
dvp_example/
├── .ci/                  # CI configuration files directory
├── .gitignore            # Git ignore file configuration
├── .it.csv               # Test case configuration file
├── CMakeLists.txt        # Project-level CMake build file
├── Makefile              # Make build file
├── README.md             # English project description document
├── README_CN.md          # Chinese project description document
├── ap/                   # AP-side code directory
│   ├── CMakeLists.txt    # AP-side CMake build file
│   ├── ap_main.c         # AP-side main entry file
│   ├── config/           # AP-side configuration files directory
│   │   └── bk7258_ap/    # BK7258 AP-side configuration
│   │       ├── config     # System configuration file
│   │       └── usr_gpio_cfg.h  # User GPIO configuration header file
│   └── dvp_test/         # DVP test code directory
│       ├── include/      # Test header files directory
│       │   ├── dvp_cli.h       # DVP command line interface header file
│       │   └── dvp_frame_list.h # DVP frame list management header file
│       └── src/          # Test source code directory
│           ├── dvp_api_test.c  # DVP API test code
│           ├── dvp_frame_list.c # DVP frame list management implementation
│           ├── dvp_func_test.c # DVP function test code
│           ├── dvp_gc0001_test.c # GC0001 sensor test code
│           └── dvp_main.c      # DVP test main program
├── app.rst               # Application description document
├── cp/                   # CP-side code directory
│   ├── CMakeLists.txt    # CP-side CMake build file
│   ├── cp_main.c         # CP-side main file
│   └── config/           # CP-side configuration files directory
│       └── bk7258/       # BK7258 CP-side configuration
│           ├── config     # System configuration file
│           └── usr_gpio_cfg.h  # User GPIO configuration header file
├── it.yaml               # Integration test configuration file
├── partitions/           # Partition configuration directory
│   └── bk7258/           # BK7258 partition configuration
│       ├── auto_partitions.csv  # Auto partition configuration
│       └── ram_regions.csv      # RAM region configuration
└── pj_config.mk          # Project configuration Makefile
```

## 3. Function Description

### 3.1 Main Features

- Support opening and closing of DVP devices
- Support outputting YUV422 format data only
- Support outputting MJPEG format data only
- Support outputting H264 format data only
- Support outputting YUV422 and MJPEG format data
- Support outputting YUV422 and H264 format data

### 3.2 Main Code

- `dvp_frame_list.c`: DVP frame list management implementation, used to manage DVP output frame data
- `dvp_api_test.c`: DVP controller interface test
- `dvp_func_test.c`: DVP function test, only distinguishing between open and close operations

### 3.3 CLI Command Usage
```bash
dvp open [width] [height] [type]  # Open DVP device
dvp close             # Close DVP device
```

Parameter Description:

- `width`: Video width
- `height`: Video height
- `type`: Video format (jpeg/h264/h265/yuv/enc_yuv)

    `jpeg`: Output MJPEG format data

    `h264`: Output H264 format data

    `yuv`: Output YUV422 format data

    `enc_yuv`: Output encod data and YUV422 format data meantime

### 3.4 User Reference Files

- `projects/dvp_example/ap/dvp_test/src/dvp_main.c` : DVP test main function
- `projects/dvp_example/ap/dvp_test/src/dvp_frame_list.c` : DVP frame list management implementation, used to manage DVP output frame data
- `projects/dvp_example/ap/dvp_test/src/dvp_api_test.c` : DVP API test code
- `projects/dvp_example/ap/dvp_test/src/dvp_func_test.c` : DVP function test code

## 4. Compilation and Operation

### 4.1 Compilation Method

Using the following command to compile the project:

```
make bk7258 PROJECT=dvp_example
```

### 4.2 Operation Method

After compilation, burn the generated firmware to the development board, and then use the following commands in the serial terminal to test the DVP function:

Command execution success print: "CMDRSP:OK"

Command execution failure print: "CMDRSP:ERROR"

### 4.3 Configuration Parameters

- `BK_DVP_864X480_30FPS_MJPEG_CONFIG`: Default resolution configuration

### 4.4 Test Examples

Test commands need to be prefixed with "ap_cmd", such as `ap_cmd dvp open 864 480 jpeg`

#### 4.4.1  - DVP Open Output JPEG Image Test
- CASE Command:

```bash
dvp open 864 480
```

- CASE Expected Result:

```
Success
```
- CASE Success Standard:

```
CMDRSP:OK
```
- CASE Success Log:

```
dvp:27[322 8KB 2010Kbps] print dvp output info dvp: frame rate[frame number frame size bit rate], print this log every 4-6 seconds
```
- CASE Success Standard:

```
CMDRSP:OK
```
- CASE Success Log:

```
dvp:27[322 8KB 2010Kbps] print dvp output info dvp: frame rate[frame number frame size bit rate], print this log every 4-6 seconds
```
- CASE Failure Standard:

```
CMDRSP:ERROR
```
- CASE Failure Log:

```
Above log does not print, or seq suddenly does not increase, or the frame rate suddenly becomes 0, it indicates an exception
```

#### 4.4.2  - DVP Close Test
- CASE Command:

```bash
dvp close
```

- CASE Expected Result:

```
Success
```
- CASE Success Standard:

```
CMDRSP:OK
```

- CASE Failure Standard:

```
CMDRSP:ERROR
```

#### 4.4.3  - DVP Open Output H264 Image Test
- CASE Command:

```bash
dvp open 864 480 h264
```

- CASE Expected Result:

```
Success
```
- CASE Success Standard:

```
CMDRSP:OK
```
- CASE Success Log:

```
dvp:27[322 8KB 2010Kbps] print dvp output info dvp: frame rate[frame number frame size bit rate], print this log every 4-6 seconds
```
- CASE Failure Standard:

```
CMDRSP:ERROR
```

#### 4.4.4  - DVP Open Output YUV Image Test
- CASE Command:

```bash
dvp open 864 480 yuv
```

- CASE Expected Result:

```
Success
```
- CASE Success Standard:

```
CMDRSP:OK
```
- CASE Success Log:

```
dvp:27[322 8KB 2010Kbps] print dvp output info dvp: frame rate[frame number frame size bit rate], print this log every 4-6 seconds
```
- CASE Failure Standard:

```
CMDRSP:ERROR
```
- CASE Failure Log:

```
Above log does not print, or seq suddenly does not increase, or the frame rate suddenly becomes 0, it indicates an exception
```

#### 4.4.5  - DVP Open Output H264&YUV Image Test
- CASE Command:

```bash
dvp open 864X480 h264 enc_yuv
```

- CASE Expected Result:

```
Success
```
- CASE Success Standard:

```
CMDRSP:OK
```
- CASE Success Log:

```
dvp:27[322 8KB 2010Kbps] print dvp output info dvp: frame rate[frame number frame size bit rate], print this log every 4-6 seconds
```
- CASE Failure Standard:

```
CMDRSP:ERROR
```
- CASE Failure Log:

```
Above log does not print, or seq suddenly does not increase, or the frame rate suddenly becomes 0, it indicates an exception
```

#### 4.4.6  - DVP Open Output JPEG and YUV Image Test
- CASE Command:

```bash
dvp open 864 480 jpeg enc_yuv
```

- CASE Expected Result:

```
Success
```
- CASE Success Standard:

```
CMDRSP:OK
```
- CASE Success Log:

```
dvp:27[322 8KB 2010Kbps] print dvp output info dvp: frame rate[frame number frame size bit rate], print this log every 4-6 seconds
```
- CASE Failure Standard:

```
CMDRSP:ERROR
```
- CASE Failure Log:

```
Above log does not print, or seq suddenly does not increase, or the frame rate suddenly becomes 0, it indicates an exception
```

#### 4.4.7  - DVP Open Output JPEG Image Test with Unsupported Resolution
- CASE Command:

```bash
dvp open 1024 600 jpeg
```

- CASE Expected Result:

```
Failure
```
- CASE Success Standard:

```
CMDRSP:ERROR
```

#### 4.4.8  - DVP Open Output H265 Image Test with Unsupported Format
- CASE Command:

```bash
dvp open 864 480 h265
```

- CASE Expected Result:

```
Failure
```
- CASE Success Standard:

```
CMDRSP:ERROR
```

### 4.5  Normal Log Printing
- CMDRSP:OK dvp open success
- CMDRSP:ERROR dvp open fail
- dvp:27[322 8KB 2010Kbps] print dvp output info dvp: frame rate[frame number frame size bit rate], print this log every 4-6 seconds