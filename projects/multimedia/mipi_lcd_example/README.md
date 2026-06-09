# MIPI LCD Example Project

* [中文](./README_CN.md)

## 1. Project Overview

This project demonstrates MIPI DSI LCD display on the Beken platform. Based on the `bk_display_*` controller APIs, it shows display initialization, the frame flush thread, and runtime pixel-format switching.

The project provides:

- Boot-time ARGB8888 display self-test: `mipi_lcd_display_argb8888_test`
- Runtime pixel-format switch test: `mipi_lcd_switch_format` (CLI / `ap_cmd`)
- Auxiliary debug command: `mipi_lcd open | close | switch ...`
- Integration-test entries in `.it.csv`

### 1.1 Test Environment

   * Hardware configuration:
      * Core board, **BK7259_QF128_12.3X12.3_V4.0**
      * PSRAM 32M
   * Default MIPI panel: `hx8399c_mipi_1080x1920` (resolution 1080x1920)
      * Boot-time format: `ARGB8888` (compressed data)
      * Supported formats: `RGB565` / `RGB888` / `NV12` / `ARGB8888` (compressed)

.. warning::
    Please use reference peripherals for familiarization and learning of the demo project. If peripheral specifications are different, the code may need to be reconfigured.

## 2. Directory Structure

The project adopts an AP-CP dual-core architecture, with the main source code located in the AP directory. The project structure is as follows:

```
mipi_lcd_example/
├── CMakeLists.txt        # Project-level CMake build file
├── Makefile              # Make build file
├── README.md             # Project documentation (English)
├── README_CN.md          # Project documentation (Chinese)
├── app.rst               # Project description
├── .it.csv               # Integration test cases
├── ap/                   # AP-side code
│   ├── CMakeLists.txt    # AP-side CMake build file
│   ├── ap_main.c         # AP entry, CLI registration, boot-time case
│   ├── config/           # AP configuration directory
│   ├── include/
│   │   ├── lcd_example.h # Interface declarations
│   │   └── lcd_image.h   # Built-in ARGB8888 compressed image source
│   └── src/
│       └── lcd_example_mipi.c # MIPI open/close, flush thread, two IT cases
├── cp/                   # CP-side code
├── partitions/           # Partition configuration
└── pj_config.mk          # Project configuration
```

## 3. Feature Description

### 3.1 Main Features

- Runs the ARGB8888 display case automatically at boot
- Provides the `mipi_lcd_switch_format` CLI command to verify runtime pixel-format switching
- Provides the `mipi_lcd` auxiliary command for manual open/close and format switching
- Uses a direct `malloc -> fill -> flush -> delay` flush thread (no queue)
- Each case auto-closes after a fixed duration to keep cases independent
- Provides unified `[RESULT][PASS]` / `[RESULT][FAIL]` logs

### 3.2 Test Cases

#### Case 1: Boot-time ARGB8888 display (`mipi_lcd_display_argb8888_test`)

1. `main()` calls `mipi_lcd_argb8888_test()` automatically at boot
2. Opens the panel in `ARGB8888` (compressed) and starts the flush thread
3. Keeps refreshing for about 20s (`MIPI_LCD_CASE_DURATION_MS`)
4. Auto-closes the display and prints:
   `[RESULT][PASS] mipi_lcd_display_argb8888_test success`

#### Case 2: Runtime pixel-format switch (`mipi_lcd_switch_format`)

1. Triggered by CLI / `ap_cmd mipi_lcd_switch_format`
2. Switches the format at runtime via `bk_display_ioctl`, in order:
   `RGB565 -> RGB888 -> ARGB8888 -> RGB565`, about 5s per step (`MIPI_LCD_SWITCH_STEP_MS`, ~20s total)
3. Auto-closes the display after switching and prints:
   `[RESULT][PASS]mipi_lcd_switch_format success`

> Note: `ARGB8888` is handled as compressed data (`decompress=true`).

## 4. Compilation and Execution

### 4.1 Compilation Method

Compile the project using the following command:

```bash
make bk7259 PROJECT=multimedia/mipi_lcd_example
```

### 4.2 Execution Method

After successful compilation, flash the generated firmware to the development board. `main()` runs Case 1 (ARGB8888 display) automatically at boot. Manual commands can also be issued from the serial terminal:

Command execution success prints: "CMDRSP:OK"
Command execution failure prints: "CMDRSP:ERROR"

#### 4.2.1 Current CLI Commands

```text
mipi_lcd_switch_format
mipi_lcd open
mipi_lcd close
mipi_lcd switch rgb565|rgb888|nv12|argb8888
```

- `mipi_lcd_switch_format` cycles `RGB565 / RGB888 / ARGB8888` to verify the switch API (IT case).
- `mipi_lcd open` / `close` manually opens / closes the panel.
- `mipi_lcd switch <fmt>` manually switches a single format (for debugging).
- `CMDRSP:OK` means the test task was created successfully; check `[RESULT]` logs for the final pass/fail status.

## 5. Test Examples

### 5.1 Boot-time ARGB8888 Display Test

Runs automatically after power-on (or `reboot`). Expected final log:

```text
[RESULT][PASS] mipi_lcd_display_argb8888_test success
```

### 5.2 Pixel-Format Switch Test

```text
ap_cmd mipi_lcd_switch_format
```

Expected final log:

```text
[RESULT][PASS]mipi_lcd_switch_format success
```

### 5.3 Integration Test Commands

`.it.csv` contains:

```text
reboot                           -> [RESULT][PASS] mipi_lcd_display_argb8888_test success
ap_cmd mipi_lcd_switch_format    -> [RESULT][PASS]mipi_lcd_switch_format success
```

## 6. Configuration Options

### 6.1 Display Configuration

The current MIPI test uses the following fixed configuration:

- Default panel: `hx8399c_mipi_1080x1920`
- Resolution: `1080 x 1920`
- Boot-time format: `BK_PIXEL_FORMAT_ARGB8888` (compressed)
- Supported formats: `RGB565` / `RGB888` / `NV12` / `ARGB8888` (compressed)
- Boot display hold time: `MIPI_LCD_CASE_DURATION_MS = 20s`
- Switch step time: `MIPI_LCD_SWITCH_STEP_MS = 5s`

## 7. Notes

1. The two cases are independent; a case auto-closes the display and releases resources when finished, and a later case preempts the previous one.
2. CLI commands only create the test task; the final result is shown by `[RESULT][PASS]` or `[RESULT][FAIL]`.
3. Format switching is done at runtime via `bk_display_ioctl`; the flush thread is stopped first, then restarted after the switch.
4. `ARGB8888` is handled as compressed data; the built-in image source is in `ap/include/lcd_image.h`.
5. Frame buffer resources are limited; avoid occupying too many buffers simultaneously.
