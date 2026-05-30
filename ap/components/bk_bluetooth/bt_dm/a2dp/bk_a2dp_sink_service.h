#ifndef BK_A2DP_SINK_SERVICE_H
#define BK_A2DP_SINK_SERVICE_H

#include <stdint.h>

#include "components/bluetooth/bk_dm_a2dp_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
    /** arg: bk_a2dp_sink_conn_t * */
    BK_A2DP_SINK_EVT_CONNECTED = 0,
    /** arg: bk_a2dp_sink_conn_t * */
    BK_A2DP_SINK_EVT_DISCONNECTED,
    /** arg: bk_a2dp_mcc_t * */
    BK_A2DP_SINK_EVT_AUDIO_CFG,
    /** arg: bk_a2dp_mcc_t * */
    BK_A2DP_SINK_EVT_STREAM_START,
    /** arg: NULL */
    BK_A2DP_SINK_EVT_STREAM_SUSPEND,
    /** arg: bk_a2dp_media_data_t * */
    BK_A2DP_SINK_EVT_MEDIA_DATA,
} bk_a2dp_sink_evt_t;

/** A2DP connection event payload. */
typedef struct
{
    uint8_t remote_bda[6];
} bk_a2dp_sink_conn_t;

/** Media packet payload. The buffer is only valid during the event callback. */
typedef struct
{
    const uint8_t *data;
    uint16_t len;
} bk_a2dp_media_data_t;

/** Codec capabilities exposed by the sink service. */
typedef enum
{
    BK_A2DP_SINK_CODEC_SBC = 0,
    BK_A2DP_SINK_CODEC_AAC,
} bk_a2dp_sink_codec_t;

/**
 * A2DP sink service event callback.
 *
 * The callback is invoked from the Bluetooth stack/service context. Copy data
 * before posting it to another task if it must live beyond the callback.
 */
typedef void (*bk_a2dp_sink_event_cb_t)(bk_a2dp_sink_evt_t evt, void *arg, void *user_data);

/** A2DP sink service configuration. */
typedef struct
{
    /** Non-zero to advertise AAC support in addition to SBC. */
    uint8_t aac_supported;
    /** Non-zero to accept or reconnect incoming devices automatically. */
    uint8_t auto_accept_conn;
} bk_a2dp_sink_cfg_t;

/** Register the application event callback. Call before service init. */
int bk_a2dp_sink_register_event_cb(bk_a2dp_sink_event_cb_t cb, void *user_data);

/** Initialize A2DP sink profile and register internal Bluetooth callbacks. */
int bk_a2dp_sink_service_init(const bk_a2dp_sink_cfg_t *cfg);

/** Deinitialize A2DP sink profile and release service resources. */
int bk_a2dp_sink_service_deinit(void);

/** Connect A2DP sink to the specified remote device. */
int bk_a2dp_sink_connect(const uint8_t bda[6]);

/** Disconnect A2DP sink from the specified remote device. */
int bk_a2dp_sink_disconnect(const uint8_t bda[6]);

#ifdef __cplusplus
}
#endif

#endif
