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

#include "bk_dm_le_audio_gap.h"
#include "bk_internal_dm_ble_gap.h"

#include <common/bk_include.h>
#include <components/log.h>

#define TAG "lea_gap"
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define LE_AUDIO_GAP_MAX_CONN_CBS 4
#define LE_AUDIO_GAP_MAX_PRE_SEAL_CBS 4

/*
 * GA (EtherMind) native GAP backend.
 *
 * LE Audio keeps advertising, the resulting ACL connection and GATT all on the
 * one GA/EtherMind stack instead of the separate BK GAP layer (bk_ble_gap):
 * advertising is issued through the GA HCI helper, and ACL connect/disconnect
 * events are delivered by the GA HCI path via a hook.
 */

typedef struct
{
    bk_dm_le_audio_gap_conn_cb_t cb;
    void *ctx;
} bk_dm_le_audio_gap_conn_listener_t;

typedef struct
{
    bk_dm_le_audio_gap_pre_seal_cb_t cb;
    void *ctx;
} bk_dm_le_audio_gap_pre_seal_listener_t;

static uint8_t s_hook_registered = 0U;
static uint8_t s_gatt_db_sealed = 0U;
static bk_dm_le_audio_gap_conn_listener_t s_conn_cbs[LE_AUDIO_GAP_MAX_CONN_CBS];
static bk_dm_le_audio_gap_pre_seal_listener_t s_pre_seal_cbs[LE_AUDIO_GAP_MAX_PRE_SEAL_CBS];

/* Commit the dynamic GATT DB the first time a server role advertises. By then
 * every server registered at init (PACS/ASCS via bap sink/source register plus
 * any enabled control-profile server) is in place, so a single commit makes
 * them all discoverable. A service added at runtime is committed by re-running
 * the same commit. */
static void bk_dm_le_audio_gap_seal_gatt_db(void)
{
    uint8_t i;

    if (s_gatt_db_sealed)
    {
        return;
    }

    /* Let profiles add any services that must be in the DB before it is
     * committed (e.g. BAP ASCS ASEs, added after all PACS records). */
    for (i = 0U; i < LE_AUDIO_GAP_MAX_PRE_SEAL_CBS; i++)
    {
        if (s_pre_seal_cbs[i].cb != NULL)
        {
            s_pre_seal_cbs[i].cb(s_pre_seal_cbs[i].ctx);
        }
    }

    (void)appl_le_audio_ga_gatt_db_register();
    s_gatt_db_sealed = 1U;
}

bk_err_t bk_dm_le_audio_gap_register_pre_seal_callback(bk_dm_le_audio_gap_pre_seal_cb_t cb, void *ctx)
{
    uint8_t i;

    if (cb == NULL)
    {
        return BK_ERR_PARAM;
    }

    for (i = 0U; i < LE_AUDIO_GAP_MAX_PRE_SEAL_CBS; i++)
    {
        if (s_pre_seal_cbs[i].cb == cb)
        {
            s_pre_seal_cbs[i].ctx = ctx;
            return BK_OK;
        }
    }

    for (i = 0U; i < LE_AUDIO_GAP_MAX_PRE_SEAL_CBS; i++)
    {
        if (s_pre_seal_cbs[i].cb == NULL)
        {
            s_pre_seal_cbs[i].cb = cb;
            s_pre_seal_cbs[i].ctx = ctx;
            return BK_OK;
        }
    }

    return BK_FAIL;
}

/* Official GA transport-event target: fan every ACL link up/down out to the
 * registered profile listeners. Runs in the GA/EtherMind context. The GA
 * transport event carries only the peer address (no status/ACL handle), so
 * status is reported as success and the ACL handle as 0. */
static void bk_dm_le_audio_gap_on_transport(uint8_t connected,
                                            uint8_t addr_type,
                                            const uint8_t *addr)
{
    uint8_t i;

    for (i = 0U; i < LE_AUDIO_GAP_MAX_CONN_CBS; i++)
    {
        if (s_conn_cbs[i].cb != NULL)
        {
            s_conn_cbs[i].cb(connected, BK_OK, addr_type, addr, 0U, s_conn_cbs[i].ctx);
        }
    }
}

static bk_err_t bk_dm_le_audio_gap_prepare(void)
{
    if (!s_hook_registered)
    {
        appl_le_audio_register_transport_cb(bk_dm_le_audio_gap_on_transport);
        s_hook_registered = 1U;
    }

    return BK_OK;
}

bk_err_t bk_dm_le_audio_gap_register_conn_callback(bk_dm_le_audio_gap_conn_cb_t cb, void *ctx)
{
    uint8_t i;

    if (cb == NULL)
    {
        return BK_ERR_PARAM;
    }
    if (bk_dm_le_audio_gap_prepare() != BK_OK)
    {
        return BK_FAIL;
    }

    for (i = 0U; i < LE_AUDIO_GAP_MAX_CONN_CBS; i++)
    {
        if (s_conn_cbs[i].cb == cb)
        {
            s_conn_cbs[i].ctx = ctx;
            return BK_OK;
        }
    }

    for (i = 0U; i < LE_AUDIO_GAP_MAX_CONN_CBS; i++)
    {
        if (s_conn_cbs[i].cb == NULL)
        {
            s_conn_cbs[i].cb = cb;
            s_conn_cbs[i].ctx = ctx;
            return BK_OK;
        }
    }

    return BK_FAIL;
}

bk_err_t bk_dm_le_audio_gap_adv_start(uint8_t handle,
                                      const bk_ble_gap_ext_adv_params_t *params,
                                      const uint8_t *adv_data,
                                      uint8_t adv_len,
                                      const uint8_t *scan_rsp,
                                      uint8_t scan_rsp_len)
{
    if (params == NULL || adv_data == NULL || adv_len == 0U)
    {
        return BK_ERR_PARAM;
    }
    if ((scan_rsp == NULL) && (scan_rsp_len != 0U))
    {
        return BK_ERR_PARAM;
    }
    if (bk_dm_le_audio_gap_prepare() != BK_OK)
    {
        return BK_FAIL;
    }

    /* Commit the GATT DB before this server first becomes connectable, so a
     * peer that connects can discover every registered LE Audio service. */
    bk_dm_le_audio_gap_seal_gatt_db();

    if (appl_le_audio_gap_adv(1U, handle, adv_data, adv_len,
                              (uint16_t)params->interval_min,
                              (uint16_t)params->interval_max) != 0U)
    {
        LOGE("ga adv start failed\n");
        return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t bk_dm_le_audio_gap_adv_stop(uint8_t handle)
{
    if (appl_le_audio_gap_adv(0U, handle, NULL, 0U, 0U, 0U) != 0U)
    {
        LOGE("ga adv stop failed\n");
        return BK_FAIL;
    }

    return BK_OK;
}
