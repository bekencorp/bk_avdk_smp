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

#include "bk_internal_dm_ble_csip.h"
#include <components/bluetooth/bk_dm_bluetooth_types.h>
#include <components/bluetooth/bk_dm_csip.h>

#define TAG "csip"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

static const bk_csip_coordinator_callbacks_t *s_coord_cbs = NULL;
static const bk_csip_member_callbacks_t *s_member_cbs = NULL;

/* ---- up-calls from appl_csip.c GA callbacks ---- */

void bk_dm_csip_internal_setup(uint16_t acl_handle, uint8_t status)
{
    if (s_coord_cbs && s_coord_cbs->on_setup)
    {
        s_coord_cbs->on_setup(acl_handle, status);
    }
    if (s_coord_cbs && s_coord_cbs->event_cb)
    {
        s_coord_cbs->event_cb(acl_handle, BK_CSIP_EVT_SETUP, status);
    }
}

void bk_dm_csip_internal_sirk(uint16_t acl_handle, uint8_t type, const uint8_t *value)
{
    bk_csip_sirk_t sirk;

    if (NULL == value)
    {
        return;
    }

    sirk.type = type;
    os_memcpy(sirk.value, value, BK_CSIP_SIRK_LEN);

    if (s_coord_cbs && s_coord_cbs->on_sirk)
    {
        s_coord_cbs->on_sirk(acl_handle, &sirk);
    }
}

void bk_dm_csip_internal_setsize(uint16_t acl_handle, uint8_t size)
{
    if (s_coord_cbs && s_coord_cbs->on_setsize)
    {
        s_coord_cbs->on_setsize(acl_handle, size);
    }
}

void bk_dm_csip_internal_rank(uint16_t acl_handle, uint8_t rank)
{
    if (s_coord_cbs && s_coord_cbs->on_rank)
    {
        s_coord_cbs->on_rank(acl_handle, rank);
    }
}

void bk_dm_csip_internal_lock_state(uint16_t acl_handle, uint8_t lock)
{
    if (s_coord_cbs && s_coord_cbs->on_lock_state)
    {
        s_coord_cbs->on_lock_state(acl_handle, lock);
    }
}

void bk_dm_csip_internal_cp_done(uint16_t acl_handle, uint8_t status)
{
    if (s_coord_cbs && s_coord_cbs->event_cb)
    {
        s_coord_cbs->event_cb(acl_handle, BK_CSIP_EVT_LOCK_DONE, status);
    }
}

void bk_dm_csip_internal_released(uint16_t acl_handle, uint8_t status)
{
    if (s_coord_cbs && s_coord_cbs->event_cb)
    {
        s_coord_cbs->event_cb(acl_handle, BK_CSIP_EVT_RELEASED, status);
    }
}

void bk_dm_csip_internal_member_lock_set(uint16_t acl_handle, uint8_t lock)
{
    if (s_member_cbs && s_member_cbs->on_lock_set)
    {
        s_member_cbs->on_lock_set(acl_handle, lock);
    }
}

/* ---- public API ---- */

bk_err_t bk_dm_csip_init(void)
{
    /* CSIP is initialized as part of the GA stack init (the CSIP client/server
     * callbacks are registered during appl_le_audio_ga_init). Nothing
     * additional to do here; kept for API symmetry with other profiles. */
    LOGI("%s\n", __func__);
    return BK_OK;
}

bk_err_t bk_dm_csip_coordinator_register(const bk_csip_coordinator_callbacks_t *callbacks)
{
    s_coord_cbs = callbacks;
    LOGI("%s\n", __func__);
    return BK_OK;
}

bk_err_t bk_dm_csip_member_register(const bk_csip_member_callbacks_t *callbacks)
{
    s_member_cbs = callbacks;
    LOGI("%s\n", __func__);
    return BK_OK;
}

bk_err_t bk_dm_csip_discover(uint8_t *addr, uint8_t addr_type)
{
    if (addr == NULL)
    {
        return BK_ERR_PARAM;
    }
    return appl_le_audio_csip_discover(addr, addr_type);
}

bk_err_t bk_dm_csip_get_sirk(uint16_t acl_handle)
{
    (void)acl_handle;
    return appl_le_audio_csip_get_sirk();
}

bk_err_t bk_dm_csip_get_setsize(uint16_t acl_handle)
{
    (void)acl_handle;
    return appl_le_audio_csip_get_setsize();
}

bk_err_t bk_dm_csip_get_rank(uint16_t acl_handle)
{
    (void)acl_handle;
    return appl_le_audio_csip_get_rank();
}

bk_err_t bk_dm_csip_get_lock(uint16_t acl_handle)
{
    (void)acl_handle;
    return appl_le_audio_csip_get_lock();
}

bk_err_t bk_dm_csip_set_lock(uint16_t acl_handle, uint8_t lock)
{
    (void)acl_handle;
    return appl_le_audio_csip_set_lock(lock);
}

bk_err_t bk_dm_csip_release(uint16_t acl_handle)
{
    (void)acl_handle;
    return appl_le_audio_csip_release();
}

bk_err_t bk_dm_csip_member_configure(const bk_csip_sirk_t *sirk, uint8_t size, uint8_t rank, uint8_t lock)
{
    if (sirk == NULL)
    {
        return BK_ERR_PARAM;
    }
    return appl_le_audio_csip_member_configure(sirk->type, sirk->value, size, rank, lock);
}
