# rlk_demo Project Overview

* [中文](./README_CN.md)

This project is the BK7258 SMP BK-RLK (Raw Link) demo. On boot the AP core initializes RLK, picks a random channel, and registers CLI commands for pairing, ping, and throughput tests.

Developer guide: `ap/docs/bk7258/en/developer-guide/wifi/bk_rlk.rst`.

## 1. Directory Layout
```
rlk_demo/
├── CMakeLists.txt                 # Top-level CMake entry
├── Makefile                       # Make build entry
├── app.rst                        # Sphinx project notes
├── ap/                            # AP-core application
│   ├── ap_main.c                  # AP entry; initializes RLK and registers CLI
│   ├── include/                   # RLK demo / ping / iperf headers
│   ├── src/                       # rlk_demo_cli, rlk_ping, rlk_iperf
│   └── config/                    # BK7258 AP-side configuration
├── cp/                            # CP-core application
│   ├── cp_main.c                  # CP entry; boots AP core
│   └── config/                    # BK7258 CP-side configuration
└── partitions/                    # Flash and RAM layout
```

## 2. Features
- Calls `bk_rlk_init()` at boot, selects a random channel `1`–`13`, and registers the receive callback.
- `rlk_ping`: Raw Link ping by peer MAC.
- `rlk_iperf`: Raw Link client / server throughput test.
- Roles and channel: `rlk_master`, `rlk_slave`, `rlk_dubid`, `rlk_chan`, `rlk_scan`, `rlk_acs`.

## 3. Hardware & Configuration
- Hardware: two BK7258 SMP boards (or one board plus an RLK-capable peer); UART0 for AP-core CLI.
- Both sides must use the same channel before traffic; set it with `rlk_chan` instead of relying on the random boot channel.
- Flash and RAM: use the default partitions in this project.

## 4. Build & Flash
```
make bk7258 PROJECT=wifi/rlk_demo
```

Flash the generated AP and CP images with the SDK flash tool, then reset the board.

## 5. Runtime Flow
Operate on the **AP-core** UART. Align the channel on both sides, then ping or run iPerf.

1. Set the same channel on both boards (for example 6):

       rlk_chan 6

2. Optional role / scan:

       rlk_master
       rlk_slave role
       rlk_slave ssid <ssid>
       rlk_slave bssid <12 hex digits, no colons>
       rlk_scan
       rlk_acs

3. Ping the peer (full 6-byte MAC, or last 3 bytes with OUI filled as `c8:47:8c`):

       rlk_ping <mac>
       rlk_ping <mac> -c 4 -i 1 -s 32 -t 1
       rlk_ping --stop

4. Raw Link iPerf (one server, one client):

       rlk_iperf -s -i 1
       rlk_iperf -c <peer_mac> -i 1 -t 60 -b 20M
       rlk_iperf --stop
       rlk_iperf -h

Typical two-board case: both run `rlk_chan` to the same channel; one runs `rlk_iperf -s`, the other runs `rlk_iperf -c <peer_MAC>`.

## 6. Troubleshooting
- **`rlk_ping` / `rlk_iperf` missing**: Confirm this project is flashed.
- **No ping / iPerf response**: Channels must match; the client MAC must be the peer STA MAC.
- **Channels differ after boot**: `ap_main` picks a random channel; always run `rlk_chan` on both sides before a test.
- **Client stays disconnected**: Start `rlk_iperf -s` on the peer first, then `-c`; use `--stop` and retry if a previous session did not tear down.
