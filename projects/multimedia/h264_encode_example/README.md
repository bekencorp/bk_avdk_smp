# H264 Encoding Example Project

* [中文](./README_CN.md)

## 1. Project Overview

This project is an H264 encoding test module designed to test H264 encoding functionality on the Beken platform. It provides a Command Line Interface (CLI) that supports H264 hardware encoding.

* For detailed information about H264 encoding, please refer to:

  - [H264 Encoding Overview](../../../developer-guide/video_codec/h264_encoding.html)

* For API reference, please refer to:

  - [H264 Encoder API](../../../api-reference/multimedia/bk_h264_encode.html)

### 1.1 Test Environment

   * Hardware configuration:
      * Core board, **BK7258_QFN88_9X9_V3.2**
      * PSRAM 8M/16M
   * Supports H264 hardware encoding
      * YUV420, YUV422 formats
      * Input: YUV frames from frame buffer
      * Output: H264 encoded stream

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

- Supports H264 hardware encoding
- Provides CLI for encoding tests
- Implements frame buffer management mechanism
- Offers regular and abnormal scenario encoding test functionality
- Supports asynchronous encoding

### 3.2 Frame Buffer Management

The project implements a frame buffer management mechanism for efficiently managing image buffers during H264 encoding:

- Supports H264 and YUV formats
- Maintains separate free queues and ready queues for each format
- Provides interfaces for buffer allocation, retrieval, and release operations

### 3.3 H264 Encoding Process

1. Initialize the H264 encoder
2. Open the encoder
3. Perform encoding operations:
   - Get input YUV frame from frame buffer
   - Request output buffer for encoded data
   - Execute encoding (encoding is asynchronous, result is returned in callback)
   - Release buffers in callback
4. Close the encoder
5. Delete the encoder instance

## 4. Compilation and Execution

### 4.1 Compilation Method

Compile the project using the following command:

```
make bk7258 PROJECT=h264_encode_example
```

### 4.2 Execution Method

After successful compilation, flash the generated firmware to the development board and use the following commands through the serial terminal to test the H264 encoding functionality:

Command execution success prints: "CMDRSP:OK"
Command execution failure prints: "CMDRSP:ERROR"

#### 4.2.1 Basic Encoding Commands

1. Initialize H264 encoder:
```
h264_encode init
```

2. Open the encoder:
```
h264_encode open
```

3. Perform encoding operation:
```
h264_encode encode
```

4. Force IDR frame:
```
h264_encode force_idr
```

5. Close the encoder:
```
h264_encode close
```

6. Delete the encoder instance:
```
h264_encode delete
```

#### 4.2.2 Regular Test Commands

1. Normal encoding test:
```
h264_encode_regular_test normal_test
```

2. Asynchronous encoding test:
```
h264_encode_regular_test async_test
```

#### 4.2.3 Error Test Commands

1. NULL handle test:
```
h264_encode_error_test null_handle_test
```

2. Invalid config test:
```
h264_encode_error_test invalid_config_test
```

## 5. Test Examples

### 5.1 Basic Encoding Test

```
h264_encode init
h264_encode open
h264_encode encode
h264_encode close
h264_encode delete
```

Normal log:
```
h264_enc_cli, XX, h264 encode init success!
h264_enc_cli, XX, h264 encode open success!
h264_enc_common, XX, h264 encode success! Encode time: XX ms
h264_enc_cli, XX, h264 encode close success!
h264_enc_cli, XX, h264 encode delete success!
```

### 5.2 Regular Test

#### 5.2.1 Normal Encoding Test

```
h264_encode_regular_test normal_test
```

Expected log:
```
h264_enc_regular, XX, H264 encode normal scenario test completed!
```

Abnormal log (indicating test failure):
```
CMDRSP:ERROR
```

#### 5.2.2 Asynchronous Encoding Test

```
h264_encode_regular_test async_test
```

Expected log:
```
h264_enc_regular, XX, H264 encode async test completed!
```

Abnormal log (indicating test failure):
```
CMDRSP:ERROR
```

## 6. Configuration Options

### 6.1 Encoder Configuration

The H264 encoder provides the following configuration options:

- **buffer_request_cb**: Callback function for requesting output buffer
  - Called when encoder needs a buffer for encoded data
  - Should return a pointer to allocated buffer or NULL on failure

- **buffer_complete_cb**: Callback function for encoding completion
  - Called when encoding is complete
  - Parameters: buffer pointer and result code

- **chnl_id**: Channel ID for the encoder
  - Default: 0

- **param**: User-defined parameter
  - Can be used to pass context to callbacks

## 7. Notes

1. Ensure the encoder is properly initialized before use
2. Remember to release related resources after encoding operations are completed
3. H264 encoding requires YUV input frames from frame buffer
4. Frame buffer resources are limited; avoid occupying too many buffers simultaneously
5. Encoding is asynchronous by default; results are returned in the buffer_complete_cb callback
6. The input frame should contain valid YUV data with correct width and height
7. **Callback Function Usage Notes**:
   - Blocking operations (such as long waits, sleep, etc.) are not recommended in callback functions to avoid impacting encoding performance and system responsiveness
   - It is recommended to perform only lightweight operations in callback functions, such as setting flags, sending messages/semaphores, etc., and move time-consuming operations to other tasks
8. **Buffer Management**:
   - Input buffers are obtained from frame buffer display queue
   - Output buffers are allocated via buffer_request_cb callback
   - Both input and output buffers should be released after use
9. **Force IDR Frame**:
   - Use force_idr command to force the next frame to be encoded as an IDR frame
   - Useful for stream synchronization or error recovery
