# LVGL Widgets V9 Example

* [中文](./README_CN.md)

## 1. Overview

`widgets_v9` is an LVGL official Widgets Demo project for BK7259. It uses **LVGL V9.3.0** and demonstrates common UI widgets such as buttons, sliders, lists, charts, images, and fonts.

In short:

1. It is the LVGL V9 bring-up example for BK7259.
2. It verifies the MIPI DSI LCD, DPU, frame buffer, LVGL vendor layer, and touch input path.
3. It can be used as a template for a new UI project. Keep the low-level initialization flow, then replace the official demo pages with your own pages.

### 1.1 Default Hardware And Display Settings

| Item | Default |
| --- | --- |
| Chip | BK7259 |
| LVGL version | LVGL V9.3.0 |
| LCD interface | MIPI DSI |
| LCD driver | `lcd_device_hx8399c_mipi_1080x1920` |
| Resolution | `1080 x 1920` |
| Pixel format | `ARGB8888` |
| DPU output decompression | Enabled by default |
| Render mode | `RENDER_PARTIAL_MODE` |
| Backlight GPIO | `GPIO_7` |
| LCD reset GPIO | `GPIO_60` |
| Touch | Enabled by default with `CONFIG_TP=y` |

If your board uses a different LCD, resolution, or GPIO assignment, adapt the display hardware first. Otherwise LVGL may start correctly but the panel can still stay black.

## 2. Directory Structure

The project uses an AP + CP dual-core structure. Beginners usually only need to focus on the `ap/` directory.

```text
widgets_v9/
├── CMakeLists.txt                 # Project-level CMake file
├── Makefile                       # make build entry
├── README.md                      # English project guide
├── README_CN.md                   # Chinese project guide
├── ap/                            # AP side: LVGL, display, touch, UI logic
│   ├── ap_main.c                  # AP entry, initializes display and starts LVGL demo
│   ├── lv_demo_widgets.c          # LVGL V9 Widgets Demo UI
│   ├── lv_demo_widgets.h
│   ├── lv_conf_custom.h           # Project-level LVGL config overrides
│   ├── assets/                    # Image assets used by the demo
│   └── config/bk7259_ap/
│       ├── defconfig              # AP feature switches
│       └── usr_gpio_cfg.h         # AP GPIO default config
├── cp/                            # CP side: starts AP system and basic system logic
└── partitions/                    # Flash/RAM partition config
```

Suggested reading order:

1. `README.md`: learn how to build and run the project.
2. `ap/ap_main.c`: understand display, touch, and LVGL initialization.
3. `ap/lv_demo_widgets.c`: inspect the official LVGL V9 Widgets Demo UI.
4. `ap/config/bk7259_ap/defconfig`: check feature switches.
5. `ap/lv_conf_custom.h`: check LVGL options enabled by this project.

## 3. What You Should See

After boot, the AP side initializes the display path and calls `lv_demo_widgets()`. The LCD should show the LVGL official Widgets Demo.

The demo includes:

| Module | Description |
| --- | --- |
| Buttons, switches, sliders | Basic interactive widgets |
| Lists, menus, tabs | Common page organization patterns |
| Image assets | How C-array image resources are compiled into the project |
| Gauge needle image | `widgets_v9` additionally compiles `img_demo_widgets_needle.c` |
| Fonts | Montserrat 12/16/18/20/24 are enabled |
| Performance monitor | FPS/CPU information when `CONFIG_LV_USE_PERF_MONITOR=y` |

## 4. Quick Start

### 4.1 Prepare The Environment

Make sure:

1. You are in the SDK root directory: `bk_avdk_smp_release_4.0.1/`.
2. The build environment is ready.
3. The board is connected to the LCD and touch panel expected by this project.

### 4.2 Build

Run this command in the SDK root directory:

```bash
make bk7259 PROJECT=lvgl/widgets_v9
```

Or use the CI-style Docker build command:

```bash
./dbuild.sh make bk7259 PROJECT=lvgl/widgets_v9
```

The firmware is generated under the `build/` directory after a successful build.

### 4.3 Flash

Use the Beken flashing tool or the corresponding SDK flashing flow to download the generated BK7259 firmware to the board.

### 4.4 Run

After flashing, power on or reset the board:

1. The serial log should show system startup, media service initialization, and LVGL initialization.
2. The backlight GPIO `GPIO_7` is driven high.
3. The LCD opens and displays the LVGL Widgets Demo.
4. If the touch panel matches the default configuration, you can tap and slide on the demo UI.

## 5. Startup Flow

`ap/ap_main.c` is the key file for understanding this project.

The main flow is:

1. `main()` calls `bk_init()`.
2. `media_service_init()` initializes the media service.
3. `bk_lodoen_enable()` configures related power registers.
4. `bk_frame_buffer_init()` initializes the frame buffer system.
5. The `widgets` CLI command is registered.
6. `lvgl_app_widgets_init()` initializes display, touch, and LVGL.

Inside `lvgl_app_widgets_init()`:

1. Create the DSI bus with `bk_display_dsi_bus_new()`.
2. Create the MIPI LCD panel with `bk_lcd_mipi_panel_new()`.
3. Create the DPU display controller with `bk_display_dpu_ctlr_new()`.
4. Initialize and open display with `bk_display_init()` and `bk_display_open()`.
5. Turn on backlight through `GPIO_7`.
6. Configure `lv_vnd_config_t`.
7. Calculate frame buffer size according to `output_compress`.
8. Allocate LVGL frame buffers.
9. Initialize the LVGL vendor layer with `lv_vendor_init()`.
10. Open touch with `drv_tp_open()` if `CONFIG_TP` is enabled.
11. Call `lv_demo_widgets()` inside `lv_vendor_disp_lock()` and `lv_vendor_disp_unlock()`.
12. Start the LVGL task with `lv_vendor_start()`.

## 6. Key Configuration

### 6.1 Feature Switches

Important options in `ap/config/bk7259_ap/defconfig`:

| Option | Description |
| --- | --- |
| `CONFIG_LVGL=y` | Enable LVGL |
| `CONFIG_LVGL_V9=y` | Select LVGL V9 |
| `CONFIG_BK_DISPLAY=y` | Enable display framework |
| `CONFIG_MIPI_DSI=y` | Enable MIPI DSI |
| `CONFIG_DPU_DRIVER=y` | Enable DPU display controller |
| `CONFIG_FRAME_BUFFER=y` | Enable frame buffer |
| `CONFIG_LCD_HX8399C_MIPI_1080x1920=y` | Enable the default MIPI LCD driver |
| `CONFIG_TP=y` | Enable touch |
| `CONFIG_LVGL_FRAME_BUFFER_NUM=2` | Use two LVGL frame buffers |
| `CONFIG_LVGL_USE_GPU_ROTATE=y` | Enable LVGL GPU rotation support |
| `CONFIG_LVGL_TASK_STACK_SIZE=8192` | LVGL task stack size |

### 6.2 Project-Level LVGL Config

`ap/lv_conf_custom.h` includes the SDK default `lv_conf.h` first, then overrides options needed by this project.

| Option | Description |
| --- | --- |
| `LV_USE_DEMO_WIDGETS=1` | Enable the official LVGL Widgets Demo |
| `LV_FONT_MONTSERRAT_24=1` | Enable Montserrat 24 |
| `LV_FONT_MONTSERRAT_20=1` | Enable Montserrat 20 |
| `LV_FONT_MONTSERRAT_18=1` | Enable Montserrat 18 |
| `LV_FONT_MONTSERRAT_16=1` | Enable Montserrat 16 |
| `LV_FONT_MONTSERRAT_12=1` | Enable Montserrat 12 |

Enable additional LVGL features here when possible. Avoid modifying the shared LVGL component config unless the change is meant for all projects.

### 6.3 ARGB8888 And Compressed Output

`widgets_v9` uses these defaults:

| Parameter | Default | Description |
| --- | --- | --- |
| `dpu_config.video.format` | `BK_PIXEL_FORMAT_ARGB8888` | DPU video layer pixel format |
| `dpu_config.video.decompress` | `true` | DPU decompression is enabled |
| `lv_vnd_config.output_compress` | `true` | LVGL output uses the compressed path |
| `draw_pixel_size` | `WIDTH * 64 * sizeof(bk_color_t)` | Draw buffer size in partial mode |

This is one of the main differences from `widgets_v8`. If you only want to modify the UI, you usually do not need to change these parameters. Review this part when changing display format, compression strategy, or performance settings.

## 7. Common Changes

### 7.1 Change LCD Model

Update these places together:

1. The LCD header included by `ap/ap_main.c`.
2. The LCD device object passed to `bk_lcd_mipi_panel_new()`.
3. The corresponding `CONFIG_LCD_xxx` option in `ap/config/bk7259_ap/defconfig`.

If the resolution changes, also update:

```c
#define WIDTH (1080)
#define HEIGHT (1920)
```

### 7.2 Change Backlight, Reset, Or Touch GPIOs

| Function | Default | Location |
| --- | --- | --- |
| Backlight | `GPIO_7` | `ap/ap_main.c`, near `bk_gpio_set_output_high(GPIO_7)` |
| LCD reset | `GPIO_60` | `ap/ap_main.c`, `panel_config.reset_pin` |
| TP reset/int/I2C | `GPIO_5/6/1/0` | `ap/config/bk7259_ap/defconfig` |

### 7.3 Replace The Demo UI

The simplest path:

1. Keep the initialization flow such as `lv_vendor_init()`, `drv_tp_open()`, and `lv_vendor_start()`.
2. Replace `lv_demo_widgets()` with your own page creation function, for example `my_ui_create()`.
3. If UI APIs are called from another task, protect them with `lv_vendor_disp_lock()` and `lv_vendor_disp_unlock()`.

```c
lv_vendor_disp_lock();
my_ui_create();
lv_vendor_disp_unlock();
```

### 7.4 Add Image Assets

Use existing files under `ap/assets/` as references:

1. Convert the image to an LVGL C-array resource.
2. Put the generated `.c` file under `ap/assets/`.
3. Add it to the `srcs` list in `ap/CMakeLists.txt`.
4. Declare and use it in your UI code.

Current image assets:

| Asset | Description |
| --- | --- |
| `assets/img_clothes.c` | Clothes image |
| `assets/img_demo_widgets_avatar.c` | Avatar image |
| `assets/img_demo_widgets_needle.c` | Needle image |
| `assets/img_lvgl_logo.c` | LVGL logo |

## 8. FAQ

### 8.1 Build Fails Because Demo, Fonts, Or Assets Are Missing

Check:

1. `LV_USE_DEMO_WIDGETS` is enabled in `ap/lv_conf_custom.h`.
2. Fonts used by the UI are enabled in `ap/lv_conf_custom.h`.
3. New source or asset files are added to `ap/CMakeLists.txt`.
4. V8 and V9 demo code are not mixed. `widgets_v9` should use LVGL V9 APIs and demo files.

### 8.2 LCD Stays Black

Check:

1. The LCD model matches `lcd_device_hx8399c_mipi_1080x1920`.
2. MIPI cable, power, and backlight are correct.
3. `GPIO_7` is the actual backlight control pin.
4. `GPIO_60` is the actual LCD reset pin.
5. The corresponding LCD driver option is enabled in `defconfig`.

### 8.3 Touch Does Not Work

Check:

1. `CONFIG_TP=y` is enabled.
2. `CONFIG_TP_RST_PIN`, `CONFIG_TP_INT_PIN`, `CONFIG_TP_I2C_SDA_PIN`, and `CONFIG_TP_I2C_SCL_PIN` match the hardware.
3. The correct touch IC option is enabled.
4. `drv_tp_open()` is executed.

### 8.4 UI Crashes Occasionally

LVGL is not thread-safe by default. If LVGL APIs are called outside the LVGL task, protect them:

```c
lv_vendor_disp_lock();
/* LVGL API calls */
lv_vendor_disp_unlock();
```

If the UI is complex, consider increasing `CONFIG_LVGL_TASK_STACK_SIZE`.

### 8.5 Migrating From LVGL V8 To V9

LVGL V9 has API and object model changes compared with V8. Suggested migration flow:

1. Do not directly copy V8 UI files into this project and expect them to build.
2. Bring up display and touch with `widgets_v9` first.
3. Migrate pages one by one using LVGL V9 APIs.
4. Build, flash, and verify after each page is migrated.

## 9. widgets_v8 Vs widgets_v9

| Item | `widgets_v8` | `widgets_v9` |
| --- | --- | --- |
| LVGL version | V8.3.9 | V9.3.0 |
| Feature switch | `CONFIG_LVGL_V8=y` | `CONFIG_LVGL_V9=y` |
| Demo code | LVGL V8 Widgets Demo | LVGL V9 Widgets Demo |
| Pixel format | `BK_PIXEL_FORMAT_RGB565` | `BK_PIXEL_FORMAT_ARGB8888` |
| DPU decompression | Disabled by default | Enabled by default |
| Extra asset | No `img_demo_widgets_needle.c` | Includes `img_demo_widgets_needle.c` |
| Recommended use | Existing LVGL V8 projects | New projects based on LVGL V9 |

For new projects without legacy LVGL V8 code, prefer this project. For existing V8 UI code, use `widgets_v8` as the reference.

