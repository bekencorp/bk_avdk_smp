# AP Powerdown Low-Voltage Example Project

* [中文](./README_CN.md)

## Function Overview
- Demonstrates powering down the AP (CPU1/CPU2) when CP enters low-voltage sleep
- Does not boot the AP at startup by default, so AP-off low-voltage power can be measured
- Supports RTC and GPIO wakeup from low-voltage sleep
- Provides CLI commands to boot or shut down the AP

* For detailed information about power save, please refer to:

  - [Power Save Overview](../../../developer-guide/power_save/index.html)

* For Power Manager CLI reference, please refer to:

  - [Power Manager Example](../../../examples/platform/bk_pwr.html)

## Main Components
- `bk_pm`: power management component
- `pwr_clk`: power/clock control, including AP boot and powerdown
- `cli_pwr`: low-power CLI (`pm` / `pm_vote` / `pm_boot_cp1` / `pm_debug`)

## CLI Command Usage
The AP is not started by default. Run these commands on the **CP serial console**.

```bash
pm_boot_cp1 9 0     # Boot AP (module=9:APP, 0:power on)
pm_boot_cp1 9 1     # Shut down AP (1:power off)
pm 1 1 0 0 12 10000 0 1                 # Enter low voltage, RTC wake after 10s
pm 1 0 8 9 12 9 2 1000000000            # Enter low voltage, GPIO9 rising-edge wake
pm_debug 8          # Dump current power / low-voltage state
```

Parameter Description:

- `pm_boot_cp1 [module_name] [ctrl_state]`
  - `module_name`: `9` is `PM_BOOT_CP1_MODULE_NAME_APP`
  - `ctrl_state`: `0` boot up, `1` shut down
- `pm [sleep_mode] [wake_source] [vote1] [vote2] [vote3] [param1] [param2] [param3]`
  - `sleep_mode`: `0` normal sleep, `1` low voltage, `2` deep sleep
  - `wake_source`: `0` GPIO, `1` RTC, `2` Wi-Fi/BT, `4` Touch
  - `vote1/vote2/vote3`: `8` BT, `9` Wi-Fi, `12` APP
  - GPIO: `param1` is GPIO ID, `param2` is trigger (`0` low, `1` high, `2` rising, `3` falling)
  - RTC: `param1` is wakeup time in ms

## Log Output
- AP boot / shutdown status
- Low-voltage entry and wakeup-source callbacks
- Error messages

## Test Examples

### Test Environment
- Development board: Armino development board (BK7258)
- Serial: CP console for commands; AP console to confirm whether AP is running
- Optional: precision supply to measure low-voltage current and VDDDIG

### pm_ap_powerdown Firmware Compilation
- Compilation command:

```bash
make bk7258 PROJECT=pm_ap_powerdown
```

Flash the generated AP and CP images, then reset the board. By default CP does not vote to boot the AP, so the AP serial console has no startup log.

### Test CASE 1 - AP Boot Test
- CASE command:

```bash
pm_boot_cp1 9 0
```

- CASE expected result:

```
The AP is booted successfully
```

- CASE success log:

```
boot_cp1 9 0 0x0 [0][0x...]E_1
boot_cp1 9 0 0x200 [0]E_2
```
- CASE failure standard:

```
AP serial has no startup log; AP is not running
```
- CASE failure log:

```
cp0 boot cp1[...] time out, boot cp1 fail!!!
```

### Test CASE 2 - AP Powerdown Test
- CASE command:

```bash
pm_boot_cp1 9 1
```

- CASE expected result:

```
The AP is powered down while CP keeps running
```

- CASE success log:

```
Shutdown_cp1[0][0][0]
```
- CASE failure standard:

```
The AP is still running and is not powered down
```
- CASE failure log:

```
No Shutdown_cp1 log
```

### Test CASE 3 - RTC Wake from Low Voltage (AP Powered Down)
- CASE command:

```bash
pm 1 1 0 0 12 10000 0 1
```

- CASE expected result:

```
The system enters low voltage and wakes after about 10 s. The AP stays powered down during sleep.
```

- CASE success log:

```
cli_pm_cmd 1 1 0 0 12 10000 0!!!
cli_pm_rtc_callback[1]
```
- CASE failure standard:

```
The system does not enter low voltage, or it does not wake at the configured time
```
- CASE failure log:

```
lowvol1 0x1 0xFFFFEFFF 0xFFFFFFFF
```

Note: In `lowvol1`, the second value is the current vote mask and the third is the required mask. If they differ, some module has not voted and the system cannot enter low voltage. Use `pm_debug 8` for more detail.

### Test CASE 4 - GPIO Wake from Low Voltage (AP Powered Down)
- CASE command:

```bash
pm 1 0 8 9 12 9 2 1000000000
```

- CASE expected result:

```
The system enters low voltage and wakes on a GPIO9 rising edge. The AP stays powered down during sleep.
```

- CASE success log:

```
cli_pm_cmd 1 0 8 9 12 9 2!!!
cli_pm_gpio_callback[0]
```
- CASE failure standard:

```
The system does not enter low voltage, or GPIO does not wake it
```
- CASE failure log:

```
No cli_pm_gpio_callback log
```

GPIO wiring:
- Rising-edge wake: start low, then drive high
- Falling-edge wake: start high, then drive low

## Notes
- Default SMP projects keep the AP powered on (WFI) during low voltage. This project enables `CONFIG_PM_AP_POWERDOWN_WHEN_LV`, so the AP is powered down in low voltage and idle power is lower.
- `bk_pm_module_vote_boot_cp1_ctrl()` is commented out in `cp/cp_main.c`, so the AP is not started after reset. Run `pm_boot_cp1 9 0` when the AP is needed.
- Low-power commands are executed on the CP console. After the AP is up, prefix AP-side commands with `ap_cmd`.
- For lower power, disable Bluetooth before entering low voltage: `AT+BLEPOWER=0`.
- Stop application traffic before entering low voltage. If entry fails, run `pm_debug 8` on the CP console to check missing votes.
- This project differs from `ap_powerdown_keepalive`: it demonstrates AP powerdown during low-voltage sleep. The keepalive project additionally keeps a CP heartbeat and supports remote wakeup.