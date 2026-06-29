# SPI LCD Example Project

* [中文](./README_CN.md)

> For beginners: this guide starts from "what does this project do" and walks you through Chapter 4 step by step to light up a small SPI-interface LCD panel.

## 1. Project Overview

This project demonstrates the minimal **SPI-interface LCD** display flow on the Beken platform. Based on the unified `bk_display_*` controller APIs, it creates the display controller, initializes and opens the panel, allocates a frame buffer, fills the whole screen with a solid color, and flushes it to the panel.

In short, it does two things:

1. **Auto color cycle at boot**: after power-on the screen automatically cycles **Red -> Green -> Blue**, holding each color for 3 seconds. If you see the cycling colors, the panel is lit successfully.
2. **Manual fill via UART**: send the `spi_lcd` command over the serial port to fill the whole screen with any color you choose.

### 1.1 Test Environment

* Hardware configuration:
    * Core board **BK7259_QF128_12.3X12.3_V4.0**
    * PSRAM 32M
* Default SPI panel: `jd9853` (resolution **240 x 296**)
    * Pixel format: `RGB565` (2 bytes per pixel)
    * Interface: SPI (reuses QSPI0 hardware, single-line mode `CONFIG_QSPI_LINE_MODE=1`)

> Note: Please use the reference peripherals above to familiarize yourself with this demo. If your panel model / resolution / pins differ, the code and configuration must be adjusted accordingly (see Chapter 6).

## 2. Directory Structure

The project uses an AP-CP dual-core architecture; the main business code is under the AP directory:

```
spi_lcd_example/
├── CMakeLists.txt            # Project-level CMake build file
├── Makefile                  # Make build entry
├── README.md                 # Project documentation (English, this file)
├── README_CN.md              # Project documentation (Chinese)
├── app.rst                   # Project description (rst skeleton)
├── ap/                       # AP-side code (core logic)
│   ├── CMakeLists.txt        # AP-side CMake build file
│   ├── ap_main.c             # AP entry: display flow, CLI command, boot auto-refresh task
│   └── config/bk7259_ap/
│       ├── defconfig         # AP feature switches (SPI LCD, display, frame buffer, etc.)
│       └── usr_gpio_cfg.h    # AP GPIO defaults (panel pins live here)
├── cp/                       # CP-side code (system boot, UART0, etc.)
└── partitions/               # Partition configuration
```

Beginners only need to focus on `ap/ap_main.c` and `ap/config/bk7259_ap/`.

## 3. Hardware Connection

The code uses the following pins by default (defined in `ap/config/bk7259_ap/usr_gpio_cfg.h` and `ap/ap_main.c`). Wire the panel pins to the GPIOs below:

| Panel pin | Description | Default GPIO | Defined in |
| --- | --- | --- | --- |
| SCLK / CLK | SPI clock | GPIO_22 | `usr_gpio_cfg.h` (QSPI0_CLK) |
| CS / CSN | Chip select | GPIO_23 | `usr_gpio_cfg.h` (QSPI0_CSN) |
| SDA / MOSI | Data (IO0) | GPIO_24 | `usr_gpio_cfg.h` (QSPI0_IO0) |
| DC / RS | Data/command select | GPIO_25 | `spi_ctlr_config.dc_pin` in `ap_main.c` |
| RESET / RST | Reset | GPIO_26 | `spi_ctlr_config.reset_pin` in `ap_main.c` |
| TE | Tearing-effect sync | Unused (`te_pin = 0`) | `ap_main.c` |
| VCC / GND / BLK | Power & backlight | 3.3V / GND per module spec | On the module |

> The SPI panel runs on the chip's QSPI0 hardware in single-line mode, so CLK/CS/MOSI reuse the QSPI0 pin names.

## 4. Quick Start (Build, Flash, Run)

### 4.1 Build

From the SDK root directory:

```bash
make bk7259 PROJECT=multimedia/spi_lcd_example
```

After a successful build, the firmware is under `build/spi_lcd_example/bk7259/`.

### 4.2 Flash

Use the Beken flashing tool to flash the generated firmware to the board.

### 4.3 Run

After flashing, power on (or type `reboot` in the serial console):

* **Expected behavior**: the screen automatically cycles **Red -> Green -> Blue**, holding each color about 3 seconds (driven by `spi_lcd_auto_refresh_task`).
* Seeing the cycling colors means the SPI panel is lit successfully.

## 5. Serial Commands

You can manually fill the screen with a specific color from the serial terminal:

| Command | Effect |
| --- | --- |
| `spi_lcd` | Fill the whole screen with the default red (`0xF800`) |
| `spi_lcd <RGB565 hex>` | Fill the whole screen with the specified color |

The color is an **RGB565** hexadecimal value. Common colors:

```text
spi_lcd F800   # Red
spi_lcd 07E0   # Green
spi_lcd 001F   # Blue
spi_lcd FFFF   # White
spi_lcd 0000   # Black
```

> Tip: the boot auto-refresh task keeps running; a color you set manually with `spi_lcd` will be overwritten on the next auto cycle. This is expected and makes screen verification quick.

## 6. Key Configuration

### 6.1 Feature Switches (`ap/config/bk7259_ap/defconfig`)

| Option | Meaning |
| --- | --- |
| `CONFIG_BK_DISPLAY=y` | Enable the unified `bk_display_*` framework |
| `CONFIG_FRAME_BUFFER=y` | Enable frame-buffer management |
| `CONFIG_LCD_SPI=y` | Enable the SPI LCD driver |
| `CONFIG_LCD_SPI_JD9853=y` | Enable the jd9853 panel driver |
| `CONFIG_LCD_SPI_COLOR_DEPTH_BYTE=2` | 2 bytes per pixel (RGB565) |
| `CONFIG_QSPI=y` / `CONFIG_QSPI_LINE_MODE=1` | SPI runs on QSPI0 hardware, single-line mode |
| `CONFIG_MEDIA_SERVICE=y` | Enable the media service |

### 6.2 Display Parameters (`ap/ap_main.c`)

```c
bk_display_spi_ctlr_config_t spi_ctlr_config = {
    .lcd_panel = &lcd_device_jd9853,         // panel driver
    .spi_id    = 0,                          // use SPI/QSPI0
    .dc_pin    = GPIO_25,                    // data/command pin
    .reset_pin = GPIO_26,                    // reset pin
    .te_pin    = 0,                          // TE not used
};
```

* Change panel: replace `lcd_device_jd9853` with the matching driver and enable the corresponding `CONFIG_LCD_SPI_xxx` in `defconfig`.
* Change pins: edit `dc_pin` / `reset_pin`, and update the CLK/CS/MOSI GPIO assignment in `usr_gpio_cfg.h` accordingly.
* Change auto-refresh cadence: edit `SPI_LCD_AUTO_REFRESH_INTERVAL_MS` (default 3000ms) and the `s_spi_lcd_auto_colors[]` color table.

## 7. Display Flow (How It Works)

In `ap_main.c`, the first flush performs a one-time initialization (`cli_spi_lcd_display_cmd`):

1. `bk_display_spi_ctlr_new()`: create the SPI display controller from `spi_ctlr_config`
2. `bk_display_init()` -> `bk_display_open()`: initialize and open the panel
3. `bk_frame_buffer_malloc()`: allocate a `width x height x 2` frame buffer
4. `lcd_spi_display_fill_pure_color()`: fill the buffer with the target color (RGB565, high byte first)
5. `bk_display_flush()`: flush the frame buffer to the panel

Subsequent refreshes repeat only steps 4 and 5; initialization runs only once.

## 8. Notes

1. The boot auto-refresh task keeps running, so a manual `spi_lcd` color is overwritten on the next cycle. This is expected.
2. The display controller is initialized only once (`is_display_init` flag) and reuses the same frame buffer throughout.
3. Colors are parsed as **RGB565**; pass a hex value (e.g. `F800`). Bits beyond 16 are truncated.
4. If the screen stays dark: first check the wiring (especially DC/RESET), power & backlight, and that the panel model matches `defconfig`.
5. If your panel spec differs from the reference peripheral, the code and configuration must be adapted.
