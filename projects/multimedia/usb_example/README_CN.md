# USB Example（BK7259）

* [English](./README.md)

## 1. 概述

`usb_example` 用于验证 BK7259 USB device/host 双角色能力。工程默认上电作为 USB device MSC U 盘设备,也提供 CLI 命令在运行时切换为 USB host,枚举外接 USB 设备并对 U 盘做文件读写测试；同时支持切换为 MTP device,让 PC 通过 MTP 浏览板端 SD 卡文件。

当前示例覆盖:

- USB device MSC:上电默认 U 盘设备模式。
- USB host 枚举:枚举任意接入 USB 设备并打印描述符。
- USB host MSC:识别外接 U 盘,挂载 FatFs `2:`,打印文件列表,写入并读回校验测试文件。
- USB device MTP:从默认 MSC gadget 切换为 MTP gadget,挂载 SD 卡 `/sd0`,PC 侧以便携设备方式浏览文件。
- CherryUSB v1.6 + RISC-V USB bridge:host 和 device 底层可通过 RISC-V CP 桥处理。

## 2. 编译和烧录

本工程是 SDK 内部 project,请在 SDK 根目录编译,不要进入 project 目录直接 make。

```bash
cd <workspace>/bk_avdk_smp_dev_7259v2_bringup_25W4801
make bk7259 PROJECT=multimedia/usb_example -j32
```

主要产物:

```text
build/bk7259/usb_example/package/all-app.bin
build/bk7259/usb_example/package/app_pack.rbl
```

烧录 `all-app.bin` 到 BK7259 后,串口连接 CLI 控制台。当前 AP 侧命令需要从主控制台使用 `ap_cmd` 前缀触发,例如:

```text
ap_cmd udisk status
ap_cmd mtp start
```

## 3. 硬件连接

### Device 模式

将板子的 USB device 口通过数据线接到 PC。上电后默认启动 MSC U 盘 gadget。

### Host 模式

切换到 host 前,请确认:

- USB host 口已接外部 VBUS 供电。
- U 盘或 UVC 摄像头插到 host 口。
- 从默认 device 切到 host 后,原 PC device 连接不再作为 MSC/MTP 使用。

### MTP 存储

MTP 后端使用 SD 卡文件系统,启动前请确认 SD 卡已插好。启动时日志应出现:

```text
[mtp] mounted SD card at /sd0
```

## 4. USB Device:默认 U 盘模式

上电后工程会自动执行 `msc_storage_init()`,默认作为 USB MSC device 向 PC 枚举。PC 侧应看到一个 U 盘设备。

常用验证:

```text
ap_cmd udisk status
```

期望默认状态:

```text
mode=device
mtp active=0
```

如果之前切到 host 或 MTP,可恢复 device MSC:

```text
ap_cmd mtp stop
ap_cmd udisk dev
```

## 5. USB Host:枚举设备

`udisk enum` 会切换到 host,等待设备枚举,并打印标准 device/config/interface/endpoint 描述符。该命令适合验证 U 盘、UVC 摄像头等任意 USB 设备是否能枚举。

```text
ap_cmd udisk enum
```

期望关键日志:

```text
==== USB host enumeration BEGIN ====
HOST mode active
New high-speed device ...
VID:PID=....
interfaces=...
==== USB host enumeration PASS ====
```

## 6. USB Host:U 盘读写测试

插入 U 盘并确认 host VBUS 后执行:

```text
ap_cmd udisk test
```

测试流程:

1. 从 device 切换到 host。
2. 等待 U 盘枚举和 MSC class 注册。
3. 挂载 FatFs `2:`。
4. 打印写入前文件列表。
5. 写入 `2:/bk_udisk_test.txt`。
6. 读回 512 bytes 并校验。
7. 打印写入后文件列表。
8. 卸载 `2:`。

期望关键日志:

```text
U-disk media READY
mounted 2:
---- U-disk file list (before write) ----
wrote 512 bytes -> 2:/bk_udisk_test.txt
read-back PASS
---- U-disk file list (after write) ----
==== U-disk host R/W test PASS ====
```

只切换 host 并等待 U 盘 ready:

```text
ap_cmd udisk host
```

手动打印 U 盘根目录:

```text
ap_cmd udisk ls
```

切回 device MSC:

```text
ap_cmd udisk dev
```

## 7. USB Device:MTP 模式

MTP 是 MSC 之外的另一个 device gadget。启动 MTP 会先关闭默认 MSC gadget,然后挂载 SD 卡 `/sd0`,并以 MTP 设备重新向 PC 枚举。

启动 MTP:

```text
ap_cmd mtp start
```

查询状态:

```text
ap_cmd mtp status
```

停止 MTP:

```text
ap_cmd mtp stop
```

期望关键日志:

```text
MTP: deinit MSC gadget
[mtp] mounted SD card at /sd0
mtp_notify_handler:11
[bk_v1_6] usb device use riscv CP bridge path
MTP: usb_mtp_init ret=0
==== MTP device active (browse the SD card on the PC) ====
mtp_notify_handler:1
mtp_notify_handler:7
```

事件含义:

- `11`:USBD init
- `1`:USB bus reset
- `7`:configured,PC 已完成配置

PC 侧:

- Windows:资源管理器中应出现便携设备,名称默认 `BekenMTP`。
- Linux:可用文件管理器 MTP/GVFS,或 `mtp-detect`、`mtp-files` 等工具。
- macOS:系统不原生支持 MTP,需要 Android File Transfer 类工具。

MTP 相关配置:

```text
CONFIG_USBD_MTP=y
CONFIG_USBD_MTP_PRODUCT_NAME="BekenMTP"
CONFIG_USBD_MTP_DEVICE_TYPE="1"
```

`CONFIG_USBD_MTP_DEVICE_TYPE` 对应 MTP `PerceivedDeviceType`:

- `0`:generic
- `1`:still image camera,当前默认
- `2`:media player
- `3`:phone
- `4`:video camera
- `5`:PIM
- `6`:audio recorder

注意:Windows 可能按 VID/PID/Serial 缓存 MTP 名称和图标。修改名称或类型后,如果 PC 仍显示旧信息,请在设备管理器卸载旧的便携设备记录,或更换 USB 口/修改 serial 后重新枚举。

## 8. 命令速查

所有命令从主控制台发送时都使用 `ap_cmd` 前缀。

| 命令 | 说明 |
| --- | --- |
| `ap_cmd udisk status` | 打印当前 USB 模式、driver init 状态、host media ready 状态 |
| `ap_cmd udisk host` | 切到 USB host,等待 U 盘 ready |
| `ap_cmd udisk dev` | 切回 USB device MSC gadget |
| `ap_cmd udisk enum` | 切到 host,等待任意 USB 设备枚举并打印描述符 |
| `ap_cmd udisk test` | 切到 host,枚举 U 盘,挂载、列目录、写入、读回校验 |
| `ap_cmd udisk ls` | 打印已挂载 U 盘 `2:` 根目录 |
| `ap_cmd mtp start` | 关闭 MSC,启动 MTP device,挂载 SD `/sd0` |
| `ap_cmd mtp stop` | 停止 MTP device |
| `ap_cmd mtp status` | 打印 MTP active 状态 |

## 9. 常见问题

### 命令找不到

请确认带了 `ap_cmd` 前缀:

```text
ap_cmd udisk status
ap_cmd mtp status
```

### U 盘 host 枚举失败

检查 host 口 VBUS、U 盘是否插稳、线材是否为数据线。可先用 `ap_cmd udisk enum` 看是否有任意 USB 设备枚举。

### MTP 没有文件

检查 SD 卡是否插好,日志是否有:

```text
[mtp] mounted SD card at /sd0
```

### MTP 名字或图标没刷新

Windows 可能缓存旧设备信息。卸载旧的便携设备记录、换 USB 口或修改 serial/PID 后重新插拔。
