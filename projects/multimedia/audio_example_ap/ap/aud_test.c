// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "cli.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <driver/dma.h>
#include <driver/audio_ring_buff.h>

#if CONFIG_AUDIO_DAC
#include <driver/aud_dac_types.h>
#include <driver/aud_dac.h>
#endif

#if CONFIG_AUDIO_ADC
#include <driver/aud_adc_types.h>
#include <driver/aud_adc.h>
#endif

#if CONFIG_AUDIO_DMIC
#include <driver/aud_dmic_types.h>
#include <driver/aud_dmic.h>
#endif

#if CONFIG_AUDIO_DTMF
#include <driver/aud_dtmf_types.h>
#include <driver/aud_dtmf.h>
#endif

#if CONFIG_I2S
#include <driver/i2s.h>
#include <driver/i2s_types.h>
#endif

#if CONFIG_XDAC
#include <driver/xdac.h>
#include <driver/xdac_types.h>
#endif

//#define AUD_ASDF_DEBUG
//#define AUD_I2S_RINGBUF_DEBUG

#define TAG "aud_test"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

extern void bk_aud_adc_dump_reg(void);
static void cli_aud_help(void)
{
#if CONFIG_AUDIO_DAC
#if CONFIG_AUDIO_DTMF
	os_printf("aud_dtmf_mcp_test {start|stop} \r\n");
	os_printf("aud_dtmf_loop_test {start|stop} \r\n");
#endif

#if CONFIG_AUDIO_ADC
	os_printf("aud_adc_mcp_test {start|stop sample_rate} \r\n");
	os_printf("aud_adc_dma_test {start|stop|dump} {a2dp|call|hint} {sample_rate} [adc|dac|off] \r\n");
	os_printf("aud_adc_ringbuf_test {start|stop} {a2dp|call|hint} {sample_rate} \r\n");
	os_printf("aud_adc_loop_test {start|stop sample_rate} \r\n");
	os_printf("aud_eq_test {start|stop} \r\n");
#if CONFIG_I2S
	os_printf("aud_adc_i2s_dac_loopback_test {start|stop} {a2dp|call|hint} {sample_rate} [gpio_group] \r\n");
	os_printf("  ADC -> I2S out, I2S in -> DAC loopback test (external I2S connection required)\r\n");
	os_printf("  Example: aud_adc_i2s_dac_loopback_test start a2dp 16000 0\r\n");
#endif
#endif

#if CONFIG_I2S
	os_printf("aud_i2s_dac_test {start|stop} {a2dp|call|hint} {sample_rate} [gpio_group] \r\n");
	os_printf("  I2S in -> DAC test (external device input to I2S RX, no ADC needed)\r\n");
	os_printf("  Example: aud_i2s_dac_test start a2dp 16000 0\r\n");
	os_printf("aud_adc_i2s_test {start|stop} {sample_rate} [gpio_group] \r\n");
	os_printf("  ADC -> I2S out test (no DAC needed)\r\n");
	os_printf("  Example: aud_adc_i2s_test start 16000 0\r\n");
#endif

#if CONFIG_AUDIO_DMIC
	os_printf("aud_dmic_mcp_test {start|stop sample_rate} \r\n");
	os_printf("aud_dmic_dma_test {start|stop sample_rate} \r\n");
	os_printf("aud_dmic_loop_test {start|stop sample_rate} \r\n");
#endif

	os_printf("aud_pcm_mcp_test {8000|16000|44100|48000} {start} \r\n");
	os_printf("aud_pcm_dma_test {8000|16000|44100|48000} {start|stop} \r\n");
	os_printf("aud_dac_dma_test {start|stop} {a2dp|call|hint} {sample_rate} \r\n");
	os_printf("aud_dac_mcp_test {start|stop} {a2dp|call|hint} {sample_rate} \r\n");
	os_printf("aud_dac_ringbuf_test {start|stop} {a2dp|call|hint} {sample_rate} \r\n");
#endif

#if CONFIG_I2S
	os_printf("aud_i2s_mcp_test {start|stop} {tx|rx} {sample_rate} [gpio_group] \r\n");
	os_printf("aud_i2s_dma_test {start|stop} {tx|rx|loopback} [dac_source] {sample_rate} [frame_duration] [stereo] [gpio_group] \r\n");
	os_printf("  Parameters:\r\n");
	os_printf("    dac_source: a2dp|call|hint (required for loopback, optional for RX, not used for TX)\r\n");
	os_printf("    sample_rate: 8000|16000|44100|48000 (required)\r\n");
	os_printf("    frame_duration: frame duration in ms (optional, default: 20)\r\n");
	os_printf("    stereo: 2=stereo, 1=mono (optional, default: 2)\r\n");
	os_printf("    gpio_group: I2S GPIO group ID (optional, default: 0)\r\n");
	os_printf("  TX mode examples:\r\n");
	os_printf("    aud_i2s_dma_test start tx 16000\r\n");
	os_printf("    aud_i2s_dma_test start tx 16000 20 2 0\r\n");
	os_printf("    aud_i2s_dma_test start tx 48000 10 1\r\n");
	os_printf("  RX mode examples:\r\n");
	os_printf("    aud_i2s_dma_test start rx call 16000\r\n");
	os_printf("    aud_i2s_dma_test start rx a2dp 48000 20 2 0\r\n");
	os_printf("    aud_i2s_dma_test start rx hint 8000 10 1\r\n");
	os_printf("  Loopback mode examples:\r\n");
	os_printf("    aud_i2s_dma_test start loopback call 16000\r\n");
	os_printf("    aud_i2s_dma_test start loopback call 16000 20 2 0\r\n");
	os_printf("    aud_i2s_dma_test start loopback a2dp 48000 10 1\r\n");
	os_printf("    aud_i2s_dma_test start loopback hint 8000 20 2\r\n");
	os_printf("  Stop:\r\n");
	os_printf("    aud_i2s_dma_test stop\r\n");
	os_printf("aud_i2s_dac_test {start|stop} {a2dp|call|hint} {sample_rate} [loopback] [gpio_group] \r\n");
	os_printf("aud_i2s_dac_ringbuf_test {start|stop} {a2dp|call|hint} {sample_rate} [gpio_group] \r\n");
#endif

	os_printf("aud_hw_test {start|stop} {loop|adc|dac|adc_dac} {sample_rate} {differential|single_end} {mic12|mic23|mic34|mic45|mic51} \r\n");
	os_printf("  Parameters:\r\n");
	os_printf("    start|stop: start or stop the hardware test\r\n");
	os_printf("    loop|adc|dac|adc_dac: test mode\r\n");
	os_printf("      loop: ADC to DAC loopback test\r\n");
	os_printf("      adc: ADC test (ADC to I2S output)\r\n");
	os_printf("      dac: DAC test (I2S input to DAC)\r\n");
	os_printf("      adc_dac: ADC and DAC simultaneous test\r\n");
	os_printf("    sample_rate: 8000|11025|12000|16000|22050|24000|32000|44100|48000\r\n");
	os_printf("    differential|single_end: ADC/DAC mode\r\n");
	os_printf("    mic12|mic23|mic34|mic45|mic51: microphone configuration\r\n");
	os_printf("  Examples:\r\n");
	os_printf("    aud_hw_test start loop 16000 differential mic12\r\n");
	os_printf("    aud_hw_test start adc 48000 single_end mic23\r\n");
	os_printf("    aud_hw_test start dac 44100 differential mic34\r\n");
	os_printf("    aud_hw_test start adc_dac 16000 single_end mic45\r\n");
	os_printf("    aud_hw_test stop\r\n");
}

#define PI 3.14159265358979323846
static uint32_t * signal = NULL;
static uint32_t signal_size = 0;   // sample cnt

static int generate_single_pcm(int sample_rate, int frequency, int amp_db, float duration_seconds, uint8_t type, int width)
{
    uint32_t *s = (uint32_t *)signal;
    double a = powf(10, amp_db/20.0f);
    int samples = (int)(sample_rate * duration_seconds);
    os_printf("[+]%s, samples: %d, %f\n", __func__, samples, a);
    if(1 == type)//xdac 12bits width/mono/offset binary
    {
        uint16_t *smp = (uint16_t *)signal;
        for (int i = 0; i < samples; i++) {
            int16_t value = (int16_t)(a * sin(2 * PI * frequency * i * 1.0f / sample_rate + PI / 2) * 0x7FF);
            value += 0x800;
            smp[i] = (value&0xFFF);
            LOGD("smp[%d]:0x%x,%d,%d\n",i,smp[i],(smp[i] >= (1<<11))?-((1<<12)-smp[i]):smp[i],value);
        }
    }
    else//16bits width/stereo/two's complement
    {
		if (width == 16) {
	        for (int i = 0; i < samples; i++) {
	            int16_t value = (int16_t)(a * sin(2 * PI * frequency * i * 1.0f / sample_rate + PI / 2) * 0x7FFF);
	            s[i] = (value & 0xFFFF) << 16 | (value & 0xFFFF);
				BK_LOG_RAW("0x%08x, ", s[i]);
				if ((i+1)%8 == 0)
					BK_LOG_RAW("\n");
	        }
		} else if (width == 24)
        {
	        for (int i = 0; i < samples; i++) {
	            int32_t value = (int32_t)(a * sin(2 * PI * frequency * i * 1.0f / sample_rate + PI / 2) * 0x7FFFFF);
	            s[i] = (value & 0xFFFFFFFF);
				BK_LOG_RAW("0x%08x, ", s[i]);
				if ((i+1)%8 == 0)
					BK_LOG_RAW("\n");
	        }
		}
    }
    return 0;
}

static void cli_aud_generate_pcm_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    int sample_rate, frequency, amp_db, duration_ms, samples;
    int width = 16;
    size_t mem_size;
    uint8_t type = 0;

    if (argc !=8 && argc !=7 && argc != 6 && argc != 2 && argc != 3) {
		goto exit;
    }

    if (os_strcmp(argv[1], "gen") == 0)
    {
        sample_rate = (int)strtoul(argv[2], NULL, 10);
        frequency   = (int)strtoul(argv[3], NULL, 10);
        amp_db      = (int)strtoul(argv[4], NULL, 10);
        duration_ms = (int)strtoul(argv[5], NULL, 10);

        if(6 < argc)
            type = (int)strtoul(argv[6], NULL, 10);

		if (argc > 7)
			width = (int)strtoul(argv[7], NULL, 10);

        if (sample_rate <= 0 || frequency <= 0 || duration_ms <= 0) {
			goto exit;
        }
        LOGI("%s:smp_rate=%d,freq=%d,amp_db=%d,dur_ms=%d,type=%d,width:%d\n",__func__, sample_rate, frequency, amp_db, duration_ms, type, width);

        // Calculate memory size needed (duration_ms is in milliseconds)
        samples = (sample_rate * duration_ms) / 1000;
        mem_size = samples * sizeof(uint32_t);

        if (signal != NULL) {
            os_free(signal);
            signal = NULL;
            signal_size = 0;
        }

        signal = (uint32_t *)os_malloc(mem_size);
        if (signal == NULL) {
            LOGE("Failed to allocate memory for PCM signal (size: %d bytes)\n", mem_size);
            return;
        }

        // Generate PCM signal (convert milliseconds to seconds)
        if (generate_single_pcm(sample_rate, frequency, amp_db, duration_ms / 1000.0f, type, width) != 0) {
            LOGE("Failed to generate PCM signal\n");
            os_free(signal);
            signal = NULL;
            signal_size = 0;
            return;
        }
        LOGI("PCM test signal generated successfully (%d samples, %d bytes)\n", samples, mem_size);

        // Save signal size for dump command
        signal_size = samples;
		return;
    }
    else if (os_strcmp(argv[1], "dump") == 0)
    {
        uint32_t *s = (uint32_t *)signal;
        if (signal == NULL || signal_size == 0) {
            LOGE("No PCM signal data available. Please generate signal first.\n");
            return;
        }
        rtos_delay_milliseconds(5000);
        uint32_t repeat_count = 1;
        if (argc == 3)
        {
            repeat_count = (uint32_t)strtoul(argv[2], NULL, 10);
        }
        for (uint32_t i = 0; i < repeat_count; i++)
        {
            // Always send the signal data once (only low 16 bits)
            for (uint32_t j = 0; j < signal_size; j++) {
                uint16_t value = (uint16_t)(s[j] & 0xFFFF);
                bk_uart_write_bytes(1, (void *)&value, sizeof(uint16_t));
            }
        }
        rtos_delay_milliseconds(5000);
		return;
    }	
exit:
	cli_aud_help();
	return;
}

static void audio_uart1_init()
{
	uart_config_t uart_config = {
		.baud_rate = 2000000,
		.data_bits = UART_DATA_8_BITS,
		.parity    = UART_PARITY_NONE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_FLOWCTRL_DISABLE,
		.src_clk   = UART_SCLK_XTAL_26M
	};
(
	*(volatile uint32_t *)(0x44000438))=0x62000048;
	bk_uart_init(1, &uart_config);
}


// Hardware reset function for audio subsystem
static void aud_hardware_reset(void)
{
	/* Reset audio registers: AUD_REG_0x2 (0x4101a008) and AUD_REG_0x5E (0x4101a178) */
	*((volatile UINT32 *)(0x4101a008)) = 0x1;
	*((volatile UINT32 *)(0x4101a178)) = 0x43210;
}

#if CONFIG_SOC_BK7259

extern const uint32_t AUD_PCM_8000[8];
extern const uint32_t AUD_PCM_16000[16];
extern const uint32_t AUD_PCM_44100[441];
extern const uint32_t AUD_PCM_48000[48];

extern const uint32_t AUD_PCM_8000_SEPERATE[16];
extern const uint32_t AUD_PCM_16000_SEPERATE[32];
extern const uint32_t AUD_PCM_44100_SEPERATE[882];
extern const uint32_t AUD_PCM_48000_SEPERATE[96];

extern const uint32_t AUD_PCM_8000_24BITS[8];
extern const uint32_t AUD_PCM_16000_24BITS[16];
extern const uint32_t AUD_PCM_44100_24BITS[441];
extern const uint32_t AUD_PCM_48000_24BITS[48];
extern const uint32_t AUD_PCM_16000_24BITS_STEREO[32];

static bool parse_dac_source(const char *src_str, aud_dac_source_t *dac_source)
{
	if (src_str == NULL || dac_source == NULL) {
		return false;
	}
	if (os_strcmp(src_str, "a2dp") == 0) {
		*dac_source = AUD_DAC_SOURCE_A2DP;
		return true;
	} else if (os_strcmp(src_str, "call") == 0) {
		*dac_source = AUD_DAC_SOURCE_CALL;
		return true;
	} else if (os_strcmp(src_str, "hint") == 0) {
		*dac_source = AUD_DAC_SOURCE_HINT;
		return true;
	}
	return false;
}

static bool get_dac_fifo_addr_by_source(aud_dac_source_t dac_source, dma_dev_t *dma_dev, uint32_t *dac_fifo_addr)
{
	if (dma_dev == NULL || dac_fifo_addr == NULL) {
		return false;
	}
	switch (dac_source) {
		case AUD_DAC_SOURCE_A2DP:
			*dma_dev = DMA_DEV_AUD_SPK0;
			bk_aud_dac_spk0_get_fifo_addr(AUD_DAC_SOURCE_A2DP, dac_fifo_addr);
			return true;
		case AUD_DAC_SOURCE_CALL:
			*dma_dev = DMA_DEV_AUD_SPK0_CALL;
			bk_aud_dac_spk0_get_fifo_addr(AUD_DAC_SOURCE_CALL, dac_fifo_addr);
			return true;
		case AUD_DAC_SOURCE_HINT:
			*dma_dev = DMA_DEV_AUD_SPK0_HINT;
			bk_aud_dac_spk0_get_fifo_addr(AUD_DAC_SOURCE_HINT, dac_fifo_addr);
			return true;
		default:
			return false;
	}
}

#if CONFIG_I2S
static uint8_t g_i2s_16r16l_mode = 0;

static bool get_i2s_test_data_by_rate_dbg(i2s_samp_rate_t samp_rate, const uint32_t **data_ptr, uint32_t *data_len, uint32_t width)
{
    if (data_ptr == NULL || data_len == NULL) {
        return false;
    }
    switch (samp_rate) {
        case I2S_SAMP_RATE_8000:
			if (width == 16) {
				*data_ptr = AUD_PCM_8000;
				*data_len = sizeof(AUD_PCM_8000);
			} else if (width == 24)
			{
				*data_ptr = AUD_PCM_8000_24BITS;
				*data_len = sizeof(AUD_PCM_8000_24BITS);
			}
            break;
        case I2S_SAMP_RATE_16000:
			if (width == 16) {
		            *data_ptr = AUD_PCM_16000;
		            *data_len = sizeof(AUD_PCM_16000);
			} else if (width == 24)
			{
				*data_ptr = AUD_PCM_16000_24BITS;
				*data_len = sizeof(AUD_PCM_16000_24BITS);
			}
            break;
        case I2S_SAMP_RATE_44100:
			if (width == 16) {
				*data_ptr = AUD_PCM_44100;
				*data_len = sizeof(AUD_PCM_44100);
			} else if (width == 24)
			{
				*data_ptr = AUD_PCM_44100_24BITS;
				*data_len = sizeof(AUD_PCM_44100_24BITS);
			}
            break;
        case I2S_SAMP_RATE_48000:
			if (width == 16) {
				*data_ptr = AUD_PCM_48000;
				*data_len = sizeof(AUD_PCM_48000);
			} else if (width == 24)
			{
				*data_ptr = AUD_PCM_48000_24BITS;
				*data_len = sizeof(AUD_PCM_48000_24BITS);
			}
            break;
        default:
            *data_ptr = AUD_PCM_8000;
            *data_len = sizeof(AUD_PCM_8000);
            break;
    }
    return true;
}
static bool get_i2s_test_data_by_rate(i2s_samp_rate_t samp_rate, const uint32_t **data_ptr, uint32_t *data_len)
{
    if (data_ptr == NULL || data_len == NULL) {
        return false;
    }
    switch (samp_rate) {
        case I2S_SAMP_RATE_8000:
            *data_ptr = AUD_PCM_8000;
            *data_len = sizeof(AUD_PCM_8000) / sizeof(AUD_PCM_8000[0]);
            break;
        case I2S_SAMP_RATE_16000:
            *data_ptr = AUD_PCM_16000;
            *data_len = sizeof(AUD_PCM_16000) / sizeof(AUD_PCM_16000[0]);
            break;
        case I2S_SAMP_RATE_44100:
            *data_ptr = AUD_PCM_44100;
            *data_len = sizeof(AUD_PCM_44100) / sizeof(AUD_PCM_44100[0]);
            break;
        case I2S_SAMP_RATE_48000:
            *data_ptr = AUD_PCM_48000;
            *data_len = sizeof(AUD_PCM_48000) / sizeof(AUD_PCM_48000[0]);
            break;
        default:
            *data_ptr = AUD_PCM_8000;
            *data_len = sizeof(AUD_PCM_8000) / sizeof(AUD_PCM_8000[0]);
            break;
    }
    return true;
}

static bool get_i2s_test_data_by_rate_seperate(i2s_samp_rate_t samp_rate, const uint32_t **data_ptr, uint32_t *data_len)
{
    if (data_ptr == NULL || data_len == NULL) {
        return false;
    }
    switch (samp_rate) {
        case I2S_SAMP_RATE_8000:
            *data_ptr = AUD_PCM_8000_SEPERATE;
            *data_len = sizeof(AUD_PCM_8000_SEPERATE) / sizeof(AUD_PCM_8000_SEPERATE[0]);
            break;
        case I2S_SAMP_RATE_16000:
            *data_ptr = AUD_PCM_16000_SEPERATE;
            *data_len = sizeof(AUD_PCM_16000_SEPERATE) / sizeof(AUD_PCM_16000_SEPERATE[0]);
            break;
        case I2S_SAMP_RATE_44100:
            *data_ptr = AUD_PCM_44100_SEPERATE;
            *data_len = sizeof(AUD_PCM_44100_SEPERATE) / sizeof(AUD_PCM_44100_SEPERATE[0]);
            break;
        case I2S_SAMP_RATE_48000:
            *data_ptr = AUD_PCM_48000_SEPERATE;
            *data_len = sizeof(AUD_PCM_48000_SEPERATE) / sizeof(AUD_PCM_48000_SEPERATE[0]);
            break;
        default:
            *data_ptr = AUD_PCM_8000_SEPERATE;
            *data_len = sizeof(AUD_PCM_8000_SEPERATE) / sizeof(AUD_PCM_8000_SEPERATE[0]);
            break;
    }
    return true;
}
#endif

static bool get_dac_test_data_by_rate(uint32_t sample_rate, const uint32_t **data_ptr, uint32_t *data_len)
{
    if (data_ptr == NULL || data_len == NULL) {
        return false;
    }
    switch (sample_rate) {
        case 8000:
            *data_ptr = AUD_PCM_8000;
            *data_len = sizeof(AUD_PCM_8000) / sizeof(AUD_PCM_8000[0]);
            break;
        case 16000:
            *data_ptr = AUD_PCM_16000;
            *data_len = sizeof(AUD_PCM_16000) / sizeof(AUD_PCM_16000[0]);
            break;
        case 44100:
            *data_ptr = AUD_PCM_44100;
            *data_len = sizeof(AUD_PCM_44100) / sizeof(AUD_PCM_44100[0]);
            break;
        case 48000:
            *data_ptr = AUD_PCM_48000;
            *data_len = sizeof(AUD_PCM_48000) / sizeof(AUD_PCM_48000[0]);
            break;
        default:
            *data_ptr = NULL;
            *data_len = 0;
            return false;
    }
    return true;
}
#endif

#if CONFIG_AUDIO_DAC

static void *aud_ptr = NULL;
static uint32_t aud_len = 0;
static beken_queue_t transfer_buf_done_que = NULL;

static void *dac_mcp_aud_ptr = NULL;
static uint32_t dac_mcp_aud_len = 0;
static beken_thread_t dac_mcp_thread = NULL;
static aud_dac_source_t dac_mcp_source = AUD_DAC_SOURCE_A2DP;
static bool dac_mcp_test_initialized = false;	

// DMA test variables with ring buffer
static RingBufferContext *dac_dma_rb = NULL;
static int32_t *dac_dma_ring_buff = NULL;
static void *dac_dma_aud_ptr = NULL;
static uint32_t dac_dma_aud_len = 0;
static dma_id_t dac_dma_id = DMA_ID_MAX;
static aud_dac_source_t dac_dma_source = AUD_DAC_SOURCE_A2DP;
static bool dac_dma_test_initialized = false;
#define DAC_DMA_RING_BUFF_SAFE_INTERVAL 20

#if (CONFIG_AUDIO_DAC && CONFIG_AUDIO_RING_BUFF)
// Ringbuf test variables
static RingBufferContext *dac_ringbuf_rb = NULL;
static uint8_t *dac_ringbuf_buffer = NULL;
#define DAC_RINGBUF_SIZE (4096)  // 4KB ring buffer
static beken_thread_t dac_ringbuf_write_thread = NULL;
static beken_thread_t dac_ringbuf_read_thread = NULL;
static bool dac_ringbuf_test_running = false;
static bool dac_ringbuf_test_initialized = false;
static void *dac_ringbuf_aud_ptr = NULL;
static uint32_t dac_ringbuf_aud_len = 0;
static uint32_t dac_ringbuf_aud_idx = 0;
static aud_dac_source_t dac_ringbuf_source = AUD_DAC_SOURCE_A2DP;
#endif

#define SPK_RING_BUFF_SIZE      (192 * 2 + 8)

#if CONFIG_AUDIO_DTMF

static void cli_aud_dtmf_isr(void)
{
	uint32_t dtmf_data;
	uint32_t dtmf_status = 0;

	bk_aud_dtmf_get_status(&dtmf_status);
	if ((dtmf_status & AUD_DTMF_NEAR_FULL_MASK) || (dtmf_status & AUD_DTMF_FIFO_FULL_MASK)) {
		bk_aud_dtmf_get_fifo_data(&dtmf_data);
		bk_aud_dac_write(dtmf_data);
	}
}

static void cli_aud_dtmf_mcp_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	aud_dtmf_config_t dtmf_config = DEFAULT_AUD_DTMF_CONFIG();
	aud_dac_config_t dac_config = DEFAULT_AUD_DAC_CONFIG();

	if (argc != 2) {
		cli_aud_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		os_printf("audio dtmf mcp test start\n");

		//init audio dtmf and dac driver
		ret = bk_aud_dtmf_init(&dtmf_config);
		if (ret != BK_OK) {
			os_printf("bk_aud_dtmf_init fail \n");
			return;
		}
		os_printf("init audio dtmf successful\n");

		ret = bk_aud_dac_init(&dac_config);
		if (ret != BK_OK) {
			os_printf("bk_aud_dac_init fail \n");
			return;
		}
		os_printf("init audio dac successful\n");

		//register isr
		ret = bk_aud_dtmf_register_isr(cli_aud_dtmf_isr);
		if (ret != BK_OK)
			return;
		os_printf("register dtmf isr successful\n");

		//enable audio dtmf interrupt
		bk_aud_dtmf_enable_int();
		os_printf("enable dtmf interrupt successful\n");

		//start adc and dac
		bk_aud_dtmf_start();
		bk_aud_dac_start();
		os_printf("enable dtmf and dac successful\n");

		os_printf("start audio dtmf mcp test successful\r\n");
	} else if (os_strcmp(argv[1], "stop") == 0) {
		os_printf("audio dtmf mcp test stop\n");
		bk_aud_dtmf_stop();
		bk_aud_dac_stop();
		bk_aud_dtmf_deinit();
		bk_aud_dac_deinit();
		os_printf("audio dtmf mcp test stop successful\n");
	} else {
		cli_aud_help();
		return;
	}
}

static void cli_aud_dtmf_loop_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	aud_dtmf_config_t dtmf_config = DEFAULT_AUD_DTMF_CONFIG();
	aud_dac_config_t dac_config = DEFAULT_AUD_DAC_CONFIG();

	if (argc != 2) {
		cli_aud_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		os_printf("audio dtmf loop test start\n");

		//init audio dtmf and dac driver
		ret = bk_aud_dtmf_init(&dtmf_config);
		if (ret != BK_OK) {
			os_printf("bk_aud_dtmf_init fail \n");
			return;
		}
		os_printf("init audio dtmf successful\n");

		ret = bk_aud_dac_init(&dac_config);
		if (ret != BK_OK) {
			os_printf("bk_aud_dac_init fail \n");
			return;
		}
		os_printf("init audio dac successful\n");

		//start dtmf and dac
		ret = bk_aud_dtmf_start();
		if (ret != BK_OK) {
			os_printf("bk_aud_dtmf_start fail\n");
			return;
		}
		os_printf("enable dtmf and dac successful\n");
		ret = bk_aud_dac_start();
		if (ret != BK_OK) {
			os_printf("bk_aud_dac_start fail\n");
			return;
		}

		//enable dtmf to dac loop test
		bk_aud_dtmf_start_loop_test();

		os_printf("enable dtmf to dac loop test successful\n");
	} else if (os_strcmp(argv[1], "stop") == 0) {
		os_printf("audio dtmf loop test stop\n");
		//stop loop test
		bk_aud_dtmf_stop_loop_test();
		//disable adc and dac
		bk_aud_dtmf_stop();
		bk_aud_dac_stop();

		bk_aud_dtmf_deinit();
		bk_aud_dac_deinit();

		os_printf("audio dtmf loop test stop successful\n");
	} else {
		cli_aud_help();
		return;
	}
}
#endif	//#if CONFIG_AUDIO_DTMF

#if CONFIG_AUDIO_ADC
static dma_id_t adc_dma_test_dma_id = DMA_ID_MAX;

static aud_dac_source_t adc_dma_dump_dac_source = AUD_DAC_SOURCE_A2DP;

// ADC to DAC DMA with ring buffer (dual ring buffer architecture)
static dma_id_t adc_ringbuf_adc_dma_id = DMA_ID_MAX;   // DMA from ADC FIFO to ADC ring buffer
static dma_id_t adc_ringbuf_dac_dma_id = DMA_ID_MAX;   // DMA from DAC ring buffer to DAC FIFO

static uint32_t dma_transfer_len = 0;  // Length for dump data (20ms)

static uint32_t g_cnt1 = 0;
static uint32_t g_cnt2 = 0;
static uint32_t g_cnt3 = 0;

// ADC ring buffer (from ADC FIFO)
static RingBufferContext *adc_ringbuf_rb = NULL;
static int32_t *adc_ringbuf_buffer = NULL;
static uint32_t adc_ringbuf_size = 0;
static uint32_t dac_ringbuf_size = 0;

#define ADC_RINGBUF_SAFE_INTERVAL 20

#if 1
static bool g_adc_dma_test_initialized = false;
static void cli_aud_adc_dac_dma_loopback_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_err_t ret = BK_OK;
    uint32_t adc_fifo_addr = 0;
    uint32_t dac_fifo_addr = 0;
    dma_config_t dma_config = {0};
    aud_adc_config_t adc_config = DEFAULT_AUD_ADC_CONFIG();
    aud_dac_config_t dac_config = DEFAULT_AUD_DAC_CONFIG();
    aud_dac_source_t dac_source = AUD_DAC_SOURCE_A2DP;

    // Check arguments: start needs 4 args, stop needs 2 args, dump needs 3 args
    if (argc < 2) {
        cli_aud_help();
        return;
    }

    if (os_strcmp(argv[1], "start") == 0) {
        if (argc != 4) {
            cli_aud_help();
            return;
        }
        if (os_strcmp(argv[2], "a2dp") == 0) {
            dac_source = AUD_DAC_SOURCE_A2DP;
        } else if (os_strcmp(argv[2], "call") == 0) {
            dac_source = AUD_DAC_SOURCE_CALL;
        } else if (os_strcmp(argv[2], "hint") == 0) {
            dac_source = AUD_DAC_SOURCE_HINT;
        } else {
            os_printf("dac source type: %s is not support\n", argv[2]);
            cli_aud_help();
            return;
        }

        adc_config.sample_rate = strtoul(argv[3], NULL, 10);
        uint32_t dac_sample_rate = adc_config.sample_rate;

        /* check whether dac source support sample rate */
        switch (dac_source)
        {
            case AUD_DAC_SOURCE_A2DP:
                if (dac_sample_rate != 44100 && dac_sample_rate != 48000) {
                    os_printf("dac a2dp source not support sample rate: %d\n", dac_sample_rate);
                    cli_aud_help();
                    return;
                }
                break;
            case AUD_DAC_SOURCE_CALL:
            case AUD_DAC_SOURCE_HINT:
                if (dac_sample_rate != 8000 && dac_sample_rate != 16000 && dac_sample_rate != 48000) {
                    os_printf("dac a2dp source not support sample rate: %d\n", dac_sample_rate);
                    cli_aud_help();
                    return;
                }
                break;

            default:
                break;
        }

        aud_hardware_reset();

        //init audio adc and dac driver
        ret = bk_aud_dac_init(&dac_config);
        if (ret != BK_OK) {
            os_printf("bk_aud_dac_init fail \n");
            return;
        }
        bk_aud_dac_set_sample_rate(dac_source, dac_sample_rate);

        ret = bk_aud_adc_init(&adc_config);
        if (ret != BK_OK) {
            os_printf("bk_aud_adc_init fail \n");
            return;
        }
        os_printf("init audio dac successful\n");

        //init dma driver
        ret = bk_dma_driver_init();
        if (ret != BK_OK) {
            os_printf("dma driver init failed\r\n");
            return;
        }
        dma_config.mode      = DMA_WORK_MODE_REPEAT;
        dma_config.chan_prio = 1;
        dma_config.src.dev   = DMA_DEV_AUD_MIC0;
        dma_config.src.width = DMA_DATA_WIDTH_16BITS;
        dma_config.dst.width = DMA_DATA_WIDTH_16BITS;
        //get adc fifo address
        if (bk_aud_adc_get_fifo_addr(AUD_ADC_MIC_DATA_BUS_0, &adc_fifo_addr) != BK_OK) {
            os_printf("get adc fifo address failed\r\n");
            return;
        } else {
            dma_config.src.addr_inc_en  = DMA_ADDR_INC_ENABLE;
            dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
            dma_config.src.start_addr   = adc_fifo_addr;
            dma_config.src.end_addr     = adc_fifo_addr + 4;
        }
        //get dac fifo address
        if (!get_dac_fifo_addr_by_source(dac_source, &dma_config.dst.dev, &dac_fifo_addr)) {
            os_printf("get dac fifo address failed\r\n");
            return;
        }
        dma_config.dst.addr_inc_en  = DMA_ADDR_INC_ENABLE;
        dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
        dma_config.dst.start_addr   = dac_fifo_addr;
        dma_config.dst.end_addr     = dac_fifo_addr + 4;
        os_printf("source_addr:0x%x, dest_addr:0x%x\r\n", dma_config.src.start_addr, dma_config.dst.start_addr);
        os_printf("dma src_dev:%d, dst_dev:%d, src_width:%d, dst_width:%d\r\n", 
                   dma_config.src.dev, dma_config.dst.dev, dma_config.src.width, dma_config.dst.width);

        //init dma channel
        adc_dma_test_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
        if ((adc_dma_test_dma_id < DMA_ID_0) || (adc_dma_test_dma_id >= DMA_ID_MAX)) {
            os_printf("malloc dma fail \r\n");
            return;
        }
        os_printf("dma_id: %d\n", adc_dma_test_dma_id);

        ret = bk_dma_init(adc_dma_test_dma_id, &dma_config);
        if (ret != BK_OK) {
            os_printf("dma init failed\r\n");
            adc_dma_test_dma_id = DMA_ID_MAX;
            return;
        }
        bk_dma_set_transfer_len(adc_dma_test_dma_id, 4);

#if (CONFIG_SPE)
        bk_dma_set_dest_sec_attr(adc_dma_test_dma_id, DMA_ATTR_SEC);
        bk_dma_set_src_sec_attr(adc_dma_test_dma_id, DMA_ATTR_SEC);
#endif

        bk_aud_adc_set_bits_width(AUD_ADC_CHL_0, 16);
        bk_aud_dac_set_bits_width(dac_source, 16);

        bk_aud_dac_spk0_source_enable(dac_source, 1);

        ret = bk_aud_dac_start(AUD_DAC_CHL_LR);
        if (ret != BK_OK) {
            os_printf("bk_aud_dac_start fail \n");
            bk_dma_deinit(adc_dma_test_dma_id);
            adc_dma_test_dma_id = DMA_ID_MAX;
            return;
        }

        // Check DAC FIFO status to ensure it's ready
        uint32_t dac_fifo_status = 0;
        bk_aud_dac_get_fifo_status(&dac_fifo_status);
        os_printf("dac_fifo_status before DMA start: 0x%x\n", dac_fifo_status);
        if (dac_fifo_status != (1 << dac_source)) {
            os_printf("Warning: DAC FIFO status mismatch, expected: 0x%x, got: 0x%x\n", (1 << dac_source), dac_fifo_status);
        }

        // Start DMA after DAC is ready
        ret = bk_dma_start(adc_dma_test_dma_id);
        if (ret != BK_OK) {
            os_printf("dma start fail \n");
            bk_aud_dac_stop(AUD_DAC_CHL_LR);
            bk_dma_deinit(adc_dma_test_dma_id);
            adc_dma_test_dma_id = DMA_ID_MAX;
            return;
        }
        os_printf("DMA started successfully\n");

        // Check DAC FIFO status after DMA start
        bk_aud_dac_get_fifo_status(&dac_fifo_status);
        os_printf("dac_fifo_status after DMA start: 0x%x\n", dac_fifo_status);
        if (dac_fifo_status != (1 << dac_source)) {
            os_printf("Warning: DAC FIFO status changed after DMA start, expected: 0x%x, got: 0x%x\n", (1 << dac_source), dac_fifo_status);
        }

        // Start ADC after DMA is running
        ret = bk_aud_adc_start(AUD_ADC_CHL_0);
        if (ret != BK_OK) {
            os_printf("bk_aud_adc_start fail \n");
            return;
        }
        bk_aud_adc_enable_used_channel(1<<AUD_ADC_CHL_0);

        os_printf("ADC started successfully\n");
        
        // Check DAC FIFO status again
        bk_aud_dac_get_fifo_status(&dac_fifo_status);
        if (dac_fifo_status != (1 << dac_source)) {
            os_printf("Warning: DAC FIFO status changed after ADC start, expected: 0x%x, got: 0x%x\n", (1 << dac_source), dac_fifo_status);
        }

        g_adc_dma_test_initialized = true;
        os_printf("enable adc and dac successful\n");
    }
    else if (os_strcmp(argv[1], "stop") == 0)
    {

        if (!g_adc_dma_test_initialized) {
            return;
        }

        //disable adc and dac
        bk_aud_adc_deinit();
        bk_aud_dac_deinit();
        //stop dma
        if (adc_dma_test_dma_id != DMA_ID_MAX) {
            bk_dma_stop(adc_dma_test_dma_id);
            bk_dma_deinit(adc_dma_test_dma_id);
            ret = bk_dma_free(DMA_DEV_AUDIO, adc_dma_test_dma_id);
            if (ret == BK_OK)
                os_printf("free dma: %d success\r\n", adc_dma_test_dma_id);
            adc_dma_test_dma_id = DMA_ID_MAX;
        }
        g_adc_dma_test_initialized = false;
    } else {
        cli_aud_help();
        return;
    }
}
#endif

#if CONFIG_AUDIO_RING_BUFF
static uint8_t g_adc_ringbuf_dump_mode = 0;  // 0: no dump, 1: dump ADC, 2: dump DAC
static uint8_t g_adc_ringbuf_initialized = 0;
static beken_queue_t adc_to_dac_transfer_que = NULL;
static beken_thread_t adc_ringbuf_transfer_thread = NULL;

typedef enum {
	ADC_TO_DAC_TRANSFER_MSG_DONE     = 0,
	ADC_TO_DAC_TRANSFER_MSG_NOT_DONE = 1,
} adc_to_dac_transfer_msg_t;

bk_err_t adc_to_dac_transfer_send_msg(adc_to_dac_transfer_msg_t done)
{
	bk_err_t ret;
	if (adc_to_dac_transfer_que) {
		ret = rtos_push_to_queue(&adc_to_dac_transfer_que, &done, BEKEN_NO_WAIT);
		if (BK_OK != ret) {
			os_printf("send msg to adc_to_dac_transfer_que failed, ret: %d\r\n", ret);
			return BK_FAIL;
		}
		return BK_OK;
	}
	return BK_FAIL;
}

static void aud_adc_ringbuf_transfer_thread(beken_thread_arg_t data)
{
	bk_err_t ret = BK_OK;
	uint32_t temp_transfer_buf[480];  // Temporary buffer for transfer (640 bytes max)
	while(1)
	{
		adc_to_dac_transfer_msg_t done;
		ret = rtos_pop_from_queue(&adc_to_dac_transfer_que, &done, BEKEN_WAIT_FOREVER);
		if (BK_OK == ret) {
			switch (done) {
				case ADC_TO_DAC_TRANSFER_MSG_DONE:
					// Transfer data from ADC ring buffer to DAC ring buffer
					// ISR is triggered every dma_transfer_len bytes, so we should read dma_transfer_len bytes
					if (adc_ringbuf_rb != NULL && dac_ringbuf_rb != NULL && dma_transfer_len > 0) {
						uint32_t adc_fill_size = ring_buffer_get_fill_size(adc_ringbuf_rb);
						uint32_t dac_free_size = ring_buffer_get_free_size(dac_ringbuf_rb);
						
						// Check if we have enough data in ADC ring buffer and space in DAC ring buffer
						if (adc_fill_size >= dma_transfer_len && dac_free_size >= dma_transfer_len) {
							// Ensure temp buffer is large enough
							uint32_t max_transfer = (dma_transfer_len > sizeof(temp_transfer_buf)) ? sizeof(temp_transfer_buf) : dma_transfer_len;
							max_transfer = (max_transfer / 4) * 4;  // Align to 4 bytes
							
							// Read dma_transfer_len bytes from ADC ring buffer
							uint32_t read_size = ring_buffer_read(adc_ringbuf_rb, (uint8_t *)temp_transfer_buf, max_transfer);
							if (read_size == max_transfer) {
								// Write to DAC ring buffer
								uint32_t write_size = ring_buffer_write(dac_ringbuf_rb, (uint8_t *)temp_transfer_buf, read_size);
								g_cnt2++;
								if (g_adc_ringbuf_dump_mode == 1) {
									bk_uart_write_bytes(1, (void *)temp_transfer_buf, read_size);
								}
								if (write_size != read_size) {
									//os_printf("the data write to dac_ringbuf_buffer error, size: %d, expected: %d\n", write_size, read_size);
								}
							} else {
								//os_printf("the data read from adc_ringbuf_rb error, size: %d, expected: %d\n", read_size, max_transfer);
							}
						}
					}
					break;
				case ADC_TO_DAC_TRANSFER_MSG_NOT_DONE:
					// Error case, skip this message
					break;
				default:
					os_printf("invalid transfer message: %d\r\n", done);
					break;
			}
		}
	}
}

static void aud_adc_ringbuf_adc_dma_finish_isr(dma_id_t dma_id)
{
	GPIO_DOWN(36);GPIO_UP(36);
	if (adc_ringbuf_rb == NULL || dma_id != adc_ringbuf_adc_dma_id) {
		return;
	}

	if (dac_ringbuf_rb != NULL)
	{
		adc_to_dac_transfer_send_msg(ADC_TO_DAC_TRANSFER_MSG_DONE);
	}
	GPIO_DOWN(36);
}

static void aud_adc_ringbuf_dac_dma_finish_isr(dma_id_t dma_id)
{
	GPIO_DOWN(37);GPIO_UP(37);

	if (adc_ringbuf_rb == NULL || dma_id != adc_ringbuf_dac_dma_id) {
		return;
	}

	// Check filled data in ring buffer
	uint32_t fill_size = ring_buffer_get_fill_size(adc_ringbuf_rb);
	//uint32_t free_size = ring_buffer_get_free_size(adc_ringbuf_rb);

	if (fill_size < 4) {
		return;
	}
	GPIO_DOWN(37);
}

static void cli_aud_adc_dac_ringbuf_loopback_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_err_t ret = BK_OK;
    dma_config_t adc_dma_config = {0};
    dma_config_t dac_dma_config = {0};
    aud_adc_config_t adc_config = DEFAULT_AUD_ADC_CONFIG();
    aud_dac_config_t dac_config = DEFAULT_AUD_DAC_CONFIG();
    aud_dac_source_t dac_source = AUD_DAC_SOURCE_A2DP;
    uint32_t adc_fifo_addr = 0;
    uint32_t dac_fifo_addr = 0;
    uint32_t dac_fifo_status = 0;

    if (argc < 2) {
        cli_aud_help();
        return;
    }

    if (os_strcmp(argv[1], "start") == 0) {
        if (argc != 4) {
            cli_aud_help();
            return;
        }
        if (os_strcmp(argv[2], "a2dp") == 0) {
            dac_source = AUD_DAC_SOURCE_A2DP;
        } else if (os_strcmp(argv[2], "call") == 0) {
            dac_source = AUD_DAC_SOURCE_CALL;
        } else if (os_strcmp(argv[2], "hint") == 0) {
            dac_source = AUD_DAC_SOURCE_HINT;
        } else {
            os_printf("dac source type: %s is not support\n", argv[2]);
            cli_aud_help();
            return;
        }

        adc_config.sample_rate = strtoul(argv[3], NULL, 10);
        uint32_t dac_sample_rate = adc_config.sample_rate;

        /* check whether dac source support sample rate */
        switch (dac_source)
        {
            case AUD_DAC_SOURCE_A2DP:
                if (dac_sample_rate != 44100 && dac_sample_rate != 48000) {
                    os_printf("dac a2dp source not support sample rate: %d\n", dac_sample_rate);
                    cli_aud_help();
                    return;
                }
                break;
            case AUD_DAC_SOURCE_CALL:
            case AUD_DAC_SOURCE_HINT:
                if (dac_sample_rate != 8000 && dac_sample_rate != 16000 && dac_sample_rate != 48000) {
                    os_printf("dac source not support sample rate: %d\n", dac_sample_rate);
                    cli_aud_help();
                    return;
                }
                break;
            default:
                break;
        }

        aud_hardware_reset();
        adc_dma_dump_dac_source = dac_source;

		// Initialize queue for ADC to DAC transfer
		if (adc_to_dac_transfer_que == NULL) {
			ret = rtos_init_queue(&adc_to_dac_transfer_que,
									"adc_to_dac_transfer_que",
									sizeof(adc_to_dac_transfer_msg_t),
									50);
			if (ret != BK_OK) {
				os_printf("init adc_to_dac_transfer_que failed\r\n");
				return;
			}
		}

		// Create transfer thread to move data from ADC ring buffer to DAC ring buffer
		if (adc_ringbuf_transfer_thread == NULL) {
			ret = rtos_create_thread(&adc_ringbuf_transfer_thread,
										BEKEN_DEFAULT_WORKER_PRIORITY - 1,
										"adc_ringbuf_transfer_thread",
										(beken_thread_function_t)aud_adc_ringbuf_transfer_thread,
										1024 * 4,
										NULL);
			if (ret != BK_OK) {
				os_printf("%s, init transfer thread failed\r\n", __func__);
				return;
			}
		}

        //init audio adc and dac driver
        ret = bk_aud_dac_init(&dac_config);
        if (ret != BK_OK) {
            os_printf("bk_aud_dac_init fail \n");
            return;
        }
        bk_aud_dac_set_sample_rate(dac_source, dac_sample_rate);

        ret = bk_aud_adc_init(&adc_config);
        if (ret != BK_OK) {
            os_printf("bk_aud_adc_init fail \n");
            return;
        }

        adc_ringbuf_size = adc_config.sample_rate / 1000 * 20 * 2 * 8 + ADC_RINGBUF_SAFE_INTERVAL;  // 1 channels, 2 bytes per sample, 2x buffering
        adc_ringbuf_buffer = (int32_t *)os_malloc(adc_ringbuf_size);
        if (adc_ringbuf_buffer == NULL) {
            os_printf("Failed to allocate ring buffer\n");
            bk_aud_adc_deinit();
            bk_aud_dac_deinit();
            return;
        }
        os_memset(adc_ringbuf_buffer, 0, adc_ringbuf_size);

        // Initialize ADC ring buffer context
        adc_ringbuf_rb = (RingBufferContext *)os_malloc(sizeof(RingBufferContext));
        if (adc_ringbuf_rb == NULL) {
            os_printf("Failed to allocate ADC ring buffer context\n");
            os_free(adc_ringbuf_buffer);
            adc_ringbuf_buffer = NULL;
            bk_aud_adc_deinit();
            bk_aud_dac_deinit();
            return;
        }

        // Allocate DAC ring buffer (same size as ADC ring buffer)
        dac_ringbuf_size = adc_ringbuf_size;
        dac_ringbuf_buffer = (uint8_t *)os_malloc(dac_ringbuf_size);
        if (dac_ringbuf_buffer == NULL) {
            os_printf("Failed to allocate DAC ring buffer\n");
            os_free(adc_ringbuf_rb);
            os_free(adc_ringbuf_buffer);
            adc_ringbuf_rb = NULL;
            adc_ringbuf_buffer = NULL;
            bk_aud_adc_deinit();
            bk_aud_dac_deinit();
            return;
        }
        os_memset(dac_ringbuf_buffer, 0, dac_ringbuf_size);

        // Initialize DAC ring buffer context
        dac_ringbuf_rb = (RingBufferContext *)os_malloc(sizeof(RingBufferContext));
        if (dac_ringbuf_rb == NULL) {
            os_printf("Failed to allocate DAC ring buffer context\n");
            os_free(dac_ringbuf_buffer);
            os_free(adc_ringbuf_rb);
            os_free(adc_ringbuf_buffer);
            dac_ringbuf_buffer = NULL;
            adc_ringbuf_rb = NULL;
            adc_ringbuf_buffer = NULL;
            bk_aud_adc_deinit();
            bk_aud_dac_deinit();
            return;
        }

        //init dma driver
        ret = bk_dma_driver_init();
        if (ret != BK_OK) {
            os_printf("dma driver init failed\r\n");
            os_free(adc_ringbuf_rb);
            os_free(adc_ringbuf_buffer);
            adc_ringbuf_rb = NULL;
            adc_ringbuf_buffer = NULL;
            bk_aud_adc_deinit();
            bk_aud_dac_deinit();
            return;
        }

		// Allocate ADC DMA channel
		adc_ringbuf_adc_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
		if ((adc_ringbuf_adc_dma_id < DMA_ID_0) || (adc_ringbuf_adc_dma_id >= DMA_ID_MAX)) {
			os_printf("malloc adc dma fail \r\n");
			os_free(adc_ringbuf_rb);
			os_free(adc_ringbuf_buffer);
			adc_ringbuf_rb = NULL;
			adc_ringbuf_buffer = NULL;
			bk_aud_adc_deinit();
			return;
		}
        // ========== DMA1: ADC FIFO to Ring Buffer ==========
        // Configure DMA1: ADC FIFO -> Ring Buffer
        adc_dma_config.mode      = DMA_WORK_MODE_REPEAT;
        adc_dma_config.chan_prio = 1;
        adc_dma_config.src.dev   = DMA_DEV_AUD_MIC0;
        adc_dma_config.src.width = DMA_DATA_WIDTH_16BITS;
        adc_dma_config.dst.dev   = DMA_DEV_DTCM;
        adc_dma_config.dst.width = DMA_DATA_WIDTH_32BITS;

        // Get ADC FIFO address
        if (bk_aud_adc_get_fifo_addr(AUD_ADC_MIC_DATA_BUS_0, &adc_fifo_addr) != BK_OK) {
            os_printf("get adc fifo address failed\r\n");
            os_free(adc_ringbuf_rb);
            os_free(adc_ringbuf_buffer);
            adc_ringbuf_rb = NULL;
            adc_ringbuf_buffer = NULL;
            bk_dma_free(DMA_DEV_AUD_MIC0, adc_ringbuf_adc_dma_id);
            adc_ringbuf_adc_dma_id = DMA_ID_MAX;
            bk_aud_adc_deinit();
            bk_aud_dac_deinit();
            return;
        }

        adc_dma_config.src.addr_inc_en  = DMA_ADDR_INC_ENABLE;  // FIFO address is fixed
        adc_dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
        adc_dma_config.src.start_addr   = adc_fifo_addr;
        adc_dma_config.src.end_addr     = adc_fifo_addr + 4;

        adc_dma_config.dst.addr_inc_en  = DMA_ADDR_INC_ENABLE;
        adc_dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
        adc_dma_config.dst.start_addr   = (uint32_t)adc_ringbuf_buffer;
        adc_dma_config.dst.end_addr     = (uint32_t)adc_ringbuf_buffer + adc_ringbuf_size;

        ret = bk_dma_init(adc_ringbuf_adc_dma_id, &adc_dma_config);
        if (ret != BK_OK) {
            os_printf("adc dma init failed\r\n");
            os_free(adc_ringbuf_rb);
            os_free(adc_ringbuf_buffer);
            adc_ringbuf_rb = NULL;
            adc_ringbuf_buffer = NULL;
            bk_dma_free(DMA_DEV_AUDIO, adc_ringbuf_adc_dma_id);
            adc_ringbuf_adc_dma_id = DMA_ID_MAX;
            bk_aud_adc_deinit();
            bk_aud_dac_deinit();
            return;
        }

        dma_transfer_len = adc_config.sample_rate / 1000 * 20 * 2;
        bk_dma_set_transfer_len(adc_ringbuf_adc_dma_id, dma_transfer_len);

        bk_dma_register_isr(adc_ringbuf_adc_dma_id, NULL, (void *)aud_adc_ringbuf_adc_dma_finish_isr);
        bk_dma_enable_finish_interrupt(adc_ringbuf_adc_dma_id);

#if (CONFIG_SPE)
        bk_dma_set_dest_sec_attr(adc_ringbuf_adc_dma_id, DMA_ATTR_SEC);
        bk_dma_set_src_sec_attr(adc_ringbuf_adc_dma_id, DMA_ATTR_SEC);
#endif

        // Set ADC write threshold to trigger interrupt/DMA
        bk_aud_adc_set_write_threshold(AUD_ADC_MIC_DATA_BUS_0, 3);

        // ========== DMA3: DAC Ring Buffer to DAC FIFO ==========
        adc_ringbuf_dac_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
        if ((adc_ringbuf_dac_dma_id < DMA_ID_0) || (adc_ringbuf_dac_dma_id >= DMA_ID_MAX)) {
            os_printf("malloc dac dma fail \r\n");
            os_free(dac_ringbuf_rb);
            os_free(dac_ringbuf_buffer);
            os_free(adc_ringbuf_rb);
            os_free(adc_ringbuf_buffer);
            dac_ringbuf_rb = NULL;
            dac_ringbuf_buffer = NULL;
            adc_ringbuf_rb = NULL;
            adc_ringbuf_buffer = NULL;
            bk_aud_adc_deinit();
            bk_aud_dac_deinit();
            return;
        }

        // Configure DMA3: DAC Ring Buffer -> DAC FIFO
        dac_dma_config.mode      = DMA_WORK_MODE_REPEAT;
        dac_dma_config.chan_prio = 1;
        dac_dma_config.src.dev   = DMA_DEV_DTCM;
        dac_dma_config.src.width = DMA_DATA_WIDTH_32BITS;
        dac_dma_config.dst.width = DMA_DATA_WIDTH_16BITS;
        dac_dma_config.dst.dev   = DMA_DEV_AUDIO;

        // Get DAC FIFO address
        if (!get_dac_fifo_addr_by_source(dac_source, &dac_dma_config.dst.dev, &dac_fifo_addr)) {
            os_printf("get dac fifo address failed\r\n");
            bk_dma_free(DMA_DEV_AUDIO, adc_ringbuf_dac_dma_id);
            adc_ringbuf_dac_dma_id = DMA_ID_MAX;
            os_free(dac_ringbuf_rb);
            os_free(dac_ringbuf_buffer);
            os_free(adc_ringbuf_rb);
            os_free(adc_ringbuf_buffer);
            dac_ringbuf_rb = NULL;
            dac_ringbuf_buffer = NULL;
            adc_ringbuf_rb = NULL;
            adc_ringbuf_buffer = NULL;
            bk_aud_adc_deinit();
            bk_aud_dac_deinit();
            return;
        }

        dac_dma_config.dst.addr_inc_en  = DMA_ADDR_INC_ENABLE;
        dac_dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
        dac_dma_config.dst.start_addr   = dac_fifo_addr;
        dac_dma_config.dst.end_addr     = dac_fifo_addr + 4;

        dac_dma_config.src.addr_inc_en  = DMA_ADDR_INC_ENABLE;
        dac_dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
        dac_dma_config.src.start_addr   = (uint32_t)dac_ringbuf_buffer;
        dac_dma_config.src.end_addr     = (uint32_t)dac_ringbuf_buffer + dac_ringbuf_size;

        ret = bk_dma_init(adc_ringbuf_dac_dma_id, &dac_dma_config);
        if (ret != BK_OK) {
            os_printf("dac dma init failed\r\n");
            bk_dma_free(DMA_DEV_AUDIO, adc_ringbuf_dac_dma_id);
            adc_ringbuf_dac_dma_id = DMA_ID_MAX;
            os_free(adc_ringbuf_rb);
            os_free(adc_ringbuf_buffer);
            adc_ringbuf_rb = NULL;
            adc_ringbuf_buffer = NULL;
            bk_aud_adc_deinit();
            bk_aud_dac_deinit();
            return;
        }

        uint32_t dac_transfer_len = dac_sample_rate / 1000 * 20 * 2;// two channles? LR? * 2; // 1channel, 20ms, 2bytes per sample
        bk_dma_set_transfer_len(adc_ringbuf_dac_dma_id, dac_transfer_len);  // Transfer 4 bytes at a time

        // Initialize DAC ring buffer with DMA association (READ type for DAC DMA)
        ring_buffer_init(dac_ringbuf_rb, (uint8_t*)dac_ringbuf_buffer, dac_ringbuf_size, adc_ringbuf_dac_dma_id, RB_DMA_TYPE_READ);
		uint8_t *pcm_data = (uint8_t *)os_malloc(dac_transfer_len);
		if (pcm_data == NULL) {
			os_printf("Failed to allocate pcm data\n");
			os_free(adc_ringbuf_rb);
			os_free(adc_ringbuf_buffer);
			adc_ringbuf_rb = NULL;
			adc_ringbuf_buffer = NULL;
			os_free(dac_ringbuf_rb);
			os_free(dac_ringbuf_buffer);
			dac_ringbuf_rb = NULL;
			dac_ringbuf_buffer = NULL;
			bk_dma_free(DMA_DEV_AUDIO, adc_ringbuf_dac_dma_id);
			adc_ringbuf_dac_dma_id = DMA_ID_MAX;
			bk_aud_adc_deinit();
			bk_aud_dac_deinit();
			return;
		}
		os_memset(pcm_data, 0x00, dac_transfer_len);
		uint32_t size = ring_buffer_write((dac_ringbuf_rb), (uint8_t *)pcm_data, dac_transfer_len);
		if (size != dac_transfer_len) {
			os_printf("the data write to dac_ringbuf_buffer error, size: %d \n", size);
			os_free(pcm_data);
			os_free(adc_ringbuf_rb);
			os_free(adc_ringbuf_buffer);
			adc_ringbuf_rb = NULL;
			adc_ringbuf_buffer = NULL;
			bk_aud_adc_deinit();
			bk_aud_dac_deinit();
			return;
		}
        ring_buffer_init(adc_ringbuf_rb, (uint8_t*)adc_ringbuf_buffer, adc_ringbuf_size, adc_ringbuf_adc_dma_id, RB_DMA_TYPE_WRITE);

        // Register ISR for DAC DMA
        bk_dma_register_isr(adc_ringbuf_dac_dma_id, NULL, (void *)aud_adc_ringbuf_dac_dma_finish_isr);
        bk_dma_enable_finish_interrupt(adc_ringbuf_dac_dma_id);

#if (CONFIG_SPE)
        bk_dma_set_dest_sec_attr(adc_ringbuf_dac_dma_id, DMA_ATTR_SEC);
        bk_dma_set_src_sec_attr(adc_ringbuf_dac_dma_id, DMA_ATTR_SEC);
#endif

        bk_aud_adc_set_bits_width(AUD_ADC_CHL_0, 16);
        bk_aud_dac_set_bits_width(dac_source, 16);

        // Set DAC FIFO thresholds (write threshold controls when DMA writes to FIFO)
//        bk_aud_dac_spk0_set_write_threshold(dac_source, 5);
//        bk_aud_dac_spk0_set_read_threshold(dac_source, 3);

        // Enable DAC source first
        bk_aud_dac_spk0_source_enable(dac_source, 1);

        // Start DAC before DMA (DAC must be ready to receive data from FIFO)
        ret = bk_aud_dac_start(AUD_DAC_CHL_LR);
        if (ret != BK_OK) {
            os_printf("bk_aud_dac_start fail \n");
            bk_dma_deinit(adc_ringbuf_adc_dma_id);
            bk_dma_deinit(adc_ringbuf_dac_dma_id);
            bk_dma_free(DMA_DEV_AUDIO, adc_ringbuf_adc_dma_id);
            bk_dma_free(DMA_DEV_AUDIO, adc_ringbuf_dac_dma_id);
            adc_ringbuf_adc_dma_id = DMA_ID_MAX;
            adc_ringbuf_dac_dma_id = DMA_ID_MAX;
            os_free(adc_ringbuf_rb);
            os_free(adc_ringbuf_buffer);
            adc_ringbuf_rb = NULL;
            adc_ringbuf_buffer = NULL;
            bk_aud_adc_deinit();
            bk_aud_dac_deinit();
            return;
        }
        os_printf("DAC started successfully\n");

        // Check DAC FIFO status
        bk_aud_dac_get_fifo_status(&dac_fifo_status);
        os_printf("dac_fifo_status before DMA start: 0x%x\n", dac_fifo_status);

        // Check ring buffer status before starting DMA
        uint32_t rb_fill = ring_buffer_get_fill_size(adc_ringbuf_rb);
        uint32_t rb_free = ring_buffer_get_free_size(adc_ringbuf_rb);
        os_printf("Ring buffer before DMA start: fill=%d bytes, free=%d bytes\n", rb_fill, rb_free);

        // Start DAC DMA (reads from ring buffer)
        // Note: Even if ring buffer is empty initially, DMA will wait for data
        ret = bk_dma_start(adc_ringbuf_dac_dma_id);
        if (ret != BK_OK) {
            os_printf("dac dma start fail \n");
            bk_aud_dac_stop(AUD_DAC_CHL_LR);
            bk_dma_deinit(adc_ringbuf_dac_dma_id);
            bk_dma_free(DMA_DEV_AUDIO, adc_ringbuf_dac_dma_id);
            adc_ringbuf_dac_dma_id = DMA_ID_MAX;
            os_free(adc_ringbuf_rb);
            os_free(adc_ringbuf_buffer);
            adc_ringbuf_rb = NULL;
            adc_ringbuf_buffer = NULL;
            bk_aud_adc_deinit();
            bk_aud_dac_deinit();
            return;
        }
        os_printf("DAC DMA started successfully\n");
        
        // Check DAC FIFO status after DMA start
        bk_aud_dac_get_fifo_status(&dac_fifo_status);
        os_printf("dac_fifo_status after DMA start: 0x%x\n", dac_fifo_status);

        // Start ADC DMA (DMA directly transfers data from ADC FIFO to ring buffer)
        ret = bk_dma_start(adc_ringbuf_adc_dma_id);
        if (ret != BK_OK) {
            os_printf("adc dma start fail \n");
            bk_dma_stop(adc_ringbuf_dac_dma_id);
            bk_aud_dac_stop(AUD_DAC_CHL_LR);
            bk_dma_deinit(adc_ringbuf_adc_dma_id);
            bk_dma_deinit(adc_ringbuf_dac_dma_id);
            bk_dma_free(DMA_DEV_AUDIO, adc_ringbuf_adc_dma_id);
            bk_dma_free(DMA_DEV_AUDIO, adc_ringbuf_dac_dma_id);
            adc_ringbuf_adc_dma_id = DMA_ID_MAX;
            adc_ringbuf_dac_dma_id = DMA_ID_MAX;
            os_free(adc_ringbuf_rb);
            os_free(adc_ringbuf_buffer);
            adc_ringbuf_rb = NULL;
            adc_ringbuf_buffer = NULL;
            bk_aud_adc_deinit();
            bk_aud_dac_deinit();
            return;
        }
        os_printf("ADC DMA started successfully\n");

        // Start ADC after DMA is running
        ret = bk_aud_adc_start(AUD_ADC_CHL_0);
        if (ret != BK_OK) {
            os_printf("bk_aud_adc_start fail \n");
            return;
        }
        bk_aud_adc_enable_used_channel(1<<AUD_ADC_CHL_0);

        os_printf("ADC started successfully\n");

        // Save parameters
        adc_dma_dump_dac_source = dac_source;  // Update for dump thread
        g_adc_ringbuf_initialized = 1;

        os_printf("start audio adc ringbuf test successful\r\n");
        return;
    }
    else if (os_strcmp(argv[1], "dump") == 0)
    {
        os_printf("%d---%d---%d\r\n", g_cnt1, g_cnt2, g_cnt3);
        if (argc != 3) {
            os_printf("Usage: aud_adc_dma_test dump {adc|dac|off}\n");
            os_printf("  adc: dump ADC (mic) data\n");
            os_printf("  dac: dump DAC (spk) data\n");
            os_printf("  off: stop dumping\n");
            return;
        }

        if (os_strcmp(argv[2], "adc") == 0) {
            g_adc_ringbuf_dump_mode = 1;
        } else if (os_strcmp(argv[2], "dac") == 0) {
            g_adc_ringbuf_dump_mode = 2;
        } else if (os_strcmp(argv[2], "off") == 0) {
            g_adc_ringbuf_dump_mode = 0;
        } else {
            os_printf("Invalid dump mode: %s, use adc|dac|off\n", argv[2]);
        }
        return;
    }
    else if (os_strcmp(argv[1], "stop") == 0)
    {
        if (!g_adc_ringbuf_initialized) {
            os_printf("adc ringbuf test not started\n");
            return;
        }

        // Stop dump thread first
        g_adc_ringbuf_dump_mode = 0;

        // Stop transfer thread
        if (adc_to_dac_transfer_que != NULL) {
            rtos_deinit_queue(&adc_to_dac_transfer_que);
            adc_to_dac_transfer_que = NULL;
        }
        if (adc_ringbuf_transfer_thread != NULL) {
            rtos_delete_thread(&adc_ringbuf_transfer_thread);
            adc_ringbuf_transfer_thread = NULL;
        }

        //disable adc and dac
        bk_aud_adc_deinit();
        bk_aud_dac_deinit();

        //stop dma
        if (adc_ringbuf_adc_dma_id != DMA_ID_MAX) {
            bk_dma_stop(adc_ringbuf_adc_dma_id);
            bk_dma_deinit(adc_ringbuf_adc_dma_id);
            bk_dma_free(DMA_DEV_AUDIO, adc_ringbuf_adc_dma_id);
            adc_ringbuf_adc_dma_id = DMA_ID_MAX;
        }

        if (adc_ringbuf_dac_dma_id != DMA_ID_MAX) {
            bk_dma_stop(adc_ringbuf_dac_dma_id);
            bk_dma_deinit(adc_ringbuf_dac_dma_id);
            bk_dma_free(DMA_DEV_AUDIO, adc_ringbuf_dac_dma_id);
            adc_ringbuf_dac_dma_id = DMA_ID_MAX;
        }

        // Free ring buffers
        if (dac_ringbuf_rb != NULL) {
            os_free(dac_ringbuf_rb);
            dac_ringbuf_rb = NULL;
        }
        if (dac_ringbuf_buffer != NULL) {
            os_free(dac_ringbuf_buffer);
            dac_ringbuf_buffer = NULL;
        }
        if (adc_ringbuf_rb != NULL) {
            os_free(adc_ringbuf_rb);
            adc_ringbuf_rb = NULL;
        }
        if (adc_ringbuf_buffer != NULL) {
            os_free(adc_ringbuf_buffer);
            adc_ringbuf_buffer = NULL;
        }
        if (adc_ringbuf_transfer_thread != NULL) {
            rtos_delete_thread(adc_ringbuf_transfer_thread);
            adc_ringbuf_transfer_thread = NULL;
        }
        if (adc_to_dac_transfer_que != NULL) {
            rtos_deinit_queue(&adc_to_dac_transfer_que);
            adc_to_dac_transfer_que = NULL;
        }
        g_adc_ringbuf_initialized = 0;
        os_printf("audio adc ringbuf test stop successful\n");
        return;
    } else {
        cli_aud_help();
        return;
    }
}
#endif

#if (CONFIG_I2S && CONFIG_AUDIO_ADC && CONFIG_AUDIO_DAC)

// I2S in -> DAC test (external input device to I2S RX)
static dma_id_t i2s_dac_i2s_dma_id = DMA_ID_MAX;  // DMA from I2S RX FIFO to DAC FIFO
static bool g_i2s_dac_test_initialized = false;

// ADC -> I2S out test (no DAC needed)
static dma_id_t adc_i2s_adc_dma_id = DMA_ID_MAX;  // DMA from ADC FIFO to I2S TX FIFO
static bool g_adc_i2s_test_initialized = false;

// ADC -> I2S out, I2S in -> DAC loopback test
static dma_id_t adc_i2s_dac_adc_dma_id = DMA_ID_MAX;  // DMA from ADC FIFO to I2S TX FIFO
static dma_id_t adc_i2s_dac_i2s_dma_id = DMA_ID_MAX;  // DMA from I2S RX FIFO to DAC FIFO
static bool g_adc_i2s_dac_test_initialized = false;

static aud_dac_source_t g_adc_i2s_dac_source = AUD_DAC_SOURCE_A2DP;

// ADC -> I2S out, I2S in -> DAC loopback test
// This test implements: ADC -> I2S TX (external loopback) -> I2S RX -> DAC
static void cli_aud_adc_i2s_dac_loopback_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_err_t ret = BK_OK;
    dma_config_t adc_dma_config = {0};
    dma_config_t i2s_dma_config = {0};
    aud_adc_config_t adc_config = DEFAULT_AUD_ADC_CONFIG();
    aud_dac_config_t dac_config = DEFAULT_AUD_DAC_CONFIG();
    i2s_config_t i2s_config = DEFAULT_I2S_CONFIG();
    aud_dac_source_t dac_source = AUD_DAC_SOURCE_A2DP;
    uint32_t adc_fifo_addr = 0;
    uint32_t i2s_tx_fifo_addr = 0;
    uint32_t i2s_rx_fifo_addr = 0;
    uint32_t dac_fifo_addr = 0;
    uint32_t sample_rate = 0;
    i2s_samp_rate_t i2s_samp_rate = I2S_SAMP_RATE_16000;
    uint32_t gpio_group = I2S_GPIO_GROUP_0;

    if (argc < 2) {
        cli_aud_help();
        return;
    }

    if (os_strcmp(argv[1], "start") == 0) {
        if (argc < 4) {
            os_printf("Usage: aud_adc_i2s_dac_loopback_test start {a2dp|call|hint} {sample_rate} [gpio_group]\n");
            os_printf("  Example: aud_adc_i2s_dac_loopback_test start a2dp 16000 0\n");
            return;
        }

        // Parse DAC source
        if (os_strcmp(argv[2], "a2dp") == 0) {
            dac_source = AUD_DAC_SOURCE_A2DP;
        } else if (os_strcmp(argv[2], "call") == 0) {
            dac_source = AUD_DAC_SOURCE_CALL;
        } else if (os_strcmp(argv[2], "hint") == 0) {
            dac_source = AUD_DAC_SOURCE_HINT;
        } else {
            os_printf("dac source type: %s is not support\n", argv[2]);
            return;
        }

        // Parse sample rate
        sample_rate = strtoul(argv[3], NULL, 10);
        adc_config.sample_rate = sample_rate;
        uint32_t dac_sample_rate = sample_rate;
        // Check whether dac source support sample rate
        switch (dac_source)
        {
            case AUD_DAC_SOURCE_A2DP:
                if (dac_sample_rate != 44100 && dac_sample_rate != 48000) {
                    os_printf("dac a2dp source not support sample rate: %d\n", dac_sample_rate);
                    return;
                }
                break;
            case AUD_DAC_SOURCE_CALL:
            case AUD_DAC_SOURCE_HINT:
                if (dac_sample_rate != 8000 && dac_sample_rate != 16000 && dac_sample_rate != 48000) {
                    os_printf("dac source not support sample rate: %d\n", dac_sample_rate);
                    return;
                }
                break;
            default:
                break;
        }

        // Parse GPIO group (optional)
        if (argc >= 5) {
            gpio_group = strtoul(argv[4], NULL, 10);
        }

        // Convert sample rate to I2S sample rate
        switch (sample_rate) {
            case 8000:
                i2s_samp_rate = I2S_SAMP_RATE_8000;
                break;
            case 16000:
                i2s_samp_rate = I2S_SAMP_RATE_16000;
                break;
            case 44100:
                i2s_samp_rate = I2S_SAMP_RATE_44100;
                break;
            case 48000:
                i2s_samp_rate = I2S_SAMP_RATE_48000;
                break;
            default:
                os_printf("I2S not support sample rate: %d\n", sample_rate);
                return;
        }


        // Initialize I2S driver
        ret = bk_i2s_driver_init();
        if (ret != BK_OK) {
            os_printf("bk_i2s_driver_init failed: %d\n", ret);
            return;
        }

        // Configure I2S
        i2s_config.samp_rate   = i2s_samp_rate;
        i2s_config.data_length = 16;                    // 16 bits per channel
        i2s_config.store_mode  = I2S_LRCOM_STORE_16R16L; // L -> R -> L -> R sequence
        i2s_config.work_mode   = I2S_WORK_MODE_LEFTJUST;
        
        ret = bk_i2s_init(gpio_group, &i2s_config);
        if (ret != BK_OK) {
            os_printf("bk_i2s_init failed: %d\n", ret);
            bk_i2s_driver_deinit();
            return;
        }
        os_printf("I2S initialized successfully (gpio_group=%d, sample_rate=%d)\n", gpio_group, sample_rate);

        // Initialize ADC
        aud_hardware_reset();
        g_adc_i2s_dac_source = dac_source;
        ret = bk_aud_adc_init(&adc_config);
        if (ret != BK_OK) {
            os_printf("bk_aud_adc_init failed: %d\n", ret);
            bk_i2s_deinit();
            bk_i2s_driver_deinit();
            return;
        }
        os_printf("ADC initialized successfully\n");

        // Initialize DAC
        ret = bk_aud_dac_init(&dac_config);
        if (ret != BK_OK) {
            os_printf("bk_aud_dac_init failed: %d\n", ret);
            bk_aud_adc_deinit();
            bk_i2s_deinit();
            bk_i2s_driver_deinit();
            return;
        }
        os_printf("DAC initialized successfully\n");
        bk_aud_dac_set_sample_rate(g_adc_i2s_dac_source, dac_sample_rate);

        // Initialize DMA driver
        ret = bk_dma_driver_init();
        if (ret != BK_OK) {
            os_printf("dma driver init failed\n");
            bk_aud_adc_deinit();
            bk_aud_dac_deinit();
            bk_i2s_deinit();
            bk_i2s_driver_deinit();
            return;
        }

        // ========== DMA1: ADC FIFO to I2S TX FIFO ==========
        adc_i2s_dac_adc_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
        if ((adc_i2s_dac_adc_dma_id < DMA_ID_0) || (adc_i2s_dac_adc_dma_id >= DMA_ID_MAX)) {
            os_printf("malloc adc dma fail\n");
            bk_aud_adc_deinit();
            bk_aud_dac_deinit();
            bk_i2s_deinit();
            bk_i2s_driver_deinit();
            return;
        }

        // Get ADC FIFO address
        if (bk_aud_adc_get_fifo_addr(AUD_ADC_MIC_DATA_BUS_0, &adc_fifo_addr) != BK_OK) {
            os_printf("get adc fifo address failed\n");
            bk_dma_free(DMA_DEV_AUDIO, adc_i2s_dac_adc_dma_id);
            adc_i2s_dac_adc_dma_id = DMA_ID_MAX;
            bk_aud_adc_deinit();
            bk_aud_dac_deinit();
            bk_i2s_deinit();
            bk_i2s_driver_deinit();
            return;
        }

        // Get I2S TX FIFO address
        if (bk_i2s_get_data_addr(I2S_CHANNEL_1, &i2s_tx_fifo_addr) != BK_OK) {
            os_printf("get i2s tx fifo address failed\n");
            bk_dma_free(DMA_DEV_AUDIO, adc_i2s_dac_adc_dma_id);
            adc_i2s_dac_adc_dma_id = DMA_ID_MAX;
            bk_aud_adc_deinit();
            bk_aud_dac_deinit();
            bk_i2s_deinit();
            bk_i2s_driver_deinit();
            return;
        }

        // Configure DMA1: ADC FIFO -> I2S TX FIFO
        adc_dma_config.mode      = DMA_WORK_MODE_REPEAT;
        adc_dma_config.chan_prio = 1;

        adc_dma_config.src.dev   = DMA_DEV_AUD_MIC0;
        adc_dma_config.dst.dev   = DMA_DEV_I2S0;

        adc_dma_config.src.width = DMA_DATA_WIDTH_16BITS;
        adc_dma_config.dst.width = DMA_DATA_WIDTH_16BITS;
        adc_dma_config.src.addr_inc_en  = DMA_ADDR_INC_DISABLE;  // FIFO address is fixed
        adc_dma_config.src.addr_loop_en = DMA_ADDR_LOOP_DISABLE;
        adc_dma_config.src.start_addr   = adc_fifo_addr;
        adc_dma_config.src.end_addr     = adc_fifo_addr + 4;
        adc_dma_config.dst.addr_inc_en  = DMA_ADDR_INC_DISABLE;  // FIFO address is fixed
        adc_dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_DISABLE;
        adc_dma_config.dst.start_addr   = i2s_tx_fifo_addr;
        adc_dma_config.dst.end_addr     = i2s_tx_fifo_addr + 4;

        ret = bk_dma_init(adc_i2s_dac_adc_dma_id, &adc_dma_config);
        if (ret != BK_OK) {
            os_printf("adc dma init failed\n");
            bk_dma_free(DMA_DEV_AUDIO, adc_i2s_dac_adc_dma_id);
            adc_i2s_dac_adc_dma_id = DMA_ID_MAX;
            bk_aud_adc_deinit();
            bk_aud_dac_deinit();
            bk_i2s_deinit();
            bk_i2s_driver_deinit();
            return;
        }

        uint32_t adc_transfer_len = sample_rate / 1000 * 20 * 2;  // 20ms, 2 bytes per sample
        bk_dma_set_transfer_len(adc_i2s_dac_adc_dma_id, adc_transfer_len);

#if (CONFIG_SPE)
        bk_dma_set_dest_sec_attr(adc_i2s_dac_adc_dma_id, DMA_ATTR_SEC);
        bk_dma_set_src_sec_attr(adc_i2s_dac_adc_dma_id, DMA_ATTR_SEC);
#endif

        // ========== DMA2: I2S RX FIFO to DAC FIFO ==========
        adc_i2s_dac_i2s_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
        if ((adc_i2s_dac_i2s_dma_id < DMA_ID_0) || (adc_i2s_dac_i2s_dma_id >= DMA_ID_MAX)) {
            os_printf("malloc i2s dma fail\n");
            bk_dma_deinit(adc_i2s_dac_adc_dma_id);
            bk_dma_free(DMA_DEV_AUDIO, adc_i2s_dac_adc_dma_id);
            adc_i2s_dac_adc_dma_id = DMA_ID_MAX;
            bk_aud_adc_deinit();
            bk_aud_dac_deinit();
            bk_i2s_deinit();
            bk_i2s_driver_deinit();
            return;
        }

        // Get I2S RX FIFO address (same as TX for this chip)
        i2s_rx_fifo_addr = i2s_tx_fifo_addr;  // Usually same address for RX/TX

        // Get DAC FIFO address
        dma_dev_t dac_dma_dev = DMA_DEV_AUDIO;
        if (!get_dac_fifo_addr_by_source(g_adc_i2s_dac_source, &dac_dma_dev, &dac_fifo_addr)) {
            os_printf("get dac fifo address failed\n");
            bk_dma_free(DMA_DEV_AUDIO, adc_i2s_dac_i2s_dma_id);
            adc_i2s_dac_i2s_dma_id = DMA_ID_MAX;
            bk_dma_deinit(adc_i2s_dac_adc_dma_id);
            bk_dma_free(DMA_DEV_AUDIO, adc_i2s_dac_adc_dma_id);
            adc_i2s_dac_adc_dma_id = DMA_ID_MAX;
            bk_aud_adc_deinit();
            bk_aud_dac_deinit();
            bk_i2s_deinit();
            bk_i2s_driver_deinit();
            return;
        }

        // Configure DMA2: I2S RX FIFO -> DAC FIFO
        i2s_dma_config.mode      = DMA_WORK_MODE_REPEAT;
        i2s_dma_config.chan_prio = 1;

        i2s_dma_config.src.dev   = DMA_DEV_I2S0_RX;
        i2s_dma_config.dst.dev   = dac_dma_dev;

        i2s_dma_config.src.width = DMA_DATA_WIDTH_16BITS;
        i2s_dma_config.dst.width = DMA_DATA_WIDTH_16BITS;
        i2s_dma_config.src.addr_inc_en  = DMA_ADDR_INC_DISABLE;  // FIFO address is fixed
        i2s_dma_config.src.addr_loop_en = DMA_ADDR_LOOP_DISABLE;
        i2s_dma_config.src.start_addr   = i2s_rx_fifo_addr;
        i2s_dma_config.src.end_addr     = i2s_rx_fifo_addr + 4;
        i2s_dma_config.dst.addr_inc_en  = DMA_ADDR_INC_DISABLE;  // FIFO address is fixed
        i2s_dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_DISABLE;
        i2s_dma_config.dst.start_addr   = dac_fifo_addr;
        i2s_dma_config.dst.end_addr     = dac_fifo_addr + 4;

        ret = bk_dma_init(adc_i2s_dac_i2s_dma_id, &i2s_dma_config);
        if (ret != BK_OK) {
            os_printf("i2s dma init failed\n");
            bk_dma_free(DMA_DEV_AUDIO, adc_i2s_dac_i2s_dma_id);
            adc_i2s_dac_i2s_dma_id = DMA_ID_MAX;
            bk_dma_deinit(adc_i2s_dac_adc_dma_id);
            bk_dma_free(DMA_DEV_AUDIO, adc_i2s_dac_adc_dma_id);
            adc_i2s_dac_adc_dma_id = DMA_ID_MAX;
            bk_aud_adc_deinit();
            bk_aud_dac_deinit();
            bk_i2s_deinit();
            bk_i2s_driver_deinit();
            return;
        }

        uint32_t i2s_transfer_len = sample_rate / 1000 * 20 * 2;  // 20ms, 2 bytes per sample
        bk_dma_set_transfer_len(adc_i2s_dac_i2s_dma_id, i2s_transfer_len);

#if (CONFIG_SPE)
        bk_dma_set_dest_sec_attr(adc_i2s_dac_i2s_dma_id, DMA_ATTR_SEC);
        bk_dma_set_src_sec_attr(adc_i2s_dac_i2s_dma_id, DMA_ATTR_SEC);
#endif

        // Set bit width
        bk_aud_adc_set_bits_width(AUD_ADC_CHL_0, 16);
        bk_aud_dac_set_bits_width(g_adc_i2s_dac_source, 16);

        // Enable DAC source
        bk_aud_dac_spk0_source_enable(g_adc_i2s_dac_source, 1);

        // Enable I2S
        ret = bk_i2s_enable(I2S_ENABLE);
        if (ret != BK_OK) {
            os_printf("bk_i2s_enable failed: %d\n", ret);
            bk_dma_deinit(adc_i2s_dac_i2s_dma_id);
            bk_dma_deinit(adc_i2s_dac_adc_dma_id);
            bk_dma_free(DMA_DEV_AUDIO, adc_i2s_dac_i2s_dma_id);
            bk_dma_free(DMA_DEV_AUDIO, adc_i2s_dac_adc_dma_id);
            adc_i2s_dac_i2s_dma_id = DMA_ID_MAX;
            adc_i2s_dac_adc_dma_id = DMA_ID_MAX;
            bk_aud_adc_deinit();
            bk_aud_dac_deinit();
            bk_i2s_deinit();
            bk_i2s_driver_deinit();
            return;
        }

        // Start I2S
        ret = bk_i2s_start();
        if (ret != BK_OK) {
            os_printf("bk_i2s_start failed: %d\n", ret);
            bk_i2s_enable(I2S_DISABLE);
            bk_dma_deinit(adc_i2s_dac_i2s_dma_id);
            bk_dma_deinit(adc_i2s_dac_adc_dma_id);
            bk_dma_free(DMA_DEV_AUDIO, adc_i2s_dac_i2s_dma_id);
            bk_dma_free(DMA_DEV_AUDIO, adc_i2s_dac_adc_dma_id);
            adc_i2s_dac_i2s_dma_id = DMA_ID_MAX;
            adc_i2s_dac_adc_dma_id = DMA_ID_MAX;
            bk_aud_adc_deinit();
            bk_aud_dac_deinit();
            bk_i2s_deinit();
            bk_i2s_driver_deinit();
            return;
        }

        // Start DAC
        ret = bk_aud_dac_start(AUD_DAC_CHL_LR);
        if (ret != BK_OK) {
            os_printf("bk_aud_dac_start failed: %d\n", ret);
            bk_i2s_stop();
            bk_i2s_enable(I2S_DISABLE);
            bk_dma_deinit(adc_i2s_dac_i2s_dma_id);
            bk_dma_deinit(adc_i2s_dac_adc_dma_id);
            bk_dma_free(DMA_DEV_AUDIO, adc_i2s_dac_i2s_dma_id);
            bk_dma_free(DMA_DEV_AUDIO, adc_i2s_dac_adc_dma_id);
            adc_i2s_dac_i2s_dma_id = DMA_ID_MAX;
            adc_i2s_dac_adc_dma_id = DMA_ID_MAX;
            bk_aud_adc_deinit();
            bk_aud_dac_deinit();
            bk_i2s_deinit();
            bk_i2s_driver_deinit();
            return;
        }
        os_printf("DAC started successfully\n");

        // Start I2S RX DMA (I2S RX -> DAC)
        ret = bk_dma_start(adc_i2s_dac_i2s_dma_id);
        if (ret != BK_OK) {
            os_printf("i2s dma start failed\n");
            bk_aud_dac_stop(AUD_DAC_CHL_LR);
            bk_i2s_stop();
            bk_i2s_enable(I2S_DISABLE);
            bk_dma_deinit(adc_i2s_dac_i2s_dma_id);
            bk_dma_deinit(adc_i2s_dac_adc_dma_id);
            bk_dma_free(DMA_DEV_AUDIO, adc_i2s_dac_i2s_dma_id);
            bk_dma_free(DMA_DEV_AUDIO, adc_i2s_dac_adc_dma_id);
            adc_i2s_dac_i2s_dma_id = DMA_ID_MAX;
            adc_i2s_dac_adc_dma_id = DMA_ID_MAX;
            bk_aud_adc_deinit();
            bk_aud_dac_deinit();
            bk_i2s_deinit();
            bk_i2s_driver_deinit();
            return;
        }
        os_printf("I2S RX DMA started successfully\n");

        // Start ADC DMA (ADC -> I2S TX)
        ret = bk_dma_start(adc_i2s_dac_adc_dma_id);
        if (ret != BK_OK) {
            os_printf("adc dma start failed\n");
            bk_dma_stop(adc_i2s_dac_i2s_dma_id);
            bk_aud_dac_stop(AUD_DAC_CHL_LR);
            bk_i2s_stop();
            bk_i2s_enable(I2S_DISABLE);
            bk_dma_deinit(adc_i2s_dac_i2s_dma_id);
            bk_dma_deinit(adc_i2s_dac_adc_dma_id);
            bk_dma_free(DMA_DEV_AUDIO, adc_i2s_dac_i2s_dma_id);
            bk_dma_free(DMA_DEV_AUDIO, adc_i2s_dac_adc_dma_id);
            adc_i2s_dac_i2s_dma_id = DMA_ID_MAX;
            adc_i2s_dac_adc_dma_id = DMA_ID_MAX;
            bk_aud_adc_deinit();
            bk_aud_dac_deinit();
            bk_i2s_deinit();
            bk_i2s_driver_deinit();
            return;
        }
        os_printf("ADC DMA started successfully\n");

        // Start ADC
        ret = bk_aud_adc_start(AUD_ADC_CHL_0);
        if (ret != BK_OK) {
            os_printf("bk_aud_adc_start failed: %d\n", ret);
            return;
        }
        bk_aud_adc_enable_used_channel(1<<AUD_ADC_CHL_0);

        g_adc_i2s_dac_test_initialized = true;
        os_printf("ADC -> I2S out, I2S in -> DAC loopback test started successfully\n");
        os_printf("Note: Please connect I2S TX to I2S RX externally for loopback test\n");
        return;
    }
    else if (os_strcmp(argv[1], "stop") == 0)
    {

        if (!g_adc_i2s_dac_test_initialized) {
            os_printf("adc i2s dac loopback test not started\n");
            return;
        }

        os_printf("ADC -> I2S out, I2S in -> DAC loopback test stop\n");
        bk_aud_adc_deinit();
        bk_aud_dac_spk0_source_enable(g_adc_i2s_dac_source, 0);
        bk_aud_dac_deinit();
        // Stop I2S
        bk_i2s_stop();
        bk_i2s_enable(I2S_DISABLE);
        
        // Stop DMA
        bk_dma_driver_deinit();
        if (adc_i2s_dac_adc_dma_id != DMA_ID_MAX) {
            bk_dma_stop(adc_i2s_dac_adc_dma_id);
            bk_dma_deinit(adc_i2s_dac_adc_dma_id);
            bk_dma_free(DMA_DEV_AUDIO, adc_i2s_dac_adc_dma_id);
            adc_i2s_dac_adc_dma_id = DMA_ID_MAX;
        }
        if (adc_i2s_dac_i2s_dma_id != DMA_ID_MAX) {
            bk_dma_stop(adc_i2s_dac_i2s_dma_id);
            bk_dma_deinit(adc_i2s_dac_i2s_dma_id);
            bk_dma_free(DMA_DEV_AUDIO, adc_i2s_dac_i2s_dma_id);
            adc_i2s_dac_i2s_dma_id = DMA_ID_MAX;
        }

        // Deinitialize
        bk_i2s_deinit();
        bk_i2s_driver_deinit();

        g_adc_i2s_dac_test_initialized = false;
        os_printf("ADC -> I2S out, I2S in -> DAC loopback test stop successful\n");
        return;
    } else {
        cli_aud_help();
        return;
    }
}

static void aud_i2s_dac_test_isr(dma_id_t dma_id)
{
	GPIO_DOWN(36);GPIO_UP(36);
	GPIO_DOWN(36);
}


// I2S in -> DAC test (external input device to I2S RX, no ADC needed)
static void cli_aud_i2s_dac_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_err_t ret = BK_OK;
    dma_config_t i2s_dma_config = {0};
    aud_dac_config_t dac_config = DEFAULT_AUD_DAC_CONFIG();
    i2s_config_t i2s_config = DEFAULT_I2S_CONFIG();
    aud_dac_source_t dac_source = AUD_DAC_SOURCE_A2DP;
    uint32_t i2s_rx_fifo_addr = 0;
    uint32_t dac_fifo_addr = 0;
    uint32_t sample_rate = 0;
    i2s_samp_rate_t i2s_samp_rate = I2S_SAMP_RATE_16000;
    uint32_t gpio_group = I2S_GPIO_GROUP_0;

    if (argc < 2) {
        cli_aud_help();
        return;
    }

    if (os_strcmp(argv[1], "start") == 0) {
		if (g_i2s_dac_test_initialized) {
			LOGW("i2s dac test already started\n");
			return;
		}
        if (argc < 4) {
            os_printf("Usage: aud_i2s_dac_test start {a2dp|call|hint} {sample_rate} [gpio_group]\n");
            os_printf("  Example: aud_i2s_dac_test start a2dp 16000 0\n");
            os_printf("  Note: External device should input audio to I2S RX pins\n");
            return;
        }

        // Parse DAC source
        if (os_strcmp(argv[2], "a2dp") == 0) {
            dac_source = AUD_DAC_SOURCE_A2DP;
        } else if (os_strcmp(argv[2], "call") == 0) {
            dac_source = AUD_DAC_SOURCE_CALL;
        } else if (os_strcmp(argv[2], "hint") == 0) {
            dac_source = AUD_DAC_SOURCE_HINT;
        } else {
            os_printf("dac source type: %s is not support\n", argv[2]);
            return;
        }

        // Parse sample rate
        sample_rate = strtoul(argv[3], NULL, 10);
        uint32_t dac_sample_rate = sample_rate;

        // Check whether dac source support sample rate
        switch (dac_source)
        {
            case AUD_DAC_SOURCE_A2DP:
//                if (dac_sample_rate != 44100 && dac_sample_rate != 48000 && dac_sample_rate != 16000)
//				{
//                    os_printf("dac a2dp source not support sample rate: %d\n", dac_sample_rate);
//                    return;
//                }
                break;
            case AUD_DAC_SOURCE_CALL:
            case AUD_DAC_SOURCE_HINT:
//                if (dac_sample_rate != 8000 && dac_sample_rate != 16000 && dac_sample_rate != 48000)
//				{
//                    os_printf("dac source not support sample rate: %d\n", dac_sample_rate);
//                    return;
//                }
                break;
            default:
                break;
        }

		i2s_role_t i2s_role = I2S_ROLE_MASTER;
		if (argc >= 5)
		{
			uint32_t role = strtoul(argv[4], NULL, 10);
			switch(role) {
				case 0:
					i2s_role = I2S_ROLE_SLAVE;
					break;
				case 1:
					i2s_role = I2S_ROLE_MASTER;
					break;
				default:
					i2s_role = I2S_ROLE_MASTER;
					break;
			}
		}
		i2s_config.role = i2s_role;

        // Parse GPIO group (optional)
        if (argc >= 6) {
            gpio_group = strtoul(argv[5], NULL, 10);
        }

		uint32_t dac_bits = 16;
		uint32_t i2s_data_len = 16;
		i2s_lrcom_store_mode_t i2s_store_mode = I2S_LRCOM_STORE_16R16L;

		if (argc >= 7) {
			dac_bits = strtoul(argv[6], NULL, 10);
		}
		if (argc >= 8) {
			i2s_store_mode = strtoul(argv[7], NULL, 10);
		}
		if (argc >= 9) {
			i2s_data_len = strtoul(argv[8], NULL, 10);
		}
		if (dac_bits != 16)
		{
			if (i2s_store_mode == I2S_LRCOM_STORE_16R16L)
			{
				i2s_data_len = 32;
				i2s_store_mode = I2S_LRCOM_STORE_LRLR;
				LOGW("Force i2s_data_len to 32 and i2s_store_mode to I2S_LRCOM_STORE_LRLR\r\n");
			}
		}
	//	i2s_data_len = dac_bits;
	//	i2s_config.lsb_first_en = 1;

        // Convert sample rate to I2S sample rate
        switch (sample_rate) {
            case 8000:
                i2s_samp_rate = I2S_SAMP_RATE_8000;
                break;
            case 16000:
                i2s_samp_rate = I2S_SAMP_RATE_16000;
                break;
			case 24000:
				i2s_samp_rate = I2S_SAMP_RATE_24000;
				break;
			case 32000:
				i2s_samp_rate = I2S_SAMP_RATE_32000;
				break;
            case 44100:
                i2s_samp_rate = I2S_SAMP_RATE_44100;
                break;
            case 48000:
                i2s_samp_rate = I2S_SAMP_RATE_48000;
                break;
            case 96000:
                i2s_samp_rate = I2S_SAMP_RATE_96000;
                break;
            default:
                os_printf("I2S not support sample rate: %d\n", sample_rate);
                return;
        }

		// Configure I2S
		i2s_config.samp_rate   = i2s_samp_rate;
		i2s_config.data_length = i2s_data_len;
		i2s_config.store_mode  = i2s_store_mode;
	//	i2s_config.work_mode   = I2S_WORK_MODE_LEFTJUST;

		if (g_i2s_dac_test_initialized) {
			os_printf("g_i2s_dac_test_initialized6 : %d\r\n", g_i2s_dac_test_initialized);
		//	return;
		}

		/* Initialize I2S driver */
		ret = bk_i2s_driver_init();
		if (ret != BK_OK) {
			os_printf("bk_i2s_driver_init failed: %d\n", ret);
			return;
		}

		ret = bk_i2s_init(gpio_group, &i2s_config);
		if (ret != BK_OK) {
			os_printf("bk_i2s_init failed: %d\n", ret);
			bk_i2s_driver_deinit();
			return;
		}
		bk_i2s_set_samp_rate(i2s_samp_rate);
		os_printf("I2S initialized successfully (gpio_group=%d, sample_rate=%d)\n", gpio_group, sample_rate);

		aud_hardware_reset();

		// Initialize DAC
		g_adc_i2s_dac_source = dac_source;
		dac_config.bits = dac_bits;
		ret = bk_aud_dac_init(&dac_config);
		if (ret != BK_OK) {
			os_printf("bk_aud_dac_init failed: %d\n", ret);
			bk_i2s_deinit();
			bk_i2s_driver_deinit();
			return;
		}
		os_printf("DAC initialized successfully\n");
		bk_aud_dac_set_sample_rate(dac_source, dac_sample_rate);

		// Initialize DMA driver
		ret = bk_dma_driver_init();
		if (ret != BK_OK) {
			os_printf("dma driver init failed\n");
			bk_aud_dac_deinit();
			bk_i2s_deinit();
			bk_i2s_driver_deinit();
			return;
		}

		// ========== DMA: I2S RX FIFO to DAC FIFO ==========
		i2s_dac_i2s_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
		if ((i2s_dac_i2s_dma_id < DMA_ID_0) || (i2s_dac_i2s_dma_id >= DMA_ID_MAX)) {
			os_printf("malloc i2s dma fail\n");
			bk_aud_dac_deinit();
			bk_i2s_deinit();
			bk_i2s_driver_deinit();
			return;
		}

		// Get I2S RX FIFO address
		if (bk_i2s_get_data_addr(I2S_CHANNEL_1, &i2s_rx_fifo_addr) != BK_OK) {
			os_printf("get i2s rx fifo address failed\n");
			bk_dma_free(DMA_DEV_AUDIO, i2s_dac_i2s_dma_id);
			i2s_dac_i2s_dma_id = DMA_ID_MAX;
			bk_aud_dac_deinit();
			bk_i2s_deinit();
			bk_i2s_driver_deinit();
			return;
		}

		// Get DAC FIFO address
		dma_dev_t dac_dma_dev = DMA_DEV_AUDIO;
		if (!get_dac_fifo_addr_by_source(dac_source, &dac_dma_dev, &dac_fifo_addr)) {
			os_printf("get dac fifo address failed\n");
			bk_dma_free(DMA_DEV_AUDIO, i2s_dac_i2s_dma_id);
			i2s_dac_i2s_dma_id = DMA_ID_MAX;
			bk_aud_dac_deinit();
			bk_i2s_deinit();
			bk_i2s_driver_deinit();
			return;
		}

		// Configure DMA: I2S RX FIFO -> DAC FIFO
		i2s_dma_config.mode      = DMA_WORK_MODE_REPEAT;
		i2s_dma_config.chan_prio = 1;

		switch(gpio_group)
		{
			case 0:
				i2s_dma_config.src.dev   = DMA_DEV_I2S0_RX;
				break;
			case 1:
				i2s_dma_config.src.dev   = DMA_DEV_I2S1_RX;
				break;
			case 2:
				i2s_dma_config.src.dev	 = DMA_DEV_I2S2_RX;
				break;
			default:
				os_printf("Do not match this group!\r\n");
				break;
		}

	//	i2s_dma_config.src.dev   = DMA_DEV_I2S0_RX;
		i2s_dma_config.dst.dev   = dac_dma_dev;
		if (dac_bits == 16)
		{
			i2s_dma_config.src.width = DMA_DATA_WIDTH_16BITS;
			i2s_dma_config.dst.width = DMA_DATA_WIDTH_16BITS;
		} else if (dac_bits == 24)
		{
			i2s_dma_config.src.width = DMA_DATA_WIDTH_32BITS;
			i2s_dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
		}
		i2s_dma_config.src.addr_inc_en  = DMA_ADDR_INC_DISABLE;  // FIFO address is fixed
		i2s_dma_config.src.addr_loop_en = DMA_ADDR_LOOP_DISABLE;
		i2s_dma_config.src.start_addr   = i2s_rx_fifo_addr;
		i2s_dma_config.src.end_addr     = i2s_rx_fifo_addr + 4;
		i2s_dma_config.dst.addr_inc_en  = DMA_ADDR_INC_DISABLE;  // FIFO address is fixed
		i2s_dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_DISABLE;
		i2s_dma_config.dst.start_addr   = dac_fifo_addr;
		i2s_dma_config.dst.end_addr     = dac_fifo_addr + 4;

		ret = bk_dma_init(i2s_dac_i2s_dma_id, &i2s_dma_config);
		if (ret != BK_OK) {
			os_printf("i2s dma init failed\n");
			bk_dma_free(DMA_DEV_AUDIO, i2s_dac_i2s_dma_id);
			i2s_dac_i2s_dma_id = DMA_ID_MAX;
			bk_aud_dac_deinit();
			bk_i2s_deinit();
			bk_i2s_driver_deinit();
			return;
		}
		uint32_t i2s_transfer_len = 0;
		if (dac_bits == 16)
			i2s_transfer_len = sample_rate / 1000 * 20 * 2;  // 20ms, 2 bytes per sample
		else
			i2s_transfer_len = sample_rate / 1000 * 20 * 4;  // 20ms, 2 bytes per sample

		bk_dma_set_transfer_len(i2s_dac_i2s_dma_id, i2s_transfer_len);

		bk_dma_register_isr(i2s_dac_i2s_dma_id, NULL, (void *)aud_i2s_dac_test_isr);
		bk_dma_enable_finish_interrupt(i2s_dac_i2s_dma_id);

#if (CONFIG_SPE)
		bk_dma_set_dest_sec_attr(i2s_dac_i2s_dma_id, DMA_ATTR_SEC);
		bk_dma_set_src_sec_attr(i2s_dac_i2s_dma_id, DMA_ATTR_SEC);
#endif

		// Enable DAC source
		bk_aud_dac_spk0_source_enable(dac_source, 1);

		// Enable I2S
		ret = bk_i2s_enable(I2S_ENABLE);
		if (ret != BK_OK) {
			os_printf("bk_i2s_enable failed: %d\n", ret);
			bk_dma_deinit(i2s_dac_i2s_dma_id);
			bk_dma_free(DMA_DEV_AUDIO, i2s_dac_i2s_dma_id);
			i2s_dac_i2s_dma_id = DMA_ID_MAX;
			bk_aud_dac_deinit();
			bk_i2s_deinit();
			bk_i2s_driver_deinit();
			return;
		}

		// Start I2S
		ret = bk_i2s_start();
		if (ret != BK_OK) {
			os_printf("bk_i2s_start failed: %d\n", ret);
			bk_i2s_enable(I2S_DISABLE);
			bk_dma_deinit(i2s_dac_i2s_dma_id);
			bk_dma_free(DMA_DEV_AUDIO, i2s_dac_i2s_dma_id);
			i2s_dac_i2s_dma_id = DMA_ID_MAX;
			bk_aud_dac_deinit();
			bk_i2s_deinit();
			bk_i2s_driver_deinit();
			return;
		}

		// Start DAC
		ret = bk_aud_dac_start(AUD_DAC_CHL_LR);
		if (ret != BK_OK) {
			os_printf("bk_aud_dac_start failed: %d\n", ret);
			bk_i2s_stop();
			bk_i2s_enable(I2S_DISABLE);
			bk_dma_deinit(i2s_dac_i2s_dma_id);
			bk_dma_free(DMA_DEV_AUDIO, i2s_dac_i2s_dma_id);
			i2s_dac_i2s_dma_id = DMA_ID_MAX;
			bk_aud_dac_deinit();
			bk_i2s_deinit();
			bk_i2s_driver_deinit();
			return;
		}
		os_printf("DAC started successfully\n");

		// Start I2S RX DMA (I2S RX -> DAC)
		ret = bk_dma_start(i2s_dac_i2s_dma_id);
		if (ret != BK_OK) {
			os_printf("i2s dma start failed\n");
			bk_aud_dac_stop(AUD_DAC_CHL_LR);
			bk_i2s_stop();
			bk_i2s_enable(I2S_DISABLE);
			bk_dma_deinit(i2s_dac_i2s_dma_id);
			bk_dma_free(DMA_DEV_AUDIO, i2s_dac_i2s_dma_id);
			i2s_dac_i2s_dma_id = DMA_ID_MAX;
			bk_aud_dac_deinit();
			bk_i2s_deinit();
			bk_i2s_driver_deinit();
			return;
		}
		os_printf("I2S RX DMA started successfully\n");

		g_i2s_dac_test_initialized = true;
		os_printf("I2S in -> DAC test started successfully\n");
		os_printf("Note: External device should input audio to I2S RX pins\n");
		return;
	}
	else if (os_strcmp(argv[1], "stop") == 0)
	{
		if (!g_i2s_dac_test_initialized) {
			os_printf("i2s dac test not started\n");
			return;
		}

		// Stop I2S
		bk_i2s_stop();
		bk_i2s_enable(I2S_DISABLE);

		bk_dma_driver_deinit();
		// Stop DMA
		if (i2s_dac_i2s_dma_id != DMA_ID_MAX) {
			bk_dma_stop(i2s_dac_i2s_dma_id);
			bk_dma_deinit(i2s_dac_i2s_dma_id);
			bk_dma_free(DMA_DEV_AUDIO, i2s_dac_i2s_dma_id);
			i2s_dac_i2s_dma_id = DMA_ID_MAX;
		}

		bk_aud_dac_spk0_source_enable(g_adc_i2s_dac_source, 0);
		bk_aud_dac_deinit();

		// Deinitialize
		bk_i2s_deinit();
		bk_i2s_driver_deinit();
		g_i2s_dac_test_initialized = false;

		//g_i2s_dac_test_initialized = false;
		os_printf("I2S in -> DAC test stop successful\n");
		return;
	} else {
		cli_aud_help();
		return;
	}
}

static void aud_adc_i2s_test_isr(dma_id_t dma_id)
{
	GPIO_DOWN(37);GPIO_UP(37);
	GPIO_DOWN(37);
}
// ADC -> I2S out test (no DAC needed)
static void cli_aud_adc_i2s_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_err_t ret = BK_OK;
    dma_config_t adc_dma_config = {0};
    aud_adc_config_t adc_config = DEFAULT_AUD_ADC_CONFIG();
    i2s_config_t i2s_config = DEFAULT_I2S_CONFIG();
    uint32_t adc_fifo_addr = 0;
    uint32_t i2s_tx_fifo_addr = 0;
    uint32_t sample_rate = 0;
    i2s_samp_rate_t i2s_samp_rate = I2S_SAMP_RATE_16000;
    uint32_t gpio_group = I2S_GPIO_GROUP_0;

    if (argc < 2) {
        cli_aud_help();
        return;
    }

    if (os_strcmp(argv[1], "start") == 0) {
        if (g_adc_i2s_test_initialized) {
            LOGW("adc i2s test already started\n");
            return;
        }
        if (argc < 3) {
            os_printf("Usage: aud_adc_i2s_test start {sample_rate} [gpio_group]\n");
            os_printf("  Example: aud_adc_i2s_test start 16000 0\n");
            os_printf("  Note: Audio from ADC will be output to I2S TX pins\n");
            return;
        }

        // Parse sample rate
        sample_rate = strtoul(argv[2], NULL, 10);
        adc_config.sample_rate = sample_rate;

		i2s_role_t i2s_role = I2S_ROLE_MASTER;
		if (argc >= 4)
		{
			uint32_t role = strtoul(argv[3], NULL, 10);
			switch(role) {
				case 0:
					i2s_role = I2S_ROLE_SLAVE;
					break;
				case 1:
					i2s_role = I2S_ROLE_MASTER;
					break;
				default:
					i2s_role = I2S_ROLE_MASTER;
					break;
			}
		}
		i2s_config.role = i2s_role;

        // Parse GPIO group (optional)
        if (argc >= 5) {
            gpio_group = strtoul(argv[4], NULL, 10);
        }

		uint32_t adc_bits = 16;
		uint32_t i2s_data_len = 16;
		i2s_lrcom_store_mode_t i2s_store_mode = I2S_LRCOM_STORE_16R16L;

		if (argc >= 6) {
			adc_bits = strtoul(argv[5], NULL, 10);
		}
		if (argc >= 7) {
			i2s_store_mode = strtoul(argv[6], NULL, 10);
		}
		if (argc >= 8) {
			i2s_data_len = strtoul(argv[7], NULL, 10);
		}

		if (adc_bits != 16)
		{
			if (i2s_store_mode == I2S_LRCOM_STORE_16R16L)
			{
				i2s_data_len = 32;
				i2s_store_mode = I2S_LRCOM_STORE_LRLR;
				LOGW("Force i2s_data_len to 32 and i2s_store_mode to I2S_LRCOM_STORE_LRLR\r\n");
			}
		}
	//	i2s_data_len = adc_bits;

	//	if (i2s_store_mode == I2S_LRCOM_STORE_LRLR) {
	//		i2s_config.work_mode   = I2S_WORK_MODE_LEFTJUST;
	//	}

        // Convert sample rate to I2S sample rate
        switch (sample_rate) {
            case 8000:
                i2s_samp_rate = I2S_SAMP_RATE_8000;
                break;
            case 16000:
                i2s_samp_rate = I2S_SAMP_RATE_16000;
                break;
            case 24000:
                i2s_samp_rate = I2S_SAMP_RATE_24000;
                break;
            case 32000:
                i2s_samp_rate = I2S_SAMP_RATE_32000;
                break;
            case 44100:
                i2s_samp_rate = I2S_SAMP_RATE_44100;
                break;
            case 48000:
                i2s_samp_rate = I2S_SAMP_RATE_48000;
                break;
            case 96000:
                i2s_samp_rate = I2S_SAMP_RATE_96000;
                break;
            default:
                os_printf("I2S not support sample rate: %d\n", sample_rate);
                return;
        }

        // Initialize I2S driver
        ret = bk_i2s_driver_init();
        if (ret != BK_OK) {
            os_printf("bk_i2s_driver_init failed: %d\n", ret);
            return;
        }

        // Configure I2S
        i2s_config.samp_rate   = i2s_samp_rate;
        i2s_config.data_length = i2s_data_len;
        i2s_config.store_mode  = i2s_store_mode;

        ret = bk_i2s_init(gpio_group, &i2s_config);
        if (ret != BK_OK) {
            os_printf("bk_i2s_init failed: %d\n", ret);
            bk_i2s_driver_deinit();
            return;
        }
        os_printf("I2S initialized successfully (gpio_group=%d, sample_rate=%d)\n", gpio_group, sample_rate);

        // Initialize ADC
        aud_hardware_reset();
		for (uint32_t i = 0; i < AUD_MIC_CHL_NUM_MAX; i++) {
			adc_config.chl_cfg[i].bits = adc_bits;
		}
        ret = bk_aud_adc_init(&adc_config);
        if (ret != BK_OK) {
            os_printf("bk_aud_adc_init failed: %d\n", ret);
            bk_i2s_deinit();
            bk_i2s_driver_deinit();
            return;
        }
        os_printf("ADC initialized successfully\n");

        // Initialize DMA driver
        ret = bk_dma_driver_init();
        if (ret != BK_OK) {
            os_printf("dma driver init failed\n");
            bk_aud_adc_deinit();
            bk_i2s_deinit();
            bk_i2s_driver_deinit();
            return;
        }

        // ========== DMA: ADC FIFO to I2S TX FIFO ==========
        adc_i2s_adc_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
        if ((adc_i2s_adc_dma_id < DMA_ID_0) || (adc_i2s_adc_dma_id >= DMA_ID_MAX)) {
            os_printf("malloc adc dma fail\n");
            bk_aud_adc_deinit();
            bk_i2s_deinit();
            bk_i2s_driver_deinit();
            return;
        }

        // Get ADC FIFO address
        if (bk_aud_adc_get_fifo_addr(AUD_ADC_MIC_DATA_BUS_0, &adc_fifo_addr) != BK_OK) {
            os_printf("get adc fifo address failed\n");
            bk_dma_free(DMA_DEV_AUDIO, adc_i2s_adc_dma_id);
            adc_i2s_adc_dma_id = DMA_ID_MAX;
            bk_aud_adc_deinit();
            bk_i2s_deinit();
            bk_i2s_driver_deinit();
            return;
        }

        // Get I2S TX FIFO address
        if (bk_i2s_get_data_addr(I2S_CHANNEL_1, &i2s_tx_fifo_addr) != BK_OK) {
            os_printf("get i2s tx fifo address failed\n");
            bk_dma_free(DMA_DEV_AUDIO, adc_i2s_adc_dma_id);
            adc_i2s_adc_dma_id = DMA_ID_MAX;
            bk_aud_adc_deinit();
            bk_i2s_deinit();
            bk_i2s_driver_deinit();
            return;
        }

		switch(gpio_group)
		{
			case 0:
				adc_dma_config.dst.dev   = DMA_DEV_I2S0;
				break;
			case 1:
				adc_dma_config.dst.dev   = DMA_DEV_I2S1;
				break;
			case 2:
				adc_dma_config.dst.dev	 = DMA_DEV_I2S2;
				break;
			default:
				os_printf("Do not match this group!\r\n");
				break;
		}

        // Configure DMA: ADC FIFO -> I2S TX FIFO
        adc_dma_config.mode      = DMA_WORK_MODE_REPEAT;
        adc_dma_config.chan_prio = 1;
        adc_dma_config.src.dev   = DMA_DEV_AUD_MIC0;
        //adc_dma_config.dst.dev   = DMA_DEV_I2S0;
		if (adc_bits == 16) {
	        adc_dma_config.src.width = DMA_DATA_WIDTH_16BITS;
	        adc_dma_config.dst.width = DMA_DATA_WIDTH_16BITS;
		} else if (adc_bits == 24)
		{
			adc_dma_config.src.width = DMA_DATA_WIDTH_32BITS;
			adc_dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
		}
        adc_dma_config.src.addr_inc_en  = DMA_ADDR_INC_DISABLE;  // FIFO address is fixed
        adc_dma_config.src.addr_loop_en = DMA_ADDR_LOOP_DISABLE;
        adc_dma_config.src.start_addr   = adc_fifo_addr;
        adc_dma_config.src.end_addr     = adc_fifo_addr + 4;
        adc_dma_config.dst.addr_inc_en  = DMA_ADDR_INC_DISABLE;  // FIFO address is fixed
        adc_dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_DISABLE;
        adc_dma_config.dst.start_addr   = i2s_tx_fifo_addr;
        adc_dma_config.dst.end_addr     = i2s_tx_fifo_addr + 4;

        ret = bk_dma_init(adc_i2s_adc_dma_id, &adc_dma_config);
        if (ret != BK_OK) {
            os_printf("adc dma init failed\n");
            bk_dma_free(DMA_DEV_AUDIO, adc_i2s_adc_dma_id);
            adc_i2s_adc_dma_id = DMA_ID_MAX;
            bk_aud_adc_deinit();
            bk_i2s_deinit();
            bk_i2s_driver_deinit();
            return;
        }

		uint32_t adc_transfer_len = 0;
		if (adc_bits == 16)
			adc_transfer_len = sample_rate / 1000 * 20 * 2;
		else
			adc_transfer_len = sample_rate / 1000 * 20 * 4;

        bk_dma_set_transfer_len(adc_i2s_adc_dma_id, adc_transfer_len);

        bk_dma_register_isr(adc_i2s_adc_dma_id, NULL, (void *)aud_adc_i2s_test_isr);
        bk_dma_enable_finish_interrupt(adc_i2s_adc_dma_id);

#if (CONFIG_SPE)
        bk_dma_set_dest_sec_attr(adc_i2s_adc_dma_id, DMA_ATTR_SEC);
        bk_dma_set_src_sec_attr(adc_i2s_adc_dma_id, DMA_ATTR_SEC);
#endif

        // Enable I2S
        ret = bk_i2s_enable(I2S_ENABLE);
        if (ret != BK_OK) {
            os_printf("bk_i2s_enable failed: %d\n", ret);
            bk_dma_deinit(adc_i2s_adc_dma_id);
            bk_dma_free(DMA_DEV_AUDIO, adc_i2s_adc_dma_id);
            adc_i2s_adc_dma_id = DMA_ID_MAX;
            bk_aud_adc_deinit();
            bk_i2s_deinit();
            bk_i2s_driver_deinit();
            return;
        }

        // Start I2S
        ret = bk_i2s_start();
        if (ret != BK_OK) {
            os_printf("bk_i2s_start failed: %d\n", ret);
            bk_i2s_enable(I2S_DISABLE);
            bk_dma_deinit(adc_i2s_adc_dma_id);
            bk_dma_free(DMA_DEV_AUDIO, adc_i2s_adc_dma_id);
            adc_i2s_adc_dma_id = DMA_ID_MAX;
            bk_aud_adc_deinit();
            bk_i2s_deinit();
            bk_i2s_driver_deinit();
            return;
        }

        // Start ADC DMA (ADC -> I2S TX)
        ret = bk_dma_start(adc_i2s_adc_dma_id);
        if (ret != BK_OK) {
            os_printf("adc dma start failed\n");
            bk_i2s_stop();
            bk_i2s_enable(I2S_DISABLE);
            bk_dma_deinit(adc_i2s_adc_dma_id);
            bk_dma_free(DMA_DEV_AUDIO, adc_i2s_adc_dma_id);
            adc_i2s_adc_dma_id = DMA_ID_MAX;
            bk_aud_adc_deinit();
            bk_i2s_deinit();
            bk_i2s_driver_deinit();
            return;
        }
        os_printf("ADC DMA started successfully\n");

        // Start ADC
        ret = bk_aud_adc_start(AUD_ADC_CHL_0);
        if (ret != BK_OK) {
            os_printf("bk_aud_adc_start failed: %d\n", ret);
            return;
        }
        bk_aud_adc_enable_used_channel(1<<AUD_ADC_CHL_0);

        g_adc_i2s_test_initialized = true;
        os_printf("ADC -> I2S out test started successfully\n");
        os_printf("Note: Audio from ADC will be output to I2S TX pins\n");

		/* dump adc reg */
		bk_aud_adc_dump_reg();

        return;
    }
    else if (os_strcmp(argv[1], "stop") == 0)
    {
        if (!g_adc_i2s_test_initialized) {
            os_printf("adc i2s test not started\n");
            return;
        }

        bk_i2s_stop();
        bk_i2s_enable(I2S_DISABLE);

        // Stop DMA
        bk_dma_driver_deinit();
        if (adc_i2s_adc_dma_id != DMA_ID_MAX) {
            bk_dma_stop(adc_i2s_adc_dma_id);
            bk_dma_deinit(adc_i2s_adc_dma_id);
            bk_dma_free(DMA_DEV_AUDIO, adc_i2s_adc_dma_id);
            adc_i2s_adc_dma_id = DMA_ID_MAX;
        }

        // Deinitialize
        bk_aud_adc_deinit();
        bk_i2s_deinit();
        bk_i2s_driver_deinit();

        g_adc_i2s_test_initialized = false;
        os_printf("ADC -> I2S out test stop successful\n");
        return;
    } else {
        cli_aud_help();
        return;
    }
}

#if CONFIG_AUDIO_RING_BUFF
// ADC -> Ringbuf -> I2S out test (DMA: ADC FIFO -> ringbuf, ringbuf -> I2S TX FIFO)
#define ADC_I2S_RINGBUF_SAFE_INTERVAL 20
static dma_id_t adc_i2s_ringbuf_adc_dma_id = DMA_ID_MAX;   // DMA from ADC FIFO to ADC ringbuf
static dma_id_t adc_i2s_ringbuf_i2s_dma_id = DMA_ID_MAX;   // DMA from I2S ringbuf to I2S TX FIFO
// ADC ringbuf: ADC DMA writes to this buffer
static RingBufferContext *adc_i2s_ringbuf_adc_rb = NULL;
static uint8_t *adc_i2s_ringbuf_adc_buffer = NULL;
static uint32_t adc_i2s_ringbuf_adc_size = 0;
// I2S ringbuf: I2S DMA reads from this buffer
static RingBufferContext *adc_i2s_ringbuf_i2s_rb = NULL;
static uint8_t *adc_i2s_ringbuf_i2s_buffer = NULL;
static uint32_t adc_i2s_ringbuf_i2s_size = 0;
static uint32_t adc_i2s_ringbuf_adc_transfer_len = 0;
static uint32_t adc_i2s_ringbuf_i2s_transfer_len = 0;
static uint8_t *adc_i2s_ringbuf_transfer_buf = NULL;  // used in task to transfer data from ADC to I2S ringbuf
static bool g_adc_i2s_ringbuf_test_initialized = false;
static uint8_t g_adc_i2s_ringbuf_dump_mode = 0;
static uint8_t g_i2s_dac_ringbuf_dump_mode = 0;

typedef enum {
	ADC_I2S_RINGBUF_ADC_DMA_DONE_MSG = 0,
} adc_i2s_ringbuf_adc_msg_t;

typedef enum {
	ADC_I2S_RINGBUF_I2S_DMA_DONE_MSG = 0,
} adc_i2s_ringbuf_i2s_msg_t;

static beken_queue_t adc_i2s_ringbuf_adc_que = NULL;
static beken_thread_t adc_i2s_ringbuf_adc_task = NULL;
static beken_queue_t adc_i2s_ringbuf_i2s_que = NULL;
static beken_thread_t adc_i2s_ringbuf_i2s_task = NULL;
static uint8_t g_dmic_en = 0;
static uint8_t g_dmic_dbg = 1;
static uint8_t g_adc_bits = 16;
static uint8_t g_dac_bits = 16;

static bk_err_t adc_i2s_ringbuf_adc_send_msg(adc_i2s_ringbuf_adc_msg_t msg)
{
	if (adc_i2s_ringbuf_adc_que == NULL) {
		return BK_FAIL;
	}
	return rtos_push_to_queue(&adc_i2s_ringbuf_adc_que, &msg, BEKEN_NO_WAIT);
}

static bk_err_t adc_i2s_ringbuf_i2s_send_msg(adc_i2s_ringbuf_i2s_msg_t msg)
{
	if (adc_i2s_ringbuf_i2s_que == NULL) {
		return BK_FAIL;
	}
	return rtos_push_to_queue(&adc_i2s_ringbuf_i2s_que, &msg, BEKEN_NO_WAIT);
}

#define ADC_FRAME_DURATION   (20)
#define DMIC_DBG             (3)
static uint32_t g_transfer_len = 0;

static void adc_i2s_ringbuf_start_cleanup(void)
{
	if (adc_i2s_ringbuf_adc_dma_id >= DMA_ID_0 && adc_i2s_ringbuf_adc_dma_id < DMA_ID_MAX) {
		bk_dma_stop(adc_i2s_ringbuf_adc_dma_id);
		bk_dma_deinit(adc_i2s_ringbuf_adc_dma_id);
		bk_dma_free(DMA_DEV_AUDIO, adc_i2s_ringbuf_adc_dma_id);
		adc_i2s_ringbuf_adc_dma_id = DMA_ID_MAX;
	}
	if (adc_i2s_ringbuf_i2s_dma_id >= DMA_ID_0 && adc_i2s_ringbuf_i2s_dma_id < DMA_ID_MAX) {
		bk_dma_stop(adc_i2s_ringbuf_i2s_dma_id);
		bk_dma_deinit(adc_i2s_ringbuf_i2s_dma_id);
		bk_dma_free(DMA_DEV_AUDIO, adc_i2s_ringbuf_i2s_dma_id);
		adc_i2s_ringbuf_i2s_dma_id = DMA_ID_MAX;
	}
	bk_i2s_stop();
	bk_i2s_enable(I2S_DISABLE);
	if (adc_i2s_ringbuf_adc_task != NULL) {
		rtos_delete_thread(&adc_i2s_ringbuf_adc_task);
		adc_i2s_ringbuf_adc_task = NULL;
	}
	if (adc_i2s_ringbuf_adc_que != NULL) {
		rtos_deinit_queue(&adc_i2s_ringbuf_adc_que);
		adc_i2s_ringbuf_adc_que = NULL;
	}
	if (adc_i2s_ringbuf_i2s_task != NULL) {
		rtos_delete_thread(&adc_i2s_ringbuf_i2s_task);
		adc_i2s_ringbuf_i2s_task = NULL;
	}
	if (adc_i2s_ringbuf_i2s_que != NULL) {
		rtos_deinit_queue(&adc_i2s_ringbuf_i2s_que);
		adc_i2s_ringbuf_i2s_que = NULL;
	}
	if (adc_i2s_ringbuf_transfer_buf != NULL) {
		os_free(adc_i2s_ringbuf_transfer_buf);
		adc_i2s_ringbuf_transfer_buf = NULL;
	}
	if (adc_i2s_ringbuf_adc_rb != NULL) {
		os_free(adc_i2s_ringbuf_adc_rb);
		adc_i2s_ringbuf_adc_rb = NULL;
	}
	if (adc_i2s_ringbuf_adc_buffer != NULL) {
		os_free(adc_i2s_ringbuf_adc_buffer);
		adc_i2s_ringbuf_adc_buffer = NULL;
	}
	if (adc_i2s_ringbuf_i2s_rb != NULL) {
		os_free(adc_i2s_ringbuf_i2s_rb);
		adc_i2s_ringbuf_i2s_rb = NULL;
	}
	if (adc_i2s_ringbuf_i2s_buffer != NULL) {
		os_free(adc_i2s_ringbuf_i2s_buffer);
		adc_i2s_ringbuf_i2s_buffer = NULL;
	}
	if (g_adc_i2s_ringbuf_dump_mode == 1) {
		g_adc_i2s_ringbuf_dump_mode = 0;
	}
	bk_dma_driver_deinit();
	bk_aud_adc_deinit();
	bk_i2s_deinit();
	bk_i2s_driver_deinit();
}

// I2S RX -> Ringbuf -> DAC test (DMA: I2S RX FIFO -> I2S RX ringbuf, DAC ringbuf -> DAC FIFO)
#define I2S_DAC_RINGBUF_SAFE_INTERVAL 20
#define I2S_DAC_FRAME_DURATION       20
static dma_id_t i2s_dac_ringbuf_i2s_dma_id = DMA_ID_MAX;   // DMA from I2S RX FIFO to I2S RX ringbuf
static dma_id_t i2s_dac_ringbuf_dac_dma_id = DMA_ID_MAX;   // DMA from DAC ringbuf to DAC FIFO
static RingBufferContext *i2s_dac_ringbuf_i2s_rb = NULL;
static uint8_t *i2s_dac_ringbuf_i2s_buffer = NULL;
static uint32_t i2s_dac_ringbuf_i2s_size = 0;
static RingBufferContext *i2s_dac_ringbuf_dac_rb = NULL;
static uint8_t *i2s_dac_ringbuf_dac_buffer = NULL;
static uint32_t i2s_dac_ringbuf_dac_size = 0;
static uint32_t i2s_dac_ringbuf_transfer_len = 0;
static uint8_t *i2s_dac_ringbuf_transfer_buf = NULL;
static bool g_i2s_dac_ringbuf_test_initialized = false;
static aud_dac_source_t g_i2s_dac_ringbuf_dac_source = AUD_DAC_SOURCE_A2DP;

typedef enum {
	I2S_DAC_RINGBUF_I2S_DMA_DONE_MSG = 0,
} i2s_dac_ringbuf_i2s_msg_t;

typedef enum {
	I2S_DAC_RINGBUF_DAC_DMA_DONE_MSG = 0,
} i2s_dac_ringbuf_dac_msg_t;

static beken_queue_t i2s_dac_ringbuf_i2s_que = NULL;
static beken_thread_t i2s_dac_ringbuf_i2s_task = NULL;
static beken_queue_t i2s_dac_ringbuf_dac_que = NULL;
static beken_thread_t i2s_dac_ringbuf_dac_task = NULL;

static bk_err_t i2s_dac_ringbuf_i2s_send_msg(i2s_dac_ringbuf_i2s_msg_t msg)
{
	if (i2s_dac_ringbuf_i2s_que == NULL) {
		return BK_FAIL;
	}
	return rtos_push_to_queue(&i2s_dac_ringbuf_i2s_que, &msg, BEKEN_NO_WAIT);
}

static bk_err_t i2s_dac_ringbuf_dac_send_msg(i2s_dac_ringbuf_dac_msg_t msg)
{
	if (i2s_dac_ringbuf_dac_que == NULL) {
		return BK_FAIL;
	}
	return rtos_push_to_queue(&i2s_dac_ringbuf_dac_que, &msg, BEKEN_NO_WAIT);
}

static void i2s_dac_ringbuf_start_cleanup(void)
{
	if (i2s_dac_ringbuf_i2s_dma_id >= DMA_ID_0 && i2s_dac_ringbuf_i2s_dma_id < DMA_ID_MAX) {
		bk_dma_stop(i2s_dac_ringbuf_i2s_dma_id);
		bk_dma_deinit(i2s_dac_ringbuf_i2s_dma_id);
		bk_dma_free(DMA_DEV_AUDIO, i2s_dac_ringbuf_i2s_dma_id);
		i2s_dac_ringbuf_i2s_dma_id = DMA_ID_MAX;
	}
	if (i2s_dac_ringbuf_dac_dma_id >= DMA_ID_0 && i2s_dac_ringbuf_dac_dma_id < DMA_ID_MAX) {
		bk_dma_stop(i2s_dac_ringbuf_dac_dma_id);
		bk_dma_deinit(i2s_dac_ringbuf_dac_dma_id);
		bk_dma_free(DMA_DEV_AUDIO, i2s_dac_ringbuf_dac_dma_id);
		i2s_dac_ringbuf_dac_dma_id = DMA_ID_MAX;
	}
	bk_i2s_stop();
	bk_i2s_enable(I2S_DISABLE);
	if (i2s_dac_ringbuf_i2s_task != NULL) {
		rtos_delete_thread(&i2s_dac_ringbuf_i2s_task);
		i2s_dac_ringbuf_i2s_task = NULL;
	}
	if (i2s_dac_ringbuf_i2s_que != NULL) {
		rtos_deinit_queue(&i2s_dac_ringbuf_i2s_que);
		i2s_dac_ringbuf_i2s_que = NULL;
	}
	if (i2s_dac_ringbuf_dac_task != NULL) {
		rtos_delete_thread(&i2s_dac_ringbuf_dac_task);
		i2s_dac_ringbuf_dac_task = NULL;
	}
	if (i2s_dac_ringbuf_dac_que != NULL) {
		rtos_deinit_queue(&i2s_dac_ringbuf_dac_que);
		i2s_dac_ringbuf_dac_que = NULL;
	}
	if (i2s_dac_ringbuf_transfer_buf != NULL) {
		os_free(i2s_dac_ringbuf_transfer_buf);
		i2s_dac_ringbuf_transfer_buf = NULL;
	}
	if (i2s_dac_ringbuf_i2s_rb != NULL) {
		os_free(i2s_dac_ringbuf_i2s_rb);
		i2s_dac_ringbuf_i2s_rb = NULL;
	}
	if (i2s_dac_ringbuf_i2s_buffer != NULL) {
		os_free(i2s_dac_ringbuf_i2s_buffer);
		i2s_dac_ringbuf_i2s_buffer = NULL;
	}
	if (i2s_dac_ringbuf_dac_rb != NULL) {
		os_free(i2s_dac_ringbuf_dac_rb);
		i2s_dac_ringbuf_dac_rb = NULL;
	}
	if (i2s_dac_ringbuf_dac_buffer != NULL) {
		os_free(i2s_dac_ringbuf_dac_buffer);
		i2s_dac_ringbuf_dac_buffer = NULL;
	}
	bk_dma_driver_deinit();
	bk_aud_dac_deinit();
	bk_i2s_deinit();
	bk_i2s_driver_deinit();
}

static __attribute__((aligned(8))) uint32_t  AUD_PCM_48000_24BITS_test_data[320] = {
#if 1
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
	0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000, 0x5a5b5c5d, 0x00000000,
#else
	0x007fffff, 0x007fffff, 0x007ee7a9, 0x007ee7a9, 0x007ba374, 0x007ba374, 0x007641ae, 0x007641ae,
	0x006ed9ea, 0x006ed9ea, 0x00658c99, 0x00658c99, 0x005a8278, 0x005a8278, 0x004debe4, 0x004debe4,
	0x003fffff, 0x003fffff, 0x0030fbc4, 0x0030fbc4, 0x002120fb, 0x002120fb, 0x0010b514, 0x0010b514,
	0x00000000, 0x00000000, 0xffef4aec, 0xffef4aec, 0xffdedf05, 0xffdedf05, 0xffcf043c, 0xffcf043c,
	0xffc00001, 0xffc00001, 0xffb2141c, 0xffb2141c, 0xffa57d88, 0xffa57d88, 0xff9a7367, 0xff9a7367,
	0xff912616, 0xff912616, 0xff89be52, 0xff89be52, 0xff845c8c, 0xff845c8c, 0xff811857, 0xff811857,
	0xff800001, 0xff800001, 0xff811857, 0xff811857, 0xff845c8c, 0xff845c8c, 0xff89be52, 0xff89be52,
	0xff912616, 0xff912616, 0xff9a7367, 0xff9a7367, 0xffa57d88, 0xffa57d88, 0xffb2141c, 0xffb2141c,
	0xffc00001, 0xffc00001, 0xffcf043c, 0xffcf043c, 0xffdedf05, 0xffdedf05, 0xffef4aec, 0xffef4aec,
	0x00000000, 0x00000000, 0x0010b514, 0x0010b514, 0x002120fb, 0x002120fb, 0x0030fbc4, 0x0030fbc4,
	0x003fffff, 0x003fffff, 0x004debe4, 0x004debe4, 0x005a8278, 0x005a8278, 0x00658c99, 0x00658c99,
	0x006ed9ea, 0x006ed9ea, 0x007641ae, 0x007641ae, 0x007ba374, 0x007ba374, 0x007ee7a9, 0x007ee7a9,
	0x007fffff, 0x007fffff, 0x007ee7a9, 0x007ee7a9, 0x007ba374, 0x007ba374, 0x007641ae, 0x007641ae,
	0x006ed9ea, 0x006ed9ea, 0x00658c99, 0x00658c99, 0x005a8278, 0x005a8278, 0x004debe4, 0x004debe4,
	0x003fffff, 0x003fffff, 0x0030fbc4, 0x0030fbc4, 0x002120fb, 0x002120fb, 0x0010b514, 0x0010b514,
	0x00000000, 0x00000000, 0xffef4aec, 0xffef4aec, 0xffdedf05, 0xffdedf05, 0xffcf043c, 0xffcf043c,
	0xffc00001, 0xffc00001, 0xffb2141c, 0xffb2141c, 0xffa57d88, 0xffa57d88, 0xff9a7367, 0xff9a7367,
	0xff912616, 0xff912616, 0xff89be52, 0xff89be52, 0xff845c8c, 0xff845c8c, 0xff811857, 0xff811857,
	0xff800001, 0xff800001, 0xff811857, 0xff811857, 0xff845c8c, 0xff845c8c, 0xff89be52, 0xff89be52,
	0xff912616, 0xff912616, 0xff9a7367, 0xff9a7367, 0xffa57d88, 0xffa57d88, 0xffb2141c, 0xffb2141c,
	0xffc00001, 0xffc00001, 0xffcf043c, 0xffcf043c, 0xffdedf05, 0xffdedf05, 0xffef4aec, 0xffef4aec,
	0x00000000, 0x00000000, 0x0010b514, 0x0010b514, 0x002120fb, 0x002120fb, 0x0030fbc4, 0x0030fbc4,
	0x003fffff, 0x003fffff, 0x004debe4, 0x004debe4, 0x005a8278, 0x005a8278, 0x00658c99, 0x00658c99,
	0x006ed9ea, 0x006ed9ea, 0x007641ae, 0x007641ae, 0x007ba374, 0x007ba374, 0x007ee7a9, 0x007ee7a9,
	0x007fffff, 0x007fffff, 0x007ee7a9, 0x007ee7a9, 0x007ba374, 0x007ba374, 0x007641ae, 0x007641ae,
	0x006ed9ea, 0x006ed9ea, 0x00658c99, 0x00658c99, 0x005a8278, 0x005a8278, 0x004debe4, 0x004debe4,
	0x003fffff, 0x003fffff, 0x0030fbc4, 0x0030fbc4, 0x002120fb, 0x002120fb, 0x0010b514, 0x0010b514,
	0x00000000, 0x00000000, 0xffef4aec, 0xffef4aec, 0xffdedf05, 0xffdedf05, 0xffcf043c, 0xffcf043c,
	0xffc00001, 0xffc00001, 0xffb2141c, 0xffb2141c, 0xffa57d88, 0xffa57d88, 0xff9a7367, 0xff9a7367,
	0xff912616, 0xff912616, 0xff89be52, 0xff89be52, 0xff845c8c, 0xff845c8c, 0xff811857, 0xff811857,
	0xff800001, 0xff800001, 0xff811857, 0xff811857, 0xff845c8c, 0xff845c8c, 0xff89be52, 0xff89be52,
	0xff912616, 0xff912616, 0xff9a7367, 0xff9a7367, 0xffa57d88, 0xffa57d88, 0xffb2141c, 0xffb2141c,
	0xffc00001, 0xffc00001, 0xffcf043c, 0xffcf043c, 0xffdedf05, 0xffdedf05, 0xffef4aec, 0xffef4aec,
	0x00000000, 0x00000000, 0x0010b514, 0x0010b514, 0x002120fb, 0x002120fb, 0x0030fbc4, 0x0030fbc4,
	0x003fffff, 0x003fffff, 0x004debe4, 0x004debe4, 0x005a8278, 0x005a8278, 0x00658c99, 0x00658c99,
	0x006ed9ea, 0x006ed9ea, 0x007641ae, 0x007641ae, 0x007ba374, 0x007ba374, 0x007ee7a9, 0x007ee7a9,
    0x007fffff, 0x007fffff, 0x007ee7a9, 0x007ee7a9, 0x007ba374, 0x007ba374, 0x007641ae, 0x007641ae,
	0x006ed9ea, 0x006ed9ea, 0x00658c99, 0x00658c99, 0x005a8278, 0x005a8278, 0x004debe4, 0x004debe4,
	0x003fffff, 0x003fffff, 0x0030fbc4, 0x0030fbc4, 0x002120fb, 0x002120fb, 0x0010b514, 0x0010b514,
	0x00000000, 0x00000000, 0xffef4aec, 0xffef4aec, 0xffdedf05, 0xffdedf05, 0xffcf043c, 0xffcf043c,
#endif
};


//__attribute__((aligned(8)))  uint32_t dbg[640];// = (uint32_t *)os_malloc(adc_i2s_ringbuf_adc_transfer_len);
//__attribute__((aligned(8)))  uint32_t zero[640];// = (uint32_t *)os_malloc(adc_i2s_ringbuf_adc_transfer_len);
static uint32_t __maybe_unused g_magic[2] = {0x5A5B5C5D, 0x6A6B6C6D};

static uint32_t * dbg = NULL;
static uint16_t * dbg_16 = NULL;

// Task 1: Transfer data from ADC ringbuf to I2S ringbuf
static void aud_adc_i2s_ringbuf_adc_task_entry(beken_thread_arg_t data)
{
	uint32_t i = 0, j = 0;
	adc_i2s_ringbuf_adc_msg_t msg;
	uint32_t * __maybe_unused p = NULL;
	uint16_t * __maybe_unused p_16 = NULL;
	if (g_adc_bits == 24) {
		dbg = (uint32_t *)psram_malloc(g_transfer_len*2);
	} else if (g_adc_bits == 16)
	{
		dbg_16 = (uint16_t *)psram_malloc(g_transfer_len*2);
	}
	os_printf("g_transfer_len : %d\r\n", g_transfer_len);
	if (dbg == NULL) {
	//	os_printf("adc task: malloc dbg failed\n");
	//	return;
	}
	if (adc_i2s_ringbuf_transfer_buf == NULL || adc_i2s_ringbuf_adc_transfer_len == 0) {
		os_printf("adc task: transfer buf not initialized\n");
		return;
	}
	while (1) {
		if (rtos_pop_from_queue(&adc_i2s_ringbuf_adc_que, &msg, BEKEN_WAIT_FOREVER) != BK_OK) {
			continue;
		}
		
		if (msg == ADC_I2S_RINGBUF_ADC_DMA_DONE_MSG) {
			GPIO_DOWN(34);GPIO_UP(34);
			// ADC DMA finished, transfer data from ADC ringbuf to I2S ringbuf
			if (adc_i2s_ringbuf_adc_rb != NULL && adc_i2s_ringbuf_i2s_rb != NULL && 
			    adc_i2s_ringbuf_transfer_buf != NULL && adc_i2s_ringbuf_adc_transfer_len > 0) {
				uint32_t adc_fill_size = ring_buffer_get_fill_size(adc_i2s_ringbuf_adc_rb);
				uint32_t i2s_free_size = ring_buffer_get_free_size(adc_i2s_ringbuf_i2s_rb);
				
				// Check if we have enough data in ADC ringbuf and space in I2S ringbuf
				if (adc_fill_size >= adc_i2s_ringbuf_adc_transfer_len && 
				    i2s_free_size >= adc_i2s_ringbuf_adc_transfer_len) {
				// Read from ADC ringbuf
				uint32_t read_size = ring_buffer_read(adc_i2s_ringbuf_adc_rb, 
				                                     adc_i2s_ringbuf_transfer_buf, 
				                                     adc_i2s_ringbuf_adc_transfer_len);
			//	os_printf("%d---%d\r\n", read_size, adc_i2s_ringbuf_adc_transfer_len);
				if (read_size == adc_i2s_ringbuf_adc_transfer_len) {
					// Dump data if enabled (dump before writing to I2S ringbuf)
					 if (g_adc_i2s_ringbuf_dump_mode == 1)
					 {
					 	GPIO_DOWN(35);GPIO_UP(35);
						uint32_t int_level = rtos_enter_critical();
					 	bk_uart_write_bytes(1, (void *)g_magic, sizeof(g_magic));
					 	bk_uart_write_bytes(1, (void *)adc_i2s_ringbuf_transfer_buf, read_size);
					 	rtos_exit_critical(int_level);
					 	GPIO_DOWN(35);
					 }
					 if (g_adc_bits == 24) {
						 p = (uint32_t *)(adc_i2s_ringbuf_transfer_buf);
						 for (i = 0, j = 0; i < (read_size>>2); i++)
						 {
							dbg[j] = p[i];//0x5a5b5c5d;
							dbg[j+1]= 0x00000000;
							j += 2;
						 }
					 } else if (g_adc_bits == 16) 
					 {
						 p_16 = (uint16_t *)(adc_i2s_ringbuf_transfer_buf);
						 for (i = 0, j = 0; i < (read_size>>1); i++)
						 {
							dbg_16[j] = p_16[i];//0x5a5b5c5d;
							dbg_16[j+1]= 0x0000;
							j += 2;
						 }
					 }
 					 if (g_adc_i2s_ringbuf_dump_mode == 2) {
					 	GPIO_DOWN(35);GPIO_UP(35);
						uint32_t int_level = rtos_enter_critical();
					 	bk_uart_write_bytes(1, (void *)g_magic, sizeof(g_magic));
					 	bk_uart_write_bytes(1, (void *)dbg_16, read_size*2);
					 	rtos_exit_critical(int_level);
					 	GPIO_DOWN(35);
					 }
						// Write to I2S ringbuf
						uint32_t write_size = 0;
						if (g_adc_i2s_ringbuf_dump_mode == 2)
						{
							write_size = ring_buffer_write(adc_i2s_ringbuf_i2s_rb, 
							                                        (uint8_t *)AUD_PCM_48000_24BITS_test_data, 
							                                        read_size);
						} else
						{
							if (g_dmic_en == 1) {
								write_size = ring_buffer_write(adc_i2s_ringbuf_i2s_rb, 
																		(uint8_t *)adc_i2s_ringbuf_transfer_buf,
																		read_size);
							} else {
								if (g_adc_bits == 24) {
									write_size = ring_buffer_write(adc_i2s_ringbuf_i2s_rb, 
																			(uint8_t *)dbg,
																			read_size * 2);
								} else if (g_adc_bits == 16) {
									write_size = ring_buffer_write(adc_i2s_ringbuf_i2s_rb, 
																			(uint8_t *)dbg_16,
																			read_size * 2);
								}
							}
						}
						if (write_size != read_size) {
						//	os_printf("write to i2s ringbuf error, size: %d, expected: %d\n", write_size, read_size);
						}
					} else {
						os_printf("read from adc ringbuf error, size: %d, expected: %d\n", read_size, adc_i2s_ringbuf_adc_transfer_len);
					}
				}
			}
			GPIO_DOWN(34);
		}
	}
}

// Task 2: Handle I2S DMA completion (for dump, etc.)
static uint8_t __maybe_unused g_cntt = 0;
static void aud_adc_i2s_ringbuf_i2s_task_entry(beken_thread_arg_t data)
{
	adc_i2s_ringbuf_i2s_msg_t msg;
	uint8_t *dump_buf = NULL;
	
	if (adc_i2s_ringbuf_transfer_buf == NULL || adc_i2s_ringbuf_i2s_transfer_len == 0) {
		os_printf("i2s task: transfer buf not initialized\n");
		return;
	}
	
	dump_buf = (uint8_t *)os_malloc(adc_i2s_ringbuf_i2s_transfer_len);
	if (dump_buf == NULL) {
		os_printf("i2s task: malloc dump buf fail\n");
		return;
	}
	os_memset(dump_buf, 0, adc_i2s_ringbuf_i2s_transfer_len);
	
	while (1) {
		if (rtos_pop_from_queue(&adc_i2s_ringbuf_i2s_que, &msg, BEKEN_WAIT_FOREVER) != BK_OK) {
			continue;
		}
		
		if (msg == ADC_I2S_RINGBUF_I2S_DMA_DONE_MSG) 
		{
			// I2S DMA finished, advance I2S ringbuf read pointer (DMA has already read data to I2S FIFO)
			if (adc_i2s_ringbuf_i2s_rb != NULL && adc_i2s_ringbuf_transfer_buf != NULL && 
			    adc_i2s_ringbuf_i2s_transfer_len > 0) {
				// Manually advance read pointer (same as single ringbuf version)
				ring_buffer_read(adc_i2s_ringbuf_i2s_rb, adc_i2s_ringbuf_transfer_buf, adc_i2s_ringbuf_i2s_transfer_len);
				// Copy to dump_buf if dump is enabled
				if (g_adc_i2s_ringbuf_dump_mode == 1) {
					os_memcpy(dump_buf, adc_i2s_ringbuf_transfer_buf, adc_i2s_ringbuf_i2s_transfer_len);
					GPIO_DOWN(35);GPIO_UP(35);
					bk_uart_write_bytes(1, (void *)dump_buf, adc_i2s_ringbuf_i2s_transfer_len);
					GPIO_DOWN(35);
				}
			}
		}
	}
	os_free(dump_buf);
	dump_buf = NULL;
}

static void aud_adc_i2s_ringbuf_adc_dma_finish_isr(dma_id_t dma_id)
{
	GPIO_DOWN(36);GPIO_UP(36);
	if (adc_i2s_ringbuf_adc_rb == NULL || dma_id != adc_i2s_ringbuf_adc_dma_id) {
		return;
	}
	adc_i2s_ringbuf_adc_send_msg(ADC_I2S_RINGBUF_ADC_DMA_DONE_MSG);
	GPIO_DOWN(36);
}

static void aud_adc_i2s_ringbuf_i2s_dma_finish_isr(dma_id_t dma_id)
{
	GPIO_DOWN(37);GPIO_UP(37);
	if (adc_i2s_ringbuf_i2s_rb == NULL || dma_id != adc_i2s_ringbuf_i2s_dma_id) {
		return;
	}
//	adc_i2s_ringbuf_i2s_send_msg(ADC_I2S_RINGBUF_I2S_DMA_DONE_MSG);
	GPIO_DOWN(37);
}

// I2S RX -> ringbuf -> DAC: task 1, transfer from I2S RX ringbuf to DAC ringbuf
static uint32_t * __maybe_unused p_dac = NULL;
static void aud_i2s_dac_ringbuf_i2s_task_entry(beken_thread_arg_t data)
{
	i2s_dac_ringbuf_i2s_msg_t msg;
	uint32_t __maybe_unused i = 0, j = 0;
	uint32_t read_size  = 0;
	p_dac = (uint32_t *)psram_malloc(i2s_dac_ringbuf_transfer_len * 2);
	if (p_dac == NULL) {
		os_printf("malloc p_dac fail\n");
		return;
	}
	if (i2s_dac_ringbuf_transfer_buf == NULL || i2s_dac_ringbuf_transfer_len == 0) {
		os_printf("i2s_dac_ringbuf i2s task: transfer buf not initialized\n");
		return;
	}
	os_printf("i2s_dac_ringbuf_transfer_len : %d\r\n", i2s_dac_ringbuf_transfer_len);
	while (1) {
		if (rtos_pop_from_queue(&i2s_dac_ringbuf_i2s_que, &msg, BEKEN_WAIT_FOREVER) != BK_OK) {
			continue;
		}
		if (msg == I2S_DAC_RINGBUF_I2S_DMA_DONE_MSG) {
			if (i2s_dac_ringbuf_i2s_rb != NULL && i2s_dac_ringbuf_dac_rb != NULL &&
			    i2s_dac_ringbuf_transfer_buf != NULL && i2s_dac_ringbuf_transfer_len > 0) {
				uint32_t i2s_fill = ring_buffer_get_fill_size(i2s_dac_ringbuf_i2s_rb);
				uint32_t dac_free = ring_buffer_get_free_size(i2s_dac_ringbuf_dac_rb);
				if (i2s_fill >= i2s_dac_ringbuf_transfer_len && dac_free >= i2s_dac_ringbuf_transfer_len) {
					read_size = ring_buffer_read(i2s_dac_ringbuf_i2s_rb, i2s_dac_ringbuf_transfer_buf, i2s_dac_ringbuf_transfer_len);
 					if (g_i2s_dac_ringbuf_dump_mode == 1) {
						GPIO_DOWN(35);GPIO_UP(35);
						uint32_t int_level = rtos_enter_critical();
						bk_uart_write_bytes(1, (void *)g_magic, sizeof(g_magic));
						bk_uart_write_bytes(1, (void *)i2s_dac_ringbuf_transfer_buf, read_size);
						rtos_exit_critical(int_level);
						GPIO_DOWN(35);
					}
					if (g_dac_bits == 16) {
						if (read_size == i2s_dac_ringbuf_transfer_len) {
							ring_buffer_write(i2s_dac_ringbuf_dac_rb, i2s_dac_ringbuf_transfer_buf, read_size);
						}
					} else if (g_dac_bits == 24) {
						uint32_t * pp = (uint32_t *)i2s_dac_ringbuf_transfer_buf;
						for (i = 0; i < ((read_size>>2)>>1); i++)
						{
							p_dac[i] = pp[2 * i];
						}
//	 					if (g_i2s_dac_ringbuf_dump_mode == 2) {
//							GPIO_DOWN(35);GPIO_UP(35);
//							uint32_t int_level = rtos_enter_critical();
//							bk_uart_write_bytes(1, (void *)g_magic, sizeof(g_magic));
//							bk_uart_write_bytes(1, (void *)p_dac, (read_size)>>1);
//							rtos_exit_critical(int_level);
//							GPIO_DOWN(35);
//						}
						if (read_size == i2s_dac_ringbuf_transfer_len) {
							ring_buffer_write(i2s_dac_ringbuf_dac_rb, (uint8_t *)p_dac, (read_size)>>1);
						//	ring_buffer_write(i2s_dac_ringbuf_dac_rb, i2s_dac_ringbuf_transfer_buf, read_size); ///16k-1280Bytes-20ms
						}
					}
				}
			}
		}
	}
}

// I2S RX -> ringbuf -> DAC: task 2, advance DAC ringbuf read ptr on DAC DMA done
static void aud_i2s_dac_ringbuf_dac_task_entry(beken_thread_arg_t data)
{
	i2s_dac_ringbuf_dac_msg_t msg;

	if (i2s_dac_ringbuf_transfer_buf == NULL || i2s_dac_ringbuf_transfer_len == 0) {
		os_printf("i2s_dac_ringbuf dac task: transfer buf not initialized\n");
		return;
	}
	while (1) {
		if (rtos_pop_from_queue(&i2s_dac_ringbuf_dac_que, &msg, BEKEN_WAIT_FOREVER) != BK_OK) {
			continue;
		}
		if (msg == I2S_DAC_RINGBUF_DAC_DMA_DONE_MSG) {
			if (i2s_dac_ringbuf_dac_rb != NULL && i2s_dac_ringbuf_transfer_buf != NULL &&
			    i2s_dac_ringbuf_transfer_len > 0) {
				ring_buffer_read(i2s_dac_ringbuf_dac_rb, i2s_dac_ringbuf_transfer_buf, i2s_dac_ringbuf_transfer_len);
			}
		}
	}
}

static void aud_i2s_dac_ringbuf_i2s_dma_finish_isr(dma_id_t dma_id)
{
	GPIO_DOWN(31);GPIO_UP(31);
	if (i2s_dac_ringbuf_i2s_rb == NULL || dma_id != i2s_dac_ringbuf_i2s_dma_id) {
		return;
	}
	i2s_dac_ringbuf_i2s_send_msg(I2S_DAC_RINGBUF_I2S_DMA_DONE_MSG);
	GPIO_DOWN(31);
}

static void aud_i2s_dac_ringbuf_dac_dma_finish_isr(dma_id_t dma_id)
{
	GPIO_DOWN(30);GPIO_UP(30);
	if (i2s_dac_ringbuf_dac_rb == NULL || dma_id != i2s_dac_ringbuf_dac_dma_id) {
		return;
	}
//	i2s_dac_ringbuf_dac_send_msg(I2S_DAC_RINGBUF_DAC_DMA_DONE_MSG);
	GPIO_DOWN(30);
}

static void cli_aud_adc_i2s_ringbuf_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	os_printf("[+]%s\r\n", __func__);
	bk_err_t ret = BK_OK;
	dma_config_t adc_dma_config = {0};
	dma_config_t i2s_dma_config = {0};
	aud_adc_config_t adc_config = DEFAULT_AUD_ADC_CONFIG();
	i2s_config_t i2s_config = DEFAULT_I2S_CONFIG();
	uint32_t adc_fifo_addr = 0;
	uint32_t i2s_tx_fifo_addr = 0;
	uint32_t sample_rate = 0;
	i2s_samp_rate_t i2s_samp_rate = I2S_SAMP_RATE_16000;
	uint32_t gpio_group = I2S_GPIO_GROUP_0;
	uint32_t adc_bits = 16;
	uint32_t __maybe_unused i2s_data_len = 16;
	i2s_lrcom_store_mode_t i2s_store_mode = I2S_LRCOM_STORE_16R16L;
	uint32_t ch_bitmap = 0;

	g_adc_i2s_ringbuf_dump_mode = 0;

	if (argc < 2) {
		cli_aud_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		if (g_adc_i2s_ringbuf_test_initialized) {
			os_printf("adc i2s ringbuf test already started\n");
			return;
		}
		if (argc < 3) {
			os_printf("Usage: aud_adc_i2s_ringbuf_test start {sample_rate} [i2s_role] [gpio_group] [adc_bits] [i2s_store_mode] [i2s_data_len]\n");
			os_printf("  Example: aud_adc_i2s_ringbuf_test start 16000 1 0\n");
			os_printf("  Note: ADC -> ringbuf (DMA) -> I2S TX\n");
			return;
		}

		sample_rate = strtoul(argv[2], NULL, 10);
		adc_config.sample_rate = sample_rate;

		i2s_role_t i2s_role = I2S_ROLE_MASTER;
		if (argc >= 4) {
			uint32_t role = strtoul(argv[3], NULL, 10);
			i2s_role = (role == 0) ? I2S_ROLE_SLAVE : I2S_ROLE_MASTER;
		}
		i2s_config.role = i2s_role;

		if (argc >= 5) {
			gpio_group = strtoul(argv[4], NULL, 10);
		}
		if (argc >= 6) {
			adc_bits = strtoul(argv[5], NULL, 10);
		}
		if (argc >= 7) {
			i2s_store_mode = strtoul(argv[6], NULL, 10);
		}
		if (argc >= 8) {
			i2s_data_len = strtoul(argv[7], NULL, 10);
		}
		if (adc_bits != 16 && i2s_store_mode == I2S_LRCOM_STORE_16R16L) {
			i2s_data_len = 32;
			i2s_store_mode = I2S_LRCOM_STORE_LRLR;
		}
		g_adc_bits = adc_bits;
		g_dmic_en = 0;
		g_dmic_dbg = 1;

		aud_dmic_config_t __maybe_unused dmic_config = DEFAULT_AUD_DMIC_CONFIG();
		if (argc >= 9) {
			if (os_strcmp(argv[8], "dmic") == 0)
			{
				g_dmic_en = 1;
				if (argc >= 10) {
					dmic_config.dmic_mode = strtoul(argv[9], NULL, 10);
					if (dmic_config.dmic_mode == AUD_DMIC_MODE_0) {
						g_dmic_dbg = 1;
					} else if (dmic_config.dmic_mode == AUD_DMIC_MODE_1) {
						g_dmic_dbg = 3;
					}
					if (argc >= 11) {
						dmic_config.channel = strtoul(argv[10], NULL, 10);
					}
				} else {
					g_dmic_dbg = 1;
				}
				os_printf("dmic_mode:%d, g_dmic_dbg:%d\r\n", dmic_config.dmic_mode, g_dmic_dbg);
			}
		}

		i2s_config.lsb_first_en = 1;

		switch (sample_rate) {
			case 8000 : i2s_samp_rate = I2S_SAMP_RATE_8000;  break;
			case 16000: i2s_samp_rate = I2S_SAMP_RATE_16000; break;
			case 24000: i2s_samp_rate = I2S_SAMP_RATE_24000; break;
			case 32000: i2s_samp_rate = I2S_SAMP_RATE_32000; break;
			case 44100: i2s_samp_rate = I2S_SAMP_RATE_44100; break;
			case 48000: i2s_samp_rate = I2S_SAMP_RATE_48000; break;
			case 96000: i2s_samp_rate = I2S_SAMP_RATE_96000; break;
			default:
				os_printf("I2S not support sample rate: %d\n", sample_rate);
				return;
		}

		ret = bk_i2s_driver_init();
		if (ret != BK_OK) {
			os_printf("bk_i2s_driver_init failed: %d\n", ret);
			return;
		}
		i2s_config.samp_rate   = i2s_samp_rate;// * 2;
		i2s_config.data_length = i2s_data_len;
		i2s_config.store_mode  = i2s_store_mode;
		ret = bk_i2s_init(gpio_group, &i2s_config);
		if (ret != BK_OK) {
			os_printf("bk_i2s_init failed: %d\n", ret);
			bk_i2s_driver_deinit();
			return;
		}

		aud_hardware_reset();
		for (uint32_t i = 0; i < AUD_MIC_CHL_NUM_MAX; i++) {
			adc_config.chl_cfg[i].bits = adc_bits;
		}
		ret = bk_aud_adc_init(&adc_config);
		if (ret != BK_OK) {
			os_printf("bk_aud_adc_init failed: %d\n", ret);
			adc_i2s_ringbuf_start_cleanup();
			return;
		}

		ret = bk_dma_driver_init();
		if (ret != BK_OK) {
			os_printf("dma driver init failed\n");
			adc_i2s_ringbuf_start_cleanup();
			return;
		}

		// Allocate ADC ringbuf (ADC DMA writes to this)
		adc_i2s_ringbuf_adc_size = sample_rate / 1000 * ADC_FRAME_DURATION * g_dmic_dbg * (adc_bits == 16 ? 2 : 4) * 6 + ADC_I2S_RINGBUF_SAFE_INTERVAL;
		os_printf("adc_i2s_ringbuf_adc_size:%d\r\n", adc_i2s_ringbuf_adc_size);
		adc_i2s_ringbuf_adc_buffer = (uint8_t *)psram_malloc(adc_i2s_ringbuf_adc_size);
		if (adc_i2s_ringbuf_adc_buffer == NULL) {
			os_printf("malloc adc ringbuf buffer fail\n");
			adc_i2s_ringbuf_start_cleanup();
			return;
		}
		os_memset(adc_i2s_ringbuf_adc_buffer, 0, adc_i2s_ringbuf_adc_size);

		adc_i2s_ringbuf_adc_rb = (RingBufferContext *)psram_malloc(sizeof(RingBufferContext));
		if (adc_i2s_ringbuf_adc_rb == NULL) {
			os_printf("malloc adc ringbuf context fail\n");
			psram_free(adc_i2s_ringbuf_adc_buffer);
			adc_i2s_ringbuf_adc_buffer = NULL;
			adc_i2s_ringbuf_start_cleanup();
			return;
		}

		// Allocate I2S ringbuf (I2S DMA reads from this)
		adc_i2s_ringbuf_i2s_size = adc_i2s_ringbuf_adc_size;// << 1;
		adc_i2s_ringbuf_i2s_buffer = (uint8_t *)psram_malloc(adc_i2s_ringbuf_i2s_size);
		if (adc_i2s_ringbuf_i2s_buffer == NULL) {
			os_printf("malloc i2s ringbuf buffer fail\n");
			psram_free(adc_i2s_ringbuf_adc_rb);
			psram_free(adc_i2s_ringbuf_adc_buffer);
			adc_i2s_ringbuf_adc_rb = NULL;
			adc_i2s_ringbuf_adc_buffer = NULL;
			adc_i2s_ringbuf_start_cleanup();
			return;
		}
		os_memset(adc_i2s_ringbuf_i2s_buffer, 0, adc_i2s_ringbuf_i2s_size);

		adc_i2s_ringbuf_i2s_rb = (RingBufferContext *)psram_malloc(sizeof(RingBufferContext));
		if (adc_i2s_ringbuf_i2s_rb == NULL) {
			os_printf("malloc i2s ringbuf context fail\n");
			psram_free(adc_i2s_ringbuf_i2s_buffer);
			psram_free(adc_i2s_ringbuf_adc_rb);
			psram_free(adc_i2s_ringbuf_adc_buffer);
			adc_i2s_ringbuf_i2s_buffer = NULL;
			adc_i2s_ringbuf_adc_rb = NULL;
			adc_i2s_ringbuf_adc_buffer = NULL;
			adc_i2s_ringbuf_start_cleanup();
			return;
		}

		adc_i2s_ringbuf_adc_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
		if ((adc_i2s_ringbuf_adc_dma_id < DMA_ID_0) || (adc_i2s_ringbuf_adc_dma_id >= DMA_ID_MAX)) {
			os_printf("malloc adc dma fail\n");
			adc_i2s_ringbuf_start_cleanup();
			return;
		}

		if (bk_aud_adc_get_fifo_addr(AUD_ADC_MIC_DATA_BUS_0, &adc_fifo_addr) != BK_OK) {
			os_printf("get adc fifo address failed\n");
			adc_i2s_ringbuf_start_cleanup();
			return;
		}

		/* DMA1: ADC FIFO -> ringbuf */
		adc_dma_config.mode      = DMA_WORK_MODE_REPEAT;
		adc_dma_config.chan_prio = 1;
		adc_dma_config.src.dev   = DMA_DEV_AUD_MIC0;
		adc_dma_config.dst.dev   = DMA_DEV_DTCM;
		if (adc_bits == 16) {
			adc_dma_config.src.width = DMA_DATA_WIDTH_16BITS;
			adc_dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
		} else {
			adc_dma_config.src.width = DMA_DATA_WIDTH_32BITS;
			adc_dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
		}
		adc_dma_config.src.addr_inc_en  = DMA_ADDR_INC_DISABLE;
		adc_dma_config.src.addr_loop_en = DMA_ADDR_LOOP_DISABLE;
		adc_dma_config.src.start_addr   = adc_fifo_addr;
		adc_dma_config.src.end_addr     = adc_fifo_addr + 4;
		adc_dma_config.dst.addr_inc_en  = DMA_ADDR_INC_ENABLE;
		adc_dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
		adc_dma_config.dst.start_addr   = (uint32_t)adc_i2s_ringbuf_adc_buffer;
		adc_dma_config.dst.end_addr     = (uint32_t)adc_i2s_ringbuf_adc_buffer + adc_i2s_ringbuf_adc_size;

		ret = bk_dma_init(adc_i2s_ringbuf_adc_dma_id, &adc_dma_config);
		if (ret != BK_OK) {
			os_printf("adc dma init failed\n");
			adc_i2s_ringbuf_start_cleanup();
			return;
		}

		adc_i2s_ringbuf_adc_transfer_len = (adc_bits == 16) ? (sample_rate / 1000 * ADC_FRAME_DURATION * 2) : (sample_rate / 1000 * ADC_FRAME_DURATION * 4);
		adc_i2s_ringbuf_adc_transfer_len *= g_dmic_dbg;
		g_transfer_len = adc_i2s_ringbuf_adc_transfer_len;
		bk_dma_set_transfer_len(adc_i2s_ringbuf_adc_dma_id, adc_i2s_ringbuf_adc_transfer_len);
		ring_buffer_init(adc_i2s_ringbuf_adc_rb, adc_i2s_ringbuf_adc_buffer, adc_i2s_ringbuf_adc_size, adc_i2s_ringbuf_adc_dma_id, RB_DMA_TYPE_WRITE);

		bk_dma_register_isr(adc_i2s_ringbuf_adc_dma_id, NULL, (void *)aud_adc_i2s_ringbuf_adc_dma_finish_isr);
		bk_dma_enable_finish_interrupt(adc_i2s_ringbuf_adc_dma_id);
#if (CONFIG_SPE)
		bk_dma_set_dest_sec_attr(adc_i2s_ringbuf_adc_dma_id, DMA_ATTR_SEC);
		bk_dma_set_src_sec_attr(adc_i2s_ringbuf_adc_dma_id, DMA_ATTR_SEC);
#endif
		bk_aud_adc_set_write_threshold(AUD_ADC_MIC_DATA_BUS_0, 3);

		adc_i2s_ringbuf_i2s_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
		if ((adc_i2s_ringbuf_i2s_dma_id < DMA_ID_0) || (adc_i2s_ringbuf_i2s_dma_id >= DMA_ID_MAX)) {
			os_printf("malloc i2s dma fail\n");
			adc_i2s_ringbuf_start_cleanup();
			return;
		}

		if (bk_i2s_get_data_addr(I2S_CHANNEL_1, &i2s_tx_fifo_addr) != BK_OK) {
			os_printf("get i2s tx fifo address failed\n");
			adc_i2s_ringbuf_start_cleanup();
			return;
		}

		switch (gpio_group) {
			case 0: i2s_dma_config.dst.dev = DMA_DEV_I2S0; break;
			case 1: i2s_dma_config.dst.dev = DMA_DEV_I2S1; break;
			case 2: i2s_dma_config.dst.dev = DMA_DEV_I2S2; break;
			default:
				os_printf("Do not match this gpio_group: %d\n", gpio_group);
				adc_i2s_ringbuf_start_cleanup();
				return;
		}

		/* DMA2: ringbuf -> I2S TX FIFO (dst.dev already set in switch above) */
		i2s_dma_config.mode      = DMA_WORK_MODE_REPEAT;
		i2s_dma_config.chan_prio = 1;
		i2s_dma_config.src.dev   = DMA_DEV_DTCM;
		if (adc_bits == 16) {
			i2s_dma_config.src.width = DMA_DATA_WIDTH_16BITS;
			i2s_dma_config.dst.width = DMA_DATA_WIDTH_16BITS;
		} else {
			i2s_dma_config.src.width = DMA_DATA_WIDTH_32BITS;
			i2s_dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
		}
		i2s_dma_config.src.addr_inc_en  = DMA_ADDR_INC_ENABLE;
		i2s_dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
		i2s_dma_config.src.start_addr   = (uint32_t)adc_i2s_ringbuf_i2s_buffer;
		i2s_dma_config.src.end_addr     = (uint32_t)adc_i2s_ringbuf_i2s_buffer + adc_i2s_ringbuf_i2s_size;
		i2s_dma_config.dst.addr_inc_en  = DMA_ADDR_INC_DISABLE;
		i2s_dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_DISABLE;
		i2s_dma_config.dst.start_addr   = i2s_tx_fifo_addr;
		i2s_dma_config.dst.end_addr     = i2s_tx_fifo_addr + 4;

		ret = bk_dma_init(adc_i2s_ringbuf_i2s_dma_id, &i2s_dma_config);
		if (ret != BK_OK) {
			os_printf("i2s dma init failed\n");
			adc_i2s_ringbuf_start_cleanup();
			return;
		}

		adc_i2s_ringbuf_i2s_transfer_len = adc_i2s_ringbuf_adc_transfer_len;
		os_printf("adc_i2s_ringbuf_i2s_transfer_len:%d\r\n", adc_i2s_ringbuf_i2s_transfer_len);
		bk_dma_set_transfer_len(adc_i2s_ringbuf_i2s_dma_id, adc_i2s_ringbuf_i2s_transfer_len);
		// Don't use RB_DMA_TYPE_READ, manually manage read pointer in task (same as single ringbuf version)
		ring_buffer_init(adc_i2s_ringbuf_i2s_rb, adc_i2s_ringbuf_i2s_buffer, adc_i2s_ringbuf_i2s_size, adc_i2s_ringbuf_i2s_dma_id, RB_DMA_TYPE_READ);

		// Allocate transfer buffer for tasks to move data from ADC ringbuf to I2S ringbuf
		adc_i2s_ringbuf_transfer_buf = (uint8_t *)psram_malloc(adc_i2s_ringbuf_i2s_transfer_len);
		if (adc_i2s_ringbuf_transfer_buf == NULL) {
			os_printf("malloc transfer buf fail\n");
			adc_i2s_ringbuf_start_cleanup();
			return;
		}
		
		// Create ADC task and queue for transferring data from ADC ringbuf to I2S ringbuf
		if (adc_i2s_ringbuf_adc_que == NULL) {
			ret = rtos_init_queue(&adc_i2s_ringbuf_adc_que,
				"adc_i2s_ringbuf_adc_que",
				sizeof(adc_i2s_ringbuf_adc_msg_t),
				64);
			if (ret != BK_OK) {
				os_printf("init adc_i2s_ringbuf_adc_que failed\n");
				adc_i2s_ringbuf_start_cleanup();
				return;
			}
		}
		if (adc_i2s_ringbuf_adc_task == NULL) {
			ret = rtos_create_thread(&adc_i2s_ringbuf_adc_task,
				BEKEN_DEFAULT_WORKER_PRIORITY - 1,
				"adc_i2s_ringbuf_adc_task",
				(beken_thread_function_t)aud_adc_i2s_ringbuf_adc_task_entry,
				1024 * 2,
				NULL);
			if (ret != BK_OK) {
				os_printf("create adc_i2s_ringbuf_adc_task failed\n");
				adc_i2s_ringbuf_start_cleanup();
				return;
			}
		}
		
		// Create I2S task and queue for handling I2S DMA completion
		if (adc_i2s_ringbuf_i2s_que == NULL) {
			ret = rtos_init_queue(&adc_i2s_ringbuf_i2s_que,
				"adc_i2s_ringbuf_i2s_que",
				sizeof(adc_i2s_ringbuf_i2s_msg_t),
				64);
			if (ret != BK_OK) {
				os_printf("init adc_i2s_ringbuf_i2s_que failed\n");
				adc_i2s_ringbuf_start_cleanup();
				return;
			}
		}
		if (adc_i2s_ringbuf_i2s_task == NULL) {
			ret = rtos_create_thread(&adc_i2s_ringbuf_i2s_task,
				BEKEN_DEFAULT_WORKER_PRIORITY - 1,
				"adc_i2s_ringbuf_i2s_task",
				(beken_thread_function_t)aud_adc_i2s_ringbuf_i2s_task_entry,
				1024 * 2,
				NULL);
			if (ret != BK_OK) {
				os_printf("create adc_i2s_ringbuf_i2s_task failed\n");
				adc_i2s_ringbuf_start_cleanup();
				return;
			}
		}
		bk_dma_register_isr(adc_i2s_ringbuf_i2s_dma_id, NULL, (void *)aud_adc_i2s_ringbuf_i2s_dma_finish_isr);
		bk_dma_enable_finish_interrupt(adc_i2s_ringbuf_i2s_dma_id);
#if (CONFIG_SPE)
		bk_dma_set_dest_sec_attr(adc_i2s_ringbuf_i2s_dma_id, DMA_ATTR_SEC);
		bk_dma_set_src_sec_attr(adc_i2s_ringbuf_i2s_dma_id, DMA_ATTR_SEC);
#endif

		ret = bk_i2s_enable(I2S_ENABLE);
		if (ret != BK_OK) {
			os_printf("bk_i2s_enable failed: %d\n", ret);
			adc_i2s_ringbuf_start_cleanup();
			return;
		}
		ret = bk_i2s_start();
		if (ret != BK_OK) {
			os_printf("bk_i2s_start failed: %d\n", ret);
			adc_i2s_ringbuf_start_cleanup();
			return;
		}

		ret = bk_dma_start(adc_i2s_ringbuf_i2s_dma_id);
		if (ret != BK_OK) {
			os_printf("i2s dma start failed\n");
			adc_i2s_ringbuf_start_cleanup();
			return;
		}
		ret = bk_dma_start(adc_i2s_ringbuf_adc_dma_id);
		if (ret != BK_OK) {
			os_printf("adc dma start failed\n");
			adc_i2s_ringbuf_start_cleanup();
			return;
		}
		ret = bk_aud_adc_start(AUD_ADC_CHL_0);
		ch_bitmap = 1 << AUD_ADC_CHL_0;
		if (g_dmic_en) {
			if (dmic_config.dmic_mode == AUD_DMIC_MODE_1)
			{
				ret = bk_aud_adc_start(AUD_ADC_CHL_1);
				ret = bk_aud_adc_start(AUD_ADC_CHL_2);
				ch_bitmap |= (1 << AUD_ADC_CHL_1) | (1 << AUD_ADC_CHL_2);
			}
			bk_aud_dmic_init(&dmic_config);
			bk_aud_adc_enable_used_channel(ch_bitmap);
			os_printf("ch_bitmap : 0x%x\r\n", ch_bitmap);
		}
		if (ret != BK_OK) {
			os_printf("bk_aud_adc_start failed: %d\n", ret);
			return;
		}
		os_printf("dma_id:%d-----%d\r\n", adc_i2s_ringbuf_adc_dma_id, adc_i2s_ringbuf_i2s_dma_id);
		g_adc_i2s_ringbuf_test_initialized = true;
		os_printf("ADC -> ringbuf -> I2S out test started (sample_rate=%d, gpio_group=%d)\n", sample_rate, gpio_group);
		return;
	} 
	else if (os_strcmp(argv[1], "dump") == 0) {
		uint8_t dump_mode = strtoul(argv[2], NULL, 10);
		g_adc_i2s_ringbuf_dump_mode = dump_mode;
		os_printf("audio adc i2s ringbuf test dump mode %d\n", g_adc_i2s_ringbuf_dump_mode);
		return;
	}
	else if (os_strcmp(argv[1], "stop") == 0) {
		if (!g_adc_i2s_ringbuf_test_initialized) {
			os_printf("adc i2s ringbuf test not started\n");
			return;
		}
		bk_aud_adc_deinit();
		bk_i2s_stop();
		bk_i2s_enable(I2S_DISABLE);
		if (adc_i2s_ringbuf_adc_dma_id != DMA_ID_MAX) {
			bk_dma_stop(adc_i2s_ringbuf_adc_dma_id);
			bk_dma_deinit(adc_i2s_ringbuf_adc_dma_id);
			bk_dma_free(DMA_DEV_AUDIO, adc_i2s_ringbuf_adc_dma_id);
			adc_i2s_ringbuf_adc_dma_id = DMA_ID_MAX;
		}
		if (adc_i2s_ringbuf_i2s_dma_id != DMA_ID_MAX) {
			bk_dma_stop(adc_i2s_ringbuf_i2s_dma_id);
			bk_dma_deinit(adc_i2s_ringbuf_i2s_dma_id);
			bk_dma_free(DMA_DEV_AUDIO, adc_i2s_ringbuf_i2s_dma_id);
			adc_i2s_ringbuf_i2s_dma_id = DMA_ID_MAX;
		}
		bk_dma_driver_deinit();
		if (adc_i2s_ringbuf_adc_task != NULL) {
			rtos_delete_thread(&adc_i2s_ringbuf_adc_task);
			adc_i2s_ringbuf_adc_task = NULL;
		}
		if (adc_i2s_ringbuf_adc_que != NULL) {
			rtos_deinit_queue(&adc_i2s_ringbuf_adc_que);
			adc_i2s_ringbuf_adc_que = NULL;
		}
		if (adc_i2s_ringbuf_i2s_task != NULL) {
			rtos_delete_thread(&adc_i2s_ringbuf_i2s_task);
			adc_i2s_ringbuf_i2s_task = NULL;
		}
		if (adc_i2s_ringbuf_i2s_que != NULL) {
			rtos_deinit_queue(&adc_i2s_ringbuf_i2s_que);
			adc_i2s_ringbuf_i2s_que = NULL;
		}
		if (adc_i2s_ringbuf_transfer_buf != NULL) {
			psram_free(adc_i2s_ringbuf_transfer_buf);
			adc_i2s_ringbuf_transfer_buf = NULL;
		}
		if (adc_i2s_ringbuf_adc_rb != NULL) {
			psram_free(adc_i2s_ringbuf_adc_rb);
			adc_i2s_ringbuf_adc_rb = NULL;
		}
		if (adc_i2s_ringbuf_adc_buffer != NULL) {
			psram_free(adc_i2s_ringbuf_adc_buffer);
			adc_i2s_ringbuf_adc_buffer = NULL;
		}
		if (adc_i2s_ringbuf_i2s_rb != NULL) {
			psram_free(adc_i2s_ringbuf_i2s_rb);
			adc_i2s_ringbuf_i2s_rb = NULL;
		}
		if (adc_i2s_ringbuf_i2s_buffer != NULL) {
			psram_free(adc_i2s_ringbuf_i2s_buffer);
			adc_i2s_ringbuf_i2s_buffer = NULL;
		}
		bk_i2s_deinit();
		bk_i2s_driver_deinit();
		g_adc_i2s_ringbuf_test_initialized = false;
		g_adc_i2s_ringbuf_dump_mode = 0;

		if (dbg)
		{
			psram_free(dbg);
			dbg = NULL;
		}
		if (dbg_16)
		{
			psram_free(dbg_16);
			dbg_16 = NULL;
		}
		os_printf("ADC -> ringbuf -> I2S out test stop successful\n");
		return;
	} else {
		cli_aud_help();
		return;
	}
}


// I2S RX -> ringbuf -> DAC CLI
static void cli_aud_i2s_dac_ringbuf_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	dma_config_t i2s_dma_config = {0};
	dma_config_t dac_dma_config = {0};
	aud_dac_config_t dac_config = DEFAULT_AUD_DAC_CONFIG();
	i2s_config_t i2s_config = DEFAULT_I2S_CONFIG();
	aud_dac_source_t dac_source = AUD_DAC_SOURCE_A2DP;
	uint32_t i2s_rx_fifo_addr = 0;
	uint32_t dac_fifo_addr = 0;
	uint32_t sample_rate = 0;
	i2s_samp_rate_t i2s_samp_rate = I2S_SAMP_RATE_16000;
	uint32_t gpio_group = I2S_GPIO_GROUP_0;
	uint32_t dac_bits = 16;
	dma_dev_t dac_dma_dev = DMA_DEV_AUDIO;

	if (argc < 2) {
		cli_aud_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		if (g_i2s_dac_ringbuf_test_initialized) {
			os_printf("i2s dac ringbuf test already started\n");
			return;
		}
		if (argc < 4) {
			os_printf("Usage: aud_i2s_dac_ringbuf_test start {a2dp|call|hint} {sample_rate} [gpio_group]\n");
			os_printf("  Example: aud_i2s_dac_ringbuf_test start a2dp 16000 0\n");
			os_printf("  Note: I2S RX -> ringbuf -> DAC\n");
			return;
		}

		if (os_strcmp(argv[2], "a2dp") == 0) {
			dac_source = AUD_DAC_SOURCE_A2DP;
		} else if (os_strcmp(argv[2], "call") == 0) {
			dac_source = AUD_DAC_SOURCE_CALL;
		} else if (os_strcmp(argv[2], "hint") == 0) {
			dac_source = AUD_DAC_SOURCE_HINT;
		} else {
			os_printf("dac source %s not support\n", argv[2]);
			return;
		}

		sample_rate = strtoul(argv[3], NULL, 10);
		if (argc >= 5) {
			gpio_group = strtoul(argv[4], NULL, 10);
		}
		if (argc >= 6) {
			dac_bits = strtoul(argv[5], NULL, 10);
		}

		uint32_t __maybe_unused i2s_data_len = 16;
		i2s_lrcom_store_mode_t i2s_store_mode = I2S_LRCOM_STORE_16R16L;

		if (argc >= 7) {
			i2s_store_mode = strtoul(argv[6], NULL, 10);
		}
		if (argc >= 8) {
			i2s_data_len = strtoul(argv[7], NULL, 10);
		}
		if (dac_bits != 16)
		{
			if (i2s_store_mode == I2S_LRCOM_STORE_16R16L)
			{
				i2s_data_len = 32;
				i2s_store_mode = I2S_LRCOM_STORE_LRLR;
				LOGW("Force i2s_data_len to 32 and i2s_store_mode to I2S_LRCOM_STORE_LRLR\r\n");
			}
		}
		g_dac_bits = dac_bits;
	//	i2s_data_len = dac_bits;
		uint32_t lsb_first_en = 1;
		if (argc >= 9) {
			lsb_first_en = strtoul(argv[8], NULL, 10);
		}
		i2s_config.lsb_first_en = lsb_first_en;
		os_printf("QQQ i2s_config_lsb : %d\r\n", i2s_config.lsb_first_en);
		switch (sample_rate) {
			case 8000 : i2s_samp_rate = I2S_SAMP_RATE_8000;  break;
			case 16000: i2s_samp_rate = I2S_SAMP_RATE_16000; break;
			case 24000: i2s_samp_rate = I2S_SAMP_RATE_24000; break;
			case 32000: i2s_samp_rate = I2S_SAMP_RATE_32000; break;
			case 44100: i2s_samp_rate = I2S_SAMP_RATE_44100; break;
			case 48000: i2s_samp_rate = I2S_SAMP_RATE_48000; break;
			case 96000: i2s_samp_rate = I2S_SAMP_RATE_96000; break;
			default:
				os_printf("I2S not support sample rate: %d\n", sample_rate);
				return;
		}

		ret = bk_i2s_driver_init();
		if (ret != BK_OK) {
			os_printf("bk_i2s_driver_init failed: %d\n", ret);
			return;
		}
		i2s_config.samp_rate   = i2s_samp_rate;
		i2s_config.data_length = i2s_data_len;  //(dac_bits == 16) ? 16 : 32;
		i2s_config.store_mode  = i2s_store_mode;//(dac_bits == 16) ? I2S_LRCOM_STORE_16R16L : I2S_LRCOM_STORE_LRLR;
		i2s_config.role        = I2S_ROLE_MASTER;
		ret = bk_i2s_init(gpio_group, &i2s_config);
		if (ret != BK_OK) {
			os_printf("bk_i2s_init failed: %d\n", ret);
			bk_i2s_driver_deinit();
			return;
		}

		aud_hardware_reset();
		g_i2s_dac_ringbuf_dac_source = dac_source;
		dac_config.bits = dac_bits;
		ret = bk_aud_dac_init(&dac_config);
		if (ret != BK_OK) {
			os_printf("bk_aud_dac_init failed: %d\n", ret);
			bk_i2s_deinit();
			bk_i2s_driver_deinit();
			return;
		}
		bk_aud_dac_set_sample_rate(dac_source, sample_rate);

		ret = bk_dma_driver_init();
		if (ret != BK_OK) {
			os_printf("dma driver init failed\n");
			i2s_dac_ringbuf_start_cleanup();
			return;
		}

		i2s_dac_ringbuf_transfer_len = (dac_bits == 16) ?
			(sample_rate / 1000 * I2S_DAC_FRAME_DURATION * 2) :
			(sample_rate / 1000 * I2S_DAC_FRAME_DURATION * 4);

		i2s_dac_ringbuf_i2s_size = sample_rate / 1000 * I2S_DAC_FRAME_DURATION * (dac_bits == 16 ? 2 : 4) * 6 + I2S_DAC_RINGBUF_SAFE_INTERVAL;
		i2s_dac_ringbuf_i2s_buffer = (uint8_t *)psram_malloc(i2s_dac_ringbuf_i2s_size);
		if (i2s_dac_ringbuf_i2s_buffer == NULL) {
			os_printf("malloc i2s rx ringbuf buffer fail\n");
			i2s_dac_ringbuf_start_cleanup();
			return;
		}
		os_memset(i2s_dac_ringbuf_i2s_buffer, 0, i2s_dac_ringbuf_i2s_size);

		i2s_dac_ringbuf_i2s_rb = (RingBufferContext *)psram_malloc(sizeof(RingBufferContext));
		if (i2s_dac_ringbuf_i2s_rb == NULL) {
			psram_free(i2s_dac_ringbuf_i2s_buffer);
			i2s_dac_ringbuf_i2s_buffer = NULL;
			i2s_dac_ringbuf_start_cleanup();
			return;
		}

		i2s_dac_ringbuf_dac_size = i2s_dac_ringbuf_i2s_size;
		i2s_dac_ringbuf_dac_buffer = (uint8_t *)psram_malloc(i2s_dac_ringbuf_dac_size);
		if (i2s_dac_ringbuf_dac_buffer == NULL) {
			psram_free(i2s_dac_ringbuf_i2s_rb);
			psram_free(i2s_dac_ringbuf_i2s_buffer);
			i2s_dac_ringbuf_i2s_rb = NULL;
			i2s_dac_ringbuf_i2s_buffer = NULL;
			i2s_dac_ringbuf_start_cleanup();
			return;
		}
		os_memset(i2s_dac_ringbuf_dac_buffer, 0, i2s_dac_ringbuf_dac_size);

		i2s_dac_ringbuf_dac_rb = (RingBufferContext *)psram_malloc(sizeof(RingBufferContext));
		if (i2s_dac_ringbuf_dac_rb == NULL) {
			psram_free(i2s_dac_ringbuf_dac_buffer);
			psram_free(i2s_dac_ringbuf_i2s_rb);
			psram_free(i2s_dac_ringbuf_i2s_buffer);
			i2s_dac_ringbuf_dac_buffer = NULL;
			i2s_dac_ringbuf_i2s_rb = NULL;
			i2s_dac_ringbuf_i2s_buffer = NULL;
			i2s_dac_ringbuf_start_cleanup();
			return;
		}

		i2s_dac_ringbuf_transfer_buf = (uint8_t *)psram_malloc(i2s_dac_ringbuf_transfer_len);
		if (i2s_dac_ringbuf_transfer_buf == NULL) {
			psram_free(i2s_dac_ringbuf_dac_rb);
			psram_free(i2s_dac_ringbuf_dac_buffer);
			psram_free(i2s_dac_ringbuf_i2s_rb);
			psram_free(i2s_dac_ringbuf_i2s_buffer);
			i2s_dac_ringbuf_dac_rb = NULL;
			i2s_dac_ringbuf_dac_buffer = NULL;
			i2s_dac_ringbuf_i2s_rb = NULL;
			i2s_dac_ringbuf_i2s_buffer = NULL;
			i2s_dac_ringbuf_start_cleanup();
			return;
		}

		i2s_dac_ringbuf_i2s_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
		if ((i2s_dac_ringbuf_i2s_dma_id < DMA_ID_0) || (i2s_dac_ringbuf_i2s_dma_id >= DMA_ID_MAX)) {
			os_printf("alloc i2s rx dma fail\n");
			i2s_dac_ringbuf_start_cleanup();
			return;
		}

		if (bk_i2s_get_data_addr(I2S_CHANNEL_1, &i2s_rx_fifo_addr) != BK_OK) {
			os_printf("get i2s rx fifo addr failed\n");
			i2s_dac_ringbuf_start_cleanup();
			return;
		}

		i2s_dma_config.mode      = DMA_WORK_MODE_REPEAT;
		i2s_dma_config.chan_prio = 1;
		switch (gpio_group) {
			case 0: i2s_dma_config.src.dev = DMA_DEV_I2S0_RX; break;
			case 1: i2s_dma_config.src.dev = DMA_DEV_I2S1_RX; break;
			case 2: i2s_dma_config.src.dev = DMA_DEV_I2S2_RX; break;
			default:
				os_printf("gpio_group %d not match\n", gpio_group);
				i2s_dac_ringbuf_start_cleanup();
				return;
		}
		i2s_dma_config.dst.dev   = DMA_DEV_DTCM;
		if (dac_bits == 16) {
			i2s_dma_config.src.width = DMA_DATA_WIDTH_16BITS;//DMA_DATA_WIDTH_32BITS;//DMA_DATA_WIDTH_16BITS;
			i2s_dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
		} else {
			i2s_dma_config.src.width = DMA_DATA_WIDTH_32BITS;
			i2s_dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
		}
		i2s_dma_config.src.addr_inc_en  = DMA_ADDR_INC_DISABLE;
		i2s_dma_config.src.addr_loop_en = DMA_ADDR_LOOP_DISABLE;
		i2s_dma_config.src.start_addr   = i2s_rx_fifo_addr;
		i2s_dma_config.src.end_addr     = i2s_rx_fifo_addr + 4;
		i2s_dma_config.dst.addr_inc_en  = DMA_ADDR_INC_ENABLE;
		i2s_dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
		i2s_dma_config.dst.start_addr   = (uint32_t)i2s_dac_ringbuf_i2s_buffer;
		i2s_dma_config.dst.end_addr     = (uint32_t)i2s_dac_ringbuf_i2s_buffer + i2s_dac_ringbuf_i2s_size;

		ret = bk_dma_init(i2s_dac_ringbuf_i2s_dma_id, &i2s_dma_config);
		if (ret != BK_OK) {
			os_printf("i2s rx dma init failed\n");
			i2s_dac_ringbuf_start_cleanup();
			return;
		}
		bk_dma_set_transfer_len(i2s_dac_ringbuf_i2s_dma_id, i2s_dac_ringbuf_transfer_len);
		ring_buffer_init(i2s_dac_ringbuf_i2s_rb, i2s_dac_ringbuf_i2s_buffer, i2s_dac_ringbuf_i2s_size, i2s_dac_ringbuf_i2s_dma_id, RB_DMA_TYPE_WRITE);
		bk_dma_register_isr(i2s_dac_ringbuf_i2s_dma_id, NULL, (void *)aud_i2s_dac_ringbuf_i2s_dma_finish_isr);
		bk_dma_enable_finish_interrupt(i2s_dac_ringbuf_i2s_dma_id);
#if (CONFIG_SPE)
		bk_dma_set_dest_sec_attr(i2s_dac_ringbuf_i2s_dma_id, DMA_ATTR_SEC);
		bk_dma_set_src_sec_attr(i2s_dac_ringbuf_i2s_dma_id, DMA_ATTR_SEC);
#endif

		if (!get_dac_fifo_addr_by_source(dac_source, &dac_dma_dev, &dac_fifo_addr)) {
			os_printf("get dac fifo addr failed\n");
			i2s_dac_ringbuf_start_cleanup();
			return;
		}

		i2s_dac_ringbuf_dac_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
		if ((i2s_dac_ringbuf_dac_dma_id < DMA_ID_0) || (i2s_dac_ringbuf_dac_dma_id >= DMA_ID_MAX)) {
			os_printf("alloc dac dma fail\n");
			i2s_dac_ringbuf_start_cleanup();
			return;
		}

		dac_dma_config.mode      = DMA_WORK_MODE_REPEAT;
		dac_dma_config.chan_prio = 1;
		dac_dma_config.src.dev   = DMA_DEV_DTCM;
		dac_dma_config.dst.dev   = dac_dma_dev;
		if (dac_bits == 16) {
			dac_dma_config.src.width = DMA_DATA_WIDTH_32BITS;
			dac_dma_config.dst.width = DMA_DATA_WIDTH_16BITS;
		} else {
			dac_dma_config.src.width = DMA_DATA_WIDTH_32BITS;
			dac_dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
		}
		dac_dma_config.src.addr_inc_en  = DMA_ADDR_INC_ENABLE;
		dac_dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
		dac_dma_config.src.start_addr   = (uint32_t)i2s_dac_ringbuf_dac_buffer;
		dac_dma_config.src.end_addr     = (uint32_t)i2s_dac_ringbuf_dac_buffer + i2s_dac_ringbuf_dac_size;
		dac_dma_config.dst.addr_inc_en  = DMA_ADDR_INC_DISABLE;
		dac_dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_DISABLE;
		dac_dma_config.dst.start_addr   = dac_fifo_addr;
		dac_dma_config.dst.end_addr     = dac_fifo_addr + 4;

		ret = bk_dma_init(i2s_dac_ringbuf_dac_dma_id, &dac_dma_config);
		if (ret != BK_OK) {
			os_printf("dac dma init failed\n");
			i2s_dac_ringbuf_start_cleanup();
			return;
		}

		bk_dma_set_transfer_len(i2s_dac_ringbuf_dac_dma_id, i2s_dac_ringbuf_transfer_len);
		ring_buffer_init(i2s_dac_ringbuf_dac_rb, i2s_dac_ringbuf_dac_buffer, i2s_dac_ringbuf_dac_size, i2s_dac_ringbuf_dac_dma_id, RB_DMA_TYPE_READ);

		ring_buffer_write(i2s_dac_ringbuf_dac_rb, i2s_dac_ringbuf_transfer_buf, i2s_dac_ringbuf_transfer_len);
		ring_buffer_write(i2s_dac_ringbuf_dac_rb, i2s_dac_ringbuf_transfer_buf, i2s_dac_ringbuf_transfer_len);

		bk_dma_register_isr(i2s_dac_ringbuf_dac_dma_id, NULL, (void *)aud_i2s_dac_ringbuf_dac_dma_finish_isr);
		bk_dma_enable_finish_interrupt(i2s_dac_ringbuf_dac_dma_id);
#if (CONFIG_SPE)
		bk_dma_set_dest_sec_attr(i2s_dac_ringbuf_dac_dma_id, DMA_ATTR_SEC);
		bk_dma_set_src_sec_attr(i2s_dac_ringbuf_dac_dma_id, DMA_ATTR_SEC);
#endif

		if (i2s_dac_ringbuf_i2s_que == NULL) {
			ret = rtos_init_queue(&i2s_dac_ringbuf_i2s_que, "i2s_dac_ringbuf_i2s_que", sizeof(i2s_dac_ringbuf_i2s_msg_t), 64);
			if (ret != BK_OK) {
				os_printf("init i2s_dac_ringbuf_i2s_que failed\n");
				i2s_dac_ringbuf_start_cleanup();
				return;
			}
		}
		if (i2s_dac_ringbuf_i2s_task == NULL) {
			ret = rtos_create_thread(&i2s_dac_ringbuf_i2s_task, BEKEN_DEFAULT_WORKER_PRIORITY - 1,
				"i2s_dac_ringbuf_i2s_task", (beken_thread_function_t)aud_i2s_dac_ringbuf_i2s_task_entry, 1024 * 2, NULL);
			if (ret != BK_OK) {
				os_printf("create i2s_dac_ringbuf_i2s_task failed\n");
				i2s_dac_ringbuf_start_cleanup();
				return;
			}
		}
		if (i2s_dac_ringbuf_dac_que == NULL) {
			ret = rtos_init_queue(&i2s_dac_ringbuf_dac_que, "i2s_dac_ringbuf_dac_que", sizeof(i2s_dac_ringbuf_dac_msg_t), 64);
			if (ret != BK_OK) {
				os_printf("init i2s_dac_ringbuf_dac_que failed\n");
				i2s_dac_ringbuf_start_cleanup();
				return;
			}
		}
		if (i2s_dac_ringbuf_dac_task == NULL) {
			ret = rtos_create_thread(&i2s_dac_ringbuf_dac_task, BEKEN_DEFAULT_WORKER_PRIORITY - 1,
				"i2s_dac_ringbuf_dac_task", (beken_thread_function_t)aud_i2s_dac_ringbuf_dac_task_entry, 1024 * 2, NULL);
			if (ret != BK_OK) {
				os_printf("create i2s_dac_ringbuf_dac_task failed\n");
				i2s_dac_ringbuf_start_cleanup();
				return;
			}
		}

		bk_aud_dac_spk0_source_enable(dac_source, 1);
		ret = bk_i2s_enable(I2S_ENABLE);
		if (ret != BK_OK) {
			os_printf("bk_i2s_enable failed: %d\n", ret);
			i2s_dac_ringbuf_start_cleanup();
			return;
		}
		ret = bk_i2s_start();
		if (ret != BK_OK) {
			os_printf("bk_i2s_start failed: %d\n", ret);
			i2s_dac_ringbuf_start_cleanup();
			return;
		}
		if (i2s_config.role == I2S_ROLE_MASTER) {
			*((uint32_t *)(i2s_rx_fifo_addr)) = 0x11;
		}
		ret = bk_aud_dac_start(AUD_DAC_CHL_LR);
		if (ret != BK_OK) {
			os_printf("bk_aud_dac_start failed: %d\n", ret);
			i2s_dac_ringbuf_start_cleanup();
			return;
		}
		ret = bk_dma_start(i2s_dac_ringbuf_dac_dma_id);
		if (ret != BK_OK) {
			os_printf("dac dma start failed\n");
			i2s_dac_ringbuf_start_cleanup();
			return;
		}
		ret = bk_dma_start(i2s_dac_ringbuf_i2s_dma_id);
		if (ret != BK_OK) {
			os_printf("i2s rx dma start failed\n");
			i2s_dac_ringbuf_start_cleanup();
			return;
		}

		g_i2s_dac_ringbuf_test_initialized = true;
		os_printf("i2s_dac_ringbuf_transfer_len:%d\r\n", i2s_dac_ringbuf_transfer_len);
		os_printf("I2S RX -> ringbuf -> DAC test started (sample_rate=%d, gpio_group=%d)\n", sample_rate, gpio_group);
		return;
	} 
	else if (os_strcmp(argv[1], "dump") == 0) {

		g_i2s_dac_ringbuf_dump_mode = strtoul(argv[2], NULL, 10);
		return;
	}
	else if (os_strcmp(argv[1], "stop") == 0) {
		if (!g_i2s_dac_ringbuf_test_initialized) {
			os_printf("i2s dac ringbuf test not started\n");
			return;
		}
		bk_aud_dac_stop(AUD_DAC_CHL_LR);
		bk_aud_dac_spk0_source_enable(g_i2s_dac_ringbuf_dac_source, 0);
		i2s_dac_ringbuf_start_cleanup();
		g_i2s_dac_ringbuf_test_initialized = false;

		if (p_dac)
		{
			psram_free(p_dac);
			p_dac = NULL;
		}

		os_printf("I2S RX -> ringbuf -> DAC test stop successful\n");
		return;
	} else {
		cli_aud_help();
		return;
	}
}




#endif

#endif

#if 1
static uint8_t g_adc_mcp_test_initialized = 0;
static void cli_aud_adc_mcp_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	aud_adc_config_t adc_config = DEFAULT_AUD_ADC_CONFIG();

	if (argc < 2) {
		cli_aud_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {

		adc_config.sample_rate = strtoul(argv[2], NULL, 10);

		uint16_t adc_bits = 16;
		if (argv[3]) {
			adc_bits = strtoul(argv[3], NULL, 10);
		}

		aud_adc_chl_t adc_chnl = AUD_ADC_CHL_0;
		os_printf("adc_bits:%d\n", adc_bits);
		adc_config.chl_cfg[adc_chnl].bits = adc_bits;
	//	if (argc > 3) {
	//		adc_chnl = (aud_adc_chl_t)strtoul(argv[3], NULL, 10);
	//	}
		aud_hardware_reset();
		ret = bk_aud_adc_init(&adc_config);
		if (ret != BK_OK) {
			os_printf("bk_aud_adc_init fail \n");
			return;
		}
		os_printf("init audio dac successful\n");

		//enable audio interrupt
		bk_aud_adc_set_write_threshold(AUD_ADC_MIC_DATA_BUS_0, 3);

		/* start adc */
		bk_aud_adc_start(adc_chnl);
		bk_aud_adc_enable_used_channel(1<<adc_chnl);
		g_adc_mcp_test_initialized = 1;
		{
			uint32_t adc_fifo_data = 0;
			uint32_t adc_fifo_status = 0;
			rtos_enter_critical();
			while(1)
			{
				bk_aud_adc_get_fifo_status(&adc_fifo_status);
				if(adc_fifo_status & AUD_ADC_MIC0_FIFO_ALMOST_FULL_MASK)
				{
					if (adc_bits == 24)
					{
						do {
							bk_aud_adc_get_fifo_data(AUD_ADC_MIC_DATA_BUS_0, &adc_fifo_data);
							uint32_t value = (uint32_t)(adc_fifo_data & 0xFFFFFF);
							bk_uart_write_bytes(1, (void *)&value, sizeof(uint32_t));
							bk_aud_adc_get_fifo_status(&adc_fifo_status);
						} while((adc_fifo_status & AUD_ADC_MIC0_FIFO_ALMOST_FULL_MASK));
					} else if (adc_bits == 16)
					{
						do {
							bk_aud_adc_get_fifo_data(AUD_ADC_MIC_DATA_BUS_0, &adc_fifo_data);
							uint16_t value = (uint16_t)(adc_fifo_data & 0xFFFF);
							bk_uart_write_bytes(1, (void *)&value, sizeof(uint16_t));
							bk_aud_adc_get_fifo_status(&adc_fifo_status);
						} while((adc_fifo_status & AUD_ADC_MIC0_FIFO_ALMOST_FULL_MASK));
					}
				}
			}
		}
	}
	else if (os_strcmp(argv[1], "stop") == 0) {
		if (!g_adc_mcp_test_initialized) {
			os_printf("adc mcp test not started\n");
			return;
		}
		bk_aud_adc_deinit();
		g_adc_mcp_test_initialized = 0;
		os_printf("audio adc mcp test stop successful\n");
	} else {
		cli_aud_help();
		return;
	}
}
#endif

#if 1
static uint8_t g_adc_mcp_dma_test_initialized = 0;
static beken_thread_t adc_mcp_dma_thread = NULL;
static uint8_t g_adc_mcp_dma_dump_flag = 0;
static uint8_t *transfer_buf = NULL;

typedef enum {
	TRANSFER_BUF_DONE_MSG_DONE     = 0,
	TRANSFER_BUF_DONE_MSG_NOT_DONE = 1,
} transfer_buf_done_msg_t;

bk_err_t transfer_buf_done_send_msg(transfer_buf_done_msg_t done)
{
	bk_err_t ret = BK_OK;
	if (transfer_buf_done_que) {
		ret = rtos_push_to_queue(&transfer_buf_done_que, &done, BEKEN_NO_WAIT);
		if (BK_OK != ret) {
			os_printf("transfer_buf_done_send_msg failed, ret: %d\r\n", ret);
			return BK_FAIL;
		}
	} else {
		os_printf("transfer_buf_done_que is NULL\r\n");
		ret = BK_FAIL;
	}
	return ret;
}


static void aud_adc_mcp_test_dma_cmd_dump_thread(beken_thread_arg_t data)
{
	bk_err_t ret = BK_OK;
	if (transfer_buf == NULL && dma_transfer_len != 0)
	{
		transfer_buf = (uint8_t *)malloc(sizeof(uint8_t) * dma_transfer_len);
		if (transfer_buf == NULL)
			os_printf("%s, %d malloc fail\r\n", __func__, __LINE__);
	}
	os_printf("transfer_buf: 0x%x, dma_transfer_len: %d\r\n", transfer_buf, dma_transfer_len);
	while(1)
	{
		transfer_buf_done_msg_t done;
		ret = rtos_pop_from_queue(&transfer_buf_done_que, &done, BEKEN_WAIT_FOREVER);
		if (BK_OK == ret) {
			switch (done) {
				case TRANSFER_BUF_DONE_MSG_DONE:
					if (transfer_buf != NULL && dma_transfer_len > 0 && adc_ringbuf_rb != NULL) {
						uint32_t adc_fill_size = ring_buffer_get_fill_size(adc_ringbuf_rb);
						if (adc_fill_size >= dma_transfer_len) {
							uint32_t read_size = ring_buffer_read(adc_ringbuf_rb, (uint8_t *)transfer_buf, dma_transfer_len);
							if (read_size == dma_transfer_len) {
								if (g_adc_mcp_dma_dump_flag) {
									bk_uart_write_bytes(1, (void *)transfer_buf, dma_transfer_len);
								}
							} else {
								//os_printf("the data read from adc_ringbuf_rb error, size: %d, expected: %d\n", read_size, dma_transfer_len);
							}
						}
					}
					break;
				case TRANSFER_BUF_DONE_MSG_NOT_DONE:
					// Error case, skip this message
					break;
				default:
					os_printf("invalid done message: %d\r\n", done);
					break;
			}
		}
	}
}

static void aud_adc_mcp_test_dma_cmd_isr(dma_id_t dma_id)
{
	GPIO_DOWN(36);GPIO_UP(36);
	static uint32_t adc_dma_count = 0;
	adc_dma_count++;
	if (adc_dma_count <= 3) {
		LOGD("[ADC-DMA] isr #%d triggered, size:%d\n", adc_dma_count, dma_transfer_len);  // for debug
	}

	if (adc_ringbuf_rb == NULL || dma_id != adc_ringbuf_adc_dma_id) {
		return;
	}

	if (transfer_buf != NULL && dma_transfer_len > 0)
	{
		transfer_buf_done_send_msg(TRANSFER_BUF_DONE_MSG_DONE);
	} else
	{
		os_printf("isr transfer_buf is 0x%x or dma_transfer_len %d\n", transfer_buf, dma_transfer_len);
	}

	GPIO_DOWN(36);
}

static void cli_aud_adc_mcp_test_dma_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	uint32_t adc_fifo_addr = 0;
	dma_config_t adc_dma_config = {0};
	aud_adc_config_t adc_config = DEFAULT_AUD_ADC_CONFIG();

	if (argc < 2) {
		cli_aud_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {

		adc_config.sample_rate = strtoul(argv[2], NULL, 10);
		os_printf("audio adc mcp test start adc_config.sample_rate:%d\n", adc_config.sample_rate);

		aud_hardware_reset();
		ret = bk_aud_adc_init(&adc_config);
		if (ret != BK_OK) {
			os_printf("bk_aud_adc_init fail \n");
			return;
		}

		//enable audio interrupt
		bk_aud_adc_set_write_threshold(AUD_ADC_MIC_DATA_BUS_0, 3);
		bk_aud_adc_set_bits_width(AUD_ADC_CHL_0, 16);

		// config dma
		adc_ringbuf_size = adc_config.sample_rate / 1000 * 20 * 2 * 20 + ADC_RINGBUF_SAFE_INTERVAL;
		adc_ringbuf_buffer = (int32_t *)os_malloc(adc_ringbuf_size);
		if (adc_ringbuf_buffer == NULL) {
			os_printf("Failed to allocate ring buffer\n");
			goto adc_deinit;
		}
		os_memset(adc_ringbuf_buffer, 0, adc_ringbuf_size);

		// Initialize ADC ring buffer context
		adc_ringbuf_rb = (RingBufferContext *)os_malloc(sizeof(RingBufferContext));
		if (adc_ringbuf_rb == NULL) {
			os_printf("Failed to allocate ADC ring buffer context\n");
			os_free(adc_ringbuf_buffer);
			adc_ringbuf_buffer = NULL;
			goto adc_deinit;
		}

		//init dma driver
		ret = bk_dma_driver_init();
		if (ret != BK_OK) {
			os_printf("dma driver init failed\r\n");
			goto adc_free;
		}

		// Allocate ADC DMA channel
		adc_ringbuf_adc_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
		if ((adc_ringbuf_adc_dma_id < DMA_ID_0) || (adc_ringbuf_adc_dma_id >= DMA_ID_MAX)) {
			os_printf("malloc adc dma fail \r\n");
			goto adc_free;
		}

		// ========== DMA1: ADC FIFO to Ring Buffer ==========
		// Configure DMA1: ADC FIFO -> Ring Buffer
		adc_dma_config.mode      = DMA_WORK_MODE_REPEAT;
		adc_dma_config.chan_prio = 1;
		adc_dma_config.src.dev   = DMA_DEV_AUD_MIC0;
		adc_dma_config.src.width = DMA_DATA_WIDTH_16BITS;
		adc_dma_config.dst.dev   = DMA_DEV_DTCM;
		adc_dma_config.dst.width = DMA_DATA_WIDTH_32BITS;

		// Get ADC FIFO address
		if (bk_aud_adc_get_fifo_addr(AUD_ADC_MIC_DATA_BUS_0, &adc_fifo_addr) != BK_OK) {
			os_printf("get adc fifo address failed\r\n");
			goto adc_dma_free;
		}

		adc_dma_config.src.addr_inc_en  = DMA_ADDR_INC_ENABLE;
		adc_dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
		adc_dma_config.src.start_addr   = adc_fifo_addr;
		adc_dma_config.src.end_addr     = adc_fifo_addr + 4;

		adc_dma_config.dst.addr_inc_en  = DMA_ADDR_INC_ENABLE;
		adc_dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
		adc_dma_config.dst.start_addr   = (uint32_t)adc_ringbuf_buffer;
		adc_dma_config.dst.end_addr     = (uint32_t)adc_ringbuf_buffer + adc_ringbuf_size;

		ret = bk_dma_init(adc_ringbuf_adc_dma_id, &adc_dma_config);
		if (ret != BK_OK) {
			os_printf("adc dma init failed\r\n");
			goto adc_dma_free;
		}

		dma_transfer_len = adc_config.sample_rate / 1000 * 20 * 2; // 1channel, 20ms, 2bytes per sample (640 bytes)
		bk_dma_set_transfer_len(adc_ringbuf_adc_dma_id, dma_transfer_len);

		ring_buffer_init(adc_ringbuf_rb, (uint8_t*)adc_ringbuf_buffer, adc_ringbuf_size, adc_ringbuf_adc_dma_id, RB_DMA_TYPE_WRITE);

		bk_dma_register_isr(adc_ringbuf_adc_dma_id, NULL, (void *)aud_adc_mcp_test_dma_cmd_isr);
		bk_dma_enable_finish_interrupt(adc_ringbuf_adc_dma_id);

#if (CONFIG_SPE)
		bk_dma_set_dest_sec_attr(adc_ringbuf_adc_dma_id, DMA_ATTR_SEC);
		bk_dma_set_src_sec_attr(adc_ringbuf_adc_dma_id, DMA_ATTR_SEC);
#endif

		if (transfer_buf_done_que == NULL) {
			ret = rtos_init_queue(&transfer_buf_done_que,
				"transfer_buf_done_que",
				sizeof(transfer_buf_done_msg_t),
				200);
			if (ret != BK_OK) {
				os_printf("init transfer_buf_done_que failed\r\n");
				return;
			}
		}
		if (adc_mcp_dma_thread == NULL) {
			ret = rtos_create_thread(&adc_mcp_dma_thread,
										BEKEN_DEFAULT_WORKER_PRIORITY - 1,
										"adc_mcp_dma_thread",
										(beken_thread_function_t)aud_adc_mcp_test_dma_cmd_dump_thread,
										1024 * 4,
										NULL);
			if (ret != BK_OK)
			{
				os_printf("%s, init thread failed\r\n", __func__);
				return;
			}
		}

		// Start ADC DMA (DMA directly transfers data from ADC FIFO to ring buffer)
		ret = bk_dma_start(adc_ringbuf_adc_dma_id);
		if (ret != BK_OK) {
			os_printf("adc dma start fail \n");
			goto adc_dma_deinit;
		}

		/* start adc */
		bk_aud_adc_start(AUD_ADC_CHL_0);
        bk_aud_adc_enable_used_channel(1<<AUD_ADC_CHL_0);

		os_printf("start audio adc mcp test successful\r\n");
		g_adc_mcp_dma_test_initialized = 1;
		return;
	}
	else if (os_strcmp(argv[1], "dump") == 0) {
		g_adc_mcp_dma_dump_flag = strtoul(argv[2], NULL, 10);
		os_printf("audio adc mcp test dump %d\n", g_adc_mcp_dma_dump_flag);
		return;
	}
	else if (os_strcmp(argv[1], "stop") == 0) {

		if (!g_adc_mcp_dma_test_initialized) {
			os_printf("adc mcp dma test not started\n");
			return;
		}

		if (adc_ringbuf_adc_dma_id != DMA_ID_MAX) {
			bk_dma_stop(adc_ringbuf_adc_dma_id);
			bk_dma_deinit(adc_ringbuf_adc_dma_id);
			bk_dma_free(DMA_DEV_AUDIO, adc_ringbuf_adc_dma_id);
			adc_ringbuf_adc_dma_id = DMA_ID_MAX;
		}

		if (adc_ringbuf_rb != NULL) {
			os_free(adc_ringbuf_rb);
			adc_ringbuf_rb = NULL;
		}
		if (adc_ringbuf_buffer != NULL) {
			os_free(adc_ringbuf_buffer);
			adc_ringbuf_buffer = NULL;
		}

		if (transfer_buf != NULL) {
			psram_free(transfer_buf);
			transfer_buf = NULL;
		}
		dma_transfer_len = 0;

		bk_aud_adc_deinit();

		rtos_delete_thread(&adc_mcp_dma_thread);
		adc_mcp_dma_thread = NULL;
		rtos_deinit_queue(&transfer_buf_done_que);
		transfer_buf_done_que = NULL;
		g_adc_mcp_dma_dump_flag = 0;
		g_adc_mcp_dma_test_initialized = 0;
		os_printf("audio adc mcp test stop successful\n");
		return;
	} else {
		cli_aud_help();
		return;
	}

adc_dma_deinit:
	bk_dma_deinit(adc_ringbuf_adc_dma_id);
adc_dma_free:
	bk_dma_free(DMA_DEV_AUD_MIC0, adc_ringbuf_adc_dma_id);
	adc_ringbuf_adc_dma_id = DMA_ID_MAX;
adc_free:
	os_free(adc_ringbuf_rb);
	os_free(adc_ringbuf_buffer);
	adc_ringbuf_rb = NULL;
	adc_ringbuf_buffer = NULL;
adc_deinit:
	bk_aud_adc_deinit();
}
#endif

static void cli_aud_adc_loop_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    os_printf("[+]cli_aud_adc_loop_test_cmd, argc:%d\n", argc);
    bk_err_t ret = BK_OK;
    aud_adc_config_t adc_config = DEFAULT_AUD_ADC_CONFIG();
    aud_dac_config_t dac_config = DEFAULT_AUD_DAC_CONFIG();

    if (argc != 2 && argc != 3) {
        cli_aud_help();
        return;
    }

    if (os_strcmp(argv[1], "start") == 0)
    {
        os_printf("audio adc loop test start\n");
        adc_config.sample_rate = strtoul(argv[2], NULL, 10);
        //uint32_t dac_sample_rate = adc_config.sample_rate;

        //init audio adc and dac driver
        ret = bk_aud_dac_init(&dac_config);
        if (ret != BK_OK) {
            os_printf("bk_aud_dac_init fail \n");
            return;
        }
        os_printf("init audio dac successful\n");

        ret = bk_aud_adc_init(&adc_config);
        if (ret != BK_OK) {
            os_printf("bk_aud_adc_init fail \n");
            return;
        }
        os_printf("init audio adc successful\n");

        //start adc
        ret = bk_aud_adc_start(AUD_ADC_CHL_0);
        if (ret != BK_OK) {
            os_printf("bk_aud_adc_start fail \n");
            return;
        }
        bk_aud_adc_enable_used_channel(1<<AUD_ADC_CHL_0);

        //start dac
        ret = bk_aud_dac_start(AUD_DAC_CHL_L);
        if (ret != BK_OK) {
            os_printf("bk_aud_dac_start fail \n");
            return;
        }

        //enable adc to dac loop test
        ret = bk_aud_adc_start_loop_test();
        if (ret != BK_OK) {
            return;
        }
        os_printf("enable adc to dac loop test successful\n");
    } 
    else if (os_strcmp(argv[1], "stop") == 0) {
        os_printf("audio adc to dac loop test stop\n");
        //stop adc to dac loop test
        ret = bk_aud_adc_stop_loop_test();
        if (ret != BK_OK)
            return;

        //disable adc and dac
        bk_aud_adc_stop(AUD_ADC_CHL_0);
        bk_aud_dac_stop(AUD_DAC_CHL_L);

        //deinit adc and dac
        bk_aud_adc_deinit();
        bk_aud_dac_deinit();
        os_printf("audio adc to dac loop test stop successful\n");
    } else {
        cli_aud_help();
        return;
    }
}

#endif	//#if CONFIG_AUDIO_ADC

// DMA finish ISR: fill ring buffer from audio source
static void aud_dac_dma_ringbuf_test_finish_isr(dma_id_t dma_id)
{
	GPIO_DOWN(36);GPIO_UP(36);
	uint32_t write_size = 0;
	uint32_t free_size = 0;
	uint32_t total_bytes = dac_dma_aud_len * 4;  // Total audio data size in bytes

	if (dac_dma_rb == NULL || dac_dma_aud_ptr == NULL || dma_id != dac_dma_id) {
		return;
	}

#if CONFIG_CACHE_ENABLE
	flush_all_dcache();
#endif

	// Check free space in ring buffer
	free_size = ring_buffer_get_free_size(dac_dma_rb);
	if (free_size < total_bytes) {
		// Not enough space, skip this time
		LOGE("Ring buffer full, skip fill (free=%d, need=%d)\n", free_size, total_bytes);
		return;
	}

	// Write entire audio array to ring buffer
	write_size = ring_buffer_write(dac_dma_rb, (uint8_t *)dac_dma_aud_ptr, total_bytes);

	if (write_size > 0) {
		// Data written successfully, will loop automatically when DMA finishes transfer
	} else {
		LOGE("[DMA ISR] write failed, write_size=0\n");
	}
	GPIO_DOWN(36);
}

static void cli_aud_dac_dma_ringbuf_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_err_t ret = BK_OK;
    aud_dac_config_t dac_config = DEFAULT_AUD_DAC_CONFIG();
    dma_config_t dma_config = {0};
    uint32_t dac_fifo_addr;
    uint32_t *aud_ptr = NULL;
    uint32_t aud_len = 0;
    aud_dac_source_t dac_source = AUD_DAC_SOURCE_A2DP;
    uint32_t dac_fifo_status = 0;
    uint32_t ring_buff_size = 0;
	uint32_t dac_bits = 16;

    if (argc != 2 && argc != 4 && argc != 5) {
		goto exit;
    }

    if (os_strcmp(argv[1], "start") == 0)
    {
        if (!parse_dac_source(argv[2], &dac_source)) {
            LOGE("dac source type: %s is not support\n", argv[2]);
			goto exit;
        }
        uint32_t dac_sample_rate = strtoul(argv[3], NULL, 10);
        switch (dac_source)
        {
            case AUD_DAC_SOURCE_A2DP:
                if (dac_sample_rate != 44100 && dac_sample_rate != 48000) {
                    LOGI("dac a2dp source not support sample rate: %d\n", dac_sample_rate);
					goto exit;
                }
                break;
            case AUD_DAC_SOURCE_CALL:
            case AUD_DAC_SOURCE_HINT:
                if (dac_sample_rate != 8000 && dac_sample_rate != 16000 && dac_sample_rate != 48000) {
                    LOGI("dac a2dp source not support sample rate: %d\n", dac_sample_rate);
					goto exit;
                }
                break;
            default:
                break;
        }

        if (argv[4])
            dac_bits = strtoul(argv[4], NULL, 10);

		if (dac_bits == 16) {
			if (!get_dac_test_data_by_rate(dac_sample_rate, (const uint32_t **)&aud_ptr, &aud_len)) {
				LOGE("unsupport pcm test sample rate: %d\n", dac_sample_rate);
				goto exit;
			}
		} else if (dac_bits == 24 && signal == NULL) {
			LOGI("Use the aud_generate_pcm_test to generate pcm 24bits test data\n");
			return;
		}

        if (signal != NULL) {
            aud_ptr = (uint32_t *)signal;
            aud_len = signal_size;
        }
        LOGI("audio dac test start, aud_len: %d, dac_bits:%d\n", aud_len, dac_bits);

        aud_hardware_reset();
		dac_config.bits = dac_bits;
        ret = bk_aud_dac_init(&dac_config);
        if (ret != BK_OK) {
            LOGE("bk_aud_dac_init fail \n");
            return;
        }
        LOGI("init audio driver and dac successful\n");

        bk_aud_dac_set_sample_rate(dac_source, dac_sample_rate);
        //bk_aud_dac_set_bits_width(dac_source, 16);

        // Ring buffer size: entire audio array size (aud_len * 4 bytes) * 2 + safe interval
        uint32_t aud_bytes = aud_len * 4;
        ring_buff_size = aud_bytes * 2 + DAC_DMA_RING_BUFF_SAFE_INTERVAL;

        // Allocate ring buffer for DMA
        dac_dma_ring_buff = (int32_t *)os_malloc(ring_buff_size);
        if (dac_dma_ring_buff == NULL) {
            LOGE("Failed to allocate DMA ring buffer\n");
            bk_aud_dac_deinit();
            return;
        }
        os_memset(dac_dma_ring_buff, 0, ring_buff_size);

        // Initialize ring buffer context
        dac_dma_rb = (RingBufferContext *)os_malloc(sizeof(RingBufferContext));
        if (dac_dma_rb == NULL) {
            LOGE("Failed to allocate ring buffer context\n");
            os_free(dac_dma_ring_buff);
            dac_dma_ring_buff = NULL;
            bk_aud_dac_deinit();
            return;
        }

        //init dma driver
        ret = bk_dma_driver_init();
        if (ret != BK_OK) {
            LOGE("dma driver init failed\r\n");
            os_free(dac_dma_rb);
            os_free(dac_dma_ring_buff);
            dac_dma_rb = NULL;
            dac_dma_ring_buff = NULL;
            bk_aud_dac_deinit();
            return;
        }

        // Allocate DMA channel
        dac_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
        if ((dac_dma_id < DMA_ID_0) || (dac_dma_id >= DMA_ID_MAX)) {
            LOGE("malloc dma fail \r\n");
            os_free(dac_dma_rb);
            os_free(dac_dma_ring_buff);
            dac_dma_rb = NULL;
            dac_dma_ring_buff = NULL;
            bk_aud_dac_deinit();
            return;
        }
        LOGI("dma_id: %d\n", dac_dma_id);

        // Configure DMA (similar to aud_tras_dac_dma_config)
        os_memset(&dma_config, 0, sizeof(dma_config));
        dma_config.mode      = DMA_WORK_MODE_REPEAT;
        dma_config.chan_prio = 1;
        dma_config.src.dev   = DMA_DEV_DTCM;
        dma_config.src.width = DMA_DATA_WIDTH_32BITS;
        dma_config.dst.dev   = DMA_DEV_AUDIO;  // Use DMA_DEV_AUDIO as reference implementation

        // Get DAC FIFO address based on source
        if (!get_dac_fifo_addr_by_source(dac_source, &dma_config.dst.dev, &dac_fifo_addr)) {
            LOGE("get dac fifo address failed\r\n");
            os_free(dac_dma_rb);
            os_free(dac_dma_ring_buff);
            dac_dma_rb = NULL;
            dac_dma_ring_buff = NULL;
            bk_aud_dac_deinit();
            return;
        }
        dma_config.dst.width        = DMA_DATA_WIDTH_32BITS;
        dma_config.dst.addr_inc_en  = DMA_ADDR_INC_ENABLE;
        dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
        dma_config.dst.start_addr   = dac_fifo_addr;
        dma_config.dst.end_addr     = dac_fifo_addr + 4;

        // Source is ring buffer
        dma_config.src.addr_inc_en  = DMA_ADDR_INC_ENABLE;
        dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
        dma_config.src.start_addr   = (uint32_t)dac_dma_ring_buff;
        dma_config.src.end_addr     = (uint32_t)dac_dma_ring_buff + ring_buff_size;

        LOGI("DMA config: src=0x%x~0x%x, dst=0x%x, transfer_len=%d, ring_buff_size=%d\r\n", 
             dma_config.src.start_addr, dma_config.src.end_addr, dma_config.dst.start_addr, aud_len * 4, ring_buff_size);

        // Initialize DMA channel
        ret = bk_dma_init(dac_dma_id, &dma_config);
        if (ret != BK_OK) {
            LOGE("dma init failed\r\n");
            os_free(dac_dma_rb);
            os_free(dac_dma_ring_buff);
            dac_dma_rb = NULL;
            dac_dma_ring_buff = NULL;
            bk_dma_free(DMA_DEV_AUDIO, dac_dma_id);
            dac_dma_id = DMA_ID_MAX;
            bk_aud_dac_deinit();
            return;
        }

        // Set transfer length: entire audio array size
        bk_dma_set_transfer_len(dac_dma_id, aud_len * 4);

        // Initialize ring buffer with DMA association
        ring_buffer_init(dac_dma_rb, (uint8_t*)dac_dma_ring_buff, ring_buff_size, dac_dma_id, RB_DMA_TYPE_READ);

        // Register ISR and enable interrupt
        bk_dma_register_isr(dac_dma_id, NULL, (void *)aud_dac_dma_ringbuf_test_finish_isr);
        bk_dma_enable_finish_interrupt(dac_dma_id);

#if (CONFIG_SPE)
        bk_dma_set_dest_sec_attr(dac_dma_id, DMA_ATTR_SEC);
        bk_dma_set_src_sec_attr(dac_dma_id, DMA_ATTR_SEC);
#endif

        // Save parameters
        dac_dma_aud_ptr = aud_ptr;
        dac_dma_aud_len = aud_len;
        dac_dma_source  = dac_source;
        dac_dma_test_initialized = true;

        // Pre-fill ring buffer with entire audio array
        // This ensures DMA has enough data to start immediately
        uint32_t write_size = ring_buffer_write(dac_dma_rb, (uint8_t*)aud_ptr, aud_bytes);
        LOGI("prefill: wrote %d bytes, ring buffer fill: %d bytes\n", write_size, ring_buffer_get_fill_size(dac_dma_rb));

#if CONFIG_CACHE_ENABLE
        flush_dcache((void *)dac_dma_ring_buff, ring_buff_size);
#endif

        // Enable DAC source first
        bk_aud_dac_spk0_source_enable(dac_source, 1);

        // Start DAC before DMA (DAC must be ready to receive data from FIFO)
        ret = bk_aud_dac_start(AUD_DAC_CHL_LR);
        if (ret != BK_OK) {
            LOGE("bk_aud_dac_start fail\n");
            ring_buffer_clear(dac_dma_rb);
            os_free(dac_dma_rb);
            os_free(dac_dma_ring_buff);
            dac_dma_rb = NULL;
            dac_dma_ring_buff = NULL;
            bk_dma_deinit(dac_dma_id);
            bk_dma_free(DMA_DEV_AUDIO, dac_dma_id);
            dac_dma_id = DMA_ID_MAX;
            bk_aud_dac_spk0_source_enable(dac_source, 0);
            bk_aud_dac_deinit();
            return;
        }

        bk_aud_dac_get_fifo_status(&dac_fifo_status);
        os_printf("dac_fifo_status before DMA start: 0x%x\n", dac_fifo_status);

        // Start DMA after DAC is ready (similar to reference implementation)
        ret = bk_dma_start(dac_dma_id);
        if (ret != BK_OK) {
            LOGE("bk_dma_start fail\n");
            bk_aud_dac_stop(AUD_DAC_CHL_LR);
            ring_buffer_clear(dac_dma_rb);
            os_free(dac_dma_rb);
            os_free(dac_dma_ring_buff);
            dac_dma_rb = NULL;
            dac_dma_ring_buff = NULL;
            bk_dma_deinit(dac_dma_id);
            bk_dma_free(DMA_DEV_AUDIO, dac_dma_id);
            dac_dma_id = DMA_ID_MAX;
            bk_aud_dac_spk0_source_enable(dac_source, 0);
            bk_aud_dac_deinit();
            return;
        }

        LOGI("audio dac dma test successful\n");
		return;
    }
    else if (os_strcmp(argv[1], "stop") == 0) {
        LOGI("audio dac dma test stop\n");

        if (!dac_dma_test_initialized) {
            LOGI("dac dma test not started\n");
            return;
        }

        ///bk_dma_disable_finish_interrupt(dac_dma_id); ////
        // Disable DAC
        bk_aud_dac_spk0_source_enable(dac_dma_source, 0);
        bk_aud_dac_deinit();

        // Deinit DMA
        // Stop DMA
        bk_dma_stop(dac_dma_id);
        bk_dma_register_isr(dac_dma_id, NULL, NULL);
        bk_dma_deinit(dac_dma_id);
        ret = bk_dma_free(DMA_DEV_AUDIO, dac_dma_id);
        if (ret == BK_OK) {
            LOGI("free dma: %d success\r\n", dac_dma_id);
        }

        // Cleanup ring buffer
        if (dac_dma_rb != NULL) {
            ring_buffer_clear(dac_dma_rb);
            os_free(dac_dma_rb);
            dac_dma_rb = NULL;
        }
        if (dac_dma_ring_buff != NULL) {
            os_free(dac_dma_ring_buff);
            dac_dma_ring_buff = NULL;
        }

        dac_dma_aud_ptr = NULL;
        dac_dma_aud_len = 0;
        dac_dma_id = DMA_ID_MAX;
        dac_dma_test_initialized = false;
		if (signal != NULL) {
			os_free(signal);
			signal = NULL;
			signal_size = 0;
		}
        LOGI("audio dac dma test stop successfully\n");
		return;
    }
exit:
	cli_aud_help();
	return;
}
static void cli_aud_dac_dma_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_err_t ret = BK_OK;
    aud_dac_config_t dac_config = DEFAULT_AUD_DAC_CONFIG();
    dma_config_t dma_config = {0};
    uint32_t dac_fifo_addr;
    uint32_t *aud_ptr = NULL;
    uint32_t aud_len = 0;
    dma_id_t dma_id = DMA_ID_MAX;
    aud_dac_source_t dac_source = AUD_DAC_SOURCE_A2DP;
	uint32_t dac_bits = 16;

    if (argc != 2 && argc != 4 && argc != 5) {
		goto exit;
    }

    if (os_strcmp(argv[1], "start") == 0) {
        if (!parse_dac_source(argv[2], &dac_source)) {
            LOGE("dac source type: %s is not support\n", argv[2]);
			goto exit;
        }
        uint32_t dac_sample_rate = strtoul(argv[3], NULL, 10);
        switch (dac_source)
        {
            case AUD_DAC_SOURCE_A2DP:
                if (dac_sample_rate != 44100 && dac_sample_rate != 48000) {
                    LOGI("dac a2dp source not support sample rate: %d\n", dac_sample_rate);
					goto exit;
                }
                break;
            case AUD_DAC_SOURCE_CALL:
            case AUD_DAC_SOURCE_HINT:
                if (dac_sample_rate != 8000 && dac_sample_rate != 16000 && dac_sample_rate != 48000) {
                    LOGI("dac a2dp source not support sample rate: %d\n", dac_sample_rate);
					goto exit;
                }
                break;
            default:
                break;
        }

        if (argv[4])
            dac_bits = strtoul(argv[4], NULL, 10);

		if (dac_bits == 16) {
			if (!get_dac_test_data_by_rate(dac_sample_rate, (const uint32_t **)&aud_ptr, &aud_len)) {
				LOGE("unsupport pcm test sample rate: %d\n", dac_sample_rate);
				goto exit;
			}
		} else if (dac_bits == 24 && signal == NULL) {
			LOGI("Use the aud_generate_pcm_test to generate pcm 24bits test data\n");
			return;
		}

        if (signal != NULL) {
            aud_ptr = (uint32_t *)signal;
            aud_len = signal_size;
        }
        LOGI("audio dac test start, aud_len: %d, dac_bits:%d\n", aud_len, dac_bits);

        /* save parameters */
        dac_mcp_aud_ptr = aud_ptr;
        dac_mcp_aud_len = aud_len;
        dac_mcp_source  = dac_source;
        dac_mcp_test_initialized = true;

        aud_hardware_reset();
		dac_config.bits = dac_bits;
        ret = bk_aud_dac_init(&dac_config);
        if (ret != BK_OK) {
            LOGE("bk_aud_dac_init fail \n");
            return;
        }
        LOGI("init audio driver and dac successful\n");
        bk_aud_dac_set_sample_rate(dac_source, dac_sample_rate);

        //init dma driver
        ret = bk_dma_driver_init();
        if (ret != BK_OK) {
            LOGE("dma driver init failed\r\n");
            return;
        }
        dma_config.mode      = DMA_WORK_MODE_REPEAT;
        dma_config.chan_prio = 1;
        dma_config.src.dev   = DMA_DEV_DTCM;
        dma_config.src.width = DMA_DATA_WIDTH_32BITS;
        if (!get_dac_fifo_addr_by_source(dac_source, &dma_config.dst.dev, &dac_fifo_addr)) {
            LOGE("get dac fifo address failed\r\n");
            return;
        }

        dma_config.dst.width        = DMA_DATA_WIDTH_32BITS;
        dma_config.dst.addr_inc_en  = DMA_ADDR_INC_ENABLE;
        dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
        dma_config.dst.start_addr   = dac_fifo_addr;
        dma_config.dst.end_addr     = dac_fifo_addr + 4;

        dma_config.src.addr_inc_en  = DMA_ADDR_INC_ENABLE;
        dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
        dma_config.src.start_addr   = (uint32_t)(uintptr_t)aud_ptr;
        dma_config.src.end_addr     = (uint32_t)(uintptr_t)aud_ptr + aud_len * 4;
        LOGD("source_addr:0x%x, source_end_addr:0x%x\r\n", dma_config.src.start_addr, dma_config.src.end_addr);
        LOGD("dest_addr:0x%x, dest_end_addr:0x%x\r\n", dma_config.dst.start_addr, dma_config.dst.end_addr);

        //init dma channel
        dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
        if ((dma_id < DMA_ID_0) || (dma_id >= DMA_ID_MAX)) {
            LOGE("malloc dma fail \r\n");
            return;
        }
        dac_dma_id = dma_id;
        LOGI("dma_id: %d\n", dma_id);

        ret = bk_dma_init(dma_id, &dma_config);
        if (ret != BK_OK) {
            LOGE("dma init failed\r\n");
            return;
        }

        //bk_aud_dac_set_bits_width(dac_source, 16);
        bk_dma_set_transfer_len(dma_id, aud_len * 4);

#if (CONFIG_SPE)
        bk_dma_set_dest_sec_attr(dma_id, DMA_ATTR_SEC);
        bk_dma_set_src_sec_attr(dma_id, DMA_ATTR_SEC);
#endif

        ret = bk_dma_start(dma_id);
        if (ret != BK_OK) {
            return;
        }

        /* enable dac source */
        bk_aud_dac_spk0_source_enable(dac_source, 1);
        /* start dac */
        bk_aud_dac_start(AUD_DAC_CHL_LR);

        LOGD("audio dac dma dac test successful\n");
		return;
    }
    else if (os_strcmp(argv[1], "stop") == 0) {

        if (!dac_mcp_test_initialized) {
            LOGE("dac dma test not started\n");
            return;
        }

        //disable dac
        bk_aud_dac_spk0_source_enable(dac_mcp_source, 0);
        bk_aud_dac_deinit();

        //stop dma
        bk_dma_stop(dac_dma_id);
        bk_dma_deinit(dac_dma_id);
        ret = bk_dma_free(DMA_DEV_AUDIO, dac_dma_id);
        if (ret != BK_OK) {
            LOGE("free dma: %d failed\r\n", dac_dma_id);
        }

        dac_mcp_aud_ptr = NULL;
        dac_mcp_aud_len = 0;
        dac_mcp_test_initialized = false;
        dac_dma_id = DMA_ID_MAX;
		if (signal != NULL) {
			os_free(signal);
			signal = NULL;
			signal_size = 0;
		}
        LOGD("audio dac dma test stop successfully\n");
		return;
    } 
exit:
	cli_aud_help();
	return;
}

static void cli_aud_dac_mcp_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	aud_dac_config_t dac_config = DEFAULT_AUD_DAC_CONFIG();
	void *aud_ptr = NULL;
	uint32_t aud_len = 0;
	aud_dac_source_t dac_source = AUD_DAC_SOURCE_A2DP;
	uint32_t dac_fifo_status = 0;
	uint32_t dac_bits = 16; // default

	if (argc != 2 && argc != 4 && argc != 5) {
		goto exit;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		if (!parse_dac_source(argv[2], &dac_source)) {
			LOGE("dac source type: %s is not support\n", argv[2]);
			goto exit;
		}
		uint32_t dac_sample_rate = strtoul(argv[3], NULL, 10);
		switch (dac_source)
		{
			case AUD_DAC_SOURCE_A2DP:
				if (dac_sample_rate != 44100 && dac_sample_rate != 48000) {
					LOGI("dac a2dp source not support sample rate: %d\n", dac_sample_rate);
					goto exit;
				}
				break;
			case AUD_DAC_SOURCE_CALL:
			case AUD_DAC_SOURCE_HINT:
				if (dac_sample_rate != 8000 && dac_sample_rate != 16000 && dac_sample_rate != 48000) {
					LOGI("dac source not support sample rate: %d\n", dac_sample_rate);
					goto exit;
				}
				break;
			default:
				break;
		}

		if (argv[4])
			dac_bits = strtoul(argv[4], NULL, 10);

		if (dac_bits == 16) {
			if (!get_dac_test_data_by_rate(dac_sample_rate, (const uint32_t **)&aud_ptr, &aud_len)) {
				LOGD("unsupport pcm test sample rate: %d\n", dac_sample_rate);
				return;
			}
		} else if (dac_bits == 24 && signal == NULL)
		{
			LOGW("Use the aud_generate_pcm_test to generate pcm 24bits test data\n");
			return;
		}
		LOGD("audio dac test %d start\n", dac_sample_rate);

		if (signal != NULL) {
			aud_ptr = (uint32_t *)signal;
			aud_len = signal_size;
		}
		LOGD("audio dac test start, aud_len: %d, dac_bits:%d\n", aud_len, dac_bits);

		aud_hardware_reset();
		dac_config.bits = dac_bits;
		ret = bk_aud_dac_init(&dac_config);
		if (ret != BK_OK) {
			LOGE("bk_aud_dac_init fail \n");
			return;
		}
		LOGI("init audio driver and dac successful\n");

		bk_aud_dac_spk0_set_write_threshold(dac_source, 5);
		bk_aud_dac_spk0_set_read_threshold(dac_source, 3);

		bk_aud_dac_set_sample_rate(dac_source, dac_sample_rate);
		bk_aud_dac_spk0_source_enable(dac_source, 1);

		bk_aud_dac_get_fifo_status(&dac_fifo_status);
		if (dac_fifo_status != (1 << dac_source)) {
			LOGE("dac enable fail!\n");
			return;
		}

		/* start dac */
		ret = bk_aud_dac_start(AUD_DAC_CHL_LR);// set LR!
		if (ret != BK_OK) {
			os_printf("bk_aud_dac_start fail \n");
			bk_aud_dac_deinit();
			return;
		}

		/* save parameters for thread */
		dac_mcp_aud_ptr = aud_ptr;
		dac_mcp_aud_len = aud_len;
		dac_mcp_source  = dac_source;
		dac_mcp_test_initialized = true;
		{
			uint32_t dac_fifo_status = 0, v = 0, i = 0;
			LOGI("[+]aud_dac_mcp_test_thread start, dac_mcp_source:%d, dac_mcp_aud_len:%d\r\n", dac_mcp_source, dac_mcp_aud_len);
			rtos_enter_critical();
			while(1)
			{
				bk_aud_dac_get_fifo_status(&dac_fifo_status);
				if (dac_fifo_status & (1 << dac_mcp_source))
				{
					if (dac_bits == 16) {
						v = (((uint32_t*)dac_mcp_aud_ptr)[i++])&0xFFFF;
					}
					else if (dac_bits == 24) {
						v = (((uint32_t*)dac_mcp_aud_ptr)[i++])&0xFFFFFF;
					}
					bk_aud_dac_spk0_write_data(dac_mcp_source, (uint32_t)(v));
					if(i >= dac_mcp_aud_len) {
						i = 0;
					}
				}
			}
			rtos_delay_milliseconds(5000);
			LOGI("[+]aud_dac_mcp_test_thread exit\r\n");
		}
	} 
	else if (os_strcmp(argv[1], "stop") == 0) {
		LOGD("audio dac mcp test stop\n");
		if (!dac_mcp_test_initialized) {
			LOGE("dac mcp test not started\n");
			return;
		}

		/* stop thread */
		if (dac_mcp_thread != NULL)
		{
			rtos_delete_thread(&dac_mcp_thread);
			dac_mcp_thread = NULL;
		}

		/* disable dac */
		bk_aud_dac_spk0_source_enable(dac_mcp_source, 0);
		bk_aud_dac_deinit();

		dac_mcp_aud_ptr = NULL;
		dac_mcp_aud_len = 0;
		dac_mcp_test_initialized = false;
		if (signal != NULL) {
			os_free(signal);
			signal = NULL;
			signal_size = 0;
		}
		LOGD("audio dac mcp test stop successfully\n");
		return;
	} 
exit:
	cli_aud_help();
	return;
}

#if (CONFIG_AUDIO_DAC && CONFIG_AUDIO_RING_BUFF)
// Ringbuf write thread: fill ringbuf from audio source
static void aud_dac_ringbuf_write_thread(beken_thread_arg_t data)
{
	uint32_t write_size = 0;
	uint32_t free_size = 0;

	LOGI("[+]aud_dac_ringbuf_write_thread start\r\n");

	while(dac_ringbuf_test_running) {
		if (dac_ringbuf_rb == NULL || dac_ringbuf_aud_ptr == NULL) {
			rtos_delay_milliseconds(10);
			continue;
		}

		// Check free space in ringbuf
		free_size = ring_buffer_get_free_size(dac_ringbuf_rb);
		if (free_size < 4) {  // At least 4 bytes (one sample)
			rtos_delay_milliseconds(2);
			continue;
		}

		// Calculate how much to write (in bytes)
		uint32_t samples_to_write = free_size / 4;  // 4 bytes per sample (32-bit)
		uint32_t bytes_to_write = samples_to_write * 4;

		// Make sure we don't exceed available audio data
		uint32_t remaining_samples = dac_ringbuf_aud_len - dac_ringbuf_aud_idx;
		if (remaining_samples == 0) {
			// Loop back to beginning
			dac_ringbuf_aud_idx = 0;
			remaining_samples = dac_ringbuf_aud_len;
		}

		if (samples_to_write > remaining_samples) {
			samples_to_write = remaining_samples;
			bytes_to_write = samples_to_write * 4;
		}

		// Write data to ringbuf
		uint8_t *src_ptr = (uint8_t *)((uint32_t *)dac_ringbuf_aud_ptr + dac_ringbuf_aud_idx);
		write_size = ring_buffer_write(dac_ringbuf_rb, src_ptr, bytes_to_write);

		if (write_size > 0) {
			dac_ringbuf_aud_idx += (write_size / 4);
			if (dac_ringbuf_aud_idx >= dac_ringbuf_aud_len) {
				dac_ringbuf_aud_idx = 0;  // Loop
			}
		}

		// Small delay to avoid CPU spinning
		if (write_size == 0) {
			rtos_delay_milliseconds(2);
		}
	}

	LOGI("[+]aud_dac_ringbuf_write_thread exit\r\n");
	rtos_delete_thread(NULL);
}

// Ringbuf read thread: read from ringbuf and write to DAC FIFO
static void aud_dac_ringbuf_read_thread(beken_thread_arg_t data)
{
	uint32_t dac_fifo_status = 0;
	uint32_t read_size = 0;
	uint8_t read_buf[64];  // Read buffer (16 samples max)

	LOGI("[+]aud_dac_ringbuf_read_thread start\r\n");

	while(dac_ringbuf_test_running) {
		if (dac_ringbuf_rb == NULL) {
			rtos_delay_milliseconds(10);
			continue;
		}

		// Check DAC FIFO status
		bk_aud_dac_get_fifo_status(&dac_fifo_status);
		if (!(dac_fifo_status & (1 << dac_ringbuf_source))) {
			// FIFO not ready, wait a bit
			rtos_delay_milliseconds(2);
			continue;
		}

		// Check available data in ringbuf
		uint32_t fill_size = ring_buffer_get_fill_size(dac_ringbuf_rb);
		if (fill_size < 4) {  // At least 4 bytes (one sample)
			rtos_delay_milliseconds(2);
			continue;
		}

		// Read from ringbuf (read up to 16 samples = 64 bytes at a time)
		uint32_t bytes_to_read = (fill_size > sizeof(read_buf)) ? sizeof(read_buf) : fill_size;
		bytes_to_read = (bytes_to_read / 4) * 4;  // Align to 4 bytes

		read_size = ring_buffer_read(dac_ringbuf_rb, read_buf, bytes_to_read);

		if (read_size > 0 && (read_size % 4) == 0) {
			// Write to DAC FIFO
			uint32_t samples_read = read_size / 4;
			uint32_t *sample_ptr = (uint32_t *)read_buf;

			for (uint32_t i = 0; i < samples_read; i++) {
				bk_aud_dac_get_fifo_status(&dac_fifo_status);
				if (dac_fifo_status & (1 << dac_ringbuf_source)) {
					bk_aud_dac_spk0_write_data(dac_ringbuf_source, sample_ptr[i]);
				} else {
					// FIFO full, break and check again later
					break;
				}
			}
		}

		// Small delay
		if (read_size == 0) {
			rtos_delay_milliseconds(2);
		}
	}

	LOGI("[+]aud_dac_ringbuf_read_thread exit\r\n");
	rtos_delete_thread(NULL);
}

static void cli_aud_dac_ringbuf_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	aud_dac_config_t dac_config = DEFAULT_AUD_DAC_CONFIG();
	void *aud_ptr = NULL;
	uint32_t aud_len = 0;
	aud_dac_source_t dac_source = AUD_DAC_SOURCE_A2DP;
	uint32_t dac_fifo_status = 0;

	if (argc != 2 && argc != 4) {
		cli_aud_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		if (!parse_dac_source(argv[2], &dac_source)) {
			LOGE("dac source type: %s is not support\n", argv[2]);
			cli_aud_help();
			return;
		}
		uint32_t dac_sample_rate = strtoul(argv[3], NULL, 10);
		switch (dac_source)
		{
			case AUD_DAC_SOURCE_A2DP:
				if (dac_sample_rate != 44100 && dac_sample_rate != 48000) {
					LOGI("dac a2dp source not support sample rate: %d\n", dac_sample_rate);
					cli_aud_help();
					return;
				}
				break;

			case AUD_DAC_SOURCE_CALL:
			case AUD_DAC_SOURCE_HINT:
				if (dac_sample_rate != 8000 && dac_sample_rate != 16000 && dac_sample_rate != 48000) {
					LOGI("dac source not support sample rate: %d\n", dac_sample_rate);
					cli_aud_help();
					return;
				}
				break;

			default:
				break;
		}

		// Select audio data based on sample rate
		if (!get_dac_test_data_by_rate(dac_sample_rate, (const uint32_t **)&aud_ptr, &aud_len)) {
			LOGI("unsupport pcm test sample rate: %d\n", dac_sample_rate);
			return;
		}
		LOGI("audio dac ringbuf test %d start\n", dac_sample_rate);

		if (signal != NULL) {
			aud_ptr = (uint32_t *)signal;
			aud_len = signal_size;
		}
		LOGI("audio dac ringbuf test start, aud_len: %d\n", aud_len);

		// Hardware reset
		aud_hardware_reset();

		// Initialize DAC
		ret = bk_aud_dac_init(&dac_config);
		if (ret != BK_OK) {
			LOGE("bk_aud_dac_init fail \n");
			return;
		}
		LOGI("init audio driver and dac successful\n");

		bk_aud_dac_spk0_set_write_threshold(dac_source, 5);
		bk_aud_dac_spk0_set_read_threshold(dac_source, 3);
		bk_aud_dac_set_sample_rate(dac_source, dac_sample_rate);
		bk_aud_dac_set_bits_width(dac_source, 16);

		// Check FIFO status
		bk_aud_dac_get_fifo_status(&dac_fifo_status);
		os_printf("1_dac_fifo_status : 0x%x\n", dac_fifo_status);

		bk_aud_dac_spk0_source_enable(dac_source, 1);

		bk_aud_dac_get_fifo_status(&dac_fifo_status);
		os_printf("2_dac_fifo_status : 0x%x\n", dac_fifo_status);

		// Start DAC
		ret = bk_aud_dac_start(AUD_DAC_CHL_LR);
		if (ret != BK_OK) {
			os_printf("bk_aud_dac_start fail \n");
			bk_aud_dac_deinit();
			return;
		}

		// Allocate ring buffer memory
		dac_ringbuf_buffer = (uint8_t *)os_malloc(DAC_RINGBUF_SIZE);
		if (dac_ringbuf_buffer == NULL) {
			LOGE("Failed to allocate ring buffer memory\n");
			bk_aud_dac_stop(AUD_DAC_CHL_LR);
			bk_aud_dac_spk0_source_enable(dac_source, 0);
			bk_aud_dac_deinit();
			return;
		}

		// Initialize ring buffer
		dac_ringbuf_rb = (RingBufferContext *)os_malloc(sizeof(RingBufferContext));
		if (dac_ringbuf_rb == NULL) {
			LOGE("Failed to allocate ring buffer context\n");
			os_free(dac_ringbuf_buffer);
			dac_ringbuf_buffer = NULL;
			bk_aud_dac_stop(AUD_DAC_CHL_LR);
			bk_aud_dac_spk0_source_enable(dac_source, 0);
			bk_aud_dac_deinit();
			return;
		}

		ring_buffer_init(dac_ringbuf_rb, (uint8_t*)dac_ringbuf_buffer, DAC_RINGBUF_SIZE, DMA_ID_MAX, RB_DMA_TYPE_NULL);

		// Save parameters
		dac_ringbuf_aud_ptr = aud_ptr;
		dac_ringbuf_aud_len = aud_len;
		dac_ringbuf_aud_idx = 0;
		dac_ringbuf_source = dac_source;
		dac_ringbuf_test_initialized = true;
		dac_ringbuf_test_running = true;

		// Create write thread (fills ringbuf from audio source)
		ret = rtos_create_thread(&dac_ringbuf_write_thread,
								 BEKEN_DEFAULT_WORKER_PRIORITY - 1,
								 "dac_rb_write",
								 (beken_thread_function_t)aud_dac_ringbuf_write_thread,
								 1024 * 2,
								 NULL);
		if (ret != BK_OK) {
			LOGE("Failed to create ringbuf write thread\n");
			ring_buffer_clear(dac_ringbuf_rb);
			os_free(dac_ringbuf_rb);
			os_free(dac_ringbuf_buffer);
			dac_ringbuf_rb = NULL;
			dac_ringbuf_buffer = NULL;
			bk_aud_dac_stop(AUD_DAC_CHL_LR);
			bk_aud_dac_spk0_source_enable(dac_source, 0);
			bk_aud_dac_deinit();
			return;
		}

		// Create read thread (reads from ringbuf and writes to DAC FIFO)
		ret = rtos_create_thread(&dac_ringbuf_read_thread,
								 BEKEN_DEFAULT_WORKER_PRIORITY - 1,
								 "dac_rb_read",
								 (beken_thread_function_t)aud_dac_ringbuf_read_thread,
								 1024 * 2,
								 NULL);
		if (ret != BK_OK) {
			LOGE("Failed to create ringbuf read thread\n");
			dac_ringbuf_test_running = false;
			if (dac_ringbuf_write_thread != NULL) {
				rtos_thread_join(&dac_ringbuf_write_thread);
				dac_ringbuf_write_thread = NULL;
			}
			ring_buffer_clear(dac_ringbuf_rb);
			os_free(dac_ringbuf_rb);
			os_free(dac_ringbuf_buffer);
			dac_ringbuf_rb = NULL;
			dac_ringbuf_buffer = NULL;
			bk_aud_dac_stop(AUD_DAC_CHL_LR);
			bk_aud_dac_spk0_source_enable(dac_source, 0);
			bk_aud_dac_deinit();
			return;
		}

		os_printf("audio dac ringbuf test successful\n");
	} else if (os_strcmp(argv[1], "stop") == 0) {
		os_printf("audio dac ringbuf test stop\n");

		if (!dac_ringbuf_test_initialized) {
			os_printf("dac ringbuf test not started\n");
			return;
		}

		// Stop threads
		dac_ringbuf_test_running = false;
		if (dac_ringbuf_write_thread != NULL) {
			rtos_thread_join(&dac_ringbuf_write_thread);
			dac_ringbuf_write_thread = NULL;
		}
		if (dac_ringbuf_read_thread != NULL) {
			rtos_thread_join(&dac_ringbuf_read_thread);
			dac_ringbuf_read_thread = NULL;
		}

		// Cleanup ring buffer
		if (dac_ringbuf_rb != NULL) {
			ring_buffer_clear(dac_ringbuf_rb);
			os_free(dac_ringbuf_rb);
			dac_ringbuf_rb = NULL;
		}
		if (dac_ringbuf_buffer != NULL) {
			os_free(dac_ringbuf_buffer);
			dac_ringbuf_buffer = NULL;
		}

		// Disable DAC
		bk_aud_dac_stop(AUD_DAC_CHL_LR);
		bk_aud_dac_spk0_source_enable(dac_ringbuf_source, 0);
		bk_aud_dac_deinit();

		dac_ringbuf_aud_ptr = NULL;
		dac_ringbuf_aud_len = 0;
		dac_ringbuf_aud_idx = 0;
		dac_ringbuf_test_initialized = false;
		os_printf("audio dac ringbuf test stop successfully\n");
	} else {
		cli_aud_help();
		return;
	}
}
#endif

static void cli_aud_dac_mcp_test_cmd_old(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_err_t ret = BK_OK;
    aud_dac_config_t dac_config = DEFAULT_AUD_DAC_CONFIG();
    uint32_t dac_fifo_status = 0;
    uint32_t test_flag = 0;
    int i = 0;
    aud_dac_source_t dac_source = AUD_DAC_SOURCE_A2DP;

    if (!parse_dac_source(argv[1], &dac_source)) {
        LOGE("dac source type: %s is not support\n", argv[1]);
        cli_aud_help();
        return;
    }

    uint32_t dac_sample_rate = strtoul(argv[2], NULL, 10);
    switch (dac_source)
    {
        case AUD_DAC_SOURCE_A2DP:
            if (dac_sample_rate != 44100 && dac_sample_rate != 48000) {
                os_printf("dac a2dp source not support sample rate: %d\n", dac_sample_rate);
                cli_aud_help();
                return;
            }
            break;

        case AUD_DAC_SOURCE_CALL:
        case AUD_DAC_SOURCE_HINT:
            if (dac_sample_rate != 8000 && dac_sample_rate != 16000 && dac_sample_rate != 48000) {
                os_printf("dac a2dp source not support sample rate: %d\n", dac_sample_rate);
                cli_aud_help();
                return;
            }
            break;

        default:
            break;
    }

    if (get_dac_test_data_by_rate(dac_sample_rate, (const uint32_t **)&aud_ptr, &aud_len)) {
        LOGI("audio dac test %d start\n", dac_sample_rate);
    } else {
        LOGI("unsupport dac test sample rate: %d\n", dac_sample_rate);
    }

    if (signal != NULL) {
        aud_ptr = (uint32_t *)signal;
        aud_len = signal_size;
    }

    aud_hardware_reset();

    ret = bk_aud_dac_init(&dac_config);
    if (ret != BK_OK) {
        LOGE("bk_aud_dac_init fail \n");
        return;
    }
    LOGI("init audio driver and dac successful\n");

    bk_aud_dac_spk0_set_write_threshold(dac_source, 5);
    bk_aud_dac_spk0_set_read_threshold(dac_source, 3);

    bk_aud_dac_set_sample_rate(dac_source, dac_sample_rate);

    bk_aud_dac_set_bits_width(dac_source, 16);

    bk_aud_dac_get_fifo_status(&dac_fifo_status);
    os_printf("1_dac_fifo_status : 0x%x\n", dac_fifo_status);  /// 0x0

    bk_aud_dac_spk0_source_enable(dac_source, 1);

    bk_aud_dac_get_fifo_status(&dac_fifo_status);
    os_printf("2_dac_fifo_status : 0x%x\n", dac_fifo_status); /// 0x2

    /* start dac */
    ret = bk_aud_dac_start(AUD_DAC_CHL_LR);   // must set LR --- 20260104
    if (ret != BK_OK)
        return;

    os_printf("enable dac successful\n");
    uint32_t dac_fifo_addr = 0;
    bk_aud_dac_get_fifo_status(&dac_fifo_status);
    bk_aud_dac_spk0_get_fifo_addr(dac_source, &dac_fifo_addr);
    os_printf("dac_fifo_addr : 0x%x, dac_fifo_status : 0x%x\n", dac_fifo_addr, dac_fifo_status);
    rtos_delay_milliseconds(2000);
    while(1)
    {
        bk_aud_dac_get_fifo_status(&dac_fifo_status);
        if(dac_fifo_status & 1<<dac_source)
        {
            bk_aud_dac_spk0_write_data(dac_source, (uint32_t)(((uint32_t*)aud_ptr)[i++]));
#if 0
            extern uint32_t bk_aud_dac_spk0_read_data(void);
            //uint16_t temp = (uint16_t)((((uint32_t*)aud_ptr)[i++])&0xFFFF);//bk_aud_dac_spk0_read_data()>>8;
            uint16_t temp = (uint16_t)((bk_aud_dac_spk0_read_data()>>8)&0xFFFF);
            bk_uart_write_bytes(1, (void *)&temp, sizeof(uint16_t));
            rtos_delay_milliseconds(2);
#endif 
            if(i == aud_len) {
                i = 0;
                test_flag++;
                if (test_flag == 10) {
                    //break;
                }
            }
        }
    }
    rtos_delay_milliseconds(5000);
    bk_aud_dac_stop(AUD_DAC_CHL_LR);   // must set LR --- 20260104
    bk_aud_dac_spk0_source_enable(dac_source, 0);
    bk_aud_dac_deinit();

    LOGI("audio dac test complete\n");
    return;
}

#ifdef AUD_ASDF_DEBUG
static void cli_aud_asdf_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t test_id = 0;

	if (argc != 2 && argc != 3) {
		cli_aud_help();
		return;
	}

	if (os_strcmp(argv[1], "pipeline") == 0) {
		test_id = strtoul(argv[2], NULL, 10);

        /* audio pipeline test */
        switch (test_id)
        {
            case 0:
            {
                extern bk_err_t asdf_pipeline_test_case_0(void);
                asdf_pipeline_test_case_0();
                break;
            }

            case 1:
            {
                extern bk_err_t asdf_pipeline_test_case_1(void);
                asdf_pipeline_test_case_1();
                break;
            }

            case 2:
            {
                extern bk_err_t asdf_pipeline_test_case_2(void);
                asdf_pipeline_test_case_2();
                break;
            }

            case 3:
            {
                extern bk_err_t asdf_pipeline_test_case_3(void);
                asdf_pipeline_test_case_3();
                break;
            }

            default:
                break;
        }
		os_printf("audio pipeline test successful\n");
	}else if (os_strcmp(argv[1], "element") == 0) {
		test_id = strtoul(argv[2], NULL, 10);

        /* audio element test */
        switch (test_id)
        {
            case 0:
            {
                extern bk_err_t asdf_element_test_case_0(void);
                asdf_element_test_case_0();
                break;
            }

            case 1:
            {
                extern bk_err_t asdf_element_test_case_1(void);
                asdf_element_test_case_1();
                break;
            }

            case 2:
            {
                extern bk_err_t asdf_element_test_case_2(void);
                asdf_element_test_case_2();
                break;
            }

            case 3:
            {
                extern bk_err_t asdf_element_test_case_3(void);
                asdf_element_test_case_3();
                break;
            }

            case 4:
            {
                extern bk_err_t asdf_element_test_case_4(void);
                asdf_element_test_case_4();
                break;
            }

            case 5:
            {
                extern bk_err_t asdf_element_test_case_5(void);
                asdf_element_test_case_5();
                break;
            }

            case 6:
            {
                extern bk_err_t asdf_element_test_case_6(void);
                asdf_element_test_case_6();
                break;
            }

            case 7:
            {
                extern bk_err_t asdf_element_test_case_7(void);
                asdf_element_test_case_7();
                break;
            }

            case 8:
            {
                extern bk_err_t asdf_element_test_case_8(void);
                asdf_element_test_case_8();
                break;
            }

            default:
                break;
        }

		os_printf("audio element successfully\n");
	}else if (os_strcmp(argv[1], "event") == 0) {
        /* audio event test */
        extern bk_err_t asdf_event_test_case_0(void);
        asdf_event_test_case_0();
        os_printf("audio event successfully\n");
	} else if (os_strcmp(argv[1], "onboard_speaker") == 0) {
        /* audio event test */
        extern bk_err_t asdf_onboard_speaker_test_case_0(void);
        asdf_onboard_speaker_test_case_0();
        os_printf("audio onboard speaker successfully\n");
	} else if (os_strcmp(argv[1], "onboard_mic") == 0) {
        /* audio event test */
        extern bk_err_t asdf_onboard_mic_test_case_0(void);
        asdf_onboard_mic_test_case_0();
        os_printf("audio onboard mic successfully\n");
	} else {
		cli_aud_help();
		return;
	}
}
#endif

#endif	//#if CONFIG_AUDIO_DAC

#if CONFIG_I2S

// I2S test variables
static bool i2s_mcp_test_running = false;
static beken_thread_t i2s_mcp_thread = NULL;
static bool i2s_mcp_test_initialized = false;
static i2s_txrx_type_t i2s_test_type = I2S_TXRX_TYPE_TX;
static i2s_channel_id_t i2s_channel = I2S_CHANNEL_1;
static i2s_gpio_group_id_t i2s_gpio_group = I2S_GPIO_GROUP_0;

static i2s_gpio_group_id_t i2s_tx_gpio_group = I2S_GPIO_GROUP_0;  // GPIO group for TX (in loopback mode)
static i2s_gpio_group_id_t i2s_rx_gpio_group = I2S_GPIO_GROUP_0;  // GPIO group for RX (in loopback mode)
static RingBufferContext *i2s_tx_rb = NULL;
static RingBufferContext *i2s_rx_rb = NULL;
static bool i2s_dma_test_initialized = false;
static aud_dac_source_t i2s_dma_dac_source = AUD_DAC_SOURCE_CALL;//AUD_DAC_SOURCE_A2DP;  // DAC source for I2S DMA RX -> DAC
static bool i2s_dma_loopback_mode = false;                       // Loopback mode: TX -> RX -> DAC
static uint8_t *i2s_dma_tx_test_data = NULL;                     // Test data for TX callback (allocated buffer)
static uint32_t i2s_dma_tx_test_len = 0;                         // Test data length in bytes
static uint32_t i2s_dma_frame_bytes = 0;                         // Frame size in bytes (frame_duration * sample_rate * channels * bytes_per_sample)

// RX ringbuf to DAC FIFO DMA variables
static dma_id_t i2s_dma_rx_dac_dma_id = DMA_ID_MAX;
static bool i2s_dma_rx_dac_dma_initialized = false;
static uint32_t *i2s_dma_rx_dac_buffer = NULL;   // Intermediate buffer for format conversion
static uint32_t i2s_dma_rx_dac_buffer_size = 0;  // Size of intermediate buffer (in bytes)
#define I2S_DMA_RX_DAC_BUFFER_SIZE (640)         // 640 bytes = 160 samples (32-bit words)

//static uint32_t __maybe_unused g_sample_rate = 16000;
//static uint32_t __maybe_unused g_frame_duration = 20;
// static uint32_t g_frame_size = g_sample_rate * g_frame_duration / 1000;
// static uint32_t g_frame_bytes = g_frame_size * 2;
// static uint32_t g_stereo = 1;


// I2S DMA callback functions
static int i2s_tx_data_handle_cb(uint32_t size)
{
	// Fill ring buffer with test data when DMA needs more data
	// Process data in frame units to ensure frame alignment
	static uint32_t frame_idx = 0;  // Index to track position in frame data
	uint32_t write_size = 0;

	if (i2s_tx_rb == NULL) {
		LOGE("[TX-CB] ERROR: i2s_tx_rb is NULL!\n");
		return 0;
	}

	// Check if frame_bytes is initialized
	if (i2s_dma_frame_bytes == 0 || i2s_dma_tx_test_data == NULL || i2s_dma_tx_test_len == 0) {
		LOGE("[TX-CB] ERROR: frame data not initialized! frame_bytes=%d, test_data=%p, test_len=%d\n", 
				i2s_dma_frame_bytes, i2s_dma_tx_test_data, i2s_dma_tx_test_len);
		return 0;
	}

	// Check available space in ring buffer - need at least one frame
	uint32_t free_size = ring_buffer_get_free_size(i2s_tx_rb);
	if (free_size < i2s_dma_frame_bytes) {
		LOGW("[TX-CB] WARNING: ring buffer free space (%d) < one frame (%d)\n", free_size, i2s_dma_frame_bytes);
		return 0;
	}

	// Calculate how many frames we can write
	// Write at least one frame, or as many frames as requested (size) allows
	uint32_t frames_to_write = (size >= i2s_dma_frame_bytes) ? (size / i2s_dma_frame_bytes) : 1;
	uint32_t max_frames_by_space = free_size / i2s_dma_frame_bytes;

	// Limit frames_to_write by available space
	if (frames_to_write > max_frames_by_space) {
		frames_to_write = max_frames_by_space;
	}

	if (frames_to_write == 0) {
		LOGW("[TX-CB] WARNING: not enough space for even one frame\n");
		return 0;
	}

	// Write frames in loop, cycling through frame data if needed
	for (uint32_t f = 0; f < frames_to_write; f++) {
		// Write one complete frame
		uint32_t frame_write_size = ring_buffer_write(i2s_tx_rb, 
														i2s_dma_tx_test_data + (frame_idx * i2s_dma_frame_bytes), 
														i2s_dma_frame_bytes);
		if (frame_write_size == i2s_dma_frame_bytes) {
			write_size += frame_write_size;
			frame_idx = (frame_idx + 1) % (i2s_dma_tx_test_len / i2s_dma_frame_bytes);  // Cycle through frames
		} else {
			LOGW("[TX-CB] WARNING: failed to write frame %d, wrote %d bytes\n", f, frame_write_size);
			break;
		}
	}
	return write_size;
}

// DMA finish ISR: read from I2S RX ring buffer, convert format, and start next DMA transfer
static void i2s_dma_rx_dac_dma_finish_isr(dma_id_t dma_id)
{
	static uint16_t left_ch = 0;
	static bool left_ch_received = false;
	uint32_t dac_fifo_status = 0;
	uint32_t expected_bit = (1 << i2s_dma_dac_source);
	
	if (i2s_rx_rb == NULL || i2s_dma_rx_dac_buffer == NULL || dma_id != i2s_dma_rx_dac_dma_id) {
		return;
	}
	
#if CONFIG_CACHE_ENABLE
	flush_all_dcache();
#endif
	
	// Check DAC FIFO status before reading more data
	bk_aud_dac_get_fifo_status(&dac_fifo_status);
	if (!(dac_fifo_status & expected_bit)) {
		// FIFO is full, skip this transfer
		// Will retry on next I2S RX callback
		return;
	}
	
	// Read data from I2S RX ring buffer and convert format
	// Read in frame units to ensure frame alignment
	uint8_t rx_buf[I2S_DMA_RX_DAC_BUFFER_SIZE];
	
	// Check if we have at least one frame of data
	uint32_t fill_size = ring_buffer_get_fill_size(i2s_rx_rb);
	if (i2s_dma_frame_bytes == 0 || fill_size < i2s_dma_frame_bytes) {
		// Not enough data for one frame, wait for more
		return;
	}
	
	// Read one frame at a time (or as much as buffer allows, rounded down to frame boundary)
	// Calculate how many frames we can read
	uint32_t available_frames = fill_size / i2s_dma_frame_bytes;
	if (available_frames == 0) {
		return;  // Not enough for even one frame
	}
	
	// Read one frame (or as many as buffer allows, but at least one frame)
	uint32_t read_size = i2s_dma_frame_bytes;
	if (read_size > I2S_DMA_RX_DAC_BUFFER_SIZE) {
		// If frame is larger than buffer, read as much as buffer allows, rounded down to frame boundary
		read_size = (I2S_DMA_RX_DAC_BUFFER_SIZE / i2s_dma_frame_bytes) * i2s_dma_frame_bytes;
		if (read_size == 0) {
			read_size = I2S_DMA_RX_DAC_BUFFER_SIZE / 2 * 2;  // At least ensure 2-byte alignment
		}
	}
	
	read_size = ring_buffer_read(i2s_rx_rb, rx_buf, read_size);
	
	// Ensure we read complete frame (read_size should be multiple of frame_bytes)
	// Also ensure it's multiple of 2 (16-bit samples)
	if (read_size > 0 && read_size % 2 == 0) {
		// Round down to frame boundary if needed
		if (read_size > i2s_dma_frame_bytes && read_size % i2s_dma_frame_bytes != 0) {
			read_size = (read_size / i2s_dma_frame_bytes) * i2s_dma_frame_bytes;
		}
		// Process received data: pair L and R channels
		// I2S RX data format: L, R, L, R... (16-bit samples)
		// DAC data format: {R(high16), L(low16)} (32-bit words)
		uint16_t *ch_data = (uint16_t *)rx_buf;
		uint32_t ch_count = read_size / 2;
		uint32_t dac_word_count = 0;
		
		for (uint32_t i = 0; i < ch_count && dac_word_count < (I2S_DMA_RX_DAC_BUFFER_SIZE / 4); i++) {
				if (!left_ch_received) {
					// First read: Left channel
				left_ch = ch_data[i];
					left_ch_received = true;
				} else {
					// Second read: Right channel
				uint16_t right_ch = ch_data[i];
					// Combine to {R, L} format: high 16bits = R, low 16bits = L
				i2s_dma_rx_dac_buffer[dac_word_count++] = ((uint32_t)right_ch << 16) | left_ch;
					left_ch_received = false;  // Reset for next pair
				}
		}
		
		// Update DMA transfer length and start next transfer
		if (dac_word_count > 0) {
			uint32_t transfer_bytes = dac_word_count * 4;
			bk_dma_set_transfer_len(i2s_dma_rx_dac_dma_id, transfer_bytes);
			
#if CONFIG_CACHE_ENABLE
			flush_dcache((void *)i2s_dma_rx_dac_buffer, transfer_bytes);
#endif
			
			// Start next DMA transfer
			// Note: If DMA is already running, hardware will handle it appropriately
			bk_dma_start(i2s_dma_rx_dac_dma_id);
		} else {
			// No data converted (e.g., incomplete L/R pair), will retry on next I2S RX callback
		}
		} else {
		// No data or incomplete data, will retry on next I2S RX callback
	}
}

static int i2s_rx_data_handle_cb(uint32_t size)
{
	// DMA callback: DMA has transferred data from I2S RX FIFO to ring buffer
	// This callback continuously triggers DAC DMA transfer to ensure data flow
	static uint32_t r_callback_count = 0;
	r_callback_count++;
	if (r_callback_count <= 10) {
		LOGD("[RX-CB] callback #%d triggered, size:%d\n", r_callback_count, size);  // for debug
	}

	// Note: DMA has already written data to ring buffer (i2s_rx_rb)
	// Continuously trigger DAC DMA transfer to ensure data flow continues
	// This handles cases where DAC DMA might have stopped (e.g., FIFO full, data insufficient)
	if (i2s_dma_rx_dac_dma_initialized && i2s_dma_rx_dac_dma_id != DMA_ID_MAX) {
		// Check if there's sufficient data in ring buffer (at least one frame)
		// and try to start/restart DAC DMA
		if (i2s_rx_rb != NULL && i2s_dma_frame_bytes > 0) {
			uint32_t fill_size = ring_buffer_get_fill_size(i2s_rx_rb);
			// Only trigger if we have at least one complete frame
			if (fill_size >= i2s_dma_frame_bytes) {
				// Check DAC FIFO status before triggering
				uint32_t dac_fifo_status = 0;
				uint32_t expected_bit = (1 << i2s_dma_dac_source);
				bk_aud_dac_get_fifo_status(&dac_fifo_status);
				if (dac_fifo_status & expected_bit) {
					// DAC FIFO has space, trigger DMA transfer
					// Note: The ISR will check conditions again and start DMA if possible
					i2s_dma_rx_dac_dma_finish_isr(i2s_dma_rx_dac_dma_id);
				}
			}
		}
	}
	return size;
}

static void aud_i2s_mcp_test_thread(beken_thread_arg_t data)
{
	uint32_t write_flag = 0;
	uint32_t read_flag = 0;
	uint32_t data_buf = 0;
	uint32_t *test_data = NULL;
	uint32_t test_len = 0;
	uint32_t i = 0;
	i2s_samp_rate_t samp_rate = *(i2s_samp_rate_t *)data;
	if (g_i2s_16r16l_mode) {
		get_i2s_test_data_by_rate(samp_rate, (const uint32_t **)&test_data, &test_len);
	} else {
		get_i2s_test_data_by_rate_seperate(samp_rate, (const uint32_t **)&test_data, &test_len);
	}
	LOGI("[+]aud_i2s_mcp_test_thread start, type:%d, samp_rate:%d, test_len:%d\r\n", i2s_test_type, samp_rate, test_len);

	i2s_mcp_test_running = true;
	while (i2s_mcp_test_running)
	{
		if (i2s_test_type == I2S_TXRX_TYPE_TX) {
			// TX mode: write data to I2S
			bk_i2s_get_write_ready(&write_flag);
			if (write_flag) {
				bk_i2s_write_data(i2s_channel, &test_data[i], 1);
				i++;
				if (i >= test_len) {
					i = 0;
				}
			}
		} else {
			// RX mode: read data from I2S
			bk_i2s_get_read_ready(&read_flag);
			if (read_flag) {
				bk_i2s_read_data(&data_buf, 1);
				// Optional: process or discard received data
			}
		}
	}

	LOGD("[+]aud_i2s_mcp_test_thread exit\r\n");
}

static void cli_aud_i2s_mcp_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	LOGD("[+]%s\r\n", __func__);
	bk_err_t ret = BK_OK;
	i2s_config_t i2s_config = DEFAULT_I2S_CONFIG();
	i2s_samp_rate_t samp_rate = I2S_SAMP_RATE_8000;
	i2s_lrcom_store_mode_t store_mode = I2S_LRCOM_STORE_LRLR;
	uint8_t data_length = 32;

	if (argc < 2)
	{
		cli_aud_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		if (os_strcmp(argv[2], "tx") == 0) {
			i2s_test_type = I2S_TXRX_TYPE_TX;
		} else if (os_strcmp(argv[2], "rx") == 0) {
			i2s_test_type = I2S_TXRX_TYPE_RX;
		} else {
			LOGW("Invalid type, use 'tx' or 'rx'\n");
			cli_aud_help();
			return;
		}

		uint32_t rate = strtoul(argv[3], NULL, 10);
		switch (rate) {
			case 8000:
				samp_rate = I2S_SAMP_RATE_8000;
				break;
			case 16000:
				samp_rate = I2S_SAMP_RATE_16000;
				break;
			case 48000:
				samp_rate = I2S_SAMP_RATE_48000;
				break;
			case 44100:
				samp_rate = I2S_SAMP_RATE_44100;
				break;
			default:
				LOGW("Unsupported sample rate: %d\n", rate);
				cli_aud_help();
				return;
		}

		// Parse GPIO group (optional, default to group 0)
		i2s_gpio_group = I2S_GPIO_GROUP_0;
		if (argc >= 5) {
			uint32_t group = strtoul(argv[4], NULL, 10);
			if (group < I2S_GPIO_GROUP_MAX) {
				i2s_gpio_group = (i2s_gpio_group_id_t)group;
			}
		}
		if (argc >= 6) {
			uint32_t mode = strtoul(argv[5], NULL, 10);
			if (mode < I2S_LRCOM_STORE_MODE_MAX) {
				store_mode = (i2s_lrcom_store_mode_t)mode;
			}
		}
		if (argc >= 7) {
			uint32_t length = strtoul(argv[6], NULL, 10);
			if (length <= 32 && length >= 16) {
				data_length = length;
			}
		}

		i2s_channel = I2S_CHANNEL_1;
		if (argc >= 8) {
			uint32_t channel = strtoul(argv[7], NULL, 10);
			if (channel < I2S_CHANNEL_MAX) {
				i2s_channel = (i2s_channel_id_t)channel;
			}
		}
		LOGD("Starting I2S MCP test: type=%s, rate=%d, gpio_group=%d, channel=%d\n", 
			i2s_test_type == I2S_TXRX_TYPE_TX ? "TX" : "RX", rate, i2s_gpio_group, i2s_channel);

		// Init I2S driver
		ret = bk_i2s_driver_init();
		if (ret != BK_OK) {
			LOGE("bk_i2s_driver_init failed: %d\n", ret);
			return;
		}

		// Configure I2S
		i2s_config.samp_rate   = samp_rate;
		i2s_config.store_mode  = store_mode;
		i2s_config.data_length = data_length;

		if (i2s_config.store_mode == I2S_LRCOM_STORE_16R16L && data_length <= 16) {
			g_i2s_16r16l_mode = 1;
		} else {
			g_i2s_16r16l_mode = 0;
		}

		ret = bk_i2s_init(i2s_gpio_group, &i2s_config);
		if (ret != BK_OK) {
			LOGE("bk_i2s_init failed: %d\n", ret);
			bk_i2s_driver_deinit();
			return;
		}

		// Enable I2S
		ret = bk_i2s_enable(I2S_ENABLE);
		if (ret != BK_OK) {
			LOGE("bk_i2s_enable failed: %d\n", ret);
			bk_i2s_deinit();
			bk_i2s_driver_deinit();
			return;
		}
		LOGD("I2S initialized and enabled successfully\n");

		// Create thread for MCP test
		ret = rtos_create_thread(&i2s_mcp_thread,
								 BEKEN_DEFAULT_WORKER_PRIORITY - 1,
								 "i2s_mcp_thread",
								 (beken_thread_function_t)aud_i2s_mcp_test_thread,
								 1024 * 4,
								 (beken_thread_arg_t)&samp_rate);
		if (ret != BK_OK) {
			LOGE("Failed to create I2S MCP thread: %d\n", ret);
			bk_i2s_enable(I2S_DISABLE);
			bk_i2s_deinit();
			bk_i2s_driver_deinit();
			return;
		}

		i2s_mcp_test_initialized = true;
		LOGD("I2S MCP test started successfully\n");

	} else if (os_strcmp(argv[1], "stop") == 0) {
		LOGD("Stopping I2S MCP test\n");

		if (!i2s_mcp_test_initialized) {
			LOGW("I2S MCP test not started\n");
			return;
		}

		// Stop thread
		i2s_mcp_test_running = false;
		if (i2s_mcp_thread != NULL) {
			rtos_delete_thread(&i2s_mcp_thread);
			i2s_mcp_thread = NULL;
		}

		// Disable and deinit I2S
		bk_i2s_enable(I2S_DISABLE);
		bk_i2s_deinit();
		bk_i2s_driver_deinit();
		g_i2s_16r16l_mode = 0;
		i2s_mcp_test_initialized = false;
		LOGD("I2S MCP test stopped successfully\n");

	} else {
		cli_aud_help();
		return;
	}
}


// aud_i2s_dma_test_cmd start loopback call 16000 20 1 0 0
// dac_source - sample_rate - frame_duration - stereo - gpio_group
static void cli_aud_i2s_dma_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	i2s_config_t i2s_config = DEFAULT_I2S_CONFIG();
	i2s_samp_rate_t samp_rate = I2S_SAMP_RATE_8000;
	uint32_t dac_bits = 16;

	if (os_strcmp(argv[1], "start") == 0) {

		if (i2s_dma_test_initialized) {
			LOGW("I2S DMA test already started\n");
			return;
		}
		// Parameter format: start {tx|rx|loopback} [dac_source] [sample_rate] [frame_duration] [stereo] [gpio_group]
		// For loopback mode: start loopback dac_source sample_rate frame_duration stereo gpio_group
		// For RX mode: start rx [dac_source] sample_rate [frame_duration] [stereo] [gpio_group]
		// For TX mode: start tx sample_rate [frame_duration] [stereo] [gpio_group]

		if (argc < 3) {
			cli_aud_help();
			return;
		}

		i2s_txrx_type_t test_type;
		i2s_dma_loopback_mode = false;
		
		if (os_strcmp(argv[2], "tx") == 0) {
			test_type = I2S_TXRX_TYPE_TX;
		} else if (os_strcmp(argv[2], "rx") == 0) {
			test_type = I2S_TXRX_TYPE_RX;
		} else if (os_strcmp(argv[2], "loopback") == 0) {
			test_type = I2S_TXRX_TYPE_RX;    // RX mode for receiving
			i2s_dma_loopback_mode = true;    // Enable loopback mode
		} else {
			LOGW("Invalid type, use 'tx', 'rx', or 'loopback'\n");
			cli_aud_help();
			return;
		}

		// Parse parameters based on new format: dac_source - sample_rate - frame_duration - stereo - gpio_group
		uint32_t param_idx = 3;        // Start from argv[3]
		uint32_t frame_duration = 20;  // Default frame duration (ms)
		uint32_t stereo = 2;           // Default stereo mode (2 = stereo, 1 = mono)
		i2s_gpio_group_id_t gpio_group = I2S_GPIO_GROUP_0;

		if (test_type == I2S_TXRX_TYPE_RX || i2s_dma_loopback_mode) {
			if (i2s_dma_loopback_mode) {
				// Loopback mode: dac_source is required
				if (argc <= param_idx || !parse_dac_source(argv[param_idx], &i2s_dma_dac_source)) {
					LOGW("Loopback mode requires dac_source parameter (a2dp|call|hint)\n");
					cli_aud_help();
					return;
				}
				param_idx++;  // Move to next parameter
			} else {
				// RX mode: dac_source is optional
				if (argc > param_idx && parse_dac_source(argv[param_idx], &i2s_dma_dac_source)) {
					param_idx++;  // Move to next parameter
				} else {
					i2s_dma_dac_source = AUD_DAC_SOURCE_CALL; // Default DAC source if not provided
				}
			}
		}

		// Parse sample rate
		if (argc <= param_idx) {
			LOGW("Missing sample rate parameter\n");
			cli_aud_help();
			return;
		}
		uint32_t rate = strtoul(argv[param_idx], NULL, 10);
		switch (rate) {
			case 8000:
				samp_rate = I2S_SAMP_RATE_8000;
				break;
			case 16000:
				samp_rate = I2S_SAMP_RATE_16000;
				break;
			case 48000:
				samp_rate = I2S_SAMP_RATE_48000;
				break;
			case 44100:
				samp_rate = I2S_SAMP_RATE_44100;
				break;
			default:
				LOGW("Unsupported sample rate: %d\n", rate);
				cli_aud_help();
				return;
		}
		param_idx++;  // Move to next parameter

		// Parse frame_duration (optional, default 20ms)
		if (argc > param_idx) {
			frame_duration = strtoul(argv[param_idx], NULL, 10);
			param_idx++;
		}

		// Parse stereo (optional, default 1 = stereo)
		if (argc > param_idx) {
			stereo = strtoul(argv[param_idx], NULL, 10);
			param_idx++;
		}

		// Parse GPIO group (optional, default 0)
		if (argc > param_idx) {
			uint32_t group = strtoul(argv[param_idx], NULL, 10);
			if (group < I2S_GPIO_GROUP_MAX) {
				gpio_group = (i2s_gpio_group_id_t)group;
			}
			param_idx++;
		}

		// Parse dac bits
		if (argc > param_idx) {
			uint32_t bits = strtoul(argv[param_idx], NULL, 10);
			if (bits == 24) {
				dac_bits = bits;
			}
		}

		// Set GPIO groups based on mode
		if (i2s_dma_loopback_mode) {
			i2s_tx_gpio_group = gpio_group;
			i2s_rx_gpio_group = gpio_group;
			LOGD("Loopback mode: DAC source=%d, rate=%d, frame_duration=%dms, stereo=%d, gpio_group=%d\n", 
			     i2s_dma_dac_source, rate, frame_duration, stereo, gpio_group);
		} else if (test_type == I2S_TXRX_TYPE_RX) {
			LOGD("RX mode: DAC source=%d, rate=%d, frame_duration=%dms, stereo=%d, gpio_group=%d\n", 
			     i2s_dma_dac_source, rate, frame_duration, stereo, gpio_group);
		} else {
			LOGD("TX mode: rate=%d, frame_duration=%dms, stereo=%d, gpio_group=%d\n", 
			     rate, frame_duration, stereo, gpio_group);
		}

		// Init I2S driver
		ret = bk_i2s_driver_init();
		if (ret != BK_OK) {
			LOGE("bk_i2s_driver_init failed: %d\n", ret);
			return;
		}

		// Configure I2S
		i2s_config.samp_rate   = samp_rate;
		if (dac_bits == 16) {
			i2s_config.data_length = 16;
			i2s_config.store_mode  = I2S_LRCOM_STORE_16R16L;
		} else if (dac_bits == 24)
		{
			i2s_config.data_length = 32;
			i2s_config.store_mode  = I2S_LRCOM_STORE_LRLR;
		}
		LOGD("I2S config: samp_rate=%d, data_length=%d, store_mode=%d\n", samp_rate, i2s_config.data_length, i2s_config.store_mode);

		if (i2s_dma_loopback_mode) {
			ret = bk_i2s_init(i2s_tx_gpio_group, &i2s_config);
			if (ret != BK_OK) {
				LOGE("bk_i2s_init TX (gpio_group=%d) failed: %d\n", i2s_tx_gpio_group, ret);
				goto i2s_driver_deinit;
			}

			LOGD("TX I2S interface initialized (gpio_group=%d)\n", i2s_tx_gpio_group);
			// Initialize RX I2S interface
//			ret = bk_i2s_init(i2s_rx_gpio_group, &i2s_config);
//			if (ret != BK_OK) {
//				LOGE("bk_i2s_init RX (gpio_group=%d) failed: %d\n", i2s_rx_gpio_group, ret);
//				goto i2s_deinit;
//			}
//			LOGD("RX I2S interface initialized (gpio_group=%d)\n", i2s_rx_gpio_group);
		}
		else {
			// Non-loopback mode: use single I2S interface
			ret = bk_i2s_init(gpio_group, &i2s_config);
			if (ret != BK_OK) {
				LOGE("bk_i2s_init failed: %d\n", ret);
				goto i2s_driver_deinit;
			}

		}
		ret = bk_i2s_set_samp_rate(samp_rate);
		if (ret != BK_OK) {
			LOGE("bk_i2s_set_samp_rate failed: %d\n", ret);
			goto i2s_deinit;
		}
		// Initialize DAC for RX mode (I2S RX -> DAC)
		if (test_type == I2S_TXRX_TYPE_RX) {
			aud_dac_config_t dac_config = DEFAULT_AUD_DAC_CONFIG();
			uint32_t dac_sample_rate = rate;
			uint32_t dac_fifo_status = 0;

			// Hardware reset
			aud_hardware_reset();
			dac_config.bits = dac_bits;
			// Initialize DAC
			ret = bk_aud_dac_init(&dac_config);
			if (ret != BK_OK) {
				LOGE("bk_aud_dac_init failed: %d\n", ret);
				goto i2s_deinit;
			}
			LOGD("DAC initialized successfully, dac_bits:%d\n", dac_bits);

			// Configure DAC thresholds
			bk_aud_dac_spk0_set_write_threshold(i2s_dma_dac_source, 5);
			bk_aud_dac_spk0_set_read_threshold(i2s_dma_dac_source, 3);

			// Configure DAC sample rate and bit width
			bk_aud_dac_set_sample_rate(i2s_dma_dac_source, dac_sample_rate);
		//	bk_aud_dac_set_bits_width(i2s_dma_dac_source, 16);

			// Enable DAC source
			bk_aud_dac_spk0_source_enable(i2s_dma_dac_source, 1);

			// Start DAC
			ret = bk_aud_dac_start(AUD_DAC_CHL_LR);
			if (ret != BK_OK) {
				LOGE("bk_aud_dac_start failed: %d\n", ret);
				goto i2s_dac_deinit;
			}
			LOGD("DAC started successfully\n");

			// Check FIFO status after starting DAC
			bk_aud_dac_get_fifo_status(&dac_fifo_status);
			if (dac_fifo_status != (1 << i2s_dma_dac_source)) {
				LOGE("DAC FIFO status error: 0x%x, expected bit: 0x%x\n", dac_fifo_status, (1 << i2s_dma_dac_source));
				goto i2s_dac_stop;
			}
		}
		const uint32_t *test_data = NULL;
		uint32_t test_len = 0;
		get_i2s_test_data_by_rate_dbg(samp_rate, &test_data, &test_len, dac_bits);
		LOGD("I2S test data: samp_rate=%d, test_len=%d bytes (%d samples), dac_bits : %d\n", samp_rate, test_len, test_len / 4, dac_bits);

		// Calculate frame size using actual sample rate value (rate), not enum (samp_rate)
		uint32_t frame_duration_samples = frame_duration * rate / 1000;
		uint32_t frame_duration_bytes = frame_duration_samples * (dac_bits/8) * stereo; //////

		// Save frame size for use in callback
		i2s_dma_frame_bytes = frame_duration_bytes;

		// Allocate buffer for one frame of test data
		i2s_dma_tx_test_data = (uint8_t *)os_malloc(frame_duration_bytes);
		if (i2s_dma_tx_test_data == NULL) {
			LOGE("Failed to allocate memory for I2S test data\n");
			return;
		}
		// Fill frame buffer by cycling through test_data
		uint32_t bytes_filled = 0;
		while (bytes_filled < frame_duration_bytes) {
			uint32_t copy_size = (frame_duration_bytes - bytes_filled) > test_len ? test_len : (frame_duration_bytes - bytes_filled);
			os_memcpy((uint8_t *)i2s_dma_tx_test_data + bytes_filled, test_data, copy_size);
			bytes_filled += copy_size;
		}
		// Store total allocated size (one frame)
		i2s_dma_tx_test_len = frame_duration_bytes;
		
		LOGD("Frame configuration: samples=%d, bytes=%d, stereo=%d\n", frame_duration_samples, frame_duration_bytes, stereo);

		// Init DMA channel
		uint32_t buff_size = frame_duration_bytes*2;/*10*2*/
		// In loopback mode, initialize both TX and RX
		if (i2s_dma_loopback_mode)
		{
			ret = bk_i2s_chl_init(I2S_CHANNEL_1, I2S_TXRX_TYPE_TX, buff_size, i2s_tx_data_handle_cb, &i2s_tx_rb);
			if (ret != BK_OK) {
				LOGE("bk_i2s_chl_init TX failed: %d\n", ret);
				goto i2s_dac_stop;
			}

			// Pre-fill TX ring buffer with zeros
			// Pre-fill at least one frame to ensure frame alignment
			// Round down to frame boundary to ensure frame alignment
			uint32_t prefill_size = (buff_size / 2) / i2s_dma_frame_bytes * i2s_dma_frame_bytes;
			if (prefill_size < i2s_dma_frame_bytes) {
				prefill_size = i2s_dma_frame_bytes;  // At least one frame
			}

			// Check if ring buffer has enough space
			uint32_t free_size = ring_buffer_get_free_size(i2s_tx_rb);
			if (free_size < prefill_size) {
				prefill_size = free_size / i2s_dma_frame_bytes * i2s_dma_frame_bytes;  // Round down to frame boundary
				if (prefill_size < i2s_dma_frame_bytes) {
					LOGW("WARNING: Not enough space for pre-fill (free=%d, need=%d)\n", free_size, i2s_dma_frame_bytes);
				}
			}

			if (prefill_size >= i2s_dma_frame_bytes) {
				uint8_t *temp_data = (uint8_t *)os_malloc(prefill_size);
				if (temp_data != NULL) {
					os_memset(temp_data, 0x00, prefill_size);
					uint32_t written = ring_buffer_write(i2s_tx_rb, temp_data, prefill_size);
					os_free(temp_data);
					LOGD("TX ring buffer pre-filled with %d bytes (%.1f frames) of zeros\n", written, (float)written / i2s_dma_frame_bytes);
				} else {
					LOGW("WARNING: Failed to allocate memory for pre-fill\n");
				}
			}

			// Initialize RX channel (ensure RX I2S interface is selected)
			// Re-initialize to set correct I2S interface index for RX
			ret = bk_i2s_chl_init(I2S_CHANNEL_1, I2S_TXRX_TYPE_RX, buff_size, i2s_rx_data_handle_cb, &i2s_rx_rb);
			if (ret != BK_OK) {
				LOGE("bk_i2s_chl_init RX failed: %d\n", ret);
				bk_i2s_chl_deinit(I2S_CHANNEL_1, I2S_TXRX_TYPE_TX);
				goto i2s_dac_stop;
			}
			LOGD("Both TX and RX DMA channels initialized for loopback mode\n");
		} 
		else if (test_type == I2S_TXRX_TYPE_TX) {
			ret = bk_i2s_chl_init(I2S_CHANNEL_1, I2S_TXRX_TYPE_TX, buff_size, i2s_tx_data_handle_cb, &i2s_tx_rb);
			if (ret != BK_OK) {
				LOGE("bk_i2s_chl_init TX failed: %d\n", ret);
				goto i2s_deinit;
			}

			// Pre-fill ring buffer with zeros
			// Round down to frame boundary to ensure frame alignment
			uint32_t prefill_size = (buff_size / 2) / i2s_dma_frame_bytes * i2s_dma_frame_bytes;
			if (prefill_size < i2s_dma_frame_bytes) {
				prefill_size = i2s_dma_frame_bytes;  // At least one frame
			}

			// Check if ring buffer has enough space
			uint32_t free_size = ring_buffer_get_free_size(i2s_tx_rb);
			if (free_size < prefill_size) {
				prefill_size = free_size / i2s_dma_frame_bytes * i2s_dma_frame_bytes;  // Round down to frame boundary
				if (prefill_size < i2s_dma_frame_bytes) {
					LOGW("WARNING: Not enough space for pre-fill (free=%d, need=%d)\n", free_size, i2s_dma_frame_bytes);
				}
			}

			if (prefill_size >= i2s_dma_frame_bytes) {
				uint8_t *temp_data = (uint8_t *)os_malloc(prefill_size);
				if (temp_data != NULL) {
					os_memset(temp_data, 0, prefill_size);
					uint32_t written = ring_buffer_write(i2s_tx_rb, temp_data, prefill_size);
					os_free(temp_data);
					LOGD("TX ring buffer pre-filled with %d bytes (%.1f frames) of zeros\n", written, (float)written / i2s_dma_frame_bytes);
				} else {
					LOGW("WARNING: Failed to allocate memory for pre-fill\n");
				}
			}
		} 
		else {
			ret = bk_i2s_chl_init(I2S_CHANNEL_1, I2S_TXRX_TYPE_RX, buff_size, i2s_rx_data_handle_cb, &i2s_rx_rb);
			if (ret != BK_OK) {
				LOGE("bk_i2s_chl_init RX failed: %d\n", ret);
				goto i2s_dac_stop;
			}
		}

		// Enable I2S
		if (i2s_dma_loopback_mode) {
			// Loopback mode: enable both TX and RX I2S interfaces
			// Enable TX I2S interface
			ret = bk_i2s_enable(I2S_ENABLE);
			if (ret != BK_OK) {
				LOGE("bk_i2s_enable RX failed: %d\n", ret);
				goto i2s_chnl_deinit;
			}
			LOGD("RX I2S interface enabled (gpio_group=%d)\n", i2s_rx_gpio_group);
		} 
		else {
			// Non-loopback mode: enable single I2S interface
			ret = bk_i2s_enable(I2S_ENABLE);
			if (ret != BK_OK) {
				LOGE("bk_i2s_enable failed: %d\n", ret);
				// Cleanup channels
				bk_i2s_chl_deinit(I2S_CHANNEL_1, test_type);
				// Cleanup DAC if RX mode
				if (test_type == I2S_TXRX_TYPE_RX) {
					goto i2s_dac_stop;
				}
				goto i2s_deinit;
			}
			LOGD("I2S enabled successfully\n");
		}

		// Start I2S
		if (i2s_dma_loopback_mode) {
			// Loopback mode: start both TX and RX I2S interfaces
			// Start TX I2S interface
			// Check TX ring buffer fill status before starting
			if (i2s_tx_rb != NULL) {
				uint32_t tx_rb_fill = ring_buffer_get_fill_size(i2s_tx_rb);
				uint32_t tx_rb_free = ring_buffer_get_free_size(i2s_tx_rb);
				LOGD("TX ring buffer before start: fill=%d bytes, free=%d bytes\n", tx_rb_fill, tx_rb_free);
			} else {
				LOGE("ERROR: TX ring buffer is NULL before start!\n");
			}
			LOGD("Starting TX I2S (gpio_group=%d)...\n", i2s_tx_gpio_group);
			ret = bk_i2s_start();
			if (ret != BK_OK) {
				LOGE("bk_i2s_start TX failed: %d\n", ret);
				goto i2s_chnl_deinit;
			}
			LOGD("TX I2S interface started (gpio_group=%d)\n", i2s_tx_gpio_group);

			// Small delay to let TX I2S start and check TX FIFO status
			rtos_delay_milliseconds(50);
			uint32_t tx_write_flag = 0;
			bk_i2s_get_write_ready(&tx_write_flag);
			LOGD("TX I2S FIFO write ready flag: %d (after 50ms delay)\n", tx_write_flag);

			// Start RX I2S interface
			// Check RX ring buffer status
			if (i2s_rx_rb != NULL) {
				uint32_t rx_rb_fill = ring_buffer_get_fill_size(i2s_rx_rb);
				uint32_t rx_rb_free = ring_buffer_get_free_size(i2s_rx_rb);
				LOGD("RX ring buffer after start: fill=%d bytes, free=%d bytes\n", rx_rb_fill, rx_rb_free);
			}

			// Small delay to let I2S start and check if RX FIFO has data
			rtos_delay_milliseconds(100);
			uint32_t rx_read_flag = 0;
			bk_i2s_get_read_ready(&rx_read_flag);
			LOGD("RX I2S FIFO read ready flag: %d (after 100ms delay)\n", rx_read_flag);
		} 
		else {
			// Non-loopback mode: start single I2S interface
			ret = bk_i2s_start();
			if (ret != BK_OK) {
				LOGE("bk_i2s_start failed: %d\n", ret);
				// Cleanup channels
				bk_i2s_chl_deinit(I2S_CHANNEL_1, test_type);
				// Cleanup DAC if RX mode
				if (test_type == I2S_TXRX_TYPE_RX) {
					goto i2s_dac_stop;
				}
				goto i2s_deinit;
			}
		}

		// Configure DMA for RX ring buffer to DAC FIFO
		if (test_type == I2S_TXRX_TYPE_RX && i2s_rx_rb != NULL) {
			// Allocate intermediate buffer for format conversion
			i2s_dma_rx_dac_buffer_size = I2S_DMA_RX_DAC_BUFFER_SIZE;
			i2s_dma_rx_dac_buffer = (uint32_t *)os_malloc(i2s_dma_rx_dac_buffer_size);
			if (i2s_dma_rx_dac_buffer == NULL) {
				LOGE("Failed to allocate RX DAC DMA buffer\n");
			} else {
				os_memset(i2s_dma_rx_dac_buffer, 0, i2s_dma_rx_dac_buffer_size);

				// Allocate DMA channel
				i2s_dma_rx_dac_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
				if ((i2s_dma_rx_dac_dma_id < DMA_ID_0) || (i2s_dma_rx_dac_dma_id >= DMA_ID_MAX)) {
					LOGE("Failed to allocate DMA channel for RX DAC\n");
					os_free(i2s_dma_rx_dac_buffer);
					i2s_dma_rx_dac_buffer = NULL;
				} else {
					// Configure DMA: from intermediate buffer to DAC FIFO
					dma_config_t dma_config = {0};
					uint32_t dac_fifo_addr = 0;

					os_memset(&dma_config, 0, sizeof(dma_config));
					dma_config.mode = DMA_WORK_MODE_REPEAT;
					dma_config.chan_prio = 1;
					dma_config.src.dev = DMA_DEV_DTCM;
					dma_config.src.width = DMA_DATA_WIDTH_32BITS;
					dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
					dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
					dma_config.src.start_addr = (uint32_t)i2s_dma_rx_dac_buffer;
					dma_config.src.end_addr = (uint32_t)i2s_dma_rx_dac_buffer + i2s_dma_rx_dac_buffer_size;

					// Get DAC FIFO address based on source
					if (!get_dac_fifo_addr_by_source(i2s_dma_dac_source, &dma_config.dst.dev, &dac_fifo_addr)) {
						os_printf("Unsupported DAC source: %d\n", i2s_dma_dac_source);
						bk_dma_free(DMA_DEV_AUDIO, i2s_dma_rx_dac_dma_id);
						i2s_dma_rx_dac_dma_id = DMA_ID_MAX;
						os_free(i2s_dma_rx_dac_buffer);
						i2s_dma_rx_dac_buffer = NULL;
					}

					if (i2s_dma_rx_dac_buffer != NULL) {
						dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
						dma_config.dst.addr_inc_en = DMA_ADDR_INC_DISABLE;  // FIFO address is fixed
						dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_DISABLE;
						dma_config.dst.start_addr = dac_fifo_addr;
						dma_config.dst.end_addr = dac_fifo_addr + 4;

						// Initialize DMA
						ret = bk_dma_init(i2s_dma_rx_dac_dma_id, &dma_config);
						if (ret != BK_OK) {
							os_printf("Failed to init DMA for RX DAC: %d\n", ret);
							bk_dma_free(DMA_DEV_AUDIO, i2s_dma_rx_dac_dma_id);
							i2s_dma_rx_dac_dma_id = DMA_ID_MAX;
							os_free(i2s_dma_rx_dac_buffer);
							i2s_dma_rx_dac_buffer = NULL;
						} else {
							// Register ISR and enable interrupt
							bk_dma_register_isr(i2s_dma_rx_dac_dma_id, NULL, (void *)i2s_dma_rx_dac_dma_finish_isr);
							bk_dma_enable_finish_interrupt(i2s_dma_rx_dac_dma_id);

#if (CONFIG_SPE)
							bk_dma_set_dest_sec_attr(i2s_dma_rx_dac_dma_id, DMA_ATTR_SEC);
							bk_dma_set_src_sec_attr(i2s_dma_rx_dac_dma_id, DMA_ATTR_SEC);
#endif

							i2s_dma_rx_dac_dma_initialized = true;
							LOGD("RX DAC DMA configured successfully (dma_id=%d)\n", i2s_dma_rx_dac_dma_id);

							// Trigger first DMA transfer by calling ISR manually (or wait for first data)
							// Actually, we should wait for I2S RX to have data first
						}
					}
				}
			}
		}

		i2s_dma_test_initialized = true;
		LOGD("I2S DMA test started successfully\n");
		return;
	} 
	else if (os_strcmp(argv[1], "stop") == 0) {
		LOGD("Stopping I2S DMA test\n");

		if (!i2s_dma_test_initialized) {
			LOGD("I2S DMA test not started\n");
			return;
		}

		// First, stop and cleanup RX DAC DMA (before stopping I2S)
		if (i2s_dma_rx_dac_dma_initialized && i2s_dma_rx_dac_dma_id != DMA_ID_MAX) {
			bk_dma_stop(i2s_dma_rx_dac_dma_id);
			bk_dma_register_isr(i2s_dma_rx_dac_dma_id, NULL, NULL);
			bk_dma_deinit(i2s_dma_rx_dac_dma_id);
			bk_dma_free(DMA_DEV_AUDIO, i2s_dma_rx_dac_dma_id);
			i2s_dma_rx_dac_dma_id = DMA_ID_MAX;
			i2s_dma_rx_dac_dma_initialized = false;
		}
		if (i2s_dma_rx_dac_buffer != NULL) {
			os_free(i2s_dma_rx_dac_buffer);
			i2s_dma_rx_dac_buffer = NULL;
			i2s_dma_rx_dac_buffer_size = 0;
		}

		// Stop DAC (if used in RX mode or loopback mode)
		if (i2s_rx_rb != NULL) {
			bk_aud_dac_stop(AUD_DAC_CHL_LR);
			bk_aud_dac_spk0_source_enable(i2s_dma_dac_source, 0);
			bk_aud_dac_deinit();
			LOGD("DAC stopped and deinitialized\n");
		}

		// Stop I2S - try to stop without re-initializing to avoid accessing invalid memory
		// Note: I2S should already be initialized from start, so we can directly stop it
		// Non-loopback mode: stop single I2S interface
		ret = bk_i2s_stop();
		if (ret != BK_OK) {
			LOGE("bk_i2s_stop failed: %d (may be already stopped)\n", ret);
		}
		ret = bk_i2s_enable(I2S_DISABLE);
		if (ret != BK_OK) {
			LOGE("bk_i2s_enable(DISABLE) failed: %d (may be already disabled)\n", ret);
		}

		// Deinit channel (need to determine type from previous state)
		// For simplicity, try both
		if (i2s_tx_rb != NULL) {
			// Try to deinit TX channel (may fail if already deinitialized, but that's OK)
			bk_i2s_chl_deinit(I2S_CHANNEL_1, I2S_TXRX_TYPE_TX);
			i2s_tx_rb = NULL;
		}
		if (i2s_rx_rb != NULL) {
			// Try to deinit RX channel (may fail if already deinitialized, but that's OK)
			bk_i2s_chl_deinit(I2S_CHANNEL_1, I2S_TXRX_TYPE_RX);
			i2s_rx_rb = NULL;
		}

		// Deinit I2S interfaces - try to deinit without re-initializing
		// This avoids accessing potentially invalid memory in bk_i2s_init
		bk_i2s_deinit();
		bk_i2s_driver_deinit();

		// Free allocated test data buffer
		if (i2s_dma_tx_test_data != NULL) {
			os_free(i2s_dma_tx_test_data);
			i2s_dma_tx_test_data = NULL;
			i2s_dma_tx_test_len = 0;
			i2s_dma_frame_bytes = 0;
		}

		i2s_dma_test_initialized = false;
		i2s_dma_loopback_mode = false;
		LOGD("I2S DMA test stopped successfully\n");
		return;
	} else {
		cli_aud_help();
		return;
	}

i2s_chnl_deinit:
	bk_i2s_chl_deinit(I2S_CHANNEL_1, I2S_TXRX_TYPE_TX);
	bk_i2s_chl_deinit(I2S_CHANNEL_1, I2S_TXRX_TYPE_RX);
i2s_dac_stop:
	bk_aud_dac_stop(AUD_DAC_CHL_LR);
i2s_dac_deinit:
	bk_aud_dac_spk0_source_enable(i2s_dma_dac_source, 0);
	bk_aud_dac_deinit();
i2s_deinit:
	bk_i2s_deinit();
i2s_driver_deinit:
	bk_i2s_driver_deinit();
	return;
}

#endif // CONFIG_I2S

#if CONFIG_XDAC
static RingBufferContext *xdac_rb = NULL;
static int32_t *xdac_ring_buff = NULL;
static dma_id_t xdac_dma_id = DMA_ID_MAX;
static xdac_ch_t xdac_ch = XDAC_CH_CNT;
static xdac_ch_t xdac_start_flag = 0;

uint32_t dma_isr_cnt = 0;

static void xdac_dma_test_finish_isr(dma_id_t dma_id)
{
	uint32_t write_size = 0;
	uint32_t free_size = 0;
	uint32_t total_bytes = signal_size * 2;  //samples*16bits Total audio data size in bytes

	if (xdac_rb == NULL || dma_id != xdac_dma_id) {
        LOGE("[DMA ISR] rb:0x%x or dma_id:%d is invalid\n",xdac_rb,dma_id);
		return;
	}

#if CONFIG_CACHE_ENABLE
	flush_all_dcache();
#endif

	// Check free space in ring buffer
	free_size = ring_buffer_get_free_size(xdac_rb);
	if (free_size < total_bytes) {
		// Not enough space, skip this time
		LOGE("Ring buffer full, skip fill (free=%d, need=%d)\n", free_size, total_bytes);
		return;
	}

	// Write entire audio array to ring buffer
	write_size = ring_buffer_write(xdac_rb, (uint8_t *)signal, total_bytes);

	if (write_size > 0) {
		// Data written successfully, will loop automatically when DMA finishes transfer
		if(0 == (++dma_isr_cnt % 30))
		{
			LOGD("[DMA ISR] w_size:%d\n",write_size);
		}
	} else {
		LOGE("[DMA ISR] write failed, write_size=0\n");
	}
}

static void cli_xdac_dma_ringbuf_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_err_t ret = BK_OK;
    dma_config_t dma_config = {0};
    uint32_t dac_fifo_addr;
    uint32_t *data_ptr = NULL;
    uint32_t data_len = 0;
    uint32_t ring_buff_size = 0;
    dma_dev_t xdac_ch_dma_dev_map[XDAC_CH_CNT] = {DMA_DEV_XDAC0,DMA_DEV_XDAC1};

    if (argc != 2 && argc != 4) {
        cli_aud_help();
        return;
    }

    if (os_strcmp(argv[1], "start") == 0)
    {
        if (signal != NULL) {
            data_ptr = (uint32_t *)signal;
            data_len = signal_size;
        }
        else
        {
            LOGE("xdac test fail,need generate test data first!\n");
            goto xdac_start_fail;
        }

        LOGI("xdac test start, data_len: %d\n", data_len);

        ret = bk_xdac_driver_init();
        if (ret != BK_OK) {
            LOGE("bk_xdac_driver_init fail \n");
            goto xdac_start_fail;
        }

        ret = bk_xdac_alloc(&xdac_ch);
        if (ret != BK_OK) {
            LOGE("bk_xdac_alloc fail \n");
            goto xdac_start_fail;
        }
        LOGI("xdac_ch: %d\n", xdac_ch);
        LOGI("bk_xdac_driver_init successful\n");

        // Ring buffer size: entire audio array size (aud_len * 4 bytes) * 2 + safe interval
        ring_buff_size = data_len * 2 * 2 + DAC_DMA_RING_BUFF_SAFE_INTERVAL;

        // Allocate ring buffer for DMA
        xdac_ring_buff = (int32_t *)os_malloc(ring_buff_size);
        if (xdac_ring_buff == NULL) {
            LOGE("Failed to allocate DMA ring buffer\n");
            goto xdac_start_fail;
        }
        os_memset(xdac_ring_buff, 0, ring_buff_size);

        // Initialize ring buffer context
        xdac_rb = (RingBufferContext *)os_malloc(sizeof(RingBufferContext));
        if (xdac_rb == NULL) {
            LOGE("Failed to allocate ring buffer context\n");
            goto xdac_start_fail;
        }

        //init dma driver
        ret = bk_dma_driver_init();
        if (ret != BK_OK) {
            LOGE("dma driver init failed\r\n");
            goto xdac_start_fail;
        }

        // Allocate DMA channel
        xdac_dma_id = bk_dma_alloc(xdac_ch_dma_dev_map[xdac_ch]);
        if ((xdac_dma_id < DMA_ID_0) || (xdac_dma_id >= DMA_ID_MAX)) {
            LOGE("malloc dma fail \r\n");
            goto xdac_start_fail;
        }
        LOGI("dma_id: %d\n", xdac_dma_id);

        // Configure DMA (similar to aud_tras_dac_dma_config)
        os_memset(&dma_config, 0, sizeof(dma_config));
        dma_config.mode = DMA_WORK_MODE_REPEAT;
        dma_config.chan_prio = 1;
        dma_config.src.dev = DMA_DEV_DTCM;
        dma_config.src.width = DMA_DATA_WIDTH_32BITS;
        dma_config.dst.dev = xdac_ch_dma_dev_map[xdac_ch];

        // Get DAC FIFO address based on source
        if (BK_OK != bk_xdac_get_fifo_addr(xdac_ch, &dac_fifo_addr)) {
            LOGE("get xdac fifo address failed\r\n");
            goto xdac_start_fail;
        }

        dma_config.dst.width = DMA_DATA_WIDTH_16BITS;
        dma_config.dst.addr_inc_en = DMA_ADDR_INC_DISABLE;
        dma_config.dst.start_addr = dac_fifo_addr;

        // Source is ring buffer
        dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
        dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
        dma_config.src.start_addr = (uint32_t)xdac_ring_buff;
        dma_config.src.end_addr = (uint32_t)xdac_ring_buff + ring_buff_size;

        LOGI("DMA config: src=0x%x~0x%x, dst=0x%x, transfer_len=%d, ring_buff_size=%d\r\n",
             dma_config.src.start_addr, dma_config.src.end_addr, dma_config.dst.start_addr,
             data_len * 2, ring_buff_size);

        // Initialize DMA channel
        ret = bk_dma_init(xdac_dma_id, &dma_config);
        if (ret != BK_OK) {
            LOGE("dma init failed\r\n");
            goto xdac_start_fail;
        }

        // Set transfer length: entire audio array size
        bk_dma_set_transfer_len(xdac_dma_id, data_len * 2);


        // Initialize ring buffer with DMA association
        ring_buffer_init(xdac_rb, (uint8_t*)xdac_ring_buff, ring_buff_size, 
                         xdac_dma_id, RB_DMA_TYPE_READ);

        // Register ISR and enable interrupt
        bk_dma_register_isr(xdac_dma_id, NULL, (void *)xdac_dma_test_finish_isr);
        bk_dma_enable_finish_interrupt(xdac_dma_id);

#if (CONFIG_SPE)
        bk_dma_set_dest_sec_attr(xdac_dma_id, DMA_ATTR_SEC);
        bk_dma_set_src_sec_attr(xdac_dma_id, DMA_ATTR_SEC);
#endif

        // Pre-fill ring buffer with entire audio array
        // This ensures DMA has enough data to start immediately
        LOGI("xdac_rb addr:0x%x,len:%d,dma_id:%d,dma_type:%d,rp:%d,wp:%d,dma_en:%d\n",
             xdac_rb->address,xdac_rb->capacity,xdac_rb->dma_id,xdac_rb->dma_type,xdac_rb->rp,xdac_rb->wp,
             bk_dma_get_enable_status(xdac_dma_id));
        uint32_t write_size = ring_buffer_write(xdac_rb, (uint8_t*)data_ptr, data_len*2);
        ring_buffer_write(xdac_rb, (uint8_t*)data_ptr, data_len*2);
        LOGI("prefill: wrote %d bytes, ring buffer fill: %d bytes\n",
             write_size, ring_buffer_get_fill_size(xdac_rb));

#if CONFIG_CACHE_ENABLE
        flush_dcache((void *)xdac_ring_buff, ring_buff_size);
#endif

        // Start DAC before DMA (DAC must be ready to receive data from FIFO)
        ret = bk_xdac_start(xdac_ch);
        if (ret != BK_OK) {
            LOGE("bk_xdac_start fail\n");
            goto xdac_start_fail;
        }

        // Start DMA after DAC is ready (similar to reference implementation)
        ret = bk_dma_start(xdac_dma_id);
        if (ret != BK_OK) {
            LOGE("bk_dma_start fail\n");
            goto xdac_start_fail;
        }

        while(!bk_dma_get_enable_status(xdac_dma_id));

        //enable xdac fifo int after dma started
        bk_xdac_set_int_en(xdac_ch, XDAC_EMPTY_INT, 1);
        bk_xdac_set_int_en(xdac_ch, XDAC_FULL_INT, 1);
        bk_xdac_set_int_en(xdac_ch, XDAC_NEAR_FULL_INT, 1);
        bk_xdac_set_int_en(xdac_ch, XDAC_NEAR_EMPTY_INT, 1);

        xdac_start_flag = 1;
        LOGI("xdac test successful\n");
        return;

    xdac_start_fail:
        if((DMA_ID_MAX != xdac_dma_id) && (XDAC_CH_CNT != xdac_ch))
        {
            bk_dma_stop(xdac_dma_id);
            bk_dma_register_isr(xdac_dma_id, NULL, NULL);
            bk_dma_deinit(xdac_dma_id);
            bk_dma_free(xdac_ch_dma_dev_map[xdac_ch], xdac_dma_id);
        }

        bk_xdac_driver_deinit();

        if(xdac_ring_buff)
        {
            os_free(xdac_ring_buff);
            xdac_ring_buff = NULL;
        }

        if(xdac_rb)
        {
            os_free(xdac_rb);
            xdac_rb = NULL;
        }

        xdac_dma_id = DMA_ID_MAX;
        xdac_ch = XDAC_CH_CNT;
    }
    else if (os_strcmp(argv[1], "stop") == 0) {
        LOGI("xdac test stop\n");

        if (!xdac_start_flag) {
            LOGI("xdac test not started\n");
            return;
        }

        //deinit XDAC
        ret = bk_xdac_deinit(xdac_ch);
        if (ret == BK_OK) {
            LOGI("free xdac[%d] deinit success\r\n", xdac_ch);
        }

        // Stop DMA
        ret = bk_dma_stop(xdac_dma_id);
        bk_dma_register_isr(xdac_dma_id, NULL, NULL);

        // Deinit DMA
        bk_dma_deinit(xdac_dma_id);
        ret = bk_dma_free(xdac_ch_dma_dev_map[xdac_ch], xdac_dma_id);
        if (ret == BK_OK) {
            LOGI("free dev:%d, dma: %d success\r\n", xdac_ch_dma_dev_map[xdac_ch], xdac_dma_id);
        }

        // Cleanup ring buffer
        if (xdac_rb) {
            ring_buffer_clear(xdac_rb);
            os_free(xdac_rb);
            xdac_rb = NULL;
        }
        if (xdac_ring_buff) {
            os_free(xdac_ring_buff);
            xdac_ring_buff = NULL;
        }

        xdac_dma_id = DMA_ID_MAX;
        xdac_ch = XDAC_CH_CNT;
        xdac_start_flag = 0;

        LOGI("xdac test stop successfully\n");
    } else {
        cli_aud_help();
        return;
    }
}

#endif

#define AUD_CMD_CNT (sizeof(s_aud_commands) / sizeof(struct cli_command))
static const struct cli_command s_aud_commands[] = {
#if CONFIG_AUDIO_DAC

#if CONFIG_AUDIO_DTMF
	{"aud_dtmf_mcp_test", "aud_dtmf_mcp_test {start|stop}", cli_aud_dtmf_mcp_test_cmd},
	{"aud_dtmf_loop_test", "aud_dtmf_loop_test {start|stop}", cli_aud_dtmf_loop_test_cmd},
#endif

#if CONFIG_AUDIO_ADC
	{"aud_adc_mcp_test", "aud_adc_mcp_test {start|stop sample_rate}", cli_aud_adc_mcp_test_cmd},
	{"aud_adc_mcp_dma_test", "aud_adc_mcp_test {start|stop sample_rate}", cli_aud_adc_mcp_test_dma_cmd},
	{"aud_adc_dac_dma_loopback_test", "aud_adc_dac_dma_loopback_test {start|stop|dump} {a2dp|call|hint} {sample_rate} [adc|dac|off]", cli_aud_adc_dac_dma_loopback_test_cmd},
#if CONFIG_AUDIO_RING_BUFF
	{"aud_adc_dac_ringbuf_loopback_test", "aud_adc_dac_ringbuf_loopback_test {start|stop} {a2dp|call|hint} {sample_rate}", cli_aud_adc_dac_ringbuf_loopback_test_cmd},
#endif
	//{"aud_adc_loop_test", "aud_adc_loop_test {start|stop sample_rate}", cli_aud_adc_loop_test_cmd},
	//{"aud_dac_eq_test", "aud_dac_eq_test {start|stop}", cli_aud_dac_eq_test_cmd},
#endif

#if 0//CONFIG_AUDIO_DMIC
	{"aud_dmic_mcp_test", "aud_adc_mcp_test {start|stop sample_rate}", cli_aud_dmic_mcp_test_cmd},
	{"aud_dmic_dma_test", "aud_adc_dma_test {start|stop sample_rate}", cli_aud_dmic_dma_test_cmd},
	{"aud_dmic_loop_test", "aud_adc_loop_test {start|stop sample_rate}", cli_aud_dmic_loop_test_cmd},
#endif

#if CONFIG_AUDIO_DAC
	{"aud_dac_mcp_test",         "aud_dac_mcp_test {start|stop} {a2dp|call|hint} {sample_rate}",         cli_aud_dac_mcp_test_cmd},
	{"aud_dac_dma_test",         "aud_dac_dma_test {start|stop} {a2dp|call|hint} {sample_rate}",         cli_aud_dac_dma_test_cmd},
	{"aud_dac_dma_ringbuf_test", "aud_dac_dma_ringbuf_test {start|stop} {a2dp|call|hint} {sample_rate}", cli_aud_dac_dma_ringbuf_test_cmd},
#endif

#if (CONFIG_AUDIO_DAC && CONFIG_AUDIO_RING_BUFF)
	{"aud_dac_ringbuf_test", "aud_dac_ringbuf_test {start|stop} {a2dp|call|hint} {sample_rate}", cli_aud_dac_ringbuf_test_cmd},
#endif

#ifdef AUD_ASDF_DEBUG
	{"aud_asdf_test", "aud_asdf_test {element|pipeline|event id}", cli_aud_asdf_test_cmd},
#endif
#endif

#if CONFIG_I2S
	{"aud_i2s_mcp_test", "aud_i2s_mcp_test {start|stop} {tx|rx} {sample_rate} [gpio_group]", cli_aud_i2s_mcp_test_cmd},
	{"aud_i2s_dma_test", "aud_i2s_dma_test {start|stop} {tx|rx|loopback} [dac_source] {sample_rate} [frame_duration] [stereo] [gpio_group]", cli_aud_i2s_dma_test_cmd},
#endif

#if (CONFIG_I2S && CONFIG_AUDIO_ADC && CONFIG_AUDIO_DAC)
	{"aud_adc_i2s_test", "aud_adc_i2s_test {start|stop} {sample_rate} [gpio_group]", cli_aud_adc_i2s_test_cmd},
	{"aud_i2s_dac_test", "aud_i2s_dac_test {start|stop} {a2dp|call|hint} {sample_rate} [gpio_group]", cli_aud_i2s_dac_test_cmd},
	{"aud_adc_i2s_dac_loopback_test", "aud_adc_i2s_dac_loopback_test {start|stop} {a2dp|call|hint} {sample_rate} [gpio_group]", cli_aud_adc_i2s_dac_loopback_test_cmd},
#if CONFIG_AUDIO_RING_BUFF
	{"aud_adc_i2s_ringbuf_test", "aud_adc_i2s_ringbuf_test {start|stop} {sample_rate} [gpio_group]", cli_aud_adc_i2s_ringbuf_test_cmd},
	{"aud_i2s_dac_ringbuf_test", "aud_i2s_dac_ringbuf_test {start|stop} {a2dp|call|hint} {sample_rate} [gpio_group]", cli_aud_i2s_dac_ringbuf_test_cmd},
#endif
#endif

#if CONFIG_XDAC
	{"xdac_dma_ringbuf_test", "xdac_dma_ringbuf_test {start|stop}", cli_xdac_dma_ringbuf_test_cmd},
#endif

	{"aud_generate_pcm_test", "aud_generate_pcm_test {gen|dump} {sample_rate} {frequency} {amp_db} {duration_seconds}", cli_aud_generate_pcm_test_cmd},
};

int cli_aud_init(void)
{
	audio_uart1_init();
	return cli_register_commands(s_aud_commands, AUD_CMD_CNT);
}

