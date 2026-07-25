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

#pragma once

#include "bk_dm_bluetooth_types.h"
#include "bk_dm_vcp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LE Audio Volume Control Profile (VCP) adapter.
 *
 * Follows the bk_dm_bap adapter style (init + role register + typed callbacks).
 *
 * Controller operations require an existing ACL/GATT connection to the peer
 * (e.g. set up via the unicast client connect path). The current adapter
 * tracks a single peer context; acl_handle is carried in the public API for
 * forward compatibility.
 */

bk_err_t bk_dm_vcp_init(void);
bk_err_t bk_dm_vcp_controller_register(const bk_vcp_controller_callbacks_t *callbacks);
bk_err_t bk_dm_vcp_renderer_register(const bk_vcp_renderer_callbacks_t *callbacks);

/* ---- Controller (VCP Controller / GATT client) ---- */
bk_err_t bk_dm_vcp_discover(uint8_t *addr, uint8_t addr_type);
bk_err_t bk_dm_vcp_get_capabilities(uint16_t acl_handle);
bk_err_t bk_dm_vcp_read_volume_state(uint16_t acl_handle);
bk_err_t bk_dm_vcp_set_abs_volume(uint16_t acl_handle, uint8_t volume);
bk_err_t bk_dm_vcp_volume_up(uint16_t acl_handle, uint8_t unmute);
bk_err_t bk_dm_vcp_volume_down(uint16_t acl_handle, uint8_t unmute);
bk_err_t bk_dm_vcp_set_mute(uint16_t acl_handle, uint8_t mute);
bk_err_t bk_dm_vcp_vocs_set_offset(uint16_t acl_handle, int16_t offset);
bk_err_t bk_dm_vcp_aics_set_gain(uint16_t acl_handle, int8_t gain);
bk_err_t bk_dm_vcp_release(uint16_t acl_handle);

/* ---- Renderer (VCP Renderer / GATT server) ---- */
bk_err_t bk_dm_vcp_renderer_set_volume(uint8_t volume);
bk_err_t bk_dm_vcp_renderer_set_mute(uint8_t mute);

#ifdef __cplusplus
}
#endif
