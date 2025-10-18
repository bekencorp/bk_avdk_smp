# RGB LCD Example Project

* [中文](./README_CN.md)

## Function Overview
- Implements display module screen filling functionality
- Supports RGB565 and RGB888 data formats


* For detailed information about display, please refer to:

  - [Display Overview](../../../developer-guide/display/qspi_lcd_display.html)

* For API reference, please refer to:

  - [Display API](../../../api-reference/multimedia/bk_display.html)

## Main Components
- `bk_display`: display component
- `lcd_qspi_st77903_h0165y008t`: screen component

## CLI Command Usage
```bash
qspi_lcd_display open   # Open LCD display
qspi_lcd_display close  # Close LCD display
```

Parameter Description:
- `none`: No parameters

## Log Output
- Device open/close status
- Error messages

## Test Examples

### Test Environment
- Development board: Armino development board
- Peripheral: st77903_h0165y008t QSPI LCD

### rgb_lcd_example Firmware Compilation
- Compilation command:

```bash
make bk7258 PROJECT=qspi_lcd_example
```

### Test CASE 1 - LCD Open and Display Test
- CASE command:

```bash
lcd_display open
```

- CASE expected result:

```
The screen displays red pure color
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

### Test CASE 2 - LCD Close Test
- CASE command:

```bash
lcd_display close
```

- CASE expected result:

```
The screen display is closed
```

- CASE success log:

```
bk_display_delete success!
CMDRSP:OK
```
- CASE failure standard:

```
the screen does not turn off the display
```
- CASE failure log:

```
CMDRSP:ERROR
```
