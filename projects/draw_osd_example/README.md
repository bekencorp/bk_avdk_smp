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
- **frame_buffer**: Frame buffer management
- **lcd_display**: LCD display controller
- **cli**: Command-line interface for testing

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

## Test Environment
- Development board: BK7258
- LCD Panel: ST7701SN (480x864 resolution)
- Compiler: ARM GCC
- Build system: CMake

## Compilation and Execution
1. Build the project using the provided Makefile or CMake
2. Flash the firmware to the development board
3. Use a serial terminal to access the command-line interface
4. Execute the OSD commands as described above

## Notes
- Ensure that the OSD controller is initialized before performing any operations
- Deinitialize the OSD controller when operations are complete
- The example uses frame buffers allocated from display memory
- Supports RGB565 pixel format for display
- PSRAM usage can be configured during initialization