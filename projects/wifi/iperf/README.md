# Wi-Fi iPerf Project Overview

* [中文](./README_CN.md)

This project is the BK7258 SMP combined Wi-Fi test example with STA / SoftAP / scan / iPerf CLI. Connect via STA or SoftAP first, then run iPerf throughput tests.

Developer guides: `ap/docs/bk7258/en/developer-guide/wifi/bk_wifi_iperf.rst`, `ap/docs/bk7258/en/developer-guide/wifi/bk_wifi_mode.rst`.

## 1. Directory Layout
```
iperf/
├── CMakeLists.txt                 # Top-level CMake entry
├── Makefile                       # Make build entry
├── app.rst                        # Sphinx project notes
├── ap/                            # AP-core application
│   ├── ap_main.c                  # AP entry; STA / SoftAP / iPerf are CLI-driven
│   └── config/                    # BK7258 AP-side configuration (CONFIG_IPERF_TEST=y)
├── cp/                            # CP-core application
│   ├── cp_main.c                  # CP entry; boots AP core
│   └── config/                    # BK7258 CP-side configuration
└── partitions/                    # Flash and RAM layout
```

## 2. Features
- STA, SoftAP, scan, and `state` query.
- iPerf TCP / UDP client and server modes.
- AP defaults: `CONFIG_IPERF_TEST=y`, `CONFIG_WIFI_SOFTAP=y`.

## 3. Hardware & Configuration
- Hardware: BK7258 SMP board; UART0 for AP-core CLI. Peer can be PC iPerf or another board.
- AP: `CONFIG_IPERF_TEST=y` in `ap/config/bk7258_ap/config`.
- Flash and RAM: use the default partitions in this project.

## 4. Build & Flash
```
make bk7258 PROJECT=wifi/iperf
```

Flash the generated AP and CP images with the SDK flash tool, then reset the board.

## 5. Runtime Flow
Operate on the **AP-core** UART. Connect to the network first, then run iPerf.

1. Join as STA, or start SoftAP:

       sta <ssid> [password]
       ap <ssid> [password]
       scan [ssid]
       state

2. iPerf (TCP by default; add `-u` for UDP):

       iperf -s                      # TCP server
       iperf -s -u                   # UDP server
       iperf -c <host>               # TCP client
       iperf -c <host> -u            # UDP client
       iperf --stop
       iperf -h

Typical flow: `sta` to a router, run `iperf -s` on a PC, then `iperf -c <PC_IP>` on the board.

## 6. Troubleshooting
- **`iperf` command missing**: Confirm this project is flashed and `CONFIG_IPERF_TEST=y` on AP.
- **Client cannot connect**: Check `state` for a valid IP; start the peer server first and keep both on the same subnet.
- **Low throughput**: Prefer a cleaner channel (5 GHz if available); for UDP, tune `-u` and the peer bandwidth options.
