#ifndef BK_HFP_HF_SERVICE_H
#define BK_HFP_HF_SERVICE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
    /** arg: bk_hfp_hf_conn_info_t * (service level connection up) */
    BK_HFP_HF_EVT_CONNECTED = 0,
    /** arg: bk_hfp_hf_conn_info_t * */
    BK_HFP_HF_EVT_DISCONNECTED,
    /** arg: bk_hfp_hf_audio_info_t * (SCO audio connected) */
    BK_HFP_HF_EVT_AUDIO_CONNECTED,
    /** arg: NULL (SCO audio disconnected) */
    BK_HFP_HF_EVT_AUDIO_DISCONNECTED,
    /** arg: bk_hfp_hf_volume_info_t * (AG volume control) */
    BK_HFP_HF_EVT_VOLUME_CHANGED,
    /** arg: bk_hfp_hf_voice_data_t * (downlink SCO voice packet) */
    BK_HFP_HF_EVT_VOICE_DATA,
    /** arg: bk_hfp_hf_at_response_info_t * */
    BK_HFP_HF_EVT_AT_RESPONSE,
    /** arg: NULL (incoming call ring) */
    BK_HFP_HF_EVT_RING,
    /** arg: bk_hfp_hf_call_info_t * (call activity indicator) */
    BK_HFP_HF_EVT_CALL_IND,
    /** arg: bk_hfp_hf_call_setup_info_t * (call setup indicator) */
    BK_HFP_HF_EVT_CALL_SETUP_IND,
    /** arg: bk_hfp_hf_clip_info_t * (calling line identification) */
    BK_HFP_HF_EVT_CLIP,
    /** arg: bk_hfp_hf_btrh_info_t * (call hold and response status) */
    BK_HFP_HF_EVT_BTRH,
    /** arg: bk_hfp_hf_unknown_data_info_t * (raw AT data from AG, e.g. +CGMI) */
    BK_HFP_HF_EVT_UNKNOWN_DATA,
} bk_hfp_hf_evt_t;

/** Service-level connection event payload. */
typedef struct
{
    uint8_t remote_bda[6];
    uint32_t peer_feat;
    uint32_t chld_feat;
} bk_hfp_hf_conn_info_t;

/** SCO audio connection event payload. */
typedef struct
{
    uint8_t remote_bda[6];
    uint8_t codec; /* CODEC_VOICE_CVSD / CODEC_VOICE_MSBC */
} bk_hfp_hf_audio_info_t;

/** AG volume control event payload. */
typedef struct
{
    uint8_t type;   /* BK_HF_VOLUME_CONTROL_TARGET_SPK / _MIC */
    uint8_t volume;
} bk_hfp_hf_volume_info_t;

/** Downlink voice packet. The buffer is only valid during the event callback. */
typedef struct
{
    const uint8_t *data;
    uint16_t len;
} bk_hfp_hf_voice_data_t;

/** AT response event payload. */
typedef struct
{
    int code;     /* BK_HF_AT_RESPONSE_CODE_* */
    int asso_cmd; /* BK_HF_AT_CMD_* */
    int cme;
} bk_hfp_hf_at_response_info_t;

/** Call activity indicator payload (+CIEV call). */
typedef struct
{
    int status;
} bk_hfp_hf_call_info_t;

/** Call setup indicator payload (+CIEV callsetup). */
typedef struct
{
    int status;
} bk_hfp_hf_call_setup_info_t;

/** Calling line identification payload (+CLIP). Strings valid only in callback. */
typedef struct
{
    const char *number;
    const char *name;
} bk_hfp_hf_clip_info_t;

/** Call hold and response status payload (+BTRH). */
typedef struct
{
    int status;
} bk_hfp_hf_btrh_info_t;

/** Raw/unknown AT data from the AG (e.g. +CGMI response). Valid only in callback. */
typedef struct
{
    const char *data;
    uint16_t len;
} bk_hfp_hf_unknown_data_info_t;

/**
 * HFP HF service event callback.
 *
 * Invoked from the Bluetooth stack/service context. Copy data before posting it
 * to another task if it must live beyond the callback.
 */
typedef void (*bk_hfp_hf_event_cb_t)(bk_hfp_hf_evt_t evt, void *arg, void *user_data);

/** Register the application event callback. Call before service init. */
int bk_hfp_hf_register_event_cb(bk_hfp_hf_event_cb_t cb, void *user_data);

/** Initialize HFP HF profile and register internal Bluetooth callbacks. */
int bk_hfp_hf_service_init(uint8_t msbc_supported);

/** Deinitialize HFP HF profile and release service resources. */
int bk_hfp_hf_service_deinit(void);

/** Initiate an HFP HF connection to the specified remote device. */
int bk_hfp_hf_connect(const uint8_t bda[6]);

/** Disconnect the HFP HF connection from the specified remote device. */
int bk_hfp_hf_disconnect(const uint8_t bda[6]);

/* Command wrappers. All operate on the currently connected service-level peer. */
int bk_hfp_hf_query_current_calls(void);
int bk_hfp_hf_volume_update(uint8_t type, uint8_t volume);
int bk_hfp_hf_send_custom_cmd(const char *cmd);
int bk_hfp_hf_query_current_operator_name(void);
int bk_hfp_hf_retrieve_subscriber_info(void);
int bk_hfp_hf_send_dtmf(const char *code);
int bk_hfp_hf_request_last_voice_tag_number(void);
int bk_hfp_hf_send_nrec(void);
int bk_hfp_hf_start_voice_recognition(void);
int bk_hfp_hf_stop_voice_recognition(void);
int bk_hfp_hf_dial(const char *num);
int bk_hfp_hf_dial_memory(int32_t location);
int bk_hfp_hf_redial(void);
int bk_hfp_hf_answer_call(void);
int bk_hfp_hf_reject_call(void);
int bk_hfp_hf_send_chld_cmd(uint8_t op);
int bk_hfp_hf_send_btrh_cmd(uint8_t op);

#ifdef __cplusplus
}
#endif

#endif
