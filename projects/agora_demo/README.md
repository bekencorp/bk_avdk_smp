# Agora 演示工程

## 1. 工程概述

本工程演示在 **BK7259** 平台上集成 **声网 Agora IoT / RTC SDK**，通过统一的 **network_transfer** 层完成音视频上行：音频经 **audio_engine** 采集与编码，视频默认走 **USB UVC** 与 **video_engine**（与 `ap/config/bk7259_ap/defconfig` 中 `CONFIG_USB_CAMERA`、`CONFIG_VIDEO_ENGINE_USE_UVC_CAMERA` 等配置一致）。设备侧支持 **BLE/Console 配网**（`bk_smart_config`）、出厂参数（`bk_factory_config`）等能力。

更细的模块说明请参阅工程内文档：

- [网络传输模块说明](./ap/com_proj/network_transfer/README_CN.md)
- [Agora RTC 集成说明](./ap/com_proj/network_transfer/agora_rtc/README.md)
- [音频引擎模块说明](./ap/com_proj/audio_engine/README_CN.md)

## 1. 目录结构

工程为 **AP + CP** 双核结构，业务与多媒体代码主要在 **AP** 侧：

```
agora_demo
├── .ci                       # CI 配置
├── .gitignore
├── CMakeLists.txt            # 工程级 CMake（project 名为 agora_demo）
├── Makefile                  # 指向 SDK 统一工程构建
├── README_CN.md              # 本说明（中文）
├── app.rst                   # 应用描述占位（文档生成用）
├── ap/                       # AP 核
│   ├── CMakeLists.txt        # AP 组件注册、源文件与依赖
│   ├── Kconfig               # 工程级功能开关（配网、网络传输、音频等）
│   ├── ap_main.c             # AP 入口：媒体、配网、network_transfer 等初始化
│   ├── config/bk7259_ap/
│   │   └── defconfig         # AP 默认差异配置（Agora、UVC、OPUS 等）
│   └── com_proj/
│       ├── audio_engine/     # 音频引擎与 CLI（aude）
│       ├── bk_app_event/     # 应用事件与提示音相关
│       ├── bk_factory_config/
│       ├── bk_key_app/       # 按键服务(板子设备暂不支持按键，暂使用串口命令)
│       ├── bk_smart_config/  # Smart Config / 配网（sconf）
│       ├── network_transfer/ # 网络传输抽象层
│       │   ├── network_transfer.c / .h
│       │   └── agora_rtc/    # Agora 适配（bk_agora_api、引擎、Agent 等）
│       └── video_engine/     # 视频采集与经 network_transfer 发送
├── cp/                       # CP 核
│   ├── CMakeLists.txt
│   ├── cp_main.c
│   └── config/bk7259/defconfig
└── partitions/bk7259/
    ├── auto_partitions.csv
    └── ram_regions.csv
```

## 3. 功能说明

### 3.1 主要功能

- **ntwk_trans_init()** 在 `ap_main.c` 中调用；在 `CONFIG_AGORA_IOT_SDK` 下注册 Agora 的音频/视频发送与启停回调。
- **配网**：`bk_sconf_init()`，串口使用 **`sconf`** 子命令完成配网、擦除、模式切换等（见 `bk_smart_config` 实现）。


### 3.2 CLI 命令说明

在串口 Shell 中执行（若当前 SDK 要求 AP 命令前缀，则与工程其他 demo 一致增加前缀，例如 `ap_cmd`）：

**配网（Smart Config）**

```text
sconf start|erase|reset|vision|text [ble|console]
ap_cmd sconf start         进入配网模式
ap_cmd sconf erase         清除配网信息
```


命令成功/失败时，CLI 通常会打印 `CMDRSP:OK` 或 `CMDRSP:ERROR`（与平台 CLI 实现一致）。

### 3.3 用户参考文件

- `projects/agora_demo/ap/ap_main.c`
- `projects/agora_demo/ap/com_proj/network_transfer/network_transfer.c`
- `projects/agora_demo/ap/com_proj/network_transfer/agora_rtc/bk_agora_api.c`
- `projects/agora_demo/ap/com_proj/network_transfer/agora_rtc/agora_config.h`

## 4. 编译和运行

### 4.1 编译方法

在 **7259BK** SDK 根目录执行：

```bash
make bk7259 PROJECT=agora_demo
```


### 4.2 运行方法

1. 编译生成固件并烧录到目标板。
2. 手机端APP注册和下载
    APP下载：https://docs.bekencorp.com/arminodoc/bk_app/app/zh_CN/v2.0.1/app_download/index.html

    注册登录：使用邮箱注册登录

note::
    更多APP操作，请参考APP文档：
    https://docs.bekencorp.com/arminodoc/bk_app/app/zh_CN/v2.0.1/app_usage/app_usage_guide/index.html#ai

3. 使用 **sconf** 命令流程完成 **WiFi 配网**。
4. 对板载mic说唤醒词 ``armino`` 或 ``阿米诺``，设备唤醒后会播放提示音 ``啊`` ，然后可以进行AI对话
5. 视觉模式切换：板载上使用命令 ``ap_cmd sconf vision ``进行切换到视觉模型，重新切回文本对话模型使用命令 ``ap_cmd sconf text``
