# RGB Display示例项目

* [English](./README.md)

## 功能概述
- 实现display模块刷屏功能
- 支持输入数据源RGB565,RGB888和YUYV数据格式

* 有关display应用开发，请参阅：

  - `Display 开发指南 <../../../developer-guide/display/rgb_lcd_display.html>`_

* 有关lcd device添加请参阅：

  - `LCD设备添加指南 <../../developer-guide/display/lcd_devices.html>`_

* 有关API参考，请参阅：

  - `Display API <../../../api-reference/multimedia/bk_display.html>`_

## 项目结构

```
rgb_lcd_example/
├── ap/
│   ├── rgb_lcd/
│   │   ├── src/
│   │   │   └── rgb_lcd_cli.c        # CLI命令行接口实现
│   │   └── include/
│   │       └── rgb_lcd_test.h       # 测试接口头文件
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

## 功能说明

本示例提供了两种测试模式：

**1. 单独API接口测试：**
- 单独测试每个Display API的功能
- 包括：init、open、flush、close、delete
- 用于学习和理解每个API的作用

**2. 组合功能测试：**
- 完整的显示流程测试
- 从初始化到关闭的完整过程
- 连续刷新多帧随机颜色

## 编译与运行

### 编译方法

使用以下命令编译项目：

```bash
make bk7258 PROJECT=rgb_lcd_example
```

### 运行方法

编译完成后，将生成的固件烧录到开发板上，然后通过串口终端使用以下命令测试显示功能：

命令执行成功打印：`CMDRSP:OK`

命令执行失败打印：`CMDRSP:ERROR`


#### 3.2.1 API测试命令

1. 创建display实例:

```
lcd_display_api init
```

2. 初始化硬件display模块：

```
lcd_display_api open
```

3. 数据刷屏：

```
lcd_display_api flush
```


4. 关闭硬件display模块：

```
lcd_display_api close
```

5. 删除显示实例：

```
lcd_display_api delete
```

#### 3.2.2 功能测试

1. 刷屏测试

```
lcd_display open
```

- CASE成功标准：

```
屏幕显示几张随机颜色后，稳定最后的颜色刷屏显示
```
- CASE成功日志：

```
ap0:media_se:D(104282):h264:0[0], dec:0[0], lcd:58[149], lcd_fps:3[8], lvgl:0[0]
ap0:media_se:D(104282):wifi:0[0, 0kbps, 0ms, 0-0], jpg:0KB[0Kbps], h264:0KB[0Kbps]

ap0:rgb_main:D(104805):bk_display_flush frame success!
CMDRSP:OK
```
- CASE失败标准：

```
屏幕没有显示刷屏
```
- CASE失败日志：

```
CMDRSP:ERROR
```

2. 关闭刷屏测试

```
lcd_display close
```

- CASE成功标准：

```
屏幕关闭显示
```
- CASE成功日志：

```
bk_display_delete success!
CMDRSP:OK
```

- CASE失败标准：

```
屏幕没有关闭显示
```

- CASE失败日志：

```
CMDRSP:ERROR
```


## 实现细节

### Display模块功能

**像素格式转换：**
- 支持YUYV转RGB565/RGB888/RGB666
- 支持RGB565转RGB888
- 支持RGB888转RGB565
- 支持RGB565转RGB666
- 支持RGB888转RGB666
- 自动转换，用户只需设置frame->fmt即可

**分辨率处理：**
- 源数据分辨率大于LCD时，自动裁剪中间区域显示
- 源数据分辨率必须 >= LCD分辨率
- 建议源数据与LCD分辨率一致以获得最佳显示效果

**RGB565格式说明：**
- 使用GPIO高位输出：R3-R7、G2-G7、B3-B7
- 16位数据格式：R(5bit) G(6bit) B(5bit)

### 内存管理

- 所有帧缓冲区从显示内存池分配（`frame_buffer_display_malloc`）
- 使用回调函数 `display_frame_free_cb` 自动释放帧
- 帧释放时机：
  - 多帧模式：下一帧刷新时释放前一帧
  - 单帧模式：close时释放

### LCD设备配置

示例使用ST7701SN设备（480x854），配置包括：
- RGB时序参数（hsync/vsync porch、pulse width）
- 时钟频率（LCD_30M）
- 数据输出边沿（NEGEDGE_OUTPUT）
- 初始化命令序列

其他LCD设备添加请参考LCD设备添加指南文档。

### API调用顺序

**单独API测试模式：**
1. `lcd_display_api init` - 创建控制器
2. `lcd_display_api open` - 打开硬件
3. `lcd_display_api flush` - 刷新显示
4. `lcd_display_api close` - 关闭硬件
5. `lcd_display_api delete` - 删除控制器

**组合功能测试模式：**
- `lcd_display open` - 自动完成init+open+flush流程
- `lcd_display close` - 自动完成close+delete流程

## 注意事项

**硬件配置：**
1. LCD LDO引脚（默认GPIO13）必须在打开显示前拉高
2. 背光引脚（默认GPIO7）需要配置为输出并拉高
3. 如果硬件配置不同，需要修改GPIO引脚定义
4. SPI初始化引脚根据LCD设备要求配置

**显示问题排查：**

1. 屏幕不亮请检查：

   - 背光IO是否打开（GPIO7）
   - LCD LDO是否使能（GPIO13）
   - LCD设备配置是否正确
   - 时序参数是否符合屏幕规格

2. 显示异常请检查：

   - 源数据分辨率是否 >= LCD分辨率
   - 像素格式配置是否正确
   - 时钟频率是否合适

**内存管理：**
- 帧释放回调函数free_cb不能为NULL
- 回调中必须正确释放帧内存
- flush失败时需要手动释放帧

**性能优化：**
- 选择合适的像素格式（RGB565内存占用小）
- 避免不必要的格式转换
- 保持控制器打开状态进行连续刷屏


