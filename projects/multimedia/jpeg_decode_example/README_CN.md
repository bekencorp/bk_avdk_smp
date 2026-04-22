# JPEG解码示例工程

* [English](./README.md)

## 1. 项目概述

本工程用于演示 Beken 平台上的 JPEG 解码能力，包含两类路径：

- 平台 JPEG 解码示例：`jpeg_decoder_test()` / `jpeg_decoder_flexa_test()`
- `vcdec` JPEG 解码示例：`vcdec_jpeg_test()` / `vcdec_jpeg_flexa_test()`

工程提供串口 CLI 入口，便于手动触发普通解码、FLEXA 解码和压力测试；当使能 `CONFIG_BK_DECODER` 时，系统上电后还会自动创建 `vcdec` JPEG 自检任务，依次执行 FLEXA 和整帧模式，便于快速确认链路是否正常。

### 1.1 测试环境

- 硬件配置
  - 核心板：`BK7258_QFN88_9X9_V3.2`
  - PSRAM：`8M/16M`
- 输入数据
  - 工程内置 JPEG 测试图片
- 输出内容
  - 解码流程日志
  - `vcdec` 结果日志
  - 压力测试运行日志

.. warning::

    请使用参考外设进行示例学习与验证。如果外设规格不同，代码和配置可能需要同步调整。

## 2. 目录结构

项目采用 AP-CP 双核结构，主要逻辑位于 AP 侧：

```text
jpeg_decode_example/
├── README.md
├── README_CN.md
├── ap/
│   ├── ap_main.c                    # AP 主入口，注册 CLI，并按配置启动 vcdec 开机自检
│   ├── jpeg_decode/
│   │   ├── include/                 # JPEG 解码示例头文件
│   │   └── src/
│   │       ├── jpeg_decode_cli.c    # jpeg_decode CLI 实现
│   │       └── vcdec_jpeg_test.c    # vcdec JPEG 普通/FLEXA 示例
│   └── jpeg_decode_stress/
│       ├── include/
│       └── src/jpeg_decode_stress.c # JPEG 解码压力测试
├── cp/
├── partitions/
└── pj_config.mk
```

## 3. 功能说明

### 3.1 当前支持的功能

- `jpeg_decode` CLI：触发普通 JPEG 解码和 FLEXA JPEG 解码
- `jpeg_decode` CLI：在使能 `CONFIG_BK_DECODER` 时触发 `vcdec` JPEG 整帧/FLEXA 解码
- `jpeg_decode_stress` CLI：启动或停止 JPEG 解码压力测试，可选普通模式或 FLEXA 模式
- 开机 `vcdec` 自检：在使能 `CONFIG_BK_DECODER` 时自动运行一次

## 4. 编译与运行

### 4.1 编译方法

```bash
make bk7259 PROJECT=multimedia/jpeg_decode_example
```

### 4.2 运行方式

烧录固件后，通过串口终端执行命令或观察启动日志。

#### 4.2.1 开机自动测试

当使能 `CONFIG_BK_DECODER` 时，`main()` 会调用 `vcdec_jpeg_run_boot_demo()`，该接口实际会创建一个独立任务，并在任务中按顺序执行：

```text
vcdec_jpeg_flexa_test()
vcdec_jpeg_test()
```

任务中间带有短暂延时，因此建议等待开机自检完成后，再手动触发新的 `vcdec` JPEG 命令，避免与开机自检同时访问同一个解码器实例。

#### 4.2.2 串口 CLI 命令

`jpeg_decode` 当前支持以下子命令：

```text
jpeg_decode help
jpeg_decode jpegd
jpeg_decode jpegd_flexa
```

如果使能了 `CONFIG_BK_DECODER`，还支持：

```text
jpeg_decode vcdec_jpegd
jpeg_decode vcdec_jpegd_flexa
```

`jpeg_decode_stress` 当前支持：

```text
jpeg_decode_stress start
jpeg_decode_stress start none
jpeg_decode_stress start flexa
jpeg_decode_stress stop
```

其中：

- `start` / `start none` 表示普通模式压力测试
- `start flexa` 表示 FLEXA 模式压力测试
- CLI 仅创建后台任务，实际解码在线程中执行

命令提交成功时返回：

```text
CMDRSP:OK
```

命令提交失败时返回：

```text
CMDRSP:ERROR
```

#### 4.2.3 如何判断测试成功或失败

`CMDRSP:OK` 仅表示 CLI 已成功创建任务，不代表解码已经通过。请结合任务日志判断。

`vcdec` JPEG 示例带有统一结果行，建议直接搜索 `RESULT`、`PASS` 或 `FAIL`。

成功示例：

```text
[RESULT][PASS] vcdec_jpeg_test success, rounds=5/5
[RESULT][PASS] vcdec_jpeg_flexa_test success, rounds=5/5
```

失败示例：

```text
[RESULT][FAIL] vcdec_jpeg_test failed at decode_frame, ret=-6, rounds=2/5
[RESULT][FAIL] vcdec_jpeg_flexa_test failed at register_bond, ret=-1, rounds=0/5
```

结果字段说明：

- `failed at ...`：失败阶段，例如 `get_img_info`、`alloc_stream_buf`、`decoder_open`、`decode_frame`
- `ret=...`：底层接口返回值
- `rounds=x/y`：总轮数为 `y`，当前已完成 `x` 轮

普通 `jpegd` 和 `jpeg_decode_stress` 路径目前没有统一的 `[RESULT][PASS]` 行，通常按以下方式判断：

- 没有出现 `malloc failed`、`decoder init/open failed` 等错误日志
- 解码流程持续向前推进，没有异常退出
- 压力测试在执行 `stop` 后能够正常收尾退出

## 5. 注意事项

1. `jpeg_decode` 与 `jpeg_decode_stress` 是两个独立 CLI 命令，命令前缀不要写错。
2. `vcdec` 相关命令和开机自检依赖 `CONFIG_BK_DECODER`；如果未使能，则对应功能不会编译进入工程。
3. `jpeg_decode` 内部带有“单任务运行”保护，同一时刻只能启动一个解码测试线程。
4. `jpeg_decode_stress` 会额外申请输入、输出和 DMA 压测缓冲区，运行前请确认内存资源足够。
5. 当前 `vcdec` 示例同时覆盖整帧模式和 FLEXA 模式，适合作为 JPEG HAL/控制流接入参考。
