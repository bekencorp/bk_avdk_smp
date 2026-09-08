# LVGL Benchmark v9 Project Overview

This project runs the official LVGL v9 benchmark demo on BK7258. It is used to validate LVGL v9 widget rendering, animation, and refresh performance.

## 1. Directory Layout
```
benchmark_v9/
├── CMakeLists.txt                 # Top-level CMake entry
├── Makefile                       # Make build entry
├── ap/                            # AP-core application sources
│   ├── ap_main.c                  # LVGL v9, LCD, touch, and benchmark initialization
│   └── config/                    # BK7258 AP-side configuration
├── cp/                            # CP-core configuration
│   └── config/
└── partitions/                    # Flash and RAM layout
```

## 2. Features
- Starts the LVGL v9 benchmark screen through `lv_demo_benchmark()`.
- Targets the ST7701S RGB LCD by default; LCD timing and pins are defined in `ap/ap_main.c`.
- Uses `lv_vendor` partial refresh and checks the return value of `lv_vendor_init()` to simplify startup debugging.
- Supports optional touch input through `CONFIG_TP`.
- Initializes media service and enables PSRAM power for LVGL code execution during startup.

## 3. Hardware & Configuration
- Hardware: BK7258 development board, ST7701S RGB LCD, and optional touch panel.
- LCD control: GPIO0/GPIO12/GPIO1/GPIO6 are used by default, with GPIO7 for backlight and GPIO13 for LCD LDO.
- Software: enable the LVGL v9 configuration, `CONFIG_LVGL`, and the benchmark demo; enable `CONFIG_TP` if touch is required.

## 4. Build & Flash
```
make bk7258 PROJECT=lvgl/benchmark_v9
```

Flash the generated firmware and reset the board. The project opens the LCD and enters the LVGL v9 benchmark screen automatically.

## 5. Runtime Flow
1. Connect the ST7701S LCD and optional touch panel.
2. Build and flash the firmware.
3. Reset the board and check UART logs for `lv_vendor_init`, LCD open, and framebuffer allocation errors.
4. Record the benchmark result after the test completes when comparing LVGL v8/v9 or display configurations.

## 6. Troubleshooting
- **Initialization failure**: Check UART logs for `lv_vendor_init fail` and framebuffer allocation errors.
- **Blank or corrupted display**: Verify the panel model, RGB timing, GPIO pins, and backlight control.
- **Results are not comparable**: Keep resolution, frame buffer count, compiler optimization, and background workload consistent.
