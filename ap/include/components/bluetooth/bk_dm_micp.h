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
#include "bk_dm_micp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LE Audio Microphone Control Profile (MICP) adapter.
 *
 * Follows the bk_dm_bap adapter style (init + role register + typed callbacks).
 *
 * Controller operations require an existing ACL/GATT connection to the peer
 * (e.g. set up via the unicast client connect path). The current adapter
 * tracks a single peer context; acl_handle is carried in the public API for
 * forward compatibility.
 */

bk_err_t bk_dm_micp_init(void);
bk_err_t bk_dm_micp_controller_register(const bk_micp_controller_callbacks_t *callbacks);
bk_err_t bk_dm_micp_device_register(const bk_micp_device_callbacks_t *callbacks);

/* ---- Controller (Microphone Controller / GATT client) ---- */
bk_err_t bk_dm_micp_discover(uint8_t *addr, uint8_t addr_type);
bk_err_t bk_dm_micp_get_capabilities(uint16_t acl_handle);
bk_err_t bk_dm_micp_read_mute(uint16_t acl_handle);
bk_err_t bk_dm_micp_set_mute(uint16_t acl_handle, uint8_t mute); /* BK_MICP_UNMUTE/MUTE/MUTE_DISABLED */
bk_err_t bk_dm_micp_aics_set_gain(uint16_t acl_handle, int8_t gain);
bk_err_t bk_dm_micp_release(uint16_t acl_handle);

/* ---- Device (Microphone Device / GATT server) ---- */
bk_err_t bk_dm_micp_device_set_mute(uint8_t mute);

#ifdef __cplusplus
}
#endif
