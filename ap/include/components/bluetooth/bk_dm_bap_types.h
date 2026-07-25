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

/* Audio Context Type bit masks (Assigned Numbers, PACS contexts). */
#define BK_GAP_CONTEXT_UNSPECIFIED        (0x0001U << 0U)
#define BK_GAP_CONTEXT_CONVERSATIONAL     (0x0001U << 1U)
#define BK_GAP_CONTEXT_MEDIA              (0x0001U << 2U)
#define BK_GAP_CONTEXT_GAME               (0x0001U << 3U)
#define BK_GAP_CONTEXT_INSTRUCTIONAL      (0x0001U << 4U)
#define BK_GAP_CONTEXT_VOICE_ASSISTANTS   (0x0001U << 5U)
#define BK_GAP_CONTEXT_LIVE               (0x0001U << 6U)
#define BK_GAP_CONTEXT_SOUND_EFFECTS      (0x0001U << 7U)
#define BK_GAP_CONTEXT_NOTIFICATIONS      (0x0001U << 8U)
#define BK_GAP_CONTEXT_RINGTONE           (0x0001U << 9U)
#define BK_GAP_CONTEXT_ALERTS             (0x0001U << 10U)
#define BK_GAP_CONTEXT_EMERGENCY_ALARM    (0x0001U << 11U)
#define BK_GAP_DEFAULT_CONTEXTS           (BK_GAP_CONTEXT_UNSPECIFIED | BK_GAP_CONTEXT_MEDIA)

#define BK_GAP_DEFAULT_ASE_COUNT          0x01U
/* Backward-compatible alias kept for older demo code. */
#define BK_GAP_DEFAULT_AES_COUNT          BK_GAP_DEFAULT_ASE_COUNT

#define BK_GAP_MAX_BIS              (2)

#define BK_BAP_CODEC_IE_LEN         24U
#define BK_BAP_METADATA_LEN         24U

/*
 * LC3 codec-specific capability bit masks used by bk_bap_pacs_cfg_t.
 * These are PACS capability masks, not the single-value codec configuration
 * enums used in bk_bap_lc3_codec_specific_conf_t.
 */
#define BK_BAP_LC3_CAP_FREQ_8KHZ             (0x0001U << 0U)
#define BK_BAP_LC3_CAP_FREQ_11KHZ            (0x0001U << 1U)
#define BK_BAP_LC3_CAP_FREQ_16KHZ            (0x0001U << 2U)
#define BK_BAP_LC3_CAP_FREQ_22KHZ            (0x0001U << 3U)
#define BK_BAP_LC3_CAP_FREQ_24KHZ            (0x0001U << 4U)
#define BK_BAP_LC3_CAP_FREQ_32KHZ            (0x0001U << 5U)
#define BK_BAP_LC3_CAP_FREQ_44KHZ            (0x0001U << 6U)
#define BK_BAP_LC3_CAP_FREQ_48KHZ            (0x0001U << 7U)
#define BK_BAP_LC3_CAP_FREQ_88KHZ            (0x0001U << 8U)
#define BK_BAP_LC3_CAP_FREQ_96KHZ            (0x0001U << 9U)
#define BK_BAP_LC3_CAP_FREQ_176KHZ           (0x0001U << 10U)
#define BK_BAP_LC3_CAP_FREQ_192KHZ           (0x0001U << 11U)
#define BK_BAP_LC3_CAP_FREQ_384KHZ           (0x0001U << 12U)

#define BK_BAP_LC3_CAP_DURATION_7_5MS         (0x01U << 0U)
#define BK_BAP_LC3_CAP_DURATION_10MS          (0x01U << 1U)
#define BK_BAP_LC3_CAP_DURATION_7_5MS_PREF    (0x01U << 4U)
#define BK_BAP_LC3_CAP_DURATION_10MS_PREF     (0x01U << 5U)

#define BK_BAP_LC3_CAP_CHANNEL_COUNT_1        (0x01U << 0U)
#define BK_BAP_LC3_CAP_CHANNEL_COUNT_2        (0x01U << 1U)
#define BK_BAP_LC3_CAP_CHANNEL_COUNT_3        (0x01U << 2U)
#define BK_BAP_LC3_CAP_CHANNEL_COUNT_4        (0x01U << 3U)
#define BK_BAP_LC3_CAP_CHANNEL_COUNT_5        (0x01U << 4U)
#define BK_BAP_LC3_CAP_CHANNEL_COUNT_6        (0x01U << 5U)
#define BK_BAP_LC3_CAP_CHANNEL_COUNT_7        (0x01U << 6U)
#define BK_BAP_LC3_CAP_CHANNEL_COUNT_8        (0x01U << 7U)

/*
 * LC3 codec configuration values used by bk_bap_lc3_codec_specific_conf_t.
 * Keep these separate from the PACS capability bit masks above.
 */
#define BK_BAP_LC3_CFG_FREQ_8KHZ              0x01U
#define BK_BAP_LC3_CFG_FREQ_11KHZ             0x02U
#define BK_BAP_LC3_CFG_FREQ_16KHZ             0x03U
#define BK_BAP_LC3_CFG_FREQ_22KHZ             0x04U
#define BK_BAP_LC3_CFG_FREQ_24KHZ             0x05U
#define BK_BAP_LC3_CFG_FREQ_32KHZ             0x06U
#define BK_BAP_LC3_CFG_FREQ_44KHZ             0x07U
#define BK_BAP_LC3_CFG_FREQ_48KHZ             0x08U
#define BK_BAP_LC3_CFG_FREQ_88KHZ             0x09U
#define BK_BAP_LC3_CFG_FREQ_96KHZ             0x0AU
#define BK_BAP_LC3_CFG_FREQ_176KHZ            0x0BU
#define BK_BAP_LC3_CFG_FREQ_192KHZ            0x0CU
#define BK_BAP_LC3_CFG_FREQ_384KHZ            0x0DU

#define BK_BAP_LC3_CFG_DURATION_7_5MS         0x00U
#define BK_BAP_LC3_CFG_DURATION_10MS          0x01U

#define BK_BAP_QOS_FRAMING_UNFRAMED           0x00U
#define BK_BAP_QOS_FRAMING_FRAMED             0x01U

/* Supported PHY bit masks used in ASE QoS capabilities. */
#define BK_BAP_QOS_PHY_1M                     (0x01U << 0U)
#define BK_BAP_QOS_PHY_2M                     (0x01U << 1U)
#define BK_BAP_QOS_PHY_CODED                  (0x01U << 2U)

/* Max transport latency values are in milliseconds. */
#define BK_BAP_QOS_LATENCY_5MS                0x0005U
#define BK_BAP_QOS_LATENCY_10MS               0x000AU
#define BK_BAP_QOS_LATENCY_20MS               0x0014U
#define BK_BAP_QOS_LATENCY_30MS               0x001EU
#define BK_BAP_QOS_LATENCY_40MS               0x0028U

/* Retransmission number values are raw counts. */
#define BK_BAP_QOS_RETRANSMISSION_0           0x00U
#define BK_BAP_QOS_RETRANSMISSION_1           0x01U
#define BK_BAP_QOS_RETRANSMISSION_2           0x02U
#define BK_BAP_QOS_RETRANSMISSION_3           0x03U
#define BK_BAP_QOS_RETRANSMISSION_4           0x04U
#define BK_BAP_QOS_RETRANSMISSION_5           0x05U

/* Presentation delay values are in microseconds. */
#define BK_BAP_QOS_PRESENTATION_DELAY_NO_PREF 0x00000000UL
#define BK_BAP_QOS_PRESENTATION_DELAY_0US     0x00000000UL
#define BK_BAP_QOS_PRESENTATION_DELAY_10MS    0x00002710UL
#define BK_BAP_QOS_PRESENTATION_DELAY_20MS    0x00004E20UL
#define BK_BAP_QOS_PRESENTATION_DELAY_30MS    0x00007530UL
#define BK_BAP_QOS_PRESENTATION_DELAY_40MS    0x00009C40UL

typedef struct
{
    uint16_t supported_contexts;
    uint16_t available_contexts;
    uint32_t audio_location;

    uint16_t supported_sampling_frequencies;
    uint8_t supported_frame_durations;
    uint8_t supported_channel_counts;
    uint16_t frame_octets_min;
    uint16_t frame_octets_max;
    uint8_t max_codec_frames_per_sdu;
} bk_bap_pacs_cfg_t;

typedef struct
{
    uint8_t ase_count;
    uint8_t pref_framing;
    uint8_t pref_phy;
    uint16_t pref_max_transport_latency;
    uint32_t pref_presentation_delay_min;
    uint32_t pref_presentation_delay_max;
    uint8_t pref_retransmission_number;
    uint32_t supported_presentation_delay_min;
    uint32_t supported_presentation_delay_max;
} bk_bap_ascs_cfg_t;

typedef enum {
    BK_BAP_SINK_INVALID = 0,
    BK_BAP_SINK_SCAN_END,
    BK_BAP_SINK_DISABLE_IND,
    BK_BAP_SINK_DISABLE_CNF,
    BK_BAP_SINK_ENABLE_CNF,
    BK_BAP_SINK_DISSOCIATE_CNF,
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
    uint32_t broadcast_id;
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

typedef enum
{
    BK_BAP_UNICAST_STATE_CODEC_CONFIGURED = 0x01,
    BK_BAP_UNICAST_STATE_QOS_CONFIGURED   = 0x02,
    BK_BAP_UNICAST_STATE_ENABLING         = 0x03,
} bk_bap_unicast_state_evt_t;

typedef struct
{
    uint8_t ase_id;
    uint8_t ase_role;
    uint8_t state;        /* bk_bap_unicast_state_evt_t */
    uint16_t acl_handle;
} bk_bap_unicast_state_t;

typedef struct
{
    uint8_t phase;       /* bk_bap_unicast_ready_evt_t */
    uint8_t status;
    uint8_t addr_type;
    uint8_t addr[6];
    uint16_t acl_handle;
} bk_bap_unicast_ready_t;

typedef enum
{
    BK_BAP_UNICAST_READY_ACL_CONNECTED    = 0x01,
    BK_BAP_UNICAST_READY_GA_SETUP         = 0x02,
    BK_BAP_UNICAST_READY_CAPABILITIES     = 0x03,
    BK_BAP_UNICAST_READY_ACL_DISCONNECTED = 0x04,
} bk_bap_unicast_ready_evt_t;

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
    void (* unicast_state_cb)(bk_bap_unicast_state_t *info);
    void (* unicast_ready_cb)(bk_bap_unicast_ready_t *info);
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
    void (* unicast_state_cb)(bk_bap_unicast_state_t *info);
    void (* unicast_ready_cb)(bk_bap_unicast_ready_t *info);
} bk_bap_source_callbacks_t;


#ifdef __cplusplus
}
#endif

