# Draw OSD 示例工程

* [English](./README.md)

## 1. 项目概述

本项目演示 Beken BK7259 平台上的 **GPU OSD（On-Screen Display）** 叠加：MIPI/UVC pipeline 在 GPU open 后创建独立 `bk_gpu_overlay`，`bk_draw_osd` 绑定 overlay，将图标和文字 sprite 作为租约图层提交并按序 `SRC_OVER` 到实时画面；关闭时先删除 OSD/overlay，再关闭 GPU。

当前工程提供：

- **MIPI 通路**：GC2053 CSI → ISP → GPU → DPU → hx8399c 1080×1920 竖屏
- **UVC 通路**：USB MJPEG → 硬解 → GPU → DPU → 同一块屏
- **`osd` CLI**：在真实视频上显示 / 更新 / 移除 OSD，或关闭 pipeline、切换通路
- **上电自启**：默认自动拉起 MIPI 摄像头 + OSD（整帧融合 `frame-end`）
- `.it.csv` 集成测试入口

### 1.1 测试环境

| 项目 | 说明 |
|------|------|
| 核心板 | **BK7259_QF128_12.3X12.3_V4.0**，PSRAM 32M |
| 默认 MIPI 屏 | **hx8399c MIPI 1080×1920**（竖屏显示） |
| MIPI 摄像头 | GC2053，1920×1080@25fps |
| UVC | USB 摄像头 MJPEG 1920×1080@30（port 1） |
| 面板 VDDIO | 需 1.8V（`display.c` 内 `PM_AUXLDO_USER_DISPLAY`） |
| 晶振 | `CONFIG_XTAL_FREQ=12000000`（MIPI sensor I2C） |

> 请使用参考外设熟悉 demo；外设不同需同步改 `display.c`、相机配置与 assets 坐标。

## 2. 目录结构

```
draw_osd_example/
├── .it.csv                   # 集成测试用例
├── ap/
│   ├── ap_main.c             # 上电注册 CLI + 自动启动 MIPI OSD
│   ├── assets/               # UI 工具生成的图标/字库
│   ├── draw_osd/             # draw_osd_cli.c、display.c（LCD/DPU/VDDIO）
│   ├── mipi/                 # mipi_pipeline.c、osd_mipi.c
│   └── uvc/                  # uvc_pipeline.c、osd_uvc.c
└── ...
```

## 3. 编译与烧录

```bash
cd <SDK根目录>
make bk7259 PROJECT=multimedia/draw_osd_example -j32
```

产物：`build/bk7259/draw_osd_example/package/all-app.bin`

烧录后串口打印 `draw_osd_example m55 running...`，约 5s 后自动开 flexa OSD 并常驻显示，成功即打印 `[RESULT][PASS] draw_osd_mipi_flexa_boot success`（需 MIPI 摄像头与屏已接好）。

- 命令成功：`CMDRSP:OK` + `[RESULT][PASS] <case名> success`
- 命令失败：`CMDRSP:ERROR` + `[RESULT][FAIL] <case名> failed at ...`

> OSD 命令在 **AP** 侧；从 CP 串口发送需加 `ap_cmd` 前缀。

## 4. 注意事项（必读）

1. **MIPI 与 UVC 不能同时开**（共用 GPU/显示内存）。切换前先 `osd <当前源> close`。
2. **OSD sprite 格式**：MIPI / UVC 均用 `ABGR8888`（补偿 GPU 压缩底图 BGRA 字节序），否则图标/汉字红蓝反色。
3. **`blend_assets` / `blend_info`** 必须以 `{.addr = NULL}` 结尾；资源指针生命周期需 ≥ OSD 实例。
4. **`update` / `remove` 后** 组件内部会重调 `bk_draw_osd_array` 生效；`clear` 只撤 OSD 保留视频；`close` 撤 OSD 并关 pipeline。

---

## 5. 使用指南（CLI 速查）

### 5.1 命令格式

```text
ap_cmd osd <mipi|uvc> <frame|flexa|update|remove|clear|close> [参数...]
```

| 动作 | 说明 |
|------|------|
| `frame` | 显示 OSD，整帧末一次 SRC_OVER（默认，元素少时推荐） |
| `flexa` | 显示 OSD，按 flexa 逐块分散融合（元素多/分散时用） |
| `update <name> <content>` | 改图标或文本（必须带 content） |
| `remove <name>` | 从显示列表移除元素 |
| `clear` | 只撤 OSD 叠加，保留实时视频 |
| `close` | 撤 OSD 并关闭该通路 pipeline（切源前必做） |

显示内容默认来自 `ap/assets/blend_dsc.c` 的 `blend_info[]`（text1、text2、wifi_group、beken_logo 等），坐标由各资源 `xpos/ypos` 决定。

### 5.2 上电默认行为

固件上电后会自动执行等价于 `ap_cmd osd mipi flexa` 的 IT case：

- 等待约 5s（避开 CP/AP 启动 log 与 HSPL UART 锁争用）
- 打开 MIPI pipeline（GC2053 + ISP + GPU + LCD），flexa 模式叠加默认 `blend_info[]`
- 开成功即打印 `[RESULT][PASS] draw_osd_mipi_flexa_boot success`，之后 OSD 常驻显示（不关 pipeline）

若摄像头未接或开 pipeline 失败，串口会打印 `[RESULT][FAIL] draw_osd_mipi_flexa_boot failed at show`。

### 5.3 常用操作示例

**只撤 OSD，视频继续：**

```text
ap_cmd osd mipi clear
```

**彻底关闭 MIPI（释放 GPU/内存）：**

```text
ap_cmd osd mipi close
```

**重新打开 MIPI OSD（关闭后或 clear 后）：**

```text
ap_cmd osd mipi frame
```

**运行时改 WiFi 图标 / 时间文字：**

```text
ap_cmd osd mipi update wifi_group wifi_rssi_full
ap_cmd osd mipi update wifi_group wifi_rssi_2
ap_cmd osd mipi update text1 12:53
```

**移除某个元素：**

```text
ap_cmd osd mipi remove wifi_group
ap_cmd osd mipi remove text1
```

**从 MIPI 切到 UVC（需 UVC 摄像头 + 先关 MIPI）：**

```text
ap_cmd osd mipi close
ap_cmd osd uvc flexa
```

UVC 侧命令与 MIPI 对称，例如：

```text
ap_cmd osd uvc update text1 13:52
ap_cmd osd uvc remove wifi_group
ap_cmd osd uvc clear
ap_cmd osd uvc close
```

**从 UVC 切回 MIPI：**

```text
ap_cmd osd uvc close
ap_cmd osd mipi frame
```

### 5.4 资产与命名

| 类型 | 列表项 `name` | `update` 的 `content` 示例 |
|------|---------------|---------------------------|
| WiFi 图标组 | `wifi_group` | `wifi_rssi_none` / `wifi_rssi_1` … `wifi_rssi_full` |
| 时间文本 | `text1` | 任意字符串，如 `12:53` |
| 欢迎语 | `text2` | 任意字符串 |
| Logo | `beken_logo` | `beken_logo` |

### 5.5 典型问题

| 现象 | 处理 |
|------|------|
| 屏不亮 | 查背光 GPIO_7、复位 GPIO_60、面板 VDDIO 1.8V |
| 图标/汉字红蓝反 | 确认 `osd_*` 里 `src_format = ABGR8888` |
| MIPI 相机打不开 | 查 VDDIO、`CONFIG_XTAL_FREQ=12000000`、GC2053 接线 |
| UVC OOM | 先 `osd mipi close` 再开 UVC |
| `update` 报错 | 必须两个参数：`update <name> <content>`；移除用 `remove <name>` |
| `no osd instance` | 先 `frame`/`flexa` 或等上电自启完成 |

### 5.6 集成测试（`.it.csv`）

自动化测试按 `.it.csv` 期望结果匹配 `[RESULT][PASS] <case名> success`。每个 CLI 子命令（mipi/uvc 对称）成功即打对应 PASS 行，失败打 `[RESULT][FAIL] <case名> failed at ...`：

| 通路 | 命令 | 期望 log |
|------|------|----------|
| 上电 | 自启 | `[RESULT][PASS] draw_osd_mipi_flexa_boot success` |
| MIPI | `osd mipi update text1 ...` | `[RESULT][PASS] osd_mipi_update_text success` |
| MIPI | `osd mipi update wifi_group ...` | `[RESULT][PASS] osd_mipi_update_wifi success` |
| MIPI | `osd mipi remove wifi_group` | `[RESULT][PASS] osd_mipi_remove_wifi success` |
| MIPI | `osd mipi clear` | `[RESULT][PASS] osd_mipi_clear success` |
| MIPI | `osd mipi flexa` | `[RESULT][PASS] osd_mipi_flexa success` |
| MIPI | `osd mipi frame` | `[RESULT][PASS] osd_mipi_frame success` |
| MIPI | `osd mipi close` | `[RESULT][PASS] osd_mipi_close success` |
| UVC | `osd uvc flexa` | `[RESULT][PASS] osd_uvc_flexa success` |
| UVC | `osd uvc update text1 ...` | `[RESULT][PASS] osd_uvc_update_text success` |
| UVC | `osd uvc update wifi_group ...` | `[RESULT][PASS] osd_uvc_update_wifi success` |
| UVC | `osd uvc remove wifi_group` | `[RESULT][PASS] osd_uvc_remove_wifi success` |
| UVC | `osd uvc clear` | `[RESULT][PASS] osd_uvc_clear success` |
| UVC | `osd uvc frame` | `[RESULT][PASS] osd_uvc_frame success` |
| UVC | `osd uvc close` | `[RESULT][PASS] osd_uvc_close success` |

> case 名规则：`osd_<pipe>_<suffix>`。UVC 相关 case 需接 UVC 摄像头；且因 MIPI/UVC 共用显存，`.it.csv` 里先跑完 MIPI 段并 `close`，再进入 UVC 段。

更详细的组件 API、doorbell 移植说明见 [`docs/bk_draw_osd_usage_CN.md`](./docs/bk_draw_osd_usage_CN.md).
