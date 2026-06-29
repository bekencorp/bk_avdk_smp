# Draw OSD 示例工程

* [English](./README.md)

## 1. 项目概述

本项目演示 Beken BK7259 平台上的软件 OSD（On-Screen Display）叠加显示流程。基于 `bk_draw_osd` 组件，将图标（ARGB8888）和字库文字混合到背景帧上，再通过 MIPI DSI 屏显示。混合运算由 `image_scale` 软件实现（`argb8888_to_*_blend` / `*_convert`），无需 GPU。

当前工程提供：

- `bk_draw_osd` 组件接口演示：控制器创建/删除、整组绘制、单图标/单字体绘制、运行时增删改、ioctl 扩展命令
- `osd` CLI 命令族（CLI / `ap_cmd`）
- 一组自检测试用例 `osd test <sub>`（稳定性、并发、多实例、非法参数、非对齐、PSRAM 切换等）
- 7259 显示兼容层 `osd_disp_compat`：把 7258 风格的 `frame_buffer_t` + `frame_buffer_display_malloc/free` 适配到 7259 的 DSI 显示栈（`bk_display_dsi_bus / panel / dpu` + `bk_frame_buffer_*` + `bk_display_flush`）
- `.it.csv` 集成测试入口

### 1.1 测试环境

   * 硬件配置：
      * 核心板，**BK7259_QF128_12.3X12.3_V4.0**
      * PSRAM 32M
   * 默认 MIPI 屏：`ST7701SN MIPI 480x854`
      * 背景帧格式：`PIXEL_FMT_RGB565_LE`
      * 图标资源格式：`ARGB8888`

> 提示：请使用参考外设进行 demo 工程的熟悉和学习。如果外设规格不一样，代码可能需要重新配置。

## 2. 目录结构

项目采用 AP-CP 双核架构，OSD 业务代码位于 AP 目录下。项目结构如下：

```
draw_osd_example/
├── CMakeLists.txt            # 项目级 CMake 构建文件
├── Makefile                  # Make 构建文件
├── README.md                 # 项目说明文档（英文）
├── README_CN.md              # 项目说明文档（中文）
├── .it.csv                   # 集成测试用例
├── pj_config.mk              # 项目配置
├── ap/                       # AP 端代码
│   ├── CMakeLists.txt        # AP 端 CMake（注册源码 + 拷贝 .it.csv）
│   ├── ap_main.c             # AP 主入口：bk_init / media_service_init / 注册 osd CLI
│   ├── assets/               # UI 工具生成的资源
│   │   ├── bk_img.c          # 图标资源（ARGB8888）
│   │   ├── bk_font.c         # 字库资源
│   │   ├── blend_dsc.c       # 资源数组 blend_assets / blend_info
│   │   └── blend.h           # 资源头文件
│   ├── config/bk7259_ap/     # AP 配置（含 CONFIG_MEDIA_OSD=y）
│   └── draw_osd/
│       ├── include/          # draw_osd_test.h / draw_osd_complex_test.h / osd_disp_compat.h
│       └── src/
│           ├── draw_osd_cli.c            # osd CLI 命令实现
│           ├── draw_osd_complex_test.c   # osd test 自检用例集
│           └── osd_disp_compat.c         # 7259 显示/帧缓冲兼容层
├── cp/                       # CP 端代码
└── partitions/bk7259/        # 分区配置
```

## 3. 功能说明

### 3.1 主要功能

- 基于 `bk_draw_osd` 组件，将图标 + 文字混合到 RGB565 背景帧并刷屏
- 提供 `osd` CLI，覆盖控制器全生命周期与单元素绘制
- 提供 `osd test <sub>` 自检用例，验证健壮性与边界（结果以 `==== [name] PASS ====` 标识）
- 通过 `osd_disp_compat` 抽象 7259 DSI 显示，业务代码与 7258 保持一致的调用风格

### 3.2 OSD 资源

资源由 UI 工具生成，位于 `ap/assets/`：

- `blend_assets`：全部可用资源数组（图标 + 字体），必须以 `{.addr = NULL}` 结尾
- `blend_info`：上电默认绘制的资源子集
- 每个资源带 `name`（同类标签，可重复）与 `content`（具体内容，用于区分）

## 4. 编译与运行

### 4.1 编译方法

```bash
make bk7259 PROJECT=multimedia/draw_osd_example -j32
```

产物位于 `build/bk7259/draw_osd_example/`，整包固件为 `package/all-app.bin`。

### 4.2 运行方法

编译完成后将固件烧录到开发板。上电后串口会打印 `draw_osd_example m55 running...`，随后可通过串口终端发送 `osd` 命令。

- 命令派发成功打印：`CMDRSP:OK`
- 命令派发失败打印：`CMDRSP:ERROR`

> `CMDRSP:OK` 仅表示命令被正确派发；`osd test` 用例最终是否通过，请看 `==== [name] PASS ====` 日志。

#### 4.2.1 当前 CLI 命令

```text
osd init                         # 创建 OSD 控制器 + 打开显示
osd array [update <name> <content> | remove <name>]
                                 # 绘制整组资源（可选先增删改再绘制）
osd img                          # 绘制单个图标（wifi）
osd font                         # 绘制单个字体（clock）
osd get_info  [no_print]         # 查询当前已添加的 OSD 元素
osd get_assets [no_print]        # 查询全部可用资源
osd test <sub>                   # 运行自检用例（见下）
osd deinit                       # 删除控制器 + 关闭显示
```

`osd test <sub>` 支持的子命令：

```text
osd test invalid_param           # 非法参数防护
osd test cfg_unchanged           # 配置不被内部篡改
osd test unaligned               # 非对齐尺寸/坐标
osd test psram_switch            # 运行时 PSRAM/SRAM 切换
osd test shrink                  # 主动释放内部缓冲后再绘制
osd test concurrent [sec]        # 多线程并发绘制（默认 10s）
osd test stability  [N]          # 反复创建/绘制/删除（默认 20 轮）
osd test multi_inst              # 多控制器实例并存
osd test null_assets             # 空资源数组容错
osd test all                     # 顺序跑全部用例
```

> `stability / multi_inst / null_assets` 自带 handle，可独立运行；其余子命令需先执行 `osd init`。

## 5. 测试示例

上电后预期日志：

```text
draw_osd_example m55 running...
```

绘制整组 OSD：

```text
ap_cmd osd init
ap_cmd osd array
```

运行全部自检用例（需先 init）：

```text
ap_cmd osd init
ap_cmd osd test all
```

预期最终日志：

```text
==== [all] ALL TESTS PASS ====
```

### 5.1 集成测试命令

`.it.csv` 共 18 条用例，覆盖：reboot 自检、基础绘制（init/array/img/font/get_info/get_assets/deinit）、以及全部 `osd test` 自检子命令。回归框架按行顺序发送命令并在超时内匹配“期望结果”子串：

```text
reboot                           -> draw_osd_example m55 running
ap_cmd osd test invalid_param    -> [invalid_param] PASS
ap_cmd osd test all              -> [all] ALL TESTS PASS
...
```

## 6. 配置选项

- 组件开关：`CONFIG_MEDIA_OSD=y`（`ap/config/bk7259_ap/defconfig`）
- 默认屏：`ST7701SN MIPI 480x854`（`CONFIG_LCD_ST7701SN_MIPI_480x854=y`）
- 背景帧：`OSD_BG_W = 480`、`OSD_BG_H = 854`、`PIXEL_FMT_RGB565_LE`（见 `osd_disp_compat.h`）
- 绘制内存池：`draw_in_psram` 默认 `false`（SRAM），可经 `OSD_CTLR_CMD_SET_PSRAM_USAGE` 运行时切换

## 7. 注意事项

1. `blend_assets` / `blend_info` 必须以 `{.addr = NULL}` 结尾；组件内部不深拷贝，资源指针需在 handle 生命周期内始终有效（建议放 const/全局区）。
2. 图标混合只支持 `ARGB8888` 源；字体需用 FontCvt.exe 生成。
3. OSD 元素坐标 + 尺寸不能超出背景帧边界，否则绘制失败。
4. `add_or_updata` / `remove` 只改内存数组，需重新调用 `bk_draw_osd_array` 才会生效。
5. 不要每帧切换 PSRAM/SRAM；切换会触发释放旧池 + 在新池重新分配，开销较大，仅用于偶发场景。
6. 显示走 7259 DSI 栈，`osd_disp_compat` 已封装初始化与刷新；如换屏需同步调整该兼容层与 defconfig。
