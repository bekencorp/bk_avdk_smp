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

#include "bk_internal_dm_ble_mcp.h"
#include <components/bluetooth/bk_dm_bluetooth_types.h>
#include <components/bluetooth/bk_dm_mcp.h>

#define TAG "mcp"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

static const bk_mcp_client_callbacks_t *s_client_cbs = NULL;
static const bk_mcp_server_callbacks_t *s_server_cbs = NULL;

/* ---- up-calls from appl_mcp_ce.c / appl_mcp_se.c GA callbacks ---- */

void bk_dm_mcp_internal_setup(uint16_t acl_handle, uint8_t status)
{
    if (s_client_cbs && s_client_cbs->on_setup)
    {
        s_client_cbs->on_setup(acl_handle, status);
    }
}

void bk_dm_mcp_internal_track_title(uint16_t acl_handle, const char *title, uint16_t len)
{
    if (s_client_cbs && s_client_cbs->on_track_title)
    {
        s_client_cbs->on_track_title(acl_handle, title, len);
    }
}

void bk_dm_mcp_internal_media_state(uint16_t acl_handle, uint8_t state)
{
    if (s_client_cbs && s_client_cbs->on_media_state)
    {
        s_client_cbs->on_media_state(acl_handle, state);
    }
}

void bk_dm_mcp_internal_track_position(uint16_t acl_handle, int32_t position)
{
    if (s_client_cbs && s_client_cbs->on_track_position)
    {
        s_client_cbs->on_track_position(acl_handle, position);
    }
}

void bk_dm_mcp_internal_cp_done(uint16_t acl_handle, uint8_t status)
{
    if (s_client_cbs && s_client_cbs->event_cb)
    {
        s_client_cbs->event_cb(acl_handle, BK_MCP_EVT_CP_DONE, status);
    }
}

void bk_dm_mcp_internal_notify_cfg(uint16_t acl_handle, uint8_t status)
{
    if (s_client_cbs && s_client_cbs->event_cb)
    {
        s_client_cbs->event_cb(acl_handle, BK_MCP_EVT_NOTIFY_CFG, status);
    }
}

void bk_dm_mcp_internal_server_control(uint16_t acl_handle, uint8_t opcode)
{
    if (s_server_cbs && s_server_cbs->on_control)
    {
        s_server_cbs->on_control(acl_handle, opcode);
    }
}

/* ---- public API ---- */

bk_err_t bk_dm_mcp_init(void)
{
    /* Server (GMCS) GATT service is registered on demand via
     * bk_dm_mcp_server_init(); the dynamic GATT DB is committed automatically
     * when a server role first advertises. Here we only bring up the client
     * entity, which adds no GATT service. */
    LOGI("%s\n", __func__);
    return (appl_le_audio_mcp_client_init() == 0) ? BK_OK : BK_FAIL;
}

bk_err_t bk_dm_mcp_client_register(const bk_mcp_client_callbacks_t *callbacks)
{
    s_client_cbs = callbacks;
    LOGI("%s\n", __func__);
    return BK_OK;
}

bk_err_t bk_dm_mcp_server_register(const bk_mcp_server_callbacks_t *callbacks)
{
    s_server_cbs = callbacks;
    LOGI("%s\n", __func__);
    return BK_OK;
}

bk_err_t bk_dm_mcp_server_init(void)
{
    appl_le_audio_mcp_server_init();
    return BK_OK;
}

bk_err_t bk_dm_mcp_discover(uint8_t *addr, uint8_t addr_type)
{
    if (addr == NULL)
    {
        return BK_ERR_PARAM;
    }
    return appl_le_audio_mcp_discover(addr, addr_type);
}

bk_err_t bk_dm_mcp_config_notify(uint16_t acl_handle, uint8_t enable)
{
    (void)acl_handle;
    return appl_le_audio_mcp_config_notify(enable);
}

bk_err_t bk_dm_mcp_control(uint16_t acl_handle, uint8_t opcode)
{
    (void)acl_handle;
    return appl_le_audio_mcp_control(opcode);
}

bk_err_t bk_dm_mcp_read_track_title(uint16_t acl_handle)
{
    (void)acl_handle;
    return appl_le_audio_mcp_read_track_title();
}

bk_err_t bk_dm_mcp_read_media_state(uint16_t acl_handle)
{
    (void)acl_handle;
    return appl_le_audio_mcp_read_media_state();
}

bk_err_t bk_dm_mcp_read_track_position(uint16_t acl_handle)
{
    (void)acl_handle;
    return appl_le_audio_mcp_read_track_position();
}

bk_err_t bk_dm_mcp_server_set_media_state(uint8_t state)
{
    return appl_le_audio_mcp_server_set_media_state(state);
}

bk_err_t bk_dm_mcp_server_set_track_title(const char *title)
{
    if (title == NULL)
    {
        return BK_ERR_PARAM;
    }
    return appl_le_audio_mcp_server_set_track_title(title);
}

bk_err_t bk_dm_mcp_server_set_track_position(int32_t position)
{
    return appl_le_audio_mcp_server_set_track_position(position);
}
