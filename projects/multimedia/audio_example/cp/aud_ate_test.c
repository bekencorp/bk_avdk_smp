
#if 1//CONFIG_ATE_TEST
#include "sys_driver.h"

#include <driver/aud_adc_types.h>
#include <driver/aud_adc.h>
#include <driver/aud_dac_types.h>
#include <driver/aud_dac.h>
#include <driver/i2s.h>
#include <driver/i2s_types.h>
#include <os/os.h>
#include <os/mem.h>
#include <driver/dma.h>
#include "sys_driver.h"

#define TAG "aud_ate"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#if CONFIG_AUDIO_RING_BUFF

extern const uint32_t AUD_PCM_8000_24BITS[8];
extern const uint32_t AUD_PCM_16000_24BITS[16];
extern const uint32_t AUD_PCM_44100_24BITS[441];
extern const uint32_t AUD_PCM_48000_24BITS[48];

static void aud_hardware_reset(void)
{
	/* Reset audio registers: AUD_REG_0x2 (0x4101a008) and AUD_REG_0x5E (0x4101a178) */
	*((volatile UINT32 *)(0x4101a008)) = 0x1;
	*((volatile UINT32 *)(0x4101a178)) = 0x43210;
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

static bool get_dac_test_data_by_rate(uint32_t sample_rate, const uint32_t **data_ptr, uint32_t *data_len)
{
    if (data_ptr == NULL || data_len == NULL) {
        return false;
    }
    switch (sample_rate) {
        case 8000:
            *data_ptr = AUD_PCM_8000_24BITS;
            *data_len = sizeof(AUD_PCM_8000_24BITS) / sizeof(AUD_PCM_8000_24BITS[0]);
            break;
        case 16000:
            *data_ptr = AUD_PCM_16000_24BITS;
            *data_len = sizeof(AUD_PCM_16000_24BITS) / sizeof(AUD_PCM_16000_24BITS[0]);
            break;
        case 44100:
            *data_ptr = AUD_PCM_44100_24BITS;
            *data_len = sizeof(AUD_PCM_44100_24BITS) / sizeof(AUD_PCM_44100_24BITS[0]);
            break;
        case 48000:
            *data_ptr = AUD_PCM_48000_24BITS;
            *data_len = sizeof(AUD_PCM_48000_24BITS) / sizeof(AUD_PCM_48000_24BITS[0]);
            break;
        default:
            *data_ptr = NULL;
            *data_len = 0;
            return false;
    }
    return true;
}

static uint32_t get_samplerate(UINT8 sample_idx)
{
	switch (sample_idx) {
		case 1:
			return 8000;
		case 4:
			return 16000;
		case 8:
			return 44100;
		case 9:
			return 48000;
		default:
			return 0;
	}
}

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
static uint8_t __maybe_unused g_adc_i2s_ringbuf_dump_mode = 0;
static uint8_t __maybe_unused g_i2s_dac_ringbuf_dump_mode = 0;

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
static uint32_t g_transfer_len = 0;

#define ADC_FRAME_DURATION   (20)
#define DMIC_DBG             (3)

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
		psram_free(i2s_dac_ringbuf_transfer_buf);
		i2s_dac_ringbuf_transfer_buf = NULL;
	}
	if (i2s_dac_ringbuf_i2s_rb != NULL) {
		psram_free(i2s_dac_ringbuf_i2s_rb);
		i2s_dac_ringbuf_i2s_rb = NULL;
	}
	if (i2s_dac_ringbuf_i2s_buffer != NULL) {
		psram_free(i2s_dac_ringbuf_i2s_buffer);
		i2s_dac_ringbuf_i2s_buffer = NULL;
	}
	if (i2s_dac_ringbuf_dac_rb != NULL) {
		psram_free(i2s_dac_ringbuf_dac_rb);
		i2s_dac_ringbuf_dac_rb = NULL;
	}
	if (i2s_dac_ringbuf_dac_buffer != NULL) {
		psram_free(i2s_dac_ringbuf_dac_buffer);
		i2s_dac_ringbuf_dac_buffer = NULL;
	}
	bk_dma_driver_deinit();
	bk_aud_dac_deinit();
	bk_i2s_deinit();
	bk_i2s_driver_deinit();
}


static __attribute__((aligned(8))) uint32_t  AUD_PCM_48000_24BITS_test_data[320] = {0};

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
//					 if (g_adc_i2s_ringbuf_dump_mode == 1)
//					 {
//					 	GPIO_DOWN(35);GPIO_UP(35);
//						uint32_t int_level = rtos_enter_critical();
//					 	bk_uart_write_bytes(1, (void *)g_magic, sizeof(g_magic));
//					 	bk_uart_write_bytes(1, (void *)adc_i2s_ringbuf_transfer_buf, read_size);
//					 	rtos_exit_critical(int_level);
//					 	GPIO_DOWN(35);
//					 }
					 if (g_adc_bits == 24) {
						 p = (uint32_t *)(adc_i2s_ringbuf_transfer_buf);
						 for (i = 0, j = 0; i < (read_size>>2); i++)
						 {
							dbg[j] = p[i];//0x5a5b5c5d;
							dbg[j+1]= p[i];//0x00000000;
							j += 2;
						 }
					 } else if (g_adc_bits == 16) 
					 {
						 p_16 = (uint16_t *)(adc_i2s_ringbuf_transfer_buf);
						 for (i = 0, j = 0; i < (read_size>>1); i++)
						 {
							dbg_16[j] = p_16[i];//0x5a5b5c5d;
							dbg_16[j+1]= p_16[i];//0x0000;
							j += 2;
						 }
					 }
// 					 if (g_adc_i2s_ringbuf_dump_mode == 2) {
//					 	GPIO_DOWN(35);GPIO_UP(35);
//						uint32_t int_level = rtos_enter_critical();
//					 	bk_uart_write_bytes(1, (void *)g_magic, sizeof(g_magic));
//					 	bk_uart_write_bytes(1, (void *)dbg_16, read_size*2);
//					 	rtos_exit_critical(int_level);
//					 	GPIO_DOWN(35);
//					 }
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
//				if (g_adc_i2s_ringbuf_dump_mode == 1) 
//				{
//					os_memcpy(dump_buf, adc_i2s_ringbuf_transfer_buf, adc_i2s_ringbuf_i2s_transfer_len);
//					GPIO_DOWN(35);GPIO_UP(35);
//					bk_uart_write_bytes(1, (void *)dump_buf, adc_i2s_ringbuf_i2s_transfer_len);
//					GPIO_DOWN(35);
//				}
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
// 					if (g_i2s_dac_ringbuf_dump_mode == 1)
//					{
//						GPIO_DOWN(35);GPIO_UP(35);
//						uint32_t int_level = rtos_enter_critical();
//						bk_uart_write_bytes(1, (void *)g_magic, sizeof(g_magic));
//						bk_uart_write_bytes(1, (void *)i2s_dac_ringbuf_transfer_buf, read_size);
//						rtos_exit_critical(int_level);
//						GPIO_DOWN(35);
//					}
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

static void aud_adc_i2s_ringbuf_test_cmd(UINT8 enable, UINT8 samplerate, UINT8 adc_dac_mode, UINT8 micchl)
{
	bk_err_t ret = BK_OK;
	dma_config_t adc_dma_config = {0};
	dma_config_t i2s_dma_config = {0};
	aud_adc_config_t adc_config = DEFAULT_AUD_ADC_CONFIG();
	i2s_config_t i2s_config = DEFAULT_I2S_CONFIG();
	uint32_t adc_fifo_addr = 0;
	uint32_t i2s_tx_fifo_addr = 0;
	uint32_t sample_rate = 0;
	i2s_samp_rate_t i2s_samp_rate = I2S_SAMP_RATE_16000;
	uint32_t gpio_group = I2S_GPIO_GROUP_1;
	uint32_t adc_bits = 24;
	uint32_t __maybe_unused i2s_data_len = 32;
	i2s_lrcom_store_mode_t i2s_store_mode = I2S_LRCOM_STORE_LRLR;
	aud_adc_chl_t adc_chl = AUD_ADC_CHL_0;

	g_adc_i2s_ringbuf_dump_mode = 0;

	if (enable == 1)//(os_strcmp(argv[1], "start") == 0)
	{
		if (g_adc_i2s_ringbuf_test_initialized) {
			os_printf("adc i2s ringbuf test already started\n");
			return;
		}

		sample_rate = get_samplerate(samplerate);//strtoul(argv[2], NULL, 10);
		if (sample_rate == 0)
			return;

		adc_config.clk_src = AUD_CLK_APLL;//clk_mode;
		adc_config.sample_rate = sample_rate;//sample_rate;
		adc_config.chl_cfg[adc_chl].adc_mode = (adc_dac_mode == 0) ? AUD_ADC_MODE_DIFFEN : AUD_ADC_MODE_SIGNAL_END;

//		i2s_role_t i2s_role = I2S_ROLE_MASTER;
//		if (argc >= 4) {
//			uint32_t role = strtoul(argv[3], NULL, 10);
//			i2s_role = (role == 0) ? I2S_ROLE_SLAVE : I2S_ROLE_MASTER;
//		}
		i2s_config.role = I2S_ROLE_MASTER;//i2s_role;

	//	if (argc >= 5) 
		{
	//		gpio_group = 1;//strtoul(argv[4], NULL, 10);
		}
	//	if (argc >= 6) 
		{
			adc_bits = 24;//strtoul(argv[5], NULL, 10);
		}
	//	if (argc >= 7) 
		{
			i2s_store_mode = I2S_LRCOM_STORE_LRLR;//strtoul(argv[6], NULL, 10);
		}
	//	if (argc >= 8) 
		{
			i2s_data_len = 32;//strtoul(argv[7], NULL, 10);
		}
	//	if (adc_bits != 16 && i2s_store_mode == I2S_LRCOM_STORE_16R16L) 
	//	{
	//		i2s_data_len = 32;
	//		i2s_store_mode = I2S_LRCOM_STORE_LRLR;
	//	}
		g_adc_bits = adc_bits;
		g_dmic_en = 0;
		g_dmic_dbg = 1;

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
		i2s_config.samp_rate   = i2s_samp_rate;
		i2s_config.data_length = i2s_data_len;
		i2s_config.store_mode  = i2s_store_mode;
		ret = bk_i2s_init(gpio_group, &i2s_config);
		if (ret != BK_OK) {
			os_printf("bk_i2s_init failed: %d\n", ret);
			bk_i2s_driver_deinit();
			return;
		}
		os_printf("Reg0x5B-1 : 0x%x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + 0x168));

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
		os_printf("Reg0x5B-2 : 0x%x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + 0x168));

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
		os_printf("Reg0x5B-3 : 0x%x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + 0x168));

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
		os_printf("Reg0x5B-4 : 0x%x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + 0x168));

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
		
		os_printf("Reg0x5B-5 : 0x%x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + 0x168));
		ret = bk_aud_adc_start(adc_chl);
		if (ret != BK_OK) {
			os_printf("bk_aud_adc_start failed: %d\n", ret);
			return;
		}
        bk_aud_adc_enable_used_channel(1<<adc_chl);

		os_printf("Reg0x5B-6 : 0x%x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + 0x168));
		os_printf("dma_id:%d-----%d\r\n", adc_i2s_ringbuf_adc_dma_id, adc_i2s_ringbuf_i2s_dma_id);
		g_adc_i2s_ringbuf_test_initialized = true;
		os_printf("ADC -> ringbuf -> I2S out test started (sample_rate=%d, gpio_group=%d)\n", sample_rate, gpio_group);
		return;
	} 
	else if (enable == 0)//(os_strcmp(argv[1], "stop") == 0)
	{
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
	} 
}


// I2S RX -> ringbuf -> DAC CLI
/* 8k/16k ---> call
*  48k/44.1k ---> a2dp
*/
static void aud_i2s_dac_ringbuf_test_cmd(UINT8 enable, UINT8 samplerate, UINT8 adc_dac_mode, UINT8 micchl)
{
	bk_err_t ret = BK_OK;
	dma_config_t i2s_dma_config = {0};
	dma_config_t dac_dma_config = {0};
	aud_dac_config_t dac_config = DEFAULT_AUD_DAC_CONFIG();
	i2s_config_t i2s_config = DEFAULT_I2S_CONFIG();
	aud_dac_source_t dac_source = AUD_DAC_SOURCE_A2DP;
	uint32_t i2s_rx_fifo_addr = 0;
	uint32_t dac_fifo_addr = 0;
	uint32_t sample_rate = 48000;
	i2s_samp_rate_t i2s_samp_rate = I2S_SAMP_RATE_48000;
	uint32_t gpio_group = I2S_GPIO_GROUP_1;
	uint32_t dac_bits = 24;
	dma_dev_t dac_dma_dev = DMA_DEV_AUDIO;

	if (enable == 1)
	{
		if (g_i2s_dac_ringbuf_test_initialized) {
			os_printf("i2s dac ringbuf test already started\n");
			return;
		}

		sample_rate = get_samplerate(samplerate);
		if (sample_rate == 0)
			return;
		switch(sample_rate)
		{
			case 8000:
				dac_source = AUD_DAC_SOURCE_CALL;
				break;
			case 16000:
				dac_source = AUD_DAC_SOURCE_CALL;
				break;
			case 44100:
				dac_source = AUD_DAC_SOURCE_A2DP;
				break;
			case 48000:
				dac_source = AUD_DAC_SOURCE_A2DP;
				break;
			default:
				break;
		}

		gpio_group = I2S_GPIO_GROUP_1;
		dac_bits = 24;

		uint32_t i2s_data_len = 32;
		i2s_lrcom_store_mode_t i2s_store_mode = I2S_LRCOM_STORE_LRLR;

		g_dac_bits = dac_bits;

		uint32_t lsb_first_en = 1;

		i2s_config.lsb_first_en = lsb_first_en;

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
		i2s_config.data_length = i2s_data_len;
		i2s_config.store_mode  = i2s_store_mode;
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
		dac_config.work_mode = (adc_dac_mode == 0) ? AUD_DAC_WORK_MODE_DIFFEN : AUD_DAC_WORK_MODE_SIGNAL_END;
		dac_config.clk_src = AUD_CLK_APLL;
		ret = bk_aud_dac_init(&dac_config);
		if (ret != BK_OK) {
			os_printf("bk_aud_dac_init failed: %d\n", ret);
			bk_i2s_deinit();
			bk_i2s_driver_deinit();
			return;
		}
		bk_aud_dac_set_sample_rate(dac_source, sample_rate);
		os_printf("dac source : %d, sample_rate:%d\r\n", dac_source, sample_rate);
		ret = bk_dma_driver_init();
		if (ret != BK_OK) {
			os_printf("dma driver init failed\n");
			i2s_dac_ringbuf_start_cleanup();
			return;
		}

		if (0)//(sample_rate == 44100) 
		{
			i2s_dac_ringbuf_transfer_len = (dac_bits == 16) ?
				(48000 / 1000 * I2S_DAC_FRAME_DURATION * 2) :
				(48000 / 1000 * I2S_DAC_FRAME_DURATION * 4);

		} else {
			i2s_dac_ringbuf_transfer_len = (dac_bits == 16) ?
				(sample_rate / 1000 * I2S_DAC_FRAME_DURATION * 2) :
				(sample_rate / 1000 * I2S_DAC_FRAME_DURATION * 4);
		}

		if (0)//(sample_rate == 44100)
		{
			i2s_dac_ringbuf_i2s_size = 48000 / 1000 * I2S_DAC_FRAME_DURATION * (dac_bits == 16 ? 2 : 4) * 6 + I2S_DAC_RINGBUF_SAFE_INTERVAL;
		} else
		{
			i2s_dac_ringbuf_i2s_size = sample_rate / 1000 * I2S_DAC_FRAME_DURATION * (dac_bits == 16 ? 2 : 4) * 6 + I2S_DAC_RINGBUF_SAFE_INTERVAL;
		}

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
		os_printf("Reg0x5A-4 : 0x%x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + 0x168));

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
		os_printf("Reg0x5A-3 : 0x%x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + 0x168));

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

		*((uint32_t *)(i2s_rx_fifo_addr)) = 0x11;

		ret = bk_aud_dac_start(AUD_DAC_CHL_LR);
		if (ret != BK_OK) {
			os_printf("bk_aud_dac_start failed: %d\n", ret);
			i2s_dac_ringbuf_start_cleanup();
			return;
		}
		os_printf("Reg0x5A-2 : 0x%x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + 0x168));
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
	else if (enable == 0)//(os_strcmp(argv[1], "stop") == 0) 
	{
		os_printf("[-]%s, g_i2s_dac_ringbuf_test_initialized:%d\r\n", __func__, g_i2s_dac_ringbuf_test_initialized);
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
	} 
}


static void aud_adc_i2s_dac_ringbuf_test_cmd(UINT8 enable, UINT8 samplerate, UINT8 adc_dac_mode, UINT8 micchl)
{
	if (enable != 0 || (!g_i2s_dac_ringbuf_test_initialized || !g_adc_i2s_ringbuf_test_initialized))
		return;

	{
		{
			
			bk_aud_dac_stop(AUD_DAC_CHL_LR);
			bk_aud_dac_spk0_source_enable(g_i2s_dac_ringbuf_dac_source, 0);
			g_i2s_dac_ringbuf_test_initialized = false;

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
				psram_free(i2s_dac_ringbuf_transfer_buf);
				i2s_dac_ringbuf_transfer_buf = NULL;
			}
			if (i2s_dac_ringbuf_i2s_rb != NULL) {
				psram_free(i2s_dac_ringbuf_i2s_rb);
				i2s_dac_ringbuf_i2s_rb = NULL;
			}
			if (i2s_dac_ringbuf_i2s_buffer != NULL) {
				psram_free(i2s_dac_ringbuf_i2s_buffer);
				i2s_dac_ringbuf_i2s_buffer = NULL;
			}
			if (i2s_dac_ringbuf_dac_rb != NULL) {
				psram_free(i2s_dac_ringbuf_dac_rb);
				i2s_dac_ringbuf_dac_rb = NULL;
			}
			if (i2s_dac_ringbuf_dac_buffer != NULL) {
				psram_free(i2s_dac_ringbuf_dac_buffer);
				i2s_dac_ringbuf_dac_buffer = NULL;
			}
		//	bk_dma_driver_deinit();
			bk_aud_dac_deinit();
		//	bk_i2s_deinit();
		//	bk_i2s_driver_deinit();
		}

		if (p_dac)
		{
			psram_free(p_dac);
			p_dac = NULL;
		}
		os_printf("I2S RX -> ringbuf -> DAC test stop successful\n");
	}

	{
		if (!g_adc_i2s_ringbuf_test_initialized) {
			os_printf("adc i2s ringbuf test not started\n");
			return;
		}
		bk_aud_adc_deinit();
	//	bk_i2s_stop();
	//	bk_i2s_enable(I2S_DISABLE);
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
	}

	return;
} 

static void *dac_mcp_aud_ptr = NULL;
static uint32_t dac_mcp_aud_len = 0;
static aud_dac_source_t dac_mcp_source = AUD_DAC_SOURCE_A2DP;
static bool dac_mcp_test_initialized = false;

// DMA test variables with ring buffer
static dma_id_t dac_dma_id = DMA_ID_MAX;
#define DAC_DMA_RING_BUFF_SAFE_INTERVAL 20

static void aud_dac_dma_test_cmd(UINT8 enable, UINT8 samplerate, UINT8 adc_dac_mode, UINT8 micchl)
{
    bk_err_t ret = BK_OK;
    aud_dac_config_t dac_config = DEFAULT_AUD_DAC_CONFIG();
    dma_config_t dma_config = {0};
    uint32_t dac_fifo_addr;
    uint32_t *aud_ptr = NULL;
    uint32_t aud_len = 0;
    dma_id_t dma_id = DMA_ID_MAX;
    aud_dac_source_t dac_source = AUD_DAC_SOURCE_A2DP;
	uint32_t dac_sample_rate = 48000;
	uint32_t dac_bits = 24;

    if (enable == 1)
	{
        dac_sample_rate = get_samplerate(samplerate);
        if (dac_sample_rate == 0)
			return;
		switch(dac_sample_rate)
		{
			case 8000:
				dac_source = AUD_DAC_SOURCE_CALL;
				break;
			case 16000:
				dac_source = AUD_DAC_SOURCE_CALL;
				break;
			case 44100:
				dac_source = AUD_DAC_SOURCE_A2DP;
				break;
			case 48000:
				dac_source = AUD_DAC_SOURCE_A2DP;
				break;
			default:
				break;
		}
        //dac_bits = 24;
		get_dac_test_data_by_rate(dac_sample_rate, (const uint32_t **)&aud_ptr, &aud_len);
		return;

        /* save parameters */
        dac_mcp_aud_ptr = aud_ptr;
        dac_mcp_aud_len = aud_len;
        dac_mcp_source  = dac_source;
        dac_mcp_test_initialized = true;

        aud_hardware_reset();
		return;

		dac_config.bits = dac_bits;
		dac_config.clk_src = AUD_CLK_APLL;
		dac_config.work_mode = (adc_dac_mode == 0) ? AUD_DAC_WORK_MODE_DIFFEN : AUD_DAC_WORK_MODE_SIGNAL_END;
        ret = bk_aud_dac_init(&dac_config);
        if (ret != BK_OK) {
            //LOGE("bk_aud_dac_init fail \n");
            return;
        }
        //LOGI("init audio driver and dac successful\n");
        bk_aud_dac_set_sample_rate(dac_source, dac_sample_rate);
		return;

        //init dma driver
        ret = bk_dma_driver_init();
        if (ret != BK_OK) {
            //LOGE("dma driver init failed\r\n");
            return;
        }
        dma_config.mode      = DMA_WORK_MODE_REPEAT;
        dma_config.chan_prio = 1;
        dma_config.src.dev   = DMA_DEV_DTCM;
        dma_config.src.width = DMA_DATA_WIDTH_32BITS;
        if (!get_dac_fifo_addr_by_source(dac_source, &dma_config.dst.dev, &dac_fifo_addr)) {
            //LOGE("get dac fifo address failed\r\n");
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

        //init dma channel
        dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
        if ((dma_id < DMA_ID_0) || (dma_id >= DMA_ID_MAX)) {
            LOGE("malloc dma fail \r\n");
            return;
        }
        dac_dma_id = dma_id;

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
    else if (enable == 0)
    {
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

        LOGD("audio dac dma test stop successfully\n");
		return;
    } 
}

#endif

void audio_ap_test_for_ate11(UINT8 enable, UINT8 test_mode, UINT8 sample_rate, UINT8 adc_dac_mode, UINT8 micchl)
{
	UINT8 ret = 0;

	/* parameter check */
	if ((enable < 0 || enable > 1) || (test_mode < 0 || test_mode > 4) ||
		(sample_rate < 1 || sample_rate > 9) || (adc_dac_mode < 0 || adc_dac_mode > 1) ||
		(micchl < 0 || micchl > 1))
	{
		ret = 1;
		goto audio_exit;
	}

	if (enable == 1) {
		/* start test */
		switch (test_mode)
		{
			/* loop mode */
			case 0:
			//	audio_auto_loop_test(1, sample_rate, adc_dac_mode);
				break;
			/* audio adc: dual mic test*/
			case 1:
				aud_adc_i2s_ringbuf_test_cmd(enable, sample_rate, adc_dac_mode, micchl);
				break;
			/* audio dac */
			case 2:
				aud_i2s_dac_ringbuf_test_cmd(enable, sample_rate, adc_dac_mode, micchl);
				break;
			/* audio adc and dac */
			case 3:
				aud_adc_i2s_ringbuf_test_cmd(enable, sample_rate, adc_dac_mode, micchl);
				aud_i2s_dac_ringbuf_test_cmd(enable, sample_rate, adc_dac_mode, micchl);
				break;
				/*only dac test*/
			case 4:
			//	aud_dac_dma_test_cmd(enable, sample_rate, adc_dac_mode, micchl);
				break;
			default:
				break;
		}
	} 
	else if (enable == 0) {
		/* stop test */
		switch (test_mode)
		{
			/* loop mode */
			case 0:
			//	audio_auto_loop_test(0, sample_rate, adc_dac_mode);
				break;
			/* audio adc */
			case 1:
				aud_adc_i2s_ringbuf_test_cmd(enable, 0, 0, 0);
				break;
			/* audio adc & dac */
			case 2:
				aud_i2s_dac_ringbuf_test_cmd(enable, 0, 0, 0);
				break;
			/* audio dac */
			case 3:
				aud_adc_i2s_dac_ringbuf_test_cmd(enable, 0, 0, 0);
				break;
			case 4:
			//	aud_dac_dma_test_cmd(enable, 0, 0, 0);
				break;
			default:
				ret = 2;
				goto audio_exit;
				break;
		}
	} else {
		ret = 3;
		goto audio_exit;
	}
audio_exit:
	extern void uart_send_bytes_for_ate(UINT8* pData, UINT8 cnt);
	uart_send_bytes_for_ate((UINT8 *)&ret, 1);
}

#endif
