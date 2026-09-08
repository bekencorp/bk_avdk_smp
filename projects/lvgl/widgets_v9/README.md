# LVGL Widgets v9 Project Overview

This project runs the official LVGL v9 widgets demo on BK7258. It demonstrates LVGL v9 controls, themes, layouts, and touch interaction.

## 1. Directory Layout
```
widgets_v9/
├── CMakeLists.txt                 # Top-level CMake entry
├── Makefile                       # Make build entry
├── ap/                            # AP-core application sources
│   ├── ap_main.c                  # LVGL v9, LCD, touch, and widgets demo initialization
│   └── config/                    # BK7258 AP-side configuration
├── cp/                            # CP-core configuration
│   └── config/
└── partitions/                    # Flash and RAM layout
```

## 2. Features
- Creates the LVGL v9 widgets demo through `lv_demo_widgets()`.
- Demonstrates LVGL v9 widgets, themes, layouts, and interactive components.
- Targets the RGB LCD described by `lcd_device_h050iwv` by default; control pins are configured in `ap/ap_main.c`.
- Uses `lv_vendor` partial refresh and allocates frame buffers based on `CONFIG_LVGL_FRAME_BUFFER_NUM`.
- Supports optional touch input through `CONFIG_TP`.

## 3. Hardware & Configuration
- Hardware: BK7258 development board, the RGB LCD matching `lcd_device_h050iwv`, and optional touch panel.
- LCD control: GPIO0/GPIO12/GPIO1/GPIO6 are used by default, with GPIO7 for backlight and GPIO13 for LCD LDO.
- Software: enable LVGL v9, `CONFIG_LVGL`, and `CONFIG_LV_USE_DEMO_WIDGETS`; enable `CONFIG_TP` if touch is required.

## 4. Build & Flash
```
make bk7258 PROJECT=lvgl/widgets_v9
```

Flash the generated firmware and reset the board. The LVGL v9 widgets demo starts automatically.

## 5. Runtime Flow
1. Connect the LCD, touch panel, and development board.
2. Build and flash the firmware.
3. Reset the board and check UART logs for framebuffer allocation, LCD open, and touch initialization.
4. Tap or scroll through the UI to verify LVGL v9 widgets and touch response.

## 6. Troubleshooting
- **Blank screen or wrong resolution**: Confirm the actual panel matches the `lcd_device_h050iwv` configuration.
- **Touch orientation is wrong**: Check the resolution and mirror parameters passed to `drv_tp_open()`.
- **Widget or theme abnormality**: Confirm the project is using LVGL v9 configuration and demo resources.
