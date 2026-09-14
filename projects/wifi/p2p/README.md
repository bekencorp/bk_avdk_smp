# Wi-Fi P2P Project Overview

* [中文](./README_CN.md)

This project is the BK7258 SMP example for Wi-Fi Direct (P2P). It enables `CONFIG_P2P` on AP and CP so the AP-core CLI can start P2P, discover peers, negotiate GO/GC, and form a group. P2P concurrent operation with WLAN STA is not supported on this SMP branch.

Developer guide: `ap/docs/bk7258/zh_CN/developer-guide/wifi/bk_wifi_p2p.rst`.

## 1. Directory Layout
```
p2p/
├── CMakeLists.txt                 # Top-level CMake entry
├── Makefile                       # Make build entry
├── ap/                            # AP-core application
│   ├── ap_main.c                  # AP entry; P2P is driven by CLI after init
│   └── config/                    # BK7258 AP-side configuration (CONFIG_P2P=y)
├── cp/                            # CP-core application
│   ├── cp_main.c                  # CP entry; boots AP core
│   └── config/                    # BK7258 CP-side configuration (CONFIG_P2P, CONFIG_WPS)
└── partitions/                    # Flash and RAM layout
```

## 2. Features
- P2P Device, Group Owner (GO), and Group Client (GC).
- GO Negotiation with Intent `0` (force GC), `15` (force GO), or `1`–`14` (preference; default `7`).
- WPS Push Button provisioning and WPA2 data encryption.
- Device discovery on social channels; listen / find / connect / cancel / disable.
- Default device name is `BEKEN SMP_P2P` when SSID is omitted.

## 3. Hardware & Configuration
- Hardware: two BK7258 SMP boards (or one board plus a P2P-capable phone); UART0 for AP-core CLI.
- AP: `CONFIG_P2P=y` in `ap/config/bk7258_ap/config`.
- CP: `CONFIG_P2P=y` and `CONFIG_WPS=y` in `cp/config/bk7258/config` (`CONFIG_WPS` is required by P2P).
- Flash and RAM: use the default partitions in this project.
- When porting, register a P2P event callback (`demo_p2p_event_cb`) as described in the developer guide.

## 4. Build & Flash
```
make bk7258 PROJECT=wifi/p2p
```

Flash the generated AP and CP images with the SDK flash tool, then reset the board.

## 5. Runtime Flow
Operate on the **AP-core** UART. CLI names are `p2p ...` (not `ap_cmd p2p`).

1. Enable P2P::

       p2p enable
       p2p enable p2p_ssid
       p2p enable p2p_ssid 0          # force GC
       p2p enable p2p_ssid 15         # force GO

2. Discover peers, or enter listen::

       p2p find
       p2p listen

3. Connect to a found device (MAC is 12 hex digits, `:` optional)::

       p2p connect <mac> <method> <intent>

4. Stop discovery, cancel the current operation (including disconnect), or disable P2P::

       p2p stop_find
       p2p cancel
       p2p disable
       p2p help

Typical two-board case: both enable P2P; one uses Intent `15` (GO) and `p2p listen`, the other uses Intent `0` (GC) and `p2p find`, then connect and confirm GO/GC IP in logs.

## 6. Troubleshooting
- **`p2p` command missing**: Confirm this project is flashed and `CONFIG_P2P=y` on AP.
- **Enable fails or listen dies after GO connect/disconnect**: Check CP `CONFIG_WPS=y`; do not run STA and P2P together (no concurrent mode).
- **No DEVICE_FOUND in logs**: Both sides should `p2p enable` then `find`/`listen`; keep devices close and on social channels 1/6/11.
- **ioctl busy / ROC errors after reconnect**: See UART `wpa` / `wifid` logs; cancel and disable before the next enable if a previous session did not tear down cleanly.
