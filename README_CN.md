# 博通集成 Armino SMP 开发框架

- [English](./README.md)

## 概述

**Armino SMP** 是 BEKEN 发布并开源的 AIoT 音视频与智能终端软件开发框架。框架提供包括 BLE、Wi-Fi、蓝牙、Thread、音视频编解码、LCD 显示、本地 AI 推理、低功耗等基础能力以及对应的上层应用组件、示例工程。旨在帮助开发者在 BEKEN 芯片平台上快速开发落地产品应用。

## 文档

- [Armino SMP 在线文档](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/smp_doc/bk7259/zh_CN/v4.0.1/index.html)

## 版本策略

`release/v4.0.1` 分支面向 **BK7259** 芯片。本框架采用“维护分支 + 发布标签”的版本管理方式：

- `release/v4.0.1` 为持续维护分支，用于该版本系列的功能迭代和问题修复，始终包含最新代码，但其中可能包含尚未完成完整发布测试的改动。
- `release/v4.0.1.x` 为正式发布标签，例如 `release/v4.0.1.1`、`release/v4.0.1.2`。每个标签均经过完整的测试与发布流程，可作为量产版本使用。
- 若同时使用基于本 SDK 的上层产品解决方案仓库，方案代码与 Armino SMP SDK 建议使用相同的标签版本。

建议您选择 `release/v4.0.1.x` 系列中版本号最大的标签进行开发和量产。仅需体验最新功能或参与开发时，才建议使用 `release/v4.0.1` 分支。

## 系统架构

BK7259 采用 **AP + CP** 双子系统架构：CP 以 CPU0 运行，也可配置为 CPU0+CPU1 SMP，负责连接与系统管理；AP 以 CPU2+CPU3 SMP 运行，负责多媒体与应用。两侧独立镜像，经 Mailbox / 共享内存协同后打包为 `all-app.bin`。

详细介绍请参阅 [AP(SMP)+CP 架构](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/smp_doc/bk7259/zh_CN/v4.0.1/developer-guide/architecture/index.html)。

## 主要特性

- **异构双镜像**：AP / CP 独立组件树与中间件，按分区表打包为统一固件。
- **SMP 支持**：AP 默认双核 SMP；CP 可按工程配置为单核或双核 SMP。
- **连接与低功耗**：Wi-Fi / BLE 协议栈与低功耗保活、电源管理主要由 CP 侧承载。
- **音视频与 GUI**：Camera / Display / H.264 / JPEG / GPU / VPU、音频开发框架及 LVGL 等能力由 AP 侧承载。
- **可配置构建**：Make + CMake + Kconfig / `menuconfig`，支持本地编译与 Docker 编译。
- **丰富示例**：`projects/` 提供默认应用、多媒体、蓝牙、Wi-Fi、TFLite Micro、低功耗等工程。

## 获取代码

Armino SMP SDK 同步发布至 GitHub、Gitee 和 GitLab，可根据网络环境及访问权限选择代码源。

- GitHub：<https://github.com/bekencorp/bk_avdk_smp>
- Gitee：<https://gitee.com/bekencorp/bk_avdk_smp>
- GitLab：<https://gitlab.bekencorp.com/armino/bk_avdk_smp>

GitHub 和 Gitee 可公开访问。GitLab 仅面向企业客户开放；企业客户如需访问，请联系对接的 FAE 或销售人员申请开通权限。

**Windows 用户注意**：使用 Git for Windows 获取代码时，建议在克隆前关闭自动换行符转换，避免脚本或源文件被转换为 CRLF 而导致编译失败。Linux、macOS 和 WSL 环境无需执行。

```bash
git config --global core.autocrlf false
```

如果代码已经克隆，修改该配置不会自动恢复文件，建议设置后重新克隆。

以下命令以 GitHub 和 `release/v4.0.1` 分支为例。**实际获取代码时，请优先选择最新正式发布标签** `release/v4.0.1.x`。使用其他代码源时，替换对应的仓库地址即可。

```bash
mkdir -p ~/armino && cd ~/armino

git clone --branch release/v4.0.1 https://github.com/bekencorp/bk_avdk_smp.git
```

代码获取的更多说明见 [快速入门](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/smp_doc/bk7259/zh_CN/v4.0.1/get-started/index.html)。

## 编译环境安装

Armino SMP 支持本地编译和 Docker 编译，可根据开发平台选择以下方式部署编译环境。

### Linux 本地编译

进入 SDK 目录并运行环境安装脚本：

```bash
# 安装脚本位于 bk_avdk_smp 仓库内
cd ~/armino/bk_avdk_smp
sudo bash tools/env_tools/setup/armino_env_setup.sh
```

### Windows 本地编译

下载并安装 [Armino Bash](https://dl.bekencorp.com/tools/arminosdk/WindowsInstaller/Armino-Bash-Setup_0.3.0.exe)。

### Docker 编译

Docker 编译镜像为 [`bekencorp/armino-idk`](https://hub.docker.com/r/bekencorp/armino-idk/tags)，请选择 `1.5` 或更高版本的镜像标签，支持 Windows / Linux / macOS。

详细安装步骤请参阅 [Armino SMP 快速入门：环境部署及编译](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/smp_doc/bk7259/zh_CN/v4.0.1/get-started/index.html)。

## 编译工程

以 `app` 工程为例（位于 `projects/app`），在 SDK 根目录本地编译：

```bash
cd ~/armino/bk_avdk_smp
make bk7259 PROJECT=app        # PROJECT 可选，默认为 app
```

也支持 Docker 编译（Linux / macOS 用 `./dbuild.sh`，Windows PowerShell 用 `.\dbuild.ps1`）：

```bash
cd ~/armino/bk_avdk_smp
./dbuild.sh make bk7259 PROJECT=app
```

编译成功后，用于烧录的固件文件位于以下路径（相对于 SDK 仓库根目录）：

```text
build/bk7259/app/package/all-app.bin
```

编译命令的详细说明请参阅 [快速入门](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/smp_doc/bk7259/zh_CN/v4.0.1/get-started/index.html)。

## 烧录固件

可选择以下任一方式烧录固件：

- 下载并使用 [BKFIL 本地烧录工具](https://dl.bekencorp.com/tools/bkfil/v4)
- 使用 [BKFIL 网页烧录工具](https://connect.aclsemi.com/)

烧录时请选择上一节编译生成的 `all-app.bin` 固件文件。

详细烧录流程请参阅 [Armino SMP 快速入门](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/smp_doc/bk7259/zh_CN/v4.0.1/get-started/index.html)。

## 参考工程

以下按类型列出典型工程；更多示例见 SDK 仓库 `projects/` 目录。

| 工程名 | 主要功能 | 详细说明（见文档） |
| --- | --- | --- |
| `isp_example` | 多媒体 / 摄像头：MIPI CSI 采集与 ISP 处理 | `projects/multimedia/isp_example/README_CN.md` |
| `mipi_lcd_example` | 多媒体 / 显示：MIPI DSI LCD 显示 | `projects/multimedia/mipi_lcd_example/README_CN.md` |
| `h264_encode_example` | 多媒体 / 编解码：H.264 编码 | `projects/multimedia/h264_encode_example/README_CN.md` |
| `player_service_example` | 多媒体 / 音频：bk_player_service 播放链路 | `projects/multimedia/player_service_example/README_CN.md` |
| `widgets_v9` | GUI：LVGL V9 Widgets Demo | `projects/lvgl/widgets_v9/README_CN.md` |
| `gatt_server` | 蓝牙：BLE GATT Server | `projects/bluetooth/gatt_server/README_CN.md` |
| `kvs_webrtc_example` | 网络 / Wi-Fi：KVS WebRTC 音视频推流 | `projects/multimedia/kvs_webrtc_example/README_CN.md` |
| `face_recognition` | 端侧 AI：YOLOFace 人脸检测与 OSD 显示 | `projects/tflite_micro/face_recognition/README_CN.md` |
| `ap_powerdown_keepalive` | 低功耗：AP 掉电保活与 CP keepalive | `projects/ap_powerdown_keepalive/README_CN.md` |

新建工程、分区与配置方法见 [新建工程介绍](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/smp_doc/bk7259/zh_CN/v4.0.1/developer-guide/create_new_project_rule/index.html)。

## 产品解决方案

以下列出基于本 SDK 的上层产品解决方案；方案代码与 Armino SMP SDK 建议使用相同的标签版本。

| 方案名 | 主要功能 | 详细说明 |
| --- | --- | --- |
| `bk_solution_ai` | 机器人 / AI：语音唤醒、AI 对话、NPU 视觉识别、多传感器外设 | [机器人方案在线文档](https://docs.bekencorp.com/arminodoc/bk_ai_smp/bk7259/zh_CN/v4.0.1/index.html) |
| `bk_solution_dashboard` | 两轮车仪表：LVGL 仪表显示、投屏、Dashcam、蓝牙音频与配网 | [两轮车方案在线文档](https://docs.bekencorp.com/arminodoc/bk_dashboard/bk7259/zh_CN/v4.0.1/index.html) |
| `bk_solution_doorbell` | 可视门铃：摄像头采集、网络图传、双向对讲、低功耗保活 | [门铃方案在线文档](https://docs.bekencorp.com/arminodoc/bk_doorbell/bk7259/zh_CN/v4.0.1/index.html) |
| `bk_solution_ipc` | 网络摄像头：无屏 IPC、AOV 低功耗、ISP/H264 图像质量联调 | [IPC 方案在线文档](https://docs.bekencorp.com/arminodoc/bk_ipc/bk7259/zh_CN/v4.0.1/index.html) |

## BEKEN 相关资源

- [BEKEN 官网](https://www.bekencorp.com/)
- [ARMINO 开发者论坛](https://armino.bekencorp.com/)
- [BEKEN 文档中心](https://docs.bekencorp.com/)
- [BK7259 Datasheet](https://docs.bekencorp.com/spec/BK7259/BK7259_Datasheet.pdf)

