#include <stdio.h>
#include <stdlib.h>
#include "os/os.h"
#include "os/mem.h"
#include "os/str.h"
#include <modules/pm.h>
#include <components/log.h>


#include "components/bluetooth/bk_dm_bluetooth_types.h"
#include "components/bluetooth/bk_dm_a2dp_types.h"
#include "components/bluetooth/bk_dm_a2dp.h"
#include "modules/mp3dec.h"
#include "modules/sbc_encoder.h"
#include "modules/audio_rsp_types.h"
#include "modules/audio_rsp.h"
#include "ring_buffer_particle.h"
#include "common/bk_assert.h"

//#include "audio_osi_wrapper.h"

#if CONFIG_VFS
    #include "bk_posix.h"
    #include "ring_buffer_particle.h"
#elif CONFIG_FATFS
    #include "diskio.h"
    #include "ff.h"
#endif
#include "modules/pm.h"
#include "driver/pwr_clk.h"
#include "a2dp_source_demo.h"
#include "a2dp_source_demo_avrcp.h"
#include "bk_a2dp_source_pcm_service.h"
#include "bk_a2dp_source_service.h"
#include "bt_manager.h"
#include "components/bluetooth/bk_dm_gap_bt.h"

#define PCM_CALL_METHOD_DIR 1
#define PCM_CALL_METHOD_SMP 3

#define PCM_CALL_METHOD PCM_CALL_METHOD_SMP

#define A2DP_CPU_FRQ PM_CPU_FRQ_320M

typedef struct
{
    uint8_t addr[6];
} device_addr_t;

typedef struct
{
    uint8_t inited;               /* preserved across ACL-disconnect memset */
    uint8_t discovery_status;     /* GAP inquiry state (demo) */
    device_addr_t peer_addr;
    uint8_t conn_state;           /* mirrored from a2dp source service events */
    uint8_t start_status;         /* mirrored from STREAM_START/SUSPEND events */
    uint8_t read_cb_pause;        /* demo pause flag, drives play-status reporting */
    uint8_t connect_issued;       /* an a2dp connect was already started this link (user or auto) */
    beken2_timer_t autoconn_timer; /* grace timer to auto-connect A2DP if the peer stays idle */

    /* ---- file read + MP3 decode state (moved from file-scope globals) ---- */
    beken_thread_t decode_thread_handle;
#if USER_A2DP_MAIN_TASK
    beken_thread_t main_thread_handle;
#endif
    FIL mp3_file;
    uint8_t file_path[64];
    MP3FrameInfo mp3_frame_info;
    uint32_t decode_trigger_size; /* 44.1khz buffer size = 176400 * DECODE_TRIGGER_TIME Bytes */
    uint8_t decode_task_run;
    uint8_t is_bk_aud_rsp_inited; /* dead flag kept for test_performance */
} a2dp_source_player_ctx_t;

enum
{
    BT_A2DP_SOURCE_MSG_READ_PCM_FROM_BUFF = 1,
};

enum
{
    A2DP_SOURCE_DEBUG_LEVEL_ERROR,
    A2DP_SOURCE_DEBUG_LEVEL_WARNING,
    A2DP_SOURCE_DEBUG_LEVEL_INFO,
    A2DP_SOURCE_DEBUG_LEVEL_DEBUG,
    A2DP_SOURCE_DEBUG_LEVEL_VERBOSE,
};



#define USER_A2DP_MAIN_TASK 0
#define TASK_PRIORITY (BEKEN_DEFAULT_WORKER_PRIORITY)
#define DISCONNECT_REASON_REMOTE_USER_TERMINATE 0x13
#define MP3_DECODE_BUFF_SIZE (MAX_NSAMP * MAX_NCHAN * MAX_NGRAN * sizeof(uint16_t))
#define DECODE_TRIGGER_TIME (50) //ms
#define DECODE_RB_SIZE ((s_a2dp_source_player_ctx.decode_trigger_size > MP3_DECODE_BUFF_SIZE ? s_a2dp_source_player_ctx.decode_trigger_size : MP3_DECODE_BUFF_SIZE) + MP3_DECODE_BUFF_SIZE + 1)
#define A2DP_SOURCE_WRITE_AUTO_TIMER_MS 30
#define SBC_SAMPLE_DEPTH 16

#if DECODE_TRIGGER_TIME <= A2DP_SOURCE_WRITE_AUTO_TIMER_MS
    #error "DECODE_TRIGGER_TIME must > A2DP_SOURCE_WRITE_AUTO_TIMER_MS !!!"
#endif

#define CONNECTION_PACKET_TYPE 0xcc18
#define CONNECTION_PAGE_SCAN_REPETITIOIN_MODE 0x01
#define CONNECTION_CLOCK_OFFSET 0x00

#define A2DP_SOURCE_DEBUG_LEVEL A2DP_SOURCE_DEBUG_LEVEL_INFO

#define LOGE(format, ...) do{if(A2DP_SOURCE_DEBUG_LEVEL >= A2DP_SOURCE_DEBUG_LEVEL_ERROR)   BK_LOGE("a2dp_s", "%s:" format "\n", __func__, ##__VA_ARGS__);}while(0)
#define LOGW(format, ...) do{if(A2DP_SOURCE_DEBUG_LEVEL >= A2DP_SOURCE_DEBUG_LEVEL_WARNING) BK_LOGW("a2dp_s", "%s:" format "\n", __func__, ##__VA_ARGS__);}while(0)
#define LOGI(format, ...) do{if(A2DP_SOURCE_DEBUG_LEVEL >= A2DP_SOURCE_DEBUG_LEVEL_INFO)    BK_LOGI("a2dp_s", "%s:" format "\n", __func__, ##__VA_ARGS__);}while(0)
#define LOGD(format, ...) do{if(A2DP_SOURCE_DEBUG_LEVEL >= A2DP_SOURCE_DEBUG_LEVEL_DEBUG)   BK_LOGI("a2dp_s", "%s:" format "\n", __func__, ##__VA_ARGS__);}while(0)
#define LOGV(format, ...) do{if(A2DP_SOURCE_DEBUG_LEVEL >= A2DP_SOURCE_DEBUG_LEVEL_VERBOSE) BK_LOGI("a2dp_s", "%s:" format "\n", __func__, ##__VA_ARGS__);}while(0)

extern bk_err_t bk_audio_osi_funcs_init(void);
static bk_err_t a2dp_source_demo_create_mp3_decode_task(void);
static bk_err_t a2dp_source_demo_stop_mp3_decode_task(void);
static void on_source_service_evt(bk_a2dp_source_service_evt_t evt, void *arg, void *user_data);


static a2dp_source_player_ctx_t s_a2dp_source_player_ctx;

/* Central is the A2DP source. On a fresh link, give the peer a short grace
 * period to bring A2DP up itself; only if it stays idle do we initiate it, so
 * we never race a peer-initiated stream (which would otherwise collide on
 * SET_CONFIG -- see the AVDTP_CONNECT_IND handling in bt_ui.c). */
#define A2DP_AUTOCONNECT_GRACE_MS   4000

static void a2dp_source_autoconnect_timer_hdl(void *larg, void *rarg)
{
    uint8_t *dev;

    (void)larg;
    (void)rarg;

    /* A2DP source (and bt_manager's peer tracking) is single-connection, so one
     * timer is enough. On expiry serve whichever device is CURRENTLY connected,
     * queried live from bt_manager -- this is what keeps things correct when
     * several devices connect around the same time (no stale stored address). */
    if (s_a2dp_source_player_ctx.connect_issued ||
        s_a2dp_source_player_ctx.conn_state != BK_A2DP_CONNECTION_STATE_DISCONNECTED)
    {
        return; /* peer brought A2DP up, or we already started it for a device */
    }

    dev = bt_manager_get_connected_device();
    if (!dev || 0 == (dev[0] | dev[1] | dev[2] | dev[3] | dev[4] | dev[5]))
    {
        return; /* no active device to serve */
    }

    LOGI("peer left A2DP idle, auto-connect source to %02x:%02x:%02x:%02x:%02x:%02x",
         dev[5], dev[4], dev[3], dev[2], dev[1], dev[0]);
    /* Non-blocking: this runs in the timer service thread; blocking here would
     * stall the BT stack's software timers and wedge media-channel setup. */
    s_a2dp_source_player_ctx.connect_issued = 1;
    bk_a2dp_source_service_connect_async(dev);
}

static void a2dp_source_arm_autoconnect(void)
{
    if (!rtos_is_oneshot_timer_init(&s_a2dp_source_player_ctx.autoconn_timer))
    {
        if (rtos_init_oneshot_timer(&s_a2dp_source_player_ctx.autoconn_timer, A2DP_AUTOCONNECT_GRACE_MS,
                                    (timer_2handler_t)a2dp_source_autoconnect_timer_hdl, NULL, NULL))
        {
            LOGE("a2dp autoconnect timer init fail");
            return;
        }
    }

    if (rtos_is_oneshot_timer_running(&s_a2dp_source_player_ctx.autoconn_timer))
    {
        rtos_stop_oneshot_timer(&s_a2dp_source_player_ctx.autoconn_timer);
    }

    rtos_start_oneshot_timer(&s_a2dp_source_player_ctx.autoconn_timer);
}

static void a2dp_source_cancel_autoconnect(void)
{
    if (rtos_is_oneshot_timer_init(&s_a2dp_source_player_ctx.autoconn_timer))
    {
        if (rtos_is_oneshot_timer_running(&s_a2dp_source_player_ctx.autoconn_timer))
        {
            rtos_stop_oneshot_timer(&s_a2dp_source_player_ctx.autoconn_timer);
        }
        /* pair the init() done in arm(); arm() re-inits on the next link */
        rtos_deinit_oneshot_timer(&s_a2dp_source_player_ctx.autoconn_timer);
    }
}

static void *mp3_private_alloc(size_t size)
{
    return os_malloc(size);
}

static void mp3_private_free(void *buff)
{
    os_free(buff);
}

static void *mp3_private_memset(void *s, unsigned char c, size_t n)
{
    return os_memset(s, c, n);
}

static void *mp3_private_alloc_psram(size_t size)
{
    return psram_malloc(size);
}

static void mp3_private_free_psram(void *buff)
{
    psram_free(buff);
}

static void *mp3_private_memset_psram(void *s, unsigned char c, size_t n)
{
    os_memset_word((uint32_t *)s, c, n);
    return s;
}

static void bt_api_event_cb(bk_gap_bt_cb_event_t event, bk_bt_gap_cb_param_t *param)
{
    /*
     * Link key (NOTIF/REQ), AUTH and ACL connection events are now owned by
     * bt_manager (gap_event_cb) with bluetooth_storage persistence. This demo
     * handler only keeps inquiry results, and is chained via
     * bt_manager_register_callback() (fan-out) so it never takes the single
     * GAP callback slot away from bt_manager.
     */
    switch (event)
    {
    case BK_BT_GAP_DISC_RES_EVT:
    {
        bk_bt_gap_cb_param_t *cb = (typeof(cb))param;

        char name[256] = {0};
        int8_t rssi = 0;
        uint32_t cod = 0;
        uint16_t eir_data_len = 0;

        uint8_t found_rssi = 0;
        uint8_t found_name = 0;

        for (int i = 0; i < cb->disc_res.num_prop; ++i)
        {
            if (cb->disc_res.prop[i].type == BK_BT_GAP_DEV_PROP_BDNAME)
            {
                found_name = 1;
                os_memcpy(name, cb->disc_res.prop[i].val, cb->disc_res.prop[i].len < sizeof(name) - 1 ? cb->disc_res.prop[i].len : sizeof(name) - 1);
            }
            else if (cb->disc_res.prop[i].type == BK_BT_GAP_DEV_PROP_COD)
            {
                os_memcpy(&cod, cb->disc_res.prop[i].val, cb->disc_res.prop[i].len);
            }
            else if (cb->disc_res.prop[i].type == BK_BT_GAP_DEV_PROP_RSSI)
            {
                found_rssi = 1;
                os_memcpy(&rssi, cb->disc_res.prop[i].val, sizeof(rssi));
            }
            else if (cb->disc_res.prop[i].type == BK_BT_GAP_DEV_PROP_EIR)
            {
                eir_data_len = cb->disc_res.prop[i].len;

                uint8_t *tmp_buff = cb->disc_res.prop[i].val;

                for (uint32_t j = 0; j < eir_data_len;)
                {
                    uint8_t len = tmp_buff[j++];
                    uint8_t type = tmp_buff[j++];
                    uint8_t *tmp_data = tmp_buff + j;

                    j = j + len - 1;

                    if (type == BK_BT_EIR_TYPE_SHORT_LOCAL_NAME || type == BK_BT_EIR_TYPE_CMPL_LOCAL_NAME)
                    {
                        found_name = 1;
                        os_memcpy(name, tmp_data, len - 1 < sizeof(name) - 1 ? len - 1 : sizeof(name) - 1);
                    }
                }
            }
        }

#define LOG_BUFFER_LEN 512
        char *log_buff = psram_malloc(LOG_BUFFER_LEN);

        if (!log_buff)
        {
            LOGE("can't alloc log buff");
            break;
        }

        uint32_t index = 0;

        index += snprintf(log_buff + index, LOG_BUFFER_LEN - 1 - index, "BK_BT_GAP_DISC_RES_EVT %02X:%02X:%02X:%02X:%02X:%02X prop count %d cod 0x%06x",
                          cb->disc_res.bda[5],
                          cb->disc_res.bda[4],
                          cb->disc_res.bda[3],
                          cb->disc_res.bda[2],
                          cb->disc_res.bda[1],
                          cb->disc_res.bda[0],
                          cb->disc_res.num_prop,
                          cod);

        if (found_rssi)
        {
            index += snprintf(log_buff + index, LOG_BUFFER_LEN - 1 - index, " rssi %d", rssi);
        }

        if (found_name)
        {
            index += snprintf(log_buff + index, LOG_BUFFER_LEN - 1 - index, " name %s", name);
        }

        // if (eir_data_len)
        // {
        //     index += snprintf(log_buff + index, LOG_BUFFER_LEN - 1 - index, " eir len %d", eir_data_len);
        // }

        LOGI("%s", log_buff);

        if (log_buff)
        {
            psram_free(log_buff);
            log_buff = NULL;
        }
    }
    break;

    case BK_BT_GAP_DISC_STATE_CHANGED_EVT:
    {
        bk_bt_gap_cb_param_t *cb = (typeof(cb))param;
        LOGI("discovery change %d", cb->disc_st_chg.state);
        s_a2dp_source_player_ctx.discovery_status = cb->disc_st_chg.state;
    }

    break;

    case BK_BT_GAP_AUTH_CMPL_EVT:
    {
        bk_bt_gap_cb_param_t *cb = (typeof(cb))param;

        /* Central is the A2DP *source*; on link-ready (auth/encryption done)
         * proactively bring up A2DP if it is still idle and nobody started it
         * yet. Some peers reconnect and only set up HFP, never initiating A2DP
         * themselves -- without this the source side would stay unconnected.
         * Starting here (right after auth) usually makes us the AVDTP initiator;
         * if the peer initiates first, the AVDTP_CONNECT_IND path in bt_ui.c
         * yields to it, so the two do not collide. */
        if (cb->auth_cmpl.stat == 0 &&
            !s_a2dp_source_player_ctx.connect_issued &&
            s_a2dp_source_player_ctx.conn_state == BK_A2DP_CONNECTION_STATE_DISCONNECTED)
        {
            LOGI("auth ok, arm A2DP source auto-connect in %d ms", A2DP_AUTOCONNECT_GRACE_MS);
            a2dp_source_arm_autoconnect();
        }
    }
    break;

    default:
        break;
    }

}

/*
 * Register the demo GAP handler once through bt_manager (fan-out) so link keys
 * stay owned by bt_manager/bluetooth_storage while the demo still sees inquiry
 * results. Never calls bk_bt_gap_register_callback (single-slot) directly.
 */
static void demo_ensure_gap_cb(void)
{
    static uint8_t registered = 0;

    if (!registered)
    {
        btm_callback_s cb = {0};
        cb.gap_cb = bt_api_event_cb;
        bt_manager_register_callback(&cb);
        registered = 1;
    }
}

static void on_source_service_evt(bk_a2dp_source_service_evt_t evt, void *arg, void *user_data)
{
    (void)user_data;

    switch (evt)
    {
    case BK_A2DP_SOURCE_SERVICE_EVT_CONNECTED:
        s_a2dp_source_player_ctx.conn_state = BK_A2DP_CONNECTION_STATE_CONNECTED;

        if (arg)
        {
            os_memcpy(s_a2dp_source_player_ctx.peer_addr.addr, arg, sizeof(s_a2dp_source_player_ctx.peer_addr.addr));
        }

        LOGI("a2dp source connected");
        break;

    case BK_A2DP_SOURCE_SERVICE_EVT_DISCONNECTED:
        s_a2dp_source_player_ctx.conn_state = BK_A2DP_CONNECTION_STATE_DISCONNECTED;
        s_a2dp_source_player_ctx.start_status = 0;
        s_a2dp_source_player_ctx.connect_issued = 0;
        a2dp_source_cancel_autoconnect();
        LOGI("a2dp source disconnected");
        a2dp_source_demo_stop_mp3_decode_task();
        break;

    case BK_A2DP_SOURCE_SERVICE_EVT_AUDIO_CFG:
        /* negotiated SBC codec is stored inside the service; nothing to do here */
        break;

    case BK_A2DP_SOURCE_SERVICE_EVT_STREAM_START:
        s_a2dp_source_player_ctx.start_status = 1;
        a2dp_source_demo_create_mp3_decode_task();
        break;

    case BK_A2DP_SOURCE_SERVICE_EVT_STREAM_SUSPEND:
        s_a2dp_source_player_ctx.start_status = 0;
        a2dp_source_demo_stop_mp3_decode_task();
        break;

    default:
        break;
    }
}

int bt_a2dp_source_demo_discover(uint32_t sec, uint32_t num_report)
{
    demo_ensure_gap_cb();
    return bt_manager_discover_bt(sec, num_report);
}

int bt_a2dp_source_demo_discover_cancel(void)
{
    demo_ensure_gap_cb();
    return bt_manager_cancel_discover_bt();
}

int bt_a2dp_source_demo_init(void)
{
    /* Eager profile init: register the a2dp source + avrcp profiles at startup so a
     * peer-initiated connection (a speaker connecting to us) is accepted and we get
     * the CONNECTED / AUDIO_CFG events (music_play relies on conn_state == CONNECTED). */
    bk_a2dp_source_service_register_event_cb(on_source_service_evt, NULL);
    bk_a2dp_source_service_init(NULL);
    demo_ensure_gap_cb();
    bt_avrcp_demo_init();
    return 0;
}

int bt_a2dp_source_demo_connect(uint8_t *addr)
{
    /* register app-facing callback; GAP (link key) is owned by bt_manager now */
    bk_a2dp_source_service_register_event_cb(on_source_service_evt, NULL);

    if (s_a2dp_source_player_ctx.discovery_status != BK_BT_GAP_DISCOVERY_STOPPED)
    {
        LOGE("currently is discovering");
        return 0;
    }

    /* service owns profile init + a2dp connect + waiting for CONNECTED/AUDIO_CFG */
    s_a2dp_source_player_ctx.connect_issued = 1;
    return bk_a2dp_source_service_connect(addr);
}


int bt_a2dp_source_demo_disconnect(uint8_t *addr)
{
    (void)addr;
    /* service tracks the connected peer internally */
    return bk_a2dp_source_service_disconnect();
}

/* bt_a2dp_source_demo_set_linkkey removed: BLE->BR/EDR key handoff now goes
 * through bt_manager_set_tmp_linkkey() / bluetooth_storage (see dm_gatt.c). */

/*
 * Legacy demo pipeline (avdtp start/suspend, the data/encode/resample stack
 * callbacks and the software SBC encoder) has been migrated into the component:
 *   - bk_a2dp_source_service : stream_start/suspend + the three stack callbacks
 *   - bk_a2dp_source_pcm_service        : resample + SBC encode worker
 */

int bt_a2dp_source_demo_music_play(uint8_t is_mp3, uint8_t *file_path)
{
    bk_err_t error = 0;

    if (file_path && file_path[0])
    {
        os_strncpy((char *)s_a2dp_source_player_ctx.file_path, (char *)file_path, sizeof(s_a2dp_source_player_ctx.file_path) - 1);
    }

    if (s_a2dp_source_player_ctx.file_path[0] == 0)
    {
        LOGE("file_path err");
        error = -1;
        goto error;
    }

    if (s_a2dp_source_player_ctx.conn_state != BK_A2DP_CONNECTION_STATE_CONNECTED)
    {
        LOGE("not connected");
        error = 0;
        goto error;
    }

    error = a2dp_source_demo_create_mp3_decode_task();

    if (error)
    {
        LOGE("create task err %d", error);
        goto error;
    }

    error = bk_a2dp_source_service_stream_start();

    if (error)
    {
        LOGE("start err!!!");
        goto error;
    }

    s_a2dp_source_player_ctx.read_cb_pause = 0;
    bk_a2dp_source_service_pcm_pause(false);
    bt_avrcp_demo_report_track_change();
    return 0;
error:;
    bt_a2dp_source_demo_music_stop();
    return error;
}

int bt_a2dp_source_demo_music_stop(void)
{
    int32_t ret = bk_a2dp_source_service_stream_suspend();

    //a2dp_source_demo_stop_mp3_decode_task();

    return ret;
}

int bt_a2dp_source_demo_music_pause(void)
{
    s_a2dp_source_player_ctx.read_cb_pause = 1;
    bk_a2dp_source_service_pcm_pause(true);
    return 0;
}

int bt_a2dp_source_demo_music_resume(void)
{
    s_a2dp_source_player_ctx.read_cb_pause = 0;
    bk_a2dp_source_service_pcm_pause(false);
    return 0;
}

int bt_a2dp_source_demo_music_prev(void)
{
    LOGW("currently not impl music prev and next function, so play same music now.");
    bt_a2dp_source_demo_music_stop();

    return bt_a2dp_source_demo_music_play(1, s_a2dp_source_player_ctx.file_path);
}

int bt_a2dp_source_demo_music_next(void)
{
    LOGW("currently not impl music prev and next function, so play same music now.");
    bt_a2dp_source_demo_music_stop();

    return bt_a2dp_source_demo_music_play(1, s_a2dp_source_player_ctx.file_path);
}

int bt_a2dp_source_demo_get_play_status(void)
{
    if (s_a2dp_source_player_ctx.conn_state == BK_A2DP_CONNECTION_STATE_CONNECTED &&
            s_a2dp_source_player_ctx.read_cb_pause == 0 &&
            s_a2dp_source_player_ctx.start_status &&
            s_a2dp_source_player_ctx.decode_thread_handle)
    {
        return A2DP_PLAY_STATUS_PLAYING;
    }

    if (s_a2dp_source_player_ctx.conn_state == BK_A2DP_CONNECTION_STATE_CONNECTED &&
            s_a2dp_source_player_ctx.read_cb_pause &&
            s_a2dp_source_player_ctx.start_status &&
            s_a2dp_source_player_ctx.decode_thread_handle)
    {
        return A2DP_PLAY_STATUS_PAUSED;
    }

    return A2DP_PLAY_STATUS_STOPPED;
}

uint32_t bt_a2dp_source_demo_get_play_pos(void)
{
    if (bt_a2dp_source_demo_get_play_status() == A2DP_PLAY_STATUS_STOPPED)
    {
        return 0xFFFFFFFF;
    }
    else
    {
        //todo: not impl
        return 0;
    }
}

static int32_t get_mp3_info(MP3FrameInfo *info)
{
    //FRESULT fr = 0;
    int32_t fr = 0;
    char full_path[64] = {0};
    uint8_t tag_header[10] = {0};
    unsigned int num_rd = 0;
    uint8_t *mp3_read_start_ptr = NULL;
    uint8_t *pcm_write_ptr = NULL;
    int bytesleft = 0;
    uint8_t *current_mp3_read_ptr = NULL;
    uint8_t id3_maj_ver = 0;
    uint16_t id3_min_ver = 0;
    uint32_t file_size = 0;
#if CONFIG_VFS
    int32_t fd = -1;
#else
    FATFS *s_pfs = NULL;
#endif
    uint32_t frame_start_offset = 0;
    int ret = 0;
    HMP3Decoder *s_mp3_decoder = NULL;

    os_memset(info, 0, sizeof(*info));
#if CONFIG_VFS
    struct bk_fatfs_partition partition = {0};

    partition.part_type = FATFS_DEVICE;
    partition.part_dev.device_name = FATFS_DEV_SDCARD;
    partition.mount_path = VFS_SD_0_PATITION_0;

    fr = mount("SOURCE_NONE", partition.mount_path, "fatfs", 0, &partition);

    if (fr < 0)
    {
        LOGE("mount failed:%d", fr);
        goto error;
    }

#else
    s_pfs = os_malloc(sizeof(*s_pfs));

    if (!s_pfs)
    {
        LOGE("s_pfs malloc failed!");
        goto error;
    }

    os_memset(s_pfs, 0, sizeof(*s_pfs));

    fr = f_mount(s_pfs, "1:", 1);

    if (fr != FR_OK)
    {
        LOGE("f_mount failed:%d", fr);
        goto error;
    }

#endif

    LOGI("f_mount OK!");

    MP3SetBuffMethod(mp3_private_alloc, mp3_private_free, mp3_private_memset);
    //MP3SetBuffMethodAlwaysFourAlignedAccess(mp3_private_alloc_psram, mp3_private_free_psram, mp3_private_memset_psram);

    s_mp3_decoder = MP3InitDecoder();

    if (!s_mp3_decoder)
    {
        LOGE("s_mp3_decoder MP3InitDecoder failed!");
        goto error;
    }

    LOGI("MP3InitDecoder init successful!");

    /*open file to read mp3 data */
#if CONFIG_VFS
    sprintf((char *)full_path, "%s/%s", VFS_SD_0_PATITION_0, s_a2dp_source_player_ctx.file_path);
    fd = open(full_path, O_RDONLY);

    if (fd < 0)
    {
        LOGE("open %s failed, ret %d", full_path, ret);
        goto error;
    }

    fr = read(fd, (void *)tag_header, sizeof(tag_header));

    if (fr < 0 || fr != sizeof(tag_header))
    {
        LOGE("read %s failed! fr %d", full_path, fr);
        goto error;
    }

#else
    sprintf((char *)full_path, "%d:/%s", DISK_NUMBER_SDIO_SD, s_a2dp_source_player_ctx.file_path);
    fr = f_open(&s_a2dp_source_player_ctx.mp3_file, full_path, FA_OPEN_EXISTING | FA_READ);

    if (fr != FR_OK)
    {
        LOGE("open %s failed!", full_path);
        goto error;
    }

    fr = f_read(&s_a2dp_source_player_ctx.mp3_file, (void *)tag_header, sizeof(tag_header), &num_rd);

    if (fr != FR_OK || num_rd != sizeof(tag_header))
    {
        LOGE("read %s failed!", full_path);
        goto error;
    }

#endif

    do
    {
        uint8_t id3v1[128] = {0};

#if CONFIG_VFS
        struct stat statbuf = {0};
        fr = stat(full_path, &statbuf);

        if (fr < 0)
        {
            LOGE("stat err %d", fr);
            goto error;
        }

        file_size = statbuf.st_size;
#else

        FILINFO info = {0};

        fr = f_stat(full_path, &info);

        if (fr != FR_OK)
        {
            LOGE("f_stat err %d", fr);
            goto error;
        }

        file_size = info.fsize;
#endif

        if (os_memcmp(tag_header, "ID3", 3) == 0)
        {
            uint32_t tag_size = ((tag_header[6] & 0x7F) << 21) | ((tag_header[7] & 0x7F) << 14) | ((tag_header[8] & 0x7F) << 7) | (tag_header[9] & 0x7F);
            frame_start_offset = sizeof(tag_header) + tag_size;

            id3_min_ver = ((tag_header[4] << 8) | tag_header[3]);
            id3_maj_ver = 2;

            LOGI("ID3v2.%d flag 0x%x len %d", id3_min_ver, tag_header[5], tag_size);

            if (id3_min_ver == 4 && (tag_header[5] & (1 << 4)))
            {
                frame_start_offset += 10;
            }
        }

#if CONFIG_VFS
        fr = lseek(fd, file_size - sizeof(id3v1), SEEK_SET);

        if (fr < 0)
        {
            LOGE("lseek err %d", fr);
            goto error;
        }

        fr = read(fd, id3v1, sizeof(id3v1));

        if (fr < 0 || fr != sizeof(id3v1))
        {
            LOGE("read %s id3v1 failed! fr %d", full_path, fr);
            goto error;
        }

#else
        fr = f_lseek(&s_a2dp_source_player_ctx.mp3_file, file_size - sizeof(id3v1));

        if (fr != FR_OK)
        {
            LOGE("f_lseek to ID3v1 end err %d", fr);
            goto error;
        }

        fr = f_read(&s_a2dp_source_player_ctx.mp3_file, id3v1, sizeof(id3v1), &num_rd);

        if (fr != FR_OK || num_rd != sizeof(id3v1))
        {
            LOGE("read ID3v1 err %d num %d", fr, num_rd);
            goto error;
        }

#endif

        if (os_memcmp(id3v1, "TAG", 3))
        {
            LOGD("ID3v1 not found!");
            break;
        }

        LOGI("found ID3v1");

        if (!id3_maj_ver)
        {
            id3_maj_ver = 1;
        }
    }
    while (0);

#if CONFIG_VFS
    lseek(fd, frame_start_offset, SEEK_SET);
#else
    f_lseek(&s_a2dp_source_player_ctx.mp3_file, frame_start_offset);
#endif
    LOGI("mp3 file open successfully!");

    mp3_read_start_ptr = os_malloc(MAINBUF_SIZE * 2);

    if (!mp3_read_start_ptr)
    {
        LOGE("mp3_read_ptr alloc err");
        goto error;
    }

    pcm_write_ptr = os_malloc(MP3_DECODE_BUFF_SIZE);

    if (!pcm_write_ptr)
    {
        LOGE("pcm_write_ptr alloc err");
        goto error;
    }

    //get frame info
    {
#if CONFIG_VFS
        fr = read(fd, mp3_read_start_ptr, MAINBUF_SIZE);

        if (fr < 0)
        {
            LOGE("lseek err %d", fr);
            goto error;
        }

#else
        fr = f_read(&s_a2dp_source_player_ctx.mp3_file, mp3_read_start_ptr, MAINBUF_SIZE, &num_rd);

        if (fr != FR_OK)
        {
            LOGE("test read frame %d %s failed!", num_rd, full_path);
            goto error;
        }

#endif
        bytesleft = MP3_DECODE_BUFF_SIZE;

        current_mp3_read_ptr = mp3_read_start_ptr;

        ret = MP3Decode(s_mp3_decoder, &current_mp3_read_ptr, &bytesleft, (int16_t *)pcm_write_ptr, 0);

        if (ret != ERR_MP3_NONE)
        {
            LOGE("MP3Decode failed, code is %d bytesleft %d", ret, bytesleft);
            goto error;
        }

        MP3GetLastFrameInfo(s_mp3_decoder, &s_a2dp_source_player_ctx.mp3_frame_info);
#if CONFIG_VFS
        uint32_t left_byte = ftell(fd);
#else
        uint32_t left_byte = f_tell(&s_a2dp_source_player_ctx.mp3_file);
#endif
        LOGI("bytesleft %d readsize %d", bytesleft, left_byte);
        LOGI("Bitrate: %d kb/s, Samprate: %d, Samplebits %d", (s_a2dp_source_player_ctx.mp3_frame_info.bitrate) / 1000, s_a2dp_source_player_ctx.mp3_frame_info.samprate, s_a2dp_source_player_ctx.mp3_frame_info.bitsPerSample);
        LOGI("Channel: %d, Version: %d, Layer: %d", s_a2dp_source_player_ctx.mp3_frame_info.nChans, s_a2dp_source_player_ctx.mp3_frame_info.version, s_a2dp_source_player_ctx.mp3_frame_info.layer);
        LOGI("OutputSamps: %d %d", s_a2dp_source_player_ctx.mp3_frame_info.outputSamps, s_a2dp_source_player_ctx.mp3_frame_info.outputSamps * s_a2dp_source_player_ctx.mp3_frame_info.bitsPerSample / 8);
#if CONFIG_VFS
        lseek(fd, frame_start_offset, SEEK_SET);
#else
        f_lseek(&s_a2dp_source_player_ctx.mp3_file, frame_start_offset);
#endif
    }

error:;

    if (s_mp3_decoder)
    {
        MP3FreeDecoder(s_mp3_decoder);
        s_mp3_decoder = NULL;
    }

#if CONFIG_VFS

    if (fd >= 0)
    {
        close(fd);
        fd = -1;
    }

    fr = umount(VFS_SD_0_PATITION_0);

    if (fr < 0)
    {
        LOGE("umount err %d", fr);
    }

#else

    if (s_a2dp_source_player_ctx.mp3_file.fs)
    {
        f_close(&s_a2dp_source_player_ctx.mp3_file);
        os_memset(&s_a2dp_source_player_ctx.mp3_file, 0, sizeof(s_a2dp_source_player_ctx.mp3_file));
    }

    if (s_pfs)
    {
        fr = f_unmount(DISK_NUMBER_SDIO_SD, "1:", 1);

        if (fr)
        {
            LOGE("f_unmount err %d", fr);
        }

        os_free(s_pfs);
        s_pfs = NULL;
    }

#endif

    if (pcm_write_ptr)
    {
        os_free(pcm_write_ptr);
        pcm_write_ptr = NULL;
    }

    if (mp3_read_start_ptr)
    {
        os_free(mp3_read_start_ptr);
        mp3_read_start_ptr = NULL;
    }

    return 0;
}

static void bt_a2dp_source_decode_task(void *arg)
{
    //FRESULT fr = 0;
    int32_t fr = 0;

    char full_path[64] = {0};
    uint8_t tag_header[10] = {0};
    unsigned int num_rd = 0;
    uint8_t *mp3_read_start_ptr = NULL;
    uint8_t *pcm_write_ptr = NULL;
    int bytesleft = 0;
    uint8_t *current_mp3_read_ptr = NULL;
    uint8_t *mp3_read_end_ptr = NULL;
    uint8_t id3_maj_ver = 0;
    uint16_t id3_min_ver = 0;
    uint8_t has_id3v1 = 0;
    uint32_t file_size = 0;
    uint32_t pcm_decode_size = 0;

    uint32_t frame_start_offset = 0;
    int ret = 0;
    MP3FrameInfo tmp_mp3_frame_info;
    HMP3Decoder *s_mp3_decoder = NULL;
    uint8_t *task_ctrl = (typeof(task_ctrl))arg;

#if CONFIG_VFS
    int32_t fd = -1;
#else
    FATFS *s_pfs = NULL;
#endif

    *task_ctrl = 1;
    os_memset(&tmp_mp3_frame_info, 0, sizeof(tmp_mp3_frame_info));

#if CONFIG_VFS
    struct bk_fatfs_partition partition = {0};
    partition.part_type = FATFS_DEVICE;
    partition.part_dev.device_name = FATFS_DEV_SDCARD;
    partition.mount_path = VFS_SD_0_PATITION_0;
    fr = mount("SOURCE_NONE", partition.mount_path, "fatfs", 0, &partition);

    if (fr < 0)
    {
        LOGE("mount failed:%d", fr);
        goto error;
    }

    LOGI("mount ok!");

#else
    s_pfs = os_malloc(sizeof(*s_pfs));

    if (NULL == s_pfs)
    {
        LOGE("s_pfs malloc failed!");
        goto error;
    }

    os_memset(s_pfs, 0, sizeof(*s_pfs));

    fr = f_mount(s_pfs, "1:", 1);

    if (fr != FR_OK)
    {
        LOGE("f_mount failed:%d", fr);
        goto error;
    }

    //can't free !!!!!
    //    os_free(s_pfs);
    //    s_pfs = NULL;

    LOGI("f_mount OK!");
#endif

    MP3SetBuffMethod(mp3_private_alloc, mp3_private_free, mp3_private_memset);
    //MP3SetBuffMethodAlwaysFourAlignedAccess(mp3_private_alloc_psram, mp3_private_free_psram, mp3_private_memset_psram);

    s_mp3_decoder = MP3InitDecoder();

    if (!s_mp3_decoder)
    {
        LOGE("s_mp3_decoder MP3InitDecoder failed!");
        goto error;
    }

    LOGI("MP3InitDecoder init successful!");

    /*open file to read mp3 data */

#if CONFIG_VFS
    sprintf((char *)full_path, "%s/%s", VFS_SD_0_PATITION_0, s_a2dp_source_player_ctx.file_path);
    fd = open(full_path, O_RDONLY);

    if (fd < 0)
    {
        LOGE("open %s failed, ret %d", full_path, ret);
        goto error;
    }

    fr = read(fd, (void *)tag_header, sizeof(tag_header));

    if (fr < 0 || fr != sizeof(tag_header))
    {
        LOGE("read %s failed! fr %d", full_path, fr);
        goto error;
    }

#else
    sprintf((char *)full_path, "%d:/%s", DISK_NUMBER_SDIO_SD, s_a2dp_source_player_ctx.file_path);
    fr = f_open(&s_a2dp_source_player_ctx.mp3_file, full_path, FA_OPEN_EXISTING | FA_READ);

    if (fr != FR_OK)
    {
        LOGE("open %s failed!", full_path);
        goto error;
    }

    fr = f_read(&s_a2dp_source_player_ctx.mp3_file, (void *)tag_header, sizeof(tag_header), &num_rd);

    if (fr != FR_OK || num_rd != sizeof(tag_header))
    {
        LOGE("read %s failed!", full_path);
        goto error;
    }

#endif

    do
    {
        uint8_t id3v1[128] = {0};
#if CONFIG_VFS
        struct stat statbuf = {0};
        fr = stat(full_path, &statbuf);

        if (fr < 0)
        {
            LOGE("stat err %d", fr);
            goto error;
        }

        file_size = statbuf.st_size;
#else

        FILINFO info = {0};

        fr = f_stat(full_path, &info);

        if (fr != FR_OK)
        {
            LOGE("f_stat err %d", fr);
            goto error;
        }

        file_size = info.fsize;
#endif

        if (os_memcmp(tag_header, "ID3", 3) == 0)
        {
            uint32_t tag_size = ((tag_header[6] & 0x7F) << 21) | ((tag_header[7] & 0x7F) << 14) | ((tag_header[8] & 0x7F) << 7) | (tag_header[9] & 0x7F);
            frame_start_offset = sizeof(tag_header) + tag_size;

            id3_min_ver = ((tag_header[4] << 8) | tag_header[3]);
            id3_maj_ver = 2;

            LOGI("ID3v2.%d flag 0x%x len %d", id3_min_ver, tag_header[5], tag_size);

            if (id3_min_ver == 4 && (tag_header[5] & (1 << 4)))
            {
                frame_start_offset += 10;
            }
        }

#if CONFIG_VFS
        fr = lseek(fd, file_size - sizeof(id3v1), SEEK_SET);

        if (fr < 0)
        {
            LOGE("lseek err %d", fr);
            goto error;
        }

        fr = read(fd, id3v1, sizeof(id3v1));

        if (fr < 0 || fr != sizeof(id3v1))
        {
            LOGE("read %s id3v1 failed! fr %d", full_path, fr);
            goto error;
        }

#else
        fr = f_lseek(&s_a2dp_source_player_ctx.mp3_file, file_size - sizeof(id3v1));

        if (fr != FR_OK)
        {
            LOGE("f_lseek to ID3v1 end err %d", fr);
            goto error;
        }

        fr = f_read(&s_a2dp_source_player_ctx.mp3_file, id3v1, sizeof(id3v1), &num_rd);

        if (fr != FR_OK || num_rd != sizeof(id3v1))
        {
            LOGE("read ID3v1 err %d num %d", fr, num_rd);
            goto error;
        }

#endif

        if (os_memcmp(id3v1, "TAG", 3))
        {
            LOGD("ID3v1 not found!");
            break;
        }

        has_id3v1 = 1;

        LOGI("found ID3v1");

        if (!id3_maj_ver)
        {
            id3_maj_ver = 1;
        }
    }
    while (0);

#if CONFIG_VFS
    lseek(fd, frame_start_offset, SEEK_SET);
#else
    f_lseek(&s_a2dp_source_player_ctx.mp3_file, frame_start_offset);
#endif

    LOGI("mp3 file open successfully!");

    mp3_read_start_ptr = os_malloc(MAINBUF_SIZE * 3 / 2);

    if (!mp3_read_start_ptr)
    {
        LOGE("mp3_read_ptr alloc err");
        goto error;
    }

    pcm_write_ptr = os_malloc(MP3_DECODE_BUFF_SIZE);

    if (!pcm_write_ptr)
    {
        LOGE("pcm_write_ptr alloc err");
        goto error;
    }

    bytesleft = 0;
    current_mp3_read_ptr = mp3_read_start_ptr;

    mp3_read_end_ptr = mp3_read_start_ptr + MAINBUF_SIZE * 3 / 2;

    while (*task_ctrl)
    {
        uint32_t left_byte = 0;

        if (current_mp3_read_ptr - mp3_read_start_ptr > MAINBUF_SIZE)
        {
            LOGV("%p move to %p %d %d", current_mp3_read_ptr, mp3_read_start_ptr, current_mp3_read_ptr - mp3_read_start_ptr, bytesleft);

            os_memmove(mp3_read_start_ptr, current_mp3_read_ptr, bytesleft);
            current_mp3_read_ptr = mp3_read_start_ptr;
        }

        if (0 != mp3_read_end_ptr - current_mp3_read_ptr - bytesleft)
        {
#if CONFIG_VFS
            fr = read(fd, current_mp3_read_ptr + bytesleft, mp3_read_end_ptr - current_mp3_read_ptr - bytesleft);

            if (fr < 0)
            {
                LOGE("read %s failed ! fr %d", full_path, fr);
                goto error;
            }

            num_rd = fr;
#else
            fr = f_read(&s_a2dp_source_player_ctx.mp3_file, current_mp3_read_ptr + bytesleft, mp3_read_end_ptr - current_mp3_read_ptr - bytesleft, &num_rd);

            if (fr != FR_OK)
            {
                LOGE("read %d %s failed!", num_rd, full_path);
                goto error;
            }

#endif

            if (!num_rd)
            {
                LOGI("file end, return to begin");

                os_memmove(mp3_read_start_ptr, current_mp3_read_ptr, bytesleft);
                current_mp3_read_ptr = mp3_read_start_ptr;
#if CONFIG_VFS
                lseek(fd, frame_start_offset, SEEK_SET);
#else
                f_lseek(&s_a2dp_source_player_ctx.mp3_file, frame_start_offset);
#endif
                continue;
            }

#if CONFIG_VFS
            left_byte = ftell(fd);
#else
            left_byte = f_tell(&s_a2dp_source_player_ctx.mp3_file);
#endif

            if (has_id3v1 && file_size - left_byte <= 128)
            {
                num_rd -= left_byte - (file_size - 128);
            }

            bytesleft += num_rd;
        }

        LOGV("bytesleft %d %p", bytesleft, current_mp3_read_ptr);

        do
        {
            uint8_t *last_success_read_ptr = current_mp3_read_ptr;
            int last_success_bytesleft = bytesleft;

            if (!bytesleft)
            {
                break;
            }

            ret = MP3Decode(s_mp3_decoder, &current_mp3_read_ptr, &bytesleft, (int16_t *)pcm_write_ptr, 0);

            if (ret != ERR_MP3_NONE)
            {
                if (ERR_MP3_INDATA_UNDERFLOW == ret)
                {
                    bytesleft = last_success_bytesleft;
                    current_mp3_read_ptr = last_success_read_ptr;
                    break;
                }

                LOGE("MP3Decode failed %d bytesleft %d %d", ret, bytesleft, last_success_bytesleft);
                goto error;
            }

            //            os_memset(&s_a2dp_source_player_ctx.mp3_frame_info, 0, sizeof(s_a2dp_source_player_ctx.mp3_frame_info));
            MP3GetLastFrameInfo(s_mp3_decoder, &tmp_mp3_frame_info);

            LOGV("start write ring buff %d", tmp_mp3_frame_info.outputSamps * tmp_mp3_frame_info.bitsPerSample / 8);

            if (bk_a2dp_source_service_write_pcm(pcm_write_ptr, tmp_mp3_frame_info.outputSamps * tmp_mp3_frame_info.bitsPerSample / 8) < 0)
            {
                /* pipeline stopped */
                goto error;
            }

            pcm_decode_size += tmp_mp3_frame_info.outputSamps * tmp_mp3_frame_info.bitsPerSample / 8;
#if CONFIG_VFS
            left_byte = ftell(fd);
#else
            left_byte = f_tell(&s_a2dp_source_player_ctx.mp3_file);
#endif
            //            LOGV("bytesleft %d %p readsize %d", bytesleft, current_mp3_read_ptr, left_byte);
            //            LOGV("Bitrate: %d kb/s, Samprate: %d", (tmp_mp3_frame_info.bitrate) / 1000, tmp_mp3_frame_info.samprate);
            //            LOGV("Channel: %d, Version: %d, Layer: %d", tmp_mp3_frame_info.nChans, tmp_mp3_frame_info.version, tmp_mp3_frame_info.layer);
            //            LOGV("OutputSamps: %d %d", tmp_mp3_frame_info.outputSamps, tmp_mp3_frame_info.outputSamps * tmp_mp3_frame_info.bitsPerSample / 8);
        }
        while (tmp_mp3_frame_info.outputSamps && *task_ctrl);

#if CONFIG_VFS
        left_byte = ftell(fd);
#else
        left_byte = f_tell(&s_a2dp_source_player_ctx.mp3_file);
#endif

        if (0)//!num_rd || (id3v2_maj_ver == 1 && file_size - left_byte <= 128))
        {
            LOGI("decode end %d %d", bytesleft, pcm_decode_size);
            LOGI("samplerate %d channel %d ver %d", tmp_mp3_frame_info.samprate, tmp_mp3_frame_info.nChans, tmp_mp3_frame_info.version);
            break;
        }

        if (!*task_ctrl)
        {
            goto error;
        }
    }

error:;

    if (s_mp3_decoder)
    {
        MP3FreeDecoder(s_mp3_decoder);
        s_mp3_decoder = NULL;
    }

#if CONFIG_VFS

    if (fd >= 0)
    {
        close(fd);
        fd = -1;
    }

    fr = umount(VFS_SD_0_PATITION_0);

    if (fr < 0)
    {
        LOGE("umount err %d", fr);
    }

#else

    if (s_a2dp_source_player_ctx.mp3_file.fs)
    {
        f_close(&s_a2dp_source_player_ctx.mp3_file);
        os_memset(&s_a2dp_source_player_ctx.mp3_file, 0, sizeof(s_a2dp_source_player_ctx.mp3_file));
    }

    if (s_pfs)
    {
        fr = f_unmount(DISK_NUMBER_SDIO_SD, "1:", 1);

        if (fr)
        {
            LOGE("f_unmount err %d", fr);
        }

        os_free(s_pfs);
        s_pfs = NULL;
    }

#endif

    if (pcm_write_ptr)
    {
        os_free(pcm_write_ptr);
        pcm_write_ptr = NULL;
    }

    if (mp3_read_start_ptr)
    {
        os_free(mp3_read_start_ptr);
        mp3_read_start_ptr = NULL;
    }

    LOGI("exit");
    //s_a2dp_source_player_ctx.decode_thread_handle = NULL;
    rtos_delete_thread(NULL);

    return;
}

static bk_err_t a2dp_source_demo_stop_mp3_decode_task(void)
{
    int ret = 0;

    LOGI("step 1");

    if (s_a2dp_source_player_ctx.decode_thread_handle)
    {
        s_a2dp_source_player_ctx.decode_task_run = 0;
        bk_a2dp_source_service_media_stop();
        rtos_thread_join(&s_a2dp_source_player_ctx.decode_thread_handle);
        s_a2dp_source_player_ctx.decode_thread_handle = NULL;
    }
    else
    {
        bk_a2dp_source_service_media_stop();
    }

    return 0;
}

static bk_err_t a2dp_source_demo_create_mp3_decode_task(void)
{
    bk_err_t err = 0;

    if (s_a2dp_source_player_ctx.decode_thread_handle)
    {
        LOGE("already create");
        return 0;
    }

    s_a2dp_source_player_ctx.decode_trigger_size = 0;
    bk_audio_osi_funcs_init();
    get_mp3_info(&s_a2dp_source_player_ctx.mp3_frame_info);

    if (!s_a2dp_source_player_ctx.mp3_frame_info.samprate)
    {
        LOGE("get_mp3_info err !!");
        err = -1;
        goto error;
    }
    else
    {
        LOGI("get_mp3_info success !!");
    }

    err = bk_a2dp_source_service_set_pcm_format(s_a2dp_source_player_ctx.mp3_frame_info.samprate, s_a2dp_source_player_ctx.mp3_frame_info.nChans, s_a2dp_source_player_ctx.mp3_frame_info.bitsPerSample);

    if (err)
    {
        LOGE("set pcm format err %d", err);
        goto error;
    }

    err = bk_a2dp_source_service_media_start();

    if (err)
    {
        LOGE("media start err %d", err);
        goto error;
    }

    if (!s_a2dp_source_player_ctx.decode_thread_handle)
    {
        err = rtos_create_thread(&s_a2dp_source_player_ctx.decode_thread_handle,
                                 TASK_PRIORITY - 2,
                                 "a2dp_source_decode",
                                 (beken_thread_function_t)bt_a2dp_source_decode_task,
                                 1024 * 3,
                                 (beken_thread_arg_t)&s_a2dp_source_player_ctx.decode_task_run);

        if (err)
        {
            LOGE("task fail");

            s_a2dp_source_player_ctx.decode_thread_handle = NULL;
            goto error;
        }
        else
        {
            /* let the decode task pre-fill the component ring buffer a bit */
            rtos_delay_milliseconds(200);
        }
    }

    return 0;

error:;

    a2dp_source_demo_stop_mp3_decode_task();

    return err;
}


bk_err_t bt_a2dp_source_demo_stop_all(void)
{
    LOGD("step 1");
    a2dp_source_demo_stop_mp3_decode_task();
    LOGD("step 2");
    bt_a2dp_source_demo_disconnect(s_a2dp_source_player_ctx.peer_addr.addr);

    return 0;
}

static void a2dp_source_demo_main_task(void *arg)
{
    bk_err_t error = BK_OK;
    uint8_t *addr = (typeof(addr))arg;

    BK_ASSERT(4 == INT_CEIL(3, 4));

    os_memcpy(s_a2dp_source_player_ctx.peer_addr.addr, addr, sizeof(s_a2dp_source_player_ctx.peer_addr.addr));

    error = bt_a2dp_source_demo_connect(s_a2dp_source_player_ctx.peer_addr.addr);

    if (error)
    {
        LOGE("connect err!!!");
        goto error;
    }

    error = bt_a2dp_source_demo_music_play(1, s_a2dp_source_player_ctx.file_path);

    if (error)
    {
        goto error;
    }

#if USER_A2DP_MAIN_TASK
    rtos_delete_thread(NULL);
#endif
    return;
error:;

    bt_a2dp_source_demo_stop_all();

#if USER_A2DP_MAIN_TASK
    rtos_delete_thread(NULL);
#endif
    return;
}

bk_err_t bt_a2dp_source_demo_test(uint8_t *addr, uint8_t is_mp3, uint8_t *file_path)
{
#if USER_A2DP_MAIN_TASK
    bk_err_t err = 0;

    if (!s_a2dp_source_player_ctx.main_thread_handle)
    {
        os_strcpy((char *)s_a2dp_source_player_ctx.file_path, (char *)file_path);
        s_is_mp3 = is_mp3;

        err = rtos_create_thread(&s_a2dp_source_player_ctx.main_thread_handle,
                                 BEKEN_DEFAULT_WORKER_PRIORITY - 1,
                                 "a2dp_source_main",
                                 (beken_thread_function_t)a2dp_source_demo_main_task,
                                 1024 * 4,
                                 (beken_thread_arg_t)addr);

        if (err)
        {
            LOGE("task fail");

            s_a2dp_source_player_ctx.main_thread_handle = NULL;
            goto error;
        }
    }

    rtos_thread_join(s_a2dp_source_player_ctx.main_thread_handle);
    s_a2dp_source_player_ctx.main_thread_handle = NULL;

    return 0;
error:;
    return err;
#else
    os_strcpy((char *)s_a2dp_source_player_ctx.file_path, (char *)file_path);
    a2dp_source_demo_main_task((void *)addr);
    return 0;
#endif
}

bk_err_t bt_a2dp_source_demo_test_performance(uint32_t cpu_fre, uint32_t bytes, uint32_t loop, uint32_t cpu_id)
{
    bk_err_t ret = 0;
    uint8_t init = 0;
    const uint32_t src_bytes = bytes;//256;//1024 * 4;
    const uint32_t dest_bytes = 4 + (typeof(dest_bytes))(src_bytes * 48000.0 / 44100);
    const uint32_t loop_count = loop;
    uint8_t *input_buff = NULL, *output_buff = NULL;

    if (s_a2dp_source_player_ctx.is_bk_aud_rsp_inited)
    {
        LOGE("rsp is run");
        ret = -1;
        goto error;
    }

#if PCM_CALL_METHOD == PCM_CALL_METHOD_SMP
    ret = bk_a2dp_source_pcm_service_init();

    if (ret)
    {
        LOGE("calcu init req err %d !!", ret);
        ret = -1;
        goto error;
    }

#endif

#if PCM_CALL_METHOD == PCM_CALL_METHOD_SMP

    bt_audio_resample_init_req_t req = {0};

    req.rsp_cfg.src_rate = 44100;
    req.rsp_cfg.src_ch = 2;
    req.rsp_cfg.src_bits = 16;
    req.rsp_cfg.dest_rate = 48000;
    req.rsp_cfg.dest_ch = 2;
    req.rsp_cfg.dest_bits = 16;
    req.rsp_cfg.complexity = 0;
    req.rsp_cfg.down_ch_idx = 0;

    ret = bk_a2dp_source_pcm_service_rsp_init_req(&req, 1);

    if (ret)
    {
        LOGE("rsp init req err %d !!", ret);
        ret = -1;
        goto error;
    }

#else

    const aud_rsp_cfg_t cfg =
    {
        .src_rate = 44100,
        .src_ch = 2,//s_input_pcm_info.nChans,
        .src_bits = 16,
        .dest_rate = 48000,
        .dest_ch = 2,
        .dest_bits = 16,
        .complexity = 0,
        .down_ch_idx = 0,
    };

    ret = bk_aud_rsp_init(cfg);

    if (ret)
    {
        LOGE("bk_aud_rsp_init err %d !!", ret);
        ret = -1;
        goto error;
    }

#endif

    init = 1;

    input_buff = os_malloc(src_bytes);
    output_buff = os_malloc(dest_bytes);

    if (!input_buff || !output_buff)
    {
        LOGE("malloc err");
        ret = -1;
        goto error;
    }

    //    os_memset(input_buff, 0, src_bytes);
    //    os_memset(output_buff, 0, dest_bytes);

    if (cpu_fre)
    {
        //bk_pm_module_vote_cpu_freq(PM_DEV_ID_BTDM, cpu_fre);//PM_CPU_FRQ_240M
    }

    beken_time_t before = rtos_get_time();

    for (int i = 0; i < loop_count; ++i)
    {
        uint32_t input_len = 0;
        uint32_t output_len = 0;

#if PCM_CALL_METHOD == PCM_CALL_METHOD_SMP

        bt_audio_resample_req_t req = {0};

        input_len = src_bytes;
        output_len = dest_bytes;

        req.in_addr = input_buff;
        req.out_addr = output_buff;
        req.in_bytes_ptr = &input_len;
        req.out_bytes_ptr = &output_len;

        ret = bk_a2dp_source_pcm_service_rsp_req(&req);

        if (ret)
        {
            LOGE("rsp req err %d !!", ret);
            ret = -1;
            goto error;
        }

#else

        input_len = src_bytes / 2;
        output_len = dest_bytes / 2;

        ret = bk_aud_rsp_process((int16_t *)input_buff, &input_len, (int16_t *)output_buff, &output_len);

        if (ret)
        {
            LOGE("bk_aud_rsp_process err %d !!", ret);
            goto error;
        }

#endif
    }

    beken_time_t after = rtos_get_time();

    LOGI("input bytes %dKB time cost %d", src_bytes * loop_count / 1024, after - before);

error:;

    if (init)
    {
#if PCM_CALL_METHOD == PCM_CALL_METHOD_SMP
        ret = bk_a2dp_source_pcm_service_rsp_init_req(NULL, 0);

        if (ret)
        {
            LOGE("rsp deinit req err %d !!", ret);
        }

#else
        bk_aud_rsp_deinit();
#endif
    }

    if (input_buff)
    {
        os_free(input_buff);
    }

    if (output_buff)
    {
        os_free(output_buff);
    }

    if (cpu_fre)
    {
        //bk_pm_module_vote_cpu_freq(PM_DEV_ID_BTDM, PM_CPU_FRQ_DEFAULT);
    }

#if PCM_CALL_METHOD == PCM_CALL_METHOD_SMP
    ret = bk_a2dp_source_pcm_service_deinit();

    if (ret)
    {
        LOGE("calcu deinit err %d !!", ret);
    }

#endif

    return ret;
}
