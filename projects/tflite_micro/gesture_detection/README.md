# TensorFlow Lite Micro Rock-Paper-Scissors Gesture Detection Project

* [中文](./README_CN.md)

## 1. Project Overview

This project demonstrates a camera-based **rock / paper / scissors gesture interaction application** on the Beken platform. It uses `GestureDetectionModel` for TFLM gesture recognition, `app_event` for game state, prompt playback, and result handling, and an LVGL-generated UI for countdown, gesture images, captured frames, and win/loss results.

### 1.1 Test Environment

* Hardware:
  * BK7259 family core board
  * USB camera or MIPI camera; current defconfig enables `CONFIG_USB_CAMERA=y`
  * MIPI display output through LT8912B bridge, 1280×720
  * Audio DAC for prompt playback
* Software:
  * `CONFIG_TFLITE_MICRO=y`
  * `CONFIG_TFLM_GESTURE_DETECTION_V1=y`
  * `CONFIG_LVGL=y`, `CONFIG_AUDIO=y`
  * Current prompt language: `CONFIG_GESTURE_DET_PROMPT_TONE_LANG_ENGLISH=y`

.. warning::

    This is a full interaction demo and depends on camera, display, LVGL, audio, and model resources. To debug only the app flow, use the CLI commands to simulate `app_event` states first.

## 2. Directory Structure

```
gesture_detection/
├── CMakeLists.txt
├── Makefile
├── README.md
├── README_CN.md
├── app.rst
├── ap/
│   ├── CMakeLists.txt
│   ├── Kconfig.projbuild                 # Prompt language selection
│   ├── ap_main.cc                        # Camera/display/UI/model flow
│   ├── app_event.c                       # Game state, prompts, image cache
│   ├── app_event.h
│   ├── app_event_test_cli.c              # rps_* test commands
│   ├── resource/prompt_tone/             # CN/EN prompt C arrays
│   └── ui/beken_generated/               # LVGL UI and image resources
├── cp/
└── partitions/bk7259/
```

Shared model component:

```
common_components/avdk_nn_module/
├── include/GestureDetectionModel.h
├── include/tflm_gesture_detection.h
└── src/tflm_gesture_detection/
    ├── GestureDetectionModel.cc
    └── gesture_detection_model_data.cc
```

## 3. Features

### 3.1 Main Features

- Detects `rock`, `paper`, and `scissors` with `GestureDetectionModel`; returns `GESTURE_NONE` when no valid gesture is detected
- `app_event` manages get-ready, detection window, stop-hand prompt, result playback, and idle transitions
- Prompt resources include `cant_det`, `detecting`, `get_ready`, `rock`, `paper`, `scissors`, `you_win`, and more
- LVGL UI displays game pages, built-in gesture images, and captured detection frames
- Provides `rps_*` CLI commands for testing when buttons or full interaction are unavailable

### 3.2 Flow

1. `bk_init()` → `media_service_init()` → `bk_auxldo_enable()` → `bk_frame_buffer_init()`
2. Configure camera, display, GPU, and device management
3. `app_event_init()` initializes event queue, player, state machine, and CLI
4. Create `GestureDetectionModel` and register gesture/image callbacks
5. Initialize LVGL UI and start display
6. Post `APP_EVENT_PROMPT_GET_READY` automatically for the first round

## 4. Build and Run

### 4.1 Build

From the SDK root:

```
make bk7259 PROJECT=tflite_micro/gesture_detection
```

### 4.2 Runtime Behavior

After flashing, the project automatically enters the first game round. Typical log keywords:

```
AP main running...
rps test CLI registered: rps_start, rps_end, rps_rule, rps_gesture, rps_prompt
======Paper is detected...
======Rock is detected...
======Scissors is detected...
```

### 4.3 CLI

Prefix each CLI line with `ap_cmd` so the shell forwards the command to the AP. Use the serial CLI to trigger or simulate the flow:

```
ap_cmd rps_start
ap_cmd rps_end
ap_cmd rps_rule
ap_cmd rps_gesture rock|paper|scissors
ap_cmd rps_prompt <name>
```

`rps_prompt` names include `cant_det`, `detecting`, `get_ready`, `my_show`, `paper`, `pls_stop_hand`, `rock`, `scissors`, `you_loss`, `you_win`, `your_show`, `draw`, and `game_rule`.

## 5. Test Plan

- UI: LVGL pages render correctly, and gesture/result images switch as expected.
- Camera: USB/MIPI camera path works.
- Audio: run `ap_cmd rps_prompt get_ready` and `ap_cmd rps_prompt rock` to verify prompt playback.
- Model: during the detection window, logs show the matching `Paper/Rock/Scissors is detected` message.
- State machine: use `ap_cmd rps_start`, `ap_cmd rps_end`, and `ap_cmd rps_gesture` to validate transitions.

### Model, UI, and Prompt Resource Updates

.. list-table::
   :header-rows: 1

   * - Item
     - Notes
   * - `gesture_detection_model_data.cc` / `.h`
     - Holds `gesture_detection_tflite[]` and `gesture_detection_tflite_size`; keep symbols unchanged when replacing the model.
   * - `GestureDetectionModel::resourceLoad()`
     - Owns input size, pixel format, model location, fast memory, and arena placement/size; current input is **192×192**.
   * - `GestureDetectionModel::post_process()`
     - Parses score, paper, rock, and scissors outputs before calling `app_event`; update it when output layout or class order changes.
   * - `ap/resource/prompt_tone/`
     - CN/EN prompts are selected by `CONFIG_GESTURE_DET_PROMPT_TONE_LANG_*`; add new prompt sources to CMake and `resource.h`.
   * - `ap/ui/beken_generated/`
     - LVGL generated pages and image resources; re-validate display and touch after changing resolution, images, or page logic.
   * - `app_event.c`
     - Manages game state, prompt sequence, image cache, and result judgment; use `ap_cmd rps_*` commands first when debugging state flow.

Regression should cover prompt playback, UI page switching, camera input, model recognition, `app_event` transitions, captured-image display, and long-run memory stability.

## 6. Configuration

- Camera path: `CONFIG_USB_CAMERA` selects `OpenUVCCameraWithDisplay()`; otherwise the MIPI path is used.
- Prompt language: `GESTURE_DET_PROMPT_TONE_LANG` in `ap/Kconfig.projbuild`.
- UI resources: `ap/ui/beken_generated/`.
- Prompt resources: `ap/resource/prompt_tone/cn` and `ap/resource/prompt_tone/en`.
- Model switch: `CONFIG_TFLM_GESTURE_DETECTION_V1`.

Current default memory placement and sizes (see `GestureDetectionModel::resourceLoad()`):

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
     - **600 KB**
     - `arena_ram_type` / `arena_data_size`
   * - Vela model copy
     - PSRAM_SLAB (`MEM_SLAB_HEAP_CODED`)
     - **823,456 bytes**
     - `model_ram_type` / `gesture_detection_tflite_size`

## 7. Notes

1. Current defconfig uses English prompts. Switch the Kconfig language option for Chinese prompts.
2. `GestureDetectionModel` input is currently **192×192**; review thresholds and output layout when replacing the model.
3. Camera, display, and audio running together put pressure on memory. Check `partitions/bk7259` when porting.
4. CLI commands test the event flow only; real recognition still depends on camera frames and model callbacks.
