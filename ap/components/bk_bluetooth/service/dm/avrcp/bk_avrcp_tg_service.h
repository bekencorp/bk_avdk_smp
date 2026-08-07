#ifndef BK_AVRCP_TG_SERVICE_H
#define BK_AVRCP_TG_SERVICE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
    /** arg: uint8_t remote_bda[6] */
    BK_AVRCP_TG_EVT_CONNECTED = 0,
    /** arg: uint8_t remote_bda[6] */
    BK_AVRCP_TG_EVT_DISCONNECTED,
    /** arg: uint8_t * volume in 0x00-0x7f range */
    BK_AVRCP_TG_EVT_VOLUME_CHANGED,
    /** arg: bk_avrcp_tg_passthrough_t * (player_mode only) */
    BK_AVRCP_TG_EVT_PASSTHROUGH,
} bk_avrcp_tg_evt_t;

/** Passthrough command delivered to the app in player_mode. */
typedef struct
{
    /** AVRCP passthrough key code (BK_AVRCP_PT_CMD_*). */
    uint8_t key_code;
    /** Non-zero when the key is pressed, zero when released. */
    uint8_t key_state;
} bk_avrcp_tg_passthrough_t;

/** AVRCP target event callback. */
typedef void (*bk_avrcp_tg_event_cb_t)(bk_avrcp_tg_evt_t evt, void *arg, void *user_data);

/** AVRCP target service configuration. */
typedef struct
{
    /** Initial local AVRCP volume in the 0x00-0x7f range. */
    uint8_t default_volume;
    /**
     * 0: volume-sink target (headset/speaker, default) - only VOLUME RN.
     * 1: media-player target (A2DP source) - accepts passthrough and answers
     *    PLAY_STATUS / PLAY_POS / BATTERY registered notifications.
     */
    uint8_t player_mode;
} bk_avrcp_tg_cfg_t;

/** Register the application TG event callback. Call before service init. */
int bk_avrcp_tg_register_event_cb(bk_avrcp_tg_event_cb_t cb, void *user_data);

/** Initialize AVRCP target role and absolute-volume notification support. */
int bk_avrcp_tg_service_init(const bk_avrcp_tg_cfg_t *cfg);

/** Deinitialize AVRCP target role and release resources. */
int bk_avrcp_tg_service_deinit(void);

/** Disconnect AVRCP target from the specified remote device. */
int bk_avrcp_tg_disconnect(const uint8_t bda[6]);

/** Return non-zero when AVRCP target is connected. */
int bk_avrcp_tg_is_connected(void);

/** Notify the peer of a local volume change if it registered for the event. */
int bk_avrcp_tg_notify_volume_change(uint8_t vol_0_7f);

/**
 * Report the current playback status to the peer (player_mode only).
 * @param playback one of bk_avrcp_playback_stat_t (BK_AVRCP_PLAYBACK_*).
 */
int bk_avrcp_tg_notify_playback_status(uint8_t playback);

/** Report a track change to the peer (player_mode only); track_id is an opaque 8-byte id. */
int bk_avrcp_tg_notify_track_change(uint64_t track_id);

/** Report the current playing position in milliseconds (player_mode only). */
int bk_avrcp_tg_notify_play_pos(uint32_t pos_ms);

/** Report the current battery status to the peer (player_mode only). */
int bk_avrcp_tg_notify_battery_status(uint8_t batt);

/** Update local volume, save it for the device address, and emit TG event. */
int bk_avrcp_tg_set_local_volume(uint8_t vol, const uint8_t *bda);

/** Return the current local volume in the AVRCP 0x00-0x7f range. */
uint8_t bk_avrcp_tg_get_local_volume_value(void);

/** Emit the current local volume through the registered TG event callback. */
void bk_avrcp_tg_emit_current_volume(void);

#ifdef __cplusplus
}
#endif

#endif
