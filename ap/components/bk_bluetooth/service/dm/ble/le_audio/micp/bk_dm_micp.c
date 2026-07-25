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

#include "bk_internal_dm_ble_micp.h"
#include <components/bluetooth/bk_dm_bluetooth_types.h>
#include <components/bluetooth/bk_dm_micp.h>

#define TAG "micp"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

static const bk_micp_controller_callbacks_t *s_ctrl_cbs = NULL;
static const bk_micp_device_callbacks_t *s_dev_cbs = NULL;

/* ---- up-calls from appl_micp.c GA callbacks ---- */

void bk_dm_micp_internal_setup(uint16_t acl_handle, uint8_t status)
{
    if (s_ctrl_cbs && s_ctrl_cbs->on_setup)
    {
        s_ctrl_cbs->on_setup(acl_handle, status);
    }
}

void bk_dm_micp_internal_mute_state(uint16_t acl_handle, uint8_t mute)
{
    if (s_ctrl_cbs && s_ctrl_cbs->on_mute_state)
    {
        s_ctrl_cbs->on_mute_state(acl_handle, mute);
    }
}

void bk_dm_micp_internal_aics_state(uint16_t acl_handle, int8_t gain, uint8_t mute, uint8_t gain_mode)
{
    if (s_ctrl_cbs && s_ctrl_cbs->on_aics_state)
    {
        s_ctrl_cbs->on_aics_state(acl_handle, gain, mute, gain_mode);
    }
}

void bk_dm_micp_internal_cp_done(uint16_t acl_handle, uint8_t status)
{
    if (s_ctrl_cbs && s_ctrl_cbs->event_cb)
    {
        s_ctrl_cbs->event_cb(acl_handle, BK_MICP_EVT_CP_DONE, status);
    }
}

void bk_dm_micp_internal_device_mute_set(uint16_t acl_handle, uint8_t mute)
{
    if (s_dev_cbs && s_dev_cbs->on_mute_set)
    {
        s_dev_cbs->on_mute_set(acl_handle, mute);
    }
}

void bk_dm_micp_internal_device_aics_gain_set(uint16_t acl_handle, int8_t gain)
{
    if (s_dev_cbs && s_dev_cbs->on_aics_gain_set)
    {
        s_dev_cbs->on_aics_gain_set(acl_handle, gain);
    }
}

/* ---- public API ---- */

bk_err_t bk_dm_micp_init(void)
{
    uint16_t ret;

    LOGI("%s\n", __func__);
    ret = appl_micp_dev_reg_opt_service();
    if (ret != BK_OK)
    {
        LOGE("register MICP AICS failed 0x%x\n", ret);
        return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t bk_dm_micp_controller_register(const bk_micp_controller_callbacks_t *callbacks)
{
    s_ctrl_cbs = callbacks;
    LOGI("%s\n", __func__);
    return BK_OK;
}

bk_err_t bk_dm_micp_device_register(const bk_micp_device_callbacks_t *callbacks)
{
    s_dev_cbs = callbacks;
    LOGI("%s\n", __func__);
    return BK_OK;
}

bk_err_t bk_dm_micp_discover(uint8_t *addr, uint8_t addr_type)
{
    if (addr == NULL)
    {
        return BK_ERR_PARAM;
    }
    return appl_le_audio_micp_discover(addr, addr_type);
}

bk_err_t bk_dm_micp_get_capabilities(uint16_t acl_handle)
{
    (void)acl_handle;
    return appl_le_audio_micp_get_capabilities();
}

bk_err_t bk_dm_micp_read_mute(uint16_t acl_handle)
{
    (void)acl_handle;
    return appl_le_audio_micp_read_mute();
}

bk_err_t bk_dm_micp_set_mute(uint16_t acl_handle, uint8_t mute)
{
    (void)acl_handle;
    return appl_le_audio_micp_set_mute(mute);
}

bk_err_t bk_dm_micp_aics_set_gain(uint16_t acl_handle, int8_t gain)
{
    (void)acl_handle;
    return appl_le_audio_micp_aics_set_gain(gain);
}

bk_err_t bk_dm_micp_release(uint16_t acl_handle)
{
    (void)acl_handle;
    return appl_le_audio_micp_release();
}

bk_err_t bk_dm_micp_device_set_mute(uint8_t mute)
{
    return appl_le_audio_micp_device_set_mute(mute);
}
