# JPEG解码示例工程

* [English](./README.md)

## 1. 项目概述

本工程演示 Beken 平台上的 JPEG 解码，包含两层 API：

| 层次 | 函数 / CLI | 说明 |
|------|-----------|------|
| 平台 JPEG | `jpeg_decode jpegd` / `jpegd_flexa` | 平台 JPEG 解码示例 |
| VCDEC（`bk_decoder`） | `jpeg_decode vcdec_jpegd` / `vcdec_jpegd_flexa` | 经 `bk_decoder` 的 JPEG 解码 |

使能 `CONFIG_BK_DECODER` 时，`main()` 自动执行开机自检：先 `vcdec_jpeg_flexa_test()`，再 `vcdec_jpeg_test()`。

legacy 的 `jpeg_decode_stress` 已从活跃工程移除，源码归档于 `verisilicon_nano/legacy/projects/jpeg_decode_example/`（见 `verisilicon_nano/legacy/README.md`）。

* 开发者指南：

  - [JPEG 解码（VPU）](../../../developer-guide/vpu/jpeg_decode.html)
  - [Frame 与 Flexa 模式](../../../developer-guide/vpu/flexa_frame.html)

### 1.1 测试环境

- 硬件：**BK7259_QF128_12.3X12.3_V4.0**，PSRAM 32M
- 输入：内置 JPEG 测试图片
- 输出：解码日志；VCDEC 路径打印 `[RESULT][PASS]` / `[RESULT][FAIL]`

.. warning::

    请使用参考外设。外设规格不同时需调整代码与配置。

## 2. 目录结构

```text
jpeg_decode_example/
├── ap/
│   ├── ap_main.c
│   └── jpeg_decode/
│       ├── include/
│       └── src/
│           ├── jpeg_decode_cli.c
│           └── vcdec_jpeg_test.c
├── cp/
├── partitions/
└── pj_config.mk
```

## 3. 功能说明

- `jpeg_decode jpegd` / `jpegd_flexa` — 平台 JPEG 解码
- `jpeg_decode vcdec_jpegd` / `vcdec_jpegd_flexa` — 需 `CONFIG_BK_DECODER`
- `CONFIG_BK_DECODER=y` 时上电自动 `vcdec` 自检
- 单任务保护：同一时刻仅允许一个解码任务

## 4. 编译与运行

### 4.1 编译

```bash
make bk7259 PROJECT=multimedia/jpeg_decode_example
```

### 4.2 CLI 命令

```text
jpeg_decode help
jpeg_decode jpegd
jpeg_decode jpegd_flexa
jpeg_decode vcdec_jpegd          # 需 CONFIG_BK_DECODER
jpeg_decode vcdec_jpegd_flexa    # 需 CONFIG_BK_DECODER
```

### 4.3 通过判定

VCDEC 路径 — 搜索 `RESULT`：

```text
[RESULT][PASS] vcdec_jpeg_test success, rounds=5/5
[RESULT][PASS] vcdec_jpeg_flexa_test success, rounds=5/5
```

平台 `jpegd` 路径无统一 `[RESULT]` 行，通常以无 init/malloc 错误为通过。

### 4.4 集成测试（`.it.csv`）

```text
ap_cmd jpeg_decode vcdec_jpegd
ap_cmd jpeg_decode vcdec_jpegd_flexa
```

## 5. 注意事项

1. `vcdec` 命令与开机自检依赖 `CONFIG_BK_DECODER`。
2. 开机自检结束后再手动触发新的 `vcdec` 命令，避免争用解码器实例。
3. legacy `jpeg_decode_stress` 已归档至 `verisilicon_nano/legacy/`。
