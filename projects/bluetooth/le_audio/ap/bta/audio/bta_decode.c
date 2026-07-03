#include <components/system.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <driver/timer.h>

#include <driver/uart.h>
#include "gpio_driver.h"

#include "bk_ring_buffer_node.h"


#include "bta_audio.h"

#include "bluetooth_config.h"
#include "bta_event.h"

#include "FreeRTOS.h"
#include "event_groups.h"


#include "bta_resample.h"
#include "bta_auracast.h"
#include "bta_decode.h"

#include "bk_list_edge.h"

#include <driver/audio_ring_buff.h>

#include <driver/aud_dac.h>
#include <driver/aud_dac_types.h>
#include <driver/dma.h>
#include "aud_hal.h"
#include "sys_driver.h"
#include <driver/uart.h>


#define TAG "bta_decode"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define DAC_RING_BUFFER_SIZE            (1024 * 20)

#define LE_AUDIO_DECODER_BUFFER_SIZE            (9064)

typedef struct
{
    EventGroupHandle_t event;
    beken_thread_t thread;
    uint8_t running;
    beken_mutex_t encoded_lock;
    LIST_HEADER_T encoded_list;

    bta_decode_config_t cfg;
    uint16_t output_length;
    uint8_t *output_buffer;
    uint8_t *work_buffer;

    union
    {
        lc3_dec_info_t lc3;
    };


} bta_decode_t;

static bta_decode_t bta_decode = {0};

#define REA_EVT_EXIT_IND                            (1 << 0)
#define REA_EVT_DECODE_DATA_IND                     (1 << 8)

#define REA_WAITTING_EVENTS  (      \
                                    REA_EVT_EXIT_IND                \
                                    | REA_EVT_DECODE_DATA_IND     \
                             )

static bk_err_t bta_decode_lc3_decoder(uint8_t *data, uint32_t length, lc3_dec_info_t *lc3, bta_decode_config_t *cfg)
{
    uint32_t decoder_len = lc3->frame_samples * lc3->pcm_sbytes * lc3->channel;

    os_memset(bta_decode.output_buffer, 0x00, length);

    if (length)
    {
        lc3_decoder_proc(lc3, data, bta_decode.output_buffer);
    }
    else
    {

    }

    if (bta_decode.cfg.cb)
    {
        bta_decode.cfg.cb(bta_decode.output_buffer, decoder_len);
    }

    return BK_OK;
}

void bta_decode_decoded_data_handler(uint8_t *data, uint32_t length, bta_decode_config_t *cfg)
{
    if (CODEC_AUDIO_LC3 == cfg->format)
    {
        bta_decode_lc3_decoder(data, length, &bta_decode.lc3, cfg);
    }
    else
    {
        LOGE("%s, unsupported codec %d \r\n", __func__, cfg->format);
    }
}

static void bta_decode_encoded_list_clear(void)
{
    bta_encoded_data_t *tmp = NULL;

    do
    {
        tmp = list_pop_edge(&bta_decode.encoded_list, bta_encoded_data_t, list);

        if (tmp)
        {
            os_free(tmp);
        }
    }
    while (tmp);

    INIT_LIST_HEAD(&bta_decode.encoded_list);
}

static void bta_decode_task(void *arg)
{
    xEventGroupClearBits(bta_decode.event, REA_EVT_EXIT_IND);

    LOGI("%s init success!! \r\n", __func__);

    while (bta_decode.running)
    {

        EventBits_t bits = xEventGroupWaitBits(bta_decode.event, REA_WAITTING_EVENTS, true, false, 300 / bk_get_ms_per_tick());

        if (bits & (REA_EVT_DECODE_DATA_IND))
        {
            bta_encoded_data_t *encoded_data = NULL;

            do
            {
                rtos_lock_mutex(&bta_decode.encoded_lock);
                encoded_data = list_pop_edge(&bta_decode.encoded_list, bta_encoded_data_t, list);
                rtos_unlock_mutex(&bta_decode.encoded_lock);

                if (encoded_data == NULL)
                {
                    break;
                }

                if (encoded_data->length)
                {
                    bta_decode_decoded_data_handler(encoded_data->payload,
                                                    encoded_data->length, &bta_decode.cfg);
                }

                os_free(encoded_data);
            }
            while (bta_decode.running);
        }

    }

    LOGI("%s exit start!! \r\n", __func__);


    bta_decode_encoded_list_clear();
    rtos_deinit_mutex(&bta_decode.encoded_lock);

    if (bta_decode.output_buffer)
    {
        os_free(bta_decode.output_buffer);
        bta_decode.output_buffer = NULL;
    }

    if (bta_decode.work_buffer)
    {
        os_free(bta_decode.work_buffer);
        bta_decode.work_buffer = NULL;
    }

    LOGI("%s end!!\r\n", __func__);

    bta_decode.thread = NULL;

    if (bta_decode.running)
    {
        bta_decode.running = 0;
        LOGE("!! speaker tash exit error !!\n");
    }

    xEventGroupSetBits(bta_decode.event, REA_EVT_EXIT_IND);

    rtos_delete_thread(NULL);

}

static void bta_decode_config(bta_decode_config_t *cfg)
{
    os_memcpy(&bta_decode.cfg, cfg, sizeof(bta_decode_config_t));

    switch (bta_decode.cfg.format)
    {
        case CODEC_AUDIO_LC3:
        {
            bta_decode.lc3.channel = cfg->channels;
            bta_decode.lc3.frame_us = cfg->duration;
            bta_decode.lc3.frame_bytes = cfg->frame_length;
            bta_decode.lc3.dec_srate_hz = cfg->sample_rate;
            bta_decode.lc3.pcm_sbytes = 2;
            bta_decode.lc3.frame_cnt = 1;

            bta_decode.lc3.frame_samples = cfg->sample_rate
                                           * cfg->channels / (1000) * (cfg->duration / 1000);
            bta_decode.lc3.bitrate = (cfg->frame_length /* frame bytes */ * 8 * 1000000UL) / 10000;

            bta_decode.work_buffer = os_malloc(LE_AUDIO_DECODER_BUFFER_SIZE);

            if (bta_decode.work_buffer == NULL)
            {
                LOGE("malloc work_buffer fail \n");
                return;
            }

            lc3_decoder_init(&bta_decode.lc3, (uint8_t *)bta_decode.work_buffer);
            bta_decode.output_length = bta_decode.lc3.frame_samples * bta_decode.lc3.pcm_sbytes * bta_decode.lc3.channel;
            bta_decode.output_buffer = os_malloc(bta_decode.output_length);

            if (bta_decode.output_buffer == NULL)
            {
                LOGE("malloc output_buffer fail \n");
                return;
            }
        }
        break;

        default:
            LOGE("%s, unsupported codec %d\n", __func__, bta_decode.cfg.format);
            break;

    }
}

int bta_decode_start(bta_decode_config_t *cfg)
{
    bk_err_t ret = BK_OK;

    if (!bta_decode.thread)
    {
        bta_decode.event = xEventGroupCreate();

        if (bta_decode.event == NULL)
        {
            LOGE("bta_decode.event null!\n");
        }

        rtos_init_mutex(&bta_decode.encoded_lock);

        INIT_LIST_HEAD(&bta_decode.encoded_list);

        bta_decode_config(cfg);

        bta_decode.running = true;

#if SMP_THREAD_USED
        ret = rtos_core1_create_thread(&bta_decode.thread,
                                       BEKEN_DEFAULT_WORKER_PRIORITY - 2,
                                       "bta_decode",
                                       (beken_thread_function_t)bta_decode_task,
                                       4096,
                                       (beken_thread_arg_t)0);
#else
        ret = rtos_create_thread(&bta_decode.thread,
                                 BEKEN_DEFAULT_WORKER_PRIORITY - 2,
                                 "bta_decode",
                                 (beken_thread_function_t)bta_decode_task,
                                 4096,
                                 (beken_thread_arg_t)0);
#endif
        if (ret != BK_OK)
        {
            LOGE("speaker task fail \r\n");
        }

        return BK_OK;
    }
    else
    {
        return BK_FAIL;
    }
}

int bta_decode_stop(void)
{
    LOGI("%s +++\n", __func__);

    bta_decode.running = false;

    if(bta_decode.event)
    {
        xEventGroupWaitBits(bta_decode.event, REA_EVT_EXIT_IND, true, false, BEKEN_WAIT_FOREVER);
        vEventGroupDelete(bta_decode.event);
        bta_decode.event = NULL;
    }

    if (bta_decode.cfg.format == CODEC_AUDIO_LC3)
    {
        if (bta_decode.work_buffer)
        {
            os_free(bta_decode.work_buffer);
            bta_decode.work_buffer = NULL;
        }

        if (bta_decode.output_buffer)
        {
            os_free(bta_decode.output_buffer);
            bta_decode.output_buffer = NULL;
        }
    }

    LOGI("%s ---\n", __func__);

    return 0;
}

int bta_decode_write(uint8_t *data, uint16_t length)
{
    if (bta_decode.running == false)
    {
        LOGE("%s task not ready\n", __func__);
        return -1;
    }

    if (length > 2048)
    {
        LOGE("%s error length: %d\n", __func__, length);
        return -1;
    }

    bta_encoded_data_t *encoded_data = (bta_encoded_data_t *)os_malloc(sizeof(bta_encoded_data_t) + length);

    encoded_data->length = length;

    if (length)
    {
        os_memcpy(encoded_data->payload, data, length);
    }
    else
    {
        LOGE("zero length\n");
    }

    rtos_lock_mutex(&bta_decode.encoded_lock);
    list_add_tail(&encoded_data->list, &bta_decode.encoded_list);
    rtos_unlock_mutex(&bta_decode.encoded_lock);

    xEventGroupSetBits(bta_decode.event, REA_EVT_DECODE_DATA_IND);

    return 0;
}

uint8_t bta_decode_is_enabled(void)
{
    return bta_decode.running;
}
