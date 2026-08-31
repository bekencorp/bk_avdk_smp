# Beken Armino SMP Development Framework

- [中文](./README_CN.md)

## Overview

**Armino SMP** is Beken's open-source software development framework for AIoT audio/video and smart terminal products. It provides foundational capabilities including BLE, Wi-Fi, Bluetooth, audio/video codecs, LCD display, voice and multimedia, and low power, along with the corresponding upper-layer application components and example projects. It is intended to help developers quickly build production applications on Beken chip platforms.

## Documentation

- [Armino SMP Online Documentation](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/smp_doc/bk7258/en/v3.1.1/index.html)

## Versioning Policy

The `release/v3.1.1` branch targets the **BK7258** chip. Armino SMP uses a maintenance branch plus release tags:

- `release/v3.1.1` is the ongoing maintenance branch for feature work and fixes. It always contains the latest code and may include changes that have not completed full release testing.
- `release/v3.1.1.x` are formal release tags, such as `release/v3.1.1.1` and `release/v3.1.1.2`. Each tag goes through a full test and release process and is suitable for production.
- When using an upper-layer product solution repository based on this SDK, it is recommended that the solution and Armino SMP SDK use the same tag version.

Prefer the highest `release/v3.1.1.x` tag for development and production. Use the `release/v3.1.1` branch only when you need the latest unreleased changes or are actively contributing.

## System Architecture

BK7258 uses an **AP + CP** dual-subsystem design. CP runs as a single core on CPU0 for connectivity and low-power keepalive. AP runs as CPU1+CPU2 SMP for multimedia and applications. The two sides run as independent images, communicate via Mailbox/shared memory, and are packaged into `all-app.bin`.

For details, see: [AP(SMP)+CP Architecture](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/smp_doc/bk7258/en/v3.1.1/developer-guide/architecture/index.html).

## Key Features

- **Dual independent images**: Separate AP/CP component trees and middleware, packaged into one firmware by the partition table.
- **SMP**: AP is dual-core SMP on CPU1+CPU2; CP is single-core on CPU0.
- **Connectivity and low power**: Wi-Fi/BLE stacks, low-power keepalive, and power management are primarily hosted on CP.
- **Audio/video and GUI**: Camera/Display/codecs, UVC, audio playback and service frameworks, and LVGL are primarily hosted on AP.
- **Configurable build**: Make + CMake + Kconfig/`menuconfig`, with local and Docker builds.
- **Examples**: `projects/` covers the default app, audio/video and display, LVGL, Bluetooth, Wi-Fi, power management, and more.

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

Example using GitHub and the `release/v3.1.1` branch. **Prefer the latest formal tag `release/v3.1.1.x` in practice.** Replace the URL for other remotes.

```bash
mkdir -p ~/armino && cd ~/armino

git clone --branch release/v3.1.1 https://github.com/bekencorp/bk_avdk_smp.git
```

For details, see: [Getting Started](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/smp_doc/bk7258/en/v3.1.1/get-started/index.html).

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

For details, see: [Armino SMP Getting Started: Environment Setup and Build](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/smp_doc/bk7258/en/v3.1.1/get-started/index.html).

## Build a Project

Using the `app` project as an example (under `projects/app`), build from the SDK root:

```bash
cd ~/armino/bk_avdk_smp
make bk7258 PROJECT=app        # PROJECT is optional; defaults to app
```

Docker builds are also supported (on Linux/macOS use `./dbuild.sh`; on Windows PowerShell use `.\dbuild.ps1`):

```bash
cd ~/armino/bk_avdk_smp
./dbuild.sh make bk7258 PROJECT=app
```

After a successful build, the firmware for flashing is at the following path (relative to the SDK repository root):

```text
build/bk7258/app/package/all-app.bin
```

For details, see: [Getting Started](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/smp_doc/bk7258/en/v3.1.1/get-started/index.html).

## Flash Firmware

Flash the firmware with either of the following:

- Download and use the [BKFIL local flash tool](https://dl.bekencorp.com/tools/bkfil/v4)
- Use the [BKFIL web flash tool](https://connect.aclsemi.com/)

Select the `all-app.bin` firmware built in the previous section.

For details, see: [Armino SMP Getting Started](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/smp_doc/bk7258/en/v3.1.1/get-started/index.html).

## Example Projects

Typical projects by category are listed below. More examples are under `projects/` in the SDK repository.

| Project | Main features | Details (see docs) |
| --- | --- | --- |
| `dvp_example` | Multimedia / camera: DVP capture with MJPEG/H.264 output | `projects/dvp_example/README.md` |
| `rgb_lcd_example` | Multimedia / display: RGB LCD | `projects/rgb_lcd_example/README.md` |
| `encoder_example` | Multimedia / codec: YUV422 to MJPEG encode | `projects/encoder_example/README.md` |
| `player_service_example` | Multimedia / audio: bk_player_service playback path | `projects/player_service_example/README.md` |
| `lvgl/music` | GUI: LVGL local music player demo | `projects/lvgl/music/README.md` |
| `bluetooth/hfp_ag` | Bluetooth: HFP AG example | `projects/bluetooth/hfp_ag/README.md` |
| `uvc_example` | Multimedia / UVC: USB camera capture | `projects/uvc_example/README.md` |
| `ap_powerdown_keepalive` | Low power: AP power-down keepalive with CP | `projects/ap_powerdown_keepalive/README.md` |

For details, see: [Create a New Project](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/smp_doc/bk7258/en/v3.1.1/developer-guide/create_new_project_rule/index.html).

## Product Solutions

The following upper-layer product solutions are based on this SDK. It is recommended that the solution and Armino SMP SDK use the same tag version.

| Solution | Main features | Details |
| --- | --- | --- |
| `bk_solution_ai` | AI device: voice wake-up, cloud dialog, image recognition, dual-screen display | [AI Solution docs](https://docs.bekencorp.com/arminodoc/bk_ai_smp/bk7258/en/v3.1.1/index.html) |
| `bk_solution_dashboard` | Two-wheeler dashboard: LVGL UI, casting, Dashcam, Bluetooth audio and provisioning | [Two-Wheeler Solution docs](https://docs.bekencorp.com/arminodoc/bk_dashboard/bk7258/en/v3.1.1/index.html) |
| `bk_solution_doorbell` | Video doorbell: camera capture, network streaming, two-way intercom, low-power keepalive | [Doorbell Solution docs](https://docs.bekencorp.com/arminodoc/bk_doorbell/bk7258/en/v3.1.1/index.html) |

## Beken Resources

- [Beken website](https://www.bekencorp.com/)
- [ARMINO developer forum](https://armino.bekencorp.com/)
- [Beken documentation center](https://docs.bekencorp.com/)
- [BK7258 Datasheet](https://docs.bekencorp.com/spec/BK7258/BK7258%C2%A0Datasheet.pdf)
