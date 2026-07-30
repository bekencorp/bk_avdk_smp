# Motion Detection Example

* [中文](./README_CN.md)

## 1. Project Overview

This project demonstrates camera-based motion detection on the Beken platform. It reads NV12 frames from the ISP, classifies the scene as `moving` or `still` using gray-frame differencing, and can preview the camera stream on a MIPI panel through the GPU.

The project provides:

- A complete SP-detection and MP-preview mode
- An MP-only detection mode without GPU or display
- `320x180` NV12 gray-image motion detection
- Motion counts, 3x3 region statistics, and processing-time logs
- Hex dump of the next detection frame

### 1.1 Test Environment

- Core board: `BK7259_QF128_12.3X12.3_V4.0`
- PSRAM: 32 MB
- Camera: CSI GC2053, `1280x720@20fps` by default
- Display: ER68576B MIPI DSI panel, `720x1280`
- Detection input: `320x180` NV12; the algorithm uses the Y plane
- Graphics: VG-Lite GPU and DPU/MIPI

> Use the reference peripherals where possible. When changing the sensor, panel, or GPIO assignment, update the board configuration in `ap_main.c` and the project Kconfig.

## 2. Directory Structure

```text
motion_detect/
├── .ci                         # CI build command
├── CMakeLists.txt              # Project CMake configuration
├── Makefile                    # Make build entry
├── README.md                   # English documentation
├── README_CN.md                # Chinese documentation
├── ap/
│   ├── ap_main.c               # AP initialization and board configuration
│   ├── CMakeLists.txt
│   ├── config/                 # AP default configuration
│   ├── include/
│   └── src/motion_detect_demo.c# Motion detection and CLI implementation
├── cp/                         # CP startup and calibration code
└── partitions/                 # Partition and RAM-region configuration
```

## 3. Feature Description

### 3.1 Full `start` Mode

1. Start the MIPI sensor and ISP MP channel
2. Create a `320x180` NV12 SP channel for detection
3. Start the MIPI display, GPU, and ISP-GPU bond for MP preview
4. Read one SP frame per second in the detection task
5. Save the first frame as the reference and compare subsequent gray frames
6. Update the reference after motion is detected and print region statistics

### 3.2 `start_mp` MP-only Mode

This mode temporarily configures ISP MP as `320x180` NV12 and reads MP frames directly. It does not start SP, GPU, or the display. The original camera configuration is restored by `stop`.

### 3.3 Default Algorithm Parameters

- Image size: `320x180`
- Per-pixel difference threshold: `64`
- Motion-count threshold: `128`
- Processing period: approximately one frame per second
- Reference policy: use the first frame, then update it after motion

In the result log, `total` is the motion count. `max_3x3`, `rows`, and `cols` describe the distribution of motion.

## 4. Build and Run

### 4.1 Build

Run from the SDK root:

```bash
make bk7259 PROJECT=multimedia/motion_detect
```

### 4.2 CLI Commands

After flashing and booting the firmware, use the serial console:

```text
ap_cmd motion_detect help
ap_cmd motion_detect start
ap_cmd motion_detect start_mp
ap_cmd motion_detect stop
ap_cmd motion_detect dump
```

- `start`: starts SP detection with MP display preview
- `start_mp`: starts MP-only detection without display
- `stop`: stops the task and releases camera, GPU, display, and frame-buffer resources
- `dump`: prints the next detection frame in hexadecimal
- `CMDRSP:OK` indicates success; `CMDRSP:ERROR` indicates invalid arguments or a resource startup failure

## 5. Test Examples

### 5.1 Motion Detection with Preview

```text
ap_cmd motion_detect start
```

Keep the scene still, then move an object in front of the camera. Expected logs include:

```text
motion_detect frame=2 state=still total=... max_3x3[...]=... rows=... cols=...
motion_detect frame=3 state=moving total=... max_3x3[...]=... rows=... cols=...
```

Stop the test with:

```text
ap_cmd motion_detect stop
```

### 5.2 MP-only Detection

```text
ap_cmd motion_detect start_mp
ap_cmd motion_detect stop
```

### 5.3 Dump Detection Input

Start either mode, then run:

```text
ap_cmd motion_detect dump
```

The next `320x180` Y plane is printed. This is a large transfer and affects real-time behavior.

## 6. Configuration

The default configuration enables ISP, MIPI CSI, frame buffers, VG-Lite GPU, DPU, MIPI DSI, GC2053, ER68576B, and media service. Main board parameters in `ap/ap_main.c` include:

- Sensor GPIOs, I2C, resolution, and frame rate
- ISP MP size and format
- MIPI panel type and GPIOs
- GPU formats and 90-degree rotation

Algorithm dimensions and thresholds are defined by the `MOTION_DETECT_*` macros in `ap/src/motion_detect_demo.c`.

## 7. Notes

1. `start` and `start_mp` cannot run together; repeated starts return busy.
2. `start` occupies camera, ISP, GPU, DPU, display, and frame-buffer resources. Run `stop` before switching modes.
3. Detection uses only the NV12 Y plane. If the size changes, update both ISP output and algorithm configuration.
4. Lighting flicker, auto-exposure changes, and camera shake may be classified as motion; tune thresholds for the target environment.
5. `dump` prints 57,600 bytes and is intended only for debugging.
