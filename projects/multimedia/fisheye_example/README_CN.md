# 鱼眼标定示例工程

- [English](./README.md)

## 1. 项目概述

本项目在 Beken 平台上演示 **鱼眼稠密重映射表生成**：根据固定 OpenCV fisheye 相机内参（`K/D`）与 TV 轮廓 7 点，在固定输入分辨率下计算输出分辨率的 `map_x` / `map_y`（`int16`，对源图做近邻采样索引）。工程提供串口 CLI，可测量 **fisheye_calibration()** 耗时，并可选将整张表以十六进制经串口 dump，便于用脚本还原为二进制 map。

- 算法与数据结构见：
  - [fisheye_calibration.h](../../../ap/include/modules/fisheye_calibration.h)

### 1.1 测试环境

- 硬件（与当前工程 `partitions` / `defconfig` 一致）：
  - 核心板，**BK7259** 系列
  - PSRAM（如 `CONFIG_ALL_CODE_IN_PSRAM` 等）
- 软件行为：
  - 输入分辨率在示例代码中固定为 **1920×1080**（`DEFAULT_INPUT_WIDTH` / `DEFAULT_INPUT_HEIGHT`）
  - 输出为稠密表：前半 `map_x`、后半 `map_y`，行主序，共 **2×output_width×output_height** 个 `int16_t`

.. warning::

```
请使用参考外设熟悉 demo。若镜头、安装方式或分辨率与标定数据不一致，需在源码中替换相机内参、轮廓点及输入宽高，并重新验证。
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
- 标定数据以静态数组写在 `fisheye_cli.c`：来自 `my_camera_new.json` 的 **`k_demo_camera_params`**、来自 `tv_corners.txt` 的 **TV 轮廓 7 点** `k_tv_points_001[]`（`FISHEYE_MAP_POINTS`）
- 输出缓冲：`bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, …)`，用完 `bk_frame_buffer_free`
- 串口打印 **fisheye calibration execute time**；dump=1 时整表十六进制输出（数据量大、耗时长，代码中会插入短延时以减轻串口压力），并打印 **fisheye calibration dump execute time**

### 3.2 处理流程

1. `bk_init()` → `media_service_init()` → `bk_auxldo_enable()` → `bk_frame_buffer_init()` → **fisheye_cli_init()**
2. 上电后自动创建低优先级 `fisheye_boot` task，延时 2 秒后执行默认 **320×180 / dump=0** 标定。
3. 用户也可以通过 CLI 手动执行 **ap_cmd fisheye cal …**
4. 分配缓冲，调用 **fisheye_calibration()**
5. 打印结果信息；若开启 dump，则打印 raw map buffer。
6. 释放缓冲

## 4. 编译与运行

### 4.1 编译

在 SDK 根目录：

```bash
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

常用调试命令：

```bash
ap_cmd fisheye cal 0 320 180
ap_cmd fisheye cal 1 320 180
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

执行 **ap_cmd fisheye cal 0**（默认输出 **320×180**、**不** dump 十六进制）时，在 **CP 日志串口**上可出现类似输出。时间戳、耗时、fallback 标志、残差与覆盖率会随输入数据和板端性能变化。

```
$ap0:fisheye:I(437615):fisheye calibration v19, dump: 0, output_width: 320, output_height: 180
fisheye calibration execute time: <duration_ms> ms, fallback: 0, max_error: 1.234, coverage: 0.995469
```

说明：`fisheye calibration, dump:` 后的数字与命令中第二个参数一致（`cal 0` → `dump: 0`）；`bk_printf_raw` 打印的 `fisheye calibration execute time` 行无 `$ap0:` 前缀属正常现象。

### 4.5 从串口 dump 提取 map（hexlog_to_bin.py）

将 **dump=1** 时保存的日志转为原始二进制。dump 行包含 `0xNN` byte token，`hexlog_to_bin.py` 只解析这些 token，并跳过其它 log 文本，便于离线保存或与分辨率对应的 **2×W×H×2 字节**（`int16` map）对齐使用：

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

## 5. 配置说明（fisheye_calibration()）

- `camera`：固定 OpenCV fisheye K/D 参数（本示例为 `k_demo_camera_params`）  
- `tv_points`：当前安装位置下的 TV 轮廓 7 点（本示例为 `k_tv_points_001`）  
- `input_width` / `input_height`：**1920** / **1080**  
- `output_width` / `output_height`：由 CLI 指定  
- `output`：**2×output_width×output_height** 个 `int16_t`

demo 中的镜头内参是按 **1920×1080** 输入画面标定的。`DEFAULT_INPUT_WIDTH` / `DEFAULT_INPUT_HEIGHT` 必须与相机 JSON 中的 `calib_dimension` / `orig_dimension` 一致。如果输入画面尺寸变化，需要同步修改输入尺寸宏，按新尺寸缩放或重新标定相机矩阵（`fx`、`fy`、`cx`、`cy`），并重新生成该坐标系下的 TV 轮廓点。

输出布局保持与旧 dump 工具兼容：先 `map_x`，后 `map_y`，均为 row-major `int16_t`。

## 6. 注意事项

1. 标定数据与 **1920×1080** 输入绑定；更换传感器、镜头或输入分辨率需更新相机内参、`DEFAULT_INPUT_WIDTH` / `DEFAULT_INPUT_HEIGHT` 与轮廓点。
2. **dump** 仅用于调试或离线取表；大分辨率会产生较大的日志。
3. **日志在 CP 端打印**，请使用 CP 对应串口观察输出。
4. 输入命令时须在**最前面**加 **ap_cmd**，例如：`ap_cmd fisheye cal 0`。

