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

#include <stdint.h>

#include <components/bluetooth/bk_dm_bluetooth_types.h>
#include <components/bluetooth/bk_dm_gap_ble.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Common LE Audio GAP helper.
 *
 * LE Audio roles that need connectable advertising (unicast server, scan
 * delegator, ...) share this helper instead of duplicating GAP callback
 * registration and advertising bring-up in every bk_dm_* profile. Advertising,
 * the resulting ACL connection and GATT are all kept on the GA/EtherMind stack:
 * advertising is issued through the GA HCI helper and ACL connect/disconnect are
 * delivered by the official GA transport events (TRANSPORT_UP/DOWN_IND), so the
 * advertising/connection data plane stays in the application/adapter layer.
 */

/* Connection state notification. connected=1 on link-up, 0 on link-down. */
typedef void (*bk_dm_le_audio_gap_conn_cb_t)(uint8_t connected,
                                             uint8_t status,
                                             uint8_t addr_type,
                                             const uint8_t addr[6],
                                             uint16_t acl_handle,
                                             void *ctx);

/* Register a connection callback. Brings up the shared GA transport hook on
 * first use. Multiple profiles may register; each is fanned out on
 * connect/disconnect. */
bk_err_t bk_dm_le_audio_gap_register_conn_callback(bk_dm_le_audio_gap_conn_cb_t cb, void *ctx);

/* Pre-seal hook: invoked once, right before the LE Audio GATT DB is committed
 * (on the first advertise). A profile registers a hook to add every service it
 * must expose while the ordering across profiles is still under its control -
 * e.g. BAP registers all PACS records eagerly, then adds its ASCS ASEs from the
 * hook so PACS and ASCS end up with contiguous, disjoint handle ranges. */
typedef void (*bk_dm_le_audio_gap_pre_seal_cb_t)(void *ctx);

bk_err_t bk_dm_le_audio_gap_register_pre_seal_callback(bk_dm_le_audio_gap_pre_seal_cb_t cb, void *ctx);

/* Configure + enable one connectable advertising set (raw AD payload). */
bk_err_t bk_dm_le_audio_gap_adv_start(uint8_t handle,
                                      const bk_ble_gap_ext_adv_params_t *params,
                                      const uint8_t *adv_data,
                                      uint8_t adv_len,
                                      const uint8_t *scan_rsp,
                                      uint8_t scan_rsp_len);

/* Stop the advertising set started with bk_dm_le_audio_gap_adv_start(). */
bk_err_t bk_dm_le_audio_gap_adv_stop(uint8_t handle);

#ifdef __cplusplus
}
#endif
