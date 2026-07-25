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

#include "bk_internal_dm_ble_ccp.h"
#include <components/bluetooth/bk_dm_bluetooth_types.h>
#include <components/bluetooth/bk_dm_ccp.h>

#define TAG "ccp"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

static const bk_ccp_client_callbacks_t *s_client_cbs = NULL;
static const bk_ccp_server_callbacks_t *s_server_cbs = NULL;

/* ---- up-calls from appl_ccp_ce.c / appl_ccp_se.c GA callbacks ---- */

void bk_dm_ccp_internal_setup(uint16_t acl_handle, uint8_t status)
{
    if (s_client_cbs && s_client_cbs->on_setup)
    {
        s_client_cbs->on_setup(acl_handle, status);
    }
}

void bk_dm_ccp_internal_call_state(uint16_t acl_handle, uint8_t call_index, uint8_t state)
{
    if (s_client_cbs && s_client_cbs->on_call_state)
    {
        s_client_cbs->on_call_state(acl_handle, call_index, state);
    }
}

void bk_dm_ccp_internal_incoming_call(uint16_t acl_handle, uint8_t call_index, const char *uri, uint16_t len)
{
    if (s_client_cbs && s_client_cbs->on_incoming_call)
    {
        s_client_cbs->on_incoming_call(acl_handle, call_index, uri, len);
    }
}

void bk_dm_ccp_internal_cp_done(uint16_t acl_handle, uint8_t status)
{
    if (s_client_cbs && s_client_cbs->event_cb)
    {
        s_client_cbs->event_cb(acl_handle, BK_CCP_EVT_CP_DONE, status);
    }
}

void bk_dm_ccp_internal_server_control(uint16_t acl_handle, uint8_t opcode, uint8_t call_index)
{
    if (s_server_cbs && s_server_cbs->on_control)
    {
        s_server_cbs->on_control(acl_handle, opcode, call_index);
    }
}

/* ---- public API ---- */

bk_err_t bk_dm_ccp_init(void)
{
    /* Server (GTBS) GATT service is registered on demand via
     * bk_dm_ccp_server_init(); the dynamic GATT DB is committed automatically
     * when a server role first advertises. Here we only bring up the client
     * entity, which adds no GATT service. */
    LOGI("%s\n", __func__);
    return (appl_le_audio_ccp_client_init() == 0) ? BK_OK : BK_FAIL;
}

bk_err_t bk_dm_ccp_client_register(const bk_ccp_client_callbacks_t *callbacks)
{
    s_client_cbs = callbacks;
    LOGI("%s\n", __func__);
    return BK_OK;
}

bk_err_t bk_dm_ccp_server_register(const bk_ccp_server_callbacks_t *callbacks)
{
    s_server_cbs = callbacks;
    LOGI("%s\n", __func__);
    return BK_OK;
}

bk_err_t bk_dm_ccp_server_init(void)
{
    appl_le_audio_ccp_server_init();
    return BK_OK;
}

bk_err_t bk_dm_ccp_discover(uint8_t *addr, uint8_t addr_type)
{
    if (addr == NULL)
    {
        return BK_ERR_PARAM;
    }
    return appl_le_audio_ccp_discover(addr, addr_type);
}

bk_err_t bk_dm_ccp_accept(uint16_t acl_handle, uint8_t call_index)
{
    (void)acl_handle;
    return appl_le_audio_ccp_accept(call_index);
}

bk_err_t bk_dm_ccp_terminate(uint16_t acl_handle, uint8_t call_index)
{
    (void)acl_handle;
    return appl_le_audio_ccp_terminate(call_index);
}

bk_err_t bk_dm_ccp_hold(uint16_t acl_handle, uint8_t call_index)
{
    (void)acl_handle;
    return appl_le_audio_ccp_local_hold(call_index);
}

bk_err_t bk_dm_ccp_retrieve(uint16_t acl_handle, uint8_t call_index)
{
    (void)acl_handle;
    return appl_le_audio_ccp_local_retrieve(call_index);
}

bk_err_t bk_dm_ccp_originate(uint16_t acl_handle, const char *uri)
{
    (void)acl_handle;
    if (uri == NULL)
    {
        return BK_ERR_PARAM;
    }
    return appl_le_audio_ccp_originate(uri);
}

bk_err_t bk_dm_ccp_read_call_state(uint16_t acl_handle)
{
    (void)acl_handle;
    return appl_le_audio_ccp_read_call_state();
}

bk_err_t bk_dm_ccp_server_incoming_call(void)
{
    return appl_le_audio_ccp_server_incoming_call();
}
