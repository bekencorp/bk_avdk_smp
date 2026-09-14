# Ethernet Example Project

* [中文](./README_CN.md)

## Overview

`eth_example` is the BK7258 Ethernet example project. Starting from the default `app` project, it enables the Ethernet MAC and PHY and enlarges the LWIP buffers to match wired-network throughput.

- The AP enables the Ethernet MAC driver (`CONFIG_ETH=y`), paired with an SMSC PHY on pin group0.
- Hardware checksum offload and Ethernet power-management callbacks are enabled.
- The AP votes the CPU frequency to 480MHz on behalf of ENET at startup.
- LWIP memory and TCP windows are noticeably larger than in `app`, network packets are placed in PSRAM, and IPv6 is disabled by default.
- Sample code that polls and prints the Ethernet IP status is provided but not enabled.

## Project layout

- `ap/ap_main.c`: AP application entry, ENET frequency vote, and Ethernet status monitor sample
- `cp/cp_main.c`: CP application entry and CP1 startup
- `ap/config/bk7258_ap/config`: AP configuration with Ethernet enabled
- `cp/config/bk7258/config`: CP configuration
- `partitions/bk7258/auto_partitions.csv`: flash partition layout
- `partitions/bk7258/ram_regions.csv`: SRAM/PSRAM region layout

## Build

Run from the SDK root:

```text
make bk7258 PROJECT=eth_example
```

## Run

Connect the Ethernet cable, flash the generated AP and CP images, connect the serial consoles, and reset the board. Check the log for PHY link up and an assigned IP address, then use the enabled CLI commands such as ping and iperf to validate connectivity and throughput; commands targeting the AP need the `ap_cmd` prefix.

`eth_status_monitor()` in `ap/ap_main.c` prints the Ethernet IP, netmask, gateway, and DNS every 5 seconds. Its thread creation is commented out by default; uncomment it and rebuild to observe the status periodically:

```text
//rtos_create_thread(NULL, 1, "eth_mon", eth_status_monitor, 2048, NULL);
```

## Notes

- This project is configured for an SMSC PHY on pin group0. When changing the PHY model or pin group, update `CONFIG_PHY_*` and `CONFIG_ETH_PIN_GROUP*` accordingly and make sure the hardware matches.
- The LWIP buffer sizes and the `ram_regions.csv` split go together and should be evaluated at the same time.
- The 480MHz frequency vote for ENET affects power consumption and should be re-evaluated for low-power scenarios.
