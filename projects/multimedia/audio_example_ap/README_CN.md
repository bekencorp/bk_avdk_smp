# 音频驱动示例工程（AP）

* [English](./README.md)

## 1. 项目概述

本工程用于验证 BK7259 AP 侧音频驱动，覆盖 ADC、DAC 和 I2S 等基础功能。
音频测试代码和 CLI 均运行在 AP，CP 负责启动 AP。

如需在 CP 运行音频驱动测试，请使用 `audio_example`。

## 2. 硬件与串口

- BK7259 开发板。
- 根据测试内容连接麦克风、扬声器或外部 I2S 设备。
- 音频 CLI 使用 AP UART，默认波特率为 1 Mbps。
- 命令直接输入 AP 串口，无需添加 `ap_cmd`。

## 3. 代码结构

```text
audio_example_ap/
├── ap/
│   ├── ap_main.c
│   ├── aud_test.c
│   └── config/bk7259_ap/defconfig
├── cp/
│   ├── cp_main.c
│   └── config/bk7259/defconfig
└── partitions/bk7259/
```

## 4. 编译与烧录

```bash
make bk7259 PROJECT=multimedia/audio_example_ap
```

固件输出路径：

```text
build/bk7259/audio_example_ap/package/all-app.bin
```

编译完成后，按照 BK7259 常规流程烧录固件。

## 5. 主要 CLI 命令

本工程与 `audio_example` 使用相同类型的音频测试命令，主要包括：

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

主要配置位于 `ap/config/bk7259_ap/defconfig`：

- `CONFIG_AUDIO`
- `CONFIG_AUD_DRIVER_V2`
- `CONFIG_AUDIO_ADC`
- `CONFIG_AUDIO_DAC`
- `CONFIG_I2S`
