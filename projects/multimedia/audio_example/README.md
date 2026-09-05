# Audio Driver Example (CP)

* [中文](./README_CN.md)

## 1. Overview

This project validates the BK7259 audio driver on the CP. It covers basic
ADC, DAC, I2S, XDAC, ring-buffer, and low-power VAD functions. The commands are
registered by `cp/aud_test.c` and run on the CP.

Use `audio_example_ap` when the equivalent tests need to run on the AP.

## 2. Hardware and Console

- BK7259 development board.
- Connect a microphone, speaker, or external I2S device as required by the test.
- Enter commands directly on the CP console; the `ap_cmd` prefix is not needed.
- The project initializes UART1 at 2 Mbps for PCM data output.

## 3. Project Structure

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

## 4. Build and Flash

```bash
make bk7259 PROJECT=multimedia/audio_example
```

The generated image is:

```text
build/bk7259/audio_example/package/all-app.bin
```

Flash the image using the standard BK7259 procedure.

## 5. Main CLI Commands

Available commands depend on the project configuration.

| Command | Purpose |
| --- | --- |
| `aud_adc_mcp_test` | Basic ADC capture |
| `aud_adc_mcp_dma_test` | ADC DMA capture |
| `aud_adc_dac_dma_loopback_test` | ADC-to-DAC DMA loopback |
| `aud_dac_mcp_test` | Basic DAC playback |
| `aud_dac_dma_test` | DAC DMA playback |
| `aud_i2s_mcp_test` | Basic I2S transmit and receive |
| `aud_i2s_dma_test` | I2S DMA transmit, receive, and loopback |
| `aud_adc_i2s_dac_loopback_test` | ADC, I2S, and DAC path loopback |
| `xdac_dma_ringbuf_test` | XDAC DMA ring-buffer test |
| `aud_lp_vad_test` | Low-power VAD test |
| `aud_generate_pcm_test` | Generate or dump PCM test data |

Examples:

```text
aud_adc_mcp_test start 16000 16
aud_adc_mcp_test stop

aud_i2s_mcp_test start tx 16000 0
aud_i2s_mcp_test stop tx 16000 0
```

Use the firmware `help` output for the complete syntax and enabled command set.

## 6. Key Configuration

The main options are in `cp/config/bk7259/defconfig`:

- `CONFIG_AUDIO`
- `CONFIG_AUDIO_ADC`
- `CONFIG_AUDIO_DAC`
- `CONFIG_AUDIO_RING_BUFF`
- `CONFIG_I2S`
- `CONFIG_XDAC`
