# TensorFlow Lite Micro Pet Detection (Cat/Dog YOLOv8) Project

* [中文](./README_CN.md)

## 1. Project Overview

This project demonstrates an end-to-end on-device **cat/dog detection** flow on the Beken platform with **TensorFlow Lite Micro (TFLM)** and **Ethos-U NPU**. The YOLOv8 int8 model is compiled by **Vela** and stored in the shared `avdk_nn_module` component. The application runs offline inference on embedded preprocessed input tensors.

### 1.1 Test Environment

* Hardware: BK7259 family core board with enough memory for model data, Ethos-U scratch, and tensor arena.
* Software:
  * `CONFIG_TFLITE_MICRO=y`
  * `CONFIG_TFLM_PET_DETECTION_V1=y`
  * Current model input is **320×320×3**, with two classes: cat and dog.

.. warning::

    `CONFIG_TFLM_PET_DETECTION_V1` must be enabled. Otherwise `pet_detect_model_data.cc` is not compiled and the link will miss `pet_detection_vela_tflite`.

## 2. Directory Structure

```
pet_detection/
├── CMakeLists.txt
├── Makefile
├── README.md
├── README_CN.md
├── app.rst
├── ap/
│   ├── CMakeLists.txt
│   ├── ap_main.c                     # Creates the tflm_demo task
│   ├── app/
│   │   ├── tflm_pet_detection_demo.cpp
│   │   └── tflm_pet_detection_demo.h
│   ├── resource/
│   │   ├── pet_image_input.h
│   │   ├── pet_image_input_1.cc
│   │   ├── pet_image_input_2.cc
│   │   └── tflite_int8_inference.py
│   └── config/bk7259_ap/defconfig
├── cp/
└── partitions/bk7259/
```

Shared Vela model component:

```
common_components/avdk_nn_module/
├── include/tflm_pet_detection_model.h
└── src/tflm_pet_detection/pet_detect_model_data.cc
```

## 3. Features

### 3.1 Main Features

- Initializes TFLM runtime with `tflite::MicroInterpreter`
- Loads `pet_detection_vela_tflite` from `avdk_nn_module`
- Configures Ethos-U scratch and TFLM tensor arena
- Runs the embedded cat and dog input tensors
- Applies YOLOv8-style post-processing: reverse letterbox mapping, per-class NMS, original-image `x1,y1,x2,y2` boxes
- Logs DWT timing and toggles **GPIO_55** around `Invoke()` for pulse-width measurement

### 3.2 Flow

1. `bk_init()` → `media_service_init()` → `bk_frame_buffer_init()`
2. Create the `tflm_demo` task
3. Allocate scratch, model buffer, and tensor arena
4. Initialize Ethos-U and run `AllocateTensors()`
5. Invoke `cat_200` and `dog_204`, then print detection results

## 4. Build and Run

### 4.1 Build

From the SDK root:

```
make bk7259 PROJECT=tflite_micro/pet_detection
```

### 4.2 Runtime Logs

After flashing, no CLI input is required. The AP task periodically calls `tflm_pet_detection_run_demo()`. Typical success logs:

```
===========>start pet_detection demo
ethosu scratch -> HSRAM ...
model -> MEM_SLAB_UNCODED ...
arena -> MEM_SLAB_UNCODED ...
arena used ... / ... bytes
model load & init (scratch+ethosu+model+arena+AllocateTensors): ...
cat_200 Invoke time: ...
=== cat_200 (500x374) detections=1 ===
dog_204 Invoke time: ...
=== dog_204 (499x375) detections=1 ===
```

## 5. Test Plan

- Function: logs should show detections for the embedded samples without `Invoke failed` or `AllocateTensors failed`.
- PC comparison: compare `class_id` and bbox values with `tflite_int8_inference.py` reference output. Small numerical differences are expected.
- Performance: use `Invoke time` logs or the GPIO_55 high pulse.

### Matching `ap/resource/pet_image_input.h`

This header binds **post-processing thresholds**, **original image sizes**, class names, and C array symbols from `pet_image_input_*.cc`. Update it together with the embedded inputs.

| Item | Notes |
|------|-------|
| `k_conf_threshold` / `k_iou_threshold` | Keep consistent with `--conf`, `--iou`, and metadata in generated `*_output.txt`. |
| `k_cat_orig_w` / `k_cat_orig_h`, `k_dog_orig_w` / `k_dog_orig_h` | Original image size for each embedded sample; used for reverse letterbox mapping. |
| `k_class_names[]` | Display names; order must match model `class_id` and `k_num_classes`. |
| `extern` `*_model_input` / `*_model_input_len` | Must match symbols in `pet_image_input_*.cc`; add or remove calls in `tflm_pet_detection_run_demo()` when sample count changes. |

## 6. Configuration

- `pet_image_input.h`: confidence / IoU thresholds, original image sizes, class names, and input tensor symbols.
- `tflm_pet_detection_model.h`: model symbol declarations, arena size, scratch size, input shape, class count, candidate count, and output channel count.
- `pet_detect_model_data.cc`: Vela-generated `pet_detection_vela_tflite[]` bytes.

When replacing the model:

1. Compile the int8 model with Vela settings that match the target Ethos-U accelerator, memory mode, and optimisation target.
2. Check Vela summary for arena, scratch, and model sizes; update `TFLM_ARENA_SIZE`, `ETHOSU_SCRATCH_SIZE`, and partition/RAM settings if needed.
3. Convert the Vela `.tflite` to a C array and replace `common_components/avdk_nn_module/src/tflm_pet_detection/pet_detect_model_data.cc`, keeping `pet_detection_vela_tflite[]` and `pet_detection_vela_tflite_len` aligned with `tflm_pet_detection_model.h`.
4. If input shape, class count, candidate count, or output channels change, update `k_input_*`, `k_num_classes`, `k_num_candidates`, and `k_out_channels`.
5. Regenerate embedded inputs and compare device logs with PC reference output.

Current default memory placement and sizes:

| Type | Location | Size | Source |
|------|----------|------|--------|
| fast memory / Ethos-U scratch | HSRAM | **250 KB** (`ETHOSU_SCRATCH_SIZE`) | `tflm_pet_detection_model.h`, `g_ethosu0_scratch_src` |
| tensor arena | MEM_SLAB_UNCODED | **3 MB** (`TFLM_ARENA_SIZE`) | `tflm_pet_detection_model.h`, `g_tensor_arena_src` |
| Vela model copy | MEM_SLAB_UNCODED | **2,802,048 bytes** (`pet_detection_vela_tflite_len`) | `pet_detect_model_data.cc`, `g_model_data_src` |

## 7. Notes

1. The demo task runs forever and repeats the embedded samples.
2. Model and arena memory usage is large; keep `partitions/bk7259` and RAM settings aligned with the model.
3. Class display names come from firmware `k_class_names`; use `class_id` as the stable comparison key.
4. GPIO_55 timing is for debugging and can be disabled with `TFLM_INVOKE_TIME_TEST=0`.
