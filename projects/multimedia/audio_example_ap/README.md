# Audio Driver Example (AP)

* [中文](./README_CN.md)

## 1. Overview

This project validates the BK7259 audio driver on the AP. It covers basic
ADC, DAC, and I2S functions. The audio tests and CLI run on the AP, while the CP
starts the AP.

Use `audio_example` when the tests need to run on the CP.

## 2. Hardware and Console

- BK7259 development board.
- Connect a microphone, speaker, or external I2S device as required by the test.
- The audio CLI uses the AP UART at 1 Mbps.
- Enter commands directly on the AP console; the `ap_cmd` prefix is not needed.

## 3. Project Structure

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

## 4. Build and Flash

```bash
make bk7259 PROJECT=multimedia/audio_example_ap
```

The generated image is:

```text
build/bk7259/audio_example_ap/package/all-app.bin
```

Flash the image using the standard BK7259 procedure.

## 5. Main CLI Commands

This project provides the same type of audio test commands as `audio_example`.

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

The main options are in `ap/config/bk7259_ap/defconfig`:

- `CONFIG_AUDIO`
- `CONFIG_AUD_DRIVER_V2`
- `CONFIG_AUDIO_ADC`
- `CONFIG_AUDIO_DAC`
- `CONFIG_I2S`
