# TensorFlow Lite Micro Hand Gesture Detection Project

* [中文](./README_CN.md)

## 1. Project Overview

This project demonstrates an offline **TensorFlow Lite Micro (TFLM) + Ethos-U NPU** hand gesture detection flow on the Beken platform. It runs a Vela-compiled YOLOv8 int8 model on embedded input tensors from `ap/resource/hand_gesture_image_input_*.cc`, then prints boxes, classes, scores, and timing from the AP log.

### 1.1 Test Environment

* Hardware: BK7259 family core board with enough memory for model data, Ethos-U scratch, and tensor arena.
* Software:
  * `CONFIG_TFLITE_MICRO=y`
  * `CONFIG_TFLM_HAND_GESTURE_DETECTION_V1=y`
  * Input tensor shape is defined in `tflm_hand_gesture_detection_model.h`; currently **320×320×3**

.. warning::

    This demo does not read camera frames. It runs the built-in test tensors only. When replacing the model or test images, update the model header, Vela output, and `ap/resource` input tensors together.

## 2. Directory Structure

```
hand_gesture_detection/
├── CMakeLists.txt
├── Makefile
├── README.md
├── README_CN.md
├── app.rst
├── ap/
│   ├── CMakeLists.txt
│   ├── ap_main.c                         # Creates the tflm_demo task
│   ├── app/
│   │   ├── tflm_hand_gesture_detection_demo.cpp
│   │   └── tflm_hand_gesture_detection_demo.h
│   ├── resource/
│   │   ├── hand_gesture_image_input.h
│   │   ├── hand_gesture_image_input_1.cc
│   │   └── hand_gesture_image_input_2.cc
│   └── config/bk7259_ap/defconfig
├── cp/
└── partitions/bk7259/
```

Shared model component:

```
common_components/avdk_nn_module/
├── include/tflm_hand_gesture_detection_model.h
└── src/tflm_hand_gesture_detection/hand_gesture_detect_model_data.cc
```

## 3. Features

### 3.1 Main Features

- Initializes Ethos-U, TFLM `MicroInterpreter`, and tensor arena
- Loads `hand_gesture_detection_vela_tflite` from `avdk_nn_module`
- Runs the built-in `test_hands_1` and `test_hands_2` inputs
- Applies YOLOv8-style post-processing: reverse letterbox mapping, per-class NMS, original-image boxes
- Logs `Invoke()` time with DWT and toggles **GPIO_55** around inference by default

### 3.2 Flow

1. `bk_init()` → `media_service_init()` → `bk_frame_buffer_init()`
2. Create the `tflm_demo` task
3. Allocate Ethos scratch, model copy buffer, and tensor arena
4. Run `AllocateTensors()`, then repeatedly invoke the two embedded images
5. Print detection results and round completion logs

## 4. Build and Run

### 4.1 Build

From the SDK root:

```
make bk7259 PROJECT=tflite_micro/hand_gesture_detection
```

### 4.2 Runtime Logs

After flashing, no CLI input is required. The AP task periodically runs the demo. Typical log keywords:

```
===========>start hand_gesture_detection demo
ethosu scratch -> HSRAM ...
model -> MEM_SLAB_UNCODED ...
arena -> MEM_SLAB_UNCODED ...
arena used ... / ... bytes
model load & init (scratch+ethosu+model+arena+AllocateTensors): ...
test_hands_1 Invoke time: ...
=== test_hands_1 (384x384) detections=... ===
hand_gesture_detection round done
```

## 5. Test Plan

- Function: no `bk_ethosu_init failed`, `AllocateTensors failed`, or `Invoke failed`.
- Result: check `detections`, `class_id`, `score`, and `bbox_orig`.
- Performance: use `{tag} Invoke time`, or measure the GPIO_55 high pulse with a logic analyzer.

### Matching ap/resource/hand_gesture_image_input.h

This header binds **post-processing thresholds**, **original image sizes**, **class names**, and C array symbols from `hand_gesture_image_input_*.cc`. Update it whenever test images, sample count, or class metadata changes.

.. list-table::
   :header-rows: 1

   * - Item
     - Notes
   * - `k_conf_threshold` / `k_iou_threshold`
     - Keep consistent with the PC-side input/reference generation thresholds.
   * - `k_test_hands_1_orig_w` / `k_test_hands_1_orig_h`, etc.
     - Original image sizes used to map model boxes back to image coordinates.
   * - `k_class_names[]`
     - Display names; count must match `k_num_classes` in `tflm_hand_gesture_detection_model.h`.
   * - `extern` `*_model_input` / `*_model_input_len`
     - Must match symbols in `hand_gesture_image_input_*.cc`; add matching calls in `tflm_hand_gesture_detection_run_demo()` for new samples.

### Model Update and Regression

1. Recompile the int8 `.tflite` with Vela settings that match the target Ethos-U configuration, then check Vela summary for scratch, arena, and model sizes.
2. Convert the Vela `.tflite` to a C array and replace `common_components/avdk_nn_module/src/tflm_hand_gesture_detection/hand_gesture_detect_model_data.cc`; keep `hand_gesture_detection_vela_tflite[]` and `hand_gesture_detection_vela_tflite_len` unchanged.
3. If input shape, class count, candidate count, or output channels change, update `k_input_*`, `k_num_classes`, `k_num_candidates`, `k_out_channels`, `TFLM_ARENA_SIZE`, and `ETHOSU_SCRATCH_SIZE` in `tflm_hand_gesture_detection_model.h`.
4. Regenerate `hand_gesture_image_input_*.cc` and `hand_gesture_image_input.h` metadata, then compare device `class_id` / bbox logs with PC reference output and re-check `Invoke time` / GPIO_55 pulse width.

## 6. Configuration

- `TFLM_ARENA_SIZE`: TFLM tensor arena size; must exceed `arena used` in logs.
- `ETHOSU_SCRATCH_SIZE`: Ethos-U scratch size; keep aligned with Vela reports and runtime code.
- `k_input_h` / `k_input_w` / `k_input_c`: model input shape.
- `k_num_classes` / `k_num_candidates` / `k_out_channels`: output layout for post-processing.
- `hand_gesture_image_input.h`: original image sizes, class names, and embedded input symbols.

Current default memory placement and sizes:

.. list-table::
   :header-rows: 1

   * - Type
     - Location
     - Size
     - Source
   * - fast memory / Ethos-U scratch
     - HSRAM
     - **256 KB** (`ETHOSU_SCRATCH_SIZE`)
     - `tflm_hand_gesture_detection_model.h`, `g_ethosu0_scratch_src`
   * - tensor arena
     - MEM_SLAB_UNCODED
     - **3 MB** (`TFLM_ARENA_SIZE`)
     - `tflm_hand_gesture_detection_model.h`, `g_tensor_arena_src`
   * - Vela model copy
     - MEM_SLAB_UNCODED
     - **2,685,984 bytes** (`hand_gesture_detection_vela_tflite_len`)
     - `hand_gesture_detect_model_data.cc`, `g_model_data_src`

## 7. Notes

1. The `tflm_demo` task runs in a loop with about 500 ms delay between rounds.
2. DWT timing assumes a 480 MHz CPU clock; adjust conversion if the clock changes.
3. When updating the model, update Vela output, model header constants, input tensors, and post-processing parameters together.
4. GPIO_55 timing is controlled by `TFLM_INVOKE_TIME_TEST`; disable or remap it if the pin conflicts with your hardware.
