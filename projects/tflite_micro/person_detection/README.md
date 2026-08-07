# TensorFlow Lite Micro Person Detection Project

* [中文](./README_CN.md)

## 1. Project Overview

This project demonstrates an on-device **person detection** flow on the Beken platform with **TensorFlow Lite Micro (TFLM)** and the **Ethos-U NPU**. The application opens the MIPI camera and uses the ISP MP output as the model input. The shared `avdk_nn_module` component handles model loading, input preprocessing, inference, and detection post-processing.

The default configuration uses the **Gray model with NV12 MP input**. It can also be explicitly switched to the **RGB model with BGRA8888 MP input**.

Main features:

- Initializes camera, frame buffer, media service, and device management
- Loads the Vela-generated `person_detection_vela_tflite` from `avdk_nn_module`
- Runs a 320x180 person detection model on the Ethos-U NPU
- In Gray mode, uses only the valid Y plane from NV12 to reduce data movement
- In RGB mode, receives BGRA8888 MP frames and converts them to RGB inside the model
- Prints detection count, top box coordinates, and score
- Prints tensor arena total size, actual used size, and remaining size after model load

## 2. Directory Structure

```
person_detection/
├── CMakeLists.txt
├── Makefile
├── README.md
├── README_CN.md
├── app.rst
├── ap/
│   ├── CMakeLists.txt
│   ├── ap_main.cc                       # Camera setup, model creation, inference startup
│   └── config/bk7259_ap/
│       ├── defconfig                    # Enables Gray person detection by default
│       └── usr_gpio_cfg.h
├── cp/
└── partitions/bk7259/
    ├── ram_regions.csv
    └── auto_partitions.csv
```

Shared model component:

```
common_components/avdk_nn_module/
├── Kconfig                              # TFLM_PERSON_DETECTION_GRAY/RGB
├── CMakeLists.txt                       # Selects gray or rgb model data by macro
├── include/tflm_person_detect.h         # PersonDetectModel declaration
└── src/tflm_person_detection/
    ├── tflm_person_detection.cc         # Preprocessing, inference, post-processing
    ├── person_detect_gray_model_data.cc # Gray Vela model
    └── person_detect_rgb_model_data.cc  # RGB Vela model
```

## 3. Configuration

### 3.1 Kconfig

Default configuration:

```
CONFIG_TFLITE_MICRO=y
CONFIG_AVDK_VIDEO_REATOR=y
CONFIG_TFLM_PERSON_DETECTION_GRAY=y
# CONFIG_TFLM_PERSON_DETECTION_RGB is not set
CONFIG_NPU=y
```

The input source macros are explicit:

- `CONFIG_TFLM_PERSON_DETECTION_GRAY=y`: compiles the Gray model and configures camera MP as `BK_PIXEL_FORMAT_NV12`
- `CONFIG_TFLM_PERSON_DETECTION_RGB=y`: compiles the RGB model and configures camera MP as `BK_PIXEL_FORMAT_BGRA8888`

Do not enable both at the same time. The `avdk_nn_module` CMake logic compiles only one model data file according to these macros, avoiding duplicate `person_detection_vela_tflite` symbols.

### 3.2 Camera Input

`ap/ap_main.cc` configures camera as follows:

- `mp_enable = true`
- `mp_width = 320`
- `mp_height = 180`
- Gray: `mp_format = BK_PIXEL_FORMAT_NV12`
- RGB: `mp_format = BK_PIXEL_FORMAT_BGRA8888`

The application only calls `OpenCameraWithoutDisplay()` and `start_infer()`. It does not actively open the LCD/display path. LCD, DPU, and GPU configuration remains enabled so display can be connected later if needed.

## 4. Build and Run

From the SDK root:

```bash
make bk7259 PROJECT=tflite_micro/person_detection
```

After flashing, no CLI input is required. The AP side opens the camera and starts the inference thread. Typical log keywords:

```text
M55 main running...
PersonDetection resourceLoad model=... arena=2097152 scratch=131072 format=...
PersonDetectionModel arena total=2097152 used=... free=...
PersonDetection input prepare: ... us
PersonDetection Invoke: ... us
PersonDetection decode + NMS: ... us
PersonDetection count=... top score=... xywh=(...)
```

## 5. Test Plan

- Function: camera opens successfully, without `PersonDetection camera open failed`.
- Initialization: no `bk_ethosu_init failed`, `AllocateTensors failed`, or `PersonDetection input shape mismatch`.
- Input format: Gray mode uses NV12; RGB mode is passed to `run()` as BGRA8888 by `AvdkVideoReator`.
- Result: observe whether `PersonDetection count`, `score`, and `xywh` change with the scene.
- Memory: check `arena total/used/free`; `used` must be below `total` with reasonable margin.

## 6. Model Update Notes

1. Compile the new model with Vela, then replace either `person_detect_gray_model_data.cc` or `person_detect_rgb_model_data.cc`.
2. Keep the model data symbols as `person_detection_vela_tflite[]` and `person_detection_vela_tflite_len`.
3. If input size, channel count, or output shape changes, update input dimensions, anchor configuration, output parsing, and post-processing in `tflm_person_detection.cc`.
4. Adjust `kPersonArenaSize` based on runtime arena usage, and adjust `kPersonScratchSize` according to the Vela report.
5. Partition files are under `partitions/bk7259/`; AP app size, model data, arena, and PSRAM regions must fit the selected model.

## 7. Notes

1. This project performs live camera inference and does not use embedded test images.
2. Gray mode uses only the NV12 Y plane; UV data is not copied into the model input.
3. RGB mode receives BGRA8888 from MP and converts it to RGB inside the model.
4. For RGB models, `AvdkVideoReator` passes BGRA8888 as the actual inference input format.
