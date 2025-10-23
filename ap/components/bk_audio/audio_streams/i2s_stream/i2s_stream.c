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
#include <components/bk_audio/audio_streams/i2s_stream.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <driver/i2s.h>
#include <driver/i2s_types.h>
#include <driver/audio_ring_buff.h>


#define TAG  "I2S_STR"


typedef struct i2s_stream
{
    i2s_gpio_group_id_t     gpio_group;       /**< I2S gpio group id */
    i2s_config_t            i2s_cfg;          /**< I2S configuration */
    i2s_channel_id_t        channel_id;       /**< I2S channel id */
    audio_stream_type_t     type;             /**< Type of stream */
    uint32_t                buff_size;        /**< Ring buffer size */
    RingBufferContext       *rb;              /**< Ring buffer context */
    int                     out_block_size;   /**< Size of output block */
    int                     out_block_num;    /**< Number of output block */
    bool                    is_open;          /**< i2s enable, true: enable, false: disable */
    beken_semaphore_t       can_process;      /**< can process */
} i2s_stream_t;


static i2s_stream_t *gl_i2s_stream = NULL;


static int i2s_data_handle_callback(uint32_t size)
{
    //BK_LOGD(TAG, "[%s] size: %d \n", __func__, size);
    bk_err_t ret = rtos_set_semaphore(&gl_i2s_stream->can_process);
    if (ret != BK_OK)
    {
        BK_LOGV(TAG, "%s, rtos_set_semaphore fail \n", __func__);
    }

    return size;
}


static int _i2s_open(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] _i2s_open \n", audio_element_get_tag(self));

    i2s_stream_t *i2s_stream = (i2s_stream_t *)audio_element_getdata(self);

    if (i2s_stream->is_open)
    {
        BK_LOGD(TAG, "[%s] i2s already opened \n", audio_element_get_tag(self));
        return BK_OK;
    }

    /* set read/write data timeout */
    if (i2s_stream->type == AUDIO_STREAM_READER)
    {
        audio_element_set_output_timeout(self, 0);
    }
    else if (i2s_stream->type == AUDIO_STREAM_WRITER)
    {
        audio_element_set_input_timeout(self, 0);
    }

    /* init semaphore */
    bk_err_t ret = rtos_init_semaphore(&i2s_stream->can_process, 1);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, rtos_init_semaphore fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    /* init i2s driver */
    if (BK_OK != bk_i2s_driver_init())
    {
        BK_LOGE(TAG, "[%s] %s, %d, init i2s driver fail \n", audio_element_get_tag(self), __func__, __LINE__);
        rtos_deinit_semaphore(&i2s_stream->can_process);
        return BK_FAIL;
    }

    /* init i2s configure */
    if (BK_OK != bk_i2s_init(i2s_stream->gpio_group, &i2s_stream->i2s_cfg))
    {
        BK_LOGE(TAG, "[%s] %s, %d, init i2s config fail \n", audio_element_get_tag(self), __func__, __LINE__);
        bk_i2s_driver_deinit();
        rtos_deinit_semaphore(&i2s_stream->can_process);
        return BK_FAIL;
    }

    /* init i2s channel */
    i2s_txrx_type_t txrx_type = (i2s_stream->type == AUDIO_STREAM_READER) ? I2S_TXRX_TYPE_RX : I2S_TXRX_TYPE_TX;

    gl_i2s_stream = i2s_stream;

    if (BK_OK != bk_i2s_chl_init(i2s_stream->channel_id, txrx_type, i2s_stream->buff_size * 2, i2s_data_handle_callback, &i2s_stream->rb))
    {
        BK_LOGE(TAG, "[%s] %s, %d, init i2s channel fail \n", audio_element_get_tag(self), __func__, __LINE__);
        bk_i2s_deinit();
        bk_i2s_driver_deinit();
        rtos_deinit_semaphore(&i2s_stream->can_process);
        gl_i2s_stream = NULL;
        return BK_FAIL;
    }

    /* write initial data for TX mode */
    if (i2s_stream->type == AUDIO_STREAM_WRITER)
    {
        uint8_t *temp_data = (uint8_t *)audio_malloc(i2s_stream->buff_size * 2);
        if (temp_data)
        {
            os_memset(temp_data, 0x00, i2s_stream->buff_size * 2);
            ring_buffer_write(i2s_stream->rb, temp_data, i2s_stream->buff_size * 2);
            audio_free(temp_data);
        }
    }

    /* start i2s */
    if (BK_OK != bk_i2s_start())
    {
        BK_LOGE(TAG, "[%s] %s, %d, start i2s fail \n", audio_element_get_tag(self), __func__, __LINE__);
        bk_i2s_chl_deinit(i2s_stream->channel_id, txrx_type);
        bk_i2s_deinit();
        bk_i2s_driver_deinit();
        rtos_deinit_semaphore(&i2s_stream->can_process);
        gl_i2s_stream = NULL;
        return BK_FAIL;
    }

    i2s_stream->is_open = true;

    BK_LOGD(TAG, "[%s] i2s open successful \n", audio_element_get_tag(self));

    return BK_OK;
}


static int _i2s_read(audio_port_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    audio_element_handle_t el = (audio_element_handle_t)context;
    i2s_stream_t *i2s_stream = (i2s_stream_t *)audio_element_getdata(el);

    BK_LOGV(TAG, "[%s] _i2s_read, len: %d \n", audio_element_get_tag(el), len);

    if (i2s_stream->rb && len > 0)
    {
        int read_len = ring_buffer_read(i2s_stream->rb, (uint8_t *)buffer, len);
        return read_len;
    }

    return 0;
}


static int _i2s_write(audio_port_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    audio_element_handle_t el = (audio_element_handle_t)context;
    i2s_stream_t *i2s_stream = (i2s_stream_t *)audio_element_getdata(el);

    BK_LOGV(TAG, "[%s] _i2s_write, len: %d \n", audio_element_get_tag(el), len);

    if (i2s_stream->rb && len > 0)
    {
        int write_len = ring_buffer_write(i2s_stream->rb, (uint8_t *)buffer, len);
        return write_len;
    }

    return 0;
}


static int _i2s_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    i2s_stream_t *i2s_stream = (i2s_stream_t *)audio_element_getdata(self);

    BK_LOGV(TAG, "[%s] _i2s_process, in_len: %d \n", audio_element_get_tag(self), in_len);

    /* wait for semaphore */
    if (BK_OK != rtos_get_semaphore(&i2s_stream->can_process, 20000000 / portTICK_RATE_MS))
    {
        BK_LOGE(TAG, "[%s] semaphore get timeout 2000ms\n", audio_element_get_tag(self));
        return 0;
    }

    /* read input data */
    int r_size = audio_element_input(self, in_buffer, in_len);
    int w_size = 0;

    if (r_size == AEL_IO_TIMEOUT)
    {
        if (i2s_stream->type == AUDIO_STREAM_WRITER)
        {
            os_memset(in_buffer, 0x00, in_len);
            w_size = audio_element_output(self, in_buffer, in_len);
        }
        else
        {
            w_size = audio_element_output(self, in_buffer, r_size);
        }
    }
    else if (r_size > 0)
    {
        w_size = audio_element_output(self, in_buffer, r_size);
    }
    else
    {
        w_size = r_size;
    }

    return w_size;
}


static int _i2s_close(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] _i2s_close \n", audio_element_get_tag(self));

    i2s_stream_t *i2s_stream = (i2s_stream_t *)audio_element_getdata(self);

    if (!i2s_stream->is_open)
    {
        BK_LOGD(TAG, "[%s] i2s already closed \n", audio_element_get_tag(self));
        return BK_OK;
    }

    /* stop i2s */
    bk_i2s_stop();

    /* deinit i2s channel */
    i2s_txrx_type_t txrx_type = (i2s_stream->type == AUDIO_STREAM_READER) ? I2S_TXRX_TYPE_RX : I2S_TXRX_TYPE_TX;
    bk_i2s_chl_deinit(i2s_stream->channel_id, txrx_type);

    /* deinit i2s */
    bk_i2s_deinit();

    /* deinit i2s driver */
    bk_i2s_driver_deinit();

    /* deinit semaphore */
    if (i2s_stream->can_process)
    {
        rtos_deinit_semaphore(&i2s_stream->can_process);
        i2s_stream->can_process = NULL;
    }

    i2s_stream->is_open = false;
    gl_i2s_stream = NULL;

    BK_LOGD(TAG, "[%s] i2s close successful \n", audio_element_get_tag(self));

    return BK_OK;
}


static int _i2s_destroy(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] _i2s_destroy \n", audio_element_get_tag(self));

    i2s_stream_t *i2s_stream = (i2s_stream_t *)audio_element_getdata(self);

    if (i2s_stream)
    {
        audio_free(i2s_stream);
        i2s_stream = NULL;
    }

    return BK_OK;
}


audio_element_handle_t i2s_stream_init(i2s_stream_cfg_t *config)
{
    if (!config)
    {
        BK_LOGE(TAG, "config is NULL\n");
        return NULL;
    }

    audio_element_handle_t el;
    i2s_stream_t *i2s_stream = audio_calloc(1, sizeof(i2s_stream_t));
    AUDIO_MEM_CHECK(TAG, i2s_stream, return NULL);

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _i2s_open;
    cfg.close = _i2s_close;
    cfg.process = _i2s_process;
    cfg.destroy = _i2s_destroy;
    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;
    cfg.buffer_len = config->out_block_size;
    cfg.out_block_size = config->out_block_size;
    cfg.out_block_num = config->out_block_num;

    if (config->type == AUDIO_STREAM_READER)
    {
        cfg.in_type = PORT_TYPE_CB;
        cfg.read = _i2s_read;
        cfg.out_type = PORT_TYPE_RB;
        cfg.write = NULL;
    }
    else if (config->type == AUDIO_STREAM_WRITER)
    {
        cfg.in_type = PORT_TYPE_RB;
        cfg.read = NULL;
        cfg.out_type = PORT_TYPE_CB;
        cfg.write = _i2s_write;
    }
    else
    {
        BK_LOGE(TAG, "i2s type: %d, is not support, please check\n", config->type);
        goto _i2s_init_exit;
    }

    cfg.tag = "i2s_stream";
    BK_LOGD(TAG, "buffer_len: %d, out_block_size: %d, out_block_num: %d\n", cfg.buffer_len, cfg.out_block_size, cfg.out_block_num);

    /* config i2s */
    i2s_stream->gpio_group = config->gpio_group;
    i2s_stream->i2s_cfg.role = config->role;
    i2s_stream->i2s_cfg.work_mode = config->work_mode;
    i2s_stream->i2s_cfg.lrck_invert = config->lrck_invert;
    i2s_stream->i2s_cfg.sck_invert = config->sck_invert;
    i2s_stream->i2s_cfg.lsb_first_en = config->lsb_first_en;
    i2s_stream->i2s_cfg.sync_length = config->sync_length;
    i2s_stream->i2s_cfg.data_length = config->data_length;
    i2s_stream->i2s_cfg.pcm_dlength = config->pcm_dlength;
    i2s_stream->i2s_cfg.store_mode = config->store_mode;
    i2s_stream->i2s_cfg.samp_rate = config->samp_rate;
    i2s_stream->i2s_cfg.pcm_chl_num = config->pcm_chl_num;
    i2s_stream->channel_id = config->channel_id;
    i2s_stream->type = config->type;
    i2s_stream->buff_size = config->buff_size;
    i2s_stream->out_block_size = config->out_block_size;
    i2s_stream->out_block_num = config->out_block_num;
    i2s_stream->is_open = false;
    i2s_stream->can_process = NULL;

    BK_LOGD(TAG, "gpio_group: %d, role: %d, work_mode: %d, samp_rate: %d, channel_id: %d, type: %d\n",
            i2s_stream->gpio_group, i2s_stream->i2s_cfg.role, i2s_stream->i2s_cfg.work_mode,
            i2s_stream->i2s_cfg.samp_rate, i2s_stream->channel_id, i2s_stream->type);

    el = audio_element_init(&cfg);
    AUDIO_MEM_CHECK(TAG, el, goto _i2s_init_exit);
    audio_element_setdata(el, i2s_stream);

    audio_element_info_t info = {0};
    info.sample_rates = 16000;
    info.channels = 2;
    info.bits = 16;
    info.codec_fmt = BK_CODEC_TYPE_PCM;
    audio_element_setinfo(el, &info);

    return el;

_i2s_init_exit:
    audio_free(i2s_stream);
    i2s_stream = NULL;
    return NULL;
}

