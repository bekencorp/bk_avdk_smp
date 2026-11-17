# 4K 拍照示例工程（shoot4k\_example）

- [English](./README.md)

## 1. 项目概述

本工程演示 BK7259 平台上基于 GC8613YA MIPI CSI 传感器的 4K 单帧拍照流程：

- 传感器输出 3840x2160 RAW10，ISP 输出一帧完整的 3840x2160 NV12；
- 支持将 4K NV12 通过 VCENC 编码为 JPEG；
- 支持通过 UART4 将 JPEG 原始数据导出到 PC；
- 提供 `capture`、`jpeg`、`loop` 三条命令行入口，并打印各阶段耗时。

由于一帧 4K RAW 约 15.8MB、一帧 4K NV12 约 12.4MB，无法同时放入一块受限的 PSRAM slab，工程采用三阶段拍摄：

| 阶段     | 动作                        | 说明                                                                                    |
| ------ | ------------------------- | ------------------------------------------------------------------------------------- |
| Pass 1 | 打开 1920x1080 NV12 @15fps  | 等待一帧，查询 AE 收敛后的曝光时间和模拟增益                                                              |
| Pass 2 | 打开 3840x2160 RAW10 @15fps | 在 sensor 停流状态下下发曝光种子，借用一帧完整 4K RAW 后关闭通道                                              |
| Pass 3 | 打开 1920x2160 NV12 @30fps  | 将 4K NV12 缓冲注入 MP 通道，RDMA 把借用的 4K RAW 分上下两半回放，ISP 直接写入 4K NV12 缓冲，拼成一帧 3840x2160 NV12 |

### 1.1 参考环境

- 硬件：BK7259 开发板 + GC8613YA（MIPI CSI）
- 传感器接口：I2C1，SCL = GPIO\_69，SDA = GPIO\_70
- 传感器控制：XCLK = GPIO\_59，RESET = GPIO\_71，PWDN 未使用（0xFF）
- 调试串口：UART0，115200
- JPEG 数据口：UART4（基地址 `0x458E0000`），默认 115200
- Flash 分区：默认 8MB（见 `partitions/bk7259/auto_partitions.csv`）

> 更换传感器或开发板时，需要同步修改 `ap/src/shoot4k_cli.c` 中的引脚、`CSI_CAM_BUS_I2C1_8BIT_2000TIMEOUT()` 总线配置以及 `ap/config/bk7259_ap/defconfig`。

## 2. 目录结构

```text
shoot4k_example/
├── .ci                     # CI 构建入口
├── CMakeLists.txt          # 工程级 CMake
├── Makefile                # 工程级 Makefile
├── README.md
├── ap/                     # AP 侧代码
│   ├── CMakeLists.txt
│   ├── config/bk7259_ap/   # AP defconfig、GPIO 默认配置
│   ├── include/shoot4k_cli.h
│   └── src/
│       ├── ap_main.c       # AP 入口：初始化 media service、frame buffer、CLI
│       └── shoot4k_cli.c   # 4K 拍摄、JPEG 编码、UART 导出与 CLI
├── cp/                     # CP 侧代码
│   ├── CMakeLists.txt
│   ├── cp_main.c
│   └── config/bk7259/
└── partitions/bk7259/      # Flash / RAM 布局
```

## 3. 编译与烧录

### 3.1 编译

在 SDK 根目录执行：

```bash
make bk7259 PROJECT=multimedia/shoot4k_example
```

使用 Docker 环境时：

```bash
./dbuild.sh make bk7259 PROJECT=multimedia/shoot4k_example
```

工程根目录的 `.ci` 中已保存同一条 Docker 编译命令。

默认 Flash 容量 8MB。需要覆盖时可在命令后追加参数：

```bash
make bk7259 PROJECT=multimedia/shoot4k_example FLASH_CAPACITY=8M PSRAM_CAPACITY=16M
```

编译完成后，在构建目录的 `package/` 下生成 `all-app.bin`（实际路径以构建日志为准）。

### 3.2 烧录与启动

1. 使用 BKFIL 选择 `all-app.bin` 烧录；首次烧录新模组需同时关闭 CRC 校验，具体步骤见《快速入门》文档。
2. 烧录完成后重新上电。
3. 通过 UART0（115200）连接调试串口，启动后应能看到 `m55 running...` 以及 `shoot4k` CLI 注册成功的日志。

## 4. CLI 使用说明

所有命令执行成功打印 `CMDRSP:OK`，失败打印 `CMDRSP:ERROR`。CLI 命令为阻塞执行，拍摄期间不要重复触发。

```text
shoot4k fmt [rgb565|yuyv422]    # 查询/切换 16bpp 输出路径，默认 rgb565
shoot4k capture                 # 拍摄一帧 3840x2160 16bpp 图像
shoot4k jpeg                    # 拍摄一帧并编码为 JPEG
shoot4k loop <n> [jpeg]         # 连续拍摄 n 次，可选每轮编码 JPEG
shoot4k uart_init [baud]        # 初始化 UART4 数据口，默认 115200
shoot4k uart_send jpeg          # 通过 UART4 发送最近一次 JPEG
```

### 4.1 验证拍摄链路：capture

```text
shoot4k fmt yuyv422
shoot4k capture
```

只验证 4K RAW 借用、RDMA 回放和 16bpp 拼接链路，**不会把图像输出到串口或存储介质**。成功时日志包含：

```text
shoot4k: start, loop=1 jpeg=0
shoot_4k: pass1 (AE probe) end ..., cost ...ms
shoot_4k: pass2 (capture, zero-copy) end ..., cost ...ms
shoot_4k: total cost ...ms
shoot4k: done, ok 1/1, total ...ms
```

注意：`capture` 成功后缓冲会保留到下一次拍摄开始时才释放，用于保持接口简单。RGB565/YUYV422 模式复用 16.6MB RAW 缓冲，NV12 模式额外保留 12.4MB。

### 4.2 拍摄并编码 JPEG

```text
shoot4k jpeg
```

流程为：拍摄 4K NV12 → VCENC 编码 JPEG → 释放 NV12，仅保留 JPEG。成功时日志包含：

```text
jpeg encode done, size=...
jpeg: 0x..., len: ...
shoot4k: done, ok 1/1, total ...ms
```

JPEG 输出缓冲固定为 4MB，若单帧 JPEG 超过该大小则编码失败。JPEG 会保留在内存中，直到下一次 `jpeg` / `loop ... jpeg` 覆盖，或通过 `uart_send jpeg` 发送。

### 4.3 连续拍摄

```text
shoot4k loop 5
shoot4k loop 5 jpeg
```

每轮拍摄完成后立即释放 NV12（以及 JPEG），因此峰值内存约为单轮拍摄占用。循环模式下 JPEG 不会保留，执行结束后 `uart_send jpeg` 不可用（日志会提示）。

### 4.4 通过 UART4 导出 JPEG

1. 将 USB 转串口工具的 RX 接到板端 UART4 TX，并共地；
2. 初始化数据口：
   ```text
   shoot4k uart_init 115200
   ```
3. 拍摄并编码：
   ```text
   shoot4k jpeg
   ```
4. 发送：
   ```text
   shoot4k uart_send jpeg
   ```
5. 在 PC 串口工具中开启“接收保存到文件”，波特率与 `uart_init` 保持一致，选择 8 数据位、无校验、1 停止位、无流控；保存后的文件直接改名为 `.jpg` 即可查看。

UART4 传输的是裸 JPEG 字节流，没有额外帧头或协议；调试日志仍从 UART0 输出，不会混入 JPEG 数据。

## 5. 拍摄流程与日志

`shoot_4k` 的完整流程如下（实现在 `isp_shoot_4k_capture()`）：

1. **Pass 1（AE 探测）**：打开 SP 通道 1920x1080 NV12 @15fps，等待一帧后通过 `BK_CAM_IOCTL_QUERY_EXPOSURE_INFO` 读取曝光时间和模拟增益，并通过 `VSI_MPI_ISP_QueryAwbInfo` 采样该帧的 AWB 增益，随后关闭通道。
   - 探测帧借用失败只打印警告，不影响后续流程；曝光/AWB 信息仍会查询。
2. **Pass 2（4K RAW 借用）**：打开 SP 通道 3840x2160 RAW10 @15fps，在开流前通过 `BK_CAM_IOCTL_SET_INITIAL_EXPOSURE` 下发 Pass 1 得到的曝光种子，借用一帧完整 4K RAW 后关闭通道。
   - 该阶段借用失败会直接导致本次拍摄失败。
3. **Pass 3（RDMA 回放与拼接）**：默认输出 **16bpp 原地复用**，格式由 `shoot4k fmt` 在 RGB565 / YUYV422 之间运行时选择。借用的 4K RAW 缓冲区直接作为输出缓冲，以零拷贝方式注入 MP 通道，打开 1920x2160 RGB565/YUYV422 @30fps；RDMA 将 RAW 帧的左/右半分别回放，MI 以 7680B 行跨距把左半写到 `base+0`、右半写到 `base+3840`，连续 pop 两帧后即得到完整 3840x2160 16bpp 帧（16,588,800B，与 RAW 缓冲同址复用），无需额外分配。
   - RGB565/YUYV422 均为 2B/px，半帧每行 3840B，正好等于 RAW 每行 7680B 的左/右半段，因此可以原地拼接。
   - Pass3 开流前会把 Pass1 采样的 AWB 增益通过 `BK_CAM_IOCTL_SET_WB`（manual）锁定，相当于在正式拍摄前多迭代了一帧 AWB，同时保证左右半帧使用同一组 WB 增益。
4. **JPEG 编码（可选）**：`shoot4k_cli.c` 内直接调用底层 `JpegEncInit/JpegEncSetPictureSize/JpegEncEncode`（RGB565 用 `JPEGENC_RGB565`，YUYV422 用 `JPEGENC_YUV422_INTERLEAVED_YUYV`）。输出缓冲优先从 CODED slab 分配，不足时回退到 UNCODED。

耗时统计日志位于 Pass 1 结束、Pass 3 结束和命令结束时，字段单位为毫秒，可直接用于评估各阶段开销。

## 6. 关键配置

`ap/config/bk7259_ap/defconfig` 中与本工程直接相关的配置：

| 配置                     | 作用                     |
| ---------------------- | ---------------------- |
| `CONFIG_ISP`           | ISP 驱动                 |
| `CONFIG_MIPI_CSI`      | MIPI CSI 接口            |
| `CONFIG_BK_CAMERA`     | 摄像头框架                  |
| `CONFIG_CSI_CAMERA`    | CSI 摄像头支持              |
| `CONFIG_CSI_GC8613YA`  | GC8613YA 传感器驱动         |
| `CONFIG_FRAME_BUFFER`  | Frame buffer / slab 管理 |
| `CONFIG_BK_H264E`      | VCENC，JPEG 编码依赖该配置     |
| `CONFIG_MEDIA_SERVICE` | 媒体服务                   |

若关闭 `CONFIG_BK_H264E`，`shoot4k jpeg` / `loop ... jpeg` 会返回不支持。

`ap/src/shoot4k_cli.c` 顶部的编译开关：

| 宏                            | 默认   | 作用                                                                         |
| ---------------------------- | ---- | -------------------------------------------------------------------------- |
| `SHOOT4K_MI_Y_LLENGTH_VALUE` | 3840 | MI 行跨距寄存器值；若实测硬件按字节解释，改为 7680                                              |

`partitions/bk7259/ram_regions.csv` 规划了两块大缓冲：

- `PSRAM_MEM_SLAB_UNCODED`：16MB，用于 RAW / NV12 等非编码数据；
- `PSRAM_MEM_SLAB_CODED`：14MB，用于 JPEG 输出等编码数据。

## 7. 注意事项

1. **仅支持固定的 4K 分辨率**：Pass 3 的 RDMA 分半回放按 1920x2160 半帧设计，修改输出分辨率需要同步调整 `isp_func_rdma_write_config()` 的参数和缓冲偏移。
2. **不支持上采样**：ISP 输出分辨率不能大于传感器输出分辨率，打开前会检查传感器格式列表。
3. **拍摄期间不可重入**：CLI 在调用线程中阻塞执行完整拍摄流程，不要并发触发多条拍摄命令。
4. **内存峰值**：16bpp 模式（RGB565/YUYV422）单次拍摄只持有复用的 16.6MB RAW 缓冲。JPEG 模式再额外申请 4MB 输出缓冲；`loop` 模式每轮结束即释放，适合长时间压力测试。
5. **16bpp 行跨距依赖应用层覆盖 MI 寄存器**：驱动的 RGB565/YUYV 分支不采纳外部 stride，`isp_shoot4k_mi_geometry_override()` 在通道打开后直接写 `MI_MP_Y_LLENGTH/SIZE/PIC_SIZE`。若画面出现半宽/行重叠，按上表调整 `SHOOT4K_MI_Y_LLENGTH_VALUE`。
6. **YUYV422 字节序**：若 YUYV 图像出现颜色错乱，把 `isp_jpeg_encode_yuyv422()` 中的 `JPEGENC_YUV422_INTERLEAVED_YUYV` 改成 `JPEGENC_YUV422_INTERLEAVED_UYVY` 复测。
7. **原地复用存在首帧风险**：从最终通道 open 到 RDMA 生效之间理论上可能完成一帧 MIPI 帧并写坏 RAW。若压测出现顶部/左半随机错行，可考虑在 RDMA 配置前临时置 MI\_SKIP。
8. **UART4 为裸数据口**：没有握手和重传，传输大文件时建议降低波特率或确认串口工具不丢数据。
9. 日志 TAG 为 `shoot4k`，可通过该关键字过滤拍摄相关输出。
