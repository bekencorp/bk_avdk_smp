#pragma once

#include <stdint.h>

#define BKI_BAP_CODEC_IE_LEN                                 24U
#define BKI_BAP_METADATA_LEN                                 24U

#define BKI_BAP_SINK_EVT_SACN_END                            0x01U
#define BKI_BAP_SINK_EVT_DISABLE_CNF                         0x02U
#define BKI_BAP_SINK_EVT_DISABLE_IND                         0x03U
#define BKI_BAP_SINK_EVT_ENABLE_CNF                          0x04U
#define BKI_BAP_SINK_EVT_DISSOCIATE_CNF                      0x05U


typedef struct
{
    uint8_t advertising_sid;
    uint8_t address_type;
    uint8_t address[6];
    int8_t rssi;
    uint8_t data[255];
    uint16_t length;
} bki_bap_source_announce_data_t;

typedef struct
{
    uint16_t handle;
} bki_bap_source_associate_data_t;

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
} bki_bap_source_big_info_t;

typedef struct
{
    /** Coding Format */
    uint8_t coding_format;

    /** Company ID */
    uint16_t company_id;

    /** Vendor Specific Codec ID */
    uint16_t vendor_codec_id;

    /** Codec Specific Capabilities/Configuration Information Element */
    uint8_t ie[BKI_BAP_CODEC_IE_LEN];

    /** Codec Information element length */
    uint8_t ie_len;

} bki_bap_codec_info_t;

typedef struct
{
    /** Metadata byte stream */
    uint8_t data[BKI_BAP_METADATA_LEN];

    /** Metadata length */
    uint8_t length;

} bki_bap_medadata_t;

typedef struct
{
    /** Codec Specific Capabilities/Configuration Information Element */
    uint8_t value[BKI_BAP_CODEC_IE_LEN];

    /** Codec Information element length */
    uint8_t length;

} bki_bap_codec_ie_t;

typedef struct
{
    /** Sampling Frequency */
    uint8_t sf;

    /** Frame Duration */
    uint8_t fd;

    /** Audio Channel Allocation */
    uint32_t aca;

    /** Octets per Codec Frame */
    uint16_t opcf;

    /** Codec Frame Blocks Per SDU */
    uint8_t mcfpSDU;
} bki_bap_lc3_codec_specific_conf_t;


typedef struct
{
    uint8_t error_code;
    uint8_t big_handle;
    uint8_t nse;
    uint8_t num_bis;
    uint16_t connection_handle[2];
} bki_bap_boradcast_paramters_t;


typedef struct
{
    void (* announcement_cb)(bki_bap_source_announce_data_t *bk_int_bap_source_announce_data);
    void (* associate_cb)(bki_bap_source_associate_data_t *bk_int_bap_source_associate_data);
    void (* config_cb)(uint8_t *data, uint16_t length);
    void (* big_info_cb)(bki_bap_source_big_info_t *bk_bap_source_big_info);
    void (* lc3_data_cb)(uint8_t *header, uint8_t *data, uint32_t length);
    void (* broadcast_event_cb)(uint32_t event, uint32_t status);
} bki_bap_sink_callbacks_t;

typedef struct
{
    void (* setup_announcement_cb)(uint32_t status, void *paramters);
    void (* end_announcement_cb)(uint32_t status, void *paramters);
    void (* broadcast_start_cb)(bki_bap_boradcast_paramters_t *bki_bap_boradcast_paramters);
    void (* broadcast_suspend_cb)(uint8_t handle, uint8_t reason);
} bki_bap_source_callbacks_t;

typedef struct
{
    uint8_t ase_id;
    uint8_t ase_role;
    uint8_t ase_state;
    uint16_t acl_handle;
} bki_bap_unicast_ase_discovered_t;

typedef struct
{
    uint8_t ase_id;
    uint8_t ase_role;
    uint8_t cig_id;
    uint8_t cis_id;
    uint16_t acl_handle;
    uint16_t local_cis_handle;
} bki_bap_unicast_cis_info_t;

typedef struct
{
    uint8_t ase_id;
    uint8_t ase_role;
    uint16_t local_cis_handle;
} bki_bap_unicast_iso_path_t;

#define BKI_BAP_UNICAST_STATE_CODEC_CONFIGURED              0x01U
#define BKI_BAP_UNICAST_STATE_QOS_CONFIGURED                0x02U
#define BKI_BAP_UNICAST_STATE_ENABLING                      0x03U

typedef struct
{
    uint8_t ase_id;
    uint8_t ase_role;
    uint8_t state;
    uint16_t acl_handle;
} bki_bap_unicast_state_t;

typedef struct
{
    uint8_t phase;
    uint8_t status;
    uint8_t addr_type;
    uint8_t addr[6];
    uint16_t acl_handle;
} bki_bap_unicast_ready_t;

#define BKI_BAP_UNICAST_READY_ACL_CONNECTED    0x01U
#define BKI_BAP_UNICAST_READY_GA_SETUP         0x02U
#define BKI_BAP_UNICAST_READY_CAPABILITIES     0x03U
#define BKI_BAP_UNICAST_READY_ACL_DISCONNECTED 0x04U

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
} bki_bap_pacs_cfg_t;

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
} bki_bap_ascs_cfg_t;

uint32_t appl_le_audio_ga_init(void);
/* See bk_internal_dm_ble_gap.h for appl_le_audio_ga_gatt_db_register(). */
uint32_t appl_le_audio_ga_pacs_register(uint8_t role, const bki_bap_pacs_cfg_t *pacs_cfg);
uint32_t appl_le_audio_ga_ascs_register(uint8_t role, const bki_bap_ascs_cfg_t *ascs_cfg);
uint32_t appl_le_audio_ga_sink_register(bki_bap_sink_callbacks_t *bki_bap_sink_callbacks);
uint32_t appl_le_audio_ga_source_register(bki_bap_source_callbacks_t *bki_bap_source_callbacks);
uint32_t appl_le_audio_unicast_set_peer(uint8_t *addr, uint8_t addr_type);
uint32_t appl_le_audio_unicast_connect(uint8_t *addr, uint8_t addr_type, uint8_t extended);
uint32_t appl_le_audio_unicast_setup(void);
uint32_t appl_le_audio_unicast_get_capabilities(uint8_t role);
uint32_t appl_le_audio_unicast_discover(void);
uint32_t appl_le_audio_unicast_configure(uint8_t ase_id, uint8_t role, uint8_t cap_index);
uint32_t appl_le_audio_unicast_set_cig(uint8_t ase_id, uint8_t cig_id, uint8_t cis_id);
uint32_t appl_le_audio_unicast_qos(uint8_t ase_id);
uint32_t appl_le_audio_unicast_enable(uint8_t ase_id, uint16_t contexts);
uint32_t appl_le_audio_unicast_create_cis(uint8_t ase_id);
uint32_t appl_le_audio_unicast_receiver_start_ready(uint8_t ase_id);
uint32_t appl_le_audio_unicast_release(uint8_t ase_id);
uint32_t appl_le_audio_unicast_remove_iso(uint16_t connection_handle, uint8_t direction);
uint32_t appl_le_audio_unicast_send(uint16_t connection_handle, uint8_t flags, uint32_t time_stamp, uint16_t sequence, uint8_t *data, uint16_t length);
void bk_dm_bap_internal_unicast_ase_discovered(const bki_bap_unicast_ase_discovered_t *info);
void bk_dm_bap_internal_unicast_cis_handle_assigned(const bki_bap_unicast_cis_info_t *info);
void bk_dm_bap_internal_unicast_cis_request(const bki_bap_unicast_cis_info_t *info);
void bk_dm_bap_internal_unicast_cis_established(const bki_bap_unicast_cis_info_t *info);
void bk_dm_bap_internal_unicast_iso_path_ready(const bki_bap_unicast_iso_path_t *info);
void bk_dm_bap_internal_unicast_state_changed(const bki_bap_unicast_state_t *info);
void bk_dm_bap_internal_unicast_ready(const bki_bap_unicast_ready_t *info);
uint32_t appl_le_audio_broadcast_scan_start(void);
uint32_t appl_le_audio_broadcast_scan_stop(void);
uint32_t appl_le_audio_broadcast_associate(bki_bap_source_announce_data_t *bap_source_announce_data);
uint32_t appl_le_audio_broadcast_enable(uint16_t handle, uint8_t *code, uint8_t bis_count, uint8_t *bis);
uint32_t appl_le_audio_broadcast_disable(uint16_t handle);
uint32_t appl_le_audio_broadcast_dissociate(uint16_t sync_handle);
uint32_t appl_le_audio_broadcast_alloc_session(uint8_t *session);
uint32_t appl_le_audio_broadcast_free_session(uint8_t session);
uint32_t appl_le_audio_broadcast_configure_session(uint8_t ssn, uint8_t phy, uint8_t packing, uint8_t *broadcast_codes);
uint32_t appl_le_audio_broadcast_sep_register(uint8_t ssn,
    bki_bap_codec_info_t *codec,
    bki_bap_medadata_t *meta,
    uint8_t nstream,
    bki_bap_codec_ie_t *stream,
    uint8_t *sep);
uint32_t appl_le_audio_broadcast_setup_announcement(uint8_t ssn, uint32_t broadcast_id, uint8_t type, uint32_t presentation_delay);
uint32_t appl_le_audio_broadcast_end_announcement(uint8_t ssn);
uint32_t appl_le_audio_broadcast_start(uint8_t ssn,
    uint32_t sdu_interval,
    uint16_t max_sdu,
    uint16_t max_latency,
    uint8_t rtn,
    uint8_t framing);
uint32_t appl_le_audio_broadcast_suspend(uint8_t ssn);
uint32_t appl_le_audio_broadcast_data_send(uint16_t connection_handle, uint8_t flags, uint32_t time_stamp, uint16_t sequence, uint8_t *data, uint16_t length);

void appl_le_audio_broadcast_create_codec_spec_conf_ltv(bki_bap_lc3_codec_specific_conf_t *bk_int_bap_lc3_codec_specific_conf,
    uint8_t *ltvarray, uint8_t *ltvarray_len);
void appl_le_audio_broadcast_create_metadata_ltv(uint8_t *ltvarray, uint8_t *ltvarray_len);

