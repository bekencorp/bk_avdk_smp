# DMA2D Example

* [中文](./README_CN.md)

## Overview
This example demonstrates the usage of the DMA2D (Direct Memory Access 2D) controller in the BK7258 platform. It provides functionality for 2D graphics operations such as filling regions with color, memory copying, pixel format conversion, and blending two image layers.


* For detailed information about DMA2D, please refer to:

  - [DMA2D Overview](../../../developer-guide/display/dma2d.html)

* For API reference, please refer to:

  - `DMA2D API <../../../api-reference/multimedia/bk_dma2d.html>`_

## Supported Features
- **Fill**: Fill a rectangular region with a specified color
- **Memory Copy**: Copy image data from source to destination
- **Pixel Format Conversion (PFC)**: Convert between different pixel formats (RGB565, RGB888, ARGB8888)
- **Blending**: Blend foreground and background layers with adjustable alpha value

## Components
- **bk_dma2d**: DMA2D controller driver
- **frame_buffer**: Frame buffer management
- **cli**: Command-line interface for testing

## Project Structure

```
dma2d_example/
├── ap/
│   ├── dma2d/
│   │   ├── src/
│   │   │   ├── dma2d_cli.c           # CLI command-line interface implementation
│   │   │   ├── dma2d_fill_test.c     # Fill function test code
│   │   │   ├── dma2d_memcpy_test.c   # Memory copy test code
│   │   │   ├── dma2d_pfc_test.c      # Pixel format conversion test code
│   │   │   ├── dma2d_blend_test.c    # Blend function test code
│   │   │   └── blend_assets/         # Image assets for blend testing
│   │   └── include/
│   │       └── dma2d_test.h          # Test interface header file
│   ├── ap_main.c                     # AP core main program
│   └── config/                       # Configuration files
├── cp/                               # CP core related code
├── partitions/                       # Partition configuration
├── README_CN.md                      # Chinese documentation
├── README.md                         # English documentation
└── CMakeLists.txt                    # Build configuration file
```

## Test Environment

   * Hardware Configuration:
      * Core board: **BK7258_QFN88_9X9_V3.2**
      * PSRAM: 8M/16M
      * LCD adapter board
      * RGB interface LCD, using st7701sn 480x854 screen in demo

.. warning::

    Please use the reference peripherals for demo project familiarization and learning. If the peripheral specifications are different, the code may need to be reconfigured.

## Compilation and Execution

Build the project using the following command:

```bash
make bk7258 PROJECT=dma2d_example
```

After compilation, flash the generated firmware to the development board, and then use the following commands via serial terminal to test the display functions:

Command execution success: `CMDRSP:OK`

Command execution failure: `CMDRSP:ERROR`

## Command Line Interface
The example provides a command-line interface for testing different DMA2D operations. The following commands are available:

### Basic Commands
```bash
# Create DMA2D controller instance 1 and initialize LCD display
> dma2d open module1

# Close DMA2D controller, release resources and delete DMA2D instance 1
> dma2d close module1

# Create DMA2D controller instance 2, creating multiple instances allows simultaneous use of DMA2D in multiple scenarios
> dma2d open module2

# Close DMA2D controller, release resources and delete DMA2D instance 2
> dma2d close module2
```

**Notes:**
- When executing `dma2d open` for the first time, the LCD display controller will also be initialized
- When all DMA2D instances are closed, the LCD display controller will be automatically closed
- Multiple instances share the same DMA2D hardware module, and the component internally performs scene switching protection

### Fill Command
```bash
# Fill a rectangular region with specified color
# Format: dma2d fill <format> <color> <frame_width> <frame_height> <xpos> <ypos> <fill_width> <fill_height>
# <format>: RGB565, RGB888, ARGB8888
# <color>: Hexadecimal color value (without 0x prefix)

# Example 1: Fill 480x854 frame with red color using RGB565 format
> dma2d fill RGB565 F800 480 854 0 0 480 854

# Example 2: Partial fill, fill 200x200 green region at position (100,100)
> dma2d fill RGB565 07E0 480 854 100 100 200 200

# Use module2 instance for filling
> dma2d fill_module2 RGB565 F800 480 854 0 0 480 854
```

**Notes:**
- Fill operation is executed synchronously (`is_sync = true`)
- After filling, it will automatically refresh to LCD display
- Frame buffer is allocated from display memory pool and automatically released after use

### Memory Copy Command
```bash
# Copy image data from source to destination (supports offset copy)
# Format: dma2d memcpy <format> <color> <src_width> <src_height> <dst_width> <dst_height> 
#                    <src_x> <src_y> <dst_x> <dst_y> <copy_width> <copy_height>

# Example 1: Complete copy of 32x48 image
> dma2d memcpy RGB565 F800 32 48 32 48 0 0 0 0 32 48

# Example 2: Partial copy, copy 20x20 from source (10,10) to destination (5,5)
> dma2d memcpy RGB565 F800 100 100 100 100 10 10 5 5 20 20

# Use module2 instance for copying
> dma2d memcpy_module2 RGB565 F800 32 48 32 48 0 0 0 0 32 48
```

**Notes:**
- Memory copy operation is executed asynchronously (`is_sync = false`), synchronized using semaphore
- Data verification is automatically performed after copy completion
- Supports memory copy with same format, input and output formats must be consistent

### Pixel Format Conversion Command
```bash
# Perform pixel format conversion during copy
# Format: dma2d pfc <input_format> <output_format> <color> <src_width> <src_height> <dst_width> <dst_height> 
#                  <src_x> <src_y> <dst_x> <dst_y> <width> <height>

# Example 1: Convert from RGB565 to RGB888
> dma2d pfc RGB565 RGB888 F800 32 48 32 48 0 0 0 0 32 48

# Example 2: Convert from RGB888 to ARGB8888
> dma2d pfc RGB888 ARGB8888 FF0000 100 100 100 100 0 0 0 0 100 100

# Use module2 instance for format conversion
> dma2d pfc_module2 RGB565 RGB888 F800 32 48 32 48 0 0 0 0 32 48
```

**Notes:**
- Pixel format conversion operation is executed synchronously (`is_sync = true`)
- Supported input formats: ARGB8888, RGB888, RGB565, ARGB1555, ARGB4444, etc.
- Supported output formats: ARGB8888, RGB888, RGB565, ARGB1555, ARGB4444
- Supports offset copy during conversion

### Blend Command
```bash
# Blend foreground and background layers with alpha control
# Format: dma2d blend <bg_format> <bg_color> <bg_width> <bg_height> <is_sync>
# Note: This command uses built-in image resources (cloud, WiFi signal, battery icon) for blend testing

# Example 1: Synchronous blend (0 means sync)
> dma2d blend RGB565 07E0 480 854 0

# Example 2: Asynchronous blend (1 means async)
> dma2d blend RGB565 07E0 480 854 1

# Use module2 instance for blending
> dma2d blend_module2 RGB565 07E0 480 854 0
```

**Notes:**
- Blend operation uses built-in image resources (img_cloudy_to_sunny, img_wifi_rssi0, img_battery1)
- Foreground images are fixed to ARGB8888 format with alpha information
- Supports red-blue color swap configuration (`fg_red_blue_swap = DMA2D_RB_SWAP`)
- After blending, it will automatically refresh to LCD display
- Alpha mode uses `DMA2D_NO_MODIF_ALPHA`, maintaining original image transparency

## Implementation Details

### Multi-Instance Management
- The project supports creating multiple DMA2D controller instances (module1 and module2)
- Multiple instances actually share the same DMA2D hardware module
- The component internally performs mutex protection to ensure safe multi-scenario usage
- You can obtain created handles via `bk_dma2d_get_handle()`

### Memory Management
- All frame buffers are allocated from the display memory pool via `frame_buffer_display_malloc()`
- Released via `frame_buffer_display_free()` after use
- For displayed frames, use callback function `display_frame_free_cb` for automatic release

### Synchronous and Asynchronous
- **Synchronous mode** (`is_sync = true`): API call blocks until operation completes
  - Fill operation uses synchronous mode
  - Pixel format conversion uses synchronous mode
- **Asynchronous mode** (`is_sync = false`): API call returns immediately, operation completes in background
  - Memory copy uses asynchronous mode, waits for completion via semaphore

### Error Handling
- Uses `AVDK_RETURN_ON_FALSE` and `AVDK_RETURN_ON_ERROR` macros for error checking
- Prints detailed error information on failure
- Ensures proper resource release to avoid memory leaks

## Notes
- Must use `dma2d open` command to open DMA2D controller before performing operations
- Recommended to use `dma2d close` command to close controller and release resources after operations
- Example uses frame buffers allocated from display memory pool, ensure PSRAM is configured correctly (8M/16M)
- LCD configuration is for st7701sn 480x854 screen, need to modify configuration for other screens
- Fill and blend operations use synchronous mode and will block until completion
- Memory copy operation uses asynchronous mode, synchronized via semaphore
- Ensure configured coordinates and dimensions do not exceed frame boundaries, otherwise may cause exceptions
- Pixel formats must be configured correctly, otherwise may cause data exceptions or hardware malfunction