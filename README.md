# Beken Armino SMP Development Framework

- [中文](./README_CN.md)

## Overview

**Armino SMP** is Beken's open-source software development framework for AIoT audio/video and smart terminal products. It provides foundational capabilities including BLE, Wi-Fi, Bluetooth, Thread, audio/video codecs, LCD display, on-device AI inference, and low power, along with the corresponding upper-layer application components and example projects. It is intended to help developers quickly build production applications on Beken chip platforms.

## Documentation

- [Armino SMP Online Documentation](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/smp_doc/bk7259/en/v4.0.1/index.html)

## Versioning Policy

The `release/v4.0.1` branch targets the **BK7259** chip. Armino SMP uses a maintenance branch plus release tags:

- `release/v4.0.1` is the ongoing maintenance branch for feature work and fixes. It always contains the latest code and may include changes that have not completed full release testing.
- `release/v4.0.1.x` are formal release tags, such as `release/v4.0.1.1` and `release/v4.0.1.2`. Each tag goes through a full test and release process and is suitable for production.
- When using an upper-layer product solution repository based on this SDK, it is recommended that the solution and Armino SMP SDK use the same tag version.

Prefer the highest `release/v4.0.1.x` tag for development and production. Use the `release/v4.0.1` branch only when you need the latest unreleased changes or are actively contributing.

## System Architecture

BK7259 uses an **AP + CP** dual-subsystem design. CP runs on CPU0 and can be configured as CPU0+CPU1 SMP for connectivity and system management. AP runs as CPU2+CPU3 SMP for multimedia and applications. The two sides run as independent images, communicate via Mailbox/shared memory, and are packaged into `all-app.bin`.

For more details, see [AP(SMP)+CP Architecture](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/smp_doc/bk7259/en/v4.0.1/developer-guide/architecture/index.html).

## Key Features

- **Dual independent images**: Separate AP/CP component trees and middleware, packaged into one firmware by the partition table.
- **SMP**: AP defaults to dual-core SMP; CP can be configured as single-core or dual-core SMP.
- **Connectivity and low power**: Wi-Fi/BLE stacks, low-power keepalive, and power management are primarily hosted on CP.
- **Audio/video and GUI**: Camera/Display/H.264/JPEG/GPU/VPU, the audio development kit, and LVGL are primarily hosted on AP.
- **Configurable build**: Make + CMake + Kconfig/`menuconfig`, with local and Docker builds.
- **Examples**: `projects/` covers the default app, multimedia, Bluetooth, Wi-Fi, TFLite Micro, power management, and more.

## Get the Code

Armino SMP is published on GitHub, Gitee, and GitLab.

- GitHub: <https://github.com/bekencorp/bk_avdk_smp>
- Gitee: <https://gitee.com/bekencorp/bk_avdk_smp>
- GitLab: <https://gitlab.bekencorp.com/armino/bk_avdk_smp>

GitHub and Gitee are public. GitLab is for enterprise customers; contact your FAE or sales representative for access.

**Windows note**: Before cloning with Git for Windows, disable automatic line-ending conversion to avoid CRLF-related build failures. Linux, macOS, and WSL do not need this step.

```bash
git config --global core.autocrlf false
```

Changing this setting does not rewrite an existing clone; re-clone after applying it.

Example using GitHub and the `release/v4.0.1` branch. **Prefer the latest formal tag `release/v4.0.1.x` in practice.** Replace the URL for other remotes.

```bash
mkdir -p ~/armino && cd ~/armino

git clone --branch release/v4.0.1 https://github.com/bekencorp/bk_avdk_smp.git
```

For more details, see [Getting Started](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/smp_doc/bk7259/en/v4.0.1/get-started/index.html).

## Build Environment

Armino SMP supports local and Docker builds. Choose a setup method based on your development platform.

### Linux local build

Enter the SDK directory and run the environment setup script:

```bash
# The setup script is in the bk_avdk_smp repository
cd ~/armino/bk_avdk_smp
sudo bash tools/env_tools/setup/armino_env_setup.sh
```

### Windows local build

Download and install [Armino Bash](https://dl.bekencorp.com/tools/arminosdk/WindowsInstaller/Armino-Bash-Setup_0.3.0.exe).

### Docker build

Use the [`bekencorp/armino-idk`](https://hub.docker.com/r/bekencorp/armino-idk/tags) image. Choose tag `1.5` or newer. Windows / Linux / macOS are supported.

For more details, see [Armino SMP Getting Started: Environment Setup and Build](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/smp_doc/bk7259/en/v4.0.1/get-started/index.html).

## Build a Project

Using the `app` project as an example (under `projects/app`), build from the SDK root:

```bash
cd ~/armino/bk_avdk_smp
make bk7259 PROJECT=app        # PROJECT is optional; defaults to app
```

Docker builds are also supported (on Linux/macOS use `./dbuild.sh`; on Windows PowerShell use `.\dbuild.ps1`):

```bash
cd ~/armino/bk_avdk_smp
./dbuild.sh make bk7259 PROJECT=app
```

After a successful build, the firmware for flashing is at the following path (relative to the SDK repository root):

```text
build/bk7259/app/package/all-app.bin
```

For more details, see [Getting Started](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/smp_doc/bk7259/en/v4.0.1/get-started/index.html).

## Flash Firmware

Flash the firmware with either of the following:

- Download and use the [BKFIL local flash tool](https://dl.bekencorp.com/tools/bkfil/v4)
- Use the [BKFIL web flash tool](https://connect.aclsemi.com/)

Select the `all-app.bin` firmware built in the previous section.

For more details, see [Armino SMP Getting Started](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/smp_doc/bk7259/en/v4.0.1/get-started/index.html).

## Example Projects

Typical projects by category are listed below. More examples are under `projects/` in the SDK repository.

| Project | Main features | Details (see docs) |
| --- | --- | --- |
| `isp_example` | Multimedia / camera: MIPI CSI capture and ISP pipeline | `projects/multimedia/isp_example/README.md` |
| `mipi_lcd_example` | Multimedia / display: MIPI DSI LCD | `projects/multimedia/mipi_lcd_example/README.md` |
| `h264_encode_example` | Multimedia / codec: H.264 encode | `projects/multimedia/h264_encode_example/README.md` |
| `player_service_example` | Multimedia / audio: bk_player_service playback path | `projects/multimedia/player_service_example/README.md` |
| `widgets_v9` | GUI: LVGL V9 widgets demo | `projects/lvgl/widgets_v9/README.md` |
| `gatt_server` | Bluetooth: BLE GATT server | `projects/bluetooth/gatt_server/README.md` |
| `kvs_webrtc_example` | Network / Wi-Fi: KVS WebRTC A/V streaming | `projects/multimedia/kvs_webrtc_example/README.md` |
| `face_recognition` | On-device AI: YOLOFace detection with OSD | `projects/tflite_micro/face_recognition/README.md` |
| `ap_powerdown_keepalive` | Low power: AP power-down keepalive with CP | `projects/ap_powerdown_keepalive/README.md` |

For more details, see [Create a New Project](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/smp_doc/bk7259/en/v4.0.1/developer-guide/create_new_project_rule/index.html).

## Product Solutions

The following upper-layer product solutions are based on this SDK. It is recommended that the solution and Armino SMP SDK use the same tag version.

| Solution | Main features | Details |
| --- | --- | --- |
| `bk_solution_ai` | Robot / AI: voice wake-up, AI dialog, NPU vision, multi-sensor peripherals | [Robot Solution docs](https://docs.bekencorp.com/arminodoc/bk_ai_smp/bk7259/en/v4.0.1/index.html) |
| `bk_solution_dashboard` | Two-wheeler dashboard: LVGL UI, casting, Dashcam, Bluetooth audio and provisioning | [Two-Wheeler Solution docs](https://docs.bekencorp.com/arminodoc/bk_dashboard/bk7259/en/v4.0.1/index.html) |
| `bk_solution_doorbell` | Video doorbell: camera capture, network streaming, two-way intercom, low-power keepalive | [Doorbell Solution docs](https://docs.bekencorp.com/arminodoc/bk_doorbell/bk7259/en/v4.0.1/index.html) |
| `bk_solution_ipc` | Network camera: headless IPC, AOV low power, ISP/H264 image-quality tuning | [IPC Solution docs](https://docs.bekencorp.com/arminodoc/bk_ipc/bk7259/en/v4.0.1/index.html) |

## Beken Resources

- [Beken website](https://www.bekencorp.com/)
- [ARMINO developer forum](https://armino.bekencorp.com/)
- [Beken documentation center](https://docs.bekencorp.com/)
- [BK7259 Datasheet](https://docs.bekencorp.com/spec/BK7259/BK7259_Datasheet.pdf)
