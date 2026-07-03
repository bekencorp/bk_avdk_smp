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

#ifdef __cplusplus
extern "C" {
#endif

#define BK_GAP_ROLE_SINK            0x01U
#define BK_GAP_ROLE_SOURCE          0x02U

#define BK_GAP_DEFAULT_CONTEXTS     0x04U
#define BK_GAP_DEFAULT_AES_COUNT    0x02U

#define BK_GAP_MAX_BIS              (2)

#define BK_BAP_CODEC_IE_LEN         24U
#define BK_BAP_METADATA_LEN         24U

typedef enum {
    BK_BAP_SINK_INVALID = 0,
    BK_BAP_SINK_SCAN_END,
    BK_BAP_SINK_DISABLE_IND,
    BK_BAP_SINK_DISABLE_CNF,
    BK_BAP_SINK_ENABLE_CNF,
} bk_bap_sink_cb_evt_t;


typedef struct
{
    uint8_t sampling_frequency;
    uint8_t frame_duration;
    uint16_t octects_per_codec_frame;
    uint32_t audio_channel_allocation;
} bk_bap_codec_spec_config_t;

typedef struct
{
    uint8_t coding_format;
    uint16_t company_id;
    uint16_t vendor_spec_codec_id;
} bk_bap_codec_id_t;

typedef struct
{
    uint8_t bis_index;
    bk_bap_codec_spec_config_t spec_cfg;
} bk_bap_bis_t;

typedef struct
{
    uint32_t presentation_delay;
    uint8_t num_subgroups;
    bk_bap_codec_id_t codec_id;
    uint8_t num_bis;
    bk_bap_codec_spec_config_t spec_cfg;
    bk_bap_bis_t bis[BK_GAP_MAX_BIS];
} bk_bap_basic_audio_config_t;

typedef struct
{
    uint8_t advertising_sid;
    uint8_t address_type;
    uint8_t address[6];
    int8_t rssi;
    uint32_t sample_rate;
} bk_bap_source_announce_data_t;

typedef struct
{
    uint16_t handle;
} bk_bap_source_associate_data_t;

typedef struct
{
    uint16_t sync_handle;
    uint8_t num_bis;
    uint8_t nse;
    uint16_t iso_interval;
    uint8_t bn;
    uint8_t pto;
    uint8_t irc;
    uint16_t max_pdu;
    uint32_t sdu_interval;
    uint16_t max_sdu;
    uint8_t phy;
    uint8_t framing;
    uint8_t encryption;
} bk_bap_source_big_info_t;


typedef struct
{
    uint8_t coding_format;
    uint16_t company_id;
    uint16_t vendor_codec_id;
    uint8_t ie[BK_BAP_CODEC_IE_LEN];
    uint8_t ie_len;
} bk_bap_codec_info_t;

typedef struct
{
    uint8_t data[BK_BAP_METADATA_LEN];
    uint8_t length;
} bk_bap_medadata_t;

typedef struct
{
    uint8_t value[BK_BAP_CODEC_IE_LEN];
    uint8_t length;
} bk_bap_codec_ie_t;

typedef struct
{
    uint8_t sf;
    uint8_t fd;
    uint32_t aca;
    uint16_t opcf;
    uint8_t mcfpSDU;
} bk_bap_lc3_codec_specific_conf_t;

typedef struct
{
    uint8_t error_code;
    uint8_t big_handle;
    uint8_t nse;
    uint8_t num_bis;
    uint16_t connection_handle[BK_GAP_MAX_BIS];
} bk_bap_boradcast_paramters_t;

typedef struct
{
    uint32_t connection_handle : 12;
    uint32_t pb_flag : 2;
    uint32_t ts_flag : 1;
    uint32_t rfu_1 : 1;
    uint32_t data_total_length : 14;
    uint32_t rfu_2 : 2;
    uint32_t time_stamp;
    uint32_t packet_sequence_number : 16;
    uint32_t iso_sdu_length : 12;
    uint32_t rfu_3 : 2;
    uint32_t packet_status_flag : 2;
} bk_bap_iso_header_t;

typedef struct
{
    uint8_t ase_id;
    uint8_t ase_role;
    uint8_t ase_state;
    uint16_t acl_handle;
} bk_bap_unicast_ase_discovered_t;

typedef struct
{
    uint8_t ase_id;
    uint8_t ase_role;
    uint8_t cig_id;
    uint8_t cis_id;
    uint16_t acl_handle;
    uint16_t local_cis_handle;
} bk_bap_unicast_cis_info_t;

typedef struct
{
    uint8_t ase_id;
    uint8_t ase_role;
    uint16_t local_cis_handle;
} bk_bap_unicast_iso_path_t;

typedef struct
{
    void (* announcement_cb)(bk_bap_source_announce_data_t *bk_bap_source_announce_data);
    void (* associate_cb)(bk_bap_source_associate_data_t *bk_bap_source_associate_data);
    void (* config_cb)(uint8_t *data, uint16_t length);
    void (* big_info_cb)(bk_bap_source_big_info_t *bk_bap_source_big_info);
    void (* lc3_data_cb)(bk_bap_iso_header_t *header, uint8_t *data, uint32_t length);
    void (* broadcast_event_cb)(bk_bap_sink_cb_evt_t event, uint32_t status);
    void (* unicast_ase_discovered_cb)(bk_bap_unicast_ase_discovered_t *info);
    void (* unicast_cis_request_cb)(bk_bap_unicast_cis_info_t *info);
    void (* unicast_cis_established_cb)(bk_bap_unicast_cis_info_t *info);
    void (* unicast_iso_path_ready_cb)(bk_bap_unicast_iso_path_t *info);
} bk_bap_sink_callbacks_t;

typedef struct
{
    void (* setup_announcement_cb)(uint32_t status, void *paramters);
    void (* end_announcement_cb)(uint32_t status, void *paramters);
    void (* broadcast_start_cb)(bk_bap_boradcast_paramters_t *bk_bap_boradcast_paramters);
    void (* broadcast_suspend_cb)(uint8_t handle, uint8_t reason);
    void (* unicast_ase_discovered_cb)(bk_bap_unicast_ase_discovered_t *info);
    void (* unicast_cis_handle_assigned_cb)(bk_bap_unicast_cis_info_t *info);
    void (* unicast_cis_established_cb)(bk_bap_unicast_cis_info_t *info);
    void (* unicast_iso_path_ready_cb)(bk_bap_unicast_iso_path_t *info);
} bk_bap_source_callbacks_t;


#ifdef __cplusplus
}
#endif

