# lwIP PSRAM Example

* [中文](./README_CN.md)

## 1. Overview

`lwip_psram_example` demonstrates how to use non-cacheable PSRAM for lwIP
dynamic memory on the BK7259 SMP platform. With this feature enabled, lwIP
`mem_malloc()` allocations are ultimately served by
`psram_nocache_malloc()`. This reduces AP SRAM usage by networking workloads
and provides more buffer space for high-throughput and multi-connection
scenarios.

The project also enables Wi-Fi, CLI, and iPerf. It does not connect to Wi-Fi or
start an iPerf test automatically; use the UART CLI after boot.

## 2. Hardware Requirements

- BK7259 development board;
- One 16 MB PSRAM device, as assumed by the current RAM layout;
- UART connection for flashing, logs, and CLI commands;
- Available 2.4 GHz Wi-Fi access point;
- A PC on the same LAN running iPerf2 for throughput tests (optional).

## 3. Project Structure

```text
lwip_psram_example/
├── README.md
├── README_CN.md
├── app.rst
├── CMakeLists.txt
├── Makefile
├── ap/
│   ├── ap_main.c
│   └── config/bk7259_ap/defconfig
├── cp/
│   ├── cp_main.c
│   └── config/bk7259/defconfig
└── partitions/bk7259/
    ├── auto_partitions.csv
    └── ram_regions.csv
```

## 4. Configuration

The required options are already enabled in this example. To use this feature
in another project, enable the following options on the AP:

```text
CONFIG_LWIP_MEM_LIBC_MALLOC=y
CONFIG_LWIP_MEM_LIBC_MALLOC_USE_PSRAM=y
CONFIG_CONTROLLER_AP_BUFFER_COPY=y
```

Enable the following option on the CP:

```text
CONFIG_CONTROLLER_AP_BUFFER_COPY=y
```

`CONFIG_LWIP_MEM_LIBC_MALLOC_USE_PSRAM` and
`CONFIG_CONTROLLER_AP_BUFFER_COPY` must be enabled together. The build-time
configuration checks reject an incomplete combination.

The RAM layout in `partitions/bk7259/ram_regions.csv` must also contain a
non-zero `AP_PSRAM_NOCACHE_HEAP`. This example uses:

```text
AP_PSRAM_NOCACHE_HEAP,  PSRAM,  , 0x020000
```

This reserves 128 KB of non-cacheable PSRAM for lwIP. Adjust the size according
to the connection count, TCP window sizes, and peak traffic. When enlarging
this region, resize the other PSRAM regions so that they do not overlap or
exceed the physical PSRAM capacity. Run `make clean` after changing the RAM
layout.

## 5. Build and Flash

Run the following command from the SDK root:

```bash
make bk7259 PROJECT=lwip_psram_example -j
```

The complete firmware image is generated at:

```text
build/bk7259/lwip_psram_example/package/all-app.bin
```

Flash `all-app.bin` to the board through UART0 using BKFIL or the SDK flashing
tool. The image includes the bootloader, CP, and AP firmware.

## 6. Run and Verify

### 6.1 Connect to Wi-Fi

After flashing, reset the board and run the following commands on the AP UART
console:

```text
scan
sta <ssid> <password>
ip
```

Replace `<ssid>` and `<password>` with the access point credentials. After the
connection succeeds, `ip` should show the IPv4 address assigned to the STA.

When using the CP UART console, forward AP commands with `ap_cmd`:

```text
ap_cmd scan
ap_cmd sta <ssid> <password>
ap_cmd ip
```

### 6.2 iPerf TCP Test

Connect the board and the PC to the same LAN. This project uses the iPerf2
protocol and port 5001 by default.

To run the board as the server, enter this command on the AP CLI:

```text
iperf -s
```

Then run this command on the PC:

```text
iperf -c <device_ip> -t 30
```

To run the PC as the server, first run:

```text
iperf -s
```

Then run this command on the AP CLI:

```text
iperf -c <pc_ip> -t 30
```

When using the CP UART, prefix board-side commands with `ap_cmd`, for example:

```text
ap_cmd iperf -s
```

Show the command help or stop the active test:

```text
iperf -h
iperf --stop
```

Add `-u` to the board-side iPerf command for a UDP test.

## 7. Expected Results

- The STA connects to the access point and obtains a valid IPv4 address;
- The iPerf client connects to the server and periodically reports throughput;
- No lwIP allocation failure, PSRAM allocation failure, or unexpected reset
  occurs during the test.

## 8. Notes

- An undersized `AP_PSRAM_NOCACHE_HEAP` can cause connection failures, reduced
  throughput, or packet loss. Size it for the peak workload;
- Non-cacheable PSRAM is slower than SRAM. This configuration reduces SRAM
  pressure but does not guarantee higher throughput in every workload;
- Higher AP network traffic also increases CP Wi-Fi control and forwarding
  overhead. Insufficient CP memory can reduce throughput;
- Check adjacent region addresses and sizes whenever the PSRAM layout changes.
