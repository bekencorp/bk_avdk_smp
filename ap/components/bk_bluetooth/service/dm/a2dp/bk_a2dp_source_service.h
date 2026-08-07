#ifndef BK_A2DP_SOURCE_SERVICE_H
#define BK_A2DP_SOURCE_SERVICE_H

#include <stdint.h>
#include <stdbool.h>

#include "components/bluetooth/bk_dm_a2dp_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * A2DP source service events.
 *
 * All service-level interfaces carry the "service" word to distinguish them
 * from the low-level SDK APIs (bk_bt_a2dp_source_*).
 */
typedef enum
{
    /** arg: uint8_t remote_bda[6] */
    BK_A2DP_SOURCE_SERVICE_EVT_CONNECTED = 0,
    /** arg: uint8_t remote_bda[6] */
    BK_A2DP_SOURCE_SERVICE_EVT_DISCONNECTED,
    /** arg: negotiated codec config (SBC rate/ch/bitpool) */
    BK_A2DP_SOURCE_SERVICE_EVT_AUDIO_CFG,
    /** arg: NULL. Stack started pulling PCM; app should start feeding PCM. */
    BK_A2DP_SOURCE_SERVICE_EVT_STREAM_START,
    /** arg: NULL. App should stop feeding PCM. */
    BK_A2DP_SOURCE_SERVICE_EVT_STREAM_SUSPEND,
} bk_a2dp_source_service_evt_t;

/**
 * A2DP source service event callback.
 *
 * Invoked from the Bluetooth stack/service context. Copy data before posting
 * it to another task if it must live beyond the callback.
 */
typedef void (*bk_a2dp_source_service_event_cb_t)(bk_a2dp_source_service_evt_t evt, void *arg, void *user_data);

/** A2DP source service configuration. */
typedef struct
{
    uint8_t reserved; /**< Reserved for future use. */
} bk_a2dp_source_service_cfg_t;

/** Register the application event callback. Call before service init. */
int bk_a2dp_source_service_register_event_cb(bk_a2dp_source_service_event_cb_t cb, void *user_data);

/** Initialize A2DP source profile and register internal Bluetooth callbacks. */
int bk_a2dp_source_service_init(const bk_a2dp_source_service_cfg_t *cfg);

/** Deinitialize A2DP source profile and release service resources. */
int bk_a2dp_source_service_deinit(void);

/** Connect A2DP source to the specified remote device. */
int bk_a2dp_source_service_connect(const uint8_t bda[6]);

/** Disconnect the current A2DP source link. */
int bk_a2dp_source_service_disconnect(void);

/** Arm the internal encode/send pipeline (ring buffer + SBC + stack callbacks). */
int bk_a2dp_source_service_media_start(void);

/** Tear down the internal pipeline. */
int bk_a2dp_source_service_media_stop(void);

/** Start the AVDTP stream (media-ctrl START); waits for AUDIO_STATE STARTED. */
int bk_a2dp_source_service_stream_start(void);

/** Suspend the AVDTP stream (media-ctrl SUSPEND); waits for AUDIO_STATE SUSPEND. */
int bk_a2dp_source_service_stream_suspend(void);

/** Set the PCM source format fed by the app (call before/at stream start). */
int bk_a2dp_source_service_set_pcm_format(uint32_t sample_rate, uint8_t ch, uint8_t bits);

/**
 * Set the negotiated SBC codec params (from the AUDIO_SOURCE_CFG event).
 *
 * Transitional (P2): the demo forwards the mcc it receives. In P3 the service
 * obtains this itself from the connection and this call becomes optional.
 */
int bk_a2dp_source_service_set_codec_cfg(const bk_a2dp_mcc_t *cap);

/**
 * Feed decoded PCM into the source pipeline (app owns file read + decode).
 *
 * @return number of bytes accepted, or a negative value on error.
 */
int32_t bk_a2dp_source_service_write_pcm(const uint8_t *pcm, uint32_t len);

/** Pause/resume PCM consumption by the pipeline. */
void bk_a2dp_source_service_pcm_pause(bool pause);

#ifdef __cplusplus
}
#endif

#endif /* BK_A2DP_SOURCE_SERVICE_H */
