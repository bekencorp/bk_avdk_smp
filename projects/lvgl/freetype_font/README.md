# LVGL FreeType Font Project Overview

This project demonstrates LVGL FreeType font loading on BK7258. It reads `Lato-Regular.ttf` from LittleFS in flash and renders text with a custom FreeType font.

## 1. Directory Layout
```
freetype_font/
├── CMakeLists.txt                 # Top-level CMake entry
├── Makefile                       # Make build entry
├── ap/                            # AP-core application sources
│   ├── ap_main.c                  # LVGL, QSPI LCD, VFS, and FreeType initialization
│   ├── resource/                  # LittleFS font resources and flashing note
│   └── config/                    # BK7258 AP-side configuration
├── cp/                            # CP-core configuration
│   └── config/
└── partitions/                    # Flash and RAM layout
```

## 2. Features
- Mounts the LittleFS partition mapped to `BK_PARTITION_USR_CONFIG` at `VFS_INTERNAL_FLASH_PATITION_0`.
- Reads `Lato-Regular.ttf` from LittleFS and creates a 24 px FreeType font through `lv_ft_font_init()`.
- Displays centered text rendered with the FreeType font.
- Uses the QSPI LCD described by `lcd_device_st77903_h0165y008t` by default.
- Supports optional touch input through `CONFIG_TP`.

## 3. Resource Preparation
`ap/resource/readme.txt` notes that `littlefs.bin` must be flashed to the usr partition. The default address is `0x3eb000`. If the partition table changes, use the current start address of `BK_PARTITION_USR_CONFIG`.

LittleFS should contain:
```
Lato-Regular.ttf
```

## 4. Hardware & Configuration
- Hardware: BK7258 development board, the QSPI LCD matching `lcd_device_st77903_h0165y008t`, and optional touch panel.
- LCD control: QSPI ID 0, reset GPIO40, and backlight GPIO7 by default.
- Software: enable `CONFIG_LVGL`, LVGL FreeType support, `CONFIG_VFS`, and LittleFS-related options.

## 5. Build & Flash
```
make bk7258 PROJECT=lvgl/freetype_font
```

After flashing the application firmware, also flash `littlefs.bin` to the usr partition. Otherwise the application will fail to open the font file at runtime.

## 6. Runtime Flow
1. Prepare a LittleFS image containing `Lato-Regular.ttf`.
2. Flash both the project firmware and the LittleFS image.
3. Reset the board and check UART logs for LittleFS mount, file read, and `lv_ft_font_init()` results.
4. The centered `Hello world` text indicates the FreeType font loaded successfully.

## 7. Troubleshooting
- **file_content open failed**: LittleFS was not flashed, the partition address is wrong, or `Lato-Regular.ttf` is missing.
- **create failed**: Make sure LVGL FreeType support is enabled, the font file is valid, and memory is sufficient.
- **Blank screen**: Check the QSPI LCD model, reset GPIO40, backlight GPIO7, and PSRAM power state.
