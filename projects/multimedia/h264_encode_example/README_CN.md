# H264编码示例工程

* [English](./README.md)

## 1. 项目概述

本项目用于演示 Beken 平台上的 H264 编码流程。当前测试路径基于 `bk_h264_encode_*` 控制器接口和 VCENC H264 后端。

当前工程提供：

- VCENC H264 整帧模式编码测试：`h264_encode vcenc_h264e`
- VCENC H264 软件 FLEXA 模式编码测试：`h264_encode vcenc_h264e_flexa`
- 整帧 / FLEXA 编码回归测试（纯编码，不叠加 OSD）
- 交互式 OSD 测试：`h264_encode osd`（8 路通道 + ARGB8888 / NV12 / Bitmap 三种格式，单帧编码）
- `CONFIG_BK_ENCODER` 使能时的上电自动编码自检
- `.it.csv` 集成测试入口

* 有关 H264 编码的详细信息，请参阅：

  - [H.264 编码（VPU）](../../../developer-guide/vpu/h264_encode.html)
  - [Frame 与 Flexa 模式](../../../developer-guide/vpu/flexa_frame.html)

* 有关API参考，请参阅：

  - [VPU API](../../../api-reference/multimedia/bk_vpu.html)

### 1.1 测试环境

   * 硬件配置：
      * 核心板，**BK7259_QF128_12.3X12.3_V4.0**
      * PSRAM 32M
   * 支持H264硬件编码
      * 测试输入：工程内置 NV12 图像
      * 输出：H264 编码流
      * 测试配置：GOP 为 15，每次测试编码 30 帧

.. warning::

    请使用参考外设，进行demo工程的熟悉和学习。如果外设规格不一样，代码可能需要重新配置。

## 2. 目录结构

项目采用AP-CP双核架构，主要源代码位于AP目录下。项目结构如下：

```
h264_encode_example/
├── .ci                   # CI配置目录
├── .gitignore            # Git忽略文件
├── CMakeLists.txt        # 项目级CMake构建文件
├── Makefile              # Make构建文件
├── README.md             # 项目说明文档（英文）
├── README_CN.md          # 项目说明文档（中文）
├── ap/                   # AP端代码
│   ├── CMakeLists.txt    # AP端CMake构建文件
│   ├── Kconfig.projbuild # Kconfig配置
│   ├── ap_main.c         # AP主入口文件
│   ├── config/           # AP配置目录
│   └── h264_encode/      # H264编码实现
│       ├── common/       # 内置 256x128 NV12 测试图像
│       ├── include/      # 头文件
│       └── src/          # CLI 与 VCENC H264 测试
├── cp/                   # CP端代码
│   ├── CMakeLists.txt    # CP端CMake构建文件
│   ├── cp_main.c         # CP主入口文件
│   └── config/           # CP配置目录
├── it.yaml               # 集成测试配置
├── partitions/           # 分区配置
└── pj_config.mk          # 项目配置
```

## 3. 功能说明

### 3.1 主要功能

- 支持 VCENC H264 整帧编码测试
- 支持 VCENC H264 软件 FLEXA 编码测试
- 使用内置 `h264_encode_stream_256x128` NV12 输入图像
- 每次测试编码 30 帧，GOP 设置为 15
- 提供统一 `[RESULT][PASS]` / `[RESULT][FAIL]` 结果日志
- CLI 触发的测试运行在独立线程中
- `CONFIG_BK_ENCODER` 使能时，上电后自动运行编码 demo

### 3.2 H264编码流程

1. 申请并填充 256x128 NV12 输入帧
2. 创建 H264 控制器
   - 整帧模式：`bk_h264_encode_frame_new()`
   - FLEXA 模式：`bk_h264_encode_sw_flexa_new()`
3. 初始化并打开编码器
4. 以 GOP 15 连续编码 30 帧
5. 每帧等待编码完成回调
6. 打印最终 `[RESULT]` 日志并释放资源

## 4. 编译与运行

### 4.1 编译方法

使用以下命令编译项目：

```bash
make bk7259 PROJECT=multimedia/h264_encode_example
```

### 4.2 运行方法

编译完成后，将生成的固件烧录到开发板上。如果 `CONFIG_BK_ENCODER` 使能，`main()` 会自动启动一次 VCENC H264 boot demo。也可以通过串口终端手动触发以下命令：

命令执行成功打印："CMDRSP:OK"

命令执行失败打印："CMDRSP:ERROR"

#### 4.2.1 当前 VCENC H264 命令

```text
h264_encode help
h264_encode vcenc_h264e
h264_encode vcenc_h264e_flexa
h264_encode osd
```

- `vcenc_h264e`：执行整帧模式编码测试
- `vcenc_h264e_flexa`：执行软件 FLEXA 模式编码测试
- `osd`：执行 OSD 测试（8 路通道 + ARGB8888 / NV12 / Bitmap 三种格式，单帧编码）
- 两个编码命令都使用内置 256x128 NV12 图像，GOP 为 15，共编码 30 帧
- `CMDRSP:OK` 只表示测试线程创建成功，最终是否通过请看 `[RESULT]` 日志

## 5. 测试示例

### 5.1 整帧模式 VCENC H264 测试

```text
h264_encode vcenc_h264e
```

预期最终日志：

```text
[RESULT][PASS] vcenc_h264_frame_test success, frames=30/30 encoded_size=... frame_type=...
```

### 5.2 软件 FLEXA 模式 VCENC H264 测试

```text
h264_encode vcenc_h264e_flexa
```

预期最终日志：

```text
[RESULT][PASS] vcenc_h264_flexa_test success, frames=30/30 encoded_size=... frame_type=...
```

### 5.3 OSD 测试（8 路 + 三种格式）

```text
h264_encode osd
```

预期最终日志：

```text
[RESULT][PASS] h264_encode_osd_test success, slots=8/8 formats=3/3 frames=1/1 encoded_size=... frame_type=...
```

单帧同时启用 8 路 OSD，其中 slot1 为 NV12、slot2 为 Bitmap，其余为 ARGB8888；每路独占一个 CTB 单元（4 列 × 2 行）。

### 5.4 集成测试命令

`.it.csv` 包含：

```text
ap_cmd h264_encode vcenc_h264e
ap_cmd h264_encode vcenc_h264e_flexa
ap_cmd h264_encode osd
```

期望结果分别匹配对应的 `[RESULT][PASS]` 日志。

## 6. 配置选项

### 6.1 编码器配置

当前 VCENC H264 测试使用以下固定配置：

- 宽度：`256`
- 高度：`128`
- 输入格式：`BK_PIXEL_FORMAT_NV12`
- GOP：`15`
- 每次测试帧数：`30`
- 整帧控制器：`bk_h264_encode_frame_new()`
- FLEXA 控制器：`bk_h264_encode_sw_flexa_new()`

### 6.2 H264 OSD 使用规范

本工程通过 `bk_h264_encode_set_osd()` 叠加 OSD，支持 ARGB8888、NV12、Bitmap 三种像素格式。使用前请遵守以下约束：

| 项目 | 说明 |
|------|------|
| 通道数量 | 硬件最多 **8 路**，index 取值 **0～7** |
| 像素格式 | **0=ARGB8888**（像素自带 Alpha，`alpha` 无效）；**1=NV12**（使用全局 `alpha`）；**2=Bitmap**（1bpp，颜色由 `bitmap_y/u/v` 指定） |
| 坐标对齐 | **x、y 需 2 像素对齐** |
| 尺寸对齐 | ARGB8888/NV12：**2 像素对齐**；Bitmap：**8 像素对齐** |
| 边界 | OSD 区域不得超出编码画面（本工程为 256×128） |
| Stride | ARGB8888：`width×4`；NV12：Y/UV stride 均为 `width`；Bitmap：`width/8` |
| CTB 重叠 | H.264 编码 CTB 为 **64×16**，**多路 OSD 不得占用同一 CTB** |
| 缓存 | CPU 写入 OSD buffer 后需 **flush D-Cache**，再调用 `bk_h264_encode_set_osd()` |
| 内存生命周期 | 通过 `buffer_free` 回调释放 OSD buffer |
| 禁用某路 | `buffer = NULL` 提交对应 index 即可清除该路 OSD |

**`h264_encode osd` 测试布局（CTB 合规，4 列 × 2 行）**

| Slot | 格式 | 标签 | x | y | 宽×高 |
|------|------|------|---|---|-------|
| 0 | ARGB8888 | 00:00:00 | 4 | 0 | 48×16 |
| 1 | NV12 | 01 | 68 | 0 | 32×16 |
| 2 | Bitmap | 02 | 132 | 0 | 32×16 |
| 3 | ARGB8888 | 03 | 196 | 0 | 32×16 |
| 4 | ARGB8888 | 04 | 4 | 16 | 32×16 |
| 5 | ARGB8888 | 05 | 68 | 16 | 32×16 |
| 6 | ARGB8888 | 06 | 132 | 16 | 32×16 |
| 7 | ARGB8888 | 07 | 196 | 16 | 32×16 |

**整帧 / Flexa 编码测试**：不叠加 OSD，仅验证纯 H.264 编码流程。

**API 调用顺序**

1. 创建并 `open` 编码器
2. 按格式分配 buffer，绘制内容并 flush cache
3. 填充 `bk_h264_encode_osd_t`（含 index、format、坐标、尺寸、alpha/bitmap 颜色、`buffer_free`）
4. 调用 `bk_h264_encode_set_osd()`（每帧编码前可更新）
5. 调用 `bk_h264_encode_start()` 开始编码

**常见错误**

- 多路 OSD 在同一 CTB 内重叠 → 编码 pipeline 异常或超时
- 坐标/尺寸未对齐、越界 → `bk_h264_encode_set_osd()` 返回失败
- 未 flush cache → 画面 OSD 内容错乱

## 7. 注意事项

1. `h264_encode` 只负责创建测试线程，最终结果以 `[RESULT][PASS]` 或 `[RESULT][FAIL]` 为准。
2. 上电 demo 和手动 CLI 测试都会使用编码硬件，建议等待上电 demo 结束后再手动触发测试。
3. 帧缓冲资源有限，请避免同时占用过多缓冲区。
4. 编码是异步的；结果在输出完成回调中返回。
5. FLEXA 模式按 16 行 block 供给输入；测试会在 FLEXA done 回调中推进写指针，直到 256x128 图像的 8 个 block 全部送入。
6. **回调函数使用注意事项**：
   - 回调函数中不建议执行阻塞操作（如长时间等待、sleep等），以避免影响编码性能和系统响应
   - 建议在回调函数中仅进行轻量级操作，如设置标志位、发送消息/信号量等，将耗时操作放到其他任务中执行
7. **缓冲区管理**：
   - 输入缓冲区从 `MEM_SLAB_HEAP_UNCODED` 申请
   - 输出缓冲区从 `MEM_SLAB_HEAP_CODED` 申请
   - 使用后应释放输入和输出缓冲区
