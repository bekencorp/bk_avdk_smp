# JPEG Decoding Example Project

* [中文](./README_CN.md)

## 1. Project Overview

This project is a JPEG decoding test module designed to test JPEG decoding functionality on the Beken platform. It provides a Command Line Interface (CLI) that supports both hardware decoding and software decoding, and also supports running the software decoder on DTCM (which improves decoding speed).

* For detailed information about JPEG decoding, please refer to:

  - [JPEG Decoding Overview](../../../developer-guide/video_codec/jpeg_decoding.html)

  - [JPEG Hardware Decoding Guide](../../../developer-guide/video_codec/jpeg_decoding_hw.html)

  - [JPEG Software Decoding Guide](../../../developer-guide/video_codec/jpeg_decoding_sw.html)

* For API reference, please refer to:

  - [JPEG Hardware Decoder API](../../../api-reference/multimedia/bk_jpegdec_hw.html)

  - [JPEG Software Decoder API](../../../api-reference/multimedia/bk_jpegdec_sw.html)

### 1.1 Test Environment

   * Hardware configuration:
      * Core board, **BK7258_QFN88_9X9_V3.2**
      * PSRAM 8M/16M
   * Supports MJPEG hardware encoding/decoding
      * YUV422
   * Supports MJPEG software decoding
      * YUV420, YUV444, YUV400, YUV422
      * When outputting YUYV format, rotation angles (0°, 90°, 180°, 270°) can be configured

.. warning::
    Please use reference peripherals for familiarization and learning of the demo project. If peripheral specifications are different, the code may need to be reconfigured.

## 2. Directory Structure

The project adopts an AP-CP dual-core architecture, with the main source code located in the AP directory. The project structure is as follows:

```
jpeg_decode_example/
├── .ci                   # CI configuration directory
├── .gitignore            # Git ignore file
├── CMakeLists.txt        # Project-level CMake build file
├── Makefile              # Make build file
├── README.md             # Project documentation (English)
├── README CN.md          # Project documentation (Chinese)
├── ap/                   # AP-side code
│   ├── CMakeLists.txt    # AP-side CMake build file
│   ├── Kconfig.projbuild # Kconfig configuration
│   ├── ap_main.c         # AP main entry file
│   ├── config/           # AP configuration directory
│   └── jpeg_decode/      # JPEG decode implementation
│       ├── data/         # Test JPEG image data
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

- Supports hardware JPEG decoding and software JPEG decoding
- Provides CLI for decoding tests
- Supports retrieving JPEG image dimension information
- Implements frame buffer management mechanism
- Offers regular and abnormal scenario decoding test functionality
- Supports running software decoder on DTCM for faster performance

### 3.2 Frame Buffer Management

The project implements a frame buffer management mechanism for efficiently managing image buffers during JPEG encoding and decoding:

- Supports three image formats: MJPEG, H264, and YUV
- Maintains separate free queues and ready queues for each format
- Provides interfaces for buffer allocation, retrieval, and release operations

### 3.3 JPEG Decoding Process

1. Initialize the JPEG decoder (hardware or software)
2. Open the decoder
3. Perform decoding operations:
   - Allocate input buffer and fill with JPEG data
   - Retrieve image dimension information
   - Allocate output buffer
   - Execute decoding
   - Release buffers
4. Close the decoder
5. Delete the decoder instance

## 4. Compilation and Execution

### 4.1 Compilation Method

Compile the project using the following command:

```
make bk7258 PROJECT=jpeg_decode_example
```

### 4.2 Execution Method

After successful compilation, flash the generated firmware to the development board and use the following commands through the serial terminal to test the JPEG decoding functionality:

Command execution success prints: "CMDRSP:OK"
Command execution failure prints: "CMDRSP:ERROR"

#### 4.2.1 Basic Decoding Commands

1. Initialize hardware JPEG decoder:
```
jpeg_decode init_hw
```

2. Initialize software JPEG decoder:
```
jpeg_decode init_sw
```

3. Initialize software JPEG decoder running on DTCM (core ID optional):
```
jpeg_decode init_sw_on_dtcm [1|2]
```

Choose the appropriate command from 1, 2, and 3 based on your test scenario.

4. Open the decoder:
```
jpeg_decode open
```

5. Perform decoding operation:
YUV422 format image decoding:
```
jpeg_decode dec 422_864_480
```
YUV420 format image decoding:
```
jpeg_decode dec 420_864_480
```

Other supported image formats:
```
jpeg_decode dec 422_865_480
jpeg_decode dec 422_864_479
jpeg_decode dec 420_865_480
jpeg_decode dec 420_864_479
```

6. Close the decoder:
```
jpeg_decode close
```

7. Delete the decoder instance:
```
jpeg_decode delete
```

#### 4.2.2 Regular Test Commands

1. Hardware decoder regular test:
```
jpeg_decode_regular_test hardware_test
```

2. Software decoder regular test:
```
jpeg_decode_regular_test software_test
```

3. Software decoder on DTCM (CP1) regular test:
```
jpeg_decode_regular_test software_dtcm_cp1_test
```

4. Software decoder on DTCM (CP2) regular test:
```
jpeg_decode_regular_test software_dtcm_cp2_test
```

## 5. Test Data

The project includes different formats of JPEG test images stored in the `ap/jpeg_decode/data/` directory. These include:

  * **422_864_480** : YUV422 format JPEG image with 864x480 resolution
  * **420_864_480** : YUV420 format JPEG image with 864x480 resolution
  * **422_865_480** : YUV422 format JPEG image with 865x480 resolution
  * **422_864_479** : YUV422 format JPEG image with 864x479 resolution
  * **420_865_480** : YUV420 format JPEG image with 865x480 resolution
  * **420_864_479** : YUV420 format JPEG image with 864x479 resolution

Hardware decoding only supports decoding YUV422 format images; YUV420 format images will fail to decode.

Hardware decoding requires images to have width as a multiple of 16 and height as a multiple of 8, otherwise decoding will fail.

Software decoding supports decoding both YUV420 and YUV422 format images.

Software decoding requires images to have width as a multiple of 2, with no restrictions on height, otherwise decoding will fail.

## 6. Test Examples

### 6.1 Basic Decoding Test

#### 6.1.1 Hardware Decoding Test

```
jpeg_decode init_hw
jpeg_decode open
jpeg_decode dec 422_864_480
jpeg_decode close
jpeg_decode delete
```

Normal log:
```
cli_jpeg_decode_cmd, XX, bk_hardware_jpeg_decode_new success!
cli_jpeg_decode_cmd, XX, jpeg decode open success!
cli_jpeg_decode_cmd, XX, jpeg decode get img dimensions success! 864x480 2
cli_jpeg_decode_cmd, XX, jpeg decode start success! Decode time: XX ms
cli_jpeg_decode_cmd, XX, jpeg decode delete success!
```

**Supported JPEG Image Formats**: 422_864_480 (Other formats will fail in hardware decoding, see section 5. Test Data for limitations)

#### 6.1.2 Software Decoding Test

```
jpeg_decode init_sw
jpeg_decode open
jpeg_decode dec 420_864_480
jpeg_decode close
jpeg_decode delete
```

Normal log:
```
cli_jpeg_decode_cmd, XX, bk_software_jpeg_decode_new success!
cli_jpeg_decode_cmd, XX, jpeg decode open success!
cli_jpeg_decode_cmd, XX, jpeg decode get img dimensions success! 864x480 2
cli_jpeg_decode_cmd, XX, jpeg decode start success! Decode time: XX ms
cli_jpeg_decode_cmd, XX, jpeg decode delete success!
```

**Supported JPEG Image Formats**: 420_864_480, 422_864_480, 422_864_479, 420_864_479 (Must meet software decoding format limitations, see section 5. Test Data)

#### 6.1.3 Software Decoding Test on DTCM (CP1)

```
jpeg_decode init_sw_on_dtcm 1
jpeg_decode open
jpeg_decode dec 420_864_480
jpeg_decode close
jpeg_decode delete
```

Normal log:
```
cli_jpeg_decode_cmd, XX, bk_software_jpeg_decode_on_dtcm_new success!
cli_jpeg_decode_cmd, XX, jpeg decode open success!
cli_jpeg_decode_cmd, XX, jpeg decode get img dimensions success! 864x480 2
cli_jpeg_decode_cmd, XX, jpeg decode start success! Decode time: XX ms
cli_jpeg_decode_cmd, XX, jpeg decode delete success!
```

**Supported JPEG Image Formats**: Same as software decoding test

#### 6.1.4 Software Decoding Test on DTCM (CP2)

```
jpeg_decode init_sw_on_dtcm 2
jpeg_decode open
jpeg_decode dec 420_864_480
jpeg_decode close
jpeg_decode delete
```

Normal log:
```
cli_jpeg_decode_cmd, XX, bk_software_jpeg_decode_on_dtcm_new success!
cli_jpeg_decode_cmd, XX, jpeg decode open success!
cli_jpeg_decode_cmd, XX, jpeg decode get img dimensions success! 864x480 2
cli_jpeg_decode_cmd, XX, jpeg decode start success! Decode time: XX ms
cli_jpeg_decode_cmd, XX, jpeg decode delete success!
```

**Supported JPEG Image Formats**: Same as software decoding test

### 6.2 Regular Test

The project provides various regular scenario decoding test functions to verify the decoder's performance under normal conditions. Here are the regular test commands and expected results:

#### 6.2.1 Hardware Decoding Test

```
jpeg_decode_regular_test hardware_test
```

Expected log:
```
cli_jpeg_decode_regular_test_cmd, XX, hardware jpeg decode Normal scenario JPEG decoding test completed!
```

Abnormal log (indicating test failure):
```
cli_jpeg_decode_regular_test_cmd, XX, not found this cmd!
```

#### 6.2.2 Software Decoding Test

```
jpeg_decode_regular_test software_test
```

Expected log:
```
cli_jpeg_decode_regular_test_cmd, XX, software jpeg decode Normal scenario JPEG decoding test completed!
```

Abnormal log (indicating test failure):
```
cli_jpeg_decode_regular_test_cmd, XX, not found this cmd!
```

#### 6.2.3 Software Decoding Test on DTCM (CP1)

```
jpeg_decode_regular_test software_dtcm_cp1_test
```

Expected log:
```
cli_jpeg_decode_regular_test_cmd, XX, software jpeg decode Normal scenario JPEG decoding test completed!
```

Abnormal log (indicating test failure):
```
cli_jpeg_decode_regular_test_cmd, XX, not found this cmd!
```

#### 6.2.4 Software Decoding Test on DTCM (CP2)

```
jpeg_decode_regular_test software_dtcm_cp2_test
```

Expected log:
```
cli_jpeg_decode_regular_test_cmd, XX, software jpeg decode Normal scenario JPEG decoding test completed!
```

Abnormal log (indicating test failure):
```
cli_jpeg_decode_regular_test_cmd, XX, not found this cmd!
```

## 7. Notes

1. Ensure the decoder is properly initialized before use
2. Remember to release related resources after decoding operations are completed
3. Hardware decoding and software decoding have different capabilities; choose the appropriate decoding method based on actual needs:
   - Hardware decoding only supports YUV422 format images, and requires image width to be a multiple of 16 and height to be a multiple of 8
   - Software decoding supports both YUV420 and YUV422 format images, and requires image width to be a multiple of 2, with no restrictions on height
4. Software decoders running on DTCM typically provide faster decoding speeds than regular software decoders
5. Frame buffer resources are limited; avoid occupying too many buffers simultaneously
