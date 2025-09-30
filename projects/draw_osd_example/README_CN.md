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
- **frame_buffer**：帧缓冲区管理
- **lcd_display**：LCD显示控制器
- **cli**：用于测试的命令行接口

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

## 测试环境
- 开发板：BK7258
- LCD面板：ST7701SN（480x864分辨率）
- 编译器：ARM GCC
- 构建系统：CMake

## 编译和执行
1. 使用提供的Makefile或CMake构建项目
2. 将固件烧录到开发板
3. 使用串口终端访问命令行接口
4. 执行上述描述的OSD命令

## 注意事项
- 执行操作前确保已初始化OSD控制器
- 操作完成后反初始化OSD控制器
- 示例使用从显示内存分配的帧缓冲区
- 支持RGB565像素格式进行显示
- 可在初始化期间配置PSRAM使用