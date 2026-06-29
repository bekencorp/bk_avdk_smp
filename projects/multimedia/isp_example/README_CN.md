# ISP 示例工程

* [English](./README.md)

## 1. 项目概述

本工程用于演示 BK7259 **MIPI CSI / DVP** 摄像头采集与 **ISP** 图像处理通路，覆盖传感器自动检测、MP/SP 双通道、Frame / Flexa 输出模式，以及 ISP tuning、dump 等调试能力。

* 开发者指南：

  - [Camera 总览](../../../developer-guide/camera/index.html)
  - [MIPI CSI](../../../developer-guide/camera/mipi_csi.html)
  - [DVP](../../../developer-guide/camera/dvp.html)

* API 参考：

  - [Camera API](../../../api-reference/multimedia/bk_camera.html)
  - [图像内存管理](../../../api-reference/multimedia/bk_frame_buffer.html)

### 1.1 测试环境

- 硬件配置
  - 核心板：**BK7259_QF128_12.3X12.3_V4.0**
  - PSRAM：32M
  - MIPI CSI 默认传感器：GC2053（1920×1080@20fps）
  - DVP 默认传感器：GC2145（1280×720@30fps）
- 软件依赖：`CONFIG_ISP`、`CONFIG_BK_CAMERA`、`CONFIG_FRAME_BUFFER`、`CONFIG_MEDIA_SERVICE`

.. warning::

    请使用参考 Sensor 与转接板进行学习验证。外设规格不同时，GPIO、I2C、分辨率与 Kconfig 可能需要重新配置。ISP **不支持放大**，传感器最大分辨率须不小于 ISP 输出分辨率。

## 2. 目录结构

```text
isp_example/
├── ISP_TEST_CASES.md          # 完整 CLI 测试用例清单
├── ap/
│   ├── ap_main.c              # 入口：media_service、frame_buffer、tuning、CLI
│   └── src/
│       ├── isp_cli.c          # CLI 注册
│       ├── isp_func_test.c    # isp 高层命令（detect/open/close/read）
│       ├── isp_api_test.c     # isp_api 分步 API（当前为桩）
│       ├── isp_tuning_test.c  # ISP tuning server
│       ├── isp_dump_test.c    # ISP dump / 传感器寄存器读写
│       └── isp_mini_code.c    # MIPI CSI 控制器初始化
├── cp/
└── partitions/
```

## 3. 功能说明

### 3.1 CLI 命令

| 命令 | 子命令 | 说明 |
|------|--------|------|
| **isp** | `detect` | 扫描 DVP / CSI 传感器 |
| | `open` | 打开通道，见下方语法 |
| | `close` | `isp close <mp\|sp>` |
| | `read` | `isp read <mp\|sp>` — 读一帧并 hex dump |
| | `sensor_open` | MIPI mini code 传感器初始化 |
| | `init` / `soft_reset` | 控制器初始化 / 软复位 |
| **isp_api** | 多子命令 | 分步调用 Camera API（当前桩实现，返回 `AVDK_ERR_UNSUPPORTED`） |
| **isp_tuning** | `start` / `stop` | 启停 ISP tuning server |
| **isp_dump** | `start` / `stop` | 启停 ISP dump server |
| | `sns_read` / `sns_write` | I2C 读写传感器寄存器 |

`isp open` 语法：

```text
isp open <mipi|dvp> <mp|sp> <sensor_w> <sensor_h> <fps> <out_w> <out_h> <frame|software|hardware> [output_fmt]
```

- `frame`：整帧输出；`software` / `hardware`：Flexa 分段输出模式
- `output_fmt` 可选，默认 `23`（NV12）

示例：

```text
isp detect
isp open mipi mp 1920 1080 20 1920 1080 frame
isp open dvp mp 1280 720 30 1280 720 frame
isp close mp
```

成功返回 `CMDRSP:OK`，失败返回 `CMDRSP:ERROR`。运行中可观察 ISR 统计：`MP:fps[...] | SP:fps[...]`。

### 3.2 启动行为

上电后**无自动 demo**，完成 `bk_init()`、`media_service_init()`、`bk_frame_buffer_init()` 后等待串口 CLI。

## 4. 编译与运行

### 4.1 编译

```bash
cd <SDK_ROOT>
make bk7259 PROJECT=multimedia/isp_example -j$(nproc)
```

### 4.2 测试用例

更完整的测试矩阵见工程内 `ISP_TEST_CASES.md`，包含：

- MIPI / DVP 单通道 Frame 与 Flexa
- MP + SP 双通道组合
- 缩小 / 放大（放大应失败）边界
- 压力与快速切换场景

## 5. 注意事项

1. `isp_api` 命令用于 API 分步调试骨架，当前未接入真实实现，请勿与 `isp` 高层命令混淆。
2. 双通道同时工作时注意帧缓冲与 PSRAM 占用。
3. Flexa 模式需与下游模块规划 Bond 或读指针同步，参见 [Frame 与 Flexa 模式](../../../developer-guide/vpu/flexa_frame.html)。
