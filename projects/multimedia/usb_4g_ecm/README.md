# USB 4G ECM Networking

* [中文](./README_CN.md)

## 1. Overview

`usb_4g_ecm` makes the BK7259 a USB host for a 4G composite device (VID/PID `3505:0003`) and runs two channels over it at once:

- **CDC-ECM network interface**: intf0/intf1 bind CherryUSB's `usbh_cdc_ecm` driver, wired to lwIP on the AP core, so applications use plain sockets.
- **Vendor AT channel**: intf3 (class `0xFF` / subclass `0x00` / protocol `0x03`) binds a class driver that lives in this project, carrying AT traffic alongside the network.

Both channels share one USB controller and a single enumeration; neither takes the device exclusively.

**Out of scope**: PPP/RNDIS switching, a general multi-module framework, automatic Wi-Fi/4G route selection, NAT forwarding, low power, voice calls, OTA. The AT layer covers serialised commands, response parsing, timeout, URCs and cancellation on disconnect - it is not a script engine.

## 2. Hardware Requirements

- SoC/board: BK7259 series development board.
- One USB cable from the board's USB host port to the 4G module.
- The host port must supply VBUS to the module (`CONFIG_USB_VBAT_CONTROL_GPIO_ID=0x20`).
- A working SIM in the module, which must ship in ECM mode.
- Serial console for CLI and logs.

## 3. Project Structure

```text
usb_4g_ecm/
├── ap/
│   ├── ap_main.c        # Startup order: lwIP -> AT layer -> USB host
│   ├── usb_modem_at.c   # intf3 USB transport (claim, rx thread, serialised write)
│   ├── modem_at.c       # AT manager (line assembly, serialisation, result codes, URCs)
│   ├── modem_at_cli.c   # Test entry: the modem_at command
│   └── config/bk7259_ap/defconfig
├── cp/                  # The CP core takes no part in USB
└── partitions/
```

Layer boundary: `usb_modem_at.c` owns only the USB side - claiming the interface, pushing raw bytes up, serialising bulk writes - and parses no AT at all. Line assembly and OK/ERROR/URC decisions live in `modem_at.c`.

## 4. Build

```bash
cd bk_avdk_smp
make bk7259 PROJECT=multimedia/usb_4g_ecm
```

Output: `build/bk7259/usb_4g_ecm/package/all-app.bin`.

After editing `defconfig`, drop the old build directory first, or the stale config will override the lines you just added:

```bash
rm -rf build/bk7259/usb_4g_ecm
```

## 5. Startup Order

The init task in `ap_main.c` runs a fixed sequence, and the order itself matters:

1. `net_ipv4stack_init()` - the ECM driver hands received packets straight to `netif->input`, so the tcpip thread and its mbox must exist first.
2. `modem_at_init()` - installs the intf3 transport callbacks, which the class driver's connect path invokes, so it must precede enumeration.
3. Register the module-config hook and the one-shot connectivity check, then `bk_usb_open(USB_HOST_MODE)`. `bk_init()` has already registered the class drivers; the controller powers up as an A-host and enumeration begins.

## 6. AT Channel Test Commands

```text
modem_at                   # show link state
modem_at probe             # run read-only identity, SIM, signal, registration, attach, PDP and UE-state queries
modem_at AT+CGDCONT?       # send any command (the console splits on spaces; it is rejoined internally)
```

From the CP prompt, prefix `ap_cmd`. The log prints the command, the response text, then `OK`, the error line, `timeout`, `offline`, `write failed`, or `busy`. Quote commands that contain a comma, for example `ap_cmd modem_at "AT+CFUN=1,1"`, because the AP shell splits on commas. The numbers below are `modem_at_send()` return codes and are not printed:

The probe also runs `AT+CESQ`, `AT+CEREG?`, `AT+CGATT?`, `AT+CGCONTRDP=1`,
`AT+CGPADDR=1`, and `AT+ECSTATUS`, matching the Lierda LTE Standard AT manual.
These commands are read-only: they do not change APN, registration policy, or PDP state.

| Code | Meaning |
| --- | --- |
| `0` | The module answered `OK` |
| `-1` | Module error (`ERROR` / `+CME ERROR` / `+CMS ERROR`); the error line is still in the response |
| `-2` | Timed out waiting for the result code |
| `-3` | Channel offline, or unplugged mid-wait |
| `-4` | USB write failed |
| `-5` | Another command is still in flight (one at a time) |

## 7. Implementation Notes

**Response attribution**: while a command is in flight, incoming lines belong to it; with no command in flight, an incoming line is a URC. The `+` prefix is never used to decide this - `+CSQ: 20,99` is a response when we asked and a URC when we did not, so keying on the prefix gets one of the two cases wrong.

**Late answers after a timeout**: a timed-out command enters a DRAIN state that swallows late output (bounded at 300 ms) so it cannot be read as the next command's answer.

**Endpoint discovery**: intf3's bulk IN/OUT are parsed from the `altsetting[0]` descriptors by direction bit, never hardcoded. The interrupt IN on that interface is logged but left unsubmitted - the AT channel needs no notifications, which saves a pipe slot and an idle urb.

**Unplug cleanup**: the disconnect path clears the online flag and the rx thread's running flag first (both must happen before killing the urbs), then kills the urbs, and only memsets the instance and releases the hubport once the rx thread has confirmed it exited.

## 8. Key Configuration

| Option | Value | Why |
| --- | --- | --- |
| `CONFIG_USBH_CDC_ECM` | `y` | The ECM class driver ships with the SDK but is not built by default |
| `CONFIG_USB_HOST_FIFO_LAYOUT_4BULK` | `y` | Two bulk endpoints for ECM plus two for AT |
| `CONFIG_LWIP_TCPIP_THREAD_STACKSIZE` | `1024` | The default 512 is too small for the ECM netif input path |
| `CONFIG_USB_RISCV_BRIDGE` | `y` | USB on the BK7259 goes through the RISC-V bridge |

## 9. Resource Usage

Current firmware (AP core, from the `make` summary): FLASH 484236 B (23.18%), RAM 175492 B (44.63%), IRAM 1432 B, DTCM 2304 B.

The AT channel's own share: a 512 B rx buffer and a 256 B tx buffer (both in `USB_NOCACHE_RAM_SECTION`), a 2048 B rx thread stack, a 257 B line buffer, a 1280 B manager response buffer, and a 257 B CLI print buffer.

## 10. Known Limitations

- Supports only the `3505:0003` module, and relies on it shipping in ECM mode; another module needs changes to the match table and the endpoint assumptions.
- intf3 matching depends on protocol `0x03`. The same module's intf2 (`0x04`) and intf4 (`0x05`) are diagnostic ports, and that one field is what separates them; if the module renumbers protocols, matching breaks.
- Single instance: one AT interface is claimed at a time.
- One AT command at a time, with no queue - a second caller gets `-5` immediately.
- This module's factory ECM configuration carries no string descriptors, so the MAC has to come from elsewhere.

## 11. Verification Status

**Verified**: the build passes (`EXIT=0`, no warnings from the new files); the class registration record does land in the `usbh_class_info` section (three entries: hub, cdc_ecm, modem_at); `modem_at.c`'s byte-stream handling passes ten cases on the build machine (`make -C test/host run`, see `test/host/test_modem_at.c`) — reassembly across packets, several lines in one packet, echo filtering, `+CME ERROR` as a failure, URCs not waking the caller, a late OK after a timeout not counting as the next command's success, over-long lines dropped whole, re-entry refused, link loss during a wait cancelling the command.

**Board record, 2026-10-22** (Lierda NT26-FCN, `3505:0003`, NAT `192.168.10.100`): 100 consecutive `AT+CSQ` commands returned `OK` with no crossed answers. DHCP gave `192.168.10.100/24`, gateway `192.168.10.1`, DNS `192.168.10.2` and `192.168.10.3`. One lookup of `www.qq.com` and one short TCP returned a payload starting with `HTTP/`. The module stayed in ECM with `nat=1` and auto-dial `AT+ECNETDEVCTL=3,1,1`; the existing APN was left as-is. This firmware rejects `AT+LRST`. After `AT+CFUN=1,1`, USB and AT came back and the dial state returned to 1, but DHCP stayed in `DISCOVER` until the BK7259 itself rebooted, which then got the address in about 2 seconds.

**Still open**: 10 unplug cycles, 30 minutes of bidirectional TCP with AT, a 2 hour soak, and the cause of DHCP not recovering across a hot reconnect. The host harness is single-threaded and cannot construct the race where a result code lands between the wait expiring and DRAIN being published.
