# QSPI LCD Example Project

* [中文](./README_CN.md)

> For beginners: this guide starts from "what does this project do" and walks you through Chapter 4 step by step to light up a QSPI-interface LCD panel.

## 1. Project Overview

This project demonstrates the minimal **QSPI-interface LCD** display flow on the Beken platform. Based on the unified `bk_display_*` controller APIs, it creates the display controller, initializes and opens the panel, turns on the backlight, allocates a frame buffer, fills the whole screen with a solid color, and flushes it to the panel.

QSPI uses **4-line (quad data line) mode**, which has higher bandwidth than plain SPI and suits larger, faster-refreshing panels.

In short, it does two things:

1. **Auto color cycle at boot**: after power-on the screen automatically cycles **Red -> Green -> Blue**, holding each color for 3 seconds. If you see the cycling colors, the panel is lit successfully.
2. **Manual fill via UART**: send the `qspi_lcd` command over the serial port to fill the whole screen with any color you choose.

### 1.1 Test Environment

* Hardware configuration:
    * Core board **BK7259_QF128_12.3X12.3_V4.0**
    * PSRAM 32M
* Default QSPI panel: `spd2010` (resolution **412 x 412**)
    * Pixel format: `RGB565` (2 bytes per pixel)
    * Interface: QSPI (4-line mode `CONFIG_QSPI_LINE_MODE=4`, TE sync enabled)

> Note: Please use the reference peripherals above to familiarize yourself with this demo. If your panel model / resolution / pins differ, the code and configuration must be adjusted accordingly (see Chapter 6).

## 2. Directory Structure

The project uses an AP-CP dual-core architecture; the main business code is under the AP directory:

```
qspi_lcd_example/
├── CMakeLists.txt            # Project-level CMake build file
├── Makefile                  # Make build entry
├── README.md                 # Project documentation (English, this file)
├── README_CN.md              # Project documentation (Chinese)
├── app.rst                   # Project description (rst skeleton)
├── ap/                       # AP-side code (core logic)
│   ├── CMakeLists.txt        # AP-side CMake build file
│   ├── ap_main.c             # AP entry: display flow, backlight, CLI command, boot auto-refresh task
│   └── config/bk7259_ap/
│       ├── defconfig         # AP feature switches (QSPI LCD, display, frame buffer, etc.)
│       └── usr_gpio_cfg.h    # AP GPIO defaults (panel pins live here)
├── cp/                       # CP-side code (system boot, UART0, etc.)
└── partitions/               # Partition configuration
```

Beginners only need to focus on `ap/ap_main.c` and `ap/config/bk7259_ap/`.

## 3. Hardware Connection

The code uses the following pins by default (defined in `ap/config/bk7259_ap/usr_gpio_cfg.h` and `ap/ap_main.c`). Wire the panel pins to the GPIOs below:

| Panel pin | Description | Default GPIO | Defined in |
| --- | --- | --- | --- |
| CLK | QSPI clock | GPIO_22 | `usr_gpio_cfg.h` (QSPI0_CLK) |
| CS / CSN | Chip select | GPIO_23 | `usr_gpio_cfg.h` (QSPI0_CSN) |
| IO0 | Data line 0 | GPIO_24 | `usr_gpio_cfg.h` (QSPI0_IO0) |
| IO1 | Data line 1 | GPIO_25 | `usr_gpio_cfg.h` (QSPI0_IO1) |
| IO2 | Data line 2 | GPIO_26 | `usr_gpio_cfg.h` (QSPI0_IO2) |
| IO3 | Data line 3 | GPIO_27 | `usr_gpio_cfg.h` (QSPI0_IO3) |
| RESET / RST | Reset | GPIO_40 | `QSPI_LCD_RESET_PIN` in `ap_main.c` |
| TE | Tearing-effect sync | GPIO_41 | `QSPI_LCD_TE_PIN` in `ap_main.c` |
| BLK / Backlight | Backlight enable | GPIO_7 | `QSPI_LCD_BACKLIGHT_PIN` in `ap_main.c` |
| VCC / GND | Power | 3.3V / GND per module spec | On the module |

> The backlight is turned on by `lcd_backlight_open()`, which drives GPIO_7 high during display initialization.

## 4. Quick Start (Build, Flash, Run)

### 4.1 Build

From the SDK root directory:

```bash
make bk7259 PROJECT=multimedia/qspi_lcd_example
```

After a successful build, the firmware is under `build/qspi_lcd_example/bk7259/`.

### 4.2 Flash

Use the Beken flashing tool to flash the generated firmware to the board.

### 4.3 Run

After flashing, power on (or type `reboot` in the serial console):

* **Expected behavior**: the screen automatically cycles **Red -> Green -> Blue**, holding each color about 3 seconds (driven by `qspi_lcd_auto_refresh_task`).
* Seeing the cycling colors means the QSPI panel is lit and the backlight is on.

## 5. Serial Commands

You can manually fill the screen with a specific color from the serial terminal:

| Command | Effect |
| --- | --- |
| `qspi_lcd` | Fill the whole screen with the default red (`0xF800`) |
| `qspi_lcd <RGB565 hex>` | Fill the whole screen with the specified color |

The color is an **RGB565** hexadecimal value. Common colors:

```text
qspi_lcd F800   # Red
qspi_lcd 07E0   # Green
qspi_lcd 001F   # Blue
qspi_lcd FFFF   # White
qspi_lcd 0000   # Black
```

> Tip: the boot auto-refresh task keeps running; a color you set manually with `qspi_lcd` will be overwritten on the next auto cycle. This is expected and makes screen verification quick.

## 6. Key Configuration

### 6.1 Feature Switches (`ap/config/bk7259_ap/defconfig`)

| Option | Meaning |
| --- | --- |
| `CONFIG_BK_DISPLAY=y` | Enable the unified `bk_display_*` framework |
| `CONFIG_FRAME_BUFFER=y` | Enable frame-buffer management |
| `CONFIG_LCD_QSPI=y` | Enable the QSPI LCD driver |
| `CONFIG_LCD_QSPI_SPD2010=y` | Enable the spd2010 panel driver |
| `CONFIG_LCD_QSPI_COLOR_DEPTH_BYTE=2` | 2 bytes per pixel (RGB565) |
| `CONFIG_QSPI=y` / `CONFIG_QSPI_LINE_MODE=4` | QSPI 4-line mode |
| `CONFIG_LCD_QSPI_TE=y` | Enable the TE tearing-effect sync signal |
| `CONFIG_LCD_QSPI_REFRESH_WITH_MAPPING=y` | Use mapping-based refresh |
| `CONFIG_MEDIA_SERVICE=y` | Enable the media service |

### 6.2 Display Parameters (`ap/ap_main.c`)

```c
#define QSPI_LCD_BACKLIGHT_PIN GPIO_7
#define QSPI_LCD_RESET_PIN     GPIO_40
#define QSPI_LCD_TE_PIN        GPIO_41

static bk_display_qspi_ctlr_config_t qspi_ctlr_config = {
    .lcd_panel = &lcd_device_spd2010,        // panel driver
    .qspi_id   = 0,                          // use QSPI0
    .reset_pin = QSPI_LCD_RESET_PIN,         // reset pin
    .te_pin    = QSPI_LCD_TE_PIN,            // TE sync pin
};
```

* Change panel: replace `lcd_device_spd2010` with the matching driver and enable the corresponding `CONFIG_LCD_QSPI_xxx` in `defconfig`.
* Change pins: edit `QSPI_LCD_RESET_PIN` / `QSPI_LCD_TE_PIN` / `QSPI_LCD_BACKLIGHT_PIN`, and update the QSPI0 data-line GPIO assignment in `usr_gpio_cfg.h` accordingly.
* Change auto-refresh cadence: edit `QSPI_LCD_AUTO_REFRESH_INTERVAL_MS` (default 3000ms) and the `s_qspi_lcd_auto_colors[]` color table.

## 7. Display Flow (How It Works)

In `ap_main.c`, the first flush performs a one-time initialization (`qspi_lcd_display_init`):

1. `bk_display_qspi_ctlr_new()`: create the QSPI display controller from `qspi_ctlr_config`
2. `bk_display_init()` -> `bk_display_open()`: initialize and open the panel
3. `bk_frame_buffer_malloc()`: allocate a frame buffer of size `qspi->frame_len` from the panel driver
4. `lcd_backlight_open()`: drive GPIO_7 high to turn on the backlight
5. Each subsequent refresh: `qspi_lcd_display_fill_pure_color()` fills the color, then `bk_display_flush()` flushes it to the panel

Initialization runs only once (`is_display_init` flag); on failure the `fail` path releases the frame buffer and controller.

## 8. Notes

1. The boot auto-refresh task keeps running, so a manual `qspi_lcd` color is overwritten on the next cycle. This is expected.
2. The display controller is initialized only once and reuses the same frame buffer throughout; on init failure resources are reclaimed automatically.
3. Colors are parsed as **RGB565**; pass a hex value (e.g. `F800`). Bits beyond 16 are truncated.
4. If the screen stays dark: first check the wiring (especially RESET / backlight GPIO_7), the 6 QSPI data lines, and that the panel model matches `defconfig`.
5. QSPI panels are sensitive to timing and routing; in 4-line mode make sure IO0~IO3, CLK, and CS are correctly wired and length-matched.
6. If your panel spec differs from the reference peripheral, the code and configuration must be adapted.
