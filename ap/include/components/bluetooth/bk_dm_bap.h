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
#include "bk_dm_bap_types.h"

#ifdef __cplusplus
extern "C" {
#endif

bk_err_t bk_dm_bap_init(void);
bk_err_t bk_dm_bap_sink_register(const bk_bap_sink_callbacks_t *bk_bap_sink_callbacks);
bk_err_t bk_dm_bap_source_register(const bk_bap_source_callbacks_t *bk_bap_source_callbacks);

uint32_t bk_dm_bap_get_channel_count(uint32_t channel_allocation);

/**
 *
 * @brief
 *  LE Audio unicast/CIS debug helpers. Thin wrappers around the EtherMind GA
 *  reference flow for step-by-step ACL + PACS/ASCS + CIS bring-up.
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  - others: fail
 *
 */
bk_err_t bk_dm_bap_unicast_set_peer(uint8_t *addr, uint8_t addr_type);
bk_err_t bk_dm_bap_unicast_connect(uint8_t *addr, uint8_t addr_type, uint8_t extended);
bk_err_t bk_dm_bap_unicast_adv(uint8_t enable);
bk_err_t bk_dm_bap_unicast_setup(void);
bk_err_t bk_dm_bap_unicast_get_capabilities(uint8_t role);
bk_err_t bk_dm_bap_unicast_discover(void);
bk_err_t bk_dm_bap_unicast_configure(uint8_t ase_id, uint8_t role, uint8_t cap_index);
bk_err_t bk_dm_bap_unicast_set_cig(uint8_t ase_id, uint8_t cig_id, uint8_t cis_id);
bk_err_t bk_dm_bap_unicast_qos(uint8_t ase_id);
bk_err_t bk_dm_bap_unicast_enable(uint8_t ase_id, uint16_t contexts);
bk_err_t bk_dm_bap_unicast_create_cis(uint8_t ase_id);
bk_err_t bk_dm_bap_unicast_receiver_start_ready(uint8_t ase_id);
bk_err_t bk_dm_bap_unicast_release(uint8_t ase_id);
bk_err_t bk_dm_bap_unicast_remove_iso(uint16_t connection_handle, uint8_t direction);
bk_err_t bk_dm_bap_unicast_send(uint16_t connection_handle, uint8_t flags, uint32_t time_stamp, uint16_t sequence, uint8_t *data, uint16_t length);

/**
 *
 * @brief
 *  This routine sets up xxxxxxxx from Broadcast sink device
 *  to the selected source of Broadcast Audio decide.
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  - others: fail
 *
 */
bk_err_t bk_dm_bap_broadcast_scan_start(void);
bk_err_t bk_dm_bap_broadcast_scan_stop(void);
bk_err_t bk_dm_bap_broadcast_associate(bk_bap_source_announce_data_t *bk_bap_source_announce_data);
bk_err_t bk_dm_bap_broadcast_dissociate(uint16_t sync_handle);
bk_err_t bk_dm_bap_broadcast_enable(uint16_t handle, uint8_t *code, uint8_t bis_count, uint8_t *bis);
bk_err_t bk_dm_bap_broadcast_disable(uint16_t handle);
bk_err_t bk_dm_bap_decode_basic_audio_config(bk_bap_basic_audio_config_t *config, uint8_t *data, uint16_t length);


/**
 *
 * @brief
 *  This routine sets up xxxxxxxx from Broadcast source device
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  - others: fail
 *
 */
bk_err_t bk_dm_bap_broadcast_alloc_session(uint8_t *session);
bk_err_t bk_dm_bap_broadcast_free_session(uint8_t session);
bk_err_t bk_dm_bap_broadcast_configure_session(uint8_t session, uint8_t phy, uint8_t packing, uint8_t *broadcast_codes);
bk_err_t bk_dm_bap_broadcast_sep_register(uint8_t session,
                                          bk_bap_codec_info_t *codec,
                                          bk_bap_medadata_t *meta,
                                          uint8_t nstream,
                                          bk_bap_codec_ie_t *stream,
                                          uint8_t *sep);
bk_err_t bk_dm_bap_setup_announcement(uint8_t session, uint32_t broadcast_id, uint8_t type, uint32_t presentation_delay);
bk_err_t bk_dm_bap_end_announcement(uint8_t session);
bk_err_t bk_dm_bap_broadcast_start(uint8_t session,
                                   uint32_t sdu_interval,
                                   uint16_t max_sdu,
                                   uint16_t max_latency,
                                   uint8_t rtn,
                                   uint8_t framing);
bk_err_t bk_dm_bap_broadcast_suspend(uint8_t session);
bk_err_t bk_dm_bap_broadcast_data_send(uint16_t connection_handle, uint8_t flags, uint32_t time_stamp, uint16_t sequence, uint8_t *data, uint16_t length);

void bk_dm_bap_create_codec_spec_conf_ltv(bk_bap_lc3_codec_specific_conf_t *bk_bap_lc3_codec_specific_conf,
                                          uint8_t *ltvarray, uint8_t *ltvarray_len);
void bk_dm_bap_create_metadata_ltv(uint8_t *ltvarray, uint8_t *ltvarray_len);

#ifdef __cplusplus
}
#endif

