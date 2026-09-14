# ISP + 屏显示例工程（TP2863 双路）

* [English](./README.md)

## 概述

TP2863 双路 AHD 输入 + ISP VC mux + GPU 屏显预览：

```
TP2863 ch1/ch2 -> MIPI VC0/VC1 -> ISP MP (frame) -> vc_mux -> GPU -> LCD (1080x1920)
```

本工程显示链路与 `doorbell_lp` 对齐：**MP frame 模式 + vc_mux peek + frame GPU**，不再使用 flexa bond 直连 GPU。

## 启动顺序要求

### 1. 固件内部初始化顺序（与 doorbell 一致）

TP2863 为 `YUYV_SWAP` 输入，MIPI/ISP 时序敏感。代码中必须保证：

1. 摄像头 LDO 上电
2. I2C bus 使能、sensor 检测
3. 从 sensor 格式表填充 `input_pixel_fmt`（TP2863 为 `YUYV_SWAP`）
4. **`bk_camera_sensor_init` + `set_format`（配置 TP2863 / MIPI CSI）**
5. 再执行 ISP `port_init` → MP `channel_open`
6. 最后启动显示：`vc_mux` + frame GPU + LCD

**禁止**在 ISP 通道已打开后再首次 `set_format`（会触发 `mipi_csi_controller_reset`，易导致偏色/花屏）。

单路 `isp open mp` 与双路 `open_vc_route` 均遵循上述顺序。

#### 关于「打开 sensor 时配置 swap」

TP2863 **没有**独立的 sensor 侧 byte-swap 寄存器；颜色相关的 swap 由 **ISP 输入像素格式** 决定：

| 层级 | 配置项 | TP2863 取值 | 作用 |
|------|--------|-------------|------|
| Sensor 格式表 | `output_pixel_fmt` | `BK_PIXEL_FORMAT_YUYV_SWAP` | 声明 MIPI 输出字节序 |
| ISP port | `input_pixel_fmt` | 从格式表拷贝 | `port_init` 写入 ISP port |
| ISP MI（MP 出 NV12） | `data_swap` | 自动为 `3` | port 为 `YUYV_SWAP` 且通道输出 NV12 时由 `vsi_isp_miv10.c` 设置 |

因此「打开 sensor 时配置 swap」的正确做法是：**在 `port_init` 之前**，用 `bk_camera_sensor_query_support_formats()` 匹配 (w,h,fps)，把对应项的 `output_pixel_fmt` 填入 `isp_ctlr_config.input_pixel_fmt`（本工程在 sensor init 前已完成）。

```c
/* 示例：swap 隐含在 pixel format 里，不是单独 IOCTL */
isp_cfg.input_pixel_fmt = fmt_arr.format_array[i].output_pixel_fmt;  /* YUYV_SWAP */
bk_camera_sensor_init(...);
bk_camera_sensor_set_format(...);   /* 先配 MIPI */
bk_isp_camera_port_init(..., &isp_cfg);  /* 再配 ISP port → MI 知悉 swap */
bk_isp_camera_channel_open(...);    /* MP NV12 输出时 data_swap=3 生效 */
```

**不能**用「sensor 已开、ISP 已跑后再补 swap」替代上述顺序：`set_format` 会 reset MIPI，与 ISP 并行时仍会偏色；swap 位本身在 `port_init` 时已确定，channel 打开后改 port 格式需关通道重来，不推荐。

若换用 `YUYV`（非 SWAP）的 sensor，只需在对应 `xxx_format_array[]` 改 `output_pixel_fmt`，ISP 侧会自动取消 `data_swap=3`，无需改 MI 寄存器。

### 2. CLI 推荐使用顺序

#### 双摄像头切换（推荐，与 doorbell_lp 相同）

必须按顺序执行；`open_vc_route` 只建链路和 LCD，**不会自动打开某路 VC 输入**，需显式 `vc_route_enable`：

```text
isp detect                          # 可选，确认 TP2863
isp open_vc_route 1280 720 25 1280 720 0
isp vc_route_enable 0               # 启用 VC0 输入（discard 可选）
isp vc_route_vc 0                   # LCD 显示 VC0（切换显示路时改 0/1）
isp vc_route_enable 1               # 需要第二路时再 enable
isp vc_route_vc 1                   # 切到 VC1 显示
isp vc_route_disable 1              # 不用时可 disable
isp close_vc_route                  # 关闭预览（先关再开单路）
```

| 步骤 | 命令 | 说明 |
|------|------|------|
| 1 | `isp open_vc_route [sw] [sh] [fps] [iw] [ih] [vc]` | 打开 camera + MP(frame) + vc_mux + GPU + LCD；`vc` 为默认显示 VC |
| 2 | `isp vc_route_enable <0\|1> [discard]` | **必须**：打开对应 VC 的 MIPI 输入，否则无帧 |
| 3 | `isp vc_route_vc <0\|1>` | 选择 LCD 显示哪一路（不影响 enable 状态） |
| 4 | `isp close_vc_route` |  teardown 显示与 camera |

#### 单路摄像头

与双路 **互斥**；若已 `open_vc_route`，须先 `close_vc_route`：

```text
isp open mp 1280 720 25 1280 720 frame
isp close mp
```

单路 MP 同样为 frame 模式 + vc_mux + frame GPU（内部自动 `vc_route_enable 0`）。

### 3. 模式互斥

| 模式 | 打开 | 关闭 |
|------|------|------|
| 单路 MP 屏显 | `isp open mp ... frame` | `isp close mp` |
| 双路 VC 屏显 | `isp open_vc_route ...` | `isp close_vc_route` |

两种模式不可同时开启。

### 4. 板级与 Kconfig

- 板级引脚（doorbell_lp）：I2C1 GPIO69/70，reset GPIO71，`pin_xclk=INVALID`
- `defconfig`：`CONFIG_CSI_TP2863`、`CONFIG_GPU`、`CONFIG_DPU`、hx8399c 1080×1920

## 命令参考

| 命令 | 说明 |
|------|------|
| `isp open_vc_route [sw] [sh] [fps] [iw] [ih] [vc]` | 打开双路 VC 路由 + LCD 显示 |
| `isp vc_route_vc <0\|1>` | 切换 LCD 显示 VC |
| `isp vc_route_enable <0\|1> [discard]` | 启用某路 VC 输入 |
| `isp vc_route_disable <0\|1>` | 禁用某路 VC 输入 |
| `isp close_vc_route` | 关闭双路预览 |
| `isp open mp <...> frame` | 单路 MP 屏显 |
| `isp close mp` | 关闭单路 MP |

## 编译

```bash
cd <SDK_ROOT>
CCACHE_DISABLE=1 make bk7259 PROJECT=multimedia/isp_display_example -j$(nproc)
```

## 自动化测试

工程目录含 `.it.csv`，可在 workspace 根配置 `armino_ai_coding.env`（远程 BKFIL + 串口）后执行：

```bash
bash .cursor/skills/multimedia-tm/auto-test/scripts/run_auto_test.sh \
  --project multimedia/isp_display_example \
  --port COM18 \
  --sdk-dir bk_avdk_smp_release_4.0.1
```

远程板卡 PC 示例：`BKFIL_MODE=remote`，`BKFIL_REMOTE_HOST=192.168.31.35`，`BKFIL_PORT=COM18`。
