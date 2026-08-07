# 鱼眼标定示例工程

- [English](./README.md)

## 1. 项目概述

本项目在 Beken 平台上演示 **鱼眼稠密重映射表生成**：根据稀疏网格点与 TV 轮廓点，在固定输入分辨率下计算输出分辨率的 `map_x` / `map_y`（`int16`，对源图做近邻采样索引）。工程提供串口 CLI，可测量 **fisheye_calibration()** 耗时，并可选将整张表以十六进制经串口 dump，便于用脚本还原为二进制 map。

- 算法与数据结构见：
  - [fisheye_calibration.h](../../../ap/properties/modules/video_codec/fisheye/fisheye_calibration.h)（组件目录 `ap/properties/modules/video_codec/fisheye/`）

### 1.1 测试环境

- 硬件（与当前工程 `partitions` / `defconfig` 一致）：
  - 核心板，**BK7259** 系列
  - PSRAM（如 `CONFIG_ALL_CODE_IN_PSRAM` 等）
- 软件行为：
  - 输入分辨率在示例代码中固定为 **1920×1080**（`DEFAULT_INPUT_WIDTH` / `DEFAULT_INPUT_HEIGHT`）
  - 输出为稠密表：前半 `map_x`、后半 `map_y`，行主序，共 **2×output_width×output_height** 个 `int16_t`

.. warning::

```
请使用参考外设熟悉 demo。若镜头、安装方式或分辨率与标定数据不一致，需在源码中替换网格点、轮廓点及输入宽高，并重新验证。
```

## 2. 目录结构

```
fisheye_example/
├── CMakeLists.txt        # 工程级 CMake（含 fisheye 组件路径）
├── Makefile
├── README.md
├── README_CN.md
├── hexlog_to_bin.py      # 将串口 hex dump 转为原始 bin
├── ap/
│   ├── CMakeLists.txt
│   ├── ap_main.c
│   ├── fisheye_cli.c     # CLI：fisheye cal …，内部调用 fisheye_calibration()
│   ├── fisheye_cli.h
│   └── config/bk7259_ap/defconfig
├── cp/
└── partitions/bk7259/
```

## 3. 功能说明

### 3.1 主要功能

- 调用 **fisheye_calibration()** 生成稠密 `map_x` / `map_y`
- 标定数据以静态数组写在 `fisheye_cli.c`：**稀疏网格** `k_fisheye_grid_points[]`、**TV 轮廓 7 点** `k_tv_points_001[]`（`FISHEYE_MAP_POINTS`）
- 输出缓冲：`bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, …)`，用完 `bk_frame_buffer_free`
- 串口打印 **fisheye calibration execute time**；dump=1 时整表十六进制输出（数据量大、耗时长，代码中会插入短延时以减轻串口压力），并打印 **fisheye calibration dump execute time**

### 3.2 处理流程

1. `bk_init()` → `media_service_init()` → `bk_auxldo_enable()` → `bk_frame_buffer_init()` → **fisheye_cli_init()**
2. 用户执行 **ap_cmd fisheye cal …**
3. 分配缓冲，调用 **fisheye_calibration()**
4. 打印耗时；若 dump，再打印 dump 段耗时
5. 释放缓冲

## 4. 编译与运行

### 4.1 编译

在 SDK 根目录：

```
make bk7259 PROJECT=multimedia/fisheye_example
```

### 4.2 串口与终端

- **Log 在 CP 端打印**，调试时请连接用于 CP 日志输出的串口查看 `fisheye` 相关打印与耗时。
- 当前工程串口波特率为 **460800**，PC 端串口终端须设置为相同波特率，否则会出现乱码或丢字。

### 4.3 CLI

本工程在终端中执行 CLI 时，须在命令**最前面**加上 **ap_cmd**（由 shell 转发至 AP）。

```
ap_cmd fisheye cal <dump> <width> <height>
```


.. list-table::
   :header-rows: 1

   * - 参数
     - 含义
     - 缺省
   * - `cal`
     - 子命令，执行一次标定/建表
     - 必填
   * - `dump`
     - `0` 不 dump，`1` 十六进制输出全表
     - `0`
   * - `width` / `height`
     - 输出分辨率（表尺寸）
     - **320×180**


说明：若未指定宽高、为 0，或超出输入 1920×1080 范围，则回退 **320×180**。若同时指定宽高，须带上 `dump` 参数顺序，例如：`ap_cmd fisheye cal 0 640 480`。

### 4.4 期望打印（ap_cmd fisheye cal 0）

执行 **ap_cmd fisheye cal 0**（默认输出 **320×180**、**不** dump 十六进制）时，在 **CP 日志串口**上可出现类似输出。时间戳与耗时随运行环境变化，**245 ms** 为与下表一致的参考值，以实机为准。

```
$ap0:fisheye:I(437615):fisheye calibration, dump: 0, output_width: 320, output_height: 180
fisheye calibration execute time: 245 ms
```

说明：`fisheye calibration, dump:` 后的数字与命令中第二个参数一致（`cal 0` → `dump: 0`）；`bk_printf_raw` 打印的 `fisheye calibration execute time` 行无 `$ap0:` 前缀属正常现象。

### 4.5 从串口 dump 提取 map（hexlog_to_bin.py）

将 **dump=1** 时保存的日志（仅含纯十六进制行，脚本会跳过其它 log 行）转为原始二进制，便于离线保存或与分辨率对应的 **2×W×H×2 字节**（`int16` map）对齐使用：

```bash
python3 hexlog_to_bin.py fisheye-320-180.log fisheye-320-180.bin
```

- 第一个参数：从串口复制的 **.log** 文件  
- 第二个参数：输出的 **.bin**（原始字节流，顺序与设备内存一致：`map_x` 后接 `map_y`）

在工程目录下执行时：

```bash
cd projects/multimedia/fisheye_example
python3 hexlog_to_bin.py fisheye-320-180.log fisheye-320-180.bin
```

## 5. 分辨率与耗时参考

以下为固定板级与串口条件下测得的 **fisheye_calibration() 耗时**（`correct_time`）与 **整表 hex dump 总耗时**（`dump_time`，含串口打印与代码中的节流延时）。不同板卡、主频、串口波特率下数值会变化，仅作对比参考。


.. list-table::
   :header-rows: 1

   * - 分辨率
     - correct_time
     - dump_time
   * - 320×180
     - 245 ms
     - 18245 ms
   * - 640×480
     - 1199 ms
     - 97200 ms
   * - 1920×1080
     - 7850 ms
     - 655850 ms


## 6. 配置说明（fisheye_calibration()）

- `gp` / `gpoints`：稀疏网格  
- `std_map_points` / `user_map_points`：各 7 个 TV 轮廓点（本示例两处均用 `k_tv_points_001`）  
- `input_width` / `input_height`：**1920** / **1080**  
- `output_width` / `output_height`：由 CLI 指定  
- `output`：**2×output_width×output_height** 个 `int16_t`

AP 依赖组件 **fisheye**；顶层 `CMakeLists.txt` 中 `EXTRA_COMPONENTS_DIRS` 指向 `ap/properties/modules/video_codec/fisheye`。

## 7. 注意事项

1. 标定数据与 **1920×1080** 输入绑定；更换传感器或分辨率需更新网格与轮廓点。
2. **dump** 仅用于调试或离线取表；大分辨率下 dump 时间可达分钟级。
3. **日志在 CP 端打印**，请使用 CP 对应串口观察输出。
4. 输入命令时须在**最前面**加 **ap_cmd**，例如：`ap_cmd fisheye cal 0`。

