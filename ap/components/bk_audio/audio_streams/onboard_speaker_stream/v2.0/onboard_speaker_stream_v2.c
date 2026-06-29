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
#include <limits.h>
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <timers.h>
#include <components/bk_audio/audio_pipeline/bsd_queue.h>
#include <components/bk_audio/audio_streams/onboard_speaker_stream_v2.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_port.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <components/bk_audio/audio_pipeline/audio_port_info_list.h>
#include <driver/aud_dac.h>
#include <driver/dma.h>
#include <bk_general_dma.h>
#include <driver/audio_ring_buff.h>
#include <driver/gpio.h>
#include "gpio_driver.h"


#define TAG  "ONBOARD_SPEAKER"

//#define ONBOARD_SPK_DEBUG   //GPIO debug

#ifdef ONBOARD_SPK_DEBUG

#define AUD_DAC_DMA_ISR_START()                 do { GPIO_DOWN(32); GPIO_UP(32);} while (0)
#define AUD_DAC_DMA_ISR_END()                   do { GPIO_DOWN(32); } while (0)

#define AUD_ONBOARD_SPK_PROCESS_START()         do { GPIO_DOWN(33); GPIO_UP(33);} while (0)
#define AUD_ONBOARD_SPK_PROCESS_END()           do { GPIO_DOWN(33); } while (0)

#define AUD_ONBOARD_SPK_INPUT_START()           do { GPIO_DOWN(34); GPIO_UP(34);} while (0)
#define AUD_ONBOARD_SPK_INPUT_END()             do { GPIO_DOWN(34); } while (0)

#define AUD_ONBOARD_SPK_OUTPUT_START()          do { GPIO_DOWN(35); GPIO_UP(35);} while (0)
#define AUD_ONBOARD_SPK_OUTPUT_END()            do { GPIO_DOWN(35); } while (0)

#else

#define AUD_DAC_DMA_ISR_START()
#define AUD_DAC_DMA_ISR_END()

#define AUD_ONBOARD_SPK_PROCESS_START()
#define AUD_ONBOARD_SPK_PROCESS_END()

#define AUD_ONBOARD_SPK_INPUT_START()
#define AUD_ONBOARD_SPK_INPUT_END()

#define AUD_ONBOARD_SPK_OUTPUT_START()
#define AUD_ONBOARD_SPK_OUTPUT_END()

#endif

/* onboard speaker data count depends on debug utils, so must config CONFIG_ADK_UTILS=y when count onboard speaker data. */
#if CONFIG_ADK_UTILS

#define ONBOARD_SPK_DATA_COUNT

#endif  //CONFIG_ADK_UTILS

#ifdef ONBOARD_SPK_DATA_COUNT

#include <components/bk_audio/audio_utils/count_util.h>

/* Multi-parameter count util for onboard speaker statistics */
static count_util_multi_t onboard_spk_count_util = {0};
#define ONBOARD_SPK_DATA_COUNT_INTERVAL     (1000 * 4)

/* Parameter indices */
#define ONBOARD_SPK_PARAM_INDEX             0
#define FILL_SILENCE_PARAM_INDEX            1
#define ONBOARD_SPK_PARAM_COUNT             2

/* Parameter tags */
#define ONBOARD_SPK_DATA_COUNT_TAG          "ONBOARD_SPK"
#define FILL_SILENCE_DATA_COUNT_TAG         "FILL_SILENCE"

/* Macro definitions */
#define ONBOARD_SPK_DATA_COUNT_OPEN() \
    do { \
        char *tags[ONBOARD_SPK_PARAM_COUNT] = {ONBOARD_SPK_DATA_COUNT_TAG, FILL_SILENCE_DATA_COUNT_TAG}; \
        count_util_multi_create(&onboard_spk_count_util, ONBOARD_SPK_DATA_COUNT_INTERVAL, tags, ONBOARD_SPK_PARAM_COUNT); \
    } while(0)

#define ONBOARD_SPK_DATA_COUNT_CLOSE()              count_util_multi_destroy(&onboard_spk_count_util)
#define ONBOARD_SPK_DATA_COUNT_ADD_SIZE(size)       count_util_multi_add_size(&onboard_spk_count_util, ONBOARD_SPK_PARAM_INDEX, size)
#define FILL_SILENCE_DATA_COUNT_ADD_SIZE(size)      count_util_multi_add_size(&onboard_spk_count_util, FILL_SILENCE_PARAM_INDEX, size)

#else

#define ONBOARD_SPK_DATA_COUNT_OPEN()
#define ONBOARD_SPK_DATA_COUNT_CLOSE()
#define ONBOARD_SPK_DATA_COUNT_ADD_SIZE(size)
#define FILL_SILENCE_DATA_COUNT_ADD_SIZE(size)

#endif  //ONBOARD_SPK_DATA_COUNT

/* dump onboard_spk stream play pcm data by uart */
//#define ONBOARD_SPK_DATA_DUMP_BY_UART

#ifdef ONBOARD_SPK_DATA_DUMP_BY_UART
#include <components/bk_audio/audio_utils/uart_util.h>
static struct uart_util gl_ob_spk_uart_util = {0};
#define ONBOARD_SPK_DATA_DUMP_UART_ID            (1)
#define ONBOARD_SPK_DATA_DUMP_UART_BAUD_RATE     (2000000)

#define ONBOARD_SPK_DATA_DUMP_BY_UART_OPEN()                    uart_util_create(&gl_ob_spk_uart_util, ONBOARD_SPK_DATA_DUMP_UART_ID, ONBOARD_SPK_DATA_DUMP_UART_BAUD_RATE)
#define ONBOARD_SPK_DATA_DUMP_BY_UART_CLOSE()                   uart_util_destroy(&gl_ob_spk_uart_util)
#define ONBOARD_SPK_DATA_DUMP_BY_UART_DATA(data_buf, len)       uart_util_tx_data(&gl_ob_spk_uart_util, data_buf, len)

#else

#define ONBOARD_SPK_DATA_DUMP_BY_UART_OPEN()
#define ONBOARD_SPK_DATA_DUMP_BY_UART_CLOSE()
#define ONBOARD_SPK_DATA_DUMP_BY_UART_DATA(data_buf, len)

#endif  //ONBOARD_MIC_DATA_DUMP_BY_UART


#define DMA_CARRY_SPK_RINGBUF_SAFE_INTERVAL    (8)

//#define SPK_DATA_DEBUG

#ifdef SPK_DATA_DEBUG
static const uint32_t PCM_8000[] = {
	0x00010000, 0x5A825A81, 0x7FFF7FFF, 0x5A825A83, 0x00000000, 0xA57FA57E, 0x80018002, 0xA57EA57E,
};
#endif


typedef struct onboard_speaker_stream
{
    uint8_t                  chl_num;                       /**< PCM channel count: 1 mono, 2 interleaved stereo */
    aud_dac_chl_t            dac_chl;                       /**< DAC output route: L / R / LR */
    uint32_t                 sample_rate[AUD_DAC_SOURCE_MAX];/**< speaker sample rate */
    float                    dig_gain;                      /**< audio dac digital gain in dB */
    int32_t                  ana_gain;                      /**< audio dac analog gain in dB */
    aud_dac_work_mode_t      work_mode;                     /**< audio dac mode: signal_ended/differen */
    uint8_t                  bits;                          /**< Bit wide (8, 16, 24, 32 bits) */
    aud_clk_t                clk_src;                       /**< audio clock: XTAL(26MHz)/APLL */
    bool                     is_open;                       /**< speaker enable, true: enable, false: disable */
    uint32_t                 frame_size[AUD_DAC_SOURCE_MAX];/**< size of one frame speaker data, the size
                                                                        when AUD_DAC_CHL_L_ENABLE mode, the size must bean integer multiple of two bytes
                                                                        when AUD_DAC_CHL_LR_ENABLE mode, the size must bean integer multiple of four bytes */
    dma_id_t                 spk_dma_id[AUD_DAC_SOURCE_MAX];            /**< one DMA per dac source: ring buffer -> DAC FIFO */
    RingBufferContext        spk_rb[AUD_DAC_SOURCE_MAX];                /**< one ring buffer per dac source */
    int8_t                  *spk_ring_buff[AUD_DAC_SOURCE_MAX];         /**< ring buffer memory per dac source */
    uint32_t                 pool_length;                   /**< speaker data pool size, the unit is byte */
    uint32_t                 pool_play_thold;               /**< the play threshold of pool, the unit is byte */
    uint32_t                 pool_pause_thold;              /**< the pause threshold of pool, the unit is byte */
    RingBufferContext        pool_rb;                       /**< the pool ringbuffer handle */
    int8_t                  *pool_ring_buff;                /**< pool ring buffer addr */
    bool                     pool_can_read;                 /**< the pool if can read */
    beken_semaphore_t        can_process;                   /**< can process */
    int8_t                  *temp_buff;                     /**< HW pack / silence / AEC-ref scratch (interleaved PCM or 32-bit L|R) */
    uint32_t                 temp_buff_len;                 /**< bytes allocated for temp_buff */
    bool                     wr_spk_rb_done[AUD_DAC_SOURCE_MAX];       /**< write one farme data to speaker ring buffer done */
    uint8_t                  valid_frame_count_in_spk_rb;   /**< the count of valid farme data in speaker ring buffer, data playback finish when the count is 0 */

    bool                     pa_ctrl_en;                    /**< control pa enable */
    uint16_t                 pa_ctrl_gpio;                  /**< the gpio id of control pa */
    uint8_t                  pa_on_level;                   /**< the gpio level of turn on pa, 0: low level, 1: high level */
    uint32_t                 pa_on_delay;                   /**< the delay time(ms) of turn on pa after enable audio dac. [dac init -> pa turn on] */
    uint32_t                 pa_off_delay;                  /**< the delay time(ms) of disable audio dac after turn off pa. [mute -> pa turn off -> dac deinit] */
    TimerHandle_t            pa_turn_on_timer;              /**< the timer handle of turn on pa */
    bool                     pa_state;                      /**< the state of pa, true: on, false: off */

#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
    int                             current_port_id;        /**< the valid audio port of currently reading speaker data, 0: element->in, >=1: element->multi_in */
    SemaphoreHandle_t               lock;                   /**< input audio port info list lock */
    input_audio_port_info_list_t    input_port_list;        /**< the list of input audio port info */
#endif
    uint32_t                 dac_source_bitmap;             /**< bitmap of active dac source,bit[x]:0:source_x inactive;1:source_x active */
    aud_dac_source_t         main_dac_source;               /**< main input source mapped to element->in */
    uint32_t                 dma_frame_size;                /**< Bytes per DMA transfer to DAC ring: same as frame_size for 44.1k/48k passthrough; otherwise length after resampling to 48k */
    aud_rsp_cfg_t            rsp_cfg[AUD_DAC_SOURCE_MAX];   /**< resampler config */
    uint16_t                *rsp_out_buff[AUD_DAC_SOURCE_MAX];/**< interleaved resampler output (mono or stereo) */
    void                    *rsp_handler[AUD_DAC_SOURCE_MAX];/**< resampler handler */
    onboard_speaker_stream_status_t status;                 /**< runtime status for upper layer */
    uint8_t                         play_energy_threshold;  /**< voice-play enter threshold in range 0~100 */
    uint8_t                         play_energy_hysteresis; /**< play-state hysteresis in range 0~100 */
    onboard_speaker_status_cb_t     status_cb;              /**< status report callback */
    void                            *status_cb_user_data;   /**< callback private data */
    beken_mutex_t                   cfg_lock;
} onboard_speaker_stream_t;

/* 16bit interleaved L,R -> 32bit word: LSB=left, MSB=right (DAC stereo_en HW split) */
static void onboard_spk_pack_interleaved16_to_hw32(const int16_t *lr, uint32_t *packed, uint32_t sample_pairs)
{
    for (uint32_t j = 0; j < sample_pairs; j++)
    {
        uint16_t left  = (uint16_t)lr[2 * j + 0];
        uint16_t right = (uint16_t)lr[2 * j + 1];
        packed[j] = ((uint32_t)right << 16) | left;
    }
}

static inline uint8_t onboard_spk_pcm_chl_num_from_dac_chl(aud_dac_chl_t dac_chl)
{
    return (dac_chl == AUD_DAC_CHL_LR) ? 2 : 1;
}

static inline bool onboard_spk_dac_chl_config_valid(aud_dac_chl_t dac_chl)
{
    return dac_chl < AUD_DAC_CHL_MAX;
}

static inline bool onboard_spk_hw_stereo_split_mode(const onboard_speaker_stream_t *onboard_spk)
{
    return (onboard_spk->dac_chl == AUD_DAC_CHL_LR) && (onboard_spk->bits == 16);
}

/* DAC FIFO index for DMA destination: LR hw-split / mono-L -> fifo0; mono-R -> fifo1 */
static inline uint32_t onboard_spk_dma_fifo_ch(const onboard_speaker_stream_t *onboard_spk)
{
    if (onboard_spk->dac_chl == AUD_DAC_CHL_R)
    {
        return 1;
    }

    return 0;
}

/* Extract left channel for AEC reference when input is interleaved L,R,... */
static void onboard_spk_extract_left_channel_16(const int16_t *lr, int16_t *left, uint32_t samples_per_ch)
{
    for (uint32_t j = 0; j < samples_per_ch; j++)
    {
        left[j] = lr[2 * j + 0];
    }
}

static onboard_speaker_stream_t *gl_onboard_speaker = NULL;
static uint32_t spk_dma_finish_bitmap = 0;
static uint32_t open_cnt = 0;//workaround of aud dac dma stop issue

static uint8_t onboard_spk_calc_energy_level(const uint8_t *data, uint32_t size, uint8_t bits)
{
    if (data == NULL || size == 0)
    {
        return 0;
    }

    uint64_t abs_sum = 0;
    uint64_t full_scale = 0;
    uint32_t sample_num = 0;

    if (bits == 16)
    {
        const int16_t *samples = (const int16_t *)data;
        sample_num = size / sizeof(int16_t);
        full_scale = INT16_MAX;
        for (uint32_t i = 0; i < sample_num; i++)
        {
            int32_t sample = samples[i];
            uint64_t abs_val = (sample < 0) ? (uint64_t)(-sample) : (uint64_t)sample;
            abs_sum += abs_val;
        }
    }
    else
    {
        const int32_t *samples = (const int32_t *)data;
        sample_num = size / sizeof(int32_t);
        full_scale = INT32_MAX;
        for (uint32_t i = 0; i < sample_num; i++)
        {
            int64_t sample = samples[i];
            uint64_t abs_val = (sample < 0) ? (uint64_t)(-sample) : (uint64_t)sample;
            abs_sum += abs_val;
        }
    }

    if (full_scale == 0 || sample_num == 0)
    {
        return 0;
    }

    uint64_t denominator = full_scale * sample_num;
    uint64_t level = (abs_sum * 100 + (denominator / 2)) / denominator;
    return (level > 100) ? 100 : (uint8_t)level;
}

static void onboard_spk_notify_status(audio_element_handle_t self, onboard_speaker_stream_t *onboard_spk)
{
    if (onboard_spk && onboard_spk->status_cb)
    {
        onboard_spk->status_cb(self, &onboard_spk->status, onboard_spk->status_cb_user_data);
    }
}

static bool onboard_spk_is_voice_playing(const onboard_speaker_stream_t *onboard_spk, uint8_t energy_level)
{
    uint8_t enter_threshold;
    uint8_t exit_threshold;

    if (onboard_spk == NULL || !onboard_spk->is_open)
    {
        return false;
    }

    enter_threshold = onboard_spk->play_energy_threshold;
    exit_threshold = (enter_threshold > onboard_spk->play_energy_hysteresis) ?
        (enter_threshold - onboard_spk->play_energy_hysteresis) : 0;

    if (onboard_spk->status.is_playing)
    {
        return (energy_level > exit_threshold);
    }

    return (energy_level > enter_threshold);
}

static void onboard_spk_update_status(audio_element_handle_t self, onboard_speaker_stream_t *onboard_spk, bool is_playing, uint8_t energy_level, bool force_report)
{
    bool changed = false;

    if (onboard_spk == NULL)
    {
        return;
    }

    if (onboard_spk->status.is_playing != is_playing)
    {
        onboard_spk->status.is_playing = is_playing;
        changed = true;
    }

    if (onboard_spk->status.energy_level != energy_level)
    {
        onboard_spk->status.energy_level = energy_level;
        changed = true;
    }

    if (force_report || changed)
    {
        onboard_spk_notify_status(self, onboard_spk);
    }
}

static void onboard_spk_update_status_by_energy(audio_element_handle_t self, onboard_speaker_stream_t *onboard_spk, uint8_t energy_level, bool force_report)
{
    bool is_playing = onboard_spk_is_voice_playing(onboard_spk, energy_level);
    onboard_spk_update_status(self, onboard_spk, is_playing, energy_level, force_report);
}

/* A2DP stereo resample (plan B): one Speex instance, src_ch=dest_ch=2, interleaved in/out */
static inline void onboard_spk_rsp_set_channel_config(onboard_speaker_stream_t *onboard_spk, uint32_t src_idx)
{
    uint32_t ch = onboard_spk->chl_num > 1 ? 2 : 1;

    onboard_spk->rsp_cfg[src_idx].src_ch = ch;
    onboard_spk->rsp_cfg[src_idx].dest_ch = ch;
}

static inline uint32_t onboard_spk_hw_dma_xfer_bytes(const onboard_speaker_stream_t *onboard_spk, uint32_t src_idx)
{
    if ((AUD_DAC_SOURCE_A2DP == src_idx) && onboard_spk->rsp_handler[src_idx])
    {
        return onboard_spk->dma_frame_size;
    }

    return onboard_spk->frame_size[src_idx];
}

/* Workspace for interleaved PCM, HW 32-bit pack, or mono AEC ref (<= max input frame, or post-rsp dma_frame) */
static inline uint32_t onboard_spk_temp_buff_len(const onboard_speaker_stream_t *onboard_spk, uint32_t max_in_frame_bytes)
{
    uint32_t len = max_in_frame_bytes;

    if (onboard_spk->dma_frame_size > len)
    {
        len = onboard_spk->dma_frame_size;
    }
    return len;
}

static bk_err_t onboard_spk_rsp_process_frame(onboard_speaker_stream_t *onboard_spk, uint32_t src_idx,
    const int16_t *in_pcm, int16_t *out_pcm, uint32_t *pcm_out_bytes)
{
    uint32_t rsp_ch = onboard_spk->chl_num > 1 ? 2 : 1;
    uint32_t rsp_in_len = onboard_spk->frame_size[src_idx] / (sizeof(int16_t) * rsp_ch);
    uint32_t rsp_out_len = onboard_spk->dma_frame_size / (sizeof(int16_t) * rsp_ch);
    uint32_t rsp_out_len_req = rsp_out_len;
    bk_err_t ret;

    ret = bk_aud_rsp_process_multi_instance((int16_t *)in_pcm, &rsp_in_len, out_pcm, &rsp_out_len,
                                            onboard_spk->rsp_handler[src_idx]);
    if (pcm_out_bytes)
    {
        *pcm_out_bytes = rsp_out_len * sizeof(int16_t) * rsp_ch;
    }

    if ((BK_OK == ret) && (rsp_out_len != rsp_out_len_req))
    {
        BK_LOGW(TAG, "%s, src:%u, rsp out samples/ch %u != %u\n", __func__, src_idx, rsp_out_len, rsp_out_len_req);
    }

    return ret;
}

/* Forward PCM to element multi_out port(s) when configured (e.g. AEC reference tap). */
static void onboard_spk_multi_output_pcm(audio_element_handle_t self, onboard_speaker_stream_t *onboard_spk,
    const int16_t *interleaved_pcm, uint32_t pcm_bytes)
{
    char *out_buf;
    uint32_t out_bytes;

    if (audio_element_get_multi_output_max_port_num(self) <= 0)
    {
        return;
    }

    if (onboard_spk->chl_num > 1)
    {
        out_bytes = pcm_bytes / 2;
        onboard_spk_extract_left_channel_16(interleaved_pcm, (int16_t *)onboard_spk->temp_buff,
                                            out_bytes / sizeof(int16_t));
        out_buf = (char *)onboard_spk->temp_buff;
    }
    else
    {
        out_buf = (char *)interleaved_pcm;
        out_bytes = pcm_bytes;
    }

    audio_element_multi_output(self, out_buf, out_bytes, 0);
}

/* Interleaved PCM in; returns buffer/length for single DAC DMA ring write */
static uint8_t *onboard_spk_prepare_dac_write_buf(onboard_speaker_stream_t *onboard_spk,
    const int16_t *interleaved_pcm, uint32_t interleaved_bytes, uint32_t *dac_bytes)
{
    if (onboard_spk_hw_stereo_split_mode(onboard_spk))
    {
        uint32_t sample_pairs = interleaved_bytes / (sizeof(int16_t) * 2);
        uint32_t *packed = (uint32_t *)onboard_spk->temp_buff;

        onboard_spk_pack_interleaved16_to_hw32(interleaved_pcm, packed, sample_pairs);
        *dac_bytes = sample_pairs * sizeof(uint32_t);
        return (uint8_t *)packed;
    }

    *dac_bytes = interleaved_bytes;
    return (uint8_t *)interleaved_pcm;
}

#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
#define input_port_list_release(handle) xSemaphoreGive(handle)
#define input_port_list_block(handle, time) xSemaphoreTake(handle, time)
#endif

//#define AEC_MIC_DELAY_POINTS_DEBUG

static bk_err_t _onboard_speaker_close(audio_element_handle_t self);

#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
//#define PORT_LIST_DEBUG
#ifdef PORT_LIST_DEBUG
#define INPUT_PORT_LIST_DEBUG(list_ptr, func, line)  audio_port_info_list_debug_print(list_ptr, func, line)
#else
#define INPUT_PORT_LIST_DEBUG(list_ptr, func, line)
#endif
#endif

#ifdef AEC_MIC_DELAY_POINTS_DEBUG
static void aec_mic_delay_debug(int16_t *data, uint32_t size)
{
    static uint32_t mic_delay_num = 0;
    mic_delay_num++;
    os_memset(data, 0, size);
    if (mic_delay_num == 50)
    {
        data[0] = 0x2FFF;
        mic_delay_num = 0;
        BK_LOGD(TAG, "AEC_MIC_DELAY_POINTS_DEBUG \n");
    }
}
#endif

#ifdef SPK_DATA_DEBUG
void change_pcm_data_to_8k(uint8_t* buffer, uint32_t size)
{
    for (uint32_t i = 0; i < (size/sizeof(PCM_8000)); i++)
    {
        os_memcpy(&buffer[i * sizeof(PCM_8000)], PCM_8000, sizeof(PCM_8000));
    }
}
#endif

/* A2DP: passthrough to DAC at 44.1k/48k; other sample rates are resampled to 48k */
static bool onboard_spk_a2dp_src_need_resample(uint32_t src_rate)
{
    return (src_rate != 44100 && src_rate != 48000);
}

static bool onboard_spk_a2dp_src_unsupported(uint32_t src_rate)
{
    return (src_rate == 8000);
}

#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
static int onboard_spk_get_multi_input_port_by_src(onboard_speaker_stream_t *onboard_spk, uint32_t src_id)
{
    uint32_t port_id = 0;

    if (src_id == onboard_spk->main_dac_source)
    {
        return -1;
    }

    for (uint32_t i = 0; i < AUD_DAC_SOURCE_MAX; i++)
    {
        if (!(onboard_spk->dac_source_bitmap & (1 << i)) || i == onboard_spk->main_dac_source)
        {
            continue;
        }

        if (i == src_id)
        {
            return (int)port_id;
        }

        port_id++;
    }

    return -1;
}
#endif

/* PA control gpio */
static void _pa_gpio_ctrl(uint16_t pa_ctrl_gpio, uint8_t pa_on_level, bool en)
{
    if (en)
    {
        BK_LOGD(TAG, "%s, %d, PA turn on \n", __func__, __LINE__);
        /* open pa according to congfig */
        if (pa_on_level)
        {
            bk_gpio_set_output_high(pa_ctrl_gpio);
        }
        else
        {
            bk_gpio_set_output_low(pa_ctrl_gpio);
        }
    }
    else
    {
        BK_LOGD(TAG, "%s, %d, PA turn off \n", __func__, __LINE__);
        if (pa_on_level)
        {
            bk_gpio_set_output_low(pa_ctrl_gpio);
        }
        else
        {
            bk_gpio_set_output_high(pa_ctrl_gpio);
        }
    }
}

/*
 * @brief: pa control api
 * @param: onboard_spk: speaker stream
 * @param: en: true: turn on, false: turn off
 * @param: delay_flag: true: delay turn on, false: no delay
 * @return: none
 */
static void pa_ctrl_en(onboard_speaker_stream_t *onboard_spk, bool en, bool delay_flag)
{
    if (!onboard_spk->pa_ctrl_en)
    {
        return;
    }

    if (en)
    {
        if (onboard_spk->pa_state)
        {
            /* pa already turn on */
            BK_LOGV(TAG, "%s, line: %d, pa already turn on \n", __func__, __LINE__);
            return;
        }
        else
        {
            if (onboard_spk->pa_turn_on_timer && delay_flag)
            {
                if (xTimerIsTimerActive(onboard_spk->pa_turn_on_timer))
                {
                    xTimerReset(onboard_spk->pa_turn_on_timer, portMAX_DELAY);
                }
                else
                {
                    BK_LOGD(TAG, "start pa_turn_on_timer, pa_on_delay: %d\n", onboard_spk->pa_on_delay);
                    xTimerStart(onboard_spk->pa_turn_on_timer, portMAX_DELAY);
                }
            }
            else
            {
                /* not need delay */
                _pa_gpio_ctrl(onboard_spk->pa_ctrl_gpio, onboard_spk->pa_on_level, true);
                if (onboard_spk->dig_gain > BK_AUD_DAC_DIG_GAIN_DB_SILENCE)
                {
                    bk_aud_dac_unmute();
                    BK_LOGV(TAG, "%s, line: %d, audio dac unmute\n", __func__, __LINE__);
                }
                onboard_spk->pa_state = true;
            }
        }
    }
    else
    {
        if (onboard_spk->pa_turn_on_timer)
        {
            if (xTimerIsTimerActive(onboard_spk->pa_turn_on_timer))
            {
                xTimerStop(onboard_spk->pa_turn_on_timer, portMAX_DELAY);
            }
        }

        if (!onboard_spk->pa_state)
        {
            /* pa already turn off */
            BK_LOGV(TAG, "%s, line: %d, pa already turn off \n", __func__, __LINE__);
            return;
        }

        /* mute -> turn off pa */
        bk_aud_dac_mute();
        BK_LOGV(TAG, "%s, line: %d, audio dac mute\n", __func__, __LINE__);
        if (onboard_spk->pa_off_delay)
        {
            rtos_delay_milliseconds(onboard_spk->pa_off_delay);
        }
        _pa_gpio_ctrl(onboard_spk->pa_ctrl_gpio, onboard_spk->pa_on_level, false);
        onboard_spk->pa_state = false;
    }
}

/* PA turn on callback */
static void pa_turn_on_timer_callback(TimerHandle_t xTimer)
{
    onboard_speaker_stream_t *onboard_spk = (onboard_speaker_stream_t *)pvTimerGetTimerID(xTimer);

    /* turn on pa according to congfig */
    _pa_gpio_ctrl(onboard_spk->pa_ctrl_gpio, onboard_spk->pa_on_level, true);

    if (onboard_spk->dig_gain > BK_AUD_DAC_DIG_GAIN_DB_SILENCE)
    {
        bk_aud_dac_unmute();
        BK_LOGV(TAG, "%s, line: %d, audio dac unmute\n", __func__, __LINE__);
    }

    onboard_spk->pa_state = true;
    BK_LOGD(TAG, "turn on pa complete, pa_ctrl_gpio: %d, pa_on_level: %d\n", onboard_spk->pa_ctrl_gpio, onboard_spk->pa_on_level);
}

static bk_err_t aud_dac_dma_deconfig(onboard_speaker_stream_t *onboard_spk)
{
    uint32_t i;

    if (onboard_spk == NULL)
    {
        return BK_OK;
    }

    for (i = 0; i < AUD_DAC_SOURCE_MAX; i++)
    {
        if (0xff != onboard_spk->spk_dma_id[i])
        {
            bk_dma_deinit(onboard_spk->spk_dma_id[i]);
            bk_dma_free(DMA_DEV_AUDIO, onboard_spk->spk_dma_id[i]);
            if (onboard_spk->spk_ring_buff[i])
            {
                ring_buffer_clear(&onboard_spk->spk_rb[i]);
                audio_dma_mem_free(onboard_spk->spk_ring_buff[i]);
                onboard_spk->spk_ring_buff[i] = NULL;
            }

            onboard_spk->spk_dma_id[i] = 0xff;
        }

        if (onboard_spk->dac_source_bitmap & (1 << i))
        {
            bk_aud_dac_source_enable(0, i, 0);
        }
    }

    return BK_OK;
}

/* Carry one frame audio dac data(20ms) to DAC FIFO complete */
static void aud_dac_dma_finish_isr(dma_id_t dma_id)
{
    uint32_t i;
    bk_err_t ret = BK_OK;
    uint32_t src_bit;

    AUD_DAC_DMA_ISR_START();
    BK_LOGV(TAG, "%s,dma_id:%d finish!\n", __func__, dma_id);

    for (i = 0; i < AUD_DAC_SOURCE_MAX; i++)
    {
        if (gl_onboard_speaker->spk_dma_id[i] == dma_id)
        {
            src_bit = (1U << i);
            spk_dma_finish_bitmap |= src_bit;

            if ((spk_dma_finish_bitmap & src_bit) == src_bit)
            {
                ret = rtos_set_semaphore(&gl_onboard_speaker->can_process);
                if (ret != BK_OK)
                {
                    BK_LOGE(TAG, "%s, %d, source:%d, dma finish bitmap:0x%x, rtos_set_semaphore fail, ret:%d \n",
                            __func__, __LINE__, i, spk_dma_finish_bitmap, ret);
                }
                spk_dma_finish_bitmap &= ~src_bit;
                gl_onboard_speaker->wr_spk_rb_done[i] = false;
                BK_LOGV(TAG, "%s,%d, source:%d, rtos_set_semaphore ok \n", __func__, __LINE__, i);
            }
            break;
        }
    }

    AUD_DAC_DMA_ISR_END();
}

static bk_err_t aud_dac_dma_config(onboard_speaker_stream_t *onboard_spk)
{
    bk_err_t ret = BK_OK;
    dma_config_t dma_config = {0};
    uint32_t dac_port_addr;
    uint32_t i = 0;
    uint32_t fifo_ch;
    uint32_t dma_buf_size, transfer_len;

#if 0
    /* init dma driver */
    ret = bk_dma_driver_init();
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, dma_driver_init fail\n", __func__, __LINE__);
        goto exit;
    }
#endif

    BK_LOGD(TAG, "dac_source_bitmap:0x%x, chl_num:%d\n",onboard_spk->dac_source_bitmap, onboard_spk->chl_num);
    for(i = 0; i < AUD_DAC_SOURCE_MAX; i++)
    {
        if(onboard_spk->dac_source_bitmap & (1 << i))
        {
            if (onboard_spk_hw_stereo_split_mode(onboard_spk))
            {
                /* HW stereo_en: one DMA, 32-bit L|R words; A2DP upsample uses dma_frame_size */
                uint32_t hw_xfer_bytes = onboard_spk_hw_dma_xfer_bytes(onboard_spk, i);

                dma_buf_size = hw_xfer_bytes * 2;
                transfer_len = hw_xfer_bytes;
            }
            else if (AUD_DAC_SOURCE_A2DP == i)
            {
                dma_buf_size = DEFAULT_AUD_DAC_SAMPLE_RATE * 20 / 1000 * 4;//A2DP always use 48000 sample rate
                transfer_len = onboard_spk->dma_frame_size;
            }
            else
            {
                dma_buf_size = onboard_spk->frame_size[i] * 2;
                transfer_len = onboard_spk->frame_size[i];
            }

            fifo_ch = onboard_spk_dma_fifo_ch(onboard_spk);

            onboard_spk->spk_dma_id[i] = bk_dma_alloc(DMA_DEV_AUDIO);
            if ((onboard_spk->spk_dma_id[i] < DMA_ID_0) || (onboard_spk->spk_dma_id[i] >= DMA_ID_MAX))
            {
                BK_LOGE(TAG, "malloc dma fail \n");
                goto exit;
            }

            /* two frames ringbuffer; +8 bytes guard for DMA pause address */
            onboard_spk->spk_ring_buff[i] = (int8_t *)audio_dma_mem_calloc(2, dma_buf_size + DMA_CARRY_SPK_RINGBUF_SAFE_INTERVAL / 2);
            AUDIO_MEM_CHECK(TAG, onboard_spk->spk_ring_buff[i], return BK_FAIL);
            ring_buffer_init(&onboard_spk->spk_rb[i], (uint8_t *)onboard_spk->spk_ring_buff[i],
                             dma_buf_size * 2 + DMA_CARRY_SPK_RINGBUF_SAFE_INTERVAL,
                             onboard_spk->spk_dma_id[i], RB_DMA_TYPE_READ);
            BK_LOGD(TAG, "%s, %d, spk_ring_buff[%d]: %p, size: %d \n", __func__, __LINE__, i,
                    onboard_spk->spk_ring_buff[i], dma_buf_size * 2 + DMA_CARRY_SPK_RINGBUF_SAFE_INTERVAL);

            os_memset(&dma_config, 0, sizeof(dma_config_t));
            dma_config.mode       = DMA_WORK_MODE_REPEAT;
            dma_config.chan_prio  = 1;
            dma_config.src.dev    = DMA_DEV_DTCM;
            dma_config.src.width  = DMA_DATA_WIDTH_32BITS;
            dma_config.trans_type = DMA_TRANS_DEFAULT;
            switch (onboard_spk->bits)
            {
            case 16:
                dma_config.dst.width = onboard_spk_hw_stereo_split_mode(onboard_spk)
                    ? DMA_DATA_WIDTH_32BITS : DMA_DATA_WIDTH_16BITS;
                break;
            case 24:
                dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
                break;
            default:
                goto exit;
            }
            BK_LOGD(TAG, "%s, %d, dma dst width:%d\n", __func__, __LINE__, dma_config.dst.width);

            ret = bk_aud_dac_get_fifo_addr(i, fifo_ch, &dma_config.dst.dev, &dac_port_addr);
            if (ret != BK_OK)
            {
                BK_LOGE(TAG, "%s, %d, get source:%d,fifo:%d fail\n", __func__, __LINE__, i, fifo_ch);
                goto exit;
            }

            dma_config.dst.addr_inc_en  = DMA_ADDR_INC_ENABLE;
            dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
            dma_config.dst.start_addr   = dac_port_addr;
            dma_config.dst.end_addr     = dac_port_addr + 4;
            dma_config.src.addr_inc_en  = DMA_ADDR_INC_ENABLE;
            dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
            dma_config.src.start_addr   = (uint32_t)(uintptr_t)onboard_spk->spk_ring_buff[i];
            dma_config.src.end_addr     = (uint32_t)(uintptr_t)onboard_spk->spk_ring_buff[i]
                + dma_buf_size * 2 + DMA_CARRY_SPK_RINGBUF_SAFE_INTERVAL;
            ret = bk_dma_init(onboard_spk->spk_dma_id[i], &dma_config);
            if (ret != BK_OK)
            {
                BK_LOGE(TAG, "%s, %d, dma_init fail\n", __func__, __LINE__);
                goto exit;
            }

            bk_dma_set_transfer_len(onboard_spk->spk_dma_id[i], transfer_len);
#if (CONFIG_SPE)
            bk_dma_set_dest_sec_attr(onboard_spk->spk_dma_id[i], DMA_ATTR_SEC);
            bk_dma_set_src_sec_attr(onboard_spk->spk_dma_id[i], DMA_ATTR_SEC);
#endif
            bk_dma_register_isr(onboard_spk->spk_dma_id[i], NULL, (void *)aud_dac_dma_finish_isr);
            bk_dma_enable_finish_interrupt(onboard_spk->spk_dma_id[i]);

            BK_LOGD(TAG, "%s, %d, dma id:%d, source:%d, fifo:%d, dst dev:%d, s_addr:0x%x, len:%d\n",
                    __func__, __LINE__, onboard_spk->spk_dma_id[i], i, fifo_ch,
                    dma_config.dst.dev, dac_port_addr, transfer_len);
        }
    }

    return BK_OK;
exit:
    aud_dac_dma_deconfig(onboard_spk);
    return BK_FAIL;
}

static bk_err_t _onboard_speaker_open(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] _onboard_speaker_open \n", audio_element_get_tag(self));
    uint32_t free_size = 0;
    uint32_t i;
    bk_err_t ret;

    onboard_speaker_stream_t *onboard_spk = (onboard_speaker_stream_t *)audio_element_getdata(self);

    if (onboard_spk->is_open)
    {
        return BK_OK;
    }

    /* set read data timeout */
    audio_element_set_input_timeout(self, 0);   // 2000, 15 / portTICK_RATE_MS

    for(i = 0; i < AUD_DAC_SOURCE_MAX; i++)
    {
        if(onboard_spk->dac_source_bitmap & (1 << i))
        {
            ret = bk_aud_dac_source_enable(0, i, 1);
            if (ret != BK_OK)
            {
                BK_LOGE(TAG, "%s, %d, aud_dac_source_enable source:%d fail!\n", __func__, __LINE__, i);
                return BK_FAIL;
            }

            if (0xff != onboard_spk->spk_dma_id[i])
            {
                free_size = ring_buffer_get_free_size(&gl_onboard_speaker->spk_rb[i]);
                if (free_size)
                {
                    uint8_t *temp_data = (uint8_t *)audio_malloc(free_size - DMA_CARRY_SPK_RINGBUF_SAFE_INTERVAL);
                    AUDIO_MEM_CHECK(TAG, temp_data, return BK_FAIL);
                    os_memset(temp_data, 0x00, free_size - DMA_CARRY_SPK_RINGBUF_SAFE_INTERVAL);
                    ring_buffer_write(&gl_onboard_speaker->spk_rb[i], temp_data,
                                      free_size - DMA_CARRY_SPK_RINGBUF_SAFE_INTERVAL);
                    audio_free(temp_data);
                    temp_data = NULL;
                }

                if (!open_cnt)
                {
                    ret = bk_dma_start(onboard_spk->spk_dma_id[i]);
                    if (ret != BK_OK)
                    {
                        BK_LOGE(TAG, "%s, %d, dac dma start fail\n", __func__, __LINE__);
                        return BK_FAIL;
                    }
                    BK_LOGD(TAG, "%s, %d, spk_dma_id[%d]:%d start ok\n",
                            __func__, __LINE__, i, onboard_spk->spk_dma_id[i]);
                }
                else
                {
                    BK_LOGD(TAG, "%s, %d, open_cnt:%d > 0, skip src:%d dma start\n",
                            __func__, __LINE__, open_cnt, i);
                }
            }
        }
    }

    if (gl_onboard_speaker->pa_ctrl_en)
    {
        /* turn off pa */
        pa_ctrl_en(onboard_spk, false, false);
    }
    else
    {
        bk_aud_dac_mute();
        BK_LOGV(TAG, "%s, line: %d, audio dac mute\n", __func__, __LINE__);
    }

    ret = bk_aud_dac_start(onboard_spk->dac_chl);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, dac start fail, dac_chl:%d\n", __func__, __LINE__, onboard_spk->dac_chl);
        return BK_FAIL;
    }

    onboard_spk->is_open = true;
    onboard_spk->valid_frame_count_in_spk_rb = 2;
    onboard_spk_update_status_by_energy(self, onboard_spk, 0, true);

    /* turn on pa */
    if (onboard_spk->pa_ctrl_en)
    {
        pa_ctrl_en(onboard_spk, true, true);
    }
    else
    {
        if (onboard_spk->dig_gain > BK_AUD_DAC_DIG_GAIN_DB_SILENCE)
        {
            rtos_delay_milliseconds(4);
            bk_aud_dac_unmute();
            BK_LOGD(TAG, "%s, line: %d, audio dac unmute\n", __func__, __LINE__);
        }
    }
    open_cnt++;

    return BK_OK;
}

static int _onboard_speaker_write(audio_port_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    audio_element_handle_t el = (audio_element_handle_t)context;
    BK_LOGV(TAG, "[%s] _onboard_speaker_write, len: %d \n", audio_element_get_tag(el), len);
    int ret = BK_OK;
    if (len)
    {
        //write some data to speaker pool
    }
    else
    {
        ret = len;
    }
    //BK_LOGD(TAG, "%s, ret: %d\n", __func__, ret);
    return ret;
}

static bk_err_t audio_dac_reconfig(onboard_speaker_stream_t *onboard_spk, int rate, int ch, int bits, aud_dac_source_t dma_src)
{
    bk_err_t ret = BK_OK;

    /* check and set sample rate, channel number, bits */
    if(AUD_DAC_SOURCE_A2DP != dma_src)
    {
        if (onboard_spk->sample_rate[dma_src] != rate)
        {
            if (BK_OK != bk_aud_dac_set_sample_rate(dma_src, rate))
            {
                BK_LOGE(TAG, "%s, line: %d, updata onboard speaker sample rate: %d fail \n", __func__, __LINE__, rate);
                return BK_FAIL;
            }
            else
            {
                BK_LOGD(TAG, "%s, line: %d, updata onboard speaker sample rate: %d ok \n", __func__, __LINE__, rate);
            }
        }
    }
    else
    {
        if (onboard_spk_a2dp_src_unsupported(rate))
        {
            BK_LOGE(TAG, "%s, line: %d, A2DP 8k is not supported, please change source or sample rate \n", __func__, __LINE__);
            return BK_FAIL;
        }

        if (onboard_spk->rsp_handler[AUD_DAC_SOURCE_A2DP])
        {
            bk_aud_rsp_deinit_multi_instance(onboard_spk->rsp_handler[AUD_DAC_SOURCE_A2DP]);
            onboard_spk->rsp_handler[AUD_DAC_SOURCE_A2DP] = NULL;
        }
        onboard_spk->sample_rate[AUD_DAC_SOURCE_A2DP] = rate;

        if (onboard_spk_a2dp_src_need_resample(rate))
        {
            onboard_spk->dma_frame_size = onboard_spk->frame_size[AUD_DAC_SOURCE_A2DP] * DEFAULT_AUD_DAC_SAMPLE_RATE
                / onboard_spk->sample_rate[AUD_DAC_SOURCE_A2DP];
            if (BK_OK != bk_aud_dac_set_sample_rate(AUD_DAC_SOURCE_A2DP, DEFAULT_AUD_DAC_SAMPLE_RATE))
            {
                BK_LOGE(TAG, "%s, line: %d, set A2DP dac sample rate %d fail \n", __func__, __LINE__, DEFAULT_AUD_DAC_SAMPLE_RATE);
                return BK_FAIL;
            }
            onboard_spk->rsp_cfg[AUD_DAC_SOURCE_A2DP].src_rate = rate;
            onboard_spk->rsp_cfg[AUD_DAC_SOURCE_A2DP].dest_rate = DEFAULT_AUD_DAC_SAMPLE_RATE;
            onboard_spk_rsp_set_channel_config(onboard_spk, AUD_DAC_SOURCE_A2DP);

            BK_LOGE(TAG, "%s, %d, rsp[%d] reconfig: src ch:%d,src rate:%d, dest rate:%d,\n",
                __func__, __LINE__, AUD_DAC_SOURCE_A2DP,
                onboard_spk->rsp_cfg[AUD_DAC_SOURCE_A2DP].src_ch,
                onboard_spk->rsp_cfg[AUD_DAC_SOURCE_A2DP].src_rate,
                onboard_spk->rsp_cfg[AUD_DAC_SOURCE_A2DP].dest_rate);

            ret = bk_aud_rsp_init_multi_instance(onboard_spk->rsp_cfg[AUD_DAC_SOURCE_A2DP], &onboard_spk->rsp_handler[AUD_DAC_SOURCE_A2DP]);
            if (ret != BK_OK)
            {
                BK_LOGE(TAG, "%s, %d, audio resampler[%d] init fail\n", __func__, __LINE__, AUD_DAC_SOURCE_A2DP);
            }
            if (onboard_spk->rsp_out_buff[AUD_DAC_SOURCE_A2DP] == NULL)
            {
                onboard_spk->rsp_out_buff[AUD_DAC_SOURCE_A2DP] = audio_calloc(1, DEFAULT_AUD_DAC_RSP_BUF_SIZE);
                if (!onboard_spk->rsp_out_buff[AUD_DAC_SOURCE_A2DP])
                {
                    BK_LOGE(TAG, "%s, %d, malloc rsp[%d] output buffer fail\n", __func__, __LINE__, AUD_DAC_SOURCE_A2DP);
                    return BK_FAIL;
                }
            }
        }
        else
        {
            onboard_spk->dma_frame_size = onboard_spk->frame_size[AUD_DAC_SOURCE_A2DP];
            if (BK_OK != bk_aud_dac_set_sample_rate(AUD_DAC_SOURCE_A2DP, rate))
            {
                BK_LOGE(TAG, "%s, line: %d, set A2DP dac sample rate: %d fail \n", __func__, __LINE__, rate);
                return BK_FAIL;
            }
            if (onboard_spk->rsp_out_buff[AUD_DAC_SOURCE_A2DP])
            {
                audio_free(onboard_spk->rsp_out_buff[AUD_DAC_SOURCE_A2DP]);
                onboard_spk->rsp_out_buff[AUD_DAC_SOURCE_A2DP] = NULL;
            }
        }

        if (onboard_spk_hw_stereo_split_mode(onboard_spk))
        {
            bk_dma_set_transfer_len(onboard_spk->spk_dma_id[AUD_DAC_SOURCE_A2DP],
                                    onboard_spk_hw_dma_xfer_bytes(onboard_spk, AUD_DAC_SOURCE_A2DP));
        }
        else
        {
            bk_dma_set_transfer_len(onboard_spk->spk_dma_id[AUD_DAC_SOURCE_A2DP], onboard_spk->dma_frame_size);
        }
    }

    /* sync dac_chl with PCM channel count when port format changes */
    if (onboard_spk->chl_num != ch)
    {
        if (ch == 2)
        {
            onboard_spk->dac_chl = AUD_DAC_CHL_LR;
        }
        else if (onboard_spk->dac_chl == AUD_DAC_CHL_LR)
        {
            onboard_spk->dac_chl = AUD_DAC_CHL_L;
        }
        onboard_spk->chl_num = ch;

        if (BK_OK != bk_aud_dac_start(onboard_spk->dac_chl))
        {
            BK_LOGE(TAG, "%s, line: %d, updata onboard speaker pcm chl_num: %d fail \n", __func__, __LINE__, ch);
            return BK_FAIL;
        }
        else
        {
            BK_LOGD(TAG, "%s, line: %d, updata onboard speaker pcm chl_num: %d ok \n", __func__, __LINE__, ch);
        }

        /* single DMA channel only: LR hw-split uses 32-bit packed words */
        dma_data_width_t dst_width = DMA_DATA_WIDTH_32BITS;
        if (bits == 16 && onboard_spk->dac_chl != AUD_DAC_CHL_LR)
        {
            dst_width = DMA_DATA_WIDTH_16BITS;
        }
        ret = bk_dma_set_dest_data_width(onboard_spk->spk_dma_id[dma_src], dst_width);

        if (ret != BK_OK)
        {
            BK_LOGE(TAG, "%s, line: %d, set dest_data_width fail \n", __func__, __LINE__);
            return BK_FAIL;
        }
        else
        {
            BK_LOGD(TAG, "%s, line: %d, set dest_data_width ok \n", __func__, __LINE__);
        }
    }
    else
    {
        BK_LOGD(TAG, "%s, line: %d, ch: %d unchanged! \n", __func__, __LINE__, ch);
    }

    return BK_OK;
}

#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
/* Check whether audio dac configuration need to be updated */
static bk_err_t _update_dac_config(audio_element_handle_t onboard_speaker_stream, uint8_t current_port_id, uint8_t new_port_id, aud_dac_source_t dma_src)
{
    onboard_speaker_stream_t *onboard_spk = (onboard_speaker_stream_t *)audio_element_getdata(onboard_speaker_stream);
    bk_err_t ret = BK_OK;

    audio_port_info_t *current_port_info = audio_port_info_list_get_by_port_id(&onboard_spk->input_port_list, current_port_id);
    audio_port_info_t *new_port_info = audio_port_info_list_get_by_port_id(&onboard_spk->input_port_list, new_port_id);

    if (new_port_info)
    {
        /* Check whether the port infomation is changed */
        if (current_port_info && current_port_info->sample_rate == new_port_info->sample_rate && current_port_info->chl_num == new_port_info->chl_num && current_port_info->bits == new_port_info->bits)
        {
            BK_LOGD(TAG, "%s, line: %d, the port infomation is not changed \n", __func__, __LINE__);
            onboard_spk->current_port_id = new_port_id;
        }
        else
        {
            /* update dac configuration */
            _onboard_speaker_close(onboard_speaker_stream);
            #if 0
            if (!current_port_info || current_port_info->sample_rate != new_port_info->sample_rate)
            {
                if (BK_OK != bk_aud_dac_set_sample_rate(dma_src, new_port_info->sample_rate))
                {
                    BK_LOGE(TAG, "%s, line: %d, updata onboard speaker sample rate: %d fail \n", __func__, __LINE__, new_port_info->sample_rate);
                }
                else
                {
                    BK_LOGD(TAG, "%s, line: %d, updata onboard speaker sample rate: %d->%d ok \n", __func__, __LINE__, current_port_info ? current_port_info->sample_rate : -1, new_port_info->sample_rate);
                }
            }
            #endif

            if (!current_port_info || current_port_info->chl_num != new_port_info->chl_num)
            {
                if (new_port_info->chl_num != 1 && new_port_info->chl_num != 2)
                {
                    BK_LOGE(TAG, "%s, line: %d, invalid pcm chl_num: %d \n", __func__, __LINE__, new_port_info->chl_num);
                    return BK_FAIL;
                }
                if (new_port_info->chl_num == 2)
                {
                    onboard_spk->dac_chl = AUD_DAC_CHL_LR;
                }
                else if (onboard_spk->dac_chl == AUD_DAC_CHL_LR)
                {
                    onboard_spk->dac_chl = AUD_DAC_CHL_L;
                }
                onboard_spk->chl_num = new_port_info->chl_num;
                if (BK_OK != bk_aud_dac_start(onboard_spk->dac_chl))
                {
                    BK_LOGE(TAG, "%s, line: %d, updata onboard speaker dac_chl:%d fail \n", __func__, __LINE__, onboard_spk->dac_chl);
                }
                else
                {
                    BK_LOGD(TAG, "%s, line: %d, updata pcm chl_num:%d dac_chl:%d ok \n", __func__, __LINE__,
                            onboard_spk->chl_num, onboard_spk->dac_chl);
                }

                /* single DMA channel only: LR hw-split uses 32-bit packed words */
                dma_data_width_t dst_width = DMA_DATA_WIDTH_32BITS;
                if (new_port_info->bits == 16 && onboard_spk->dac_chl != AUD_DAC_CHL_LR)
                {
                    dst_width = DMA_DATA_WIDTH_16BITS;
                }
                ret = bk_dma_set_dest_data_width(onboard_spk->spk_dma_id[dma_src], dst_width);

                if (ret != BK_OK)
                {
                    BK_LOGE(TAG, "%s, line: %d, set dest_data_width fail, chl_num:%d \n", __func__, __LINE__, new_port_info->chl_num);
                }
                else
                {
                    BK_LOGD(TAG, "%s, line: %d, set dest_data_width ok, chl_num:%d \n", __func__, __LINE__, new_port_info->chl_num);
                }
            }

            if (!current_port_info || current_port_info->bits != new_port_info->bits)
            {
                //TODO
                BK_LOGD(TAG, "%s, line: %d, updata onboard speaker bits: %d->%d ok \n", __func__, __LINE__, current_port_info ? current_port_info->bits : -1, new_port_info->bits);
            }
            onboard_spk->current_port_id = new_port_id;
            _onboard_speaker_open(onboard_speaker_stream);
        }
    }
    else
    {
        BK_LOGE(TAG, "%s, line: %d, new_port_id: %d is not valid \n", __func__, __LINE__, new_port_id);
        return BK_FAIL;
    }

    return BK_OK;
}
#endif

static int _onboard_speaker_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    onboard_speaker_stream_t *onboard_spk = (onboard_speaker_stream_t *)audio_element_getdata(self);
    uint32_t main_src = onboard_spk->main_dac_source;
    int r_size = 0;
    int w_size = 0;
    uint32_t write_size;
    uint8_t *write_addr;
    uint32_t rsp_out_len,rsp_in_len;
    bk_err_t ret;

    if (BK_OK != rtos_get_semaphore(&onboard_spk->can_process, 2000 / portTICK_RATE_MS)) //portMAX_DELAY, 25 / portTICK_RATE_MS
    {
        //return -1;
        BK_LOGE(TAG, "[%s] semaphore get timeout 2000ms\n", audio_element_get_tag(self));
    }
    AUD_ONBOARD_SPK_PROCESS_START();
    BK_LOGV(TAG, "[%s] _onboard_speaker_process \n", audio_element_get_tag(self));

    /* check whether pool enable */

    /* read input data */
#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
    uint32_t i;

    for(i = 0; i < AUD_DAC_SOURCE_MAX; i++)
    {
        if(gl_onboard_speaker->dac_source_bitmap & (1 << i))
        {
            AUD_ONBOARD_SPK_INPUT_START();
            if (i == onboard_spk->main_dac_source)
            {
                r_size = audio_element_input(self, in_buffer, onboard_spk->frame_size[i]);
            }
            else
            {
                int multi_port_id = onboard_spk_get_multi_input_port_by_src(onboard_spk, i);
                if (multi_port_id < 0)
                {
                    continue;
                }

                if(multi_port_id < audio_element_get_multi_input_max_port_num(self))
                {
                    if(onboard_spk->wr_spk_rb_done[i] == false)
                    {
                        uint32_t wanted_size = onboard_spk->frame_size[i];
                        uint8_t port_id = (uint8_t)(multi_port_id + 1);
                        audio_port_info_t *port_info = NULL;

                        if (onboard_spk->lock)
                        {
                            input_port_list_block(onboard_spk->lock, portMAX_DELAY);
                        }
                        port_info = audio_port_info_list_get_by_port_id(&onboard_spk->input_port_list, port_id);
                        if (onboard_spk->lock)
                        {
                            input_port_list_release(onboard_spk->lock);
                        }

                        /* If multi-in port provides mono but speaker runs stereo, read mono bytes first. */
                        if (port_info && port_info->chl_num == 1 && onboard_spk->chl_num > 1)
                        {
                            wanted_size = onboard_spk->frame_size[i] / 2;
                        }

                        r_size = audio_element_multi_input(self, in_buffer, wanted_size, multi_port_id, 0);
                        if(r_size <= 0)
                        {
                            continue;
                        }

                        /* Convert mono -> interleaved stereo (L=R) when speaker runs stereo. */
                        if (port_info && port_info->chl_num == 1 && onboard_spk->chl_num > 1)
                        {
                            int16_t *out_lr = (int16_t *)onboard_spk->temp_buff;
                            const int16_t *in_mono = (const int16_t *)in_buffer;
                            uint32_t mono_samples = (uint32_t)r_size / sizeof(int16_t);

                            for (uint32_t s = 0; s < mono_samples; s++)
                            {
                                int16_t v = in_mono[s];
                                out_lr[2 * s + 0] = v;
                                out_lr[2 * s + 1] = v;
                            }

                            /* overwrite in_buffer with stereo interleaved for downstream */
                            os_memcpy(in_buffer, out_lr, (size_t)r_size * 2);
                            r_size = (int)onboard_spk->frame_size[i];
                        }
                    }
                    else
                    {
                        continue;//skip reading input if dma interrupt not received
                    }
                }
                else
                {
                    break;//no more multi input port
                }
            }
            AUD_ONBOARD_SPK_INPUT_END();


            if (onboard_spk->wr_spk_rb_done[i] == false)
            {
                BK_LOGV(TAG, "%s,%d,r_size:%d,onboard_spk->frame_size[%d]:%d\n", __func__, __LINE__, r_size, i, onboard_spk->frame_size[i]);

                if (r_size == onboard_spk->frame_size[i])
                {
                    const int16_t *play_pcm = (const int16_t *)in_buffer;
                    uint32_t play_pcm_bytes = r_size;

                    if (AUD_DAC_SOURCE_A2DP == i && onboard_spk->rsp_handler[i])
                    {
                        play_pcm_bytes = onboard_spk->dma_frame_size;
                        if (onboard_spk->chl_num > 1)
                        {
                            ret = onboard_spk_rsp_process_frame(onboard_spk, i, play_pcm,
                                                                (int16_t *)onboard_spk->rsp_out_buff[i],
                                                                &play_pcm_bytes);
                            if (BK_OK != ret)
                            {
                                BK_LOGE(TAG, "%s:%d A2DP stereo resample fail\n", __func__, __LINE__);
                            }
                            play_pcm = (const int16_t *)onboard_spk->rsp_out_buff[i];
                        }
                        else
                        {
                            rsp_out_len = play_pcm_bytes / 2;
                            rsp_in_len = r_size / 2;
                            ret = bk_aud_rsp_process_multi_instance((int16_t *)in_buffer,
                                                                    &rsp_in_len,
                                                                    (int16_t *)onboard_spk->rsp_out_buff[i],
                                                                    &rsp_out_len,
                                                                    onboard_spk->rsp_handler[i]);
                            if (BK_OK != ret)
                            {
                                BK_LOGE(TAG, "%s:%d resample fail\n", __func__, __LINE__);
                            }
                            play_pcm = (const int16_t *)onboard_spk->rsp_out_buff[i];
                            play_pcm_bytes = rsp_out_len * sizeof(int16_t);
                        }
                    }

                    write_addr = onboard_spk_prepare_dac_write_buf(onboard_spk, play_pcm, play_pcm_bytes, &write_size);
#ifdef AEC_MIC_DELAY_POINTS_DEBUG
                    aec_mic_delay_debug((int16_t *)write_addr, write_size);
#endif
#ifdef SPK_DATA_DEBUG
                    change_pcm_data_to_8k((uint8_t *)write_addr, write_size);
#endif
                    ONBOARD_SPK_DATA_DUMP_BY_UART_DATA(write_addr, write_size);
                    ring_buffer_write(&onboard_spk->spk_rb[i],
                                      write_addr, write_size);
                    onboard_spk->wr_spk_rb_done[i] = true;
                    onboard_spk_update_status_by_energy(self, onboard_spk,
                        onboard_spk_calc_energy_level(write_addr, write_size, onboard_spk->bits), false);
                    onboard_spk_multi_output_pcm(self, onboard_spk, (const int16_t *)in_buffer, r_size);
                    ONBOARD_SPK_DATA_COUNT_ADD_SIZE(onboard_spk->frame_size[i]);
                }
                else
                {
                    uint32_t interleaved_bytes;

                    if (AUD_DAC_SOURCE_A2DP == i && onboard_spk->rsp_handler[i])
                    {
                        interleaved_bytes = onboard_spk->dma_frame_size;
                    }
                    else
                    {
                        interleaved_bytes = onboard_spk->frame_size[i];
                    }

                    os_memset(onboard_spk->temp_buff, 0x00, interleaved_bytes);
#ifdef AEC_MIC_DELAY_POINTS_DEBUG
                    aec_mic_delay_debug((int16_t *)onboard_spk->temp_buff, interleaved_bytes);
#endif
                    BK_LOGV(TAG, "[%s] fill silence data \n", audio_element_get_tag(self));
#ifdef SPK_DATA_DEBUG
                    change_pcm_data_to_8k((uint8_t *)onboard_spk->temp_buff, interleaved_bytes);
#endif
                    write_addr = onboard_spk_prepare_dac_write_buf(onboard_spk,
                        (const int16_t *)onboard_spk->temp_buff, interleaved_bytes, &write_size);
                    ONBOARD_SPK_DATA_DUMP_BY_UART_DATA(write_addr, write_size);
                    ring_buffer_write(&onboard_spk->spk_rb[i],
                                      write_addr, write_size);
                    onboard_spk->wr_spk_rb_done[i] = true;
                    onboard_spk_update_status_by_energy(self, onboard_spk, 0, false);
                    onboard_spk_multi_output_pcm(self, onboard_spk, (const int16_t *)onboard_spk->temp_buff,
                                               onboard_spk->frame_size[i]);
                    FILL_SILENCE_DATA_COUNT_ADD_SIZE(onboard_spk->frame_size[i]);
                }
            }

            if(0 ==  audio_element_get_multi_input_max_port_num(self))
            {
                if (r_size > 0)
                {
                    w_size = r_size;
                }
                else
                {
                    /* check r_size value
                       If r_size is AEL_IO_DONE, return AEL_IO_DONE until speaker data of pool_ring_buff is empty.
                       If r_size is AEL_IO_TIMEOUT, return in_len.
                       If r_size is others, return r_size.
                    */
                    if (r_size == AEL_IO_TIMEOUT)
                    {
                        w_size = in_len;
                    }
                    else if (r_size == AEL_IO_DONE)
                    {
                        /* two frames in speaker ring buffer playback finish */
                        if (onboard_spk->valid_frame_count_in_spk_rb == 0)
                        {
                            w_size = r_size;
                            onboard_spk_update_status(self, onboard_spk, false, 0, false);
                        }
                        else
                        {
                            onboard_spk->valid_frame_count_in_spk_rb -=1;
                            w_size = in_len;
                        }
                    }
                    else
                    {
#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
                        /* When reading data from other sources returns AEL_IO_ABORT, it indicates that the data source has stopped.
                           In this case, no error code is returned to prevent the speaker from stopping playback.
                         */
                        if (onboard_spk->current_port_id > 0)
                        {
                            w_size = in_len;
                        }
                        else
                        {
                            w_size = r_size;
                        }
#else
                        w_size = r_size;
#endif
                    }
                }
            }
            else
            {
                //no error code is returned to prevent the speaker from stopping playback
                w_size = onboard_spk->frame_size[main_src];
            }
        }
    }
#else
    AUD_ONBOARD_SPK_INPUT_START();
    r_size = audio_element_input(self, in_buffer, onboard_spk->frame_size[main_src]);
    AUD_ONBOARD_SPK_INPUT_END();

    if (onboard_spk->wr_spk_rb_done[main_src] == false)
    {
        BK_LOGV(TAG, "%s,%d,r_size:%d,onboard_spk->frame_size:%d\n", __func__, __LINE__, r_size, onboard_spk->frame_size[main_src]);
        if (r_size == onboard_spk->frame_size[main_src])
        {
            const int16_t *play_pcm = (const int16_t *)in_buffer;
            uint32_t play_pcm_bytes = r_size;

            if (onboard_spk->rsp_handler[main_src])
            {
                play_pcm_bytes = onboard_spk->dma_frame_size;
                if (onboard_spk->chl_num > 1)
                {
                    ret = onboard_spk_rsp_process_frame(onboard_spk, main_src, play_pcm,
                                                        (int16_t *)onboard_spk->rsp_out_buff[main_src],
                                                        &play_pcm_bytes);
                    if (BK_OK != ret)
                    {
                        BK_LOGE(TAG, "%s:%d A2DP stereo resample fail\n", __func__, __LINE__);
                    }
                    play_pcm = (const int16_t *)onboard_spk->rsp_out_buff[main_src];
                }
                else
                {
                    rsp_out_len = play_pcm_bytes / 2;
                    rsp_in_len = r_size / 2;
                    ret = bk_aud_rsp_process_multi_instance((int16_t *)in_buffer,
                                                            &rsp_in_len,
                                                            (int16_t *)onboard_spk->rsp_out_buff[main_src],
                                                            &rsp_out_len,
                                                            onboard_spk->rsp_handler[main_src]);
                    if (BK_OK != ret)
                    {
                        BK_LOGE(TAG, "%s:%d resample fail\n", __func__, __LINE__);
                    }
                    play_pcm = (const int16_t *)onboard_spk->rsp_out_buff[main_src];
                    play_pcm_bytes = rsp_out_len * sizeof(int16_t);
                }
            }

            write_addr = onboard_spk_prepare_dac_write_buf(onboard_spk, play_pcm, play_pcm_bytes, &write_size);
#ifdef AEC_MIC_DELAY_POINTS_DEBUG
            aec_mic_delay_debug((int16_t *)write_addr, write_size);
#endif
#ifdef SPK_DATA_DEBUG
            change_pcm_data_to_8k((uint8_t *)write_addr, write_size);
#endif
            ONBOARD_SPK_DATA_DUMP_BY_UART_DATA(write_addr, write_size);
            ring_buffer_write(&onboard_spk->spk_rb[main_src],
                              write_addr, write_size);
            onboard_spk->wr_spk_rb_done[main_src] = true;
            onboard_spk_update_status_by_energy(self, onboard_spk,
                onboard_spk_calc_energy_level(write_addr, write_size, onboard_spk->bits), false);
            onboard_spk_multi_output_pcm(self, onboard_spk, (const int16_t *)in_buffer, r_size);
            ONBOARD_SPK_DATA_COUNT_ADD_SIZE(onboard_spk->frame_size[main_src]);
        }
        else
        {
            uint32_t interleaved_bytes = onboard_spk->rsp_handler[main_src]
                ? onboard_spk->dma_frame_size : onboard_spk->frame_size[main_src];

            os_memset(onboard_spk->temp_buff, 0x00, interleaved_bytes);
#ifdef AEC_MIC_DELAY_POINTS_DEBUG
            aec_mic_delay_debug((int16_t *)onboard_spk->temp_buff, interleaved_bytes);
#endif
            BK_LOGV(TAG, "[%s] fill silence data \n", audio_element_get_tag(self));
#ifdef SPK_DATA_DEBUG
            change_pcm_data_to_8k((uint8_t *)onboard_spk->temp_buff, interleaved_bytes);
#endif
            write_addr = onboard_spk_prepare_dac_write_buf(onboard_spk,
                (const int16_t *)onboard_spk->temp_buff, interleaved_bytes, &write_size);
            ONBOARD_SPK_DATA_DUMP_BY_UART_DATA(write_addr, write_size);
            ring_buffer_write(&onboard_spk->spk_rb[main_src],
                              write_addr, write_size);
            onboard_spk->wr_spk_rb_done[main_src] = true;
            onboard_spk_update_status_by_energy(self, onboard_spk, 0, false);
            onboard_spk_multi_output_pcm(self, onboard_spk, (const int16_t *)onboard_spk->temp_buff,
                                       onboard_spk->frame_size[main_src]);
            FILL_SILENCE_DATA_COUNT_ADD_SIZE(onboard_spk->frame_size[main_src]);
        }
    }

    w_size = 0;
    if (r_size > 0)
    {
        AUD_ONBOARD_SPK_OUTPUT_START();
        /* call _onboard_speaker_write to play or pause if pool ring buffer is exist */
        {
            w_size = r_size;
        }
        AUD_ONBOARD_SPK_OUTPUT_END();
        /* Update the pointer for processing data */
        //audio_element_update_byte_pos(self, w_size);
    }
    else
    {
        /* check r_size value
           If r_size is AEL_IO_DONE, return AEL_IO_DONE until speaker data of pool_ring_buff is empty.
           If r_size is AEL_IO_TIMEOUT, return in_len.
           If r_size is others, return r_size.
        */
        if (r_size == AEL_IO_TIMEOUT)
        {
            /* call _onboard_speaker_write to play or pause if pool ring buffer is exist */
            w_size = in_len;
        }
        else if (r_size == AEL_IO_DONE)
        {
            /* two frames in speaker ring buffer playback finish */
            if (onboard_spk->valid_frame_count_in_spk_rb == 0)
            {
                w_size = r_size;
                onboard_spk_update_status(self, onboard_spk, false, 0, false);
            }
            else
            {
                {
                    onboard_spk->valid_frame_count_in_spk_rb -=1;
                }
                w_size = in_len;
            }
        }
        else
        {
#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
            /* When reading data from other sources returns AEL_IO_ABORT, it indicates that the data source has stopped.
               In this case, no error code is returned to prevent the speaker from stopping playback.
             */
            if (onboard_spk->current_port_id > 0)
            {
                w_size = in_len;
            }
            else
            {
                w_size = r_size;
            }
#else
            w_size = r_size;
#endif
        }
    }
#endif
    //w_size = onboard_spk->frame_size;

    AUD_ONBOARD_SPK_PROCESS_END();
    //BK_LOGD(TAG, "%s, %d, w_size: %d\n", __func__, __LINE__, w_size);
    return w_size;
}

static bk_err_t _onboard_speaker_close(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] _onboard_speaker_close \n", audio_element_get_tag(self));
    uint32_t i;
    bk_err_t ret;

    onboard_speaker_stream_t *onboard_spk = (onboard_speaker_stream_t *)audio_element_getdata(self);

    for(i = 0; i < AUD_DAC_SOURCE_MAX; i++)
    {
        if (!(onboard_spk->dac_source_bitmap & (1 << i)))
        {
            continue;
        }

        ret = bk_aud_dac_source_enable(0, i, 0);
        if (ret != BK_OK)
        {
            BK_LOGE(TAG, "%s, %d, aud_dac_source_disable source:%d fail!\n", __func__, __LINE__, i);
            return BK_FAIL;
        }
        BK_LOGD(TAG, "%s, %d, aud_dac_source_disable source:%d ok\n", __func__, __LINE__, i);
    }

    pa_ctrl_en(onboard_spk, false, false);

    ret = bk_aud_dac_stop(onboard_spk->dac_chl);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, dac stop fail, dac_chl:%d\n", __func__, __LINE__, onboard_spk->dac_chl);
        return BK_FAIL;
    }

    onboard_spk->is_open = false;

    for(i = 0; i < AUD_DAC_SOURCE_MAX; i++)
    {
        onboard_spk->wr_spk_rb_done[i] = false;
    }
    onboard_spk->valid_frame_count_in_spk_rb = 0;
    onboard_spk_update_status(self, onboard_spk, false, 0, true);

    return BK_OK;
}

static bk_err_t _onboard_speaker_destroy(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] _onboard_speaker_destroy \n", audio_element_get_tag(self));

    onboard_speaker_stream_t *onboard_spk = (onboard_speaker_stream_t *)audio_element_getdata(self);
    uint32_t i = 0;
    /* deinit dma */
    aud_dac_dma_deconfig(onboard_spk);
    /* deinit dac */
    bk_aud_dac_deinit();

    /* free spk pool */
    if (onboard_spk)
    {
        if(onboard_spk->temp_buff)
        {
            audio_free(onboard_spk->temp_buff);
            onboard_spk->temp_buff = NULL;
        }

        for(i = 0; i < AUD_DAC_SOURCE_MAX; i++)
        {
            if(onboard_spk->rsp_handler[i])
            {
                bk_aud_rsp_deinit_multi_instance(onboard_spk->rsp_handler[i]);
                onboard_spk->rsp_handler[i] = NULL;
                if (onboard_spk->rsp_out_buff[i])
                {
                    audio_free(onboard_spk->rsp_out_buff[i]);
                    onboard_spk->rsp_out_buff[i] = NULL;
                }
            }
        }
    }
    
    if (onboard_spk && onboard_spk->can_process)
    {
        rtos_deinit_semaphore(&onboard_spk->can_process);
        onboard_spk->can_process = NULL;
    }
    if (onboard_spk && onboard_spk->cfg_lock)
    {
        rtos_deinit_mutex(&onboard_spk->cfg_lock);
        onboard_spk->cfg_lock = NULL;
    }

    if (onboard_spk->pa_turn_on_timer)
    {
        xTimerDelete(onboard_spk->pa_turn_on_timer, portMAX_DELAY);
        onboard_spk->pa_turn_on_timer = NULL;
    }

#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
    /* free input port list */
    if (onboard_spk && onboard_spk->lock)
    {
        audio_port_info_list_clear(&onboard_spk->input_port_list);
        vSemaphoreDelete(onboard_spk->lock);
        onboard_spk->lock = NULL;
    }
#endif

    if (onboard_spk)
    {
        audio_free(onboard_spk);
        onboard_spk = NULL;
    }

    spk_dma_finish_bitmap = 0;
    open_cnt = 0;

    ONBOARD_SPK_DATA_COUNT_CLOSE();
    ONBOARD_SPK_DATA_DUMP_BY_UART_CLOSE();

    return BK_OK;
}

audio_element_handle_t onboard_speaker_stream_init(onboard_speaker_stream_cfg_t *config)
{
    audio_element_handle_t el;
    bk_err_t ret = BK_OK;
    uint32 i = 0, k = 0;
    aud_dac_chl_t effective_dac_chl;
    BK_LOGD(TAG, "%s, line:%d\n", __func__, __LINE__);

    gl_onboard_speaker = audio_calloc(1, sizeof(onboard_speaker_stream_t));
    AUDIO_MEM_CHECK(TAG, gl_onboard_speaker, return NULL);
    os_memset(gl_onboard_speaker, 0, sizeof(onboard_speaker_stream_t));

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open     = _onboard_speaker_open;
    cfg.close    = _onboard_speaker_close;
    cfg.process  = _onboard_speaker_process;
    cfg.destroy  = _onboard_speaker_destroy;
    cfg.out_type = PORT_TYPE_CB;
    cfg.write    = _onboard_speaker_write;
    cfg.in_type  = PORT_TYPE_RB;
    cfg.read     = NULL;
    cfg.task_stack = config->task_stack;
    cfg.task_prio  = config->task_prio;
    cfg.task_core  = config->task_core;

    cfg.multi_in_port_num  = config->multi_in_port_num;
    cfg.multi_out_port_num = config->multi_out_port_num;

    cfg.tag = "onboard_speaker";
    gl_onboard_speaker->chl_num = config->chl_num;

    k = config->frame_size[0];
    for(i = 0; i < AUD_DAC_SOURCE_MAX; i++)
    {
        gl_onboard_speaker->sample_rate[i] = config->sample_rate[i];
        gl_onboard_speaker->frame_size[i]  = config->frame_size[i];

        if(k < config->frame_size[i])
        {
            k = config->frame_size[i];
        }
    }

    /*input buffer size = max frame size*/
    cfg.buffer_len = k;
    BK_LOGD(TAG, "cfg.buffer_len: %d\n", cfg.buffer_len);

    uint32_t dma_frame_ref_src = AUD_DAC_SOURCE_A2DP;
    if (config->dac_source_bitmap & (1 << AUD_DAC_SOURCE_A2DP))
    {
        if (onboard_spk_a2dp_src_unsupported(gl_onboard_speaker->sample_rate[AUD_DAC_SOURCE_A2DP]))
        {
            BK_LOGE(TAG, "%s, %d, A2DP 8k is not supported, please change source or sample rate \n", __func__, __LINE__);
            goto _onboard_speaker_init_exit;
        }

        if (onboard_spk_a2dp_src_need_resample(gl_onboard_speaker->sample_rate[AUD_DAC_SOURCE_A2DP]))
        {
            gl_onboard_speaker->dma_frame_size = config->frame_size[AUD_DAC_SOURCE_A2DP] * DEFAULT_AUD_DAC_SAMPLE_RATE
                / gl_onboard_speaker->sample_rate[AUD_DAC_SOURCE_A2DP];
        }
        else
        {
            gl_onboard_speaker->dma_frame_size = config->frame_size[AUD_DAC_SOURCE_A2DP];
        }
    }
    else
    {
        for (i = 0; i < AUD_DAC_SOURCE_MAX; i++)
        {
            if (config->dac_source_bitmap & (1 << i))
            {
                dma_frame_ref_src = i;
                break;
            }
        }
        gl_onboard_speaker->dma_frame_size = config->frame_size[dma_frame_ref_src];
    }
    BK_LOGD(TAG, "%s, %d, gl_onboard_speaker->dma_frame_size:%d \n", __func__, __LINE__, gl_onboard_speaker->dma_frame_size);

    gl_onboard_speaker->dig_gain  = config->dig_gain;
    gl_onboard_speaker->ana_gain  = config->ana_gain;
    gl_onboard_speaker->work_mode = config->work_mode;
    gl_onboard_speaker->bits      = config->bits;
    gl_onboard_speaker->clk_src   = config->clk_src;
    gl_onboard_speaker->status.is_playing = false;
    gl_onboard_speaker->status.energy_level = 0;
    gl_onboard_speaker->play_energy_threshold  = (config->play_energy_threshold > 100) ? 100 : config->play_energy_threshold;
    gl_onboard_speaker->play_energy_hysteresis = (config->play_energy_hysteresis > 100) ? 100 : config->play_energy_hysteresis;
    gl_onboard_speaker->status_cb = config->status_cb;
    gl_onboard_speaker->status_cb_user_data = config->status_cb_user_data;

    gl_onboard_speaker->pool_length      = config->pool_length;
    gl_onboard_speaker->pool_play_thold  = config->pool_play_thold;
    gl_onboard_speaker->pool_pause_thold = config->pool_pause_thold;
    gl_onboard_speaker->pa_ctrl_en       = config->pa_ctrl_en;
    gl_onboard_speaker->pa_ctrl_gpio     = config->pa_ctrl_gpio;
    gl_onboard_speaker->pa_on_level      = config->pa_on_level;
    gl_onboard_speaker->pa_on_delay      = config->pa_on_delay;
    gl_onboard_speaker->pa_off_delay     = config->pa_off_delay;

#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
    gl_onboard_speaker->current_port_id = 0;
#endif
    gl_onboard_speaker->dac_source_bitmap = config->dac_source_bitmap;
    gl_onboard_speaker->main_dac_source   = config->main_dac_source;
    if (gl_onboard_speaker->main_dac_source >= AUD_DAC_SOURCE_MAX)
    {
        BK_LOGW(TAG, "%s, %d, main_dac_source:%d is invalid, fallback to default:%d \n",
            __func__, __LINE__, gl_onboard_speaker->main_dac_source, DEFAULT_DAC_SOURCE);
        gl_onboard_speaker->main_dac_source = DEFAULT_DAC_SOURCE;
    }

    if (!(gl_onboard_speaker->dac_source_bitmap & (1 << gl_onboard_speaker->main_dac_source)))
    {
        bool found_main_src = false;
        for (i = 0; i < AUD_DAC_SOURCE_MAX; i++)
        {
            if (gl_onboard_speaker->dac_source_bitmap & (1 << i))
            {
                gl_onboard_speaker->main_dac_source = i;
                found_main_src = true;
                break;
            }
        }

        if (!found_main_src)
        {
            BK_LOGE(TAG, "%s, %d, no active source in dac_source_bitmap:0x%x \n",
                __func__, __LINE__, gl_onboard_speaker->dac_source_bitmap);
            goto _onboard_speaker_init_exit;
        }

        BK_LOGW(TAG, "%s, %d, configured main_dac_source is inactive, fallback to source:%d \n",
            __func__, __LINE__, gl_onboard_speaker->main_dac_source);
    }

    /* init onboard speaker */
    aud_dac_config_t aud_dac_cfg = DEFAULT_AUD_DAC_CONFIG();
    if (config->chl_num != 1 && config->chl_num != 2)
    {
        BK_LOGE(TAG, "chl_num: %d invalid, use 1=mono, 2=stereo \n", config->chl_num);
        goto _onboard_speaker_init_exit;
    }
    if (!onboard_spk_dac_chl_config_valid(config->dac_chl))
    {
        BK_LOGE(TAG, "dac_chl: %d invalid \n", config->dac_chl);
        goto _onboard_speaker_init_exit;
    }
    if (config->chl_num == 2)
    {
        effective_dac_chl = AUD_DAC_CHL_LR;
    }
    else if (config->dac_chl == AUD_DAC_CHL_R)
    {
        effective_dac_chl = AUD_DAC_CHL_R;
    }
    else
    {
        effective_dac_chl = AUD_DAC_CHL_L;
    }
    gl_onboard_speaker->dac_chl = effective_dac_chl;
    aud_dac_cfg.dac_chl = effective_dac_chl;
    //aud_dac_cfg.sample_rate = config->sample_rate;
    aud_dac_cfg.work_mode = config->work_mode;
    aud_dac_cfg.clk_src   = config->clk_src;
    aud_dac_cfg.dig_gain  = config->dig_gain;
    aud_dac_cfg.ana_gain  = config->ana_gain;
    BK_LOGD(TAG, "dac_cfg pcm_chl_num:%d, dac_chl:%d, dig_gain_db:%.2f, clk_src:%s, dac_mode:%s \n",
            config->chl_num,
            aud_dac_cfg.dac_chl,
            aud_dac_cfg.dig_gain,
            aud_dac_cfg.clk_src == 1 ? "APLL" : "XTAL",
            aud_dac_cfg.work_mode == 1 ? "AUD_DAC_WORK_MODE_SIGNAL_END" : "AUD_DAC_WORK_MODE_DIFFEN");

    bk_aud_hardware_reset();
    ret = bk_aud_dac_init(&aud_dac_cfg);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, aud_dac_init fail\n", __func__, __LINE__);
        goto _onboard_speaker_init_exit;
    }

    if (aud_dac_cfg.dig_gain <= BK_AUD_DAC_DIG_GAIN_DB_SILENCE)
    {
        bk_aud_dac_mute();
        BK_LOGV(TAG, "%s, line: %d, audio dac mute\n", __func__, __LINE__);
    }
    else
    {
        bk_aud_dac_unmute();
        BK_LOGV(TAG, "%s, line: %d, audio dac unmute\n", __func__, __LINE__);
    }

    //TODO
    /* set speaker mode */
    /*
        if (config->chl_num == 1) {
            ret = bk_aud_dac_set_mic_mode(AUD_MIC_MIC1, config->adc_cfg.mode);
        } else {
            ret = bk_aud_adc_set_mic_mode(AUD_MIC_BOTH, config->adc_cfg.mode);
        }
    */
    os_memset(&gl_onboard_speaker->spk_rb, 0x00, sizeof(RingBufferContext) * AUD_DAC_SOURCE_MAX);
    gl_onboard_speaker->temp_buff = NULL;

    for(i = 0; i < AUD_DAC_SOURCE_MAX; i++)
    {
        gl_onboard_speaker->rsp_out_buff[i] = NULL;
        gl_onboard_speaker->rsp_handler[i] = NULL;
        gl_onboard_speaker->spk_dma_id[i] = 0xff;

        if(gl_onboard_speaker->dac_source_bitmap & (1 << i))
        {
            if (AUD_DAC_SOURCE_A2DP == i)
            {
                if (onboard_spk_a2dp_src_unsupported(gl_onboard_speaker->sample_rate[i]))
                {
                    BK_LOGE(TAG, "%s, %d, A2DP 8k is not supported, please change source or sample rate \n", __func__, __LINE__);
                    goto _onboard_speaker_init_exit;
                }

                if (onboard_spk_a2dp_src_need_resample(gl_onboard_speaker->sample_rate[i]))
                {
                    ret = bk_aud_dac_set_sample_rate(i, DEFAULT_AUD_DAC_SAMPLE_RATE);
                    if (ret != BK_OK)
                    {
                        BK_LOGE(TAG, "%s, %d, dac_set_sample_rate fail\n", __func__, __LINE__);
                        goto _onboard_speaker_init_exit;
                    }

                    gl_onboard_speaker->rsp_cfg[i].complexity  = 1;
                    gl_onboard_speaker->rsp_cfg[i].src_bits    = gl_onboard_speaker->bits;
                    gl_onboard_speaker->rsp_cfg[i].dest_bits   = gl_onboard_speaker->bits;
                    gl_onboard_speaker->rsp_cfg[i].src_rate    = gl_onboard_speaker->sample_rate[i];
                    gl_onboard_speaker->rsp_cfg[i].dest_rate   = DEFAULT_AUD_DAC_SAMPLE_RATE;
                    gl_onboard_speaker->rsp_cfg[i].down_ch_idx = 0;
                    onboard_spk_rsp_set_channel_config(gl_onboard_speaker, i);

                    BK_LOGE(TAG, "%s, %d, rsp[%d]: src ch:%d,src rate:%d, dest rate:%d,\n",
                        __func__, __LINE__, i,
                        gl_onboard_speaker->rsp_cfg[i].src_ch,
                        gl_onboard_speaker->rsp_cfg[i].src_rate,
                        gl_onboard_speaker->rsp_cfg[i].dest_rate);

                    ret = bk_aud_rsp_init_multi_instance(gl_onboard_speaker->rsp_cfg[i], &gl_onboard_speaker->rsp_handler[i]);
                    if (ret != BK_OK)
                    {
                        BK_LOGE(TAG, "%s, %d, audio resampler[%d] init fail\n", __func__, __LINE__, i);
                        goto _onboard_speaker_init_exit;
                    }

                    gl_onboard_speaker->rsp_out_buff[i] = audio_calloc(1, DEFAULT_AUD_DAC_RSP_BUF_SIZE);
                    if (!gl_onboard_speaker->rsp_out_buff[i])
                    {
                        BK_LOGE(TAG, "%s, %d, malloc rsp[%d] output buffer fail\n", __func__, __LINE__, i);
                        goto _onboard_speaker_init_exit;
                    }
                }
                else
                {
                    ret = bk_aud_dac_set_sample_rate(i, gl_onboard_speaker->sample_rate[i]);
                    if (ret != BK_OK)
                    {
                        BK_LOGE(TAG, "%s, %d, dac_set_sample_rate fail\n", __func__, __LINE__);
                        goto _onboard_speaker_init_exit;
                    }
                }
            }
            else
            {
                ret = bk_aud_dac_set_sample_rate(i, gl_onboard_speaker->sample_rate[i]);
                if (ret != BK_OK)
                {
                    BK_LOGE(TAG, "%s, %d, dac_set_sample_rate fail\n", __func__, __LINE__);
                    goto _onboard_speaker_init_exit;
                }
            }
            BK_LOGD(TAG, "%s, %d, set gl_onboard_speaker->sample_rate[%d]:%d, frame_size[%d]:%d\n",
                        __func__, __LINE__, i,
                        gl_onboard_speaker->sample_rate[i],i,gl_onboard_speaker->frame_size[i]);
        }
    }

    ret = aud_dac_dma_config(gl_onboard_speaker);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, dac_dma_init fail\n", __func__, __LINE__);
        goto _onboard_speaker_init_exit;
    }

    /* init speaker ringbuffer pool */
    gl_onboard_speaker->temp_buff_len = onboard_spk_temp_buff_len(gl_onboard_speaker, cfg.buffer_len);
    gl_onboard_speaker->temp_buff = (int8_t *)audio_calloc(1, gl_onboard_speaker->temp_buff_len);
    AUDIO_MEM_CHECK(TAG, gl_onboard_speaker->temp_buff, goto _onboard_speaker_init_exit);
    os_memset(gl_onboard_speaker->temp_buff, 0x00, gl_onboard_speaker->temp_buff_len);
    BK_LOGD(TAG, "temp_buff_len:%u (in_frame_max:%u dma_frame:%u)\n",
            gl_onboard_speaker->temp_buff_len, cfg.buffer_len, gl_onboard_speaker->dma_frame_size);

    ret = rtos_init_semaphore(&gl_onboard_speaker->can_process, DEFAULT_AUD_DAC_PROCESS_SEM_CNT);//mono only,stereo need futher development,todo
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, rtos_init_semaphore fail\n", __func__, __LINE__);
        goto _onboard_speaker_init_exit;
    }

    ret = rtos_init_mutex(&gl_onboard_speaker->cfg_lock);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, cfg_lock create fail\n", __func__, __LINE__);
        goto _onboard_speaker_init_exit;
    }

#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
    if (cfg.multi_in_port_num > 0)
    {
        audio_port_info_list_init(&gl_onboard_speaker->input_port_list);

        gl_onboard_speaker->lock = xSemaphoreCreateMutex();
        if (gl_onboard_speaker->lock == NULL)
        {
            BK_LOGE(TAG, "[%s] create semaphore fail \n", cfg.tag);
            goto _onboard_speaker_init_exit;
        }
    }
#endif

    if (gl_onboard_speaker->pa_ctrl_en)
    {
        if (gl_onboard_speaker->pa_on_delay > 0)
        {
            gl_onboard_speaker->pa_turn_on_timer = xTimerCreate(
                "pa_turn_on_timer",
                BK_MS_TO_TICKS(gl_onboard_speaker->pa_on_delay),
                pdFALSE,
                (void *)gl_onboard_speaker,
                pa_turn_on_timer_callback);
            if (gl_onboard_speaker->pa_turn_on_timer == NULL)
            {
                BK_LOGE(TAG, "create pa_turn_on_timer fail \n");
                goto _onboard_speaker_init_exit;
            }
        }

        /* config gpio to output */
        bk_gpio_enable_output(gl_onboard_speaker->pa_ctrl_gpio);
    }

    spk_dma_finish_bitmap = 0;
    open_cnt = 0;

    el = audio_element_init(&cfg);
    AUDIO_MEM_CHECK(TAG, el, goto _onboard_speaker_init_exit);
    audio_element_setdata(el, gl_onboard_speaker);

    audio_element_info_t info = {0};
    info.sample_rates = config->sample_rate[gl_onboard_speaker->main_dac_source];
    info.channels     = config->chl_num;
    info.bits         = config->bits;
    info.codec_fmt    = BK_CODEC_TYPE_PCM;
    audio_element_setinfo(el, &info);

    ONBOARD_SPK_DATA_COUNT_OPEN();
    ONBOARD_SPK_DATA_DUMP_BY_UART_OPEN();

    return el;
_onboard_speaker_init_exit:
    /* deinit dma */
    aud_dac_dma_deconfig(gl_onboard_speaker);
    /* deinit dac */
    bk_aud_dac_deinit();
    bk_aud_driver_deinit();
    /* free spk pool */

    if(gl_onboard_speaker->temp_buff)
    {
        audio_free(gl_onboard_speaker->temp_buff);
        gl_onboard_speaker->temp_buff = NULL;
    }

    for(i = 0; i < AUD_DAC_SOURCE_MAX; i++)
    {
        if(gl_onboard_speaker->rsp_handler[i])
        {
            bk_aud_rsp_deinit_multi_instance(gl_onboard_speaker->rsp_handler[i]);
            gl_onboard_speaker->rsp_handler[i] = NULL;
            if (gl_onboard_speaker->rsp_out_buff[i])
            {
                audio_free(gl_onboard_speaker->rsp_out_buff[i]);
                gl_onboard_speaker->rsp_out_buff[i] = NULL;
            }
        }
    }

    if (gl_onboard_speaker->can_process)
    {
        rtos_deinit_semaphore(&gl_onboard_speaker->can_process);
        gl_onboard_speaker->can_process = NULL;
    }
    if (gl_onboard_speaker->cfg_lock)
    {
        rtos_deinit_mutex(&gl_onboard_speaker->cfg_lock);
        gl_onboard_speaker->cfg_lock = NULL;
    }
    
#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
    /* Delete mutex lock, no need to free list nodes as they are not added yet in init phase */
    if (gl_onboard_speaker->lock)
    {
        vSemaphoreDelete(gl_onboard_speaker->lock);
        gl_onboard_speaker->lock = NULL;
    }
#endif

    if (gl_onboard_speaker->pa_turn_on_timer)
    {
        xTimerDelete(gl_onboard_speaker->pa_turn_on_timer, portMAX_DELAY);
        gl_onboard_speaker->pa_turn_on_timer = NULL;
    }

    audio_free(gl_onboard_speaker);
    gl_onboard_speaker = NULL;
    return NULL;
}

bk_err_t onboard_speaker_stream_set_param(audio_element_handle_t onboard_speaker_stream, int rate, int bits, int ch, aud_dac_source_t dma_src)
{
    bk_err_t err = BK_OK;
    onboard_speaker_stream_t *onboard_spk = (onboard_speaker_stream_t *)audio_element_getdata(onboard_speaker_stream);
    audio_element_state_t state = audio_element_get_state(onboard_speaker_stream);

    BK_LOGD(TAG, "%s \n", __func__);

    /* check param */
    if (rate != 8000 && rate != 110250 && rate != 12000 && rate != 16000 && rate != 22050 && rate != 24000 && rate != 32000 && rate != 44100 && rate != 48000)
    {
        BK_LOGE(TAG, "sample rate: %d is not support \n", rate);
        return BK_FAIL;
    }
    if (ch < 1 || ch > 2)
    {
        BK_LOGE(TAG, "pcm chl_num: %d is not support \n", ch);
        return BK_FAIL;
    }
    if (bits != 16)
    {
        BK_LOGE(TAG, "bits: %d is not support \n", bits);
        return BK_FAIL;
    }

    if (onboard_spk->sample_rate[dma_src] == rate && onboard_spk->chl_num == ch && onboard_spk->bits == bits)
    {
        BK_LOGD(TAG, "current dma_src:%d sample_rate: %d, chl_num: %d, bits: %d \n", dma_src, onboard_spk->sample_rate[dma_src], onboard_spk->chl_num, onboard_spk->bits);
        BK_LOGD(TAG, "new samp_rate: %d, chl_num: %d, bits: %d \n", rate, ch, bits);
        BK_LOGD(TAG, "not need update onboard speaker \n");
        return BK_OK;
    }

    if (state == AEL_STATE_RUNNING)
    {
        /* set read data timeout */
        audio_element_set_input_timeout(onboard_speaker_stream, 0);
        if (BK_OK != audio_element_pause(onboard_speaker_stream))
        {
            BK_LOGE(TAG, "%s, line: %d, audio_element_pause fail \n", __func__, __LINE__);
        }
    }

    if (BK_OK == audio_dac_reconfig(onboard_spk, rate, ch, bits, dma_src))
    {
        onboard_spk->sample_rate[dma_src] = rate;
        onboard_spk->chl_num = ch;
        onboard_spk->bits = bits;
        audio_element_setdata(onboard_speaker_stream, onboard_spk);
    }
    else
    {
        BK_LOGE(TAG, "%s, line: %d, updata onboard speaker config fail \n", __func__, __LINE__);
        err = BK_FAIL;
    }

    audio_element_set_music_info(onboard_speaker_stream, rate, ch, bits);

    if (state == AEL_STATE_RUNNING)
    {
        audio_element_resume(onboard_speaker_stream, 0, 0);
        /* set read data timeout */
        audio_element_set_input_timeout(onboard_speaker_stream, 2000);
    }

    return err;
}

bk_err_t onboard_speaker_stream_set_digital_gain(audio_element_handle_t onboard_speaker_stream, float gain_db)
{
    onboard_speaker_stream_t *onboard_spk = (onboard_speaker_stream_t *)audio_element_getdata(onboard_speaker_stream);

    if (onboard_spk == NULL)
    {
        BK_LOGE(TAG, "%s, line: %d, onboard_spk is not init \n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (gain_db != gain_db)
    {
        BK_LOGE(TAG, "%s: gain_db is NaN\n", __func__);
        return BK_FAIL;
    }

    rtos_lock_mutex(&onboard_spk->cfg_lock);
    bk_err_t err = bk_aud_dac_set_dig_gain_db(gain_db);
    if (err != BK_OK)
    {
        BK_LOGE(TAG, "%s, line: %d, updata speaker digital gain fail \n", __func__, __LINE__);
        rtos_unlock_mutex(&onboard_spk->cfg_lock);
        return err;
    }

    /*
     * Compare against the silence threshold, not against 0 dB: 0 dB is a
     * normal audible gain, while gains <= BK_AUD_DAC_DIG_GAIN_DB_SILENCE
     * mean the caller wants true silence (e.g. the lowest volume level).
     * Digital gain alone leaves a residual DAC noise floor, so hard-mute
     * the DAC for the silence case and unmute otherwise.
     */
    if (gain_db <= BK_AUD_DAC_DIG_GAIN_DB_SILENCE)
    {
        //pa_ctrl_en(onboard_spk, false, false);
        bk_aud_dac_mute();
        BK_LOGV(TAG, "%s, line: %d, audio dac mute\n", __func__, __LINE__);
    }
    else
    {
        //pa_ctrl_en(onboard_spk, true, false);
        bk_aud_dac_unmute();
        BK_LOGV(TAG, "%s, line: %d, audio dac unmute\n", __func__, __LINE__);
    }

    float res = 0.0f;
    err = bk_aud_dac_get_dig_gain_db(&res);
    if (err != BK_OK)
    {
        BK_LOGE(TAG, "%s, line: %d, get dig gain fail \n", __func__, __LINE__);
        rtos_unlock_mutex(&onboard_spk->cfg_lock);
        return err;
    }
    BK_LOGD(TAG, "%s, line: %d, get dig gain_db: %.2f \n", __func__, __LINE__, res);
    onboard_spk->dig_gain = res;
    audio_element_setdata(onboard_speaker_stream, onboard_spk);

    rtos_unlock_mutex(&onboard_spk->cfg_lock);
    return BK_OK;
}

bk_err_t onboard_speaker_stream_get_digital_gain(audio_element_handle_t onboard_speaker_stream, float *gain_db)
{
    onboard_speaker_stream_t *onboard_spk = (onboard_speaker_stream_t *)audio_element_getdata(onboard_speaker_stream);

    if (gain_db == NULL)
    {
        BK_LOGE(TAG, "%s, line: %d, gain_db is NULL\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (onboard_spk == NULL)
    {
        BK_LOGE(TAG, "%s, line: %d, onboard_spk is not init \n", __func__, __LINE__);
        return BK_FAIL;
    }

    rtos_lock_mutex(&onboard_spk->cfg_lock);
    bk_err_t err = bk_aud_dac_get_dig_gain_db(gain_db);
    if (err != BK_OK)
    {
        BK_LOGE(TAG, "%s, line: %d, get dig gain fail \n", __func__, __LINE__);
        rtos_unlock_mutex(&onboard_spk->cfg_lock);
        return err;
    }

    rtos_unlock_mutex(&onboard_spk->cfg_lock);
    return BK_OK;
}


bk_err_t onboard_speaker_stream_dac_mute_en(audio_element_handle_t onboard_speaker_stream, uint8_t value)
{
    onboard_speaker_stream_t *onboard_spk = (onboard_speaker_stream_t *)audio_element_getdata(onboard_speaker_stream);

    /* check param */
    if (onboard_spk == NULL)
    {
        BK_LOGE(TAG, "%s, line: %d, onboard_spk is not init \n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (value == 0)
    {
        bk_aud_dac_unmute();
        BK_LOGV(TAG, "%s, line: %d, audio dac unmute\n", __func__, __LINE__);
    }
    else
    {
        bk_aud_dac_mute();
        BK_LOGV(TAG, "%s, line: %d, audio dac mute\n", __func__, __LINE__);
    }

    return BK_OK;
}

bk_err_t onboard_speaker_stream_set_analog_gain(audio_element_handle_t onboard_speaker_stream, int32_t gain_db)
{
    onboard_speaker_stream_t *onboard_spk = (onboard_speaker_stream_t *)audio_element_getdata(onboard_speaker_stream);

    /* check param */
    if (onboard_spk == NULL)
    {
        BK_LOGE(TAG, "%s, line: %d, onboard_spk is not init \n", __func__, __LINE__);
        return BK_FAIL;
    }

    rtos_lock_mutex(&onboard_spk->cfg_lock);
    if (onboard_spk->ana_gain == gain_db)
    {
        BK_LOGD(TAG, "not need update onboard spk analog gain \n");
        rtos_unlock_mutex(&onboard_spk->cfg_lock);
        return BK_OK;
    }

    if (BK_OK == bk_aud_dac_set_ana_gain_db(gain_db))
    {
        onboard_spk->ana_gain = gain_db;
        audio_element_setdata(onboard_speaker_stream, onboard_spk);
    } else
    {
        BK_LOGE(TAG, "%s, line: %d, update spk analog gain fail \n", __func__, __LINE__);
        rtos_unlock_mutex(&onboard_spk->cfg_lock);
        return BK_FAIL;
    }
    rtos_unlock_mutex(&onboard_spk->cfg_lock);
    return BK_OK;
}

bk_err_t onboard_speaker_stream_get_analog_gain(audio_element_handle_t onboard_speaker_stream, int32_t *gain_db)
{
    onboard_speaker_stream_t *onboard_spk = (onboard_speaker_stream_t *)audio_element_getdata(onboard_speaker_stream);
    /* check param */
    if (gain_db == NULL)
    {
        BK_LOGE(TAG, "%s, line: %d, gain_db is NULL\n", __func__, __LINE__);
        return BK_FAIL;
    }
    /* check param */
    if (onboard_spk == NULL)
    {
        BK_LOGE(TAG, "%s, line: %d, onboard_spk is not init \n", __func__, __LINE__);
        return BK_FAIL;
    }

    rtos_lock_mutex(&onboard_spk->cfg_lock);
    *gain_db = onboard_spk->ana_gain;
    rtos_unlock_mutex(&onboard_spk->cfg_lock);
    return BK_OK;
}

bk_err_t onboard_speaker_stream_get_status(audio_element_handle_t onboard_speaker_stream, onboard_speaker_stream_status_t *status)
{
    onboard_speaker_stream_t *onboard_spk = (onboard_speaker_stream_t *)audio_element_getdata(onboard_speaker_stream);

    if (status == NULL)
    {
        BK_LOGE(TAG, "%s, line: %d, status is NULL\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (onboard_spk == NULL)
    {
        BK_LOGE(TAG, "%s, line: %d, onboard_spk is not init \n", __func__, __LINE__);
        return BK_FAIL;
    }

    *status = onboard_spk->status;
    return BK_OK;
}

#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
bk_err_t onboard_speaker_stream_get_input_port_info_by_port_id(audio_element_handle_t onboard_speaker_stream, uint8_t port_id, audio_port_info_t **port_info)
{
    onboard_speaker_stream_t *onboard_spk = (onboard_speaker_stream_t *)audio_element_getdata(onboard_speaker_stream);

    /* check param */
    if (onboard_spk == NULL)
    {
        BK_LOGE(TAG, "%s, line: %d, onboard_spk is not init \n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (port_info == NULL)
    {
        BK_LOGE(TAG, "%s, line: %d, port_info is NULL \n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (port_id > (audio_element_get_multi_input_max_port_num(onboard_speaker_stream) + 1))
    {
        BK_LOGE(TAG, "%s, line: %d, audio port id: %d out of range: 0 ~ %d \n", __func__, __LINE__, port_id, audio_element_get_multi_input_max_port_num(onboard_speaker_stream) + 1);
        return BK_FAIL;
    }

    *port_info = audio_port_info_list_get_by_port_id(&onboard_spk->input_port_list, port_id);
    return BK_OK;
}

bk_err_t onboard_speaker_stream_set_input_port_info(audio_element_handle_t onboard_speaker_stream, audio_port_info_t *port_info)
{
    onboard_speaker_stream_t *onboard_spk = (onboard_speaker_stream_t *)audio_element_getdata(onboard_speaker_stream);

    INPUT_PORT_LIST_DEBUG(&onboard_spk->input_port_list, __func__, __LINE__);

    /* check param */
    if (onboard_spk == NULL || port_info == NULL)
    {
        BK_LOGE(TAG, "%s, line: %d, onboard_spk is not init or port_info: %p is NULL \n", __func__, __LINE__, port_info);
        return BK_FAIL;
    }

    if (port_info->port_id > (audio_element_get_multi_input_max_port_num(onboard_speaker_stream) + 1))
    {
        BK_LOGE(TAG, "%s, line: %d, audio port id: %d out of range: 0 ~ %d \n", __func__, __LINE__, port_info->port_id, audio_element_get_multi_input_max_port_num(onboard_speaker_stream) + 1);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "%s, line: %d, port_id: %d, priority: %d, port: %p \n", __func__, __LINE__, port_info->port_id, port_info->priority, port_info->port);

#if 0
    /* Only one input port is allowed under one priority level */
    input_audio_port_info_item_t *audio_port_info_item = NULL;
    STAILQ_FOREACH(audio_port_info_item, &onboard_spk->input_port_list, next)
    {
        if (audio_port_info_item && audio_port_info_item->port_info.priority == port_info->priority && audio_port_info_item->port_info.port_id == port_info->port_id && port_info->port != NULL)
        {
            BK_LOGE(TAG, "%s, line: %d, audio port: %d, priority: %d is exist, different ports are not allowed to use the same priority\n", __func__, __LINE__, port_info->port_id, port_info->priority);
            return BK_FAIL;
        }
    }
#endif

    /* Check all ports in the input port list to see if there is a port with the same port ID as port_info, and update the port information */
    audio_port_info_t *tmp_port_info = audio_port_info_list_get_by_port_id(&onboard_spk->input_port_list, port_info->port_id);
    if (tmp_port_info)
    {
        /* check whether port_info is same as tmp_port_info */
        if (memcmp(tmp_port_info, port_info, sizeof(audio_port_info_t)) == 0)
        {
            BK_LOGD(TAG, "%s, line: %d, port_info is same as tmp_port_info, not update\n", __func__, __LINE__);
            return BK_OK;
        }

        /* update audio port info in port list */
        input_port_list_block(onboard_spk->lock, portMAX_DELAY);
        audio_port_info_list_update(&onboard_spk->input_port_list, port_info);
        input_port_list_release(onboard_spk->lock);
        /* check and update audio port */
        if (port_info->port_id == 0)
        {
            if (audio_element_get_input_port(onboard_speaker_stream) != port_info->port)
            {
                audio_element_set_input_port(onboard_speaker_stream, port_info->port);
            }
        }
        else
        {
            if (audio_element_get_multi_input_port(onboard_speaker_stream, port_info->port_id - 1) != port_info->port)
            {
                audio_element_set_multi_input_port(onboard_speaker_stream, port_info->port, (int)(port_info->port_id - 1));
            }
        }
    }
    else
    {
        /* if port_info->port is NULL, not add to list */
        if (port_info->port != NULL)
        {
            /* add new port */
            input_port_list_block(onboard_spk->lock, portMAX_DELAY);
            audio_port_info_list_add(&onboard_spk->input_port_list, port_info);
            input_port_list_release(onboard_spk->lock);
        }
        /* check and update audio port */
        if (port_info->port_id == 0)
        {
            audio_element_set_input_port(onboard_speaker_stream, port_info->port);
        }
        else
        {
            audio_element_set_multi_input_port(onboard_speaker_stream, port_info->port, (int)(port_info->port_id - 1));
        }
    }

    INPUT_PORT_LIST_DEBUG(&onboard_spk->input_port_list, __func__, __LINE__);
    return BK_OK;
}
#endif
