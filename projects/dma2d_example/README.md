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

## Command Line Interface
The example provides a command-line interface for testing different DMA2D operations. The following commands are available:

### Basic Commands
```bash
# Open DMA2D controller
> dma2d open

# Close DMA2D controller
> dma2d close

# Delete DMA2D controller
> dma2d delete
```

### Fill Command
```bash
# Fill a region with specified color
# Format: dma2d fill <format> <color> <frame_width> <frame_height> <xpos> <ypos> <fill_width> <fill_height>
# <format>: RGB565, RGB888, ARGB8888
# <color>: Hexadecimal color value
> dma2d fill RGB565 0xF800 320 240 0 0 320 240
```

### Memory Copy Command
```bash
# Copy image data from source to destination
# Format: dma2d memcpy <format> <color> <src_width> <src_height> <dst_width> <dst_height> <src_x> <src_y> <dst_x> <dst_y> <width> <height>
> dma2d memcpy RGB565 0xF800 320 240 320 240 0 0 0 0 320 240
```

### Pixel Format Conversion Command
```bash
# Convert pixel format
# Format: dma2d pfc <input_format> <output_format> <color> <src_width> <src_height> <dst_width> <dst_height> <src_x> <src_y> <dst_x> <dst_y> <width> <height>
> dma2d pfc RGB565 RGB888 0xF800 320 240 320 240 0 0 0 0 320 240
```

### Blend Command
```bash
# Blend foreground and background layers
# Format: dma2d blend <fg_format> <bg_format> <output_format> <bg_color> <fg_color> <bg_width> <bg_height> <fg_width> <fg_height> <dst_width> <dst_height> <bg_x> <bg_y> <fg_x> <fg_y> <dst_x> <dst_y> <width> <height> <alpha>
# <alpha>: Hexadecimal alpha value (00-FF)
> dma2d blend RGB565 RGB565 RGB565 0x07E0 0xF800 320 240 160 120 320 240 0 0 80 60 0 0 160 120 FF
```

## Test Environment
- Development board: BK7258
- Compiler: ARM GCC
- Build system: CMake

## Compilation and Execution
1. Build the project using the provided Makefile or CMake
2. Flash the firmware to the development board
3. Use a serial terminal to access the command-line interface
4. Execute the DMA2D commands as described above

## Notes
- Ensure that the DMA2D controller is opened before performing any operations
- Close and delete the DMA2D controller when operations are complete
- The example uses frame buffers allocated from display memory
- All operations can be performed in synchronous mode