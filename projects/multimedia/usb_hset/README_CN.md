# USB HSET CDC 示例工程

* [English](./README.md)

## 1. 项目概述

`usb_hset` 是 BK7259 平台面向 USBHSET / USB HS Electrical Test Tool 的 USB 2.0 device 合规测试示例工程。工程上电后自动以 USB 高速（High-Speed）CDC-ACM 从设备枚举，用于配合合规测试主机触发 USB 2.0 高速测试模式，进行信号质量 / 眼图合规测试。

工作原理：

- 上电后 AP 侧启动一个 HS CDC-ACM 从设备（`bk_usb_hset_cdc_device_init()`）。
- PC/合规主机枚举该设备后，可下发标准请求 `SET_FEATURE(TEST_MODE)`。
- CherryUSB device core 在控制传输 status 阶段完成后，调用 SDK MHDRC 端口中的 `usbd_execute_test_mode()`，写入 MUSB `TESTMODE` 寄存器，进入 `Test_J` / `Test_K` / `Test_SE0_NAK` / `Test_Packet` 之一。
- 用示波器抓取 D+/D- 差分信号，叠加 USB-IF HS 眼图模板即可完成测试。

## 2. 硬件需求

- SoC/开发板：BK7259 系列开发板。
- USB 连接：USB device 口（MHDRC / 高速 PHY）连接到 PC 或合规测试主机。
- 示波器：用于抓取 D+/D- 差分信号做眼图 / 信号质量测量。
- 调试接口：串口控制台，用于查看枚举与测试日志。

## 3. 目录结构

```text
usb_hset/
├── CMakeLists.txt
├── Makefile
├── README.md
├── README_CN.md
├── ap/
│   ├── ap_main.c        # 上电启动 USB HSET CDC 设备
│   ├── usb_hset_cdc.c   # HS CDC-ACM 从设备的描述符与枚举业务逻辑
│   └── config/
├── cp/
│   ├── cp_main.c
│   └── config/
└── partitions/
```

说明：进入测试模式所需的底层能力 `usbd_execute_test_mode()` 位于 SDK 内
（`ap/components/bk_usb/CherryUSB_v1_6/port/beken_musb/usb_dc_beken_musb_mhdrc.c`），
由 `CONFIG_USB_HSET` 开关控制；CDC 枚举的业务逻辑（描述符、类接口、`usbd_initialize`）位于本工程 `ap/usb_hset_cdc.c`。

## 4. 编译与烧录

在 SDK 根目录执行编译命令：

```bash
make bk7259 PROJECT=multimedia/usb_hset -j
```

编译完成后，固件位于：

```text
build/bk7259/usb_hset/package/all-app.bin
```

将 `all-app.bin` 烧录到开发板并复位。

## 5. 使用方法

1. 烧录固件并复位开发板。
2. 用 USB 线把开发板的 USB device 口接到 PC / 合规测试主机。
3. PC 应枚举出一个高速 CDC 设备，产品名为 `BK7259 USB HSET CDC`（Windows 下同时出现一个虚拟串口 COMx，Linux 下为 `/dev/ttyACM0`）。
4. 串口日志可见：
   ```text
   [usb-hset] HS CDC device up; waiting for host SET_FEATURE(TEST_MODE)
   [usb-hset] device CONFIGURED (HS)
   ```
5. 在合规测试主机上选中该设备所在的根端口，选择 `TEST_PACKET`（眼图）或 `Test_J` / `Test_K` / `Test_SE0_NAK`，工具会下发 `SET_FEATURE(TEST_MODE)`。
6. 用示波器抓取 D+/D- 波形并叠加 HS 眼图模板完成测量。

测试选择子（test selector）与 MUSB `TESTMODE` 寄存器位对应关系：

| Selector (wIndex 高字节) | 测试模式 | TESTMODE 写入值 |
| --- | --- | --- |
| 1 | Test_J | 0x02 |
| 2 | Test_K | 0x04 |
| 3 | Test_SE0_NAK | 0x01 |
| 4 | Test_Packet（眼图） | 0x08（先向 EP0 FIFO 载入 53 字节标准测试包，再置 TxPktRdy） |

## 6. 关键配置

```text
CONFIG_USB=y
CONFIG_BK_USB_CHERRYUSB_V1_6=y
CONFIG_USB_DEVICE=y
CONFIG_USB_HSET=y
```

- `CONFIG_USB_HSET=y` 会令 SDK 定义 `CONFIG_USBDEV_TEST_MODE`（打开 CherryUSB device core 对 `SET_FEATURE(TEST_MODE)` 的分发路径），编译 MHDRC 端口中的 `usbd_execute_test_mode()`，并编译设备侧 CDC-ACM 类驱动 `usbd_cdc_acm.c`。
- device 构建会无条件强制 `CONFIG_USB_HS`，因此设备以高速枚举（眼图测试前提）。
- `CONFIG_USB_HOST` 保持默认开启：本工程运行时不使用 host，但设备侧 PHY / 时钟 bring-up 及 RISC-V USB bridge 的辅助函数在本 SDK 中位于 host 控制器文件内，device 链接需要它们。

## 7. 注意事项

1. 眼图 / 测试模式仅在 USB 高速（HS）下有意义，请确认 PC 端将设备枚举为 High-Speed（可用 USB Tree View / `lsusb -t` 确认 `480M`）。
2. 本工程不实现 host 相关任何功能；如需 host 请使用 `usb_example`。
3. VID/PID 默认为占位值 `0xFFFF/0xFFFF`，不影响眼图测试；如需正式设备名可自行修改 `ap/usb_hset_cdc.c` 中的描述符。
