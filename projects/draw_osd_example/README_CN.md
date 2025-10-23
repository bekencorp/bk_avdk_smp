# Draw OSD 示例

* [English](./README.md)

## 概述
本示例展示了BK7258平台中OSD（屏幕显示）控制器的使用方法。它提供了在LCD屏幕上绘制图像、文本和管理显示资源的功能。

* 有关draw_osd 应用的详细信息，请参阅：

  - `draw_osd 开发指南 <../../../developer-guide/display/draw_osd.html>`_

* 有关API参考，请参阅：

  - `draw_osd API <../../../api-reference/multimedia/bk_draw_osd.html>`_

## 支持的功能
- **OSD控制器管理**：初始化和反初始化OSD控制器
- **图像绘制**：在帧缓冲区上绘制图像
- **字体渲染**：在帧缓冲区上显示文本
- **资源管理**：添加、更新、删除和查询显示资源
- **信息查询**：获取当前绘制信息和可用资源
- **PSRAM支持**：配置PSRAM和SRAM之间的内存使用

## 组件
- **bk_draw_osd**：OSD控制器驱动
- **bk_dma2d**：DMA2D硬件加速（用于图像混合）
- **frame_buffer**：帧缓冲区管理
- **lcd_display**：LCD显示控制器
- **cli**：用于测试的命令行接口

## 项目结构

```
draw_osd_example/
├── ap/
│   ├── draw_osd/
│   │   ├── src/
│   │   │   ├── draw_osd_cli.c       # CLI命令行接口实现
│   │   │   └── blend_assets/        # OSD资源文件
│   │   │       ├── blend.h          # 资源头文件
│   │   │       ├── bk_dsc.c         # 资源描述数组
│   │   │       ├── bk_font.c        # 字体资源
│   │   │       └── bk_img.c         # 图片资源
│   │   └── include/
│   │       └── draw_osd_test.h      # 测试接口头文件
│   ├── ap_main.c                    # AP核心主程序
│   └── config/                      # 配置文件
├── cp/                              # CP核心相关代码
├── partitions/                      # 分区配置
├── README_CN.md                     # 中文说明文档
├── README.md                        # 英文说明文档
└── CMakeLists.txt                   # 构建配置文件
```

## 测试环境

   * 硬件配置：
      * 核心板：**BK7258_QFN88_9X9_V3.2**
      * PSRAM: 8M/16M
      * LCD转接小板
      * RGB接口的LCD，demo中使用ST7701SN 480x854屏幕

.. warning::

    请使用参考外设进行demo工程的熟悉和学习。如果外设规格不一样，代码可能需要重新配置。

## 编译和执行

使用以下命令编译项目：

```bash
make bk7258 PROJECT=draw_osd_example
```

编译完成后，将生成的固件烧录到开发板上，然后通过串口终端使用以下命令测试OSD功能：

命令执行成功打印：`CMDRSP:OK`

命令执行失败打印：`CMDRSP:ERROR`

## 命令行接口
本示例提供了命令行接口用于测试不同的OSD操作。以下是可用的命令：

### 基本命令
```bash
# 初始化OSD控制器和LCD显示
> osd init
```

- CASE成功标准：

```
CMDRSP:OK
```


```bash
# 反初始化OSD控制器和LCD显示
> osd deinit
```

- CASE成功标准：

```
CMDRSP:OK
```

### 绘制资源
```bash
# 显示资源数组并更新显示
> osd array
```

- CASE成功标准：

```
CMDRSP:OK
```


```bash
# 在数组中添加或更新资源
> osd array updata <resource_name> <content>

比如：
  - osd array updata wifi wifi3
  - osd  array updata clock 12:77

```
- CASE成功标准：

```
CMDRSP:OK
```


```bash
# 从数组中删除资源
> osd array remove <resource_name>
```
比如：
 - osd array remove clock
 - osd array remove wifi

- CASE成功标准：

```
CMDRSP:OK
```

### 单独绘制命令
```bash
# 绘制图像
> osd img
```

- CASE成功标准：

```
CMDRSP:OK
```


```bash
# 绘制文本
> osd font
```

- CASE成功标准：

```
CMDRSP:OK
```

### 信息查询命令 
```bash
# 获取当前绘制信息（带日志打印）
> osd info
```

- CASE成功标准：

```
ap0:draw_osd:I(6755910):Dynamic array elements: 6
ap0:draw_osd:I(6755911):Element 1:
ap0:draw_osd:I(6755911):  Name: clock
ap0:draw_osd:I(6755911):  Content: 12:77
ap0:draw_osd:I(6755911):  Type: Font
ap0:draw_osd:I(6755911):  Position: X=0, Y=0
ap0:draw_osd:I(6755911):  Size: Width=120, Height=44
ap0:draw_osd:I(6755911):----------------------------------------
ap0:draw_osd:I(6755911):Element 2:
ap0:draw_osd:I(6755911):  Name: date
ap0:draw_osd:I(6755911):  Content: 
ap0:draw_osd:I(6755911):  Type: Font
ap0:draw_osd:I(6755911):  Position: X=210, Y=0
ap0:draw_osd:I(6755911):  Size: Width=180, Height=24
ap0:draw_osd:I(6755911):----------------------------------------
ap0:draw_osd:I(6755911):Element 3:
ap0:draw_osd:I(6755911):  Name: ver
ap0:draw_osd:I(6755911):  Content: v 1.0.0
ap0:draw_osd:I(6755911):  Type: Font
ap0:draw_osd:I(6755911):  Position: X=300, Y=830
ap0:draw_osd:I(6755911):  Size: Width=180, Height=24
ap0:draw_osd:I(6755911):----------------------------------------

.....................

CMDRSP:OK
```

```bash
# 获取当前绘制信息（无日志打印）
> osd info no_print
```
- CASE成功标准：

```
CMDRSP:OK
```


```bash
# 获取所有可用资源（带日志打印）
> osd assets
```

- CASE成功标准：

```
CMDRSP:OK
```


```bash
# 获取所有可用资源（无日志打印）
> osd assets no_print
```

- CASE成功标准：

```
CMDRSP:OK
```

## 实现细节

### OSD资源管理

示例中的OSD资源通过UI工具生成，包含：

**字体资源：**
- `font_clock`: 时钟显示字体（120x44像素）
- `font_dates`: 日期显示字体（180x24像素）
- `font_ver`: 版本显示字体（180x24像素）

**图像资源：**
- `img_wifi_rssi0-4`: WiFi信号强度图标（5个等级）
- `img_battery1`: 电池图标
- `img_cloudy_to_sunny`: 天气图标

**资源数组：**
- `blend_assets`: 总资源数组，包含所有可用资源
- `blend_info`: 默认绘制的资源数组

### 内存管理

- 所有帧缓冲区通过 `frame_buffer_display_malloc()` 从显示内存池分配
- 使用完成后通过 `frame_buffer_display_free()` 释放
- 对于显示的帧，使用回调函数 `display_frame_free_cb` 自动释放
- OSD绘制缓冲区根据最大OSD元素尺寸自动分配

### OSD绘制流程

1. **初始化阶段：**
   - 创建LCD显示控制器
   - 打开LCD显示
   - 创建OSD控制器并配置资源

2. **绘制阶段：**
   - 分配背景帧缓冲区
   - 调用OSD绘制API（array/image/font）
   - 刷新到LCD显示

3. **清理阶段：**
   - 删除OSD控制器
   - 关闭并删除LCD显示控制器
   - 释放所有资源

### DMA2D加速

OSD绘制内部使用DMA2D硬件加速进行图像混合：
- 字体渲染：使用DMA2D的blend功能
- 图像叠加：使用DMA2D的blend功能
- 支持ARGB8888格式的透明度混合

## 注意事项

**基本使用：**
- 执行操作前必须先使用 `osd init` 命令初始化控制器
- 操作完成后建议使用 `osd deinit` 命令释放资源
- 示例使用ST7701SN 480x854屏幕，使用其他屏幕需修改配置

**资源配置：**
- OSD资源通过UI工具生成，工具地址：https://docs.bekencorp.com/arminodoc/bk_app/ui_designer/zh_CN/latest/index.html
- 图像必须使用ARGB8888格式
- 字体必须使用FontCvt.exe工具生成
- 资源数组必须以 `{.addr = NULL}` 结尾

**内存使用：**
- 可在初始化时配置是否使用PSRAM绘制
- PSRAM适合大尺寸OSD元素
- SRAM绘制速度更快但内存有限
- 确保PSRAM配置正确（8M/16M）

**动态更新：**
- 使用 `osd array updata` 更新内容后需要重新调用 `osd array` 才能显示
- 使用 `osd array remove` 删除后需要重新调用 `osd array` 才能生效
- name相同的元素会被统一管理
- content用于区分同name下的不同资源

**坐标和尺寸：**
- OSD元素坐标基于背景帧的原点(0,0)
- OSD元素不能超出背景帧边界
- 绘制顺序会影响层叠效果