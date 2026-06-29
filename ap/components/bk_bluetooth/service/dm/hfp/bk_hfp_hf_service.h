/**
 * @file bk_hfp_hf_service.h
 *
 * @brief Hands-Free Profile HF service wrapper APIs.
 *
 * This service wraps common HFP Hands-Free operations, including service-level
 * connection management, SCO audio events, call control, volume control, and
 * AT command exchange with an Audio Gateway.
 */

#ifndef BK_HFP_HF_SERVICE_H
#define BK_HFP_HF_SERVICE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
    /** HFP service-level connection is established. Event data: bk_hfp_hf_conn_info_t. */
    BK_HFP_HF_EVT_CONNECTED = 0,
    /** HFP service-level connection is disconnected. Event data: bk_hfp_hf_conn_info_t. */
    BK_HFP_HF_EVT_DISCONNECTED,
    /** SCO audio connection is established. Event data: bk_hfp_hf_audio_info_t. */
    BK_HFP_HF_EVT_AUDIO_CONNECTED,
    /** SCO audio connection is disconnected. Event data: NULL. */
    BK_HFP_HF_EVT_AUDIO_DISCONNECTED,
    /** Audio Gateway volume state is changed. Event data: bk_hfp_hf_volume_info_t. */
    BK_HFP_HF_EVT_VOLUME_CHANGED,
    /** Downlink SCO voice packet is received. Event data: bk_hfp_hf_voice_data_t. */
    BK_HFP_HF_EVT_VOICE_DATA,
    /** AT command response is received. Event data: bk_hfp_hf_at_response_info_t. */
    BK_HFP_HF_EVT_AT_RESPONSE,
    /** Incoming call ring indication is received. Event data: NULL. */
    BK_HFP_HF_EVT_RING,
    /** Call activity indicator is changed. Event data: bk_hfp_hf_call_info_t. */
    BK_HFP_HF_EVT_CALL_IND,
    /** Call setup indicator is changed. Event data: bk_hfp_hf_call_setup_info_t. */
    BK_HFP_HF_EVT_CALL_SETUP_IND,
    /** Calling line identification is received. Event data: bk_hfp_hf_clip_info_t. */
    BK_HFP_HF_EVT_CLIP,
    /** Call hold and response status is received. Event data: bk_hfp_hf_btrh_info_t. */
    BK_HFP_HF_EVT_BTRH,
    /** Raw AT data is received from the Audio Gateway. Event data: bk_hfp_hf_unknown_data_info_t. */
    BK_HFP_HF_EVT_UNKNOWN_DATA,
} bk_hfp_hf_evt_t;

/** Service-level connection event payload. */
typedef struct
{
    uint8_t remote_bda[6]; /**< Remote Bluetooth device address. */
    uint32_t peer_feat;    /**< Peer-supported HFP features. */
    uint32_t chld_feat;    /**< Peer-supported call hold features. */
} bk_hfp_hf_conn_info_t;

/** SCO audio connection event payload. */
typedef struct
{
    uint8_t remote_bda[6]; /**< Remote Bluetooth device address. */
    uint8_t codec;         /**< SCO codec, such as CODEC_VOICE_CVSD or CODEC_VOICE_MSBC. */
} bk_hfp_hf_audio_info_t;

/** AG volume control event payload. */
typedef struct
{
    uint8_t type;   /**< Volume target, such as speaker or microphone. */
    uint8_t volume; /**< Volume value reported by the Audio Gateway. */
} bk_hfp_hf_volume_info_t;

/** Downlink voice packet. The buffer is only valid during the event callback. */
typedef struct
{
    const uint8_t *data; /**< Voice packet buffer. */
    uint16_t len;        /**< Voice packet length in bytes. */
} bk_hfp_hf_voice_data_t;

/** AT response event payload. */
typedef struct
{
    int code;     /**< AT response code, see BK_HF_AT_RESPONSE_CODE_*. */
    int asso_cmd; /**< Associated AT command, see BK_HF_AT_CMD_*. */
    int cme;      /**< CME error code when present. */
} bk_hfp_hf_at_response_info_t;

/** Call activity indicator payload (+CIEV call). */
typedef struct
{
    int status; /**< Call activity status. */
} bk_hfp_hf_call_info_t;

/** Call setup indicator payload (+CIEV callsetup). */
typedef struct
{
    int status; /**< Call setup status. */
} bk_hfp_hf_call_setup_info_t;

/** Calling line identification payload (+CLIP). Strings valid only in callback. */
typedef struct
{
    const char *number; /**< Caller number string. */
    const char *name;   /**< Caller name string, if provided by the peer. */
} bk_hfp_hf_clip_info_t;

/** Call hold and response status payload (+BTRH). */
typedef struct
{
    int status; /**< Call hold and response status. */
} bk_hfp_hf_btrh_info_t;

/** Raw/unknown AT data from the AG (e.g. +CGMI response). Valid only in callback. */
typedef struct
{
    const char *data; /**< Raw AT data buffer. */
    uint16_t len;     /**< Raw AT data length in bytes. */
} bk_hfp_hf_unknown_data_info_t;

/**
 * HFP HF service event callback.
 *
 * Invoked from the Bluetooth stack/service context. Copy data before posting it
 * to another task if it must live beyond the callback.
 *
 * @param evt Event ID.
 * @param arg Event payload. The concrete type depends on the event ID.
 * @param user_data User context passed to bk_hfp_hf_register_event_cb().
 */
typedef void (*bk_hfp_hf_event_cb_t)(bk_hfp_hf_evt_t evt, void *arg, void *user_data);

/**
 * @brief Register the application event callback.
 *
 * Call this function before bk_hfp_hf_service_init().
 *
 * @param cb Event callback.
 * @param user_data User context passed back in the callback.
 *
 * @return 0 on success, otherwise error code.
 */
int bk_hfp_hf_register_event_cb(bk_hfp_hf_event_cb_t cb, void *user_data);

/**
 * @brief Initialize the HFP HF service.
 *
 * @param msbc_supported Non-zero to enable mSBC support, zero to use CVSD only.
 *
 * @return 0 on success, otherwise error code.
 */
int bk_hfp_hf_service_init(uint8_t msbc_supported);

/**
 * @brief Deinitialize the HFP HF service.
 *
 * @return 0 on success, otherwise error code.
 */
int bk_hfp_hf_service_deinit(void);

/**
 * @brief Connect to a remote HFP Audio Gateway.
 *
 * @param bda Remote Bluetooth device address.
 *
 * @return 0 on success, otherwise error code.
 */
int bk_hfp_hf_connect(const uint8_t bda[6]);

/**
 * @brief Disconnect from a remote HFP Audio Gateway.
 *
 * @param bda Remote Bluetooth device address.
 *
 * @return 0 on success, otherwise error code.
 */
int bk_hfp_hf_disconnect(const uint8_t bda[6]);

/**
 * @brief Query the current call list from the connected Audio Gateway.
 *
 * @return 0 on success, otherwise error code.
 */
int bk_hfp_hf_query_current_calls(void);

/**
 * @brief Update speaker or microphone volume on the connected Audio Gateway.
 *
 * @param type Volume target, such as speaker or microphone.
 * @param volume Volume value.
 *
 * @return 0 on success, otherwise error code.
 */
int bk_hfp_hf_volume_update(uint8_t type, uint8_t volume);

/**
 * @brief Send a custom AT command to the connected Audio Gateway.
 *
 * @param cmd AT command string.
 *
 * @return 0 on success, otherwise error code.
 */
int bk_hfp_hf_send_custom_cmd(const char *cmd);

/**
 * @brief Query the current operator name.
 *
 * @return 0 on success, otherwise error code.
 */
int bk_hfp_hf_query_current_operator_name(void);

/**
 * @brief Retrieve subscriber number information.
 *
 * @return 0 on success, otherwise error code.
 */
int bk_hfp_hf_retrieve_subscriber_info(void);

/**
 * @brief Send a DTMF code.
 *
 * @param code DTMF code string.
 *
 * @return 0 on success, otherwise error code.
 */
int bk_hfp_hf_send_dtmf(const char *code);

/**
 * @brief Request the last voice tag number.
 *
 * @return 0 on success, otherwise error code.
 */
int bk_hfp_hf_request_last_voice_tag_number(void);

/**
 * @brief Send the NREC command.
 *
 * @return 0 on success, otherwise error code.
 */
int bk_hfp_hf_send_nrec(void);

/**
 * @brief Start voice recognition on the connected Audio Gateway.
 *
 * @return 0 on success, otherwise error code.
 */
int bk_hfp_hf_start_voice_recognition(void);

/**
 * @brief Stop voice recognition on the connected Audio Gateway.
 *
 * @return 0 on success, otherwise error code.
 */
int bk_hfp_hf_stop_voice_recognition(void);

/**
 * @brief Dial a phone number.
 *
 * @param num Phone number string.
 *
 * @return 0 on success, otherwise error code.
 */
int bk_hfp_hf_dial(const char *num);

/**
 * @brief Dial a number from the Audio Gateway memory location.
 *
 * @param location Memory location index.
 *
 * @return 0 on success, otherwise error code.
 */
int bk_hfp_hf_dial_memory(int32_t location);

/**
 * @brief Redial the last dialed number.
 *
 * @return 0 on success, otherwise error code.
 */
int bk_hfp_hf_redial(void);

/**
 * @brief Answer an incoming call.
 *
 * @return 0 on success, otherwise error code.
 */
int bk_hfp_hf_answer_call(void);

/**
 * @brief Reject an incoming call.
 *
 * @return 0 on success, otherwise error code.
 */
int bk_hfp_hf_reject_call(void);

/**
 * @brief Send a call hold command.
 *
 * @param op CHLD operation value.
 *
 * @return 0 on success, otherwise error code.
 */
int bk_hfp_hf_send_chld_cmd(uint8_t op);

/**
 * @brief Send a call response and hold command.
 *
 * @param op BTRH operation value.
 *
 * @return 0 on success, otherwise error code.
 */
int bk_hfp_hf_send_btrh_cmd(uint8_t op);

#ifdef __cplusplus
}
#endif

#endif
