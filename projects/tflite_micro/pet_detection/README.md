# TensorFlow Lite Micro Pet Detection (Cat/Dog YOLOv8) Project

* [中文](./README_CN.md)

## 1. Project Overview

This project demonstrates an end-to-end on-device **cat/dog detection** flow on the Beken platform with **TensorFlow Lite Micro (TFLM)** and **Ethos-U NPU**. The flow includes YOLOv8 int8 quantized inference and post-processing. The model weights are compiled by **Vela** and stored in the shared `avdk_nn_module` component. The application runs offline sample inference with embedded preprocessed input tensors.

Main work covered by this project:

- Initialize the TFLM runtime (`tflite::MicroInterpreter`)
- Load the Vela-generated `pet_detection_vela_tflite` data from `avdk_nn_module`
- Configure and pass the **scratch** buffer for Ethos-U
- Configure the TFLM **tensor arena** (size defined in `tflm_pet_detection_model.h`)
- Run inference on two embedded inputs, which are letterbox+int8 tensors from the same source as the PC-side `tflite_int8_inference.py`, then print detection boxes, classes, scores, and support **DWT timing** plus **GPIO_55 pulse-width** measurement for the `Invoke()` interval

> Required Kconfig options: `TFLITE_MICRO` and `CONFIG_TFLM_PET_DETECTION_V1`. Otherwise `pet_detect_model_data.cc` is not compiled and linking will miss the `pet_detection_vela_tflite` symbol.

## 2. Directory Structure

The project uses an AP/CP dual-core layout. **Inference and logging run on the AP side**. The directory style follows `tflite_micro_example`:

```
pet_detection/
├── .ci                             # CI configuration
├── CMakeLists.txt                  # Project CMake, including EXTRA_COMPONENTS_DIRS such as avdk_nn_module
├── Makefile                        # Make wrapper
├── ap/
|   ├── config/bk7259_ap/           # AP board/default Kconfig
|   |   └── defconfig
|   ├── ap_main.c                   # AP entry: media/cache initialization, creates tflm_demo task
|   ├── CMakeLists.txt              # AP component: ap_main, demo, resource/*.cc
|   ├── app/                        # Pet detection demo
|   |   ├── tflm_pet_detection_demo.h
|   |   └── tflm_pet_detection_demo.cpp   # TFLM + YOLOv8 post-processing, DWT, GPIO55
|   └── resource/
|       ├── pet_image_input.h              # Thresholds, original image sizes, extern tensor names
|       ├── pet_image_input_1.cc           # Embedded tensor 1, for example cat_200 input
|       ├── pet_image_input_2.cc           # Embedded tensor 2, for example dog_204 input
|       └── tflite_int8_inference.py        # PC: generates *_model_input.cc and *_output.txt
├── cp/
|   ├── config/bk7259/defconfig     # CP default configuration
|   ├── CMakeLists.txt
|   └── cp_main.c                   # CP entry, not directly bound to AP inference
└── partitions/bk7259/
    ├── ram_regions.csv             # RAM regions
    └── auto_partitions.csv         # Auto partitions
```

**Vela model and header in the shared component**. This project pulls them through `avdk_nn_module`:

```
common_components/avdk_nn_module/
├── Kconfig                                 # CONFIG_TFLM_PET_DETECTION_V1
├── CMakeLists.txt                          # Conditionally compiles pet_detect_model_data.cc
├── include/tflm_pet_detection_model.h      # Tensor shape, TFLM_ARENA_SIZE, Vela symbol declarations, etc.
└── src/tflm_pet_detection/
    └── pet_detect_model_data.cc            # pet_detection_vela_tflite[] + _len, large generated file
```

## 3. Features

### 3.1 Main Features

1. **On-device inference and post-processing**

    - Runs the int8 detection model with TFLM + Ethos-U.
    - Uses YOLOv8-style post-processing: reverse letterbox mapping, per-class NMS, and outputs original-image `x1,y1,x2,y2` plus `class_id` (`k_class_names`: cat / dog).

2. **Input**

    - The current demo does not read from a camera. It only uses the **fixed model input tensors** in `pet_image_input_*.cc`, which match the buffer passed to the Interpreter in the Python generation script.

3. **Output information**

    - First initialization prints scratch / model / arena source, pointer, and size.
    - Each sample image prints the number of `detections` and each bbox, score, and class.

### 3.2 Inference Time and GPIO (tflm_pet_detection_demo.cpp)

- **DWT**: `pet_log_dwt_interval` prints timing after "model load+init+AllocateTensors" and after each `Invoke`; it converts cycle differences to `us` (<1 ms) or `ms` (>=1 ms) using **480 MHz**, consistent with the person example in `tflite_micro_example`. If the chip clock is not 480 MHz, adjust the conversion.
- **GPIO_55**: pulled high before each `Invoke()` and pulled low after it finishes. A logic analyzer can measure the high-level width as single-inference latency.

### 3.3 Memory Sources (same idea as the TFLite example)

- `tflm_mem_src_t` in `tflm_pet_detection_demo.cpp` selects memory sources such as `HSRAM`, `PSRAM`, and `MEM_SLAB_UNCODED` for **Ethos scratch**, **model copy buffer**, and **tensor arena**. Use the current source enum and allocation path as the reference.
- The model and arena allocation path requires **16-byte alignment**, consistent with Ethos command stream constraints. The sample aligns allocated addresses.

## 4. Build and Run

### 4.1 Build

From the SDK root, run the same command as `.ci`:

```
make bk7259 PROJECT=tflite_micro/pet_detection
```

### 4.2 Runtime and Serial Logs

After flashing, the AP task with TAG `tflite_demo` periodically calls `tflm_pet_detection_run_demo()`. Typical success logs are shown below. Keywords may differ slightly by version:

```
ap1:tflite_d:I(1):===========>start pet_detection demo
ap1:pet_tflm:I(2):ethosu scratch -> HSRAM 0x28100020 size=256000
ap1:ETHOSU:W(3):W: Initializing NPU: base_address=0x48200000, fast_memory=0x28100020, fast_memory_size=256000, secure=1, privileged=1

ap1:ethosu:I(3):Ethos-U driver initialized successfully, ret=0
ap1:pet_tflm:I(74):model -> MEM_SLAB_UNCODED 0x60d53e40 size=2802048
ap1:pet_tflm:I(74):arena -> MEM_SLAB_UNCODED 0x60a53dc0 size=3145728
ap1:pet_tflm:I(74):arena used 2734676 / 3145728 bytes
ap1:pet_tflm:I(74):model load & init (scratch+ethosu+model+arena+AllocateTensors): 72 ms
ap1:pet_tflm:I(178):cat_200 Invoke time: 95 ms
ap1:pet_tflm:I(181):=== cat_200 (500x374) detections=1 ===
ap1:pet_tflm:I(181):  [0] class_id=0 (class0) score=0.8338 bbox_orig: x1=138.55 y1=9.84 x2=347.00 y2=272.24
ap1:pet_tflm:I(284):dog_204 Invoke time: 95 ms
ap1:pet_tflm:I(286):=== dog_204 (499x375) detections=1 ===
ap1:pet_tflm:I(286):  [0] class_id=1 (class1) score=0.9515 bbox_orig: x1=133.38 y1=46.53 x2=419.72 y2=342.67
```

- **DWT/Invoke line**: `{tag} Invoke time: ...` corresponds to one inference.
- **Detection line**: `=== <tag> (WxH) detections=n ===` followed by several `[i] class_id=...` lines. When `n==0`, `(none above conf=...)` may be printed.
- **Errors**: for example `alloc ethosu scratch failed`, `bk_ethosu_init failed`, `AllocateTensors failed`, `Invoke failed`, and shape mismatch errors. Check Kconfig, model version, and arena/scratch sizes.

## 5. Test Plan

### Matching ap/resource/pet_image_input.h

This header binds **post-processing parameters**, **original image sizes for each embedded sample**, **C symbol names**, and `pet_image_input_*.cc`. When changing images, script parameters, or sample count, update this file and the calls in `tflm_pet_detection_demo.cpp` together.

.. list-table::
   :header-rows: 1

   * - Item
     - Notes
   * - `k_conf_threshold` / `k_iou_threshold`
     - Keep them consistent with `--conf`, `--iou`, and `# conf_threshold` / `# iou_threshold` in `*_output.txt`; otherwise the PC reference and device post-processing thresholds differ.
   * - `k_cat_orig_w` / `k_cat_orig_h`, `k_dog_orig_w` / `k_dog_orig_h`
     - Original width and height in pixels for each embedded input. Must match `# orig_image_size` and `letterbox_params` in `*_output.txt`; update them to the new original image size when changing samples.
   * - `k_class_names[]`
     - Detection class display names. Depends on `k_num_classes` in `tflm_pet_detection_model.h`, and order must match model `class_id` (currently cat / dog). Update together with the model header when class count changes.
   * - `extern` `*_model_input` / `*_model_input_len`
     - Must exactly match the array names and length symbols in `pet_image_input_*.cc`; when adding/removing `.cc` files or renaming symbols, update declarations here and pass matching tags/pointers in `tflm_pet_detection_run_demo()` / `run_one_embedded_image()`.

This file already includes `tflm_pet_detection_model.h`, so **input resolution and class count** are determined by the model header. If only test images change and the model does not, usually only thresholds, original image sizes, and `extern` symbols need updates.

### Updating the Model with Vela and Integrating into Firmware

**Prerequisite**: install the `vela` command line tool that matches the Ethos-U toolchain. Put `vela.ini` and the int8 model in the same work directory, or pass the real path with `--config`. The input model must match the training/quantization artifact, for example `yolov8n_full_integer_quant.tflite`.

#### Step 1: Compile the Model with Vela

Run the following in the directory containing `vela.ini`. Parameters must match the **on-chip** Ethos accelerator, **PSRAM/SRAM**, and optimization target. Tune them based on the actual hardware environment:

```bash
vela yolov8n_full_integer_quant.tflite \
  --output-dir ./vela_out \
  --accelerator-config ethos-u65-256 \
  --config vela.ini \
  --system-config Ethos_U65_PSRAM \
  --memory-mode Dedicated_Sram_256KB \
  --optimise Performance
```

- Check the generated `.tflite`, reports, and possible `.cc` files under `--output-dir`, depending on the tool version. Read the **summary CSV** in the same directory for recommended **arena, scratch, and weight sizes**, then verify that `TFLM_ARENA_SIZE`, `ETHOSU_SCRATCH_SIZE`, and partitions still have enough space.

#### Matching common_components/avdk_nn_module/include/tflm_pet_detection_model.h

After updating the Vela model or changing the network structure, check this header according to the **new Flatbuffer and Vela report**, in addition to replacing `pet_detect_model_data.cc`. This avoids `AllocateTensors` failures and shape-check errors, because `tflm_pet_detection_demo.cpp` asserts input/output dimensions.

.. list-table::
   :header-rows: 1

   * - Item
     - Notes
   * - `pet_detection_vela_tflite` / `pet_detection_vela_tflite_len`
     - Must match the array name and length constant in `pet_detect_model_data.cc`. Usually only the `.cc` implementation changes while the header keeps `extern`.
   * - `TFLM_ARENA_SIZE`
     - TFLM **tensor arena** size in bytes. It should be **>=** the Vela summary / runtime `arena_used` log, with margin; too small causes `AllocateTensors failed`.
   * - `ETHOSU_SCRATCH_SIZE`
     - Ethos-U **scratch** size. Must match the requirement for this network from Vela and the length passed to `bk_ethosu_init`.
   * - `k_input_h` / `k_input_w` / `k_input_c`
     - Model **input tensor** height, width, and channels; must match the letterboxed tensor shape in `pet_image_input` (currently 320×320×3).
   * - `k_num_classes`
     - Number of detection classes; must match the class part of the model output channels (currently 2: cat/dog).
   * - `k_num_candidates`
     - Number of flattened feature-map candidate boxes, such as 2100; must match the last dimension of TFLite output `out->dims`.
   * - `k_out_channels`
     - Defined as `4 + k_num_classes` (cx, cy, w, h + per-class score), and must match the YOLOv8 head output layout. It changes with class count.

Current default memory placement and sizes:

.. list-table::
   :header-rows: 1

   * - Type
     - Location
     - Size
     - Source
   * - fast memory / Ethos-U scratch
     - HSRAM
     - **250 KB** (`ETHOSU_SCRATCH_SIZE`)
     - `tflm_pet_detection_model.h`, `g_ethosu0_scratch_src`
   * - tensor arena
     - MEM_SLAB_UNCODED
     - **3 MB** (`TFLM_ARENA_SIZE`)
     - `tflm_pet_detection_model.h`, `g_tensor_arena_src`
   * - Vela model copy
     - MEM_SLAB_UNCODED
     - **2,802,048 bytes** (`pet_detection_vela_tflite_len`)
     - `pet_detect_model_data.cc`, `g_model_data_src`

If you only rerun Vela and the network structure does not change, usually only **arena/scratch** and `pet_detection_vela_tflite_len` need checking. If input/output shapes or class count change, update this header, regenerate embedded inputs with `tflite_int8_inference.py`, and check `k_class_names` plus `extern` samples in `pet_image_input.h`.

#### Step 2: Convert the Binary to a C Source File

If Vela only outputs a Flatbuffer binary, or you need to convert a `*.tflite` into a C array matching the existing `pet_detect_model_data.cc`, use `xxd` to generate a raw unsigned array and then **manually** rename it to the project symbols:

```bash
xxd -i -c 12 xxxx.tflite > xxxx.cc
```

- `-c 12`: 12 bytes per line
- `xxd -i` generates `unsigned char filename_parts[]` and a length macro/constant based on the **file name**. You **must** rename them to match the header and add the alignment/linkage attributes used by this project, for example:
  - Symbols: `pet_detection_vela_tflite[]`, `pet_detection_vela_tflite_len` (matching the `extern` declarations in `tflm_pet_detection_model.h`)
  - Recommended: `#include "avdk_nn_module.h"`, use `extern "C"` and `DATA_ALIGN_ATTRIBUTE` for the array, as shown near the top of the existing `pet_detect_model_data.cc`
  - The normal Ethos path should run **Vela first**, then run `xxd` on the Vela output or directly use the C file exported by Vela

#### Step 3: Replace the Model Source File in the Repository

1. Replace the whole file `common_components/avdk_nn_module/src/tflm_pet_detection/pet_detect_model_data.cc` with the array prepared in Step 2.
2. Confirm that `pet_detection_vela_tflite_len` matches the array byte count.
3. Update the macros and `constexpr` values in `tflm_pet_detection_model.h` according to the previous section, so they match the **new** Vela artifact and TFLite tensor shapes.
4. Keep `CONFIG_TFLM_PET_DETECTION_V1=y` in Kconfig.
5. Rebuild the full `pet_detection` project and run it. Compare serial logs with the PC-side `*_output.txt` generated by `tflite_int8_inference.py`. If inputs or classes changed, also update the section "Matching `pet_image_input.h`".

### Function and Regression Suggestions

- Function: serial logs should stably print `detections` and bbox for both samples, without `Invoke failed` / `AllocateTensors failed`.
- PC comparison: for the same image, `class_id` and bbox from `*_output.txt` should be numerically close to device output. Small differences from quantization or implementation are expected.
- Performance: observe `Invoke time` or the GPIO_55 high-level pulse width.

## 6. Notes

1. The inference task is called in a **dead loop** from `ap_main` and does not exit by itself.
2. The Vela model and tensor arena are large; **partition and RAM configuration** (`partitions/bk7259`) must meet runtime requirements.
3. When updating the model, update the Vela artifact, header constants, embedded `model_input`, and Python reference `*_output.txt` metadata together.
4. Class display names come from firmware `k_class_names`; the PC `output.txt` may use strings such as `class0`, so use `class_id` as the comparison key.
5. For more general TFLM usage, refer to `tflite_micro/tflite_micro_example/README_CN_.md`.

## 7. Reference Project

.. list-table::
   :header-rows: 1

   * - Description
     - Path
   * - General TFLM + Vela example (person detection)
     - `tflite_micro/tflite_micro_example/`
