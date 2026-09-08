# LVGL Widgets Project Overview

This project runs the official LVGL widgets demo on BK7258. It demonstrates common controls, themes, lists, charts, images, and interactive widgets.

## 1. Directory Layout
```
widgets/
├── CMakeLists.txt                 # Top-level CMake entry
├── Makefile                       # Make build entry
├── ap/                            # AP-core application sources
│   ├── ap_main.c                  # LVGL, LCD, touch, and widgets demo initialization
│   ├── assets/                    # Widgets demo image resources
│   └── config/                    # BK7258 AP-side configuration
├── cp/                            # CP-core configuration
│   └── config/
└── partitions/                    # Flash and RAM layout
```

## 2. Features
- Creates the LVGL widgets demo through `lv_demo_widgets()`.
- Demonstrates buttons, sliders, lists, charts, images, and other common LVGL widgets.
- Uses the ST7701S RGB LCD by default with `lv_vendor` partial refresh.
- Supports optional touch input through `CONFIG_TP`.
- Enables LCD LDO, backlight, and PSRAM power during startup.

## 3. Hardware & Configuration
- Hardware: BK7258 development board, ST7701S RGB LCD, and optional touch panel.
- LCD control: GPIO0/GPIO12/GPIO1/GPIO6 are used by default, with GPIO7 for backlight and GPIO13 for LCD LDO.
- Software: enable `CONFIG_LVGL` and `CONFIG_LV_USE_DEMO_WIDGETS`; enable `CONFIG_TP` if touch is required.

## 4. Build & Flash
```
make bk7258 PROJECT=lvgl/widgets
```

Flash the generated firmware and reset the board. The widgets demo starts automatically after LVGL initialization.

## 5. Runtime Flow
1. Connect the LCD, touch panel, and development board.
2. Build and flash the project firmware.
3. Reset the board and check UART logs for framebuffer, LCD, and touch initialization.
4. Tap and scroll through the UI to verify widget rendering and touch response.

## 6. Troubleshooting
- **Widget resources look wrong**: Confirm resources under `ap/assets` are included in the build and were not stripped.
- **Touch coordinates are inaccurate**: Check the touch resolution and mirror settings against the LCD orientation.
- **UI refresh is slow**: Verify PSRAM, frame buffer count, and background system load.
