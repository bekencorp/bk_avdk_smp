/*
 * A2DP Sink Offload Player - CP playback driver.
 *
 * Owns the playback worker (queue + task). Offload (Layer 4) callbacks run in
 * controller context and only copy+enqueue here; the task dequeues and drives
 * the audio assembler (a2dp_sink_audio). Volume is cached locally and applied
 * at START / live while playing.
 *
 * The queue/task/event flow always builds. Only the real audio_play calls are
 * gated by CONFIG_AUDIO_PLAY; with it off, the full offload->player pipeline
 * still runs (parse cfg, count frames, cache volume, log) for debugging.
 */
#include "a2dp_sink_offload_player.h"

#if CONFIG_BLUETOOTH_A2DP_OFFLOAD_SINK

#include <string.h>
#include <components/system.h>
#include <os/os.h>

#include "a2dp_sink_offload.h"
#if CONFIG_AUDIO_PLAY
#include "a2dp_sink_audio.h"   /* pulls audio_play/onboard_speaker; only when available */
#endif

#define TAG "a2dp_ply"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

/* ---- worker parameters ---- */
#define PLAYER_FRAME_MAX     (1024u)   /* max frame size (matches L3 reassembly cap) */
#define PLAYER_Q_DEPTH       (8u)
#define PLAYER_TASK_PRIO     (5u)
#define PLAYER_TASK_STACK    (4096u)   /* AAC path uses ~1KB stack frame buffer */

/* Default AVRCP absolute volume (0~127) until the first VOLUME event arrives. */
#define PLAYER_DEFAULT_VOLUME (90u)

#if CONFIG_AUDIO_PLAY
/* audio_play open needs the full vote (A2DP + USER); no user gating on CP. */
#define PLAYER_OPEN_VOTE_FULL \
    ((1u << BK_A2DP_AUDIO_OPEN_VOTE_A2DP) | (1u << BK_A2DP_AUDIO_OPEN_VOTE_USER))
#endif

/* queue message types */
enum {
    PLAYER_MSG_FRAME = 0,   /* raw codec frame */
    PLAYER_MSG_START,       /* session start */
    PLAYER_MSG_STOP,        /* session stop */
    PLAYER_MSG_VOLUME,      /* AVRCP volume (carried in msg.len) */
};

/* Queue message: frames and control events share one queue (value-copied by
 * rtos). data[] reuse: FRAME holds frame bytes; START holds cfg[] (<=16B). */
typedef struct {
    uint8_t  type;
    uint8_t  codec;
    uint16_t handle;
    uint16_t len;         /* FRAME: frame len; START: cfg len; VOLUME: volume */
    uint32_t ts;          /* FRAME: RTP timestamp */
    uint8_t  data[PLAYER_FRAME_MAX];
} player_msg_t;

static beken_thread_t s_task;
static beken_queue_t  s_queue;
static volatile bool  s_running;

static uint8_t s_volume = PLAYER_DEFAULT_VOLUME;   /* last known AVRCP volume */
static bool    s_playing;                          /* audio_start done */
#if CONFIG_AUDIO_PLAY
static uint8_t s_mix_multi_channel;                /* product config, default 0 */
#endif

/* ---- offload callbacks (controller context): copy + enqueue only ---- */
static void player_on_frame(uint16_t handle, uint8_t codec,
                            const uint8_t *frame, uint16_t len, uint32_t ts)
{
    player_msg_t msg;

    if (!s_running || !frame || len == 0) {
        return;
    }
    if (len > PLAYER_FRAME_MAX) {
        len = PLAYER_FRAME_MAX;   /* truncation guard, should not trigger */
    }
    msg.type   = PLAYER_MSG_FRAME;
    msg.codec  = codec;
    msg.handle = handle;
    msg.len    = len;
    msg.ts     = ts;
    memcpy(msg.data, frame, len);

    if (rtos_push_to_queue(&s_queue, &msg, 0) != BK_OK) {
        LOGW("frame dropped, queue full (len=%u)\n", (unsigned)len);
    }
}

static void player_on_ctrl(a2dp_sink_offload_evt_t evt,
                           const a2dp_sink_offload_session_t *info)
{
    player_msg_t msg;

    if (!s_running || !info) {
        return;
    }
    memset(&msg, 0, sizeof(msg));
    msg.codec  = info->codec;
    msg.handle = info->handle;

    switch (evt) {
    case A2DP_SINK_OFFLOAD_EVT_START:
        msg.type = PLAYER_MSG_START;
        msg.len  = info->cfg_len;
        if (info->cfg_len) {
            uint16_t n = info->cfg_len;
            if (n > A2DP_SINK_OFFLOAD_CODEC_CFG_MAX) {
                n = A2DP_SINK_OFFLOAD_CODEC_CFG_MAX;
            }
            memcpy(msg.data, info->cfg, n);
            msg.len = n;
        }
        break;
    case A2DP_SINK_OFFLOAD_EVT_STOP:
        msg.type = PLAYER_MSG_STOP;
        break;
    case A2DP_SINK_OFFLOAD_EVT_VOLUME:
        msg.type = PLAYER_MSG_VOLUME;
        msg.len  = info->volume;
        break;
    default:
        return;
    }

    if (rtos_push_to_queue(&s_queue, &msg, 0) != BK_OK) {
        LOGW("ctrl msg dropped, queue full (type=%u)\n", (unsigned)msg.type);
    }
}

/* ---- worker task: dequeue -> drive audio assembler ---- */
static void player_handle_msg(player_msg_t *msg)
{
    switch (msg->type) {
    case PLAYER_MSG_START: {
        bk_a2dp_mcc_t mcc;
        /* parse is audio-independent (offload shim + types), keep it for debug */
        if (a2dp_sink_offload_parse_cfg(msg->codec, msg->data, (uint8_t)msg->len, &mcc) != BK_OK) {
            LOGE("parse cfg failed codec=%u cfg_len=%u\n", (unsigned)msg->codec, (unsigned)msg->len);
            break;
        }
#if CONFIG_AUDIO_PLAY
        if (a2dp_sink_audio_start(&mcc, PLAYER_OPEN_VOTE_FULL, s_mix_multi_channel, s_volume) == BK_OK) {
            s_playing = true;
            LOGI("START codec=%u volume=%u\n", (unsigned)msg->codec, (unsigned)s_volume);
        } else {
            LOGE("audio start failed\n");
        }
#else
        s_playing = true;   /* track flow only; no audio */
        LOGI("START codec=%u volume=%u (audio_play off)\n", (unsigned)msg->codec, (unsigned)s_volume);
#endif
        break;
    }
    case PLAYER_MSG_STOP:
#if CONFIG_AUDIO_PLAY
        a2dp_sink_audio_stop();
#endif
        s_playing = false;
        LOGI("STOP\n");
        break;
    case PLAYER_MSG_VOLUME:
        s_volume = (uint8_t)msg->len;
#if CONFIG_AUDIO_PLAY
        if (s_playing) {
            a2dp_sink_audio_set_gain(s_volume);
        }
#endif
        LOGI("VOLUME %u (playing=%d)\n", (unsigned)s_volume, (int)s_playing);
        break;
    case PLAYER_MSG_FRAME:
#if CONFIG_AUDIO_PLAY
        a2dp_sink_audio_handle_data(msg->data, msg->len);
#endif
        break;
    default:
        LOGW("unknown msg type %u\n", (unsigned)msg->type);
        break;
    }
}

static void player_task(void *arg)
{
    player_msg_t msg;

    (void)arg;
    while (s_running) {
        if (rtos_pop_from_queue(&s_queue, &msg, BEKEN_WAIT_FOREVER) != BK_OK) {
            continue;
        }
        player_handle_msg(&msg);
    }

    s_task = NULL;
    rtos_delete_thread(NULL);
}

bk_err_t a2dp_sink_offload_player_init(void)
{
    int ret;

    if (s_running) {
        return BK_OK;
    }
    s_volume     = PLAYER_DEFAULT_VOLUME;
    s_playing    = false;

    ret = rtos_init_queue(&s_queue, "a2dp_player_q",
                          sizeof(player_msg_t), PLAYER_Q_DEPTH);
    if (ret != BK_OK) {
        LOGE("init queue failed %d\n", ret);
        return BK_FAIL;
    }

    s_running = true;
    ret = rtos_create_thread(&s_task, PLAYER_TASK_PRIO, "a2dp_player",
                             (beken_thread_function_t)player_task,
                             PLAYER_TASK_STACK, NULL);
    if (ret != BK_OK) {
        s_running = false;
        rtos_deinit_queue(&s_queue);
        LOGE("create task failed %d\n", ret);
        return BK_FAIL;
    }

    /* register with Layer 4 only after the worker is up */
    a2dp_sink_offload_register_report_cb(player_on_frame);
    a2dp_sink_offload_register_ctrl_cb(player_on_ctrl);
    LOGI("cp a2dp sink offload player inited\n");
    return BK_OK;
}

void a2dp_sink_offload_player_deinit(void)
{
    if (!s_running) {
        return;
    }
    /* detach from Layer 4 first to stop new events/frames from enqueuing */
    a2dp_sink_offload_register_report_cb(NULL);
    a2dp_sink_offload_register_ctrl_cb(NULL);
    s_running = false;
    /* task exits and deletes itself once it sees s_running=false after pop */
    rtos_deinit_queue(&s_queue);
    if (s_playing) {
#if CONFIG_AUDIO_PLAY
        a2dp_sink_audio_stop();
#endif
        s_playing = false;
    }
    LOGI("cp a2dp sink offload player deinited\n");
}

#endif /* CONFIG_BLUETOOTH_A2DP_OFFLOAD_SINK */
