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
#include "bta_encode.h"

#include "bta_dac.h"

#define TAG "bta_audio"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define LE_AUDIO_FRAME_NODES_BUFFER_SIZE        (16 * 1024)
#define LE_AUDIO_DECODER_BUFFER_SIZE            (9064)
#define DEFAULT_AUDIO_VOLUME                 (0x12)

#define BT_AUDIO_SINK_DEMO_MSG_COUNT            (60)
#define LE_AUDIO_DEMO_TASK_PRIORITY             (4)
#define CONFIG_LE_AUDIO_CACHE_FRAME_NUM         6

#define BTA_AUDIO_DAC_PLAY_THRESHOLD            (2)
#define BTA_AUDIO_DAC_SYNC_THRESHOLD            (7)

static bta_audio_info_t bta_audio_info = {0};

int bta_audio_init(void)
{
    bta_dac_set_gain_value(GET_VOLUME_GAIN(DEFAULT_AUDIO_VOLUME));

    os_memset(&bta_audio_info, 0, sizeof(bta_audio_info_t));
    bta_audio_info.volume = DEFAULT_AUDIO_VOLUME;
    return BK_OK;
}

void bta_audio_decode_data_callback(uint8_t *data, uint32_t length)
{
    if (bta_resample_is_enabled())
    {
        bta_resample_write(data, length);
    }
    else
    {
        if (bta_dac_write(data, length) < 0)
        {
            LOGE("dac fifo full: %d\n", length);
        }

        if (bta_dac_get_state() == DAC_STATE_STOP)
        {
            if (bta_dac_get_fill_size() > bta_dac_get_pcm_length() * 3)
            {
                bta_dac_start();
            }
        }
    }
}

void bta_audio_resample_data_callback(uint8_t *data, uint32_t length)
{
    uint32_t play_threshold = 0;

    if (bta_dac_write(data, length) < 0)
    {
        LOGD("dac fifo full: %d\n", length);
    }

    if (bta_audio_info.encoded)
    {
        bta_encode_write(data, length);

        play_threshold = bta_dac_get_pcm_length() * BTA_AUDIO_DAC_SYNC_THRESHOLD;

        if (bta_audio_info.dac_reset == true)
        {
            bta_dac_stop();
            LOGE("reset dac\n");
            bta_audio_info.dac_reset = false;
        }
    }
    else
    {
        play_threshold = bta_dac_get_pcm_length() * BTA_AUDIO_DAC_PLAY_THRESHOLD;
    }

    if (bta_dac_get_state() == DAC_STATE_STOP)
    {
        if (bta_dac_get_fill_size() > play_threshold)
        {
            LOGE("start dac\n");
            bta_dac_start();
        }
    }
}

void bta_audio_encode_data_callback(uint8_t *data, uint32_t length)
{
    //LOGI("lc3 enc: %d\n", length);
    bta_auracast_data_send(data, length);
}


void bta_audio_lc3_dec_data_send(void *data, uint32_t length)
{
    bta_decode_write(data, length);
}

void bta_audio_set_dec_config(bta_codec_config_t *cfg)
{
    os_memcpy(&bta_audio_info.dec_cfg, cfg, sizeof(bta_codec_config_t));
}

void bta_audio_lc3_dec_init(void)
{
    LOGI("%s\n", __func__);

    bta_dac_config_t dac_config;
    bta_decode_config_t dec_cfg;

    dec_cfg.format =  bta_audio_info.dec_cfg.format;
    dec_cfg.channels =  bta_audio_info.dec_cfg.channels;
    dec_cfg.sample_rate =  bta_audio_info.dec_cfg.sample_rate;
    dec_cfg.frame_length =  bta_audio_info.dec_cfg.frame_length;
    dec_cfg.duration = bta_audio_info.dec_cfg.duration;
    dec_cfg.cb = bta_audio_decode_data_callback;

    dac_config.sample_rate = dec_cfg.sample_rate;
    dac_config.duration = 10 * 1000;

    bta_decode_start(&dec_cfg);
    bta_dac_init(&dac_config);

}

void bta_audio_lc3_dec_deinit(void)
{
    bta_dac_deinit();
    bta_decode_stop();
}

void bta_audio_lc3_enc_init(bta_codec_config_t *cfg)
{
    os_memcpy(&bta_audio_info.enc_cfg, cfg, sizeof(bta_codec_config_t));

    bta_encode_config_t enc_config;
    enc_config.format = CODEC_AUDIO_LC3;
    enc_config.channels = cfg->channels;
    enc_config.sample_rate = cfg->sample_rate;
    enc_config.frame_length = cfg->frame_length;
    enc_config.duration = cfg->duration;

    enc_config.cb = bta_audio_encode_data_callback;

    bta_encode_start(&enc_config);

    bta_audio_info.encoded = true;
    bta_audio_info.dac_reset = true;
}

void bta_audio_lc3_enc_deinit(void)
{
    bta_encode_stop();
}

void bta_audio_set_speaker_gain_value(uint8_t gain)
{
    bta_dac_set_gain_value(gain);
}

bk_err_t bta_audio_set_speaker_gain(uint8_t gain)
{
    return bta_dac_set_gain(gain);
}

void bta_audio_set_abs_volume_handle(uint32_t per)
{
    uint32_t gain = ((per * SPEAKER_GAIN_MAX) / 100) & 0xFF;

    LOGI("%s: %d% gain: %u\n", __func__, per, gain);

    bta_audio_set_speaker_gain(gain);
}


uint8_t bta_audio_calc_volume(uint8_t value, uint8_t up)
{
    char volume = GET_VOLUME_STEP(value);

    if (up)
    {
        volume++;
    }
    else
    {
        volume--;
    }

    if (volume < VOLUME_STEP_MIN)
    {
        volume = VOLUME_STEP_MIN;
    }
    else if (volume >= VOLUME_STEP_MAX)
    {
        volume = 0x7F;
    }
    else
    {
        volume = (volume - 1) * 8 + 9;
    }

    return volume & 0xFF;
}

uint8_t bta_audio_calc_volume_gain(uint8_t value, uint8_t up)
{
    return bta_audio_calc_volume(value, up) >> 1;
}

void bta_audio_volume_up_handle(void)
{
    uint8_t volume = bta_audio_calc_volume(bta_audio_info.volume, true);

    LOGI("%s: %u -> %u, %d -> %d\n", __func__,
         bta_audio_info.volume, volume,
         GET_VOLUME_GAIN(bta_audio_info.volume),  GET_VOLUME_GAIN(volume));

    bta_audio_info.volume = volume;

    bta_audio_set_speaker_gain(GET_VOLUME_GAIN(bta_audio_info.volume));
}

void bta_audio_volume_down_handle(void)
{
    uint8_t volume = bta_audio_calc_volume(bta_audio_info.volume, false);

    LOGI("%s: %u -> %u, %d -> %d\n", __func__,
         bta_audio_info.volume, volume,
         GET_VOLUME_GAIN(bta_audio_info.volume),  GET_VOLUME_GAIN(volume));

    bta_audio_info.volume = volume;

    bta_audio_set_speaker_gain(GET_VOLUME_GAIN(bta_audio_info.volume));
}


void bta_audio_set_abs_volume(uint8_t per)
{
    bta_event_send(BTA_EVT_AUDIO_VOLUME_ABS, per, 0);
}

void bta_audio_volume_up(void)
{
    bta_event_send(BTA_EVT_AUDIO_VOLUME_UP, 0, 0);
}

void bta_audio_volume_down(void)
{
    bta_event_send(BTA_EVT_AUDIO_VOLUME_DOWN, 0, 0);
}

void bta_audio_event_dispather(uint32_t event, uint32_t param, uint32_t extra)
{
    switch (event)
    {
        case BTA_EVT_AUDIO_INIT:
        {

        }
        break;

        case BTA_EVT_AUDIO_VOLUME_ABS:
        {
            bta_audio_set_abs_volume_handle(param);
        }
        break;

        case BTA_EVT_AUDIO_VOLUME_UP:
        {
            bta_audio_volume_up_handle();
        }
        break;

        case BTA_EVT_AUDIO_VOLUME_DOWN:
        {
            bta_audio_volume_down_handle();
        }
        break;

        default:
            break;
    }
}

