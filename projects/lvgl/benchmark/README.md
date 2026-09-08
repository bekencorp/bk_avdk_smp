# LVGL Benchmark Project Overview

This project runs the official LVGL benchmark demo on BK7258. It is used to evaluate widget rendering, animation, and display refresh performance.

## 1. Directory Layout
```
benchmark/
├── CMakeLists.txt                 # Top-level CMake entry
├── Makefile                       # Make build entry
├── ap/                            # AP-core application sources
│   ├── ap_main.c                  # LVGL, LCD, touch, and benchmark initialization
│   └── config/                    # BK7258 AP-side configuration
├── cp/                            # CP-core configuration
│   └── config/
└── partitions/                    # Flash and RAM layout
```

## 2. Features
- Starts the LVGL benchmark screen through `lv_demo_benchmark()`.
- Uses the ST7701S RGB LCD by default; the resolution comes from the LCD device descriptor.
- Uses `lv_vendor` partial refresh and allocates frame buffers based on `CONFIG_LVGL_FRAME_BUFFER_NUM`.
- Supports optional touch input for demo interaction.
- Initializes `media_service` and enables PSRAM power for LVGL code execution during startup.

## 3. Hardware & Configuration
- Hardware: BK7258 development board, ST7701S RGB LCD, and optional touch panel.
- LCD control: GPIO0/GPIO12/GPIO1/GPIO6 are used by default, with GPIO7 for backlight and GPIO13 for LCD LDO.
- Software: enable `CONFIG_LVGL` and `CONFIG_LV_USE_DEMO_BENCHMARK`; enable `CONFIG_TP` if touch is required.

## 4. Build & Flash
```
make bk7258 PROJECT=lvgl/benchmark
```

Flash the generated firmware and reset the board. The benchmark demo starts automatically after LVGL initialization.

## 5. Runtime Flow
1. Connect the LCD and optional touch panel.
2. Build and flash the project firmware.
3. Reset the board and wait for the benchmark screen to appear.
4. Record the on-screen benchmark result when comparing LVGL configuration, frame buffer count, or display driver changes.

## 6. Troubleshooting
- **Benchmark not shown**: Make sure `CONFIG_LVGL` and `CONFIG_LV_USE_DEMO_BENCHMARK` are enabled.
- **Display abnormality**: Check the LCD model, RGB timing, and control pin assignments.
- **Unstable results**: Compare runs with the same optimization level, frame buffer count, display resolution, and background workload.
