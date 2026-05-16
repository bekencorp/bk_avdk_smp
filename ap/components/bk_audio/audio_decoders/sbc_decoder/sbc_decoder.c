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
#include "FreeRTOS.h"
#include "task.h"
#include <components/bk_audio/audio_decoders/sbc_dec.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <os/os.h>
#include <os/mem.h>
#include <modules/sbc_decoder.h>
#include <components/bk_audio/audio_utils/debug_dump_util.h>

#define TAG  "SBC_DEC"

//#define SBC_DECODER_DEBUG   //GPIO debug

#ifdef SBC_DECODER_DEBUG

#define SBC_DECODER_PROCESS_START()         do { GPIO_DOWN(33); GPIO_UP(33);} while (0)
#define SBC_DECODER_PROCESS_END()           do { GPIO_DOWN(33); } while (0)

#define SBC_DECODER_INPUT_START()           do { GPIO_DOWN(34); GPIO_UP(34);} while (0)
#define SBC_DECODER_INPUT_END()             do { GPIO_DOWN(34); } while (0)

#define SBC_DECODER_OUTPUT_START()          do { GPIO_DOWN(35); GPIO_UP(35);} while (0)
#define SBC_DECODER_OUTPUT_END()            do { GPIO_DOWN(35); } while (0)

#else

#define SBC_DECODER_PROCESS_START()
#define SBC_DECODER_PROCESS_END()

#define SBC_DECODER_INPUT_START()
#define SBC_DECODER_INPUT_END()

#define SBC_DECODER_OUTPUT_START()
#define SBC_DECODER_OUTPUT_END()

#endif

/* dump sbc_decoder stream output pcm data by uart */
//#define SBC_DEC_DATA_DUMP_BY_UART

#ifdef SBC_DEC_DATA_DUMP_BY_UART
#include <components/bk_audio/audio_utils/uart_util.h>
static struct uart_util gl_sbc_dec_uart_util = {0};
#define SBC_DEC_DATA_DUMP_UART_ID            (1)
#define SBC_DEC_DATA_DUMP_UART_BAUD_RATE     (2000000)

#define SBC_DEC_DATA_DUMP_BY_UART_OPEN()                    uart_util_create(&gl_sbc_dec_uart_util, SBC_DEC_DATA_DUMP_UART_ID, SBC_DEC_DATA_DUMP_UART_BAUD_RATE)
#define SBC_DEC_DATA_DUMP_BY_UART_CLOSE()                   uart_util_destroy(&gl_sbc_dec_uart_util)
#define SBC_DEC_DATA_DUMP_BY_UART_DATA(data_buf, len)       uart_util_tx_data(&gl_sbc_dec_uart_util, data_buf, len)

#else

#define SBC_DEC_DATA_DUMP_BY_UART_OPEN()
#define SBC_DEC_DATA_DUMP_BY_UART_CLOSE()
#define SBC_DEC_DATA_DUMP_BY_UART_DATA(data_buf, len)

#endif  //SBC_ENC_DATA_DUMP_BY_UART

/**
 * @brief SBC decoder context structure
 */
typedef struct {
    sbc_t             sbc;
    struct sbc_frame  frame;
    uint32_t          sample_rate;
    uint8_t           channel_number;
    uint8_t           pending[2048];
    uint32_t          pending_len;
    int16_t           pcml[SBC_MAX_SAMPLES];
    int16_t           pcmr[SBC_MAX_SAMPLES];
    int16_t           pcm_out[2 * SBC_MAX_SAMPLES];
} sbc_decoder_t;

/**
 * @brief SBC decoder frame info report function
 *        Check whether the current frame information is consistent with the last reported information,
 *        and update and notify the upper layer if it is inconsistent
 */
static bk_err_t music_info_report(audio_element_handle_t self)
{
    sbc_decoder_t *sbc_dec = (sbc_decoder_t *)audio_element_getdata(self);
    int sample_rate    = sbc_get_freq_hz(sbc_dec->frame.freq);
    int channel_number = (sbc_dec->frame.mode == SBC_MODE_MONO) ? 1 : 2;

    if (sample_rate <= 0) {
        BK_LOGE(TAG, "[%s] invalid sample_rate from frame freq:%d", audio_element_get_tag(self), sbc_dec->frame.freq);
        return BK_FAIL;
    }

    /* Check frame information and report only when it changes. */
    if (sbc_dec->sample_rate != (uint32_t)sample_rate || sbc_dec->channel_number != (uint8_t)channel_number) {
        BK_LOGD(TAG, "[%s] new sbc frame info sample_rate:%d, channel_number:%d, bits:16",
                audio_element_get_tag(self), sample_rate, channel_number);

        audio_element_info_t info = {0};
        bk_err_t ret = audio_element_getinfo(self, &info);
        if (ret != BK_OK) {
            BK_LOGE(TAG, "[%s] audio_element_getinfo fail", audio_element_get_tag(self));
            return BK_FAIL;
        }

        /* SBC decoder output is fixed at 16 bits PCM. */
        info.bits = 16;
        info.sample_rates = sample_rate;
        info.channels     = channel_number;
        ret = audio_element_setinfo(self, &info);
        if (ret != BK_OK) {
            BK_LOGE(TAG, "[%s] audio_element_setinfo fail", audio_element_get_tag(self));
            return BK_FAIL;
        }

        ret = audio_element_report_info(self);
        if (ret != BK_OK) {
            BK_LOGE(TAG, "[%s] audio_element_report_info fail", audio_element_get_tag(self));
            return BK_FAIL;
        }

        sbc_dec->sample_rate    = (uint32_t)sample_rate;
        sbc_dec->channel_number = (uint8_t)channel_number;
    }

    return BK_OK;
}

static bk_err_t _sbc_decoder_open(audio_element_handle_t self)
{
    sbc_decoder_t *dec = (sbc_decoder_t *)audio_element_getdata(self);

    sbc_reset(&dec->sbc);
    dec->sample_rate    = 0;
    dec->channel_number = 0;
    dec->pending_len    = 0;

    audio_element_set_input_timeout(self, 20 / portTICK_RATE_MS);
    BK_LOGD(TAG, "[%s] opened (sw-sbc)\n", audio_element_get_tag(self));

    return BK_OK;
}

static int _sbc_decoder_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    sbc_decoder_t *dec = (sbc_decoder_t *)audio_element_getdata(self);
    int r_size = audio_element_input(self, in_buffer, in_len);
    int w_size = 0;

    if (r_size <= 0) {
        return r_size;
    }

    if ((dec->pending_len + (uint32_t)r_size) > sizeof(dec->pending)) {
        dec->pending_len = 0;
    }

    os_memcpy(dec->pending + dec->pending_len, in_buffer, r_size);
    dec->pending_len += (uint32_t)r_size;

    uint32_t offset = 0;
    while (offset < dec->pending_len) {
        uint8_t *d = dec->pending + offset;
        uint32_t remain = dec->pending_len - offset;

        if (d[0] != 0x9C && d[0] != 0xAD) {
            offset++;
            continue;
        }
        if (remain < SBC_HEADER_SIZE) {
            break;
        }
        if (sbc_probe(d, &dec->frame) < 0) {
            offset++;
            continue;
        }

        uint32_t fsize = sbc_get_frame_size(&dec->frame);
        if (fsize == 0 || remain < fsize) {
            break;
        }

        int ret = sbc_decode(&dec->sbc, d, fsize, &dec->frame, dec->pcml, 1, dec->pcmr, 1);
        if (ret < 0) {
            BK_LOGW(TAG, "[%s] sbc_decode err, resync", audio_element_get_tag(self));
            offset++;
            continue;
        }
        offset += fsize;

        int nch     = (dec->frame.mode == SBC_MODE_MONO) ? 1 : 2;
        int pcm_len = dec->frame.nblocks * dec->frame.nsubbands;

        if (music_info_report(self) != BK_OK) {
            BK_LOGW(TAG, "[%s] report info failed", audio_element_get_tag(self));
        }
        int out_len = 0;
        int output_size = 0;

        if (nch == 1) {
            output_size = pcm_len * (int)sizeof(int16_t);
            out_len = audio_element_output(self, (char *)dec->pcml, output_size);
            if (out_len > 0) {
                w_size += out_len;
            }
        } else {
            for (int i = 0; i < pcm_len; i++) {
                dec->pcm_out[2 * i]     = dec->pcml[i];
                dec->pcm_out[2 * i + 1] = dec->pcmr[i];
            }
            output_size = pcm_len * 2 * (int)sizeof(int16_t);
            out_len = audio_element_output(self, (char *)dec->pcm_out, output_size);
            if (out_len > 0) {
                w_size += out_len;
            }
        }
        if(is_aud_dump_valid(DUMP_TYPE_DEC_OUT_DATA))
        {
            /*update header*/
            DEBUG_DATA_DUMP_UPDATE_HEADER_DUMP_FILE_TYPE(DUMP_TYPE_DEC_OUT_DATA, 0, DUMP_FILE_TYPE_PCM);
            DEBUG_DATA_DUMP_UPDATE_HEADER_DATA_FLOW_LEN(DUMP_TYPE_DEC_OUT_DATA, 0, out_len);
            DEBUG_DATA_DUMP_UPDATE_HEADER_TIMESTAMP(DUMP_TYPE_DEC_OUT_DATA);

            /*dump data function is called by multi-thread,need suspend task scheduler until data dump finished*/
            DEBUG_DATA_DUMP_SUSPEND_ALL;

            /*dump header*/
            DEBUG_DATA_DUMP_BY_UART_HEADER(DUMP_TYPE_DEC_OUT_DATA);

            /*dump data*/
            DEBUG_DATA_DUMP_BY_UART_DATA(dec->pcm_out, out_len);
            DEBUG_DATA_DUMP_RESUME_ALL;

            /*update seq*/
            DEBUG_DATA_DUMP_UPDATE_HEADER_SEQ_NUM(DUMP_TYPE_DEC_OUT_DATA);
        }
    }

    if (offset > 0 && offset <= dec->pending_len) {
        os_memmove(dec->pending, dec->pending + offset, dec->pending_len - offset);
        dec->pending_len -= offset;
    }
    return (w_size > 0) ? w_size : r_size;
}

static bk_err_t _sbc_decoder_close(audio_element_handle_t self)
{
    BK_LOGI(TAG, "[%s] closed\n", audio_element_get_tag(self));
    return BK_OK;
}

static bk_err_t _sbc_decoder_destroy(audio_element_handle_t self)
{
    sbc_decoder_t *dec = (sbc_decoder_t *)audio_element_getdata(self);
    audio_free(dec);
    return BK_OK;
}

audio_element_handle_t sbc_dec_init(sbc_decoder_cfg_t *config)
{
    audio_element_handle_t el = NULL;
    sbc_decoder_t *sbc_dec = audio_calloc(1, sizeof(sbc_decoder_t));
    AUDIO_MEM_CHECK(TAG, sbc_dec, return NULL);

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open  = _sbc_decoder_open;
    cfg.close = _sbc_decoder_close;
    cfg.seek  = NULL;
    cfg.process = _sbc_decoder_process;
    cfg.destroy = _sbc_decoder_destroy;
    cfg.in_type = PORT_TYPE_RB;
    cfg.read    = NULL;
    cfg.out_type = PORT_TYPE_RB;
    cfg.write    = NULL;
    cfg.task_stack = config->task_stack;
    cfg.task_prio  = config->task_prio;
    cfg.task_core  = config->task_core;
    cfg.out_block_size = config->out_block_size;
    cfg.out_block_num  = config->out_block_num;
    cfg.buffer_len     = config->buf_sz;
    cfg.tag = "sbc_decoder";

    el = audio_element_init(&cfg);
    AUDIO_MEM_CHECK(TAG, el, goto _sbc_decoder_init_exit);
    audio_element_setdata(el, sbc_dec);

    /* Set the initial SBC audio frame information */
    audio_element_info_t info = {0};
    audio_element_getinfo(el, &info);
    info.sample_rates = 0;
    info.channels     = 0;
    info.bits         = 0;
    info.codec_fmt    = BK_CODEC_TYPE_SBC;
    audio_element_setinfo(el, &info);

    return el;

_sbc_decoder_init_exit:
    audio_free(sbc_dec);
    return NULL;
}

audio_element_handle_t sbc_decoder_init(sbc_decoder_cfg_t *config)
{
    return sbc_dec_init(config);
}
