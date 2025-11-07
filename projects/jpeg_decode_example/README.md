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
- Supports hardware asynchronous decoding and burst mode testing
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
   - Configure input and output frame dimensions: Set the parsed image width and height to input and output frame structures
   - Allocate output buffer
   - Execute decoding (if rotation angle is configured, output frame width and height will be reconfigured internally based on the rotation angle during decoding)
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

4. Initialize hardware optimized JPEG decoder:
```
jpeg_decode init_hw_opt
```

Choose the appropriate command from 1, 2, 3, and 4 based on your test scenario.

5. Open the decoder:
```
jpeg_decode open
```

6. Perform decoding operation:
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

7. Close the decoder:
```
jpeg_decode close
```

8. Delete the decoder instance:
```
jpeg_decode delete
```

#### 4.2.2 Regular Test Commands

1. Hardware decoder regular test:
```
jpeg_decode_regular_test hardware_test
```

2. Hardware decoder asynchronous test:
```
jpeg_decode_regular_test hardware_async_test
```

3. Hardware decoder asynchronous burst test (10 consecutive times):
```
jpeg_decode_regular_test hardware_async_burst_test
```

4. Software decoder regular test:
```
jpeg_decode_regular_test software_test
```

5. Software decoder on DTCM (CP1) regular test:
```
jpeg_decode_regular_test software_dtcm_cp1_test
```

6. Software decoder on DTCM (CP2) regular test:
```
jpeg_decode_regular_test software_dtcm_cp2_test
```

7. Software decoder on DTCM (CP1) asynchronous test:
```
jpeg_decode_regular_test software_dtcm_cp1_async_test
```

8. Software decoder on DTCM (CP1) asynchronous burst test (10 consecutive times):
```
jpeg_decode_regular_test software_dtcm_cp1_async_burst_test
```

9. Software decoder on DTCM (CP2) asynchronous test:
```
jpeg_decode_regular_test software_dtcm_cp2_async_test
```

10. Software decoder on DTCM (CP2) asynchronous burst test (10 consecutive times):
```
jpeg_decode_regular_test software_dtcm_cp2_async_burst_test
```

11. Software decoder on DTCM (CP1+CP2) asynchronous test:
```
jpeg_decode_regular_test software_dtcm_cp1_cp2_async_test
```

12. Software decoder on DTCM (CP1+CP2) asynchronous burst test (10 consecutive times):
```
jpeg_decode_regular_test software_dtcm_cp1_cp2_async_burst_test
```

13. Hardware optimized decoder regular test (optional parameter: 0=single buffer mode, 1=ping-pong mode):
```
jpeg_decode_regular_test hardware_opt_test [0|1]
```
Examples:
```
jpeg_decode_regular_test hardware_opt_test       # Use single buffer mode (default)
jpeg_decode_regular_test hardware_opt_test 0     # Use single buffer mode
jpeg_decode_regular_test hardware_opt_test 1     # Use ping-pong mode
```

14. Hardware optimized decoder asynchronous test (optional parameter: 0=single buffer mode, 1=ping-pong mode):
```
jpeg_decode_regular_test hardware_opt_async_test [0|1]
```

15. Hardware optimized decoder asynchronous burst test (optional parameters: count=burst count, default 10; 0=single buffer mode, 1=ping-pong mode):
```
jpeg_decode_regular_test hardware_opt_async_burst_test [count] [0|1]
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

#### 6.1.5 Hardware Optimized Decoder Test

```
jpeg_decode init_hw_opt
jpeg_decode open
jpeg_decode dec 422_864_480
jpeg_decode close
jpeg_decode delete
```

Normal log:
```
cli_jpeg_decode_cmd, XX, bk_hardware_jpeg_decode_opt_new success!
cli_jpeg_decode_cmd, XX, jpeg decode open success!
cli_jpeg_decode_cmd, XX, jpeg decode get img dimensions success! 864x480 2
cli_jpeg_decode_cmd, XX, jpeg decode start success! Decode time: XX ms
cli_jpeg_decode_cmd, XX, jpeg decode delete success!
```

**Supported JPEG Image Formats**: 422_864_480 (Same limitations as hardware decoding)

**Features**:
- Uses SRAM buffering for optimized decoding, reducing peak memory usage
- Supports Ping-Pong buffering mode for improved efficiency
- Configurable copy method (MEMCPY or DMA)

### 6.2 Regular Test

The project provides various regular scenario decoding test functions to verify the decoder's performance under normal conditions. Here are the regular test commands and expected results:

#### 6.2.1 Hardware Decoding Test

This command is a synchronous decoding command; the function returns only after decoding is completed. It first prints jpeg_decode_out_complete, then prints perform_jpeg_decode_async_test.

```
jpeg_decode_regular_test hardware_test
```

Expected log:
```
ap1:jdec_com:D(XX):jpeg_decode_out_complete, XX, jpeg decode success! format_type: 5, out_frame: 0xXX
ap0:jdec_com:D(XX):perform_jpeg_decode_async_test, XX, jpeg async decode success! Decode time: 30 ms
```

Abnormal log (indicating test failure):
```
CMDRSP:ERROR
```

#### 6.2.2 Hardware Decoding Asynchronous Test

This command is an asynchronous decoding command; the function returns immediately after the command is sent successfully. It first prints perform_jpeg_decode_async_test, then prints jpeg_decode_out_complete.

```
jpeg_decode_regular_test hardware_async_test
```

Expected log:
```
ap0:jdec_com:D(XX):perform_jpeg_decode_async_test, XX, jpeg async decode success!
ap1:jdec_com:D(XX):jpeg_decode_out_complete, XX, jpeg decode success! format_type: 5, out_frame: 0xXX
```

Abnormal log (indicating test failure):
```
CMDRSP:ERROR
```

#### 6.2.3 Hardware Decoding Asynchronous Burst Test

This command is an asynchronous burst test command that calls the asynchronous decoding function multiple times at once. Images are first stored in a queue, then data is retrieved from the queue for decoding sequentially.
After printing perform_jpeg_decode_async_burst_test multiple times consecutively, it prints jpeg_decode_out_complete.

```
jpeg_decode_regular_test hardware_async_burst_test
```

Expected log:
```
ap0:jdec_com:I(XX):perform_jpeg_decode_async_burst_test, XX, Start hardware_test with 10 bursts!
ap0:jdec_com:D(XX):perform_jpeg_decode_async_burst_test, XX, Burst test 1/10
...
ap0:jdec_com:D(XX):perform_jpeg_decode_async_burst_test, XX, Burst test 10/10
ap0:jdec_com:D(XX):jpeg_decode_out_complete, XX, jpeg decode success! format_type: 5, out_frame: 0xXX
...
ap1:jdec_com:D(XX):jpeg_decode_out_complete, XX, jpeg decode success! format_type: 5, out_frame: 0xXX
```

Abnormal log (indicating test failure):
```
CMDRSP:ERROR
```

#### 6.2.4 Software Decoding Test

```
jpeg_decode_regular_test software_test
```

Expected log:
```
cli_jpeg_decode_regular_test_cmd, XX, software jpeg decode Normal scenario JPEG decoding test completed!
```

Abnormal log (indicating test failure):
```
CMDRSP:ERROR
```

#### 6.2.5 Software Decoding Test on DTCM (CP1)

```
jpeg_decode_regular_test software_dtcm_cp1_test
```

Expected log:
```
cli_jpeg_decode_regular_test_cmd, XX, software jpeg decode Normal scenario JPEG decoding test completed!
```

Abnormal log (indicating test failure):
```
CMDRSP:ERROR
```

#### 6.2.6 Software Decoding Test on DTCM (CP2)

```
jpeg_decode_regular_test software_dtcm_cp2_test
```

Expected log:
```
cli_jpeg_decode_regular_test_cmd, XX, software jpeg decode Normal scenario JPEG decoding test completed!
```

Abnormal log (indicating test failure):
```
CMDRSP:ERROR
```

#### 6.2.7 Software Decoder on DTCM (CP1) Asynchronous Test

```
jpeg_decode_regular_test software_dtcm_cp1_async_test
```

Expected log:
```
cli_jpeg_decode_regular_test_cmd, XX, software jpeg async decode on CP1 test completed!
```

Abnormal log (indicating test failure):
```
CMDRSP:ERROR
```

#### 6.2.8 Software Decoder on DTCM (CP1) Asynchronous Burst Test

```
jpeg_decode_regular_test software_dtcm_cp1_async_burst_test
```

Expected log:
```
cli_jpeg_decode_regular_test_cmd, XX, software jpeg async burst decode on CP1 test completed!
```

Abnormal log (indicating test failure):
```
CMDRSP:ERROR
```

#### 6.2.9 Software Decoder on DTCM (CP2) Asynchronous Test

```
jpeg_decode_regular_test software_dtcm_cp2_async_test
```

Expected log:
```
cli_jpeg_decode_regular_test_cmd, XX, software jpeg async decode on CP2 test completed!
```

Abnormal log (indicating test failure):
```
CMDRSP:ERROR
```

#### 6.2.10 Software Decoder on DTCM (CP2) Asynchronous Burst Test

```
jpeg_decode_regular_test software_dtcm_cp2_async_burst_test
```

Expected log:
```
cli_jpeg_decode_regular_test_cmd, XX, software jpeg async burst decode on CP2 test completed!
```

Abnormal log (indicating test failure):
```
CMDRSP:ERROR
```

#### 6.2.11 Software Decoder on DTCM (CP1+CP2) Asynchronous Test

```
jpeg_decode_regular_test software_dtcm_cp1_cp2_async_test
```

Expected log:
```
cli_jpeg_decode_regular_test_cmd, XX, software jpeg async decode on CP1+CP2 test completed!
```

Abnormal log (indicating test failure):
```
CMDRSP:ERROR
```

#### 6.2.12 Software Decoder on DTCM (CP1+CP2) Asynchronous Burst Test

```
jpeg_decode_regular_test software_dtcm_cp1_cp2_async_burst_test
```

Expected log:
```
cli_jpeg_decode_regular_test_cmd, XX, software jpeg async burst decode on CP1+CP2 test completed!
```

Abnormal log (indicating test failure):
```
CMDRSP:ERROR
```

#### 6.2.13 Hardware Optimized Decoder Regular Test

This test supports an optional parameter to select buffer mode:

Single buffer mode test (default):
```
jpeg_decode_regular_test hardware_opt_test
```
or
```
jpeg_decode_regular_test hardware_opt_test 0
```

Ping-pong buffer mode test:
```
jpeg_decode_regular_test hardware_opt_test 1
```

Expected log:
```
cli_jpeg_decode_regular_test_cmd, XX, Using single buffer mode (or Using pingpong mode)
cli_jpeg_decode_regular_test_cmd, XX, hardware opt jpeg decode Normal scenario JPEG decoding test completed!
```

Abnormal log (indicating test failure):
```
CMDRSP:ERROR
```

**Notes**:
- Parameter 0 or no parameter: Use single buffer mode for lower memory usage
- Parameter 1: Use ping-pong dual buffer mode for higher decoding efficiency

#### 6.2.14 Hardware Optimized Decoder Asynchronous Test

This test supports an optional parameter to select buffer mode:

Single buffer mode test (default):
```
jpeg_decode_regular_test hardware_opt_async_test
```
or
```
jpeg_decode_regular_test hardware_opt_async_test 0
```

Ping-pong buffer mode test:
```
jpeg_decode_regular_test hardware_opt_async_test 1
```

Expected log:
```
ap0:jdec_com:D(XX):perform_jpeg_decode_async_test, XX, jpeg async decode success!
ap1:jdec_com:D(XX):jpeg_decode_out_complete, XX, jpeg decode success! format_type: 5, out_frame: 0xXX
cli_jpeg_decode_regular_test_cmd, XX, hardware opt jpeg async decode test completed!
```

Abnormal log (indicating test failure):
```
CMDRSP:ERROR
```

**Notes**:
- Parameter 0 or no parameter: Use single buffer mode for lower memory usage
- Parameter 1: Use ping-pong dual buffer mode for higher decoding efficiency

#### 6.2.15 Hardware Optimized Decoder Asynchronous Burst Test

This command is an asynchronous burst test that calls the asynchronous decoding function multiple times at once.

```
jpeg_decode_regular_test hardware_opt_async_burst_test
```

Or with optional parameters:
```
jpeg_decode_regular_test hardware_opt_async_burst_test [count] [0|1]
```

Expected log:
```
ap0:jdec_com:I(XX):perform_jpeg_decode_async_burst_test, XX, Start hardware_opt_async_burst_test with 10 bursts!
ap0:jdec_com:D(XX):perform_jpeg_decode_async_burst_test, XX, Burst test 1/10
...
ap0:jdec_com:D(XX):perform_jpeg_decode_async_burst_test, XX, Burst test 10/10
ap0:jdec_com:D(XX):jpeg_decode_out_complete, XX, jpeg decode success! format_type: 5, out_frame: 0xXX
...
```

Abnormal log (indicating test failure):
```
CMDRSP:ERROR
```

**Notes**:
- count parameter is optional, default is 10
- Parameter 0 or no parameter: Use single buffer mode for lower memory usage
- Parameter 1: Use ping-pong dual buffer mode for higher decoding efficiency

## 7. Configuration Options

### 7.1 Thread Stack Size Configuration

The decoder thread stack sizes can be configured via Kconfig:

- **CONFIG_HW_JPEG_DECODE_TASK_STACK_SIZE**: Thread stack size for hardware JPEG decode task (bytes)
  - Default value: 1024 bytes
  - Configuration path: menuconfig -> JPEG Decoder -> Hardware JPEG decode task stack size

- **CONFIG_HW_JPEG_DECODE_OPT_TASK_STACK_SIZE**: Thread stack size for hardware optimized JPEG decode task (bytes)
  - Default value: 2048 bytes
  - Configuration path: menuconfig -> JPEG Decoder -> Hardware optimized JPEG decode task stack size

- **CONFIG_SW_JPEG_DECODE_TASK_STACK_SIZE**: Thread stack size for software JPEG decode task (bytes)
  - Default value: 1024 bytes
  - Configuration path: menuconfig -> JPEG Decoder -> Software JPEG decode task stack size

Note: Adjust the stack size based on actual decoding scenarios and memory resources; increase this value if stack overflow occurs.

### 7.2 Hardware Optimized Decoder Configuration

The hardware optimized decoder provides the following configuration options:

- **is_pingpong**: Whether to enable Ping-Pong buffering mode
  - true: Enable dual buffering for improved parallelism
  - false: Use single buffering to reduce memory usage

- **copy_method**: Data copy method
  - JPEG_DECODE_OPT_COPY_METHOD_MEMCPY: Use os_memcpy for data transfer
  - JPEG_DECODE_OPT_COPY_METHOD_DMA: Use DMA for data transfer (currently falls back to MEMCPY)

- **sram_buffer**: SRAM buffer pointer
  - NULL: Automatically allocate SRAM buffer
  - Non-NULL: Use specified SRAM buffer

- **lines_per_block**: Number of lines to decode per block
  - Must be 8 or 16
  - Recommended value: 16 (suitable for most scenarios)

- **image_max_width**: Maximum image width
  - Used to calculate SRAM buffer size
  - Default value: 864

## 8. Notes

1. Ensure the decoder is properly initialized before use
2. Remember to release related resources after decoding operations are completed
3. Hardware decoding and software decoding have different capabilities; choose the appropriate decoding method based on actual needs:
   - Hardware decoding only supports YUV422 format images, and requires image width to be a multiple of 16 and height to be a multiple of 8
   - Hardware optimized decoding has the same format limitations as hardware decoding, but uses SRAM buffering optimization for lower peak memory usage
   - Software decoding supports both YUV420 and YUV422 format images, and requires image width to be a multiple of 2, with no restrictions on height
4. Software decoders running on DTCM typically provide faster decoding speeds than regular software decoders
5. Frame buffer resources are limited; avoid occupying too many buffers simultaneously
6. In the input frame, the length within the structure needs to be set to the actual valid data length, and in the output frame, the size within the structure needs to be set to the maximum storable size;
   A length of 0 for the input frame or a size of 0 for the output frame will result in decoding errors;
7. **Image Dimension and Rotation Processing Notes**:

   - After retrieving JPEG image information, the decoder sets the parsed width and height to the input and output frame structures
   - For software decoding, if a rotation angle is configured, the output frame's width and height will be reconfigured internally based on the rotation angle:

     * For 90 or 270 degree rotation: Output frame width and height are swapped (width = original height, height = original width)
     * For 0 or 180 degree rotation: Output frame maintains original width and height

   - Hardware decoding and hardware optimized decoding do not support rotation; output frame dimensions remain consistent with input frame dimensions

8. **Hardware Optimized Decoder Usage Recommendations**:
   - Recommended for memory-constrained application scenarios
   - Ping-Pong mode is suitable for high-throughput scenarios
   - Single buffer mode is suitable for memory-constrained scenarios
   - lines_per_block is recommended to be set to 16 for optimal performance
   - SRAM buffer can be reused to avoid frequent allocation and deallocation

9. **Callback Function Usage Notes**:
   - Blocking operations (such as long waits, sleep, etc.) are not recommended in callback functions to avoid impacting decoding performance and system responsiveness
   - It is recommended to perform only lightweight operations in callback functions, such as setting flags, sending messages/semaphores, etc., and move time-consuming operations to other tasks
