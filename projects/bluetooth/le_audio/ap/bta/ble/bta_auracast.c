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

#include <common/sys_config.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <components/log.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>

#include <driver/gpio.h>
#include <driver/int.h>


#include <components/bluetooth/bk_assigned_numbers.h>
#include <components/bluetooth/bk_dm_bap.h>

#include "bta_event.h"


#include "bta_audio.h"
#include "bta_auracast.h"

#define TAG "bta_auracast"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define BROADCAST_CONFIG_PHY       (0x02)
#define BROADCAST_CONFIG_PACKING   (0x00)
#define BROADCAST_CONFIG_ID        (0x00)
#define BT_ADDR_SIZE               (6)

typedef struct
{
    uint8_t bis_count;
    uint8_t bis[2];
    uint8_t state;
    uint8_t scan;
    uint8_t session;
    uint8_t broadcast_sep;
    uint8_t role;
    uint16_t connection_handle[BK_GAP_MAX_BIS];
    uint16_t associate_handle;
    uint16_t sequence;
    bta_auracast_scan_cfg_t scan_cfg;
    bk_bap_source_announce_data_t remote_source_data;
} bta_auracast_info_t;


bta_auracast_info_t auracast_info;
static bool s_bap_initialized;
static bool s_sink_registered;
static bool s_source_registered;


bool bta_auracast_is_zero_address(uint8_t *address)
{
    if (address[0] == 0x00
        && address[1] == 0x00
        && address[2] == 0x00
        && address[3] == 0x00
        && address[4] == 0x00
        && address[5] == 0x00)
    {
        return true;
    }

    return false;
}

int bta_auracast_address_cmp(uint8_t *src, uint8_t *dst)
{
    if ((src[0] == dst[0]
         && src[1] == dst[1]
         && src[2] == dst[2]
         && src[3] == dst[3]
         && src[4] == dst[4]
         && src[5] == dst[5])
        || (src[0] == dst[5]
            && src[1] == dst[4]
            && src[2] == dst[3]
            && src[3] == dst[2]
            && src[4] == dst[1]
            && src[5] == dst[0]))
    {
        return 0;
    }

    return -1;
}

void bta_auracast_set_role(uint8_t role)
{
    auracast_info.role = role;
}

uint8_t bta_auracast_get_role(void)
{
    return auracast_info.role;
}

static void bta_auracast_announcement_sink_cb(bk_bap_source_announce_data_t *bk_bap_source_announce_data)
{
    if (AURACAST_STATE_TURN_ON == bta_auracast_get_state())
    {
        if (!bta_auracast_is_zero_address(auracast_info.remote_source_data.address)
            && !os_memcmp(auracast_info.remote_source_data.address, bk_bap_source_announce_data->address, 6))
        {
            LOGI("reassociate source: %02X:%02X:%02X:%02X:%02X:%02X Type: 0x%02X, %d\n",
                 bk_bap_source_announce_data->address[5],
                 bk_bap_source_announce_data->address[4],
                 bk_bap_source_announce_data->address[3],
                 bk_bap_source_announce_data->address[2],
                 bk_bap_source_announce_data->address[1],
                 bk_bap_source_announce_data->address[0],
                 bk_bap_source_announce_data->address_type,
                 bk_bap_source_announce_data->rssi);

            bta_auracast_boradcast_associate(bk_bap_source_announce_data);
        }
        else
        {
            LOGI("reassociate source: try, our %02x:%02x:%02x:%02x:%02x:%02x\n",
                            auracast_info.remote_source_data.address[5],
                            auracast_info.remote_source_data.address[4],
                            auracast_info.remote_source_data.address[3],
                            auracast_info.remote_source_data.address[2],
                            auracast_info.remote_source_data.address[1],
                            auracast_info.remote_source_data.address[0]);
        }
        return;
    }

    bk_bap_source_announce_data_t *data = os_malloc(sizeof(bk_bap_source_announce_data_t));

    if (data == NULL)
    {
        LOGE("%s malloc faild\n", __func__);
        return;
    }

    os_memcpy(data, bk_bap_source_announce_data, sizeof(bk_bap_source_announce_data_t));
    bta_event_send(BTA_EVT_AURACAST_SINK_BROADCAST_SCAN_RESULT_IND, (uint32_t)data, 0);
}

static void bta_auracast_associate_sink_cb(bk_bap_source_associate_data_t *bk_bap_ass_ant_res)
{
    LOGI("sync handle: %x\n", bk_bap_ass_ant_res->handle);
    auracast_info.associate_handle = bk_bap_ass_ant_res->handle;

    bk_dm_bap_broadcast_scan_stop();
}

static void bta_auracast_lc3_data_sink_cb(bk_bap_iso_header_t *header, uint8_t *data, uint32_t length)
{
    LOGD("lc3 %02X, %d, %u, %u, %u, %u\n",
         header->connection_handle,
         header->ts_flag,
         header->data_total_length,
         header->time_stamp,
         header->packet_sequence_number,
         header->iso_sdu_length);

    //GPIO_DOWN(28);GPIO_UP(28);
    LOGD("LC3 %d %02X %02X %02X %02X\n", auracast_info.sequence, data[0], data[1], data[2], data[3]);
    bta_audio_lc3_dec_data_send(data, length);
    //GPIO_DOWN(28);
    auracast_info.sequence++;
}

static inline void auracast_info_sink_callback(uint8_t *data, uint16_t length)
{
    bk_bap_basic_audio_config_t config;
    bk_bap_codec_spec_config_t *spec_cfg;
    lc3_dec_info_t dec_info;

    bk_dm_bap_decode_basic_audio_config(&config, data, length);
    spec_cfg = &config.bis[0].spec_cfg;

    dec_info.channel = bk_dm_bap_get_channel_count(spec_cfg->audio_channel_allocation);

    if (spec_cfg->frame_duration == BK_BT_CODEC_FRAME_DURATION_7500US)
    {
        dec_info.frame_us = 7500;
    }
    else if (spec_cfg->frame_duration == BK_BT_CODEC_FRAME_DURATION_10000US)
    {
        dec_info.frame_us = 10000;
    }
    else
    {
        dec_info.frame_us = 10000;
    }

    switch (spec_cfg->sampling_frequency)
    {
        case BK_BT_CODEC_CFG_FREQ_48KHZ:
            dec_info.dec_srate_hz = 48000;
            break;

            //TODO
    }

    dec_info.frame_bytes = spec_cfg->octects_per_codec_frame;
    dec_info.pcm_sbytes = sizeof(uint16_t);
    dec_info.frame_cnt = sizeof(uint8_t);

    dec_info.frame_samples = dec_info.dec_srate_hz
                             * dec_info.channel / (1000) * (dec_info.frame_us / 1000);
    dec_info.bitrate = (spec_cfg->octects_per_codec_frame /* frame bytes */ * 8 * 1000000UL) / 10000;


    LOGD("channel: %d, interval: %d, samples: %d, per oct: %d, bitrate: %d, Hz: %d, pcm bytes: %d, cnt: %d\n",
         dec_info.channel,
         dec_info.frame_us,
         dec_info.frame_samples,
         dec_info.frame_bytes,
         dec_info.bitrate,
         dec_info.dec_srate_hz,
         dec_info.pcm_sbytes,
         dec_info.frame_cnt);

    auracast_info.bis_count = 1;
    auracast_info.bis[0] = config.bis[0].bis_index;

    bta_codec_config_t codec_config = {0};
    codec_config.format = CODEC_AUDIO_LC3;
    codec_config.channels = dec_info.channel;
    codec_config.sample_rate = dec_info.dec_srate_hz;
    codec_config.frame_length = dec_info.frame_bytes;
    codec_config.duration = dec_info.frame_us;

    bta_audio_set_dec_config(&codec_config);
}

static inline void bta_auracast_big_info_sink_callback(bk_bap_source_big_info_t *bk_int_bap_source_big_info)
{

}

static inline void bta_auracast_broadcast_event_sink_callback(bk_bap_sink_cb_evt_t event, uint32_t status)
{
    bta_event_send(BTA_EVT_AURACAST_SINK_BROADCAST_CALLBACK_IND, event, status);
}

static inline void bta_auracast_setup_announcement_source_cb(uint32_t status, void *paramters)
{
    LOGI("sink setup announcement callback: %d\n", status);
    bta_event_send(BTA_EVT_AURACAST_SROUCE_SETUP_ANNOUNCEMENT_CNF, status, (uint32_t)paramters);

}

static inline void bta_auracast_end_announcement_source_cb(uint32_t status, void *paramters)
{
    LOGI("sink end announcement callback: %d\n", status);
    bta_event_send(BTA_EVT_AURACAST_SROUCE_END_ANNOUNCEMENT_CNF, status, (uint32_t)paramters);
}


static inline void bta_auracast_broadcast_start_source_cb(bk_bap_boradcast_paramters_t *bk_bap_boradcast_paramters)
{
    os_memcpy(auracast_info.connection_handle, bk_bap_boradcast_paramters->connection_handle, sizeof(auracast_info.connection_handle));
    LOGI("connection handle: %d, %d\n",
         auracast_info.connection_handle[0],
         auracast_info.connection_handle[1]);

    bta_event_send(BTA_EVT_AURACAST_SROUCE_START_CNF, 0, 0);
}

static inline void bta_auracast_broadcast_suspend_source_cb(uint8_t handle, uint8_t reason)
{
    bta_event_send(BTA_EVT_AURACAST_SROUCE_SUSPEND_CNF, 0, 0);
}



static const bk_bap_sink_callbacks_t bta_auracast_sink_callbacks =
{
    .announcement_cb = bta_auracast_announcement_sink_cb,
    .associate_cb = bta_auracast_associate_sink_cb,
    .lc3_data_cb  = bta_auracast_lc3_data_sink_cb,
    .config_cb = auracast_info_sink_callback,
    .big_info_cb = bta_auracast_big_info_sink_callback,
    .broadcast_event_cb = bta_auracast_broadcast_event_sink_callback,
};

static const bk_bap_source_callbacks_t bta_auracast_source_callbacks =
{
    .setup_announcement_cb = bta_auracast_setup_announcement_source_cb,
    .end_announcement_cb = bta_auracast_end_announcement_source_cb,
    .broadcast_start_cb = bta_auracast_broadcast_start_source_cb,
    .broadcast_suspend_cb = bta_auracast_broadcast_suspend_source_cb,
};


void bta_auracast_boradcast_associate(bk_bap_source_announce_data_t *bk_bap_source_announce_data)
{
    os_memcpy(&auracast_info.remote_source_data, bk_bap_source_announce_data, sizeof(bk_bap_source_announce_data_t));
    bta_auracast_set_role(AURACAST_ROLE_SINK);
    bk_dm_bap_broadcast_associate(&auracast_info.remote_source_data);
}

void bta_auracast_boradcast_dissociate(void)
{
    bk_dm_bap_broadcast_dissociate(auracast_info.associate_handle);
}

void bta_auracast_boradcast_enable(void)
{
    bta_audio_lc3_dec_init();
    bk_dm_bap_broadcast_enable(auracast_info.associate_handle,
                               NULL,
                               auracast_info.bis_count,
                               auracast_info.bis);
}

void bta_auracast_boradcast_disable(void)
{
    bta_audio_lc3_dec_deinit();
    bk_dm_bap_broadcast_disable(auracast_info.associate_handle);
}

static char *bta_auracast_get_string_state(uint8_t state)
{
    switch (state)
    {
        case AURACAST_STATE_TURN_OFF:
            return "TURN_OFF";
        case AURACAST_STATE_TURNING_OFF:
            return "TURNING_OFF";
        case AURACAST_STATE_TURNING_ON:
            return "TURNING_ON";
        case AURACAST_STATE_TURN_ON:
            return "TURN_ON";
    }

    return "UNKNOWN";
}

void bta_auracast_state_change(uint8_t state)
{
    LOGI("%s old: %s -> new: %s\n", __func__,
         bta_auracast_get_string_state(auracast_info.state),
         bta_auracast_get_string_state(state));

    auracast_info.state = state;

    switch (state)
    {
        case AURACAST_STATE_TURN_OFF:
            bta_auracast_set_role(AURACAST_ROLE_NUKNOWN);
            break;
    }
}

uint8_t bta_auracast_get_state(void)
{
    return auracast_info.state;
}

uint8_t bta_auracast_get_scan_state(void)
{
    return auracast_info.scan;
}

void bta_auracast_boradcast_setup_announcement(void)
{
    bk_bap_codec_info_t broadcast_codec = {0};
    bk_bap_medadata_t broadcast_meta = {0};
    bk_bap_codec_ie_t broadcast_stream = {0};
    uint8_t broadcast_nstream = 1U;

    bk_dm_bap_broadcast_alloc_session(&auracast_info.session);

    LOGI("alloc session %d\n", auracast_info.session);

    bk_dm_bap_broadcast_configure_session(auracast_info.session, BROADCAST_CONFIG_PHY, BROADCAST_CONFIG_PACKING, NULL);

    bk_bap_lc3_codec_specific_conf_t cs_conf;
    cs_conf.sf = BK_BT_CODEC_CFG_FREQ_48KHZ; /* Value: 48KHz */
    cs_conf.fd = BK_BT_CODEC_FRAME_DURATION_10000US;  /* Value: 10ms Frame duration */
    cs_conf.aca = 0x01; /* Value: Channel Count 1 - Bit 0  */
    cs_conf.opcf = LC3_ENC_PER_CODE_FRAME;  /* Value: Number of octets supported per codec frame - 100 */
    cs_conf.mcfpSDU = 1; /* Value - 1, by default */

    bta_codec_config_t enc_cfg;
    enc_cfg.format = CODEC_AUDIO_LC3;
    enc_cfg.channels = 1;
    enc_cfg.sample_rate = LC3_ENC_SAMPLE_RATE;
    enc_cfg.frame_length = LC3_ENC_PER_CODE_FRAME;
    enc_cfg.duration = LC3_ENC_DURATION;

    bta_audio_lc3_enc_init(&enc_cfg);

    broadcast_codec.coding_format = BK_BT_CODEC_ID_LC3;
    broadcast_codec.company_id = 0;
    broadcast_codec.vendor_codec_id = 0;

    os_memset(&broadcast_codec.ie, 0, BK_BAP_CODEC_IE_LEN);
    bk_dm_bap_create_codec_spec_conf_ltv(&cs_conf, broadcast_codec.ie, &broadcast_codec.ie_len);

    os_memset(&broadcast_meta, 0, sizeof(bk_bap_medadata_t));
    //bk_dm_bap_create_metadata_ltv(broadcast_meta.data, &broadcast_meta.length);

    bk_dm_bap_create_codec_spec_conf_ltv(&cs_conf, broadcast_stream.value, &broadcast_stream.length);

    bk_dm_bap_broadcast_sep_register(auracast_info.session,
                                     &broadcast_codec,
                                     &broadcast_meta,
                                     broadcast_nstream,
                                     &broadcast_stream,
                                     &auracast_info.broadcast_sep);

    bk_dm_bap_setup_announcement(auracast_info.session,
                                 BROADCAST_CONFIG_ID,
                                 0,
                                 40 * 1000);
}

void bta_auracast_boradcast_start(void)
{
    uint32_t broadcast_sdu_interval = 0;
    uint16_t broadcast_max_sdu = 0;
    uint16_t broadcast_max_latency = 40;
    uint8_t broadcast_bcast_rtn = 0x02;
    uint8_t broadcast_bcast_framing = 0x00;

    broadcast_sdu_interval = LC3_ENC_DURATION;
    broadcast_max_sdu = LC3_ENC_PER_CODE_FRAME;
    auracast_info.sequence = 1;


    LOGI("%s, frame us: %d, bytes: %d\n", __func__,
         broadcast_sdu_interval, broadcast_max_sdu);

    bk_dm_bap_broadcast_start(auracast_info.session,
                              broadcast_sdu_interval,
                              broadcast_max_sdu,
                              broadcast_max_latency,
                              broadcast_bcast_rtn,
                              broadcast_bcast_framing
                             );
}

void bta_auracast_lc3_dummy(void)
{
    static uint8_t index = 0;

    uint8_t lc3_data[100] = {0};
    lc3_data[0] = index++;
    bta_auracast_data_send(lc3_data, sizeof(lc3_data));
}

void bta_auracast_data_send(uint8_t *data, uint16_t length)
{
    if (AURACAST_STATE_TURN_ON == bta_auracast_get_state())
    {
        //GPIO_DOWN(27);GPIO_UP(27);
        LOGD("LC3 %d %02X %02X %02X %02X\n", auracast_info.sequence, data[0], data[1], data[2], data[3]);
        bk_dm_bap_broadcast_data_send(auracast_info.connection_handle[0],
                                      0, 0, auracast_info.sequence,
                                      data, length);
        auracast_info.sequence++;
        //GPIO_DOWN(27);
    }
}

void bta_auracast_boradcast_scan_start(bta_auracast_scan_cfg_t *cfg)
{
    if (AURACAST_SCAN_IDLE != bta_auracast_get_scan_state())
    {
        LOGI("%s auracast scan busy: %d\n", __func__, bta_auracast_get_scan_state());
        return;
    }

    auracast_info.scan = AURACAST_SCANNING;

    os_memcpy(&auracast_info.scan_cfg, cfg, sizeof(bta_auracast_scan_cfg_t));

    bta_auracast_set_role(AURACAST_ROLE_SINK);
    bta_auracast_state_change(AURACAST_STATE_TURNING_ON);

    bta_event_send(BTA_EVT_AURACAST_SINK_BROADCAST_SCAN_START, 0, 0);
}

void bta_auracast_boradcast_scan_stop(void)
{
    bta_event_send(BTA_EVT_AURACAST_SINK_BROADCAST_SCAN_STOP, 0, 0);
}

void bta_auracast_client_stop(void)
{
    if (AURACAST_STATE_TURN_ON != bta_auracast_get_state() && AURACAST_STATE_TURNING_ON != bta_auracast_get_state())
    {
        LOGI("%s auracast busy: %d\n", __func__, bta_auracast_get_state());
        return;
    }

    if(auracast_info.scan == AURACAST_SCANNING && bta_auracast_get_state() == AURACAST_STATE_TURNING_ON)
    {
        bta_auracast_state_change(AURACAST_STATE_TURNING_OFF);
        bta_auracast_boradcast_scan_stop();
    }
    else
    {
        bta_auracast_state_change(AURACAST_STATE_TURNING_OFF);
        bta_auracast_boradcast_scan_stop();
        bta_event_send(BTA_EVT_AURACAST_SINK_DISABLE_REQ, 0, 0);
    }
}

void bta_auracast_client_play(void)
{
    if (AURACAST_STATE_TURN_OFF != bta_auracast_get_state())
    {
        LOGI("%s auracast busy: %d\n", __func__, bta_auracast_get_state());
        return;
    }

    bta_auracast_state_change(AURACAST_STATE_TURNING_ON);

    bta_auracast_set_role(AURACAST_ROLE_SINK);

    bta_event_send(BTA_EVT_AURACAST_SINK_BROADCAST_SCAN_START, 0, 0);
}

void bta_auracast_client_pause(void)
{
    if (AURACAST_STATE_TURN_ON != bta_auracast_get_state())
    {
        LOGI("%s auracast busy: %d\n", __func__, bta_auracast_get_state());
        return;
    }

    bta_auracast_state_change(AURACAST_STATE_TURNING_OFF);

    bta_event_send(BTA_EVT_AURACAST_SINK_DISABLE_REQ, 0, 0);
}


void bta_auracast_server_start(void)
{
    if (AURACAST_STATE_TURN_OFF != bta_auracast_get_state())
    {
        LOGI("%s auracast busy: %d\n", __func__, bta_auracast_get_state());
        return;
    }

    bta_auracast_state_change(AURACAST_STATE_TURNING_ON);

    bta_auracast_set_role(AURACAST_ROLE_SOURCE);

    bta_auracast_boradcast_setup_announcement();
}

void bta_auracast_server_stop(void)
{
    if (AURACAST_STATE_TURN_ON != bta_auracast_get_state())
    {
        LOGI("%s auracast busy: %d\n", __func__, bta_auracast_get_state());
        return;
    }

    bta_auracast_state_change(AURACAST_STATE_TURNING_OFF);

    bta_event_send(BTA_EVT_AURACAST_SOURCE_DISABLE_REQ, 0, 0);
}

void bta_auracast_init_handle(void)
{
    bk_err_t ret;

    if (!s_bap_initialized)
    {
        ret = bk_dm_bap_init();
        if (ret != BK_OK)
        {
            LOGE("%s bk_dm_bap_init failed: %d\n", __func__, ret);
            return;
        }

        s_bap_initialized = true;
    }

    if (!s_sink_registered)
    {
        ret = bk_dm_bap_sink_register(&bta_auracast_sink_callbacks);
        if (ret != BK_OK)
        {
            LOGE("%s sink register failed: %d\n", __func__, ret);
            return;
        }

        s_sink_registered = true;
    }

    if (!s_source_registered)
    {
        ret = bk_dm_bap_source_register(&bta_auracast_source_callbacks);
        if (ret != BK_OK)
        {
            LOGE("%s source register failed: %d\n", __func__, ret);
            return;
        }

        s_source_registered = true;
    }
}

void bta_auracast_sink_bc_callback(uint32_t event, uint32_t status)
{
    switch (event)
    {
        case BTA_EVT_AURACAST_INIT:
        {
            bta_auracast_init_handle();
        }
        break;

        case BK_BAP_SINK_SCAN_END:
        {
            LOGI("BK_BAP_SINK_SCAN_END %d\n", bta_auracast_get_state());

            if (auracast_info.scan_cfg.stop_cb)
            {
                auracast_info.scan_cfg.stop_cb(NULL);
            }

            //bta_auracast_state_change(AURACAST_STATE_TURNING_ON);
            auracast_info.scan = AURACAST_SCAN_IDLE;

            auracast_info.sequence = 1;

            if(bta_auracast_get_state() == AURACAST_STATE_TURNING_ON)// || bta_auracast_get_state() == AURACAST_STATE_TURN_ON)
            {
                bta_auracast_boradcast_enable();
            }
            else
            {
                bta_auracast_state_change(AURACAST_STATE_TURN_OFF);
            }
        }
        break;

        case BK_BAP_SINK_DISABLE_IND:
        {
            LOGI("BK_BAP_SINK_DISABLE_IND %d\n", bta_auracast_get_state());

            if (AURACAST_STATE_TURN_ON == bta_auracast_get_state() || AURACAST_STATE_TURNING_ON == bta_auracast_get_state())
            {
                bta_audio_lc3_dec_deinit();

                bta_auracast_state_change(AURACAST_STATE_TURNING_ON);
                bk_dm_bap_broadcast_scan_start();
            }
        }
        break;

        case BK_BAP_SINK_DISABLE_CNF:
        {
            LOGI("BK_BAP_SINK_DISABLE_CNF\n");
            bta_auracast_state_change(AURACAST_STATE_TURN_OFF);
            //TODO?
        }
        break;

        case BK_BAP_SINK_ENABLE_CNF:
        {
            LOGI("BK_BAP_SINK_ENABLE_CNF\n");

            bk_dm_bap_broadcast_dissociate(auracast_info.associate_handle);

            bta_auracast_state_change(AURACAST_STATE_TURN_ON);
        }
        break;

        default:
            break;

    }
}

void bta_auracast_event_dispather(uint32_t event, uint32_t param, uint32_t extra)
{
    switch (event)
    {
        case BTA_EVT_AURACAST_INIT:
        {
            bta_auracast_init_handle();
        }
        break;

        case BTA_EVT_AURACAST_SINK_BROADCAST_CALLBACK_IND:
        {
            bta_auracast_sink_bc_callback(param, extra);
        }
        break;

        case BTA_EVT_AURACAST_SINK_BROADCAST_SCAN_START:
        {
            bk_dm_bap_broadcast_scan_start();
        }
        break;

        case BTA_EVT_AURACAST_SINK_BROADCAST_SCAN_STOP:
        {
            bk_dm_bap_broadcast_scan_stop();
        }
        break;

        case BTA_EVT_AURACAST_SINK_BROADCAST_SCAN_RESULT_IND:
        {
            bk_bap_source_announce_data_t *result = (bk_bap_source_announce_data_t *)param;

            if (result)
            {
                if (auracast_info.scan_cfg.result_cb)
                {
                    auracast_info.scan_cfg.result_cb(result);
                }
                os_free(result);
            }
        }
        break;

        case BTA_EVT_AURACAST_SINK_DISABLE_REQ:
        {
            bta_auracast_boradcast_disable();
        }
        break;

        case BTA_EVT_AURACAST_SOURCE_DISABLE_REQ:
        {
            bk_dm_bap_broadcast_suspend(auracast_info.session);
        }
        break;

        case BTA_EVT_AURACAST_SROUCE_START_CNF:
        {
            bta_auracast_state_change(AURACAST_STATE_TURN_ON);
        }
        break;

        case BTA_EVT_AURACAST_SROUCE_SUSPEND_CNF:
        {
            bk_dm_bap_end_announcement(auracast_info.session);
        }
        break;

        case BTA_EVT_AURACAST_SROUCE_SETUP_ANNOUNCEMENT_CNF:
        {
            bta_auracast_boradcast_start();
        }
        break;

        case BTA_EVT_AURACAST_SROUCE_END_ANNOUNCEMENT_CNF:
        {
            bk_dm_bap_broadcast_free_session(auracast_info.session);
            bta_auracast_state_change(AURACAST_STATE_TURN_OFF);
        }
        break;


        default:
            break;
    }
}

void bta_auracast_init(void)
{
    os_memset(&auracast_info, 0, sizeof(bta_auracast_info_t));

    auracast_info.state = AURACAST_STATE_TURN_OFF;
    auracast_info.role = AURACAST_ROLE_NUKNOWN;
    auracast_info.scan = AURACAST_SCAN_IDLE;

    bta_event_send(BTA_EVT_AURACAST_INIT, 0, 0);
}

