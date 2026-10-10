#ifndef BK_AVRCP_CT_SERVICE_H
#define BK_AVRCP_CT_SERVICE_H

#include <stdint.h>

#include "components/bluetooth/bk_dm_avrcp_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
    /** arg: uint8_t remote_bda[6] */
    BK_AVRCP_CT_EVT_CONNECTED = 0,
    /** arg: uint8_t remote_bda[6] */
    BK_AVRCP_CT_EVT_DISCONNECTED,
    /** arg: uint8_t * playback status */
    BK_AVRCP_CT_EVT_PLAY_STATUS_CHANGED,
    /** arg: uint64_t * track id */
    BK_AVRCP_CT_EVT_TRACK_CHANGED,
    /** arg: bk_avrcp_ct_cb_param_t * */
    BK_AVRCP_CT_EVT_ELEM_ATTR_RSP,
    /** arg: uint32_t * playing position in milliseconds */
    BK_AVRCP_CT_EVT_PLAY_POS_CHANGED,
    /** arg: uint8_t * peer volume 0x00-0x7f (remote_volume_mode) */
    BK_AVRCP_CT_EVT_REMOTE_VOLUME_CHANGED,
    /** arg: uint8_t * peer battery status (remote_volume_mode) */
    BK_AVRCP_CT_EVT_REMOTE_BATTERY_CHANGED,
    /** arg: bk_avrcp_ct_abs_vol_rsp_t * (remote_volume_mode) */
    BK_AVRCP_CT_EVT_SET_ABS_VOLUME_RSP,
} bk_avrcp_ct_evt_t;

/** Result of a set-absolute-volume command sent to the peer. */
typedef struct
{
    /** 0 on success, non-zero when the peer rejected/failed the request. */
    uint8_t status;
    /** Volume echoed back by the peer, in the 0x00-0x7f range. */
    uint8_t volume;
} bk_avrcp_ct_abs_vol_rsp_t;

/** AVRCP controller event callback. */
typedef void (*bk_avrcp_ct_event_cb_t)(bk_avrcp_ct_evt_t evt, void *arg, void *user_data);

/** AVRCP controller service configuration. */
typedef struct
{
    /** Non-zero to auto-connect CT after A2DP connects. */
    uint8_t auto_ct_connect_after_a2dp;
    /**
     * 0: media-control controller (headset - registers PLAY_STATUS/TRACK/POS
     *    and drives remote media, default).
     * 1: remote-volume controller (A2DP source - registers peer VOLUME/BATTERY
     *    and sends absolute volume to the peer).
     */
    uint8_t remote_volume_mode;
} bk_avrcp_ct_cfg_t;

/** Register the application CT event callback. Call before service init. */
int bk_avrcp_ct_register_event_cb(bk_avrcp_ct_event_cb_t cb, void *user_data);

/** Initialize AVRCP controller role and internal state. */
int bk_avrcp_ct_service_init(const bk_avrcp_ct_cfg_t *cfg);

/** Deinitialize AVRCP controller role and release resources. */
int bk_avrcp_ct_service_deinit(void);

/** Notify CT service of A2DP connection state for optional AVRCP auto-connect. */
void bk_avrcp_ct_service_notify_a2dp_state(uint8_t connected, const uint8_t bda[6]);

/** Stop the pending AVRCP controller auto-connect timer, if any. */
int bk_avrcp_stop_reconnect(void);

/** Connect AVRCP controller to the specified remote device. */
int bk_avrcp_ct_connect(const uint8_t bda[6]);

/** Disconnect AVRCP controller from the specified remote device. */
int bk_avrcp_ct_disconnect(const uint8_t bda[6]);

/** Return non-zero when AVRCP controller is connected. */
int bk_avrcp_ct_is_connected(void);

/** Send AVRCP play passthrough command. */
int bk_avrcp_ct_play(void);

/** Send AVRCP pause passthrough command. */
int bk_avrcp_ct_pause(void);

/** Send AVRCP next-track passthrough command. */
int bk_avrcp_ct_next(void);

/** Send AVRCP previous-track passthrough command. */
int bk_avrcp_ct_prev(void);

/** Send AVRCP rewind command for the requested duration in milliseconds. */
int bk_avrcp_ct_rewind(uint32_t ms);

/** Send AVRCP fast-forward command for the requested duration in milliseconds. */
int bk_avrcp_ct_fast_forward(uint32_t ms);

/** Increase local volume and notify/send passthrough according to peer support. */
int bk_avrcp_ct_vol_up(void);

/** Decrease local volume and notify/send passthrough according to peer support. */
int bk_avrcp_ct_vol_down(void);

/** Request media element attributes. Pass 0 to request the default title attr. */
int bk_avrcp_ct_get_attr(uint32_t attr_id);

/** Send an absolute volume (0x00-0x7f) to the peer (remote_volume_mode). */
int bk_avrcp_ct_send_absolute_volume(uint8_t vol_0_7f);

#ifdef __cplusplus
}
#endif

#endif
