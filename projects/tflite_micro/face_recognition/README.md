# TensorFlow Lite Micro Face Recognition Project

* [中文](./README_CN.md)

## 1. Project Overview

This project demonstrates camera-based **YOLOFace detection with OSD display** on the Beken platform. It uses `YolofaceDetectionModel` from `avdk_nn_module`, opens the MIPI camera and MIPI LCD through `AvdkVideoReatorOSD`, and draws detected face boxes on the preview.

### 1.1 Test Environment

* Hardware:
  * BK7259 family core board
  * MIPI camera, currently configured as **1088×1088 @ 15 fps**
  * MIPI LCD, currently `lcd_device_hx8399c_mipi_1080x1920`
* Software:
  * `CONFIG_TFLITE_MICRO=y`
  * `CONFIG_TFLM_YOLOFACE_V1=y`
  * `CONFIG_BK_CAMERA=y`, `CONFIG_CSI_CAMERA=y`, `CONFIG_LCD_DSI=y`

.. warning::

    Although the project directory is named `face_recognition`, the current application implements face detection and OSD box display only. It does not include face embedding matching or identity enrollment.

## 2. Directory Structure

```
face_recognition/
├── CMakeLists.txt
├── Makefile
├── README.md
├── README_CN.md
├── app.rst
├── ap/
│   ├── CMakeLists.txt
│   ├── ap_main.cc                 # Camera/display/GPU/model flow
│   └── config/bk7259_ap/defconfig
├── cp/
└── partitions/bk7259/
```

Shared model component:

```
common_components/avdk_nn_module/
├── include/YolofaceDetectionModel.h
└── src/tflm_yoloface_detection/
    ├── YolofaceDetectionModel.cc
    └── yoloface_detect_model_data.cc
```

## 3. Features

### 3.1 Main Features

- Initializes media service, frame buffer, camera, display, and GPU configuration
- Runs `YolofaceDetectionModel` for face detection
- Uses a **56×56** model input and maps boxes to a 1088×1088 display canvas
- Draws retained face boxes with `box_detection_path_build()`
- Logs the top box score and `xywh`

### 3.2 Flow

1. `bk_init()` → `media_service_init()` → `bk_auxldo_enable()` → `bk_frame_buffer_init()`
2. Configure camera, display, GPU, then call `devices_mgmt_init()`
3. Create `YolofaceDetectionModel` and register the box callback
4. `AvdkVideoReatorOSD` opens camera and display
5. Start the detection and OSD display pipeline

## 4. Build and Run

### 4.1 Build

From the SDK root:

```
make bk7259 PROJECT=tflite_micro/face_recognition
```

### 4.2 Runtime Logs

No CLI input is required after flashing. Typical log keywords:

```
AP main running...
detection_box_cb: score=..., x=..., y=..., w=..., h=...
```

The LCD should show camera preview. Face boxes appear when faces enter the frame.

## 5. Test Plan

- Camera: preview is stable.
- Detection: `detection_box_cb` logs appear and boxes are drawn when a face is visible.
- Stability: no `Invoke failed`, allocation failure, or display abnormality during long runs.

### Model Resource and Detection Pipeline Updates

.. list-table::
   :header-rows: 1

   * - Item
     - Notes
   * - `yoloface_detect_model_data.cc` / `.h`
     - Holds `g_yoloface_int8_vela_tflite[]` and `YOLOFACE_MODEL_DATA_SIZE`; keep symbols and size aligned when replacing the model.
   * - `YolofaceDetectionModel::resourceLoad()`
     - Owns input size, pixel format, model location, fast memory, and arena placement/size; current input is **56×56**.
   * - `YolofaceDetectionModel::resolverLoad()`
     - Add resolver ops if the new model uses additional operators, otherwise `AllocateTensors()` may fail.
   * - `YolofaceDetectionModel::run()`
     - Contains input format checks, preprocessing, and post-processing; update it when output layout, anchors, or thresholds change.
   * - `detection_box_cb()`
     - OSD mapping currently assumes **56×56** model coordinates to a **1088×1088** display canvas; update it when model input or canvas size changes.

Regression should cover camera preview, face box placement, multi-face scenes, long-run stability, and no `AllocateTensors failed` / `Invoke failed` errors.

## 6. Configuration

- Camera input: `camera_board.mipi` and `camera_board.isp` in `ap_main.cc`.
- Display output: `display_board.mipi.panel` and `display_board.dpu_video`.
- GPU rotation/scaling: `gpu_board.flexa`.
- Model switch: `CONFIG_TFLM_YOLOFACE_V1`.
- Model data: `yoloface_detect_model_data.cc`.

Current default memory placement and sizes (see `YolofaceDetectionModel::resourceLoad()`):

.. list-table::
   :header-rows: 1

   * - Type
     - Location
     - Size
     - Source
   * - fast memory / Ethos-U scratch
     - HSRAM
     - **40 KB**
     - `fast_ram_type` / `fast_ram_data_size`
   * - tensor arena
     - HSRAM
     - **80 KB**
     - `arena_ram_type` / `arena_data_size`
   * - Vela model
     - Flash (not copied to RAM)
     - **34,416 bytes**
     - `YOLOFACE_MODEL_DATA_SIZE` / `g_yoloface_int8_vela_tflite_size`

## 7. Notes

1. This project performs face detection only; it does not register faces or recognize identities.
2. Box mapping assumes the current model input and 1088×1088 canvas. Review callback mapping when changing model size.
3. When changing camera or LCD hardware, update pins, resolution, and panel configuration.
4. Model resources are conditionally compiled by `avdk_nn_module`; keep `CONFIG_TFLM_YOLOFACE_V1=y`.
