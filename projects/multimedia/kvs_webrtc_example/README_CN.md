# KVS WebRTC 示例工程

* [English](./README.md)

## 1. 工程概述

本工程演示 BK7259 平台作为 Amazon Kinesis Video Streams
WebRTC Master，通过 Wi-Fi 接入路由器后，将本地摄像头音视频通过
AWS KVS WebRTC 信令通道提供给网页 Viewer 观看。

当前工程提供：

- AP 侧 KVS WebRTC Master 启动命令：`ap_cmd kvs_wb master [channel]`
- 默认信道名：`kvs_doorbell_channel`
- AWS key 支持两种配置方式：`ap/ap_main.c` 中的编译期默认，或运行时
  CLI 命令 `ap_cmd kvs_wb cred <ak> <sk> [region]`
- 通过 AWS 官方 KVS WebRTC Test Page 作为网页 Viewer 验证

## 2. 测试环境

- 开发板：BK7259 系列或兼容平台
- 网络：开发板连接可访问公网的路由器
- AWS 账号：已开通 Kinesis Video Streams WebRTC，并具备创建/访问信令通道权限
- 浏览器：PC 浏览器可访问 AWS KVS WebRTC Test Page
- 串口终端：用于查看日志并输入 AP CLI 命令

## 3. 目录结构

```text
kvs_webrtc_example/
├── ap/
│   ├── ap_main.c                       # AP 入口，配置 AWS key 并初始化 KVS
│   ├── config/bk7259_ap/defconfig      # AP 配置
│   └── kvs_webrtc/
│       ├── kvs_webrtc_cli.c            # kvs_wb CLI
│       ├── kvs_webrtc_master.c         # KVS WebRTC Master
│       └── kvs_common.*                # KVS 公共逻辑
├── cp/
├── partitions/
├── CMakeLists.txt
└── Makefile
```

## 4. 编译与烧录

在 SDK 根目录执行：

```bash
cd <SDK_ROOT>
make bk7259 PROJECT=multimedia/kvs_webrtc_example -j$(nproc)
```

编译完成后，使用常用烧录工具将固件烧录到开发板。启动后打开 AP
串口控制台，后续命令均在 AP 控制台输入。

## 5. 使用步骤

### 5.1 配置 AWS key

支持两种方式，二者可并存：**编译期默认**（改源码，开机自动生效）与
**运行时命令输入**（CLI，随时覆盖）。

#### 方式 A：编译期默认（改 `ap_main.c`）

打开 `ap/ap_main.c`，将 `kvs_set_aws_credentials_env()` 中的占位符替换为
实际 AWS Access Key 与 Secret Key：

```c
setenv("AWS_ACCESS_KEY_ID", "YOUR_ACCESS_KEY_ID", 1);
setenv("AWS_SECRET_ACCESS_KEY", "YOUR_SECRET_ACCESS_KEY", 1);
//setenv("AWS_DEFAULT_REGION", "us-west-2", 1);
```

如使用的 KVS 信令通道不在默认区域，请同时打开并修改
`AWS_DEFAULT_REGION`。该 IAM key 需要具备 Kinesis Video Streams 与
Signaling Channel 相关权限。

> 注意：此方式会把明文 key 写进源码，提交代码时请勿泄露该文件。

#### 方式 B：运行时命令输入（推荐，源码零明文）

无需改源码，直接在 AP 串口控制台用 `kvs_wb cred` 命令设置 key（仅存于内存，
掉电不保存），设置后再启动 master 即可：

```text
# 设置 AWS 凭据（region 可选）
ap_cmd kvs_wb cred <access_key> <secret_key> [region]

# 查看当前凭据（Secret 只显示位数，不回显明文）
ap_cmd kvs_wb cred show

# 清除凭据
ap_cmd kvs_wb cred clear
```

使用示例：

```text
ap_cmd kvs_wb cred AWS_ACCESS_KEY_ID  AWS_SECRET_ACCESS_KEY  AWS_DEFAULT_REGION
ap_cmd kvs_wb master
```

说明：

- 命令通过 `setenv` 设置环境变量，会**覆盖**方式 A 的编译期默认值，
  因此可用命令临时切换到另一套 key。
- master 线程在 `ap_cmd kvs_wb master` 启动时才读取凭据，务必
  **先 `cred` 再 `master`**。
- 因仅存于内存，**设备重启后需重新执行 `ap_cmd kvs_wb cred ...`**
  （或依赖方式 A 的编译期默认）。

### 5.2 连接路由器

开发板启动后，通过 AP 串口连接 Wi-Fi 路由器：

```text
ap_cmd sta <ssid> <password>
```

连接成功后，日志中应能看到 STA connected、获取 IP 等信息。也可以查看
Wi-Fi 状态：

```text
ap_cmd state
```

请确认路由器可以访问公网，否则后续 AWS 信令、STUN/TURN 和 NTP 都可能失败。

### 5.3 获取 NTP 时间

KVS WebRTC 鉴权依赖正确的系统时间。确认网络连通后执行：

```text
ap_cmd uptime
```

工程已打开 `CONFIG_NTP_SYNC_RTC`，该命令会触发 NTP 同步并打印类似
`Get local time from NTP server`、`NTP Time` 的日志。若没有获取到有效时间，
请先检查路由器联网、DNS 和 NTP 服务器访问情况。

### 5.4 启动 KVS WebRTC Master

使用默认信道启动：

```text
ap_cmd kvs_wb master
```

或指定自定义信道名：

```text
ap_cmd kvs_wb master <channel_name>
```

默认信道名定义在 `ap/kvs_webrtc/kvs_webrtc_cli.c`：

```text
kvs_doorbell_channel
```

启动后关注串口日志中的 `kvs_db_cli`、`KVS Master`、`Signaling`、`ICE`
等关键字，确认信令连接和 WebRTC 连接状态。

### 5.5 使用网页 Viewer 观看

在 PC 浏览器打开 AWS 官方 KVS WebRTC Test Page：

<https://awslabs.github.io/amazon-kinesis-video-streams-webrtc-sdk-js/examples/index.html>

网页端配置需与设备端保持一致：

1. 填入与设备端相同的 `Access Key ID`、`Secret Access Key` 和 `Region`。
2. `Channel Name` 填写设备端使用的信道名，例如 `kvs_doorbell_channel`。
3. 选择 `Viewer` 角色。
4. 点击 `Start Viewer`。

连接成功后，网页端应能看到来自开发板的音视频流。若信道不存在，网页或设备端
会尝试通过 AWS API 创建/描述信道，具体行为取决于当前 AWS key 权限。

## 6. 常见问题与诊断

- `AWS_ACCESS_KEY_ID must be set` 或 `AWS_SECRET_ACCESS_KEY must be set`：
  检查 `ap/ap_main.c` 中 key 是否已替换并重新编译烧录，或在启动 master 前
  用 `ap_cmd kvs_wb cred <ak> <sk> [region]` 设置；可用
  `ap_cmd kvs_wb cred show` 确认当前是否已设置。
- 鉴权失败或请求被 AWS 拒绝：检查 key、region、IAM 权限和设备 RTC/NTP 时间。
- 网页 Viewer 无法连接：确认网页端与设备端的 key、region、channel 完全一致。
- ICE 连接失败：确认路由器可访问公网，网络没有阻断 UDP、STUN/TURN 或
  WebSocket 连接。
- 设备端没有推流：先确认 `ap_cmd kvs_wb master` 已启动成功，再查看
  `KVS Master`、`Signaling`、`ICE` 日志。

## 7. 快速命令参考

```text
# 连接路由器
ap_cmd sta <ssid> <password>

# 获取并打印 NTP/RTC 时间
ap_cmd uptime

# 运行时设置 AWS 凭据（可选，覆盖编译期默认；region 可选）
ap_cmd kvs_wb cred <access_key> <secret_key> [region]

# 查看 / 清除已设置的凭据
ap_cmd kvs_wb cred show
ap_cmd kvs_wb cred clear

# 使用默认信道启动 KVS WebRTC Master
ap_cmd kvs_wb master

# 使用指定信道启动 KVS WebRTC Master
ap_cmd kvs_wb master <channel_name>
```
