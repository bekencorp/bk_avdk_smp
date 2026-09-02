# lwIP PSRAM 示例工程

* [English](./README.md)

## 1. 项目概述

`lwip_psram_example` 演示在 BK7259 SMP 平台上使用 non-cacheable PSRAM
作为 lwIP 动态内存。启用该功能后，lwIP 的 `mem_malloc()` 最终通过
`psram_nocache_malloc()` 分配内存，从而降低网络业务对 AP SRAM 的占用，并为
大吞吐量、多连接等场景提供更大的网络缓冲空间。

工程同时启用了 Wi-Fi、CLI 和 iPerf，可通过 STA 联网和 iPerf 测试验证网络功能。
工程启动后不会自动连接 Wi-Fi 或运行 iPerf，需要通过串口 CLI 手动操作。

## 2. 硬件需求

- BK7259 开发板；
- 工程当前 RAM 分区按单颗 16 MB PSRAM 配置；
- 串口连接，用于烧录、查看日志和输入 CLI 命令；
- 可用的 2.4 GHz Wi-Fi 接入点；
- 与开发板处于同一局域网的 PC，用于运行 iPerf2（可选）。

## 3. 目录结构

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

## 4. 关键配置

工程已配置好所需选项。如果需要将该功能移植到其他工程，AP 侧必须同时使能：

```text
CONFIG_LWIP_MEM_LIBC_MALLOC=y
CONFIG_LWIP_MEM_LIBC_MALLOC_USE_PSRAM=y
CONFIG_CONTROLLER_AP_BUFFER_COPY=y
```

CP 侧必须使能：

```text
CONFIG_CONTROLLER_AP_BUFFER_COPY=y
```

`CONFIG_LWIP_MEM_LIBC_MALLOC_USE_PSRAM` 与
`CONFIG_CONTROLLER_AP_BUFFER_COPY` 是配套配置，缺少任意一项都会触发编译期
配置检查。

RAM 分区文件 `partitions/bk7259/ram_regions.csv` 中还必须提供大小非零的
`AP_PSRAM_NOCACHE_HEAP`。本工程配置为：

```text
AP_PSRAM_NOCACHE_HEAP,  PSRAM,  , 0x020000
```

即为 lwIP 预留 128 KB non-cacheable PSRAM。实际项目可根据并发连接数、TCP
窗口和收发峰值调整该值。增大该区域时，需要相应调整其他 PSRAM 分区，确保分区
不重叠且总大小不超过物理 PSRAM 容量。修改 RAM 分区后建议先执行 `make clean`。

## 5. 编译与烧录

在 SDK 根目录执行：

```bash
make bk7259 PROJECT=lwip_psram_example -j
```

编译成功后，完整固件位于：

```text
build/bk7259/lwip_psram_example/package/all-app.bin
```

使用 BKFIL 或 SDK 配套烧录工具，通过 UART0 将 `all-app.bin` 烧录到开发板。
该固件已经包含 bootloader、CP 和 AP 镜像。

## 6. 运行与验证

### 6.1 连接 Wi-Fi

烧录完成后复位开发板，在 AP 串口控制台执行：

```text
scan
sta <ssid> <password>
ip
```

将 `<ssid>` 和 `<password>` 替换为实际的 Wi-Fi 名称和密码。连接成功后，`ip`
命令应显示 STA 获取到的 IPv4 地址。

如果使用 CP 串口，需要通过 `ap_cmd` 将命令转发到 AP：

```text
ap_cmd scan
ap_cmd sta <ssid> <password>
ap_cmd ip
```

### 6.2 iPerf TCP 测试

设备与 PC 需要连接到同一局域网。本工程使用 iPerf2 协议，默认端口为 5001。

设备作为服务端时，在 AP CLI 执行：

```text
iperf -s
```

然后在 PC 上执行：

```text
iperf -c <device_ip> -t 30
```

PC 作为服务端时，先在 PC 上执行：

```text
iperf -s
```

然后在设备 AP CLI 执行：

```text
iperf -c <pc_ip> -t 30
```

使用 CP 串口时，设备侧命令需要添加 `ap_cmd` 前缀，例如：

```text
ap_cmd iperf -s
```

查看帮助或停止测试：

```text
iperf -h
iperf --stop
```

如需 UDP 测试，在设备 iPerf 命令中增加 `-u`。

## 7. 预期结果

- STA 成功连接接入点并获取有效 IPv4 地址；
- iPerf 客户端成功连接服务端，并周期性输出传输速率；
- 测试期间不出现 lwIP 分配失败、PSRAM 分配失败或异常复位。

## 8. 注意事项

- `AP_PSRAM_NOCACHE_HEAP` 过小可能导致连接失败、吞吐下降或丢包，应根据业务
  峰值调整分区大小；
- non-cacheable PSRAM 的访问性能低于 SRAM，本配置用于降低 SRAM 压力，并不
  保证所有场景下吞吐量都会提升；
- AP 网络流量增大时，CP 侧的 Wi-Fi 控制和数据转发开销也会增加。如果 CP 可用
  内存不足，可能导致网络吞吐下降；
- 修改 PSRAM 分区时必须检查相邻分区的地址和大小，避免重叠。
