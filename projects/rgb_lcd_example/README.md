# RGB LCD Example Project

* [中文](./README_CN.md)

## Function Overview
- Implements display module screen filling functionality
- Supports RGB565, RGB888 and YUYV data formats


* For detailed information about display, please refer to:

  - [Display Overview](../../../developer-guide/display/rgb_lcd_display.html)

* For API reference, please refer to:

  - [Display API](../../../api-reference/multimedia/bk_display.html)

## Main Components
- `bk_display`: display hardware component
- `lcd_st7701sn`: screen component

## Project Structure

```
rgb_lcd_example/
├── ap/
│   ├── rgb_lcd/
│   │   ├── src/
│   │   │   └── rgb_lcd_cli.c        # CLI command-line interface implementation
│   │   └── include/
│   │       └── rgb_lcd_test.h       # Test interface header file
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

## Function Description

This example provides two test modes:

**1. Individual API Testing:**
- Test each Display API function separately
- Includes: init, open, flush, close, delete
- For learning and understanding each API's purpose

**2. Combined Function Testing:**
- Complete display flow test
- Full process from initialization to closing
- Continuous refresh of multiple random color frames

## Compilation and Execution

### Compilation Method

Build the project using the following command:

```bash
make bk7258 PROJECT=rgb_lcd_example
```

### Execution Method

After compilation, flash the generated firmware to the development board, and use the following commands via serial terminal:

Command execution success: `CMDRSP:OK`

Command execution failure: `CMDRSP:ERROR`

## CLI Command Usage
```bash
lcd_display open   # Open LCD display
lcd_display close  # Close LCD display
```

## Test Examples

### Test CASE 1 - LCD Open and Display Test
- CASE command:

```bash
lcd_display open
```

- CASE expected result:

```
The screen displays several random colors and then stabilizes with the last color filling the screen
```

- CASE success log:

```
ap0:media_se:D(104282):h264:0[0], dec:0[0], lcd:58[149], lcd_fps:3[8], lvgl:0[0]
ap0:media_se:D(104282):wifi:0[0, 0kbps, 0ms, 0-0], jpg:0KB[0Kbps], h264:0KB[0Kbps]

ap0:rgb_main:D(104805):bk_display_flush frame success!
CMDRSP:OK
```
- CASE failure standard:

```
lcd display nothing
```
- CASE failure log:

```
CMDRSP:ERROR
```

## Implementation Details

### Display Module Features

**Pixel Format Conversion:**
- Supports YUYV to RGB565/RGB888/RGB666
- Supports RGB565 to RGB888
- Supports RGB888 to RGB565
- Supports RGB565 to RGB666
- Supports RGB888 to RGB666
- Automatic conversion by setting frame->fmt

**Resolution Handling:**
- Auto-crops center region when source resolution > LCD resolution
- Source resolution must be >= LCD resolution
- Recommended: source resolution matches LCD resolution for best display

**RGB565 Format Notes:**
- Uses GPIO high bits: R3-R7, G2-G7, B3-B7
- 16-bit format: R(5bit) G(6bit) B(5bit)

### Memory Management

- All frame buffers allocated from display memory pool (`frame_buffer_display_malloc`)
- Auto-released via callback function `display_frame_free_cb`
- Frame release timing:
  - Multi-frame mode: Previous frame released when next frame is flushed
  - Single-frame mode: Released on close

### LCD Device Configuration

Example uses ST7701SN device (480x854), configuration includes:
- RGB timing parameters (hsync/vsync porch, pulse width)
- Clock frequency (LCD_30M)
- Data output edge (NEGEDGE_OUTPUT)
- Initialization command sequence

For adding other LCD devices, please refer to the LCD Device Addition Guide documentation.

### API Call Sequence

**Individual API Test Mode:**
1. `lcd_display_api init` - Create controller
2. `lcd_display_api open` - Open hardware
3. `lcd_display_api flush` - Refresh display
4. `lcd_display_api close` - Close hardware
5. `lcd_display_api delete` - Delete controller

**Combined Function Test Mode:**
- `lcd_display open` - Automatically completes init+open+flush flow
- `lcd_display close` - Automatically completes close+delete flow

## Notes

**Hardware Configuration:**
1. LCD LDO pin (default GPIO13) must be pulled high before opening display
2. Backlight pin (default GPIO7) needs to be configured as output and pulled high
3. Modify GPIO pin definitions if hardware configuration differs
4. SPI init pins configured based on LCD device requirements

**Display Troubleshooting:**

1. If screen doesn't light up, check:

   - Backlight IO enabled (GPIO7)
   - LCD LDO enabled (GPIO13)
   - LCD device configuration correct
   - Timing parameters match screen specs

2. If display is abnormal, check:

   - Source resolution >= LCD resolution
   - Pixel format configured correctly
   - Clock frequency appropriate

**Memory Management:**
- Frame free callback (free_cb) cannot be NULL
- Callback must properly release frame memory
- Manually release frame if flush fails

**Performance Optimization:**
- Choose appropriate pixel format (RGB565 uses less memory)
- Avoid unnecessary format conversions
- Keep controller open for continuous screen refresh
