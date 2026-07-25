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

#include "bk_internal_dm_ble_vcp.h"
#include <components/bluetooth/bk_dm_bluetooth_types.h>
#include <components/bluetooth/bk_dm_vcp.h>

#define TAG "vcp"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define VCP_SERVICE_TYPE_VOCS 0x00U
#define VCP_SERVICE_TYPE_AICS 0x01U

static const bk_vcp_controller_callbacks_t *s_ctrl_cbs = NULL;
static const bk_vcp_renderer_callbacks_t *s_rndr_cbs = NULL;

/* ---- up-calls from appl_vcp.c GA callbacks ---- */

void bk_dm_vcp_internal_setup(uint16_t acl_handle, uint8_t status)
{
    if (s_ctrl_cbs && s_ctrl_cbs->on_setup)
    {
        s_ctrl_cbs->on_setup(acl_handle, status);
    }
}

void bk_dm_vcp_internal_volume_state(uint16_t acl_handle, const bki_vcp_volume_state_t *state)
{
    bk_vcp_volume_state_t public_state;

    if (state == NULL)
    {
        return;
    }

    public_state.volume = state->volume;
    public_state.mute = state->mute;
    public_state.change_counter = state->change_counter;

    if (s_ctrl_cbs && s_ctrl_cbs->on_volume_state)
    {
        s_ctrl_cbs->on_volume_state(acl_handle, &public_state);
    }
}

void bk_dm_vcp_internal_vocs_offset(uint16_t acl_handle, int16_t offset)
{
    if (s_ctrl_cbs && s_ctrl_cbs->on_vocs_offset)
    {
        s_ctrl_cbs->on_vocs_offset(acl_handle, offset);
    }
}

void bk_dm_vcp_internal_aics_state(uint16_t acl_handle, int8_t gain, uint8_t mute, uint8_t gain_mode)
{
    if (s_ctrl_cbs && s_ctrl_cbs->on_aics_state)
    {
        s_ctrl_cbs->on_aics_state(acl_handle, gain, mute, gain_mode);
    }
}

void bk_dm_vcp_internal_cp_done(uint16_t acl_handle, uint8_t status)
{
    if (s_ctrl_cbs && s_ctrl_cbs->event_cb)
    {
        s_ctrl_cbs->event_cb(acl_handle, BK_VCP_EVT_CP_DONE, status);
    }
}

void bk_dm_vcp_internal_renderer_volume_set(uint16_t acl_handle, uint8_t volume, uint8_t mute)
{
    if (s_rndr_cbs && s_rndr_cbs->on_volume_set)
    {
        s_rndr_cbs->on_volume_set(acl_handle, volume, mute);
    }
}

/* ---- public API ---- */

bk_err_t bk_dm_vcp_init(void)
{
    uint16_t ret;

    LOGI("%s\n", __func__);
    ret = appl_vcp_rd_reg_opt_service(VCP_SERVICE_TYPE_VOCS);
    if (ret != BK_OK)
    {
        LOGE("register VOCS failed 0x%x\n", ret);
        return BK_FAIL;
    }

    ret = appl_vcp_rd_reg_opt_service(VCP_SERVICE_TYPE_AICS);
    if (ret != BK_OK)
    {
        LOGE("register VCP AICS failed 0x%x\n", ret);
        return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t bk_dm_vcp_controller_register(const bk_vcp_controller_callbacks_t *callbacks)
{
    s_ctrl_cbs = callbacks;
    LOGI("%s\n", __func__);
    return BK_OK;
}

bk_err_t bk_dm_vcp_renderer_register(const bk_vcp_renderer_callbacks_t *callbacks)
{
    s_rndr_cbs = callbacks;
    LOGI("%s\n", __func__);
    return BK_OK;
}

bk_err_t bk_dm_vcp_discover(uint8_t *addr, uint8_t addr_type)
{
    if (addr == NULL)
    {
        return BK_ERR_PARAM;
    }
    return appl_le_audio_vcp_discover(addr, addr_type);
}

bk_err_t bk_dm_vcp_get_capabilities(uint16_t acl_handle)
{
    (void)acl_handle;
    return appl_le_audio_vcp_get_capabilities();
}

bk_err_t bk_dm_vcp_read_volume_state(uint16_t acl_handle)
{
    (void)acl_handle;
    return appl_le_audio_vcp_read_volume_state();
}

bk_err_t bk_dm_vcp_set_abs_volume(uint16_t acl_handle, uint8_t volume)
{
    (void)acl_handle;
    return appl_le_audio_vcp_set_abs_volume(volume);
}

bk_err_t bk_dm_vcp_volume_up(uint16_t acl_handle, uint8_t unmute)
{
    (void)acl_handle;
    return appl_le_audio_vcp_volume_up(unmute);
}

bk_err_t bk_dm_vcp_volume_down(uint16_t acl_handle, uint8_t unmute)
{
    (void)acl_handle;
    return appl_le_audio_vcp_volume_down(unmute);
}

bk_err_t bk_dm_vcp_set_mute(uint16_t acl_handle, uint8_t mute)
{
    (void)acl_handle;
    return appl_le_audio_vcp_set_mute(mute);
}

bk_err_t bk_dm_vcp_vocs_set_offset(uint16_t acl_handle, int16_t offset)
{
    (void)acl_handle;
    return appl_le_audio_vcp_vocs_set_offset(offset);
}

bk_err_t bk_dm_vcp_aics_set_gain(uint16_t acl_handle, int8_t gain)
{
    (void)acl_handle;
    return appl_le_audio_vcp_aics_set_gain(gain);
}

bk_err_t bk_dm_vcp_release(uint16_t acl_handle)
{
    (void)acl_handle;
    return appl_le_audio_vcp_release();
}

bk_err_t bk_dm_vcp_renderer_set_volume(uint8_t volume)
{
    return appl_le_audio_vcp_renderer_set_volume(volume);
}

bk_err_t bk_dm_vcp_renderer_set_mute(uint8_t mute)
{
    return appl_le_audio_vcp_renderer_set_mute(mute);
}
