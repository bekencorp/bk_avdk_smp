# MCU LCD Example Project

* [中文](./README_CN.md)

## Function Overview
- Implements display module screen filling functionality
- Supports RGB565, RGB888 and YUYV data formats


* For detailed information about display, please refer to:

  - [Display Overview](../../../developer-guide/display/mcu_lcd_display.html)

* For API reference, please refer to:

  - [Display API](../../../api-reference/multimedia/bk_display.html)

## Main Components
- `bk_display`: display hardware component
- `lcd_st7796s`: screen component

## CLI Command Usage
```bash
lcd_display open   # Open LCD display
lcd_display close  # Close LCD display
```

Parameter Description:
- `none`: No parameters

## Log Output
- Device open/close status
- Error messages

## Test Examples

### Test Environment
- Development board: Armino development board
- Peripheral: st7796s LCD

### mcu_lcd_example Firmware Compilation
- Compilation command:

```bash
make bk7258 PROJECT=mcu_lcd_example
```

### Test CASE 1 - LCD Open and Display Test
- CASE command:

```bash
lcd_display open
```

- CASE expected result:
- CASE success standard:
  
```
The screen displays several random colors and then stabilizes with the last color filling the screen
```

- CASE success log:

```
ap0:mcu_ctlr:D(3381):display start, frame width, height 320, 480

ap0:media_se:D(104282):h264:0[0], dec:0[0], lcd:58[149], lcd_fps:3[8], lvgl:0[0]
ap0:media_se:D(104282):wifi:0[0, 0kbps, 0ms, 0-0], jpg:0KB[0Kbps], h264:0KB[0Kbps]

ap0:rgb_main:D(104805):bk_display_flush frame success!
```
- CASE failure standard:

```
lcd display nothing
```

### Test CASE 2 - LCD Close Test
- CASE command:

```bash
lcd_display close
```

- CASE expected result:
- CASE success standard:
  
```
The screen is black and the display is closed
```
- CASE success log:

```
ap0:rgb_main:D(104805):bk_display_close success!
```
