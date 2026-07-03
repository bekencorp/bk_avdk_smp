# TensorFlow Lite Micro Palm Recognition Project

* [中文](./README_CN.md)

## 1. Project Overview

This project demonstrates camera-based **palm detection, OSD display, and servo tracking** on the Beken platform. It uses `PalmDetectionModel` from `avdk_nn_module`, opens the MIPI camera and MIPI LCD through `AvdkVideoReatorOSD`, draws detection boxes on the display, and drives a PWM servo to follow the largest detected palm.

### 1.1 Test Environment

* Hardware:
  * BK7259 family core board
  * MIPI camera, currently configured as **1088×1088 @ 15 fps**
  * MIPI LCD, currently `lcd_device_hx8399c_mipi_1080x1920`
  * Optional PWM servo, default `SERVO_CHAN=0`
* Software:
  * `CONFIG_TFLITE_MICRO=y`
  * `CONFIG_TFLM_PALM_DETECTION_V1=y`
  * `CONFIG_BK_CAMERA=y`, `CONFIG_CSI_CAMERA=y`, `CONFIG_LCD_DSI=y`

.. warning::

    Camera, LCD, and servo pins are tied to this board configuration. When changing modules or panels, review `ap_main.cc`, `usr_gpio_cfg.h`, and `servo.h`.

## 2. Directory Structure

```
palm_recognition/
├── CMakeLists.txt
├── Makefile
├── README.md
├── README_CN.md
├── app.rst
├── ap/
│   ├── CMakeLists.txt
│   ├── ap_main.cc                 # Camera/display/GPU/model flow
│   ├── servo.c                    # PWM servo driver and tracking controller
│   ├── servo.h
│   └── config/bk7259_ap/defconfig
├── cp/
└── partitions/bk7259/
```

Shared model component:

```
common_components/avdk_nn_module/
├── include/PalmDetectionModel.h
└── src/tflm_palm_detection/
    ├── PalmDetectionModel.cc
    └── palm_detect_model_data.cc
```

## 3. Features

### 3.1 Main Features

- Initializes media service, frame buffer, camera, display, and GPU configuration
- Runs `PalmDetectionModel` with 256×256 palm detection input
- Maps detection boxes to a 1088×1088 OSD canvas with `box_detection_path_build()`
- Selects the largest palm box as the servo tracking target
- Uses `palm_track_servo()` to incrementally adjust servo angle from palm center offset

### 3.2 Flow

1. `bk_init()` → `media_service_init()` → `bk_auxldo_enable()` → `bk_frame_buffer_init()`
2. Configure camera, display, GPU, then call `devices_mgmt_init()`
3. Initialize the servo and move it to center
4. Create `PalmDetectionModel` and register the box callback
5. `AvdkVideoReatorOSD` opens camera/display and starts the detection/display pipeline

## 4. Build and Run

### 4.1 Build

From the SDK root:

```
make bk7259 PROJECT=tflite_micro/palm_recognition
```

### 4.2 Runtime Logs

No CLI input is required after flashing. Typical log keywords:

```
M55 main running...
[servo] self-test start, chan=0
PalmDetection: ...
detection_box_cb: count=... target=... score=... xywh=(...)
[track] axis=cx ...
```

The LCD should show the camera preview and palm boxes. If a servo is connected, it follows the target palm.

## 5. Test Plan

- Camera: preview is stable, without black screen or corrupted frames.
- Detection: palm boxes appear and logs show `PalmDetection` / `detection_box_cb`.
- Servo: `[track]` logs appear when the palm is off center, and servo motion is smooth.

### Model Resource and Peripheral Bring-up

.. list-table::
   :header-rows: 1

   * - Item
     - Notes
   * - `palm_detect_model_data.cc` / `.h`
     - Holds `palm_detection_builtin_256_integer_quant_vela_tflite[]` and its size; keep symbols aligned with the header when replacing the model.
   * - `PalmDetectionModel::resourceLoad()`
     - Owns model input size, pixel format, model location, fast memory, and arena placement/size; update `width` / `height` and `arena_data_size` when the model changes.
   * - `PalmDetectionModel::resolverLoad()`
     - Add resolver ops if the new model uses additional operators, otherwise `AllocateTensors()` may fail.
   * - `detection_box_cb()`
     - OSD mapping currently assumes **256×256** model coordinates to a **1088×1088** display canvas; update it when model input or canvas size changes.
   * - `servo.h` / `servo.c`
     - Tune servo direction, gain, dead band, and max step for the mechanical setup.

Regression should cover camera preview, box placement, largest-area target selection, servo direction, and long-run memory stability.

## 6. Configuration

- Camera input: `camera_board.mipi` and `camera_board.isp` in `ap_main.cc`.
- Display output: `display_board.mipi.panel` and `display_board.dpu_video`.
- GPU rotation/scaling: `gpu_board.flexa`.
- Servo tuning: `SERVO_CHAN`, `SERVO_TRACK_GAIN`, `SERVO_TRACK_MAX_STEP`, `SERVO_TRACK_DIR` in `servo.h`.
- Model switch: `CONFIG_TFLM_PALM_DETECTION_V1`.

Current default memory placement and sizes (see `PalmDetectionModel::resourceLoad()`):

.. list-table::
   :header-rows: 1

   * - Type
     - Location
     - Size
     - Source
   * - fast memory / Ethos-U scratch
     - HSRAM
     - **128 KB**
     - `fast_ram_type` / `fast_ram_data_size`
   * - tensor arena
     - PSRAM_SLAB (`MEM_SLAB_HEAP_CODED`)
     - **2 MB**
     - `arena_ram_type` / `arena_data_size`
   * - Vela model copy
     - PSRAM_SLAB (`MEM_SLAB_HEAP_CODED`)
     - **2,207,168 bytes**
     - `model_ram_type` / `palm_detection_builtin_256_integer_quant_vela_tflite_size`

## 7. Notes

1. Detection input and OSD mapping currently assume 256×256 → 1088×1088. Update the mapping when changing model size.
2. If servo direction is reversed for your mechanical setup, adjust `SERVO_TRACK_DIR`.
3. If the servo GPIO conflicts with other functions, enable and edit the `SERVO_USE_CUSTOM_GPIO` mapping.
4. This project depends on camera, display, and model resources; review memory and peripheral settings when porting.
