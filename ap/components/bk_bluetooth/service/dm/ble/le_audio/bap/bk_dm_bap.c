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


#include <common/bk_include.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <components/log.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>

#include "../le_audio/bk_dm_le_audio_gap.h"
#include "bk_internal_dm_ble_bap.h"
#include <components/bluetooth/bk_assigned_numbers.h>
#include <components/bluetooth/bk_dm_bluetooth_types.h>
#include <components/bluetooth/bk_dm_bap.h>

#define TAG "bap"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)


static const bk_bap_sink_callbacks_t *sink_callbacks = NULL;
static const bk_bap_source_callbacks_t *source_callbacks = NULL;

static const bk_bap_pacs_cfg_t s_bap_default_pacs_cfg =
{
    .supported_contexts = BK_GAP_DEFAULT_CONTEXTS,
    .available_contexts = BK_GAP_DEFAULT_CONTEXTS,
    .audio_location = BK_BT_AUDIO_LOCATION_FRONT_LEFT | BK_BT_AUDIO_LOCATION_FRONT_RIGHT,
    .supported_sampling_frequencies = BK_BAP_LC3_CAP_FREQ_48KHZ,
    .supported_frame_durations = BK_BAP_LC3_CAP_DURATION_10MS,
    .supported_channel_counts = BK_BAP_LC3_CAP_CHANNEL_COUNT_1,
    .frame_octets_min = 100U,
    .frame_octets_max = 100U,
    .max_codec_frames_per_sdu = 1U,
};

static const bk_bap_ascs_cfg_t s_bap_default_ascs_cfg =
{
    .ase_count = BK_GAP_DEFAULT_AES_COUNT,
    .pref_framing = BK_BAP_QOS_FRAMING_UNFRAMED,
    .pref_phy = BK_BAP_QOS_PHY_1M,
    .pref_max_transport_latency = BK_BAP_QOS_LATENCY_10MS,
    .pref_presentation_delay_min = BK_BAP_QOS_PRESENTATION_DELAY_0US,
    .pref_presentation_delay_max = BK_BAP_QOS_PRESENTATION_DELAY_40MS,
    .pref_retransmission_number = BK_BAP_QOS_RETRANSMISSION_2,
    .supported_presentation_delay_min = BK_BAP_QOS_PRESENTATION_DELAY_0US,
    .supported_presentation_delay_max = BK_BAP_QOS_PRESENTATION_DELAY_40MS,
};

static bki_bap_pacs_cfg_t s_bap_sink_pacs_cfg;
static bki_bap_pacs_cfg_t s_bap_source_pacs_cfg;
static bki_bap_ascs_cfg_t s_bap_sink_ascs_cfg;
static bki_bap_ascs_cfg_t s_bap_source_ascs_cfg;

static void bk_dm_bap_copy_pacs_cfg(bki_bap_pacs_cfg_t *dst, const bk_bap_pacs_cfg_t *src)
{
    if (src == NULL)
    {
        src = &s_bap_default_pacs_cfg;
    }

    dst->supported_contexts = src->supported_contexts;
    dst->available_contexts = src->available_contexts;
    dst->audio_location = src->audio_location;
    dst->supported_sampling_frequencies = src->supported_sampling_frequencies;
    dst->supported_frame_durations = src->supported_frame_durations;
    dst->supported_channel_counts = src->supported_channel_counts;
    dst->frame_octets_min = src->frame_octets_min;
    dst->frame_octets_max = src->frame_octets_max;
    dst->max_codec_frames_per_sdu = src->max_codec_frames_per_sdu;
}

static void bk_dm_bap_copy_ascs_cfg(bki_bap_ascs_cfg_t *dst, const bk_bap_ascs_cfg_t *src)
{
    if (src == NULL)
    {
        src = &s_bap_default_ascs_cfg;
    }

    dst->ase_count = src->ase_count;
    dst->pref_framing = src->pref_framing;
    dst->pref_phy = src->pref_phy;
    dst->pref_max_transport_latency = src->pref_max_transport_latency;
    dst->pref_presentation_delay_min = src->pref_presentation_delay_min;
    dst->pref_presentation_delay_max = src->pref_presentation_delay_max;
    dst->pref_retransmission_number = src->pref_retransmission_number;
    dst->supported_presentation_delay_min = src->supported_presentation_delay_min;
    dst->supported_presentation_delay_max = src->supported_presentation_delay_max;
}

/* PACS (Sink/Source PAC records) is registered eagerly in the sink/source
 * register calls, but ASCS ASE registration is deferred to the GATT DB pre-seal
 * hook (see bk_dm_bap_pre_seal_cb). Registering ASEs interleaved with PACS (sink
 * PACS -> sink ASE -> source PACS -> source ASE) splits PACS's attribute handles
 * across ASCS, yielding two primary services with overlapping handle ranges that
 * peers cannot discover (ASCS shows up empty). Deferring ASCS keeps the two
 * service ranges disjoint. */
static uint8_t s_bap_sink_registered = 0U;
static uint8_t s_bap_source_registered = 0U;
static uint8_t s_bap_ascs_registered = 0U;

/* Unicast server connectable extended advertising, driven through the shared LE
 * Audio GAP helper (GA-native ext-adv path). */
#define BAP_UNICAST_ADV_HANDLE        0U
#define BAP_UNICAST_ADV_INTERVAL_MIN  0x0078U
#define BAP_UNICAST_ADV_INTERVAL_MAX  0x00A0U

static const uint8_t s_unicast_full_name[] = "BK7259 LE Audio";
static uint8_t s_unicast_gap_cb_registered = 0U;
static uint8_t s_unicast_adv_started = 0U;

/* BAP-compliant Unicast Server announcement for connectable extended
 * advertising (BAP spec Table 3.7): Flags + Service Data (ASCS UUID +
 * Announcement Type + Available Audio Contexts + Metadata) + Complete Local
 * Name. Extended connectable advertising is non-scannable, so everything goes
 * in the advertising data (there is no scan response). */
static void bk_dm_bap_build_unicast_adv_ext(uint8_t *adv_data, uint8_t *adv_len)
{
	uint8_t pos = 0U;
	uint16_t avail_sink = BK_GAP_DEFAULT_CONTEXTS;
	uint16_t avail_src = BK_GAP_DEFAULT_CONTEXTS;

	adv_data[pos++] = 0x02U; /* Flags */
	adv_data[pos++] = 0x01U;
	adv_data[pos++] = 0x06U; /* LE General Discoverable + BR/EDR Not Supported */

	/* Service Data - 16-bit UUID: BAP Unicast Server Announcement (Table 3.7).
	 * length = type(1) + ASCS UUID(2) + Announcement Type(1) +
	 *          Available Audio Contexts(4) + Metadata_Length(1) = 9 */
	adv_data[pos++] = 0x09U;
	adv_data[pos++] = 0x16U; /* Service Data - 16-bit UUID */
	adv_data[pos++] = (uint8_t)(BK_BT_UUID_ASCS & 0xFFU);
	adv_data[pos++] = (uint8_t)((BK_BT_UUID_ASCS >> 8U) & 0xFFU);
	adv_data[pos++] = 0x00U; /* Announcement Type: General */
	adv_data[pos++] = (uint8_t)(avail_sink & 0xFFU);        /* Sink available contexts */
	adv_data[pos++] = (uint8_t)((avail_sink >> 8U) & 0xFFU);
	adv_data[pos++] = (uint8_t)(avail_src & 0xFFU);         /* Source available contexts */
	adv_data[pos++] = (uint8_t)((avail_src >> 8U) & 0xFFU);
	adv_data[pos++] = 0x00U; /* Metadata_Length = 0 */

	adv_data[pos++] = (uint8_t)(1U + sizeof(s_unicast_full_name) - 1U);
	adv_data[pos++] = 0x09U; /* Complete Local Name */
	os_memcpy(&adv_data[pos], s_unicast_full_name, sizeof(s_unicast_full_name) - 1U);
	pos += (uint8_t)(sizeof(s_unicast_full_name) - 1U);

	*adv_len = pos;
}

/* ACL connect-ready up-call: fed from the shared GAP helper on link-up. */
static void bk_dm_bap_gap_conn_cb(uint8_t connected,
                                  uint8_t status,
                                  uint8_t addr_type,
                                  const uint8_t addr[6],
                                  uint16_t acl_handle,
                                  void *ctx)
{
    bki_bap_unicast_ready_t info;

    (void)ctx;
    if (connected && status != BK_OK)
    {
        return;
    }

    os_memset(&info, 0, sizeof(info));
    info.phase = connected ? BKI_BAP_UNICAST_READY_ACL_CONNECTED
                           : BKI_BAP_UNICAST_READY_ACL_DISCONNECTED;
    info.status = status;
    info.addr_type = addr_type;
    info.acl_handle = acl_handle;
    if (addr != NULL)
    {
        os_memcpy(info.addr, addr, sizeof(info.addr));
    }

    /* Connectable extended advertising is auto-terminated on connection;
     * clear the flag so a later re-advertise starts fresh. */
    s_unicast_adv_started = 0U;

    bk_dm_bap_internal_unicast_ready(&info);
}

static void bk_dm_bap_pre_seal_cb(void *ctx);

static bk_err_t bk_dm_bap_register_gap_events(void)
{
    bk_err_t ret;

    if (s_unicast_gap_cb_registered)
    {
        return BK_OK;
    }

    ret = bk_dm_le_audio_gap_register_conn_callback(bk_dm_bap_gap_conn_cb, NULL);
    if (ret != BK_OK)
    {
        return ret;
    }

    /* Add the ASCS ASEs from the pre-seal hook so they land after all PACS
     * records but before the GATT DB is committed. */
    ret = bk_dm_le_audio_gap_register_pre_seal_callback(bk_dm_bap_pre_seal_cb, NULL);
    if (ret == BK_OK)
    {
        s_unicast_gap_cb_registered = 1U;
    }
    return ret;
}

/* Pre-seal hook: add the ASCS ASEs once, after all PACS records are in place.
 * The shared GAP helper invokes this right before it commits the GATT DB (on the
 * first advertise), so the ASCS attributes are laid out after - and with a handle
 * range disjoint from - PACS. Registering ASEs interleaved with PACS would split
 * PACS's handles across ASCS, yielding two primary services with overlapping
 * handle ranges that peers cannot discover (ASCS shows up empty). */
static void bk_dm_bap_pre_seal_cb(void *ctx)
{
    uint16_t ret;

    (void)ctx;

    if (s_bap_ascs_registered)
    {
        return;
    }

    if (s_bap_sink_registered)
    {
        ret = appl_le_audio_ga_ascs_register(BK_GAP_ROLE_SINK, &s_bap_sink_ascs_cfg);
        LOGI("%s ascs role=sink ret=%d\n", __func__, ret);
    }

    if (s_bap_source_registered)
    {
        ret = appl_le_audio_ga_ascs_register(BK_GAP_ROLE_SOURCE, &s_bap_source_ascs_cfg);
        LOGI("%s ascs role=source ret=%d\n", __func__, ret);
    }

    s_bap_ascs_registered = 1U;
}

static bk_err_t bk_dm_bap_unicast_adv_set(uint8_t enable)
{
    uint8_t adv_data[64];
    uint8_t adv_len = 0;
    bk_ble_gap_ext_adv_params_t adv_param;
    bk_err_t ret;

    if (bk_dm_bap_register_gap_events() != BK_OK)
    {
        return BK_FAIL;
    }

    if (!enable)
    {
        if (s_unicast_adv_started)
        {
            ret = bk_dm_le_audio_gap_adv_stop(BAP_UNICAST_ADV_HANDLE);
            if (ret != BK_OK)
            {
                return ret;
            }
            s_unicast_adv_started = 0U;
        }
        return BK_OK;
    }

    os_memset(&adv_param, 0, sizeof(adv_param));
    adv_param.interval_min = BAP_UNICAST_ADV_INTERVAL_MIN;
    adv_param.interval_max = BAP_UNICAST_ADV_INTERVAL_MAX;

    /* BAP Table 3.7 Unicast Server announcement over connectable extended
     * advertising (non-scannable, so no scan response). */
    bk_dm_bap_build_unicast_adv_ext(adv_data, &adv_len);

    ret = bk_dm_le_audio_gap_adv_start(BAP_UNICAST_ADV_HANDLE, &adv_param,
                                       adv_data, adv_len, NULL, 0U);
    if (ret != BK_OK)
    {
        return ret;
    }

    s_unicast_adv_started = 1U;
    return BK_OK;
}

void bk_dm_bap_internal_unicast_ase_discovered(const bki_bap_unicast_ase_discovered_t *info)
{
    bk_bap_unicast_ase_discovered_t public_info;

    if (info == NULL)
    {
        return;
    }

    public_info.ase_id = info->ase_id;
    public_info.ase_role = info->ase_role;
    public_info.ase_state = info->ase_state;
    public_info.acl_handle = info->acl_handle;

    if (sink_callbacks && sink_callbacks->unicast_ase_discovered_cb)
    {
        sink_callbacks->unicast_ase_discovered_cb(&public_info);
    }

    if (source_callbacks && source_callbacks->unicast_ase_discovered_cb)
    {
        if (!sink_callbacks || source_callbacks->unicast_ase_discovered_cb != sink_callbacks->unicast_ase_discovered_cb)
        {
            source_callbacks->unicast_ase_discovered_cb(&public_info);
        }
    }
}

void bk_dm_bap_internal_unicast_cis_handle_assigned(const bki_bap_unicast_cis_info_t *info)
{
    bk_bap_unicast_cis_info_t public_info;

    if (info == NULL)
    {
        return;
    }

    public_info.ase_id = info->ase_id;
    public_info.ase_role = info->ase_role;
    public_info.cig_id = info->cig_id;
    public_info.cis_id = info->cis_id;
    public_info.acl_handle = info->acl_handle;
    public_info.local_cis_handle = info->local_cis_handle;

    if (source_callbacks && source_callbacks->unicast_cis_handle_assigned_cb)
    {
        source_callbacks->unicast_cis_handle_assigned_cb(&public_info);
    }
}

void bk_dm_bap_internal_unicast_cis_request(const bki_bap_unicast_cis_info_t *info)
{
    bk_bap_unicast_cis_info_t public_info;

    if (info == NULL)
    {
        return;
    }

    public_info.ase_id = info->ase_id;
    public_info.ase_role = info->ase_role;
    public_info.cig_id = info->cig_id;
    public_info.cis_id = info->cis_id;
    public_info.acl_handle = info->acl_handle;
    public_info.local_cis_handle = info->local_cis_handle;

    if (sink_callbacks && sink_callbacks->unicast_cis_request_cb)
    {
        sink_callbacks->unicast_cis_request_cb(&public_info);
    }
}

void bk_dm_bap_internal_unicast_cis_established(const bki_bap_unicast_cis_info_t *info)
{
    bk_bap_unicast_cis_info_t public_info;

    if (info == NULL)
    {
        return;
    }

    public_info.ase_id = info->ase_id;
    public_info.ase_role = info->ase_role;
    public_info.cig_id = info->cig_id;
    public_info.cis_id = info->cis_id;
    public_info.acl_handle = info->acl_handle;
    public_info.local_cis_handle = info->local_cis_handle;

    if (sink_callbacks && sink_callbacks->unicast_cis_established_cb)
    {
        sink_callbacks->unicast_cis_established_cb(&public_info);
    }

    if (source_callbacks && source_callbacks->unicast_cis_established_cb)
    {
        if (!sink_callbacks || source_callbacks->unicast_cis_established_cb != sink_callbacks->unicast_cis_established_cb)
        {
            source_callbacks->unicast_cis_established_cb(&public_info);
        }
    }
}

void bk_dm_bap_internal_unicast_iso_path_ready(const bki_bap_unicast_iso_path_t *info)
{
    bk_bap_unicast_iso_path_t public_info;

    if (info == NULL)
    {
        return;
    }

    public_info.ase_id = info->ase_id;
    public_info.ase_role = info->ase_role;
    public_info.local_cis_handle = info->local_cis_handle;

    if (sink_callbacks && sink_callbacks->unicast_iso_path_ready_cb)
    {
        sink_callbacks->unicast_iso_path_ready_cb(&public_info);
    }

    if (source_callbacks && source_callbacks->unicast_iso_path_ready_cb)
    {
        if (!sink_callbacks || source_callbacks->unicast_iso_path_ready_cb != sink_callbacks->unicast_iso_path_ready_cb)
        {
            source_callbacks->unicast_iso_path_ready_cb(&public_info);
        }
    }
}

void bk_dm_bap_internal_unicast_state_changed(const bki_bap_unicast_state_t *info)
{
    bk_bap_unicast_state_t public_info;

    if (info == NULL)
    {
        return;
    }

    public_info.ase_id = info->ase_id;
    public_info.ase_role = info->ase_role;
    public_info.state = info->state;
    public_info.acl_handle = info->acl_handle;

    if (sink_callbacks && sink_callbacks->unicast_state_cb)
    {
        sink_callbacks->unicast_state_cb(&public_info);
    }

    if (source_callbacks && source_callbacks->unicast_state_cb)
    {
        if (!sink_callbacks || source_callbacks->unicast_state_cb != sink_callbacks->unicast_state_cb)
        {
            source_callbacks->unicast_state_cb(&public_info);
        }
    }
}

void bk_dm_bap_internal_unicast_ready(const bki_bap_unicast_ready_t *info)
{
    bk_bap_unicast_ready_t public_info;

    if (info == NULL)
    {
        return;
    }

    public_info.phase = info->phase;
    public_info.status = info->status;
    public_info.addr_type = info->addr_type;
    public_info.acl_handle = info->acl_handle;
    os_memcpy(public_info.addr, info->addr, sizeof(public_info.addr));

    if (sink_callbacks && sink_callbacks->unicast_ready_cb)
    {
        sink_callbacks->unicast_ready_cb(&public_info);
    }

    if (source_callbacks && source_callbacks->unicast_ready_cb)
    {
        if (!sink_callbacks || source_callbacks->unicast_ready_cb != sink_callbacks->unicast_ready_cb)
        {
            source_callbacks->unicast_ready_cb(&public_info);
        }
    }
}

uint32_t bk_dm_bap_get_channel_count(uint32_t channel_allocation)
{
    uint32_t count = 0, i;

    for (i = 0; i < 32; i++)
    {
        if ((1UL << i) & channel_allocation)
        {
            count++;
        }
    }

    return count;
}

bk_err_t bk_dm_bap_unicast_set_peer(uint8_t *addr, uint8_t addr_type)
{
    return appl_le_audio_unicast_set_peer(addr, addr_type);
}

bk_err_t bk_dm_bap_unicast_connect(uint8_t *addr, uint8_t addr_type, uint8_t extended)
{
    if (bk_dm_bap_register_gap_events() != BK_OK)
    {
        return BK_FAIL;
    }
    return appl_le_audio_unicast_connect(addr, addr_type, extended);
}

bk_err_t bk_dm_bap_unicast_adv(uint8_t enable)
{
    return bk_dm_bap_unicast_adv_set(enable);
}

bk_err_t bk_dm_bap_unicast_setup(void)
{
    return appl_le_audio_unicast_setup();
}

bk_err_t bk_dm_bap_unicast_get_capabilities(uint8_t role)
{
    return appl_le_audio_unicast_get_capabilities(role);
}

bk_err_t bk_dm_bap_unicast_discover(void)
{
    return appl_le_audio_unicast_discover();
}

bk_err_t bk_dm_bap_unicast_configure(uint8_t ase_id, uint8_t role, uint8_t cap_index)
{
    return appl_le_audio_unicast_configure(ase_id, role, cap_index);
}

bk_err_t bk_dm_bap_unicast_set_cig(uint8_t ase_id, uint8_t cig_id, uint8_t cis_id)
{
    return appl_le_audio_unicast_set_cig(ase_id, cig_id, cis_id);
}

bk_err_t bk_dm_bap_unicast_qos(uint8_t ase_id)
{
    return appl_le_audio_unicast_qos(ase_id);
}

bk_err_t bk_dm_bap_unicast_enable(uint8_t ase_id, uint16_t contexts)
{
    return appl_le_audio_unicast_enable(ase_id, contexts);
}

bk_err_t bk_dm_bap_unicast_create_cis(uint8_t ase_id)
{
    return appl_le_audio_unicast_create_cis(ase_id);
}

bk_err_t bk_dm_bap_unicast_receiver_start_ready(uint8_t ase_id)
{
    return appl_le_audio_unicast_receiver_start_ready(ase_id);
}

bk_err_t bk_dm_bap_unicast_release(uint8_t ase_id)
{
    return appl_le_audio_unicast_release(ase_id);
}

bk_err_t bk_dm_bap_unicast_remove_iso(uint16_t connection_handle, uint8_t direction)
{
    return appl_le_audio_unicast_remove_iso(connection_handle, direction);
}

bk_err_t bk_dm_bap_unicast_send(uint16_t connection_handle, uint8_t flags, uint32_t time_stamp, uint16_t sequence, uint8_t *data, uint16_t length)
{
    return appl_le_audio_unicast_send(connection_handle, flags, time_stamp, sequence, data, length);
}

bk_err_t bk_dm_bap_decode_codec_spec_config(bk_bap_codec_spec_config_t *config, uint8_t *data, uint8_t length)
{
    bk_err_t ret = BK_FAIL;
    uint8_t *ptr = data;
    uint8_t sub_len = 0;

    if (config == NULL || data == NULL)
    {
        return BK_FAIL;
    }

    while (length > 0)
    {
        uint8_t type;
        uint8_t *value;

        sub_len = *ptr++;

        /* A zero LTV length would stall the loop without advancing. */
        if (sub_len == 0 || sub_len >= length)
        {
            //LOGW("%s fail, sub_len: %d, length: %d\n", __func__, sub_len, length);
            break;
        }

        length -= sub_len + sizeof(sub_len);

        type = *ptr++;
        value = ptr;

        switch (type)
        {
            case BK_BT_AUDIO_SAMPLING_FREQUENCY:
                BK_BT_STREAM_TO_UINT8(config->sampling_frequency, ptr);
                LOGV("%s len: %d, sampling frequency: %d\n", __func__, sub_len, config->sampling_frequency);
                break;

            case BK_BT_AUDIO_FRAME_DURATION:
                BK_BT_STREAM_TO_UINT8(config->frame_duration, ptr);
                LOGV("%s len: %d, frame_duration: %d\n", __func__, sub_len, config->frame_duration);
                break;

            case BK_BT_AUDIO_CHANNEL_ALLOCATION:
                BK_BT_STREAM_TO_UINT32(config->audio_channel_allocation, ptr);
                LOGV("%s len: %d, audio channel allocation: %x\n", __func__, sub_len, config->audio_channel_allocation);
                break;

            case BK_BT_AUDIO_OCTETS_PER_CODEC_FRAME:
                BK_BT_STREAM_TO_UINT16(config->octects_per_codec_frame, ptr);
                LOGV("%s len: %d, octects per codec frame: %d\n", __func__, sub_len, config->octects_per_codec_frame);
                break;

            default:
                LOGV("%s skip unknown type: 0x%02x, len: %d\n", __func__, type, sub_len);
                break;
        }

        /* Always advance to the next LTV regardless of type/value-size, so an
         * unknown type or a known type with an unexpected value length cannot
         * desynchronize the parser. */
        ptr = value + (sub_len - 1);
        ret = BK_OK;
    }

    return ret;
}

bk_err_t bk_dm_bap_decode_basic_audio_config(bk_bap_basic_audio_config_t *config, uint8_t *data, uint16_t length)
{
    bk_err_t ret = BK_FAIL;
    int data_len = 0;
    uint8_t *ptr = NULL;
    uint8_t config_length = 0;
    uint8_t metadata_length = 0;
    uint8_t bis_index = 0;

    if (config == NULL || data == NULL || length == 0)
    {
        LOGE("%s fail\n", __func__);
        goto out;
    }

    data_len = data[0] & 0xFF;

    if (data_len >= length)
    {
        LOGE("%s invalid length: %d\n", __func__, data_len);
        goto out;
    }

    if (data[1] != BK_BT_DATA_SVC_DATA16
        || data[2] != (BK_BT_UUID_BASIC_AUDIO & 0xFF)
        || data[3] != ((BK_BT_UUID_BASIC_AUDIO >> 8) & 0xFF))
    {
        LOGE("%s invalid service: %02X, %02X%02X\n", __func__, data[1], data[3], data[2]);
        goto out;
    }

    ptr = &data[4];

    BK_BT_STREAM_TO_UINT24(config->presentation_delay, ptr);
    BK_BT_STREAM_TO_UINT8(config->num_subgroups, ptr);
    BK_BT_STREAM_TO_UINT8(config->num_bis, ptr);
    BK_BT_STREAM_TO_UINT8(config->codec_id.coding_format, ptr);
    BK_BT_STREAM_TO_UINT16(config->codec_id.company_id, ptr);
    BK_BT_STREAM_TO_UINT16(config->codec_id.vendor_spec_codec_id, ptr);

    BK_BT_STREAM_TO_UINT8(config_length, ptr);
    bk_dm_bap_decode_codec_spec_config(&config->spec_cfg, ptr, config_length);
    ptr += config_length;

    BK_BT_STREAM_TO_UINT8(metadata_length, ptr);
    //TODO
    ptr += metadata_length;

    for (bis_index = 0; (bis_index < config->num_bis) && (bis_index < BK_GAP_MAX_BIS); bis_index++)
    {
        BK_BT_STREAM_TO_UINT8(config->bis[bis_index].bis_index, ptr);
        BK_BT_STREAM_TO_UINT8(config_length, ptr);
        bk_dm_bap_decode_codec_spec_config(&config->bis[bis_index].spec_cfg, ptr, config_length);
        ptr += config_length;
    }

    LOGV("presentation delay: %d, subgroups: %d, num bis: %d\n",
         config->presentation_delay,
         config->num_subgroups,
         config->num_bis);

    LOGV("codec id: %d, commpany id: %d, vid: %d\n",
         config->codec_id.coding_format,
         config->codec_id.company_id,
         config->codec_id.vendor_spec_codec_id);

    ret = BK_OK;
out:

    return ret;
}


static inline void bk_dm_bap_announcement_sink_callback(bki_bap_source_announce_data_t *bap_source_announce_data)
{
    if (sink_callbacks
        && sink_callbacks->announcement_cb)
    {
        bk_bap_source_announce_data_t bk_bap_source_announce_data;
        os_memcpy(bk_bap_source_announce_data.address, bap_source_announce_data->address, 6);
        bk_bap_source_announce_data.advertising_sid = bap_source_announce_data->advertising_sid;
        bk_bap_source_announce_data.address_type = bap_source_announce_data->address_type;
        bk_bap_source_announce_data.rssi = bap_source_announce_data->rssi;
        bk_bap_source_announce_data.broadcast_id =
            (bap_source_announce_data->length >= 3U) ?
            ((uint32_t)bap_source_announce_data->data[0] |
             ((uint32_t)bap_source_announce_data->data[1] << 8) |
             ((uint32_t)bap_source_announce_data->data[2] << 16)) :
            0U;

        //BK_MEM_DUMP("source: \n", (uint32_t)bap_source_announcement->data, (uint32_t)bap_source_announcement->length);
        sink_callbacks->announcement_cb(&bk_bap_source_announce_data);
    }
}

static inline void bk_dm_bap_associate_data_sink_callback(bki_bap_source_associate_data_t *bap_source_associate_data)
{
    if (sink_callbacks
        && sink_callbacks->associate_cb)
    {
        bk_bap_source_associate_data_t bk_bap_source_associate_data;
        bk_bap_source_associate_data.handle = bap_source_associate_data->handle;
        sink_callbacks->associate_cb(&bk_bap_source_associate_data);
    }
}

static inline void bk_dm_bap_config_sink_callback(uint8_t *data, uint16_t length)
{
    if (sink_callbacks
        && sink_callbacks->config_cb)
    {
        sink_callbacks->config_cb(data, length);
    }

    //bk_bap_basic_audio_config_t config;
    //bk_dm_bap_decode_basic_audio_config(&config, data, length);

}

static inline void bk_dm_bap_big_info_sink_callback(bki_bap_source_big_info_t *bk_int_bap_source_big_info)
{
    if (sink_callbacks
        && sink_callbacks->big_info_cb)
    {
        bk_bap_source_big_info_t bk_bap_source_big_info;
        bk_bap_source_big_info.sync_handle = bk_int_bap_source_big_info->sync_handle;
        bk_bap_source_big_info.num_bis = bk_int_bap_source_big_info->num_bis;
        bk_bap_source_big_info.nse = bk_int_bap_source_big_info->nse;
        bk_bap_source_big_info.iso_interval = bk_int_bap_source_big_info->iso_interval;
        bk_bap_source_big_info.bn = bk_int_bap_source_big_info->bn;
        bk_bap_source_big_info.pto = bk_int_bap_source_big_info->pto;
        bk_bap_source_big_info.irc = bk_int_bap_source_big_info->irc;
        bk_bap_source_big_info.max_pdu = bk_int_bap_source_big_info->max_pdu;
        bk_bap_source_big_info.sdu_interval = bk_int_bap_source_big_info->sdu_interval;
        bk_bap_source_big_info.max_sdu = bk_int_bap_source_big_info->max_sdu;
        bk_bap_source_big_info.phy = bk_int_bap_source_big_info->phy;
        bk_bap_source_big_info.framing = bk_int_bap_source_big_info->framing;
        bk_bap_source_big_info.encryption = bk_int_bap_source_big_info->encryption;
        sink_callbacks->big_info_cb(&bk_bap_source_big_info);
    }
}

static inline void bk_dm_bap_lc3_data_sink_callback(uint8_t *header, uint8_t *data, uint32_t length)
{
    if (sink_callbacks
        && sink_callbacks->lc3_data_cb)
    {
        bk_bap_iso_header_t bk_bap_iso_header;
        uint8_t *ptr = header;
        uint16_t word0 = 0;
        uint16_t word1 = 0;
        uint16_t seq = 0;
        uint16_t sdu = 0;

        os_memset(&bk_bap_iso_header, 0, sizeof(bk_bap_iso_header));

        /* Parse the HCI ISO Data packet header explicitly, byte by byte, to
         * avoid relying on bit-field layout / endianness / alignment of the
         * receive buffer (the previous uint32 type-pun was UB). */
        BK_BT_STREAM_TO_UINT16(word0, ptr);
        BK_BT_STREAM_TO_UINT16(word1, ptr);

        bk_bap_iso_header.connection_handle = word0 & 0x0FFF;
        bk_bap_iso_header.pb_flag           = (word0 >> 12) & 0x03;
        bk_bap_iso_header.ts_flag           = (word0 >> 14) & 0x01;
        bk_bap_iso_header.rfu_1             = (word0 >> 15) & 0x01;
        bk_bap_iso_header.data_total_length = word1 & 0x3FFF;
        bk_bap_iso_header.rfu_2             = (word1 >> 14) & 0x03;

        if (bk_bap_iso_header.ts_flag)
        {
            BK_BT_STREAM_TO_UINT32(bk_bap_iso_header.time_stamp, ptr);
        }
        else
        {
            bk_bap_iso_header.time_stamp = 0;
        }

        BK_BT_STREAM_TO_UINT16(seq, ptr);
        BK_BT_STREAM_TO_UINT16(sdu, ptr);

        bk_bap_iso_header.packet_sequence_number = seq;
        bk_bap_iso_header.iso_sdu_length         = sdu & 0x0FFF;
        bk_bap_iso_header.rfu_3                   = (sdu >> 12) & 0x03;
        bk_bap_iso_header.packet_status_flag      = (sdu >> 14) & 0x03;

        sink_callbacks->lc3_data_cb(&bk_bap_iso_header,data, length);
    }
}

static inline void bk_dm_bap_broadcast_event_sink_cb(uint32_t event, uint32_t status)
{
    if (sink_callbacks
        && sink_callbacks->broadcast_event_cb)
    {
        bk_bap_sink_cb_evt_t sink_cb_evt = BK_BAP_SINK_INVALID;

        switch (event)
        {
            case BKI_BAP_SINK_EVT_SACN_END:
                sink_cb_evt = BK_BAP_SINK_SCAN_END;
            break;

            case BKI_BAP_SINK_EVT_DISABLE_IND:
                sink_cb_evt = BK_BAP_SINK_DISABLE_IND;
            break;

            case BKI_BAP_SINK_EVT_DISABLE_CNF:
                sink_cb_evt = BK_BAP_SINK_DISABLE_CNF;
            break;

            case BKI_BAP_SINK_EVT_ENABLE_CNF:
                sink_cb_evt = BK_BAP_SINK_ENABLE_CNF;
            break;

            case BKI_BAP_SINK_EVT_DISSOCIATE_CNF:
                sink_cb_evt = BK_BAP_SINK_DISSOCIATE_CNF;
            break;

        }

        sink_callbacks->broadcast_event_cb(sink_cb_evt, status);
    }
}

static inline void bk_dm_bap_setup_announcement_source_cb(uint32_t status, void *paramters)
{
    if (source_callbacks
        && source_callbacks->setup_announcement_cb)
    {
        source_callbacks->setup_announcement_cb(status, paramters);
    }
}

static inline void bk_dm_bap_end_announcement_source_cb(uint32_t status, void *paramters)
{
    if (source_callbacks
        && source_callbacks->end_announcement_cb)
    {
        source_callbacks->end_announcement_cb(status, paramters);
    }
}


static inline void bk_dm_bap_broadcast_start_source_cb(bki_bap_boradcast_paramters_t *bki_bap_boradcast_paramters)
{
    if (source_callbacks
        && source_callbacks->broadcast_start_cb)
    {
        bk_bap_boradcast_paramters_t bk_bap_boradcast_paramters;
        uint8_t num_bis = bki_bap_boradcast_paramters->num_bis;
        uint8_t i;

        if (num_bis > BK_GAP_MAX_BIS)
        {
            num_bis = BK_GAP_MAX_BIS;
        }

        bk_bap_boradcast_paramters.error_code = bki_bap_boradcast_paramters->error_code;
        bk_bap_boradcast_paramters.big_handle = bki_bap_boradcast_paramters->big_handle;
        bk_bap_boradcast_paramters.nse = bki_bap_boradcast_paramters->nse;
        bk_bap_boradcast_paramters.num_bis = num_bis;

        for (i = 0; i < num_bis; i++)
        {
            bk_bap_boradcast_paramters.connection_handle[i] = bki_bap_boradcast_paramters->connection_handle[i];
        }

        source_callbacks->broadcast_start_cb(&bk_bap_boradcast_paramters);
    }
}

static inline void bk_dm_bap_broadcast_suspend_source_cb(uint8_t handle, uint8_t reason)
{
    if (source_callbacks
        && source_callbacks->broadcast_suspend_cb)
    {
        source_callbacks->broadcast_suspend_cb(handle, reason);
    }
}

bk_err_t bk_dm_bap_broadcast_scan_start(void)
{
    bk_err_t ret = BK_FAIL;

    ret = appl_le_audio_broadcast_scan_start();

    return ret;
}

bk_err_t bk_dm_bap_broadcast_scan_stop(void)
{
    bk_err_t ret = BK_FAIL;

    ret = appl_le_audio_broadcast_scan_stop();

    return ret;
}

bk_err_t bk_dm_bap_broadcast_associate(bk_bap_source_announce_data_t *bk_bap_source_announce_data)
{
    bk_err_t ret = BK_FAIL;

    bki_bap_source_announce_data_t bap_source_announce_data;

    os_memcpy(bap_source_announce_data.address, bk_bap_source_announce_data->address, 6);
    bap_source_announce_data.advertising_sid = bk_bap_source_announce_data->advertising_sid;
    bap_source_announce_data.address_type = bk_bap_source_announce_data->address_type;
    bap_source_announce_data.rssi = bk_bap_source_announce_data->rssi;

    ret = appl_le_audio_broadcast_associate(&bap_source_announce_data);

    return ret;
}

bk_err_t bk_dm_bap_broadcast_dissociate(uint16_t sync_handle)
{
    bk_err_t ret = BK_FAIL;

    ret = appl_le_audio_broadcast_dissociate(sync_handle);

    return ret;
}


bk_err_t bk_dm_bap_broadcast_enable(uint16_t handle, uint8_t *code, uint8_t bis_count, uint8_t *bis)
{
    bk_err_t ret = BK_FAIL;

    ret = appl_le_audio_broadcast_enable(handle, code, bis_count, bis);

    return ret;
}

bk_err_t bk_dm_bap_broadcast_disable(uint16_t handle)
{
    bk_err_t ret = BK_FAIL;

    ret = appl_le_audio_broadcast_disable(handle);

    return ret;
}


bk_err_t bk_dm_bap_broadcast_alloc_session(uint8_t *session)
{
    bk_err_t ret = BK_FAIL;

    ret = appl_le_audio_broadcast_alloc_session(session);

    return ret;
}

bk_err_t bk_dm_bap_broadcast_free_session(uint8_t session)
{
    bk_err_t ret = BK_FAIL;

    ret = appl_le_audio_broadcast_free_session(session);

    return ret;
}

bk_err_t bk_dm_bap_broadcast_configure_session(uint8_t session, uint8_t phy, uint8_t packing, uint8_t *broadcast_codes)
{
    bk_err_t ret = BK_FAIL;

    ret = appl_le_audio_broadcast_configure_session(session, phy, packing, broadcast_codes);

    return ret;
}

bk_err_t bk_dm_bap_broadcast_sep_register(uint8_t session,
                                          bk_bap_codec_info_t *codec,
                                          bk_bap_medadata_t *meta,
                                          uint8_t nstream,
                                          bk_bap_codec_ie_t *stream,
                                          uint8_t *sep)
{
    bk_err_t ret = BK_FAIL;

    bki_bap_codec_info_t bk_int_bap_codec_info;
    bki_bap_medadata_t bk_int_bap_medadata;
    bki_bap_codec_ie_t bk_int_bap_codec_ie[BK_GAP_MAX_BIS];
    uint8_t i;

    if (codec == NULL || meta == NULL || stream == NULL || nstream == 0)
    {
        return BK_FAIL;
    }

    if (nstream > BK_GAP_MAX_BIS)
    {
        LOGW("%s nstream %d clamped to %d\n", __func__, nstream, BK_GAP_MAX_BIS);
        nstream = BK_GAP_MAX_BIS;
    }

    os_memcpy(&bk_int_bap_codec_info, codec, sizeof(bk_bap_codec_info_t));
    os_memcpy(&bk_int_bap_medadata, meta, sizeof(bk_bap_medadata_t));

    /* Copy every stream element, not just the first one. */
    for (i = 0; i < nstream; i++)
    {
        os_memcpy(&bk_int_bap_codec_ie[i], &stream[i], sizeof(bk_bap_codec_ie_t));
    }

    ret = appl_le_audio_broadcast_sep_register(session, &bk_int_bap_codec_info, &bk_int_bap_medadata, nstream, bk_int_bap_codec_ie, sep);

    return ret;
}

bk_err_t bk_dm_bap_setup_announcement(uint8_t session, uint32_t broadcast_id, uint8_t type, uint32_t presentation_delay)
{
    bk_err_t ret = BK_FAIL;

    ret = appl_le_audio_broadcast_setup_announcement(session, broadcast_id, type, presentation_delay);

    return ret;
}

bk_err_t bk_dm_bap_end_announcement(uint8_t session)
{
    bk_err_t ret = BK_FAIL;

    ret = appl_le_audio_broadcast_end_announcement(session);

    return ret;
}


bk_err_t bk_dm_bap_broadcast_start(uint8_t session,
                                   uint32_t sdu_interval,
                                   uint16_t max_sdu,
                                   uint16_t max_latency,
                                   uint8_t rtn,
                                   uint8_t framing)
{
    bk_err_t ret = BK_FAIL;

    ret = appl_le_audio_broadcast_start(session, sdu_interval, max_sdu, max_latency, rtn, framing);

    return ret;
}

bk_err_t bk_dm_bap_broadcast_suspend(uint8_t session)
{
    bk_err_t ret = BK_FAIL;

    ret = appl_le_audio_broadcast_suspend(session);

    return ret;
}

bk_err_t bk_dm_bap_broadcast_data_send(uint16_t connection_handle, uint8_t flags, uint32_t time_stamp, uint16_t sequence, uint8_t *data, uint16_t length)
{
    bk_err_t ret = BK_FAIL;

    ret = appl_le_audio_broadcast_data_send(connection_handle, flags, time_stamp, sequence, data, length);

    return ret;
}

void bk_dm_bap_create_codec_spec_conf_ltv(bk_bap_lc3_codec_specific_conf_t *bk_bap_lc3_codec_specific_conf,
                                          uint8_t *ltvarray, uint8_t *ltvarray_len)
{
    bki_bap_lc3_codec_specific_conf_t  bk_int_bap_lc3_codec_specific_conf;

    os_memcpy(&bk_int_bap_lc3_codec_specific_conf, bk_bap_lc3_codec_specific_conf, sizeof(bki_bap_lc3_codec_specific_conf_t));

    appl_le_audio_broadcast_create_codec_spec_conf_ltv(&bk_int_bap_lc3_codec_specific_conf, ltvarray, ltvarray_len);
}

void bk_dm_bap_create_metadata_ltv(uint8_t *ltvarray, uint8_t *ltvarray_len)
{
    appl_le_audio_broadcast_create_metadata_ltv(ltvarray, ltvarray_len);
}

static bki_bap_sink_callbacks_t internal_sink_callbacks =
{
    .announcement_cb = bk_dm_bap_announcement_sink_callback,
    .associate_cb = bk_dm_bap_associate_data_sink_callback,
    .config_cb = bk_dm_bap_config_sink_callback,
    .lc3_data_cb = bk_dm_bap_lc3_data_sink_callback,
    .big_info_cb = bk_dm_bap_big_info_sink_callback,
    .broadcast_event_cb = bk_dm_bap_broadcast_event_sink_cb,
};

static bki_bap_source_callbacks_t internal_source_callbacks =
{
    .setup_announcement_cb = bk_dm_bap_setup_announcement_source_cb,
    .end_announcement_cb = bk_dm_bap_end_announcement_source_cb,
    .broadcast_start_cb = bk_dm_bap_broadcast_start_source_cb,
    .broadcast_suspend_cb = bk_dm_bap_broadcast_suspend_source_cb,
};


bk_err_t bk_dm_bap_init(void)
{
    bk_err_t ret = BK_FAIL;

    ret = appl_le_audio_ga_init();
    LOGI("%s appl_le_audio_ga_init ret=%d\n", __func__, ret);

    return ret;
}

bk_err_t bk_dm_bap_pacs_register(uint8_t role, const bk_bap_pacs_cfg_t *pacs_cfg)
{
    bk_err_t ret = BK_FAIL;

    if (role == BK_GAP_ROLE_SINK)
    {
        bk_dm_bap_copy_pacs_cfg(&s_bap_sink_pacs_cfg, pacs_cfg);
        ret = appl_le_audio_ga_pacs_register(BK_GAP_ROLE_SINK, &s_bap_sink_pacs_cfg);
        LOGI("%s pacs role=sink ret=%d\n", __func__, ret);
        return ret;
    }
    if (role == BK_GAP_ROLE_SOURCE)
    {
        bk_dm_bap_copy_pacs_cfg(&s_bap_source_pacs_cfg, pacs_cfg);
        ret = appl_le_audio_ga_pacs_register(BK_GAP_ROLE_SOURCE, &s_bap_source_pacs_cfg);
        LOGI("%s pacs role=source ret=%d\n", __func__, ret);
        return ret;
    }

    return BK_ERR_PARAM;
}

bk_err_t bk_dm_bap_ascs_register(uint8_t role, const bk_bap_ascs_cfg_t *ascs_cfg)
{
    bk_err_t ret;

    ret = bk_dm_bap_register_gap_events();
    if (ret != BK_OK)
    {
        return ret;
    }

    if (role == BK_GAP_ROLE_SINK)
    {
        bk_dm_bap_copy_ascs_cfg(&s_bap_sink_ascs_cfg, ascs_cfg);
        s_bap_sink_registered = 1U;
        return BK_OK;
    }
    if (role == BK_GAP_ROLE_SOURCE)
    {
        bk_dm_bap_copy_ascs_cfg(&s_bap_source_ascs_cfg, ascs_cfg);
        s_bap_source_registered = 1U;
        return BK_OK;
    }

    return BK_ERR_PARAM;
}

bk_err_t bk_dm_bap_sink_register(const bk_bap_sink_callbacks_t *bk_bap_sink_callbacks)
{
    bk_err_t ret;

    sink_callbacks = bk_bap_sink_callbacks;
    ret = appl_le_audio_ga_sink_register(&internal_sink_callbacks);
    LOGI("%s role callback ret=%d\n", __func__, ret);
    return ret;
}

bk_err_t bk_dm_bap_source_register(const bk_bap_source_callbacks_t *bk_bap_source_callbacks)
{
    bk_err_t ret;

    source_callbacks = bk_bap_source_callbacks;
    ret = appl_le_audio_ga_source_register(&internal_source_callbacks);
    LOGI("%s role callback ret=%d\n", __func__, ret);
    return ret;
}
