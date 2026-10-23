# TensorFlow Lite Micro Example Project

* [中文](./README_CN.md)

## 1. Project Overview
This project is a combined TensorFlow Lite Micro (TFLM) and Ethos-U NPU example used to verify that the on-device inference pipeline works properly on the Beken platform.

The example provides two build-time selectable modes (switched by `TFLM_MULTI_INTERPRETER_DEMO` in `ap/ap_main.c`):
- **Dual-interpreter parallel mode (default, `=1`)**: two different models run concurrently in two core-pinned tasks, simulating the real "one NPU serving multiple AI workloads" product scenario:
  - `tflm_person` task on AP core0 (CPU2): person_detection model (96x96x1 grayscale, lightweight classification)
  - `tflm_pet` task on AP core1 (CPU3): pet_detection model (YOLOv8n, 320x320x3, detection with NMS post-processing)
- **Single-interpreter mode (`=0`)**: the original person_detection demo, kept as the performance baseline and fallback path (files untouched).

Both modes share the same NPU runtime: the Ethos-U65 driver's reserve/release mechanism serializes NPU access between the two tasks, while CPU-side work (input preparation, post-processing) runs in parallel on the two AP cores.

## 2. Directory Structure
The project uses an AP/CP dual-core architecture, with the main logic running on the AP side. The directory structure is as follows:
```
tflite_micro_example/
├── .ci                             # CI configuration directory
├── CMakeLists.txt                  # Project-level CMake build file
├── Makefile                        # Make build file
├── ap/
|   ├── config/
|   |   └── defconfig               # Default AP configuration
│   ├── ap_main.c                   # AP entry point: TFLM_MULTI_INTERPRETER_DEMO switches dual/single mode
│   ├── CMakeLists.txt              # AP CMake build file
│   ├── person_detection/           # person_detection model example (single-interpreter baseline, unchanged)
│   │   ├── tflm_person_detection_demo.cpp           # Main inference logic (TFLM + Ethos-U + timing/memory strategy)
│   │   ├── tflm_person_detection_image_provider.cc  # Input image provider
│   │   └── tflm_person_detection_model_vela_data.cc # Model data (Vela output)
│   └── multi_model/                # Dual-interpreter parallel demo (person + pet)
│       ├── tflm_multi_model_demo.h/.cpp              # Core: model descriptor table, per-instance contexts,
│       │                                              # single-threaded init phase, core-pinned inference tasks
│       ├── tflm_pet_postprocess.h/.cpp               # Pet YOLOv8 post-process (letterbox + dequant + per-class NMS)
│       ├── pet_detect_model_data.cc                  # Pet model data (self-contained copy from avdk_nn_module)
│       └── pet_image_input_1.cc / pet_image_input_2.cc # Embedded cat/dog int8 input tensors
├── cp/
|   ├── config/
|   |   └── defconfig               # Default CP configuration
│   ├── CMakeLists.txt              # CP CMake build file
│   └── cp_main.c                   # CP entry point
└── partitions/                     # Partition / RAM configuration directory
    └── bk7259/
        ├── ram_regions.csv         # RAM region configuration
        └── auto_partitions.csv     # Auto partition configuration
```

## 3. Functional Description

### 3.1 Dual-Interpreter Parallel Demo (default mode)
`ap/multi_model/tflm_multi_model_demo.cpp` implements the dual-interpreter demo:

**Initialization (single-threaded phase, in `main` before task creation)**
1. Allocate the shared Ethos-U fast scratch from HSRAM (250KB, one copy for both models, driver-mutexed in use)
2. Call `bk_ethosu_init()` exactly once (idempotent-guarded inside the component)
3. For each of the two models: copy the model data (16-byte aligned), allocate the tensor arena, construct the op resolver and interpreter, and run `AllocateTensors()`

**Runtime**
- Each interpreter has fully independent resources (model copy / arena / resolver / interpreter / post-process buffers); nothing writable is shared between the two instances
- The two tasks are core-pinned (`rtos_core0/1_create_psram_thread`): `tflm_person` on core0 with a 100ms period, `tflm_pet` on core1 with a 500ms period
- NPU access is serialized by the Ethos-U driver's reserve/release mechanism; the completion interrupt is routed to CPU2 and wakes the waiting task cross-core
- DWT cycle counters measure `Invoke()` on each core independently

**Memory layout**

| Resource | Region | Size | Owner |
|---|---|---|---|
| person model + arena | PSRAM heap | 262KB + 100KB | T0 only |
| pet model + arena | MEM_SLAB_UNCODED | 2.68MB + 3MB | T1 only |
| pet post-process buffers | PSRAM heap | ~145KB | T1 only |
| NPU fast scratch | HSRAM | 250KB (max of both models) | shared, driver-mutexed |
| task stacks | PSRAM | 8KB x 2 | per task |

**Expected serial log**
```
===========>start multi-interpreter (person + pet) demo init
ethosu scratch -> HSRAM 0x2c100020 size=256000
[T0:person] ready: model 0x64e60050 (267104 B, PSRAM) arena 0x64ea13e8 (102400 B, used 74924, PSRAM)
[T1:pet] ready: model 0x60d52d80 (2806272 B, MEM_SLAB_UNCODED) arena 0x60a52d00 (3145728 B, used 2734676, MEM_SLAB_UNCODED)
multi-interpreter init OK (shared NPU, 2 interpreters)
dual-interpreter tasks started (core0: person, core1: pet)
[T1:pet] started on core 1 (period 500 ms)
[T0:person] started on core 0 (period 100 ms)
[T0:person] Invoke time: 6 ms
[T0:person] input=person person score:76%
[T1:pet] Invoke time: 170 ms
[T1:pet] input=cat_200 (500x374) detections=1
  [0] class_id=0 (cat) score=0.8338 bbox_orig: x1=138.55 y1=9.84 x2=347.00 y2=272.24
```
The person task alternates between the person image (score ~76%) and the no-person image (score ~28%); the pet task alternates between the embedded cat and dog inputs.

**Expected parallel behavior**
- While the pet model occupies the NPU (~170ms per inference), the person task's `Invoke()` rises from ~6ms to ~157ms (NPU queuing)
- The pet NMS post-processing (CPU, core1) overlaps in time with person inference (core0)
- Total NPU throughput stays within the single-NPU limit: the two workloads share, not multiply, the NPU capacity
- Results are stable and identical to each model's standalone baseline (no cross-interference)

### 3.2 Single-Interpreter person_detection Demo (fallback mode)
Setting `TFLM_MULTI_INTERPRETER_DEMO` to `0` in `ap/ap_main.c` restores the original demo:
- Initialize the TFLM runtime (`tflite::MicroInterpreter`) and load the person_detection model (Vela output)
- Configure the Ethos-U scratch buffer and the TFLM tensor arena
- Acquire input images, run inference, and output the person / no person scores

### 3.3 Automatic Inference Time Measurement (DWT)
Both demos measure the `Invoke()` duration with the DWT cycle counter and convert it at a fixed 480MHz CPU frequency. The print uses `us` below 1ms and `ms` above.
- The single-interpreter demo gates this with `CONFIG_PERSON_DETECTION_INFER_TIME_AUTO` (must be defined at build time)
- The dual-interpreter demo always prints the per-round time with the `[T0:person]` / `[T1:pet]` prefix; since the tasks are core-pinned, each task uses its own core's DWT counter

### 3.4 Memory / Performance Strategy (HSRAM / PSRAM / SRAM)
In the single-interpreter demo, `PERSON_DETECTION_PERF_TEST` in `tflm_person_detection_demo.cpp` controls the memory strategy:
- `PERSON_DETECTION_PERF_TEST=1` (performance mode): scratch / arena / model data prefer HSRAM, falling back to SRAM when insufficient
- `PERSON_DETECTION_PERF_TEST=0`: scratch uses HSRAM, arena / model data use PSRAM

The dual-interpreter demo uses a fixed policy (see the table in 3.1): person -> PSRAM heap, pet -> MEM_SLAB_UNCODED, shared scratch -> HSRAM.

Both demos enforce 16-byte alignment of model data (Ethos-U command stream requirement).

### 3.5 GPIO-Based Inference Time Observation (Logic Analyzer)
If `PERSON_DETECTION_DEBUG` is defined in `tflm_person_detection_demo.cpp` (single-interpreter demo), specified GPIOs are toggled during different phases of inference:
- `GPIO_34`: toggled at the start / end of inference (measures the `Invoke()` duration)
- `GPIO_32` / `GPIO_33` / `GPIO_35`: distinguish image acquisition, inference, and result processing phases

## 4. Build and Run
### 4.1 Build
Use the following command to build the project:
```
make bk7259 PROJECT=tflite_micro/tflite_micro_example
```

### 4.2 Run
After building and flashing the firmware to the development board:
1. (Default) The dual-interpreter demo initializes both models in the single-threaded phase, then starts `tflm_person` on core0 and `tflm_pet` on core1
2. The person task runs every 100ms and prints the person score for alternating person / no-person images; the pet task runs every 500ms and prints cat / dog detections with bounding boxes
3. Observe the `Invoke time` logs of both tasks and the NPU queuing behavior described in 3.1

To switch back to the single-interpreter demo, set `TFLM_MULTI_INTERPRETER_DEMO` to `0` in `ap/ap_main.c` and rebuild.

## 5. Test Plan
1. Functional verification: both tasks print stable results matching their standalone baselines (person ~76% / ~28%, cat score ~0.83, dog score ~0.95), with no `Invoke failed` / `AllocateTensors failed` / allocation failure logs
2. Concurrency verification: during pet inference windows, the person `Invoke time` rises (NPU queuing) while both tasks keep producing results; long-run (>=10min) stability with no hardfault / deadlock / result drift
3. Performance verification (optional): collect typical `Invoke time` ranges of both tasks; compare the person task time inside vs. outside pet inference windows to quantify NPU sharing overhead
4. Memory verification (optional): check the init-phase memory layout log (addresses and regions) against the table in 3.1; in single-interpreter mode, `PERSON_DETECTION_PERF_TEST` strategies can still be verified

## 6. Notes
1. The inference tasks run continuously and do not exit automatically
2. Ethos-U scratch, tensor arenas, and (in dual mode) the large pet model consume substantial memory; ensure the corresponding regions are available. The dual-interpreter demo raises AP flash usage to ~96% of the 8M partition (pet model 2.68MB is the main contributor); a larger flash variant is required if further content is added
3. The 16-byte model data alignment requirement must not be broken; alignment handling is included in both demos
4. DWT cycle conversion uses a fixed `480MHz`; if the actual platform frequency is different, the converted time may be inaccurate
5. The two models are both fully fused by Vela into a single `ethos-u` custom op: each `Invoke()` triggers exactly one NPU command-stream execution, and NPU contention between tasks happens at whole-inference granularity (reserve/release)
