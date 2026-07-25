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
#include "bk_internal_dm_ble_bass.h"
#include <components/bluetooth/bk_dm_bluetooth_types.h>
#include <components/bluetooth/bk_dm_gap_ble.h>
#include <components/bluetooth/bk_dm_bass.h>

#define TAG "bass"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

static const bk_bass_client_callbacks_t *s_client_cbs = NULL;
static const bk_bass_server_callbacks_t *s_server_cbs = NULL;

#define BASS_DELEG_ADV_HANDLE        0U
#define BASS_DELEG_UUID              0x184FU
#define BASS_DELEG_ADV_INTERVAL_MIN  0x0030U
#define BASS_DELEG_ADV_INTERVAL_MAX  0x0050U

static const uint8_t s_deleg_short_name[] = "BK-BASS";
static const uint8_t s_deleg_full_name[] = "BK7259 Scan Delegator";
static uint8_t s_deleg_adv_started = 0U;

static void bk_dm_bass_build_delegator_adv(uint8_t *adv_data, uint8_t *adv_len,
        uint8_t *scan_rsp, uint8_t *scan_rsp_len)
{
    uint8_t pos = 0;
    uint8_t sr_pos = 0;

    adv_data[pos++] = 0x02; /* Flags */
    adv_data[pos++] = 0x01;
    adv_data[pos++] = 0x06; /* LE General Discoverable + BR/EDR Not Supported */

    adv_data[pos++] = 0x03; /* Complete 16-bit UUID list: BASS */
    adv_data[pos++] = 0x03;
    adv_data[pos++] = (uint8_t)(BASS_DELEG_UUID & 0xFFU);
    adv_data[pos++] = (uint8_t)((BASS_DELEG_UUID >> 8U) & 0xFFU);

    adv_data[pos++] = (uint8_t)(1U + sizeof(s_deleg_short_name) - 1U);
    adv_data[pos++] = 0x09; /* Complete Local Name */
    os_memcpy(&adv_data[pos], s_deleg_short_name, sizeof(s_deleg_short_name) - 1U);
    pos += (uint8_t)(sizeof(s_deleg_short_name) - 1U);
    *adv_len = pos;

    scan_rsp[sr_pos++] = (uint8_t)(1U + sizeof(s_deleg_full_name) - 1U);
    scan_rsp[sr_pos++] = 0x09;
    os_memcpy(&scan_rsp[sr_pos], s_deleg_full_name, sizeof(s_deleg_full_name) - 1U);
    sr_pos += (uint8_t)(sizeof(s_deleg_full_name) - 1U);
    *scan_rsp_len = sr_pos;
}

/* ---- up-calls from appl_bass.c GA callbacks ---- */

void bk_dm_bass_internal_setup(uint16_t acl_handle, uint8_t status)
{
    if (s_client_cbs && s_client_cbs->on_setup)
    {
        s_client_cbs->on_setup(acl_handle, status);
    }
}

void bk_dm_bass_internal_rx_state(uint16_t acl_handle, const uint8_t *data, uint16_t len)
{
    if (s_client_cbs && s_client_cbs->on_rx_state)
    {
        s_client_cbs->on_rx_state(acl_handle, data, len);
    }
}

void bk_dm_bass_internal_cp_done(uint16_t acl_handle, uint8_t status)
{
    if (s_client_cbs && s_client_cbs->event_cb)
    {
        s_client_cbs->event_cb(acl_handle, BK_BASS_EVT_CP_DONE, status);
    }
}

void bk_dm_bass_internal_server_control(uint16_t acl_handle, uint8_t opcode)
{
    if (s_server_cbs && s_server_cbs->on_control)
    {
        s_server_cbs->on_control(acl_handle, opcode);
    }
}

void bk_dm_bass_internal_server_add_source(uint16_t acl_handle, uint8_t source_id,
        uint8_t addr_type, const uint8_t *addr, uint8_t adv_sid, uint32_t broadcast_id,
        uint8_t pa_sync, uint16_t pa_interval, uint8_t num_subgroups, uint32_t bis_sync)
{
    bk_bass_source_info_t info;

    if (!s_server_cbs || !s_server_cbs->on_add_source)
    {
        return;
    }

    os_memset(&info, 0, sizeof(info));
    info.source_id     = source_id;
    info.addr_type     = addr_type;
    if (addr)
    {
        os_memcpy(info.addr, addr, sizeof(info.addr));
    }
    info.adv_sid       = adv_sid;
    info.broadcast_id  = broadcast_id;
    info.pa_sync       = pa_sync;
    info.pa_interval   = pa_interval;
    info.num_subgroups = num_subgroups;
    info.bis_sync      = bis_sync;

    s_server_cbs->on_add_source(acl_handle, &info);
}

void bk_dm_bass_internal_server_set_broadcast_code(uint16_t acl_handle, uint8_t source_id,
        const uint8_t *code)
{
    if (s_server_cbs && s_server_cbs->on_set_broadcast_code)
    {
        s_server_cbs->on_set_broadcast_code(acl_handle, source_id, code);
    }
}

void bk_dm_bass_internal_server_remove_source(uint16_t acl_handle, uint8_t source_id)
{
    if (s_server_cbs && s_server_cbs->on_remove_source)
    {
        s_server_cbs->on_remove_source(acl_handle, source_id);
    }
}

/* ---- Scan Delegator Broadcast Receive State ---- */

#define BK_DM_BASS_SE_RX_STATE_MAX_LEN   64U
#define BK_DM_BASS_BD_ADDR_SIZE          6U

#define BK_DM_BASS_CP_ADD_SOURCE           0x02U
#define BK_DM_BASS_CP_MODIFY_SOURCE        0x03U
#define BK_DM_BASS_CP_REMOVE_SOURCE        0x05U
#define BK_DM_BASS_PA_SYNC_NOT_SYNC        0x00U
#define BK_DM_BASS_PA_SYNC_INFO_REQUEST    0x01U
#define BK_DM_BASS_BIG_NOT_ENCRYPTED       0x00U

static uint8_t  s_se_rx_state[BK_DM_BASS_SE_RX_STATE_MAX_LEN];
static uint16_t s_se_rx_state_len;
static uint8_t  s_se_src_id_next = 1U;
static uint8_t  s_se_src_added;

void bk_dm_bass_internal_se_reset(void)
{
    s_se_rx_state_len = 0U;
    s_se_src_id_next  = 1U;
    s_se_src_added    = 0U;
}

uint16_t bk_dm_bass_internal_se_get_rx_state(const uint8_t **data)
{
    if (data != NULL)
    {
        *data = s_se_rx_state;
    }

    return s_se_rx_state_len;
}

static void bk_dm_bass_se_build_rx_state(const uint8_t *cp, uint16_t cp_len, uint8_t add_src)
{
    uint16_t i;
    uint16_t o;
    uint8_t  num_sg;
    uint8_t  sg;

    if ((cp == NULL) || (cp_len < 6U))
    {
        return;
    }

    i = 1U;

    if (add_src != 0U)
    {
        o = 0U;
        s_se_rx_state[o++] = s_se_src_id_next;
        s_se_rx_state[o++] = cp[i++];
        os_memcpy(&s_se_rx_state[o], &cp[i], BK_DM_BASS_BD_ADDR_SIZE);
        o += BK_DM_BASS_BD_ADDR_SIZE;
        i += BK_DM_BASS_BD_ADDR_SIZE;
        s_se_rx_state[o++] = cp[i++];
        os_memcpy(&s_se_rx_state[o], &cp[i], 3U);
        o += 3U;
        i += 3U;
    }
    else
    {
        s_se_rx_state[0U] = cp[i++];
        o = 12U;
    }

    s_se_rx_state[o++] = (cp[i] != 0U) ? BK_DM_BASS_PA_SYNC_INFO_REQUEST
                                       : BK_DM_BASS_PA_SYNC_NOT_SYNC;
    i++;
    i += 2U;

    s_se_rx_state[o++] = BK_DM_BASS_BIG_NOT_ENCRYPTED;

    num_sg = cp[i++];
    s_se_rx_state[o++] = num_sg;
    for (sg = 0U; sg < num_sg; sg++)
    {
        uint8_t md_len;

        if (((uint16_t)(i + 5U) > cp_len) ||
            ((uint16_t)(o + 5U) > BK_DM_BASS_SE_RX_STATE_MAX_LEN))
        {
            break;
        }
        os_memcpy(&s_se_rx_state[o], &cp[i], 4U);
        o += 4U;
        i += 4U;

        md_len = cp[i++];
        if ((uint16_t)(i + md_len) > cp_len)
        {
            md_len = (uint8_t)(cp_len - i);
        }
        if ((uint16_t)(o + 1U + md_len) > BK_DM_BASS_SE_RX_STATE_MAX_LEN)
        {
            md_len = (uint8_t)(BK_DM_BASS_SE_RX_STATE_MAX_LEN - o - 1U);
        }
        s_se_rx_state[o++] = md_len;
        os_memcpy(&s_se_rx_state[o], &cp[i], md_len);
        o += md_len;
        i += md_len;
    }

    s_se_rx_state_len = o;
}

void bk_dm_bass_internal_se_handle_cp(uint16_t acl_handle, uint8_t opcode,
                                      const uint8_t *cp, uint16_t cp_len)
{
    bk_dm_bass_internal_server_control(acl_handle, opcode);

    switch (opcode)
    {
    case BK_DM_BASS_CP_ADD_SOURCE:
        bk_dm_bass_se_build_rx_state(cp, cp_len, 1U);
        s_se_src_added = 1U;
        (void)appl_le_audio_bass_server_notify();
        if (cp_len >= 16U)
        {
            uint32_t bcast_id = (uint32_t)cp[9] | ((uint32_t)cp[10] << 8) |
                                ((uint32_t)cp[11] << 16);
            uint16_t pa_intv  = (uint16_t)cp[13] | ((uint16_t)cp[14] << 8);
            uint32_t bis_sync = 0U;

            if (cp_len >= 20U)
            {
                bis_sync = (uint32_t)cp[16] | ((uint32_t)cp[17] << 8) |
                           ((uint32_t)cp[18] << 16) | ((uint32_t)cp[19] << 24);
            }
            bk_dm_bass_internal_server_add_source(acl_handle,
                s_se_rx_state[0U], cp[1], &cp[2], cp[8],
                bcast_id, cp[12], pa_intv, cp[15], bis_sync);
        }
        break;

    case BK_DM_BASS_CP_MODIFY_SOURCE:
        if (s_se_src_added != 0U)
        {
            bk_dm_bass_se_build_rx_state(cp, cp_len, 0U);
            (void)appl_le_audio_bass_server_notify();
        }
        break;

    case BK_DM_BASS_CP_REMOVE_SOURCE:
    {
        uint8_t rem_src_id = (cp_len >= 2U) ? cp[1] : 0U;

        s_se_rx_state_len = 0U;
        s_se_src_added    = 0U;
        s_se_src_id_next++;
        (void)appl_le_audio_bass_server_notify();
        bk_dm_bass_internal_server_remove_source(acl_handle, rem_src_id);
        break;
    }

    case 0x04U:
        if (cp_len >= 18U)
        {
            bk_dm_bass_internal_server_set_broadcast_code(acl_handle, cp[1], &cp[2]);
        }
        break;

    default:
        break;
    }
}

uint32_t bk_dm_bass_internal_se_set_pa_state(uint8_t pa_sync_state)
{
    if (s_se_rx_state_len >= 13U)
    {
        s_se_rx_state[12U] = pa_sync_state;
        return appl_le_audio_bass_server_notify();
    }

    return 0U;
}

/* ---- public API ---- */

bk_err_t bk_dm_bass_init(void)
{
    /* Server (Scan Delegator) GATT service is registered on demand via
     * bk_dm_bass_server_init(); the dynamic GATT DB is committed automatically
     * when a server role first advertises. Here we only bring up the client
     * entity, which adds no GATT service. */
    LOGI("%s\n", __func__);
    return (appl_le_audio_bass_client_init() == 0) ? BK_OK : BK_FAIL;
}

bk_err_t bk_dm_bass_client_register(const bk_bass_client_callbacks_t *callbacks)
{
    s_client_cbs = callbacks;
    LOGI("%s\n", __func__);
    return BK_OK;
}

bk_err_t bk_dm_bass_server_register(const bk_bass_server_callbacks_t *callbacks)
{
    s_server_cbs = callbacks;
    LOGI("%s\n", __func__);
    return BK_OK;
}

bk_err_t bk_dm_bass_server_init(void)
{
    appl_le_audio_bass_server_init();
    return BK_OK;
}

bk_err_t bk_dm_bass_discover(uint8_t *addr, uint8_t addr_type)
{
    if (addr == NULL)
    {
        return BK_ERR_PARAM;
    }
    return appl_le_audio_bass_discover(addr, addr_type);
}

bk_err_t bk_dm_bass_read_rx_state(uint16_t acl_handle)
{
    (void)acl_handle;
    return appl_le_audio_bass_read_rx_state();
}

bk_err_t bk_dm_bass_scan_start(uint16_t acl_handle)
{
    (void)acl_handle;
    return appl_le_audio_bass_scan(1U);
}

bk_err_t bk_dm_bass_scan_stop(uint16_t acl_handle)
{
    (void)acl_handle;
    return appl_le_audio_bass_scan(0U);
}

bk_err_t bk_dm_bass_add_source(uint16_t acl_handle, uint8_t *src_addr, uint8_t src_addr_type, uint8_t adv_sid)
{
    (void)acl_handle;
    if (src_addr == NULL)
    {
        return BK_ERR_PARAM;
    }
    return appl_le_audio_bass_add_source(src_addr, src_addr_type, adv_sid);
}

bk_err_t bk_dm_bass_add_source_ex(uint16_t acl_handle, uint8_t *src_addr, uint8_t src_addr_type,
                                  uint8_t adv_sid, uint32_t broadcast_id, uint32_t bis_sync)
{
    (void)acl_handle;
    if (src_addr == NULL)
    {
        return BK_ERR_PARAM;
    }
    return appl_le_audio_bass_add_source_ex(src_addr, src_addr_type, adv_sid, broadcast_id, bis_sync);
}

bk_err_t bk_dm_bass_set_broadcast_code(uint16_t acl_handle, uint8_t source_id, const uint8_t *code)
{
    (void)acl_handle;
    if (code == NULL)
    {
        return BK_ERR_PARAM;
    }
    return appl_le_audio_bass_set_broadcast_code(source_id, code);
}

bk_err_t bk_dm_bass_remove_source(uint16_t acl_handle, uint8_t source_id)
{
    (void)acl_handle;
    return appl_le_audio_bass_remove_source(source_id);
}

bk_err_t bk_dm_bass_server_notify_rx_state(void)
{
    return appl_le_audio_bass_server_notify();
}

bk_err_t bk_dm_bass_delegator_adv(uint8_t enable)
{
    uint8_t adv_data[31];
    uint8_t adv_len = 0;
    uint8_t scan_rsp[31];
    uint8_t scan_rsp_len = 0;
    bk_ble_gap_ext_adv_params_t adv_param;
    bk_err_t ret;

    if (!enable)
    {
        if (s_deleg_adv_started)
        {
            ret = bk_dm_le_audio_gap_adv_stop(BASS_DELEG_ADV_HANDLE);
            if (ret != BK_OK)
            {
                return ret;
            }
            s_deleg_adv_started = 0U;
        }
        return BK_OK;
    }

    os_memset(&adv_param, 0, sizeof(adv_param));
    adv_param.type = BK_BLE_GAP_SET_EXT_ADV_PROP_LEGACY_IND;
    adv_param.interval_min = BASS_DELEG_ADV_INTERVAL_MIN;
    adv_param.interval_max = BASS_DELEG_ADV_INTERVAL_MAX;
    adv_param.channel_map = BK_ADV_CHNL_ALL;
    adv_param.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
    adv_param.filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;
    adv_param.primary_phy = BK_BLE_GAP_PRI_PHY_1M;
    adv_param.secondary_phy = BK_BLE_GAP_PHY_1M;
    adv_param.sid = 0U;
    adv_param.scan_req_notif = 0U;
    adv_param.tx_power = 0x7FU;

    bk_dm_bass_build_delegator_adv(adv_data, &adv_len, scan_rsp, &scan_rsp_len);

    ret = bk_dm_le_audio_gap_adv_start(BASS_DELEG_ADV_HANDLE, &adv_param,
                                       adv_data, adv_len, scan_rsp, scan_rsp_len);
    if (ret != BK_OK)
    {
        return ret;
    }

    s_deleg_adv_started = 1U;
    return BK_OK;
}

bk_err_t bk_dm_bass_delegator_setup_past(void)
{
    return (appl_le_audio_bass_sd_setup_default_past() == 0) ? BK_OK : BK_FAIL;
}

bk_err_t bk_dm_bass_delegator_set_pa_state(uint8_t pa_sync_state)
{
    return (bk_dm_bass_internal_se_set_pa_state(pa_sync_state) == 0U) ? BK_OK : BK_FAIL;
}

bk_err_t bk_dm_bass_assistant_send_past(uint8_t *deleg_addr, uint8_t deleg_addr_type, uint16_t sync_handle)
{
    if (deleg_addr == NULL)
    {
        return BK_ERR_PARAM;
    }
    return (appl_le_audio_bass_ba_send_past(deleg_addr, deleg_addr_type, sync_handle) == 0) ? BK_OK : BK_FAIL;
}
