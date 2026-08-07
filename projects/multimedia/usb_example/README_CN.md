# USB 示例工程

* [English](./README.md)

## 1. 项目概述

`usb_example` 是 BK7259 平台的 USB device/host 双角色示例工程。工程上电后默认以 USB device MSC 方式枚举为 U 盘设备，同时提供 CLI 命令用于在运行时切换到 USB host 模式，验证外接 USB 设备枚举、U 盘读写、UVC 摄像头 MJPEG 接收，以及切换到 MTP device 模式浏览板端 SD 卡文件。

本示例包含：

- USB device MSC：上电默认启动的 U 盘设备模式。
- USB host 枚举：打印外接 USB 设备的标准描述符。
- USB host MSC：枚举外接 U 盘，挂载 FatFs `2:`，执行文件写入和读回校验。
- USB host UVC：枚举 UVC 摄像头，打开 MJPEG 流，接收并校验完整 JPEG 帧。
- USB device MTP：关闭默认 MSC gadget，挂载 SD 卡 `/sd0`，让 PC 以 MTP 方式浏览文件。
- CherryUSB v1.6 和 RISC-V USB bridge 相关路径验证。

## 2. 硬件需求

- SoC/开发板：BK7259 系列开发板。
- USB device 连接：USB device 口连接 PC，用于 MSC 或 MTP device 模式。
- USB host 连接：USB host 口连接 U 盘、UVC 摄像头或其他 USB 设备。
- USB host 供电：host 模式需要确认 host 口 VBUS 已正确供电。
- SD 卡：MTP 模式需要 SD 卡，文件系统挂载路径为 `/sd0`。
- 调试接口：串口控制台，用于输入 CLI 命令和查看日志。

注意：同一个 USB 控制器在同一时刻只能工作在一种角色下。切换到 host 模式后，默认 MSC device 连接不再作为 U 盘设备使用；启动 MTP 时也会先关闭默认 MSC gadget。

## 3. 目录结构

```text
usb_example/
├── CMakeLists.txt
├── Makefile
├── README.md
├── README_CN.md
├── app.rst
├── ap/
│   ├── ap_main.c      # USB MSC/MTP/U-disk CLI 和默认 MSC 启动逻辑
│   ├── uvc_test.c     # UVC 摄像头 MJPEG 接收测试
│   └── config/
├── cp/
│   ├── cp_main.c
│   └── config/
└── partitions/
```

## 4. 编译与烧录

在 SDK 根目录执行编译命令：

```bash
make bk7259 PROJECT=multimedia/usb_example -j
```

编译完成后，固件位于：

```text
build/bk7259/usb_example/package/all-app.bin
```

将 `all-app.bin` 烧录到开发板。烧录完成后复位开发板，并通过串口控制台输入命令。

AP 侧命令需要从主控制台使用 `ap_cmd` 前缀触发，例如：

```text
ap_cmd udisk status
ap_cmd mtp status
ap_cmd uvc test
```

## 5. USB Device MSC 模式

工程上电后会自动调用 `msc_storage_init()`，默认作为 USB MSC device 向 PC 枚举。PC 侧应识别到一个 U 盘设备。

查询当前状态：

```text
ap_cmd udisk status
```

默认状态日志通常包含：

```text
mode=device, driver_init=0, host_media=not_ready
```

如需查询 MTP 状态，请执行：

```text
ap_cmd mtp status
```

如果此前切换到了 host 模式，可执行以下命令切回 device 角色：

```text
ap_cmd udisk dev
```

如果已经启动 MTP，`ap_cmd mtp stop` 只停止 MTP gadget，不会重新启动上电默认的 MSC gadget。需要恢复默认 MSC device 时，请复位开发板。

## 6. USB Host 设备枚举

`udisk enum` 用于切换到 host 模式，等待外接 USB 设备枚举，并打印 device、configuration、interface 和 endpoint 描述符。该命令可用于快速确认 U 盘、UVC 摄像头或其他 USB 设备是否能被 host 控制器识别。

操作步骤：

1. 确认 USB host 口 VBUS 已供电。
2. 将 USB 设备接入 host 口。
3. 执行枚举命令。

```text
ap_cmd udisk enum
```

期望日志：

```text
==== USB host enumeration BEGIN ====
HOST mode active
VID:PID=....
interfaces=...
==== USB host enumeration PASS ====
```

## 7. USB Host U 盘读写测试

`udisk test` 会切换到 host 模式，等待 U 盘枚举，挂载 FatFs `2:`，写入测试文件并读回校验。

操作步骤：

1. 确认 host 口 VBUS 已供电。
2. 将 U 盘接入 host 口。
3. 执行测试命令。

```text
ap_cmd udisk test
```

测试流程：

1. 从 device 模式切换到 host 模式。
2. 等待 U 盘枚举和 MSC class 注册。
3. 挂载 FatFs `2:`。
4. 打印写入前文件列表。
5. 写入 `2:/bk_udisk_test.txt`。
6. 读回 512 bytes 并校验数据。
7. 打印写入后文件列表。
8. 卸载 `2:`。

期望日志：

```text
U-disk media READY
mounted 2:
---- U-disk file list (before write) ----
wrote 512 bytes -> 2:/bk_udisk_test.txt
read-back PASS
---- U-disk file list (after write) ----
==== U-disk host R/W test PASS ====
```

相关命令：

- `ap_cmd udisk host`：切换到 USB host 并等待 U 盘 ready。
- `ap_cmd udisk dev`：切回 USB device MSC。
- `ap_cmd udisk ls`：打印已挂载 U 盘 `2:` 根目录，主要用于调试已挂载状态。

## 8. USB Host U 盘测速

`udisk speed` 用于测量外接 U 盘的顺序写入和读取吞吐。它复用与 `udisk test` 相同的 host 切换和挂载流程，然后按固定块大小写入一个大文件（停止写入计时前会先 `f_sync`，让结果反映真正落盘的数据而非 FatFs 缓存），再重新打开文件读回、报告吞吐、删除测试文件并卸载。

默认参数：

- 总量：`32 MB`
- 块大小：`128 KB`
- 测试文件：`2:/bk_udisk_speed.bin`（测试结束后自动删除）

运行默认测试：

```text
ap_cmd udisk speed
```

指定总量（MB）和块大小（KB）：

```text
ap_cmd udisk speed 64 128
ap_cmd udisk speed 8 32
```

期望日志：

```text
==== U-disk speed test BEGIN (total=32 MB, block=128 KB) ====
U-disk media READY after 1400 ms
write: 32768 KB in 11569 ms -> 2832 KB/s (2.76 MB/s)
read : 32768 KB in 3545 ms -> 9243 KB/s (9.02 MB/s)
==== U-disk speed test PASS ====
```

注意事项：

- 总量至少取几 MB，让传输进入稳态；总量太小主要测到的是 FatFs/USB 命令开销和 U 盘内部缓存。
- 写吞吐受 U 盘闪存和 FAT 更新限制，读通常快得多。块大小超过 64~128 KB 后收益递减，因为瓶颈在闪存和 host 传输路径，而不是缓冲区大小。
- 测试文件以 `FA_CREATE_ALWAYS` 创建，请勿在存有重要数据的 U 盘上测试。

## 9. USB Device MTP 模式

> **MTP 默认关闭，使用前必须先在 defconfig 中启用。** 在 `ap/config/bk7259_ap/defconfig` 中该项默认是 `# CONFIG_USBD_MTP is not set`，因此 `mtp` CLI 命令不会编进默认固件。要使用 MTP，请在 defconfig 中设置 `CONFIG_USBD_MTP=y`（或通过 `menuconfig` 打开），然后重新编译并烧录。其它 USB 功能（MSC / host U 盘 / UVC）默认已开启，无需此步骤。

MTP 是 MSC 之外的另一种 USB device gadget。启动 MTP 时，工程会先关闭默认 MSC gadget，然后挂载 SD 卡 `/sd0`，并以 MTP 设备重新向 PC 枚举。

先在 `ap/config/bk7259_ap/defconfig` 中启用 MTP：

```text
CONFIG_USBD_MTP=y
```

重新编译并烧录后，启动 MTP：

```text
ap_cmd mtp start
```

查询 MTP 状态：

```text
ap_cmd mtp status
```

停止 MTP：

```text
ap_cmd mtp stop
```

期望日志：

```text
MTP: deinit MSC gadget
[mtp] mounted SD card at /sd0
MTP: usb_mtp_init ret=0
==== MTP device active (browse the SD card on the PC) ====
mtp_notify_handler:7
```

PC 侧现象：

- Windows：资源管理器中应出现便携设备，默认名称为 `BekenMTP`。
- Linux：可使用文件管理器的 MTP/GVFS 集成，或 `mtp-detect`、`mtp-files` 等工具。
- macOS：系统不原生支持 MTP，需要使用 Android File Transfer 类工具。

MTP 相关配置（`CONFIG_USBD_MTP` **默认关闭**，需按上文先打开）：

```text
CONFIG_USBD_MTP=y
CONFIG_USBD_MTP_PRODUCT_NAME="BekenMTP"
CONFIG_USBD_MTP_DEVICE_TYPE="1"
```

`CONFIG_USBD_MTP_DEVICE_TYPE` 对应 MTP `PerceivedDeviceType`，当前默认 `1` 表示 still image camera。

## 10. USB Host UVC 摄像头测试

`uvc` 命令用于验证 USB host UVC 摄像头 MJPEG 接收流程。命令会释放默认 MSC device gadget，切换 USB 控制器到 host 模式，枚举指定 port 上的 UVC 摄像头，打开 MJPEG 流并校验完整 JPEG 帧。

默认参数：

- host port：`1`
- 分辨率：`1920x1080`
- 帧率：`30fps`
- 自测目标：至少 20 帧完整 MJPEG

运行默认测试：

```text
ap_cmd uvc test
```

指定 port、分辨率和帧率：

```text
ap_cmd uvc test 1 1280 720 30
```

只打开并持续接收流：

```text
ap_cmd uvc open 1 1920 1080 30
```

关闭流：

```text
ap_cmd uvc close 1
```

期望日志：

```text
==== UVC MJPEG receive test BEGIN (port=1 1920x1080@30, target=20 frames) ====
host prepared (MSC gadget released, host class drivers registered)
camera ready on port 1 after ... ms
camera VID:PID=....
UVC opened: port=1 MJPEG 1920x1080@30
MJPEG frame #1 OK ...
MJPEG frame #20 OK ...
==== UVC MJPEG receive test PASS ====
```

如果摄像头不支持指定 fps，示例会回退到该分辨率下摄像头描述符报告的第一个 fps。如果摄像头不支持默认 `1920x1080@30`，请改用摄像头实际支持的 MJPEG 分辨率。

## 11. 命令速查

所有 AP 侧命令从主控制台执行时都需要添加 `ap_cmd` 前缀。

- `ap_cmd udisk status`：打印当前 USB 模式、driver init 状态和 host media ready 状态。
- `ap_cmd udisk host`：切换到 USB host，并等待 U 盘 ready。
- `ap_cmd udisk dev`：切回 USB device MSC gadget。
- `ap_cmd udisk enum`：切换到 host，等待任意 USB 设备枚举并打印描述符。
- `ap_cmd udisk test`：切换到 host，枚举 U 盘，挂载、列目录、写入、读回并校验。
- `ap_cmd udisk ls`：打印已挂载 U 盘 `2:` 根目录。
- `ap_cmd udisk speed [MB] [blockKB]`：顺序写/读吞吐测速（默认 32MB/128KB）。
- `ap_cmd mtp start`：关闭 MSC，启动 MTP device，并挂载 SD 卡 `/sd0`。
- `ap_cmd mtp stop`：停止 MTP device；该命令不会自动恢复默认 MSC device。
- `ap_cmd mtp status`：打印 MTP active 状态。
- `ap_cmd uvc test [port] [w] [h] [fps]`：打开 UVC MJPEG 流，接收并校验至少 20 帧完整 JPEG。
- `ap_cmd uvc open [port] [w] [h] [fps]`：打开 UVC MJPEG 流并持续运行。
- `ap_cmd uvc close [port]`：停止并关闭指定 port 的 UVC 流。

## 12. 关键配置

本示例依赖以下主要配置：

```text
CONFIG_USB=y
CONFIG_USB_DEVICE=y
CONFIG_USBD_MSC=y
CONFIG_USBD_MTP=y
CONFIG_USB_HOST=y
CONFIG_USB_HUB=y
CONFIG_USBH_MSC=y
CONFIG_USBH_UVC=y
CONFIG_USB_CAMERA=y
CONFIG_BK_USB_CHERRYUSB_V1_6=y
CONFIG_USB_RISCV_BRIDGE=y
CONFIG_SDCARD=y
CONFIG_FATFS=y
CONFIG_FATFS_SDCARD=y
```

## 13. 注意事项

1. USB device 和 USB host 不能同时使用同一个控制器。执行 host 或 MTP 相关命令会改变当前 USB 角色。
2. Host 模式依赖外部 VBUS 供电，供电异常会导致设备无法枚举。
3. `udisk test` 会在 U 盘根目录写入 `bk_udisk_test.txt`，请勿在存放重要数据的 U 盘上直接测试。
4. MTP 使用 SD 卡 `/sd0` 作为后端存储，启动前请确认 SD 卡已插入并可挂载。
5. `mtp stop` 只停止 MTP gadget。如需恢复上电默认 MSC device，请复位开发板。
6. Windows 可能按 VID、PID 和 Serial 缓存 MTP 名称和图标。修改产品名或设备类型后，可能需要卸载旧设备记录或更换 USB 口重新枚举。

## 14. 常见问题

### 命令提示找不到

请确认命令带有 `ap_cmd` 前缀，例如：

```text
ap_cmd udisk status
ap_cmd mtp status
```

### USB host 枚举失败

请检查 host 口 VBUS、USB 线材、设备连接和设备供电。可先执行 `ap_cmd udisk enum` 确认是否能枚举任意 USB 设备。

### U 盘读写测试失败

请确认 U 盘已接入 host 口，并且文件系统可被 FatFs 识别。若日志出现 `NO_FILESYSTEM`，请将 U 盘格式化为 FAT/FAT32 后重试。

### UVC 摄像头打开失败

请确认摄像头为 UVC 设备、port 参数正确，并使用摄像头支持的 MJPEG 分辨率和 fps。可先执行 `ap_cmd udisk enum` 查看设备是否能枚举。

### MTP 没有显示文件

请确认 SD 卡已插入，且日志中出现：

```text
[mtp] mounted SD card at /sd0
```

### MTP 名称或图标未刷新

Windows 可能缓存旧设备信息。可在设备管理器中卸载旧的便携设备记录，或更换 USB 口后重新插拔。
