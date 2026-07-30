# 二维码识别示例工程

* [English](./README.md)

## 1. 项目概述

本项目演示 Beken 平台上的摄像头二维码识别流程。工程从 ISP 获取 `640x360` NV12 图像，使用 ZBar 解码 Y 分量中的二维码，并通过串口打印二维码位置、内容和识别耗时；也可同时通过 GPU 和 MIPI 屏预览摄像头画面。

当前工程提供：

- SP 识别、MP 屏幕预览的完整模式
- 不启动 GPU 和显示的 MP-only 识别模式
- 可选的二维码 Wi-Fi 配网
- 单帧最多 4 个二维码识别结果
- 二维码四角坐标、payload 和扫描耗时日志

### 1.1 测试环境

- 核心板：`BK7259_QF128_12.3X12.3_V4.0`
- PSRAM：32 MB
- 摄像头：CSI GC2053，默认 `1280x720@30fps`
- 显示屏：MIPI DSI ER68576B，`720x1280`
- 识别输入：`640x360` NV12，ZBar 使用 Y 分量
- 图形模块：VG-Lite GPU、DPU/MIPI

> 请优先使用参考外设。更换 sensor、屏幕或 GPIO 后，需要同步修改 `ap_main.c` 中的板级配置和工程 Kconfig。

## 2. 目录结构

```text
qr_example/
├── .ci                         # CI 编译命令
├── app.rst                     # 文档框架文件
├── CMakeLists.txt              # 项目级 CMake 配置
├── Makefile                    # Make 构建入口
├── README.md                   # 英文说明
├── README_CN.md                # 中文说明
├── ap/
│   ├── ap_main.c               # AP 初始化和板级配置
│   ├── CMakeLists.txt
│   ├── config/                 # AP 默认配置
│   ├── include/
│   └── src/qr_demo.c           # ZBar 识别和 CLI 实现
├── cp/                         # CP 启动及校准代码
└── partitions/                 # 分区和 RAM 区域配置
```

## 3. 功能说明

### 3.1 `start` 预览识别模式

1. 启动 MIPI sensor 和 ISP MP 通道
2. 新建 `640x360` NV12 SP 通道用于二维码识别
3. 启动 MIPI 显示、GPU 和 ISP-GPU bond，显示 MP 预览
4. 从 SP 帧读取连续图像，取 NV12 的 Y 分量送入 ZBar
5. 每次扫描后间隔 100 ms
6. 识别成功后打印数量、四角坐标、payload 和耗时

### 3.2 `start_mp` MP-only 模式

该模式临时将 ISP MP 配置为 `640x360` NV12，直接使用 MP 图像识别二维码，不启动 SP、GPU 和显示。执行 `stop` 后恢复原有摄像头配置。

### 3.3 识别参数

- 输入尺寸：`640x360`
- 输入格式：NV12，识别器使用前 `640x360` 字节的 Y 分量
- 单帧最大结果数：4
- 单个 payload 缓冲区：8,896 字节
- 扫描间隔：100 ms

### 3.4 可选二维码 Wi-Fi 配网

`start wifi` 和 `start_mp wifi` 会保持二维码识别持续运行，并额外把识别到的 payload 按 Wi-Fi 配网数据解析。合法 payload 必须是只包含以下三个字符串字段的 JSON object：

```json
{"p":"12345678","s":"test_wifi","t":"1234512345"}
```

- `s`：Wi-Fi SSID，长度 1 到 32 字节
- `p`：Wi-Fi 密码，允许为空；非空时长度必须为 8 到 63 字节
- `t`：token/筛选字段，目前只要求存在

一次合法 payload 触发 Wi-Fi 连接后，后续扫码仍会继续打印识别结果；但本地 QR Wi-Fi 状态处于连接中、已连接或已获取 IP 时，不会再次控制 Wi-Fi start。首次扫描未找到 AP（`WIFI_REASON_NO_AP_FOUND`）会保持 `CONNECTING`，因为 Wi-Fi 栈后续可能继续全信道重试。收到 `EVENT_NETIF_GOT_IP4` 后，Demo 会记录并打印分配到的 IP，并将状态切到 `GOT_IP`。

## 4. 编译与运行

### 4.1 编译

在 SDK 根目录执行：

```bash
make bk7259 PROJECT=multimedia/qr_example
```

### 4.2 CLI 命令

固件烧录并启动后，通过串口终端执行：

```text
ap_cmd qr help
ap_cmd qr start
ap_cmd qr start wifi
ap_cmd qr start_mp
ap_cmd qr start_mp wifi
ap_cmd qr stop
ap_cmd qr stop_mp
ap_cmd qr wifi status
ap_cmd qr wifi disconnect
```

- `start`：启动 SP 二维码识别和 MP 屏幕预览
- `start wifi`：启动 SP 识别，并开启二维码 Wi-Fi 配网
- `start_mp`：启动 MP-only 二维码识别，不开启显示
- `start_mp wifi`：启动 MP-only 识别，并开启二维码 Wi-Fi 配网
- `stop`：停止识别并释放 camera、ZBar、GPU、display 和 frame buffer 资源
- `stop_mp`：MP-only 停止别名，释放与 `stop` 相同的二维码识别资源
- `wifi status`：打印本地 QR Wi-Fi 状态、当前 STA 链路状态、RSSI 和已记录的 IP
- `wifi disconnect`：停止 Wi-Fi STA，并重置 QR Wi-Fi 状态
- `CMDRSP:OK` 表示命令执行成功，`CMDRSP:ERROR` 表示参数错误或资源启动失败

## 5. 测试示例

### 5.1 带预览的二维码识别

```text
ap_cmd qr start
```

将二维码置于摄像头视野中，预期日志类似：

```text
frame=... found 1 QR code(s), scan=... us
QR[0] corners=(...,...),(...,...),(...,...),(...,...)
QR[0] len=... data=...
```

测试结束后执行：

```text
ap_cmd qr stop
```

### 5.2 MP-only 识别

```text
ap_cmd qr start_mp
```

该模式无屏幕预览，识别结果仍通过串口输出。结束时执行 `ap_cmd qr stop`。

### 5.3 Wi-Fi 配网

```text
ap_cmd qr start wifi
```

将包含 JSON 配网 payload 的二维码置于摄像头视野中。Demo 会继续打印所有二维码识别结果，只有 payload 通过 `p/s/t` 格式和长度检查后才会启动 Wi-Fi。可用 `ap_cmd qr wifi status` 查看本地配网状态、Wi-Fi STA 链路状态、RSSI 和 IP，或用 `ap_cmd qr wifi disconnect` 停止 STA 连接。

预期 Wi-Fi 配网日志包括：

```text
QR Wi-Fi state: IDLE -> CONNECTING
QR Wi-Fi connected: ssid=...
QR Wi-Fi state: CONNECTING -> CONNECTED
QR Wi-Fi got IP: if=... ip=...
QR Wi-Fi state: CONNECTED -> GOT_IP
```

## 6. 配置说明

工程默认使能 ISP、MIPI CSI、frame buffer、VG-Lite GPU、DPU、MIPI DSI、GC2053、ER68576B、media service、CJSON、Wi-Fi VNET controller、BK netif 和 LWIP。主要板级参数位于 `ap/ap_main.c`：

- sensor GPIO、I2C、分辨率和帧率
- ISP MP 尺寸与格式
- MIPI 屏型号及 GPIO
- GPU 输入/输出格式和 90 度旋转

识别尺寸、结果数和扫描周期定义在 `ap/src/qr_demo.c` 的 `QR_*` 宏中。ZBar 组件由工程依赖的 `zbar` 模块提供。

## 7. 注意事项

1. `start` 与 `start_mp` 不能同时运行；重复启动会返回 busy。
2. `start` 占用 camera、ISP、GPU、DPU、显示和帧缓冲资源，切换模式前应先执行 `ap_cmd qr stop`。
3. ZBar 只读取 NV12 的 Y 分量；修改识别尺寸时需同步调整 ISP 输出。
4. 二维码应清晰、完整且具有足够对比度；反光、失焦、运动模糊或尺寸过小会降低识别率。
5. `CMDRSP:OK` 只表示识别任务启动成功；二维码是否识别成功以 `found ... QR code(s)` 日志为准。
6. `start wifi` 和 `start_mp wifi` 应用新的二维码 Wi-Fi 配置前会先调用 `bk_wifi_sta_stop()`，确保重复配网从干净的 STA 状态开始。
7. `CONNECTING` 阶段临时出现的 `WIFI_REASON_NO_AP_FOUND` 只视为扫描 miss，不作为最终失败；密码错误或后续断开事件仍会将本地状态切到 `FAILED`。
