// Copyright 2025-2026 Beken
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <components/bk_audio/audio_streams/onboard_mic_stream_v2.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_port.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <components/bk_audio/audio_pipeline/audio_uplink_layout.h>
#include <driver/aud_adc.h>
#include <driver/dma.h>
#include <driver/audio_ring_buff.h>
#include <driver/flash.h>
#include <driver/flash_types.h>


#define TAG  "OB_MIC"

#define DMA_CARRY_MIC_FRAME_NUM                (2)
#define DMA_CARRY_MIC_RINGBUF_SAFE_INTERVAL    (32)

//#define ONBOARD_MIC_DEBUG   //GPIO debug

#ifdef ONBOARD_MIC_DEBUG

#define AUD_ADC_DMA_ISR_START()                 do { GPIO_DOWN(32); GPIO_UP(32);} while (0)
#define AUD_ADC_DMA_ISR_END()                   do { GPIO_DOWN(32); } while (0)

#define AUD_ONBOARD_MIC_PROCESS_START()         do { GPIO_DOWN(33); GPIO_UP(33);} while (0)
#define AUD_ONBOARD_MIC_PROCESS_END()           do { GPIO_DOWN(33); } while (0)

#define AUD_ONBOARD_MIC_SEM_WAIT_START()        do { GPIO_DOWN(34); GPIO_UP(34);} while (0)
#define AUD_ONBOARD_MIC_SEM_WAIT_END()          do { GPIO_DOWN(34); } while (0)

#define AUD_ONBOARD_MIC_INPUT_START()           do { GPIO_DOWN(35); GPIO_UP(35);} while (0)
#define AUD_ONBOARD_MIC_INPUT_END()             do { GPIO_DOWN(35); } while (0)

#define AUD_ONBOARD_MIC_OUTPUT_START()          do { GPIO_DOWN(36); GPIO_UP(36);} while (0)
#define AUD_ONBOARD_MIC_OUTPUT_END()            do { GPIO_DOWN(36); } while (0)

#else

#define AUD_ADC_DMA_ISR_START()
#define AUD_ADC_DMA_ISR_END()

#define AUD_ONBOARD_MIC_PROCESS_START()
#define AUD_ONBOARD_MIC_PROCESS_END()

#define AUD_ONBOARD_MIC_SEM_WAIT_START()
#define AUD_ONBOARD_MIC_SEM_WAIT_END()

#define AUD_ONBOARD_MIC_INPUT_START()
#define AUD_ONBOARD_MIC_INPUT_END()

#define AUD_ONBOARD_MIC_OUTPUT_START()
#define AUD_ONBOARD_MIC_OUTPUT_END()

#endif

/* onboard mic data count depends on debug utils, so must config CONFIG_ADK_UTILS=y when count onboard mic data. */
#if CONFIG_ADK_UTILS

#define ONBOARD_MIC_DATA_COUNT

#endif  //CONFIG_ADK_UTILS


#ifdef ONBOARD_MIC_DATA_COUNT

#include <components/bk_audio/audio_utils/count_util.h>
static count_util_t onboard_mic_count_util = {0};
#define ONBOARD_MIC_DATA_COUNT_INTERVAL     (1000 * 4)
#define ONBOARD_MIC_DATA_COUNT_TAG          "ONBOARD_MIC"

#define ONBOARD_MIC_DATA_COUNT_OPEN()               count_util_create(&onboard_mic_count_util, ONBOARD_MIC_DATA_COUNT_INTERVAL, ONBOARD_MIC_DATA_COUNT_TAG)
#define ONBOARD_MIC_DATA_COUNT_CLOSE()              count_util_destroy(&onboard_mic_count_util)
#define ONBOARD_MIC_DATA_COUNT_ADD_SIZE(size)       count_util_add_size(&onboard_mic_count_util, size)

#else

#define ONBOARD_MIC_DATA_COUNT_OPEN()
#define ONBOARD_MIC_DATA_COUNT_CLOSE()
#define ONBOARD_MIC_DATA_COUNT_ADD_SIZE(size)

#endif  //ONBOARD_MIC_DATA_COUNT

/* dump onboard_mic stream output pcm data by uart */
//#define ONBOARD_MIC_DATA_DUMP_BY_UART

#ifdef ONBOARD_MIC_DATA_DUMP_BY_UART
#include <components/bk_audio/audio_utils/uart_util.h>
static struct uart_util gl_ob_mic_uart_util = {0};
#define ONBOARD_MIC_DATA_DUMP_UART_ID            (1)
#define ONBOARD_MIC_DATA_DUMP_UART_BAUD_RATE     (2000000)

#define ONBOARD_MIC_DATA_DUMP_BY_UART_OPEN()                    uart_util_create(&gl_ob_mic_uart_util, ONBOARD_MIC_DATA_DUMP_UART_ID, ONBOARD_MIC_DATA_DUMP_UART_BAUD_RATE)
#define ONBOARD_MIC_DATA_DUMP_BY_UART_CLOSE()                   uart_util_destroy(&gl_ob_mic_uart_util)
#define ONBOARD_MIC_DATA_DUMP_BY_UART_DATA(data_buf, len)       uart_util_tx_data(&gl_ob_mic_uart_util, data_buf, len)

#else

#define ONBOARD_MIC_DATA_DUMP_BY_UART_OPEN()
#define ONBOARD_MIC_DATA_DUMP_BY_UART_CLOSE()
#define ONBOARD_MIC_DATA_DUMP_BY_UART_DATA(data_buf, len)

#endif  //ONBOARD_MIC_DATA_DUMP_BY_UART


typedef struct onboard_mic_stream
{
    aud_adc_config_t         adc_cfg;          /**< ADC mode configuration */
    bool                     is_open;          /**< mic enable, true: enable, false: disable */
    uint32_t                 frame_size;       /**< size of one frame mic data, the size
                                                        when AUD_MIC_CHL_MIC1 mode, the size must bean integer multiple of two bytes
                                                        when AUD_MIC_CHL_DUAL mode, the size must bean integer multiple of four bytes */
    dma_id_t                 mic_dma_id;       /**< dma id that dma carry mic data from fifo to ring buffer */
    RingBufferContext        mic_rb;           /**< mic rb handle */
    int8_t                  *mic_ring_buff;    /**< mic ring buffer address */
    int                      out_block_size;   /**< Size of output block */
    int                      out_block_num;    /**< Number of output block */
    beken_semaphore_t        can_process;      /**< can process */
    uint32_t                 ch_bitmap;        /**< Active adc channel bitmap,bit[x]:0:ch_x inactive;1:ch_x active */
    uint32_t                 fade_samp;        /**< amplitude (pop) fade-in length in int16 samples after a (re)start */
    uint32_t                 fade_remain;      /**< remaining int16 samples of the amplitude fade-in */
    uint32_t                 lp_samp;          /**< spectral low-pass open length in int16 samples (decoupled, longer) */
    uint32_t                 lp_remain;        /**< remaining int16 samples of the spectral low-pass window */
    int32_t                  lpf_y[3];         /**< cascaded one-pole low-pass states (one per order) used during the low-pass window */
    beken_mutex_t            cfg_lock;
} onboard_mic_stream_t;

/* Mic (re)start transient shaper. Two decoupled windows; see the parameter
 * tuning doc (onboard_mic_startup_shaping.md) for the why and how to retune.
 *   FADEIN_MS : amplitude smoothstep fade, masks the start "pop".
 *   LPF_MS    : cascaded-LP open window, masks the high-freq "energy spread".
 *               Longer than the fade and opens late (r^5), as the spread peaks
 *               ~300-500ms and decays to ~800-900ms. Effective wall-clock is
 *               ~half the configured ms in this path.
 *   LPF_A_MIN : per-stage one-pole coeff at window start, Q15 (7875 ~= 700Hz).
 *   LPF_ORDER : cascaded one-pole stages (~6 dB/oct each); must match lpf_y[]. */
#define ONBOARD_MIC_FADEIN_MS    (800u)
#define ONBOARD_MIC_LPF_MS       (2100u)
#define ONBOARD_MIC_LPF_A_MIN    (7875)
#define ONBOARD_MIC_LPF_ORDER    (3u)

static onboard_mic_stream_t *gl_onboard_mic = NULL;

static uint8_t aud_adc_get_active_ch_num(void)
{
    uint32_t i;
    uint8_t ch_num = 0;
    
    for(i = 0; i < AUD_ADC_CHL_MAX; i++)
    {
        if(gl_onboard_mic->ch_bitmap & (1 << i))
        {
            ch_num++;
        }
    }
    /* Add AEC loopback channel when AEC is enabled */
    if (gl_onboard_mic->adc_cfg.aec_en) {
        ch_num += 1;
    }
    return ch_num;
}

static void flash_op_notify_onboard_mic_stream_handler(uint32_t param, void *args)
{
    return;
    audio_element_handle_t onboard_mic_stream = (audio_element_handle_t)args;
    uint32_t i;
    if (!onboard_mic_stream)
    {
        return;
    }
    onboard_mic_stream_t *onboard_mic = (onboard_mic_stream_t *)audio_element_getdata(onboard_mic_stream);
    if (onboard_mic && audio_element_get_state(onboard_mic_stream) == AEL_STATE_RUNNING)
    {
        if (param)
        {
            BK_LOGV(TAG, "%s, start earse or write flash, stop dma and adc \n", __func__);
            bk_dma_stop(onboard_mic->mic_dma_id);

            for(i = 0; i < AUD_ADC_CHL_MAX; i++)
            {
                if(onboard_mic->ch_bitmap & (1 << i))
                {
                    bk_aud_adc_stop(i);
                }
            }
            
            ring_buffer_clear(&onboard_mic->mic_rb);
        }
        else
        {
            BK_LOGV(TAG, "%s, stop earse or write flash, start dma and adc \n", __func__);
            bk_dma_start(onboard_mic->mic_dma_id);
            for(i = 0; i < AUD_ADC_CHL_MAX; i++)
            {
                if(onboard_mic->ch_bitmap & (1 << i))
                {
                    bk_aud_adc_start(i);
                }
            }
        }
    }
}

static bk_err_t aud_adc_dma_deconfig(onboard_mic_stream_t *onboard_mic)
{
    if (onboard_mic == NULL)
    {
        return BK_OK;
    }

    bk_dma_deinit(onboard_mic->mic_dma_id);
    bk_dma_free(DMA_DEV_AUDIO, onboard_mic->mic_dma_id);
    //bk_dma_driver_deinit();
    if (onboard_mic->mic_ring_buff)
    {
        ring_buffer_clear(&onboard_mic->mic_rb);
        audio_dma_mem_free(onboard_mic->mic_ring_buff);
        onboard_mic->mic_ring_buff = NULL;
    }

    return BK_OK;
}

/* Carry one frame audio dac data(20ms) from ADC FIFO complete */
static void aud_adc_dma_finish_isr(dma_id_t dma_id)
{
    AUD_ADC_DMA_ISR_START();
    BK_LOGV(TAG, "%s,%d adc dma[%d] finish!\n", __func__, __LINE__, dma_id);
    bk_err_t ret = rtos_set_semaphore(&gl_onboard_mic->can_process);
    if (ret != BK_OK)
    {
        BK_LOGV(TAG, "%s, rtos_set_semaphore fail \n", __func__);
    }
    AUD_ADC_DMA_ISR_END();
}

static bk_err_t aud_adc_dma_config(onboard_mic_stream_t *onboard_mic)
{
    bk_err_t ret = BK_OK;
    dma_config_t dma_config = {0};
    uint32_t adc_port_addr;
    uint32_t frame_size = 0;

    os_memset(&dma_config, 0, sizeof(dma_config_t));

    /* malloc dma channel */
    onboard_mic->mic_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
    if ((onboard_mic->mic_dma_id < DMA_ID_0) || (onboard_mic->mic_dma_id >= DMA_ID_MAX))
    {
        BK_LOGE(TAG, "malloc dma fail \n");
        goto exit;
    }

    /* DMA must carry adcl and adcr data together. frame_size is one channel data size.
     * If channel number is one, need double frame_size.
     */
    uint8_t active_ch_num = aud_adc_get_active_ch_num();
    #if 0
    if (onboard_mic->adc_cfg.chl_num == 1)
    {
        frame_size = onboard_mic->frame_size * 2;
    }
    else
    {
        frame_size = onboard_mic->frame_size;
    }
    #endif
    frame_size = active_ch_num * onboard_mic->frame_size;

    /* init ringbuffer to save two frame data. */
    onboard_mic->mic_ring_buff = (int8_t *)audio_dma_mem_calloc(DMA_CARRY_MIC_FRAME_NUM, frame_size + DMA_CARRY_MIC_RINGBUF_SAFE_INTERVAL / DMA_CARRY_MIC_FRAME_NUM);
    AUDIO_MEM_CHECK(TAG, onboard_mic->mic_ring_buff, return BK_FAIL);
    /* init dma channel */
    dma_config.mode       = DMA_WORK_MODE_REPEAT;
    dma_config.chan_prio  = 1;
    dma_config.trans_type = DMA_TRANS_DEFAULT;
    dma_config.src.dev    = DMA_DEV_AUD_MIC0;
    dma_config.dst.dev    = DMA_DEV_DTCM;
    dma_config.src.width  = DMA_DATA_WIDTH_32BITS;
    dma_config.dst.width  = DMA_DATA_WIDTH_32BITS;
    /* get adc fifo address */
    if (bk_aud_adc_get_fifo_addr(AUD_ADC_MIC_DATA_BUS_0, &adc_port_addr) != BK_OK)
    {
        BK_LOGE(TAG, "get adc fifo address failed\r\n");
        goto exit;
    }
    else
    {
        dma_config.src.addr_inc_en  = DMA_ADDR_INC_ENABLE;
        dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
        dma_config.src.start_addr   = adc_port_addr;
        dma_config.src.end_addr     = adc_port_addr + 4;
    }
    dma_config.trans_type       = DMA_TRANS_DEFAULT;
    dma_config.dst.addr_inc_en  = DMA_ADDR_INC_ENABLE;
    dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
    dma_config.dst.start_addr   = (uint32_t)(uintptr_t)onboard_mic->mic_ring_buff;
    dma_config.dst.end_addr     = (uint32_t)(uintptr_t)onboard_mic->mic_ring_buff + frame_size * DMA_CARRY_MIC_FRAME_NUM + DMA_CARRY_MIC_RINGBUF_SAFE_INTERVAL;
    ret = bk_dma_init(onboard_mic->mic_dma_id, &dma_config);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, dma_init fail\n", __func__, __LINE__);
        goto exit;
    }

    /* set dma transfer length */
    bk_dma_set_transfer_len(onboard_mic->mic_dma_id, frame_size);
    /* register dma isr */
    bk_dma_register_isr(onboard_mic->mic_dma_id, NULL, (void *)aud_adc_dma_finish_isr);
    bk_dma_enable_finish_interrupt(onboard_mic->mic_dma_id);

#if (CONFIG_SPE)
    bk_dma_set_dest_sec_attr(onboard_mic->mic_dma_id, DMA_ATTR_SEC);
    bk_dma_set_src_sec_attr(onboard_mic->mic_dma_id, DMA_ATTR_SEC);
#endif

    ring_buffer_init(&onboard_mic->mic_rb, (uint8_t *)onboard_mic->mic_ring_buff, frame_size * DMA_CARRY_MIC_FRAME_NUM + DMA_CARRY_MIC_RINGBUF_SAFE_INTERVAL, onboard_mic->mic_dma_id, RB_DMA_TYPE_WRITE);

    BK_LOGD(TAG, "adc_dma_cfg mic_dma_id: %d, transfer_len: %d \n", onboard_mic->mic_dma_id, frame_size);
    BK_LOGD(TAG, "src_start_addr: 0x%08x, src_end_addr: 0x%08x \n", dma_config.src.start_addr, dma_config.src.end_addr);
    BK_LOGD(TAG, "dst_start_addr: 0x%08x, dst_end_addr: 0x%08x \n", dma_config.dst.start_addr, dma_config.dst.end_addr);

    return BK_OK;
exit:
    aud_adc_dma_deconfig(onboard_mic);
    return BK_FAIL;
}

static bk_err_t onboard_mic_init_dmic(const onboard_mic_stream_cfg_t *config, uint32_t ch_bitmap)
{
    uint32_t dmic_mode0_mask = ONBOARD_MIC_ADC_ACTIVE_CH_0_BIT;
    uint32_t dmic_mode1_mask = ONBOARD_MIC_ADC_ACTIVE_CH_1_BIT | ONBOARD_MIC_ADC_ACTIVE_CH_2_BIT;
    aud_dmic_mode_t dmic_mode = config->dmic_cfg.dmic_mode;
    aud_dmic_config_t dmic_config = DEFAULT_AUD_DMIC_CONFIG();

    /* DMIC mode and ADC channel bitmap are strongly related:
     *  - AUD_DMIC_MODE_0 -> adc ch0 path
     *  - AUD_DMIC_MODE_1 -> adc ch1/ch2 path
     * Auto-correct obvious mismatches to avoid routing DMIC data to inactive ADC channels.
     */
    if (dmic_mode == AUD_DMIC_MODE_0)
    {
        if ((ch_bitmap & dmic_mode0_mask) == 0)
        {
            BK_LOGE(TAG, "dmic_mode(0) not match ch_bitmap:0x%x, need adc ch0 enabled\n", ch_bitmap);
            return BK_FAIL;
        }
    }
    else if (dmic_mode == AUD_DMIC_MODE_1)
    {
        if ((ch_bitmap & dmic_mode1_mask) == 0)
        {
            BK_LOGE(TAG, "dmic_mode(1) not match ch_bitmap:0x%x, need adc ch1/ch2 enabled\n", ch_bitmap);
            return BK_FAIL;
        }
    }
    else
    {
        BK_LOGE(TAG, "invalid dmic_mode:%d now.\n", dmic_mode);
        return BK_FAIL;
    }

    dmic_config.dmic_clk_gpio  = config->dmic_cfg.dmic_clk_gpio;
    dmic_config.dmic_data_gpio = config->dmic_cfg.dmic_data_gpio;
    dmic_config.dmic_mode      = dmic_mode;
    dmic_config.channel        = config->dmic_cfg.channel;

    return bk_aud_dmic_init(&dmic_config);
}

static bk_err_t _onboard_mic_open(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] %s\n", audio_element_get_tag(self), __func__);
    uint32_t i;

    onboard_mic_stream_t *onboard_mic = (onboard_mic_stream_t *)audio_element_getdata(self);

    if (onboard_mic->is_open)
    {
        return BK_OK;
    }

    /* set read data timeout */
    //audio_element_set_input_timeout(self, 15 / portTICK_RATE_MS);
    ring_buffer_clear(&onboard_mic->mic_rb);

    bk_err_t ret = bk_dma_start(onboard_mic->mic_dma_id);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, adc dma start fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    for(i = 0; i < AUD_ADC_CHL_MAX; i++)
    {
        if(onboard_mic->ch_bitmap & (1 << i))
        {
            ret = bk_aud_adc_start(i);
            if (ret != BK_OK)
            {
                BK_LOGE(TAG, "%s, %d, adc start fail\n", __func__, __LINE__);
                return BK_FAIL;
            }
        }
    }

    bk_aud_adc_enable_used_channel(onboard_mic->ch_bitmap);

    /* Arm pop suppression to mask the mic enable/reset settling transient. The
     * mic read path outputs a mono int16 stream, so size the ramp in mono int16
     * samples. */
    onboard_mic->fade_samp   = ONBOARD_MIC_FADEIN_MS * onboard_mic->adc_cfg.sample_rate / 1000u;
    onboard_mic->fade_remain = onboard_mic->fade_samp;
    onboard_mic->lp_samp     = ONBOARD_MIC_LPF_MS * onboard_mic->adc_cfg.sample_rate / 1000u;
    onboard_mic->lp_remain   = onboard_mic->lp_samp;
    for (uint32_t k = 0; k < ONBOARD_MIC_LPF_ORDER; k++)
        onboard_mic->lpf_y[k] = 0;

    onboard_mic->is_open = true;
    return BK_OK;
}

static int _onboard_mic_read(audio_port_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    audio_element_handle_t el = (audio_element_handle_t)context;
    BK_LOGV(TAG, "[%s] %s, len: %d \n", audio_element_get_tag(el), __func__, len);

    onboard_mic_stream_t *onboard_mic = (onboard_mic_stream_t *)audio_element_getdata(el);
    int ret = BK_OK;
    uint32_t read_size = 0;

    if (len)
    {
        uint32_t fill_size = ring_buffer_get_fill_size(&onboard_mic->mic_rb);
        BK_LOGV(TAG, "[%s] %s, fill_size: %d \n", audio_element_get_tag(el), __func__, fill_size);
        if (fill_size >= len)
        {
            read_size = ring_buffer_read(&onboard_mic->mic_rb, (uint8_t *)buffer, len);
            if (read_size == len)
            {
                ret = read_size;
                BK_LOGV(TAG, "[%s] %s, read data ok, read_size: %d, fill_size: %d\n", audio_element_get_tag(el), __func__, read_size, fill_size);
            }
            else
            {
                BK_LOGE(TAG, "[%s] %s, read data error, read_size: %d, fill_size: %d\n", audio_element_get_tag(el), __func__, read_size, fill_size);
                ret = -1;
            }
        }
        else
        {
            BK_LOGW(TAG, "[%s] %s, mic_fill: %d < len: %d \n", audio_element_get_tag(el), __func__, fill_size, len);
            /* dma carry data length is not right */
            //TODO
            /* ============== workaround handle================== */
            os_memset(buffer, 0, len);
            ret = len;
            //ret = 0;
        }
    }
    else
    {
        ret = len;
    }
    if (ret > 0)
    {
        if (onboard_mic->adc_cfg.chl_cfg[0].bits == 16)
        {
            int16_t *ptr = (int16_t *)buffer;
            for (uint32_t i = 0; i < ret / 4; i++)
            {
                ptr[i] = ptr[2 * i];
            }
            ret = ret / 2;
        }
        #if 0
        else if (onboard_mic->adc_cfg.chl_cfg[0].bits == 24)
        {
            int32_t *ptr = (int32_t *)buffer;
            uint32_t val = 0;
            for (uint32_t i = 0; i < ret / 4; i++)
            {
                val = ptr[i];
                ptr[i] = (int32_t)(uint32_t)(val & 0xFFFFFFu);
            }
        }
        #endif
    }

    return ret;
}

/* Apply the (re)start transient shaping (amplitude smoothstep + cascaded LP) to
 * one mono int16 frame, in place. Both ramp positions advance incrementally so
 * the inner loop stays division-free (only two divides per call to seed the Q15
 * step). No-op once both windows have elapsed. Q15 fixed point throughout. */
static void onboard_mic_apply_startup_shaping(onboard_mic_stream_t *m, int16_t *p, uint32_t n)
{
    if (!m->fade_remain && !m->lp_remain)
        return;

    int32_t dr  = m->fade_remain ? (int32_t)((1 << 15) / m->fade_samp) : 0;
    int32_t drl = m->lp_remain   ? (int32_t)((1 << 15) / m->lp_samp)   : 0;
    int32_t r   = m->fade_remain ? (int32_t)(((int64_t)(m->fade_samp - m->fade_remain) << 15) / m->fade_samp) : (1 << 15);
    int32_t rl  = m->lp_remain   ? (int32_t)(((int64_t)(m->lp_samp   - m->lp_remain)   << 15) / m->lp_samp)   : (1 << 15);

    for (uint32_t i = 0; i < n && (m->fade_remain || m->lp_remain); i++)
    {
        int64_t x = p[i];

        /* spectral: cascaded one-pole LP, per-stage coeff a opens with rl^5 */
        if (m->lp_remain)
        {
            int32_t rl2 = (int32_t)(((int64_t)rl * rl) >> 15);
            int32_t rl3 = (int32_t)(((int64_t)rl2 * rl) >> 15);
            int32_t rl5 = (int32_t)(((int64_t)rl3 * rl2) >> 15);
            int32_t a   = ONBOARD_MIC_LPF_A_MIN
                        + (int32_t)(((int64_t)(32768 - ONBOARD_MIC_LPF_A_MIN) * rl5) >> 15);
            for (uint32_t k = 0; k < ONBOARD_MIC_LPF_ORDER; k++)
            {
                m->lpf_y[k] += (int32_t)(((int64_t)a * (x - m->lpf_y[k])) >> 15);
                x = m->lpf_y[k];
            }
            rl += drl;
            if (rl > (1 << 15)) rl = (1 << 15);
            m->lp_remain--;
        }

        /* amplitude: smoothstep g = 3r^2 - 2r^3 */
        if (m->fade_remain)
        {
            int32_t r2 = (int32_t)(((int64_t)r * r) >> 15);
            int32_t r3 = (int32_t)(((int64_t)r2 * r) >> 15);
            int32_t g  = 3 * r2 - 2 * r3;
            x = (x * g) >> 15;
            r += dr;
            if (r > (1 << 15)) r = (1 << 15);
            m->fade_remain--;
        }

        p[i] = (int16_t)x;
    }
}

static int _onboard_mic_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    onboard_mic_stream_t *onboard_mic = (onboard_mic_stream_t *)audio_element_getdata(self);

    AUD_ONBOARD_MIC_PROCESS_START();

    AUD_ONBOARD_MIC_SEM_WAIT_START();
    if (kNoErr != rtos_get_semaphore(&onboard_mic->can_process, 2000)) //portMAX_DELAY
    {
        BK_LOGE(TAG, "[%s] %s, rtos_get_semaphore fail\n", audio_element_get_tag(self), __func__);
        //return -1;
    }
    AUD_ONBOARD_MIC_SEM_WAIT_END();

    BK_LOGV(TAG, "[%s] _onboard_mic_process \n", audio_element_get_tag(self));

    /* read input data */
    AUD_ONBOARD_MIC_INPUT_START();
    int r_size = audio_element_input(self, in_buffer, in_len);
    AUD_ONBOARD_MIC_INPUT_END();

    /* used to test dma pause function */
#if 0
    static uint32_t count = 0;
    if (count > 500)
    {
        //BK_LOGD(TAG, "%s, count: %d\n", __func__, count);
        rtos_delay_milliseconds(300);
    }
    else
    {
        count++;
    }
#endif

    int w_size = 0;
    if (r_size == AEL_IO_TIMEOUT)
    {
        r_size = 0;
    }
    else if (r_size > 0)
    {
        /* Mask the mic (re)start transient: amplitude pop + high-freq spread.
         * See onboard_mic_apply_startup_shaping() and the tuning doc. */
        onboard_mic_apply_startup_shaping(onboard_mic,
                                          (int16_t *)in_buffer,
                                          (uint32_t)r_size / sizeof(int16_t));   //~700us

        ONBOARD_MIC_DATA_DUMP_BY_UART_DATA(in_buffer, r_size);

        //audio_element_multi_output(self, in_buffer, r_size, 0);
        AUD_ONBOARD_MIC_OUTPUT_START();
        w_size = audio_element_output(self, in_buffer, r_size);
        AUD_ONBOARD_MIC_OUTPUT_END();

        ONBOARD_MIC_DATA_COUNT_ADD_SIZE(r_size);

        /* write data to multiple audio port */
        /* unblock write, and not check write result */
        //TODO
        audio_element_multi_output(self, in_buffer, r_size, 0);

        //更新处理数据的指针
        //audio_element_update_byte_pos(self, w_size);
    }
    else
    {
        w_size = r_size;
    }

    AUD_ONBOARD_MIC_PROCESS_END();

    return w_size;
}

static bk_err_t _onboard_mic_close(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] %s\n", audio_element_get_tag(self), __func__);
    uint32_t i;

    onboard_mic_stream_t *onboard_mic = (onboard_mic_stream_t *)audio_element_getdata(self);

    bk_err_t ret = bk_dma_stop(onboard_mic->mic_dma_id);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, dac dma stop fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    for(i = 0; i < AUD_ADC_CHL_MAX; i++)
    {
        if(onboard_mic->ch_bitmap & (1 << i))
        {
            ret = bk_aud_adc_stop(i);
            if (ret != BK_OK)
            {
                BK_LOGE(TAG, "%s, %d, dac stop fail\n", __func__, __LINE__);
                return BK_FAIL;
            }
        }
    }

    onboard_mic->is_open = false;
    onboard_mic->fade_remain = 0;
    onboard_mic->lp_remain = 0;

    return BK_OK;
}

static bk_err_t _onboard_mic_destroy(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] _onboard_mic_destroy \n", audio_element_get_tag(self));

    onboard_mic_stream_t *onboard_mic = (onboard_mic_stream_t *)audio_element_getdata(self);
    /* deinit dma */
    aud_adc_dma_deconfig(onboard_mic);
    /* deinit dac */
    bk_aud_adc_deinit();

    if (onboard_mic && onboard_mic->can_process)
    {
        rtos_deinit_semaphore(&onboard_mic->can_process);
        onboard_mic->can_process = NULL;
    }
    if (onboard_mic && onboard_mic->cfg_lock)
    {
        rtos_deinit_mutex(&onboard_mic->cfg_lock);
        onboard_mic->cfg_lock = NULL;
    }

    audio_free(onboard_mic);
    onboard_mic = NULL;

    mb_flash_unregister_op_onboard_mic_stream_notify();

    ONBOARD_MIC_DATA_COUNT_CLOSE();
    ONBOARD_MIC_DATA_DUMP_BY_UART_CLOSE();

    return BK_OK;
}

/* Publish the capture-side uplink caps so a downstream AEC can derive the
 * interleaved lane layout itself (no hand-set dual_ch/aec_loop/adc_ch_num). The mic
 * is the natural owner of this geometry.
 *
 * Two hardware-reference mechanisms are expressed here:
 *   - aec_en==1 : APPEND. The hw adds a dedicated ref lane at the tail; every
 *                 enabled ADC channel is a mic. ref_ch is not used (stays AUTO).
 *   - aec_en==0 + hw_ref_ch valid : IN-CHANNEL. A real ADC channel carries the
 *                 loopback reference. The ADC channel number is translated into its
 *                 LANE index (its position among the enabled channels, since the DMA
 *                 interleaves active channels in ascending order) and published as
 *                 caps.ref_ch. The remaining enabled channels are mics. If hw_ref_ch
 *                 is NONE/not enabled, ref_ch stays AUTO (mics only / no in-channel
 *                 ref) - unchanged legacy behaviour.
 *
 * The in-channel reference is an ANALOG loopback wired to an ADC pin, so it must NOT
 * land on a channel DMIC drives digitally (mode 0 owns ch0, mode 1 owns ch1/ch2). */
static void _onboard_mic_publish_uplink_caps(audio_element_handle_t el,
                                             const onboard_mic_stream_cfg_t *config)
{
    aud_uplink_caps_t caps;
    os_memset(&caps, 0x00, sizeof(caps));
    caps.valid     = 1;
    caps.aec_en    = gl_onboard_mic->adc_cfg.aec_en ? 1 : 0;
    caps.ref_ch    = AUD_UPLINK_REF_CH_AUTO;
    caps.version   = AUD_UPLINK_CAPS_VERSION;
    caps.ch_bitmap = gl_onboard_mic->ch_bitmap;

    if (!caps.aec_en)
    {
        uint8_t refch = config->hw_ref_ch;
        uint32_t dmic_mask = 0;
        if (config->dmic_en)
        {
            dmic_mask = (config->dmic_cfg.dmic_mode == AUD_DMIC_MODE_0)
                            ? ONBOARD_MIC_ADC_ACTIVE_CH_0_BIT
                            : (ONBOARD_MIC_ADC_ACTIVE_CH_1_BIT | ONBOARD_MIC_ADC_ACTIVE_CH_2_BIT);
        }

        if (refch >= AUD_ADC_CHL_MAX || !(gl_onboard_mic->ch_bitmap & (1u << refch)))
        {
            if (refch != ONBOARD_MIC_HW_REF_CH_NONE)
            {
                BK_LOGE(TAG, "invalid hw_ref_ch=%d (not enabled in ch_bitmap=0x%x); ignored\n",
                        refch, gl_onboard_mic->ch_bitmap);
            }
            /* aec_en==0 and no valid in-channel reference declared: every enabled ADC
             * channel becomes a mic (ref_ch stays AUTO). With >=2 channels this is very
             * likely a misconfiguration - one channel was probably meant as the
             * in-channel hardware reference but hw_ref_ch was not set - which would make
             * the downstream AEC treat the reference lane as a second mic (dual_ch=1)
             * and lose the reference. We deliberately do NOT guess/auto-correct: just
             * warn so the integrator declares hw_ref_ch. If the reference genuinely
             * arrives via software (multi_input), this is expected and can be ignored. */
            else if (gl_onboard_mic->adc_cfg.chl_num >= 2)
            {
                BK_LOGW(TAG, "%d adc channels enabled (ch_bitmap=0x%x) but no in-channel hw ref "
                             "declared; all are treated as mics. If one is the hardware reference, "
                             "set hw_ref_ch; if the ref is software (multi_input), ignore.\n",
                        gl_onboard_mic->adc_cfg.chl_num, gl_onboard_mic->ch_bitmap);
            }
        }
        else if (dmic_mask & (1u << refch))
        {
            BK_LOGE(TAG, "hw_ref_ch=%d conflicts with dmic (mode=%d owns mask=0x%x); "
                         "the analog ref channel must be outside the dmic channels; ignored\n",
                    refch, config->dmic_cfg.dmic_mode, dmic_mask);
        }
        else
        {
            uint8_t lane = 0;
            for (uint8_t b = 0; b < refch; b++)
            {
                if (gl_onboard_mic->ch_bitmap & (1u << b)) { lane++; }
            }
            caps.ref_ch = lane;
            BK_LOGI(TAG, "in-channel hw ref: adc_ch=%d -> lane=%d (ch_bitmap=0x%x, dmic_en=%d)\n",
                    refch, lane, gl_onboard_mic->ch_bitmap, config->dmic_en);
        }
    }

    audio_element_set_uplink_caps(el, &caps);
}

audio_element_handle_t onboard_mic_stream_init(onboard_mic_stream_cfg_t *config)
{
    audio_element_handle_t el;
    bk_err_t ret = BK_OK;
    uint32_t i;

    gl_onboard_mic = audio_calloc(1, sizeof(onboard_mic_stream_t));
    AUDIO_MEM_CHECK(TAG, gl_onboard_mic, return NULL);

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open    = _onboard_mic_open;
    cfg.close   = _onboard_mic_close;
    cfg.process = _onboard_mic_process;
    cfg.destroy = _onboard_mic_destroy;
    cfg.in_type = PORT_TYPE_CB;
    cfg.read    = _onboard_mic_read;
    cfg.out_type = PORT_TYPE_RB;
    cfg.write    = NULL;
    cfg.task_stack = config->task_stack;
    cfg.task_prio  = config->task_prio;
    cfg.task_core  = config->task_core;
    cfg.multi_out_port_num = config->multi_out_port_num;

    os_memcpy(&gl_onboard_mic->adc_cfg, &config->adc_cfg, sizeof(aud_adc_config_t));

    /* the buffer_len is the parameter of _onboard_mic_process api */

    gl_onboard_mic->ch_bitmap = config->ch_bitmap;

    /* Backward-compat guard for a historical misconfiguration. When digital mics
     * (dmic) are enabled, the real mic channels are exactly the dmic-owned ADC
     * channels (mode 0 -> ch0, mode 1 -> ch1/ch2). Some old solutions enabled ALL
     * three ADC channels by mistake (e.g. ch_bitmap=0x7 with dmic mode 1 + append
     * ref), which makes the downstream AEC see a phantom extra mic lane (mic_cnt=3)
     * and misbehave. Rather than hard-failing, narrow ch_bitmap to the channels that
     * are actually meaningful and WARN loudly so the integrator fixes the config.
     * A non-dmic channel explicitly designated as the in-channel hw reference
     * (aec_en==0 + hw_ref_ch) is legitimate and kept. */
    if (config->dmic_en)
    {
        uint32_t dmic_mask = (config->dmic_cfg.dmic_mode == AUD_DMIC_MODE_0)
                                 ? ONBOARD_MIC_ADC_ACTIVE_CH_0_BIT
                                 : (ONBOARD_MIC_ADC_ACTIVE_CH_1_BIT | ONBOARD_MIC_ADC_ACTIVE_CH_2_BIT);
        uint32_t keep_mask = dmic_mask;
        if (!gl_onboard_mic->adc_cfg.aec_en
            && config->hw_ref_ch < AUD_ADC_CHL_MAX
            && (gl_onboard_mic->ch_bitmap & (1u << config->hw_ref_ch)))
        {
            keep_mask |= (1u << config->hw_ref_ch);
        }

        uint32_t spurious = gl_onboard_mic->ch_bitmap & ~keep_mask;
        if (spurious)
        {
            uint32_t corrected = gl_onboard_mic->ch_bitmap & keep_mask;
            BK_LOGW(TAG, "dmic_en=1(mode %d) but non-dmic adc channels 0x%x also enabled; "
                         "auto-correcting ch_bitmap 0x%x -> 0x%x (spurious channels dropped). "
                         "Please fix the config.\n",
                    config->dmic_cfg.dmic_mode, spurious, gl_onboard_mic->ch_bitmap, corrected);
            gl_onboard_mic->ch_bitmap = corrected;
        }
    }

    uint8_t adc_active_ch_num = 0;
    for (i = 0; i < AUD_ADC_CHL_MAX; i++)
    {
        if (gl_onboard_mic->ch_bitmap & (1 << i))
        {
            adc_active_ch_num++;
        }
    }
    if (adc_active_ch_num == 0)
    {
        BK_LOGE(TAG, "invalid ch_bitmap: 0x%x, no active adc channel\n", gl_onboard_mic->ch_bitmap);
        goto _onboard_mic_init_exit;
    }
    /* ch_bitmap is the source of truth for active ADC channels. */
    gl_onboard_mic->adc_cfg.chl_num = adc_active_ch_num;

    cfg.tag            = "onboard_mic";
    cfg.buffer_len     = config->frame_size * aud_adc_get_active_ch_num();
    cfg.out_block_size = config->frame_size * aud_adc_get_active_ch_num();
    cfg.out_block_num  = config->out_block_num;

    gl_onboard_mic->frame_size     = config->frame_size;
    gl_onboard_mic->out_block_size = cfg.out_block_size;
    gl_onboard_mic->out_block_num  = cfg.out_block_num;

    BK_LOGD(TAG, "ch_bitmap:%d, buffer_len: %d, out_block_size: %d, out_block_num: %d\n", 
        gl_onboard_mic->ch_bitmap, cfg.buffer_len, gl_onboard_mic->out_block_size, gl_onboard_mic->out_block_num);

    /* SSOT self-describe: log the capture-side lane layout so it can be eyeballed
     * against the AEC consume-side log; also verify the SSOT reproduces the legacy
     * lane count. Behaviour-neutral. See audio_uplink_layout.h. */
    {
        aud_uplink_layout_t cap = aud_uplink_capture_layout(gl_onboard_mic->ch_bitmap, gl_onboard_mic->adc_cfg.aec_en);
        uint8_t legacy_lanes = aud_adc_get_active_ch_num();
        BK_LOGI(TAG, "uplink_capture_layout: lanes=%d mic_cnt=%d ref_index=%d aec_en=%d ch_bitmap=0x%x\n",
                cap.lane_num, cap.mic_cnt, cap.ref_index, gl_onboard_mic->adc_cfg.aec_en, gl_onboard_mic->ch_bitmap);
        if (cap.lane_num != legacy_lanes) {
            BK_LOGE(TAG, "uplink SSOT lane mismatch: ssot=%d legacy=%d (ch_bitmap=0x%x aec_en=%d)\n",
                    cap.lane_num, legacy_lanes, gl_onboard_mic->ch_bitmap, gl_onboard_mic->adc_cfg.aec_en);
        }
    }

    /* init audio adc */
    bk_aud_hardware_reset();

    ret = bk_aud_adc_init(&gl_onboard_mic->adc_cfg);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, aud_adc_init fail\n", __func__, __LINE__);
        goto _onboard_mic_init_exit;
    }

    if (config->dmic_en)
    {
        ret = onboard_mic_init_dmic(config, gl_onboard_mic->ch_bitmap);
        if (ret != BK_OK)
        {
            BK_LOGE(TAG, "%s, %d, bk_aud_dmic_init fail\n", __func__, __LINE__);
            goto _onboard_mic_init_exit;
        }
    }

    //uint32_t adc_bits = config->adc_cfg.chl_cfg[0].bits;
    for(i = 0; i < AUD_ADC_CHL_MAX; i++)
    {
        if(gl_onboard_mic->ch_bitmap & (1 << i))
        {
            gl_onboard_mic->adc_cfg.chl_cfg[i].bits = 16;   // Force all channels to the same 16bits!
            bk_aud_adc_set_ana_gain_db(i, config->adc_cfg.chl_cfg[i].ana_gain);
            BK_LOGD(TAG, "adc_cfg chl_num: %d, adc_gain_db: %.2f, samp_rate: %d, clk_src: %s, adc_mode: %s \n",
                i, gl_onboard_mic->adc_cfg.chl_cfg[i].dig_gain, gl_onboard_mic->adc_cfg.sample_rate, gl_onboard_mic->adc_cfg.clk_src == 1 ? "APLL" : "XTAL", gl_onboard_mic->adc_cfg.chl_cfg[i].adc_mode == 1 ? "AUD_ADC_MODE_SIGNAL_END" : "AUD_ADC_MODE_DIFFEN");
        }
    }

    ret = aud_adc_dma_config(gl_onboard_mic);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, adc_dma_init fail\n", __func__, __LINE__);
        goto _onboard_mic_init_exit;
    }

    ret = rtos_init_semaphore(&gl_onboard_mic->can_process, 1);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, rtos_init_semaphore fail\n", __func__, __LINE__);
        goto _onboard_mic_init_exit;
    }

    ret = rtos_init_mutex(&gl_onboard_mic->cfg_lock);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, cfg_lock create fail\n", __func__, __LINE__);
        goto _onboard_mic_init_exit;
    }

    el = audio_element_init(&cfg);
    AUDIO_MEM_CHECK(TAG, el, goto _onboard_mic_init_exit);
    audio_element_setdata(el, gl_onboard_mic);

    audio_element_info_t info = {0};
    info.sample_rates = config->adc_cfg.sample_rate;
    info.channels     = aud_adc_get_active_ch_num();
    info.bits         = config->adc_cfg.chl_cfg[0].bits;
    info.codec_fmt    = BK_CODEC_TYPE_PCM;
    audio_element_setinfo(el, &info);

    /* Publish capture-side uplink caps (lane geometry + reference) for the AEC. */
    _onboard_mic_publish_uplink_caps(el, config);

    mb_flash_register_op_onboard_mic_stream_notify(flash_op_notify_onboard_mic_stream_handler, el);

    ONBOARD_MIC_DATA_COUNT_OPEN();
    ONBOARD_MIC_DATA_DUMP_BY_UART_OPEN();

    return el;
_onboard_mic_init_exit:
    /* deinit dma */
    aud_adc_dma_deconfig(gl_onboard_mic);
    /* deinit adc */
    bk_aud_adc_deinit();
    bk_aud_driver_deinit();
    if (gl_onboard_mic->can_process)
    {
        rtos_deinit_semaphore(&gl_onboard_mic->can_process);
        gl_onboard_mic->can_process = NULL;
    }
    if (gl_onboard_mic->cfg_lock)
    {
        rtos_deinit_mutex(&gl_onboard_mic->cfg_lock);
        gl_onboard_mic->cfg_lock = NULL;
    }

    audio_free(gl_onboard_mic);
    gl_onboard_mic = NULL;
    return NULL;
}

bk_err_t onboard_mic_stream_set_digital_gain(audio_element_handle_t onboard_mic_stream, float gain_db, aud_adc_chl_t ch)
{
    onboard_mic_stream_t *onboard_mic = (onboard_mic_stream_t *)audio_element_getdata(onboard_mic_stream);

    /* check param */
    if (onboard_mic == NULL)
    {
        BK_LOGE(TAG, "%s, line: %d, onboard_mic is not init \n", __func__, __LINE__);
        return BK_FAIL;
    }

    rtos_lock_mutex(&onboard_mic->cfg_lock);
    if (onboard_mic->adc_cfg.chl_cfg[ch].dig_gain == gain_db)
    {
        BK_LOGD(TAG, "not need update onboard mic digital gain \n");
        rtos_unlock_mutex(&onboard_mic->cfg_lock);
        return BK_OK;
    }

    if (BK_OK == bk_aud_adc_set_dig_gain_db(ch, gain_db))
    {
        onboard_mic->adc_cfg.chl_cfg[ch].dig_gain = gain_db;
        audio_element_setdata(onboard_mic_stream, onboard_mic);
    }
    else
    {
        BK_LOGE(TAG, "%s, line: %d, update mic digital gain fail \n", __func__, __LINE__);
        rtos_unlock_mutex(&onboard_mic->cfg_lock);
        return BK_FAIL;
    }

    rtos_unlock_mutex(&onboard_mic->cfg_lock);
    return BK_OK;
}

bk_err_t onboard_mic_stream_get_digital_gain(audio_element_handle_t onboard_mic_stream, float *gain_db, aud_adc_chl_t ch)
{
    onboard_mic_stream_t *onboard_mic = (onboard_mic_stream_t *)audio_element_getdata(onboard_mic_stream);

    /* check param */
    if (gain_db == NULL)
    {
        BK_LOGE(TAG, "%s, line: %d, gain_db is NULL\n", __func__, __LINE__);
        return BK_FAIL;
    }

    /* check param */
    if (onboard_mic == NULL)
    {
        BK_LOGE(TAG, "%s, line: %d, onboard_mic is not init \n", __func__, __LINE__);
        return BK_FAIL;
    }

    rtos_lock_mutex(&onboard_mic->cfg_lock);
    *gain_db = onboard_mic->adc_cfg.chl_cfg[ch].dig_gain;
    rtos_unlock_mutex(&onboard_mic->cfg_lock);

    return BK_OK;
}

bk_err_t onboard_mic_stream_set_analog_gain(audio_element_handle_t onboard_mic_stream, int32_t gain_db, aud_adc_chl_t ch)
{
    onboard_mic_stream_t *onboard_mic = (onboard_mic_stream_t *)audio_element_getdata(onboard_mic_stream);

    /* check param */
    if (onboard_mic == NULL)
    {
        BK_LOGE(TAG, "%s, line: %d, onboard_mic is not init \n", __func__, __LINE__);
        return BK_FAIL;
    }

    rtos_lock_mutex(&onboard_mic->cfg_lock);
    if (onboard_mic->adc_cfg.chl_cfg[ch].ana_gain == gain_db)
    {
        BK_LOGD(TAG, "not need update onboard mic analog gain \n");
        rtos_unlock_mutex(&onboard_mic->cfg_lock);
        return BK_OK;
    }

    if (BK_OK == bk_aud_adc_set_ana_gain_db(ch, gain_db))
    {
        onboard_mic->adc_cfg.chl_cfg[ch].ana_gain = gain_db;
        audio_element_setdata(onboard_mic_stream, onboard_mic);
    } else
    {
        BK_LOGE(TAG, "%s, line: %d, update mic analog gain fail \n", __func__, __LINE__);
        rtos_unlock_mutex(&onboard_mic->cfg_lock);
        return BK_FAIL;
    }

    rtos_unlock_mutex(&onboard_mic->cfg_lock);
    return BK_OK;
}

bk_err_t onboard_mic_stream_get_analog_gain(audio_element_handle_t onboard_mic_stream, int32_t *gain_db, aud_adc_chl_t ch)
{
    onboard_mic_stream_t *onboard_mic = (onboard_mic_stream_t *)audio_element_getdata(onboard_mic_stream);
    /* check param */
    if (gain_db == NULL)
    {
        BK_LOGE(TAG, "%s, line: %d, gain_db is NULL\n", __func__, __LINE__);
        return BK_FAIL;
    }

    /* check param */
    if (onboard_mic == NULL)
    {
        BK_LOGE(TAG, "%s, line: %d, onboard_mic is not init \n", __func__, __LINE__);
        return BK_FAIL;
    }

    rtos_lock_mutex(&onboard_mic->cfg_lock);
    *gain_db = onboard_mic->adc_cfg.chl_cfg[ch].ana_gain;
    rtos_unlock_mutex(&onboard_mic->cfg_lock);

    return BK_OK;
}

