# Wi-Fi Bridge Project Overview

* [中文](./README_CN.md)

This project demonstrates Beken Wi-Fi bridge mode on BK7258 SMP: STA joins an upstream AP, then a bridged SoftAP is brought up so downlink stations share the same L2 domain through lwIP `br0`. Control the demo from the AP-core UART CLI.

## 1. Directory Layout
```
bridge/
├── CMakeLists.txt                 # Top-level CMake entry
├── Makefile                       # Make build entry
├── app.rst                        # Sphinx project note
├── ap/                            # AP-core application
│   ├── ap_main.c                  # AP entry; bridge is driven by CLI after init
│   └── config/                    # BK7258 AP-side configuration (CONFIG_BRIDGE=y)
├── cp/                            # CP-core application
│   ├── cp_main.c                  # CP entry; boots AP core
│   └── config/                    # BK7258 CP-side configuration (CONFIG_BRIDGE=y)
└── partitions/                    # Flash and RAM layout
```

## 2. Features
- STA uplink plus SoftAP downlink in bridge mode.
- Default SoftAP SSID is `<sta_ssid>_brr` when `bridge_ssid` is omitted.
- Optional password (`0` means open upstream AP) and `keep_sta` on close.
- Bring-up continues in the Wi-Fi driver after STA GOT_IP.
- Query link status with the `state` CLI.

## 3. Hardware & Configuration
- Hardware: BK7258 SMP board; UART0 for AP-core CLI.
- Software: enable `CONFIG_BRIDGE=y` on both AP (`ap/config/bk7258_ap/config`) and CP (`cp/config/bk7258/config`). The two sides must match.
- Upstream AP on the same channel plan as a normal STA connection.

## 4. Build & Flash
```
make bk7258 PROJECT=wifi/bridge
```

Flash the generated AP and CP images with the SDK flash tool, then reset the board.

## 5. Runtime Flow
1. Connect serial CLI to the AP core.
2. Start bridge (STA joins upstream AP, then SoftAP is created)::

       bridge open <upstream_ssid> <password>
       bridge open <upstream_ssid> <password> <bridge_softap_ssid>
       bridge open <upstream_ssid> 0 <bridge_softap_ssid>
       bridge open <upstream_ssid> <password> <bridge_softap_ssid> 1

3. Check status::

       state

4. Stop bridge::

       bridge close

5. Associate a phone or PC to the bridge SoftAP and ping through the upstream LAN.

## 6. Troubleshooting
- **`bridge open` fails immediately**: Confirm `CONFIG_BRIDGE=y` on both AP and CP, and that the firmware is this project image.
- **STA never gets IP / SoftAP never starts**: Check upstream SSID/password, RF environment, and logs from `bk_bridge_fsm` / bring-up thread.
- **`state` shows bridge down**: Wait for GOT_IP; if bring-up rolls back, inspect UART logs for the failing step.
- **Downlink clients cannot reach the WAN**: Confirm they joined the SoftAP SSID (default `<sta_ssid>_brr`) and that the upstream AP allows multiple STAs.
