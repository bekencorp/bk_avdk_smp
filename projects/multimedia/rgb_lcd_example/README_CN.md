# RGB LCD 示例工程

* [English](./README.md)

## 1. 项目概述

本项目用于演示 Beken 平台上的 RGB 接口 LCD 显示流程。基于 `bk_display_*` 控制器接口，演示显示初始化、帧刷新线程以及像素格式运行时切换。

当前工程提供：

- 上电自动运行的 RGB565 显示自检：`rgb_lcd_display_rgb565_test`
- 像素格式运行时切换测试：`rgb_lcd_switch_format`（CLI / `ap_cmd`）
- `.it.csv` 集成测试入口

### 1.1 测试环境

   * 硬件配置：
      * 核心板，**BK7259_QF128_12.3X12.3_V4.0**
      * PSRAM 32M
   * 默认 RGB 屏：`st7701sn_rgb_480x854`（分辨率 480x854）
      * 上电默认格式：`RGB565`
      * 支持格式：`RGB565` 与 `ARGB8888`（压缩数据）

.. warning::

    请使用参考外设，进行 demo 工程的熟悉和学习。如果外设规格不一样，代码可能需要重新配置。

## 2. 目录结构

项目采用 AP-CP 双核架构，主要源代码位于 AP 目录下。项目结构如下：

```
rgb_lcd_example/
├── CMakeLists.txt        # 项目级 CMake 构建文件
├── Makefile              # Make 构建文件
├── README.md             # 项目说明文档（英文）
├── README_CN.md          # 项目说明文档（中文）
├── app.rst               # 工程说明
├── .it.csv               # 集成测试用例
├── ap/                   # AP 端代码
│   ├── CMakeLists.txt    # AP 端 CMake 构建文件
│   ├── ap_main.c         # AP 主入口、CLI 命令注册、上电 case
│   ├── config/           # AP 配置目录
│   ├── include/
│   │   └── lcd_example.h # 接口声明
│   └── src/
│       └── lcd_example_rgb.c # RGB 显示开关、刷新线程、两个 IT case
├── cp/                   # CP 端代码
├── partitions/           # 分区配置
└── pj_config.mk          # 项目配置
```

## 3. 功能说明

### 3.1 主要功能

- 上电后自动运行 RGB565 显示 case
- 提供 `rgb_lcd_switch_format` CLI 命令验证像素格式运行时切换
- 使用直接 `malloc -> 填充 -> flush -> 延时` 的刷新线程（无队列）
- 每个 case 显示固定时长后自动关闭，保证用例相互独立
- 提供统一 `[RESULT][PASS]` / `[RESULT][FAIL]` 结果日志

### 3.2 测试用例说明

#### Case 1：上电 RGB565 显示（`rgb_lcd_display_rgb565_test`）

1. 上电时 `main()` 自动调用 `rgb_lcd_rgb565_test()`
2. 以 `RGB565` 打开屏并启动刷新线程
3. 持续刷新约 20s（`RGB_LCD_BOOT_HOLD_MS`）
4. 自动关闭显示，打印：
   `[RESULT][PASS] rgb_lcd_display_rgb565_test success`

#### Case 2：像素格式运行时切换（`rgb_lcd_switch_format`）

1. 由 CLI / `ap_cmd rgb_lcd_switch_format` 触发
2. 通过 `bk_display_ioctl` 运行时切换格式，顺序为：
   `RGB565 -> ARGB8888 -> RGB565`，每档显示约 5s（`RGB_LCD_SWITCH_STEP_MS`，合计约 15s）
3. 切换完成后自动关闭显示，打印：
   `[RESULT][PASS]rgb_lcd_switch_format success`

> 说明：RGB 屏仅支持 `RGB565` 与 `ARGB8888`，其中 `ARGB8888` 按压缩数据处理（`decompress=true`）。

## 4. 编译与运行

### 4.1 编译方法

使用以下命令编译项目：

```bash
make bk7259 PROJECT=multimedia/rgb_lcd_example
```

### 4.2 运行方法

编译完成后，将生成的固件烧录到开发板上。上电后 `main()` 会自动运行 Case 1（RGB565 显示）。也可以通过串口终端手动触发切换测试：

命令执行成功打印："CMDRSP:OK"

命令执行失败打印："CMDRSP:ERROR"

#### 4.2.1 当前 CLI 命令

```text
rgb_lcd_switch_format
```

- `rgb_lcd_switch_format`：运行时切换 `RGB565 / ARGB8888` 验证切换 API
- `CMDRSP:OK` 只表示测试线程创建成功，最终是否通过请看 `[RESULT]` 日志

## 5. 测试示例

### 5.1 上电 RGB565 显示测试

设备上电（或发送 `reboot`）后自动运行，预期最终日志：

```text
[RESULT][PASS] rgb_lcd_display_rgb565_test success
```

### 5.2 像素格式切换测试

```text
ap_cmd rgb_lcd_switch_format
```

预期最终日志：

```text
[RESULT][PASS]rgb_lcd_switch_format success
```

### 5.3 集成测试命令

`.it.csv` 包含：

```text
reboot                          -> [RESULT][PASS] rgb_lcd_display_rgb565_test success
ap_cmd rgb_lcd_switch_format    -> [RESULT][PASS]rgb_lcd_switch_format success
```

## 6. 配置选项

### 6.1 显示配置

当前 RGB 测试使用以下固定配置：

- 默认屏：`st7701sn_rgb_480x854`
- 分辨率：`480 x 854`
- 上电格式：`BK_PIXEL_FORMAT_RGB565`
- 支持格式：`BK_PIXEL_FORMAT_RGB565`、`BK_PIXEL_FORMAT_ARGB8888`（压缩）
- 上电显示保持时长：`RGB_LCD_BOOT_HOLD_MS = 20s`
- 切换每档时长：`RGB_LCD_SWITCH_STEP_MS = 5s`

## 7. 注意事项

1. 两个 case 互相独立，case 运行结束会自动关闭显示并释放资源；后启动的 case 会抢占前一个。
2. CLI 命令只负责创建测试线程，最终结果以 `[RESULT][PASS]` 或 `[RESULT][FAIL]` 为准。
3. 格式切换通过 `bk_display_ioctl` 运行时完成，期间会先停止刷新线程，切换后再重启刷新。
4. RGB 屏的 `ARGB8888` 按压缩数据处理，帧缓冲仍按 `宽 x 高 x 4` 分配并填满。
5. 帧缓冲资源有限，请避免同时占用过多缓冲区。
