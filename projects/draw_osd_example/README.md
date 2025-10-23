# Draw OSD Example

* [中文](./README_CN.md)

## Overview
This example demonstrates the usage of the OSD (On-Screen Display) controller in the BK7258 platform. It provides functionality for drawing images, text, and managing display resources on LCD screens.


* For detailed information about draw_osd, please refer to:

  - [draw_osd Overview](../../../developer-guide/display/draw_osd.html)

* For API reference, please refer to:

  - [draw_osd API](../../../api-reference/multimedia/bk_draw_osd.html)

## Supported Features
- **OSD Controller Management**: Initialize and deinitialize OSD controller
- **Image Drawing**: Draw images on the frame buffer
- **Font Rendering**: Display text on the frame buffer
- **Resource Management**: Add, update, remove and query display resources
- **Information Query**: Get current drawing information and available assets
- **PSRAM Support**: Configure memory usage between PSRAM and SRAM

## Components
- **bk_draw_osd**: OSD controller driver
- **bk_dma2d**: DMA2D hardware acceleration (for image blending)
- **frame_buffer**: Frame buffer management
- **lcd_display**: LCD display controller
- **cli**: Command-line interface for testing

## Project Structure

```
draw_osd_example/
├── ap/
│   ├── draw_osd/
│   │   ├── src/
│   │   │   ├── draw_osd_cli.c       # CLI command-line interface implementation
│   │   │   └── blend_assets/        # OSD resource files
│   │   │       ├── blend.h          # Resource header file
│   │   │       ├── bk_dsc.c         # Resource description array
│   │   │       ├── bk_font.c        # Font resources
│   │   │       └── bk_img.c         # Image resources
│   │   └── include/
│   │       └── draw_osd_test.h      # Test interface header file
│   ├── ap_main.c                    # AP core main program
│   └── config/                      # Configuration files
├── cp/                              # CP core related code
├── partitions/                      # Partition configuration
├── README_CN.md                     # Chinese documentation
├── README.md                        # English documentation
└── CMakeLists.txt                   # Build configuration file
```

## Test Environment

   * Hardware Configuration:
      * Core board: **BK7258_QFN88_9X9_V3.2**
      * PSRAM: 8M/16M
      * LCD adapter board
      * RGB interface LCD, using ST7701SN 480x854 screen in demo

.. warning::

    Please use the reference peripherals for demo project familiarization and learning. If the peripheral specifications are different, the code may need to be reconfigured.

## Compilation and Execution

Build the project using the following command:

```bash
make bk7258 PROJECT=draw_osd_example
```

After compilation, flash the generated firmware to the development board, and then use the following commands via serial terminal to test OSD functions:

Command execution success: `CMDRSP:OK`

Command execution failure: `CMDRSP:ERROR`

## Command Line Interface
The example provides a command-line interface for testing different OSD operations. The following commands are available:

### Basic Commands
```bash
# Initialize OSD controller and LCD display
> osd init

# Deinitialize OSD controller and LCD display
> osd deinit
```

### Resource Management Commands
```bash
# Display array of resources and update the display
> osd array

# Add or update a resource in the array
> osd array updata <resource_name> <content>

# Remove a resource from the array
> osd array remove <resource_name>
```

### Drawing Commands
```bash
# Draw an image
> osd img

# Draw text
> osd font
```

### Information Query Commands
```bash
# Get current drawing information (with log printing)
> osd info

# Get current drawing information (without log printing)
> osd info no_print

# Get all available assets (with log printing)
> osd assets

# Get all available assets (without log printing)
> osd assets no_print
```

## Implementation Details

### OSD Resource Management

OSD resources in this example are generated using UI tool, including:

**Font Resources:**
- `font_clock`: Clock display font (120x44 pixels)
- `font_dates`: Date display font (180x24 pixels)
- `font_ver`: Version display font (180x24 pixels)

**Image Resources:**
- `img_wifi_rssi0-4`: WiFi signal strength icons (5 levels)
- `img_battery1`: Battery icon
- `img_cloudy_to_sunny`: Weather icon

**Resource Arrays:**
- `blend_assets`: Total resource array containing all available resources
- `blend_info`: Default resource array to be drawn

### Memory Management

- All frame buffers allocated via `frame_buffer_display_malloc()` from display memory pool
- Released via `frame_buffer_display_free()` after use
- For displayed frames, use callback function `display_frame_free_cb` for automatic release
- OSD drawing buffer automatically allocated based on largest OSD element size

### OSD Drawing Flow

1. **Initialization Phase:**
   - Create LCD display controller
   - Open LCD display
   - Create OSD controller and configure resources

2. **Drawing Phase:**
   - Allocate background frame buffer
   - Call OSD drawing APIs (array/image/font)
   - Flush to LCD display

3. **Cleanup Phase:**
   - Delete OSD controller
   - Close and delete LCD display controller
   - Release all resources

### DMA2D Acceleration

OSD drawing internally uses DMA2D hardware acceleration for image blending:
- Font rendering: Uses DMA2D blend function
- Image overlay: Uses DMA2D blend function
- Supports ARGB8888 format alpha blending

## Notes

**Basic Usage:**
- Must use `osd init` command to initialize controller before operations
- Recommended to use `osd deinit` command to release resources after operations
- Example uses ST7701SN 480x854 screen; configuration needs modification for other screens

**Resource Configuration:**
- OSD resources generated by UI tool: https://docs.bekencorp.com/arminodoc/bk_app/ui_designer/zh_CN/latest/index.html
- Images must use ARGB8888 format
- Fonts must be generated using FontCvt.exe tool
- Resource array must end with `{.addr = NULL}`

**Memory Usage:**
- Can configure PSRAM or SRAM for drawing during initialization
- PSRAM suitable for large OSD elements
- SRAM drawing is faster but memory limited
- Ensure PSRAM is configured correctly (8M/16M)

**Dynamic Updates:**
- After `osd array updata`, must call `osd array` again to display
- After `osd array remove`, must call `osd array` again to take effect
- Elements with same name are managed together
- content distinguishes different resources under the same name

**Coordinates and Dimensions:**
- OSD element coordinates relative to background frame origin (0,0)
- OSD elements must not exceed background frame boundaries
- Drawing order affects layering effect