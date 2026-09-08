# LVGL Image Decode Project Overview

This project demonstrates LVGL image decoding and display on BK7258. It reads image resources from LittleFS in flash and displays the decoded image through LVGL.

## 1. Directory Layout
```
img_decode/
├── CMakeLists.txt                 # Top-level CMake entry
├── Makefile                       # Make build entry
├── ap/                            # AP-core application sources
│   ├── ap_main.c                  # LVGL, LCD, VFS, and image decode initialization
│   ├── resource/                  # LittleFS image resources and flashing note
│   └── config/                    # BK7258 AP-side configuration
├── cp/                            # CP-core configuration
│   └── config/
└── partitions/                    # Flash and RAM layout
```

## 2. Features
- Mounts the LittleFS partition mapped to `BK_PARTITION_USR_CONFIG` at `VFS_INTERNAL_FLASH_PATITION_0`.
- Loads `/img/anim/anim-0.jpg` from LittleFS through `lv_jpeg_img_load_with_hw_dec()` by default.
- Keeps examples for JPEG software decoding and PNG loading in code; PNG requires `LV_USE_PNG`.
- Supports both LVGL v8 and non-v8 image object APIs.
- Uses the ST7701S RGB LCD by default and displays the image through `lv_vendor` partial refresh.

## 3. Resource Preparation
`ap/resource/readme.txt` notes that `littlefs.bin` must be flashed to the usr partition. The default address is `0x3eb000`. If the partition table changes, use the current usr partition address.

The default runtime path requires LittleFS to contain:
```
/img/anim/anim-0.jpg
```

If you switch to another sample path in `ap/ap_main.c`, include that file in the LittleFS image.

## 4. Hardware & Configuration
- Hardware: BK7258 development board, ST7701S RGB LCD, and optional touch panel.
- LCD control: GPIO0/GPIO12/GPIO1/GPIO6 are used by default, with GPIO7 for backlight and GPIO13 for LCD LDO.
- Software: enable `CONFIG_LVGL`, `CONFIG_VFS`, LittleFS, and JPEG decode support; enable `LV_USE_PNG` if PNG loading is required.

## 5. Build & Flash
```
make bk7258 PROJECT=lvgl/img_decode
```

After flashing the application firmware, also flash the `littlefs.bin` image containing the image resources to the usr partition. Otherwise image loading will fail.

## 6. Runtime Flow
1. Prepare a LittleFS image containing `/img/anim/anim-0.jpg`.
2. Flash both the project firmware and the LittleFS image.
3. Reset the board and check UART logs for LittleFS mount and image decode results.
4. The image should appear centered on the display once resource loading and decoding are working.

## 7. Troubleshooting
- **Image not shown**: Confirm the LittleFS image was flashed to the usr partition and the image path matches the code.
- **PNG loading fails**: Make sure `LV_USE_PNG` is enabled and the path passed to `lv_png_img_load()` exists.
- **JPEG decode fails**: Check whether the image format is supported by hardware decoding, or switch to the software decode helper for validation.
