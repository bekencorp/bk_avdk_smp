#include <components/system.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <driver/timer.h>

#include "FreeRTOS.h"
#include "event_groups.h"


#include <driver/uart.h>
#include "gpio_driver.h"

#include "bk_ring_buffer_node.h"
#include "bta_audio.h"
#include "bluetooth_config.h"
#include "bta_encode.h"



#define TAG "bta_encode"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define EVT_EXIT_IND        (1 << 0)
#define EVT_PCM_DATA_IND    (1 << 8)
#define WAITTING_EVENTS     (EVT_PCM_DATA_IND)

#define LE_AUDIO_DECODER_BUFFER_SIZE            (9064)

typedef struct
{
    uint8_t *ring_buffer;
    bk_ring_buffer_octets_context_t context;
    EventGroupHandle_t event;
    beken_thread_t hanndle;
    beken_mutex_t ctx_lock;
    uint8_t running;
    bta_encode_config_t cfg;

    uint8_t *work_buffer;

    union
    {
        lc3_enc_info_t lc3;
    };
} bta_encode_info_t;

#define ENCODE_RING_BUFFER_SIZE     (1024 * 20)

static bta_encode_info_t encode_info = {0};

static void bta_encode_task_entry(void *arg)
{
    bk_err_t ret;

    xEventGroupClearBits(encode_info.event, EVT_EXIT_IND);

    while (encode_info.running == true)
    {
        EventBits_t bits = xEventGroupWaitBits(encode_info.event, WAITTING_EVENTS, true, false, 300 / bk_get_ms_per_tick());

        if (bits & EVT_PCM_DATA_IND)
        {
            switch (encode_info.cfg.format)
            {
                case CODEC_AUDIO_LC3:
                {
                    uint8_t lc3_data[100] = {0};
                    uint16_t pcm_data[480] = {0};

                    do
                    {
                        ret = bk_ring_buffer_octets_len(&encode_info.context);

                        if (ret < sizeof(pcm_data))
                        {
                            LOGD("pcm not enough\n");
                            break;
                        }

                        rtos_lock_mutex(&encode_info.ctx_lock);
                        ret = bk_ring_buffer_octets_read(&encode_info.context, (uint8_t *)pcm_data, sizeof(pcm_data));
                        rtos_unlock_mutex(&encode_info.ctx_lock);

                        if (sizeof(pcm_data) != ret)
                        {
                            LOGE("read error enough\n");
                            return;
                        }

                        lc3_encoder_proc(&encode_info.lc3, (uint8_t *)pcm_data, lc3_data);

                        if (encode_info.cfg.cb)
                        {
                            encode_info.cfg.cb(lc3_data, sizeof(lc3_data));
                        }

                    }
                    while (true);
                }
                break;

                default:
                    LOGE("%s, unsupported codec %d\n", __func__, encode_info.cfg.format);
                    break;

            }
        }

    }

    xEventGroupSetBits(encode_info.event, EVT_EXIT_IND);

    rtos_delete_thread(NULL);

}

void bta_encode_write(uint8 *data, uint32_t length)
{
    int ret;

    rtos_lock_mutex(&encode_info.ctx_lock);
    ret = bk_ring_buffer_octets_write(&encode_info.context, data, length);
    rtos_unlock_mutex(&encode_info.ctx_lock);

    if (ret < 0)
    {
        LOGE("%s error\n", __func__);
        return;
    }

    xEventGroupSetBits(encode_info.event, EVT_PCM_DATA_IND);
}

static void bta_encode_config(bta_encode_config_t *cfg)
{
    os_memcpy(&encode_info.cfg, cfg, sizeof(bta_encode_config_t));

    switch (encode_info.cfg.format)
    {
        case CODEC_AUDIO_LC3:
        {
            encode_info.lc3.channel = encode_info.cfg.channels;
            encode_info.lc3.frame_us = encode_info.cfg.duration;
            encode_info.lc3.frame_bytes = encode_info.cfg.frame_length;
            encode_info.lc3.enc_srate_hz = encode_info.cfg.sample_rate;
            encode_info.lc3.pcm_sbytes = 2;
            encode_info.lc3.frame_cnt = 1;

            encode_info.lc3.frame_samples = (encode_info.lc3.enc_srate_hz * encode_info.lc3.frame_us * encode_info.lc3.channel) / 1000000UL;
            encode_info.lc3.bitrate = (encode_info.lc3.frame_bytes * 8 * 1000000UL) / 10000;

            LOGI("channel: %d, interval: %d, samples: %d, per oct: %d, bitrate: %d, Hz: %d, pcm bytes: %d, cnt: %d\n",
                 encode_info.lc3.channel,
                 encode_info.lc3.frame_us,
                 encode_info.lc3.frame_samples,
                 encode_info.lc3.frame_bytes,
                 encode_info.lc3.bitrate,
                 encode_info.lc3.enc_srate_hz,
                 encode_info.lc3.pcm_sbytes,
                 encode_info.lc3.frame_cnt);

            encode_info.work_buffer = os_malloc(LE_AUDIO_DECODER_BUFFER_SIZE);

            if (encode_info.work_buffer == NULL)
            {
                LOGE("malloc work_buffer fail \n");
                return;
            }

            lc3_encoder_init(&encode_info.lc3, (uint8_t *)encode_info.work_buffer);
        }
        break;

        default:
            LOGE("%s, unsupported codec %d\n", __func__, encode_info.cfg.format);
            break;

    }
}

void bta_encode_start(bta_encode_config_t *config)
{
    bk_err_t ret = BK_OK;

    LOGI("%s\n", __func__);


    rtos_init_mutex(&encode_info.ctx_lock);

    encode_info.event = xEventGroupCreate();
    encode_info.ring_buffer = os_malloc(ENCODE_RING_BUFFER_SIZE);

    bta_encode_config(config);

    bk_ring_buffer_octets_init(&encode_info.context, encode_info.ring_buffer, ENCODE_RING_BUFFER_SIZE);

    if (encode_info.event == NULL)
    {
        LOGE("encode_info.event null!\n");
    }

    encode_info.running = true;

#if SMP_THREAD_USED
    ret = rtos_core1_create_thread(&encode_info.hanndle,
                                   BEKEN_DEFAULT_WORKER_PRIORITY - 2,
                                   "encode",
                                   (beken_thread_function_t)bta_encode_task_entry,
                                   1024 * 6,
                                   (beken_thread_arg_t)0);
#else
    ret = rtos_create_thread(&encode_info.hanndle,
                             BEKEN_DEFAULT_WORKER_PRIORITY - 2,
                             "encode",
                             (beken_thread_function_t)bta_encode_task_entry,
                             1024 * 6,
                             (beken_thread_arg_t)0);
#endif
    if (ret != BK_OK)
    {
        LOGE("%s fail \n", __func__);
    }
}

void bta_encode_stop(void)
{
    LOGI("%s +++\n", __func__);

    encode_info.running = false;

    xEventGroupWaitBits(encode_info.event, EVT_EXIT_IND, true, false, BEKEN_WAIT_FOREVER);
    vEventGroupDelete(encode_info.event);

    LOGI("%s ---\n", __func__);

}

uint8_t bta_encode_is_enabled(void)
{
    return encode_info.running;
}

