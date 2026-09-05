# 音频驱动示例工程（CP）

* [English](./README.md)

## 1. 项目概述

本工程用于验证 BK7259 CP 侧音频驱动，覆盖 ADC、DAC、I2S、XDAC、
Ring Buffer 和低功耗 VAD 等基础功能。测试命令由
`cp/aud_test.c` 注册并在 CP 上运行。

`audio_example_ap` 提供相同类型的 AP 侧测试，请根据需要选择工程。

## 2. 硬件与串口

- BK7259 开发板。
- 根据测试内容连接麦克风、扬声器或外部 I2S 设备。
- 测试命令直接输入 CP 串口，无需添加 `ap_cmd`。
- 工程会初始化 UART1，默认以 2 Mbps 用于 PCM 数据输出。

## 3. 代码结构

```text
audio_example/
├── ap/
│   └── ap_main.c
├── cp/
│   ├── cp_main.c
│   ├── aud_test.c
│   ├── aud_test_data.c
│   ├── aud_ate_test.c
│   └── config/bk7259/defconfig
└── partitions/bk7259/
```

## 4. 编译与烧录

```bash
make bk7259 PROJECT=multimedia/audio_example
```

固件输出路径：

```text
build/bk7259/audio_example/package/all-app.bin
```

编译完成后，按照 BK7259 常规流程烧录固件。

## 5. 主要 CLI 命令

实际可用命令取决于工程配置，常用命令包括：

| 命令 | 功能 |
| --- | --- |
| `aud_adc_mcp_test` | ADC 基础采集测试 |
| `aud_adc_mcp_dma_test` | ADC DMA 采集测试 |
| `aud_adc_dac_dma_loopback_test` | ADC 到 DAC DMA 回环 |
| `aud_dac_mcp_test` | DAC 基础播放测试 |
| `aud_dac_dma_test` | DAC DMA 播放测试 |
| `aud_i2s_mcp_test` | I2S 基础收发测试 |
| `aud_i2s_dma_test` | I2S DMA 收发及回环测试 |
| `aud_adc_i2s_dac_loopback_test` | ADC、I2S、DAC 链路回环 |
| `xdac_dma_ringbuf_test` | XDAC DMA Ring Buffer 测试 |
| `aud_lp_vad_test` | 低功耗 VAD 测试 |
| `aud_generate_pcm_test` | 生成或输出 PCM 测试数据 |

示例：

```text
aud_adc_mcp_test start 16000 16
aud_adc_mcp_test stop

aud_i2s_mcp_test start tx 16000 0
aud_i2s_mcp_test stop tx 16000 0
```

请通过固件 `help` 输出确认完整命令格式和当前已启用的命令。

## 6. 关键配置

主要配置位于 `cp/config/bk7259/defconfig`：

- `CONFIG_AUDIO`
- `CONFIG_AUDIO_ADC`
- `CONFIG_AUDIO_DAC`
- `CONFIG_AUDIO_RING_BUFF`
- `CONFIG_I2S`
- `CONFIG_XDAC`
