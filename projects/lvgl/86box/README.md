# LVGL 86box Project Overview

This project is an LVGL-based 86 box UI demo for BK7258. It demonstrates page switching, home-control style screens, image resources, and optional touch interaction.

## 1. Directory Layout
```
86box/
├── CMakeLists.txt                 # Top-level CMake entry
├── Makefile                       # Make build entry
├── ap/                            # AP-core application sources
│   ├── ap_main.c                  # LVGL, LCD, touch, and page entry initialization
│   ├── images/                    # Page image resources
│   └── config/                    # BK7258 AP-side configuration
├── cp/                            # CP-core configuration
│   └── config/
└── partitions/                    # Flash and RAM layout
```

## 2. Features
- Initializes the LVGL display stack through `lv_vendor` with partial refresh and multiple frame buffers.
- Targets the ST7701S RGB LCD by default; control pins are defined in `rgb_ctlr_config` in `ap/ap_main.c`.
- Creates the 86 box UI through `hor_page_load_main()`.
- Supports optional touch input when `CONFIG_TP` is enabled.
- Enables LCD LDO, backlight, and PSRAM power during startup.

## 3. Hardware & Configuration
- Hardware: BK7258 development board, ST7701S RGB LCD, and optional touch panel.
- LCD control: GPIO0/GPIO12/GPIO1/GPIO6 are used for RGB LCD control, GPIO7 for backlight, and GPIO13 for LCD LDO by default.
- Software: enable `CONFIG_LVGL`; enable `CONFIG_TP` if touch input is required.

## 4. Build & Flash
```
make bk7258 PROJECT=lvgl/86box
```

Flash the generated firmware to the board. After reset, the application initializes LVGL, opens the LCD, and enters the 86 box demo UI automatically.

## 5. Runtime Flow
1. Connect the LCD, optional touch panel, and development board.
2. Flash the firmware and reset the board.
3. Check UART logs for framebuffer allocation, LCD open, and touch initialization errors.
4. Use touch gestures or taps to verify page interaction after the UI appears.

## 6. Troubleshooting
- **Blank screen**: Check the ST7701S panel model, RGB control pins, backlight GPIO7, and LCD LDO GPIO13.
- **Touch not working**: Make sure `CONFIG_TP` is enabled and verify the touch I2C wiring and mirror settings.
- **Startup failure or low memory**: Confirm PSRAM is available and adjust `CONFIG_LVGL_FRAME_BUFFER_NUM` or framebuffer allocation for the display resolution.
