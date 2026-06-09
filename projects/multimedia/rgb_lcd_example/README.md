# RGB LCD Example Project

* [中文](./README_CN.md)

## 1. Project Overview

This project demonstrates RGB-interface LCD display on the Beken platform. Based on the `bk_display_*` controller APIs, it shows display initialization, the frame flush thread, and runtime pixel-format switching.

The project provides:

- Boot-time RGB565 display self-test: `rgb_lcd_display_rgb565_test`
- Runtime pixel-format switch test: `rgb_lcd_switch_format` (CLI / `ap_cmd`)
- Integration-test entries in `.it.csv`

### 1.1 Test Environment

   * Hardware configuration:
      * Core board, **BK7259_QF128_12.3X12.3_V4.0**
      * PSRAM 32M
   * Default RGB panel: `st7701sn_rgb_480x854` (resolution 480x854)
      * Boot-time format: `RGB565`
      * Supported formats: `RGB565` and `ARGB8888` (compressed data)

.. warning::
    Please use reference peripherals for familiarization and learning of the demo project. If peripheral specifications are different, the code may need to be reconfigured.

## 2. Directory Structure

The project adopts an AP-CP dual-core architecture, with the main source code located in the AP directory. The project structure is as follows:

```
rgb_lcd_example/
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
│   │   └── lcd_example.h # Interface declarations
│   └── src/
│       └── lcd_example_rgb.c # RGB open/close, flush thread, two IT cases
├── cp/                   # CP-side code
├── partitions/           # Partition configuration
└── pj_config.mk          # Project configuration
```

## 3. Feature Description

### 3.1 Main Features

- Runs the RGB565 display case automatically at boot
- Provides the `rgb_lcd_switch_format` CLI command to verify runtime pixel-format switching
- Uses a direct `malloc -> fill -> flush -> delay` flush thread (no queue)
- Each case auto-closes after a fixed duration to keep cases independent
- Provides unified `[RESULT][PASS]` / `[RESULT][FAIL]` logs

### 3.2 Test Cases

#### Case 1: Boot-time RGB565 display (`rgb_lcd_display_rgb565_test`)

1. `main()` calls `rgb_lcd_rgb565_test()` automatically at boot
2. Opens the panel in `RGB565` and starts the flush thread
3. Keeps refreshing for about 20s (`RGB_LCD_BOOT_HOLD_MS`)
4. Auto-closes the display and prints:
   `[RESULT][PASS] rgb_lcd_display_rgb565_test success`

#### Case 2: Runtime pixel-format switch (`rgb_lcd_switch_format`)

1. Triggered by CLI / `ap_cmd rgb_lcd_switch_format`
2. Switches the format at runtime via `bk_display_ioctl`, in order:
   `RGB565 -> ARGB8888 -> RGB565`, about 5s per step (`RGB_LCD_SWITCH_STEP_MS`, ~15s total)
3. Auto-closes the display after switching and prints:
   `[RESULT][PASS]rgb_lcd_switch_format success`

> Note: The RGB panel supports only `RGB565` and `ARGB8888`. `ARGB8888` is handled as compressed data (`decompress=true`).

## 4. Compilation and Execution

### 4.1 Compilation Method

Compile the project using the following command:

```bash
make bk7259 PROJECT=multimedia/rgb_lcd_example
```

### 4.2 Execution Method

After successful compilation, flash the generated firmware to the development board. `main()` runs Case 1 (RGB565 display) automatically at boot. The switch test can also be started from the serial terminal:

Command execution success prints: "CMDRSP:OK"
Command execution failure prints: "CMDRSP:ERROR"

#### 4.2.1 Current CLI Commands

```text
rgb_lcd_switch_format
```

- `rgb_lcd_switch_format` switches `RGB565 / ARGB8888` at runtime to verify the switch API.
- `CMDRSP:OK` means the test task was created successfully; check `[RESULT]` logs for the final pass/fail status.

## 5. Test Examples

### 5.1 Boot-time RGB565 Display Test

Runs automatically after power-on (or `reboot`). Expected final log:

```text
[RESULT][PASS] rgb_lcd_display_rgb565_test success
```

### 5.2 Pixel-Format Switch Test

```text
ap_cmd rgb_lcd_switch_format
```

Expected final log:

```text
[RESULT][PASS]rgb_lcd_switch_format success
```

### 5.3 Integration Test Commands

`.it.csv` contains:

```text
reboot                          -> [RESULT][PASS] rgb_lcd_display_rgb565_test success
ap_cmd rgb_lcd_switch_format    -> [RESULT][PASS]rgb_lcd_switch_format success
```

## 6. Configuration Options

### 6.1 Display Configuration

The current RGB test uses the following fixed configuration:

- Default panel: `st7701sn_rgb_480x854`
- Resolution: `480 x 854`
- Boot-time format: `BK_PIXEL_FORMAT_RGB565`
- Supported formats: `BK_PIXEL_FORMAT_RGB565`, `BK_PIXEL_FORMAT_ARGB8888` (compressed)
- Boot display hold time: `RGB_LCD_BOOT_HOLD_MS = 20s`
- Switch step time: `RGB_LCD_SWITCH_STEP_MS = 5s`

## 7. Notes

1. The two cases are independent; a case auto-closes the display and releases resources when finished, and a later case preempts the previous one.
2. CLI commands only create the test task; the final result is shown by `[RESULT][PASS]` or `[RESULT][FAIL]`.
3. Format switching is done at runtime via `bk_display_ioctl`; the flush thread is stopped first, then restarted after the switch.
4. `ARGB8888` on the RGB panel is handled as compressed data, but the frame buffer is still allocated and fully filled at `width x height x 4`.
5. Frame buffer resources are limited; avoid occupying too many buffers simultaneously.
