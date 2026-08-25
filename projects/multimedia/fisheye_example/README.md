# Fisheye Calibration Example Project

* [中文](./README_CN.md)

## 1. Project Overview

This project demonstrates **dense fisheye remap table generation** on the Beken platform: given fixed OpenCV fisheye camera intrinsics (`K/D`) and seven TV contour points, it computes `map_x` / `map_y` (`int16` nearest-neighbor indices into the source) for a fixed **1920×1080** input and a configurable output size. A serial CLI measures **`fisheye_calibration()`** time and can hex-dump the full table for offline conversion to a binary map.

* Data structures and API:

  - [`fisheye_calibration.h`](../../../ap/include/modules/fisheye_calibration.h)

### 1.1 Test Environment

   * Hardware (matches this project’s `partitions` / `defconfig`):
      * Core board: **BK7259** family
      * PSRAM (e.g. `CONFIG_ALL_CODE_IN_PSRAM`)
   * Software:
      * Input size fixed in code to **1920×1080** (`DEFAULT_INPUT_WIDTH` / `DEFAULT_INPUT_HEIGHT`)
      * Output layout: first half `map_x`, second half `map_y`, row-major, **2×output_width×output_height** `int16_t` values

.. warning::

    Use reference hardware while learning the demo. If the lens, mounting, or resolution differs from the calibration data, update camera intrinsics, contour points, and input size in source and re-validate.

## 2. Directory Structure

```
fisheye_example/
├── CMakeLists.txt        # Project CMake (adds fisheye component path)
├── Makefile
├── README.md
├── README_CN.md
├── hexlog_to_bin.py      # Convert UART hex dump log to raw .bin
├── ap/
│   ├── CMakeLists.txt
│   ├── ap_main.c
│   ├── fisheye_cli.c     # CLI: fisheye cal …, calls fisheye_calibration()
│   ├── fisheye_cli.h
│   └── config/bk7259_ap/defconfig
├── cp/
└── partitions/bk7259/
```

## 3. Features

### 3.1 Main Features

- Calls **`fisheye_calibration()`** to build dense `map_x` / `map_y`
- Calibration data in `fisheye_cli.c`: **`k_demo_camera_params`** from `my_camera_new.json` and **`k_tv_points_001[]`** from `tv_corners.txt` (`FISHEYE_MAP_POINTS`)
- Buffer from frame-buffer heap: `bk_frame_buffer_malloc` / `bk_frame_buffer_free`
- Logs **`fisheye calibration execute time`**; with **dump=1**, hex-dumps the buffer (slow; throttled for UART) and logs **`fisheye calibration dump execute time`**

### 3.2 Flow

1. `bk_init()` → `media_service_init()` → `bk_auxldo_enable()` → `bk_frame_buffer_init()` → **`fisheye_cli_init()`**
2. A low-priority `fisheye_boot` task starts automatically, waits 2 seconds, then runs the default **320×180 / dump=0** calibration.
3. User can also run **`ap_cmd fisheye cal …`** manually from the CLI.
4. Allocate buffer, run **`fisheye_calibration()`**
5. Print result info; if dump is enabled, print the raw map buffer.
6. Free buffer

## 4. Build and Run

### 4.1 Build

From the SDK root:

```bash
make bk7259 PROJECT=multimedia/fisheye_example
```

### 4.2 Serial / console

- **Logs are printed on the CP side**; connect the UART used for CP log output to see `fisheye` prints and timing.
- The project UART baud rate is **460800**; set your PC terminal to the same rate to avoid garbled or dropped characters.

### 4.3 CLI

Prefix every CLI line with **`ap_cmd`** so the shell forwards the command to the AP:

```
ap_cmd fisheye cal <dump> <width> <height>
```

Common debug commands:

```bash
ap_cmd fisheye cal 0 320 180
ap_cmd fisheye cal 1 320 180
```

.. list-table::
   :header-rows: 1

   * - Argument
     - Meaning
     - Default
   * - `cal`
     - Subcommand: run one calibration / table build
     - Required
   * - `dump`
     - `0` no hex dump, `1` dump full table
     - `0`
   * - `width` / `height`
     - Output resolution (table size)
     - **320×180**

If width/height are missing, zero, or out of range for 1920×1080 input, they fall back to **320×180**. To pass both dimensions, include `dump` in order, e.g. `ap_cmd fisheye cal 0 640 480`.

### 4.4 Expected log (ap_cmd fisheye cal 0)

For **`ap_cmd fisheye cal 0`** (default **320×180** output, **no** hex dump), the **CP log UART** may show something like below. The timestamp, duration, fallback flag, residual, and coverage vary with input data and board performance.

```
$ap0:fisheye:I(437615):fisheye calibration v19, dump: 0, output_width: 320, output_height: 180
fisheye calibration execute time: <duration_ms> ms, fallback: 0, max_error: 1.234, coverage: 0.995469
```

The `dump:` field matches the second CLI argument (`cal 0` → `dump: 0`). The line `fisheye calibration execute time` is printed via `bk_printf_raw` and may appear without the `$ap0:` prefix.

### 4.5 Extract map from UART dump (hexlog_to_bin.py)

Save the serial log from a **`dump=1`** run. The dump lines contain `0xNN` byte tokens; `hexlog_to_bin.py` decodes only those tokens and skips other log text. Convert to a raw binary file whose size matches **2×W×H×2** bytes for `int16` `map_x` then `map_y`:

```bash
python3 hexlog_to_bin.py fisheye-320-180.log fisheye-320-180.bin
```

- First argument: input **`.log`** copied from the UART capture  
- Second argument: output **`.bin`** (raw bytes, same layout as device memory)

From the project directory:

```bash
cd projects/multimedia/fisheye_example
python3 hexlog_to_bin.py fisheye-320-180.log fisheye-320-180.bin
```

## 5. Configuration (fisheye_calibration)

- `camera`: fixed OpenCV fisheye K/D parameters (`k_demo_camera_params` in this demo)  
- `tv_points`: seven TV contour points in this installation (`k_tv_points_001` in this demo)  
- `input_width` / `input_height`: **1920** / **1080**  
- `output_width` / `output_height`: from CLI  
- `output`: **2×output_width×output_height** `int16_t` values  

The demo camera intrinsics are calibrated for **1920×1080** input frames. `DEFAULT_INPUT_WIDTH` / `DEFAULT_INPUT_HEIGHT` must match the camera JSON `calib_dimension` / `orig_dimension`. If the input frame size changes, update the input-size macros, scale or recalibrate the camera matrix (`fx`, `fy`, `cx`, `cy`), and regenerate TV contour points for the new coordinate system.

The output layout remains compatible with previous dump tooling: first `map_x`, then `map_y`, both row-major `int16_t`.

## 6. Notes

1. Calibration data is tied to **1920×1080** input; change sensor, lens, or input resolution → update camera intrinsics, `DEFAULT_INPUT_WIDTH` / `DEFAULT_INPUT_HEIGHT`, and TV points.  
2. **Dump** is for debug or offline extraction only; large resolutions produce large logs.  
3. **Logs are printed on the CP core**; use the CP UART to view output.  
4. Prefix commands with **`ap_cmd`**, e.g. `ap_cmd fisheye cal 0`.
