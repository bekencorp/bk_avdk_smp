# Video Pipeline Example Project

* [中文](./README_CN.md)

## 1. Project Overview

This project is a video pipeline test module designed to test the video pipeline functionality on the Beken platform. It provides a Command Line Interface (CLI) that supports H.264 encoding, JPEG decoding, and image rotation (both hardware and software rotation).

* For detailed information about Video Pipeline, please refer to:

  - [Video Pipeline Development Guide](../../../developer-guide/video_codec/video_pipeline.html)

* For API reference about Video Pipeline, please refer to:

  - [Video Pipeline API](../../../api-reference/multimedia/bk_video_pipeline.html)

### 1.1 Test Environment

   * Hardware configuration:
      * Core board, **BK7258_QFN88_9X9_V3.2**
      * PSRAM 8M/16M
   * Supports hardware/software rotation
      * 0°, 90°, 180°, 270°
   * Supports MJPEG hardware decoding
      * YUV422
   * Supports MJPEG software decoding
      * YUV420
   * Supports H264 hardware encoding

.. warning::
    Please use reference peripherals for familiarization and learning of the demo project. If peripheral specifications are different, the code may need to be reconfigured.

.. warning::
   The H264 encoded data from YUV422 images is the image before rotation, while the H264 encoded data from YUV420 images is the image after rotation.
   During hardware decoding, decoding is performed first, and the decoded data is simultaneously sent to rotation and H264 encoding, so the H264 encoded data is the pre-rotation data;
   During software decoding, decoding and rotation are performed simultaneously, and the decoded data is then H264 encoded, so the H264 encoded data is the post-rotation data;

## 2. Directory Structure

The project adopts an AP-CP dual-core architecture, with the main source code located in the AP directory. The project structure is as follows:

```
video_pipeline_example/
├── .ci                        # CI configuration files
├── .gitignore                 # Git ignore file
├── CMakeLists.txt             # Project-level CMake build file
├── Makefile                   # Make build file
├── README.md                  # English project documentation
├── README CN.md               # Chinese project documentation
├── ap/                        # Application Processor code
│   ├── CMakeLists.txt         # AP CMake build file
│   ├── Kconfig.projbuild      # AP Kconfig configuration
│   ├── ap_main.c              # AP main entry file
│   ├── config/                # AP configuration files
│   └── video_pipeline/        # Video pipeline implementation
│       ├── data/              # Test data (JPEG images)
│       ├── include/           # Header files
│       └── src/               # Source code files
├── cp/                        # Co-processor code
│   ├── CMakeLists.txt         # CP CMake build file
│   ├── config/                # CP configuration files
│   └── cp_main.c              # CP main entry file
├── partitions/                # Memory partition configurations
│   └── bk7258/                # BK7258 SoC specific partitions
└── pj_config.mk               # Project configuration makefile
```

## 3. Feature Description

### 3.1 Main Features

- Supports H.264 video encoding
- Supports JPEG image decoding
- Provides hardware and software image rotation capabilities
- Offers regular and abnormal scenario test functionality
- Provides Command Line Interface (CLI) for testing various features

### 3.2 Video Pipeline Process

The typical usage flow for the video pipeline is as follows:

1. Initialize the video pipeline: `video_pipeline init`

2. Open specific modules:
   - H.264 encoder: `video_pipeline open_h264e`
   - Rotation module: `video_pipeline open_rotate`

3. Perform operations through the respective APIs

4. Close the modules:
   - H.264 encoder: `video_pipeline close_h264e`
   - Rotation module: `video_pipeline close_rotate`

5. Deinitialize the video pipeline: `video_pipeline deinit`

## 4. Compilation and Execution

### 4.1 Compilation Method

Compile the project using the following command:

```
make bk7258 PROJECT=video_pipeline_example
```

### 4.2 Execution Method

After successful compilation, flash the generated firmware to the development board and use the following commands through the serial terminal to test the video pipeline functionality:

Command execution success prints: "CMDRSP:OK"

Command execution failure prints: "CMDRSP:ERROR"

#### 4.2.1 Basic Video Pipeline Commands

1. Initialize the video pipeline:
```
video_pipeline init
```

2. Deinitialize the video pipeline:
```
video_pipeline deinit
```

3. Open H.264 encoder:
```
video_pipeline open_h264e
```

4. Close H.264 encoder:
```
video_pipeline close_h264e
```

5. Open rotation module:
```
video_pipeline open_rotate
```

6. Close rotation module:
```
video_pipeline close_rotate
```

## 5. Regular Test Commands

The project provides various regular scenario test functions to verify the video pipeline's performance under normal conditions. Here are the regular test commands:

Command execution success prints: "CMDRSP:OK"

Command execution failure prints: "CMDRSP:ERROR"

1. Hardware rotation test:
```
video_pipeline_regular_test hardware_rotate
```

2. Software rotation test:
```
video_pipeline_regular_test software_rotate
```

3. H.264 encoding test:
```
video_pipeline_regular_test h264_encode
```

4. H.264 encoding with hardware rotation test:
```
video_pipeline_regular_test h264_encode_and_hw_rotate
```

5. H.264 encoding with software rotation test:
```
video_pipeline_regular_test h264_encode_and_sw_rotate
```


## 6. Test Data

The project includes different formats of JPEG test images stored in the `ap/video_pipeline/data/` directory. These include:

* **jpeg_data_422_864_480.c** ：YUV422 format JPEG image with 864x480 resolution
* **jpeg_data_420_864_480.c** ：YUV420 format JPEG image with 864x480 resolution
* **jpeg_data_422_864_479.c** ：YUV422 format JPEG image with 864x479 resolution
* **jpeg_data_420_864_479.c** ：YUV420 format JPEG image with 864x479 resolution
* **jpeg_data_422_865_480.c** ：YUV422 format JPEG image with 865x480 resolution
* **jpeg_data_420_865_480.c** ：YUV420 format JPEG image with 865x480 resolution

## 7. Configuration Options

### 7.1 Pipeline Thread Stack Size Configuration

The thread stack sizes for pipeline-related tasks can be configured via Kconfig:

- **CONFIG_JPEG_DECODE_PIPELINE_TASK_STACK_SIZE**: Thread stack size for JPEG decode pipeline task (bytes)
  - Default value: 4096 bytes
  - Configuration path: menuconfig -> Media -> JPEG decode pipeline task stack size

- **CONFIG_JPEG_GET_PIPELINE_TASK_STACK_SIZE**: Thread stack size for JPEG get pipeline task (bytes)
  - Default value: 1024 bytes
  - Configuration path: menuconfig -> Media -> JPEG get pipeline task stack size

- **CONFIG_YUV_ROTATE_PIPELINE_TASK_STACK_SIZE**: Thread stack size for YUV rotate pipeline task (bytes)
  - Default value: 2048 bytes
  - Configuration path: menuconfig -> Media -> YUV rotate pipeline task stack size

- **CONFIG_H264_ENCODE_PIPELINE_TASK_STACK_SIZE**: Thread stack size for H264 encode pipeline task (bytes)
  - Default value: 2048 bytes
  - Configuration path: menuconfig -> Media -> H264 encode pipeline task stack size

Note: Adjust the stack size based on actual usage scenarios and memory resources; increase this value if stack overflow occurs.

## 8. Notes

1. Ensure the video pipeline is properly initialized before use
2. Remember to release related resources after operations are completed
3. Different modules have specific requirements and limitations; refer to the API documentation for details
4. Frame buffer resources are limited; avoid occupying too many buffers simultaneously
5. **Callback Function Usage Notes**:
   - Blocking operations (such as long waits, sleep, etc.) are not recommended in callback functions to avoid impacting decoding performance and system responsiveness
   - It is recommended to perform only lightweight operations in callback functions, such as setting flags, sending messages/semaphores, etc., and move time-consuming operations to other tasks
