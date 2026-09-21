#include <common/bk_include.h>
#include <stdbool.h>

#include "lwip/tcp.h"
#include "bk_uart.h"
#include <os/mem.h>
#include <os/os.h>
#include <os/str.h>
#include <common/bk_kernel_err.h>
#include <components/video_types.h>

#include "lwip/sockets.h"
#include <stdlib.h>

#include "network_type.h"
#include "network_transfer.h"
#include "ntwk_cs2_service.h"
#include "network_transfer_internal.h"

#include "PPCS_API.h"
#include "PPCS_Error.h"
#include "PPCS_Type.h"

#include "PPCS_cs2_comm.h"

#include "cs2_congestion_drop.h"

#define TAG "ntwk-cs2"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

ntwk_cs2_info_t *ntwk_cs2_info = NULL;

static void ntwk_cs2_session_close(int session_id);

/*
 * CS2 拥塞/丢帧/强制 IDR 策略已整体迁移到 common/bk_video_drop_policy/
 * cs2_congestion_drop.{c,h}。本文件只保留 CS2 连接与收发（传输）能力，
 * 发送口通过 ntwk_cs2_cong_* 接口向 drop 策略询问每会话准入。
 */

static int ntwk_cs2_check_channel_backlog(int session_id, uint8_t channel, uint32_t *write_size)
{
    UINT32 WriteSize = 0;
    int32_t check_ret = PPCS_Check_Buffer(session_id, channel, &WriteSize, NULL);

    if (check_ret < 0)
    {
        LOGE("%s PPCS_Check_Buffer: Session=%d,CH=%d,WriteSize=%d,ret=%d %s\n", __func__,
             session_id, channel, WriteSize, check_ret, get_p2p_error_code_info(check_ret));
        return check_ret;
    }

    if (write_size != NULL)
    {
        *write_size = WriteSize;
    }

    return 0;
}

static st_PPCS_NetInfo s_cs2_p2p_networkinfo;

static cs2_session_t *ntwk_cs2_find_session_locked(int session_id)
{
    if (ntwk_cs2_info == NULL)
    {
        return NULL;
    }

    for (int i = 0; i < NTWK_CS2_MAX_NUM_CONNECTIONS; i++)
    {
        cs2_session_t *session = &ntwk_cs2_info->sessions[i];
        if (session->used && session->session_id == session_id)
        {
            return session;
        }
    }

    return NULL;
}

static cs2_session_t *ntwk_cs2_find_session(int session_id)
{
    cs2_session_t *session = NULL;

    if (ntwk_cs2_info == NULL)
    {
        return NULL;
    }

    rtos_lock_mutex(&ntwk_cs2_info->mutex);
    session = ntwk_cs2_find_session_locked(session_id);
    rtos_unlock_mutex(&ntwk_cs2_info->mutex);

    return session;
}

static bool ntwk_cs2_session_video_enabled_locked(const cs2_session_t *session)
{
    if (session == NULL
        || !session->used
        || session->state != CS2_SESSION_CONNECTED)
    {
        return false;
    }

    return ntwk_cs2_cong_is_enabled() ? session->video_subscribed : true;
}

static bool ntwk_cs2_session_audio_enabled_locked(const cs2_session_t *session)
{
    if (session == NULL
        || !session->used
        || session->state != CS2_SESSION_CONNECTED)
    {
        return false;
    }

    return ntwk_cs2_cong_is_enabled() ? session->audio_subscribed : true;
}

static int ntwk_cs2_get_first_session_id(void)
{
    int session_id = CS2_INVALID_SESSION_ID;

    if (ntwk_cs2_info == NULL)
    {
        return CS2_INVALID_SESSION_ID;
    }

    rtos_lock_mutex(&ntwk_cs2_info->mutex);
    for (int i = 0; i < NTWK_CS2_MAX_NUM_CONNECTIONS; i++)
    {
        cs2_session_t *session = &ntwk_cs2_info->sessions[i];
        if (session->used && session->state == CS2_SESSION_CONNECTED)
        {
            session_id = session->session_id;
            break;
        }
    }
    rtos_unlock_mutex(&ntwk_cs2_info->mutex);

    return session_id;
}

static int ntwk_cs2_get_active_session_count_locked(void)
{
    int count = 0;

    if (ntwk_cs2_info == NULL)
    {
        return 0;
    }

    for (int i = 0; i < NTWK_CS2_MAX_NUM_CONNECTIONS; i++)
    {
        if (ntwk_cs2_info->sessions[i].used
            && ntwk_cs2_info->sessions[i].state == CS2_SESSION_CONNECTED)
        {
            count++;
        }
    }

    return count;
}

static cs2_session_t *ntwk_cs2_alloc_session_locked(int session_id)
{
    if (ntwk_cs2_info == NULL)
    {
        return NULL;
    }

    if (ntwk_cs2_find_session_locked(session_id) != NULL)
    {
        return NULL;
    }

    for (int i = 0; i < NTWK_CS2_MAX_NUM_CONNECTIONS; i++)
    {
        cs2_session_t *session = &ntwk_cs2_info->sessions[i];
        if (!session->used)
        {
            os_memset(session, 0, sizeof(cs2_session_t));
            session->session_id = session_id;
            session->index = i;
            session->used = BK_TRUE;
            session->state = CS2_SESSION_CONNECTED;
            session->video_subscribed = BK_FALSE;
            session->audio_subscribed = BK_FALSE;
            session->connected_tick = rtos_get_time();
            rtos_init_mutex(&session->tx_lock);
            session->tx_lock_inited = BK_TRUE;
            ntwk_cs2_cong_reset_session(i);
            return session;
        }
    }

    return NULL;
}

static void ntwk_cs2_release_session_locked(cs2_session_t *session)
{
    if (session == NULL || !session->used)
    {
        return;
    }

    if (session->tx_lock_inited)
    {
        rtos_deinit_mutex(&session->tx_lock);
    }

    ntwk_cs2_cong_reset_session(session->index);

    os_memset(session, 0, sizeof(cs2_session_t));
    session->session_id = CS2_INVALID_SESSION_ID;
    session->state = CS2_SESSION_UNUSED;
}

#if THROUGHPUT_DEBUG
static beken_timer_t s_throughput_timer;
static volatile uint32_t s_send_video_count = 0;
static volatile uint32_t s_send_video_bytes = 0;
static volatile uint32_t s_send_audio_bytes = 0;
static volatile uint32_t s_recv_audio_bytes = 0;
static volatile uint32_t s_recv_total_bytes = 0;
static volatile uint32_t s_send_time = 0;
static volatile uint32_t s_delay_count = 0;

static bool ntwk_cs2_throughput_timer_running(void)
{
    bool running = false;

    if (ntwk_cs2_info == NULL)
    {
        return false;
    }

    rtos_lock_mutex(&ntwk_cs2_info->mutex);
    running = rtos_is_timer_init(&s_throughput_timer)
              && rtos_is_timer_running(&s_throughput_timer);
    rtos_unlock_mutex(&ntwk_cs2_info->mutex);

    return running;
}

static void throughput_anlayse_timer_hdl(void *param)
{
    LOGD("cs2_tp, send bytes %d, count %d, %.3f count/s, %.3f KB/s, %.3f ms/perc, %.3fB/perc, delay count %d\n",
         s_send_video_bytes,
         s_send_video_count,
         1.0 * s_send_video_count / (THROUGHPUT_ANLAYSE_MS / 1000),
         (float)(s_send_video_bytes / (THROUGHPUT_ANLAYSE_MS / 1000) / 1024.0),
         1.0 * s_send_time / s_send_video_count,
         1.0 * s_send_video_bytes / s_send_video_count,
         s_delay_count);

    s_delay_count = s_send_time = s_send_video_count = s_recv_total_bytes = s_send_video_bytes = s_send_audio_bytes = s_recv_audio_bytes = 0;

    return;

}

void ntwk_cs2_video_timer_deinit(void)
{
    bk_err_t t_err;

    if (!ntwk_cs2_info)
    {
        return;
    }

    rtos_lock_mutex(&ntwk_cs2_info->mutex);

    if (rtos_is_timer_init(&s_throughput_timer))
    {
        if (rtos_is_timer_running(&s_throughput_timer))
        {
            t_err = rtos_stop_timer(&s_throughput_timer);

            if (t_err != BK_OK)
            {
                LOGE("stop throughput timer fail\n");
                rtos_unlock_mutex(&ntwk_cs2_info->mutex);
                return ;
            }
        }

        t_err = rtos_deinit_timer(&s_throughput_timer);
        if (t_err != BK_OK)
        {
            LOGE("deinit throughput timer fail\n");
            rtos_unlock_mutex(&ntwk_cs2_info->mutex);
            return ;
        }
    }

    rtos_unlock_mutex(&ntwk_cs2_info->mutex);
}

bk_err_t ntwk_cs2_video_timer_init(void)
{
    bk_err_t t_err;

    rtos_lock_mutex(&ntwk_cs2_info->mutex);
    if (rtos_is_timer_init(&s_throughput_timer))
    {
        rtos_unlock_mutex(&ntwk_cs2_info->mutex);
        return BK_OK;
    }

    t_err = rtos_init_timer(&s_throughput_timer, THROUGHPUT_ANLAYSE_MS, throughput_anlayse_timer_hdl, NULL);

    if (t_err != BK_OK)
    {
        LOGE("init throughput timer fail\n");
        goto timer_err;
    }

    t_err = rtos_change_period(&s_throughput_timer, THROUGHPUT_ANLAYSE_MS);
    if (t_err != BK_OK)
    {
        LOGE("change throughput timer period fail\n");
        goto timer_err;
    }

    t_err = rtos_start_timer(&s_throughput_timer);
    if (t_err != BK_OK)
    {
        LOGE("start throughput timer fail\n");
        goto timer_err;
    }

    rtos_unlock_mutex(&ntwk_cs2_info->mutex);

    return BK_OK;

timer_err:

    rtos_unlock_mutex(&ntwk_cs2_info->mutex);
    ntwk_cs2_video_timer_deinit();

    return BK_FAIL;
}
#endif

static bk_err_t ntwk_cs2_collect_video_backlog(uint32_t *min_size, uint32_t *max_size, int *checked_count)
{
    UINT32 write_size = 0;
    UINT32 min_write_size = 0xFFFFFFFFU;
    UINT32 max_write_size = 0;
    int count = 0;

    if (ntwk_cs2_info == NULL)
    {
        return -1;
    }

    rtos_lock_mutex(&ntwk_cs2_info->mutex);
    for (int i = 0; i < NTWK_CS2_MAX_NUM_CONNECTIONS; i++)
    {
        cs2_session_t *session = &ntwk_cs2_info->sessions[i];
        int32_t check_ret;

        if (!ntwk_cs2_session_video_enabled_locked(session))
        {
            continue;
        }

        check_ret = PPCS_Check_Buffer(session->session_id, VIDEO_P2P_CHANNEL, &write_size, NULL);
        if (check_ret < 0)
        {
            LOGE("%s PPCS_Check_Buffer: Session=%d,CH=%d,WriteSize=%d,ret=%d %s\n", __func__,
                 session->session_id, VIDEO_P2P_CHANNEL, write_size, check_ret, get_p2p_error_code_info(check_ret));
            rtos_unlock_mutex(&ntwk_cs2_info->mutex);
            return -1;
        }

        if (write_size > max_write_size)
        {
            max_write_size = write_size;
        }
        if (write_size < min_write_size)
        {
            min_write_size = write_size;
        }
        count++;
    }
    rtos_unlock_mutex(&ntwk_cs2_info->mutex);

    if (count == 0)
    {
        return -1;
    }

    if (min_size != NULL)
    {
        *min_size = min_write_size;
    }
    if (max_size != NULL)
    {
        *max_size = max_write_size;
    }
    if (checked_count != NULL)
    {
        *checked_count = count;
    }

    return BK_OK;
}

bk_err_t ntwk_cs2_get_current_write_size(uint32_t *write_size)
{
    uint32_t max_write_size = 0;

    if (write_size == NULL)
    {
        LOGE("%s invalid parameter\n", __func__);
        return -1;
    }

    if (ntwk_cs2_collect_video_backlog(NULL, &max_write_size, NULL) != BK_OK)
    {
        return -1;
    }

    *write_size = max_write_size;
    return BK_OK;
}

bk_err_t ntwk_cs2_get_video_backlog_range(uint32_t *min_size, uint32_t *max_size)
{
    if (min_size == NULL || max_size == NULL)
    {
        return -1;
    }

    return ntwk_cs2_collect_video_backlog(min_size, max_size, NULL);
}

static int ntwk_cs2_p2p_write_internal(int SessionID, uint8_t Channel, uint8_t *buff,
                                       uint32_t size, bool ignore_watermark)
{
    int32_t ret = 0;
    int32_t Check_ret = 0;
    UINT32 WriteSize = size;
    uint32_t write_index = 0;
    uint32_t write_not_send_thr = PPCS_TX_BUFFER_THD;
    const uint32_t write_per_count_thr = size;

    if (size == 0)
    {
        LOGE("%s, size: 0\n", __func__);
        return 0;
    }

    do
    {
        uint32_t will_write_size = ((size - write_index < write_per_count_thr) ? (size - write_index) : write_per_count_thr);
        Check_ret = PPCS_Check_Buffer(SessionID, Channel, &WriteSize, NULL);

        if (0 > Check_ret)
        {
            LOGE("%s PPCS_Check_Buffer: Session=%d,CH=%d,WriteSize=%d,ret=%d %s\n", __func__, SessionID, Channel, WriteSize, Check_ret, get_p2p_error_code_info(Check_ret));
            ret = Check_ret;
            goto WRITE_FAIL;
        }

        if (Channel == VIDEO_P2P_CHANNEL)
        {
            write_not_send_thr = CS2_IMG_MAX_TX_BUFFER_THD;
        }
        else if (Channel == AUD_P2P_CHANNEL)
        {
            write_not_send_thr = CS2_AUD_MAX_TX_BUFFER_THD;
        }

        if (ignore_watermark || WriteSize <= write_not_send_thr)
        {
            ret = PPCS_Write(SessionID, Channel, (CHAR *)(buff + write_index), will_write_size);

            if (0 > ret)
            {
                if (ERROR_PPCS_SESSION_CLOSED_TIMEOUT == ret)
                {
                    LOGE("%s Session=%d,CH=%d,ret=%d, Session Closed TimeOUT!!\n", __func__, SessionID, Channel, ret);
                }
                else if (ERROR_PPCS_SESSION_CLOSED_REMOTE == ret)
                {
                    LOGE("%s Session=%d,CH=%d,ret=%d, Session Remote Closed!!\n", __func__, SessionID, Channel, ret);
                }
                else if (ERROR_PPCS_INVALID_PARAMETER == ret)
                {
                    LOGE("%s Session=%d,CH=%d,ret=%d, ERROR_PPCS_INVALID_PARAMETER %d!!\n", __func__, SessionID, Channel, ret, will_write_size);
                }
                else
                {
                    LOGE("%s Session=%d,CH=%d,ret=%d %s\n", __func__, SessionID, Channel, ret, get_p2p_error_code_info(ret));
                }

                goto WRITE_FAIL;
            }

            write_index += ret;
        }
        else
        {
            break;
        }
    }
    while (write_index < size);

    return write_index;

WRITE_FAIL:

    return ret;
}

int ntwk_cs2_p2p_write(int SessionID, uint8_t Channel, uint8_t *buff, uint32_t size)
{
    return ntwk_cs2_p2p_write_internal(SessionID, Channel, buff, size, false);
}

static int ntwk_cs2_p2p_write_all(int session_id, uint8_t channel, uint8_t *data, uint32_t size)
{
    uint32_t index = 0;
    int send_byte = 0;

    while (index < size)
    {
        send_byte = ntwk_cs2_p2p_write_internal(session_id, channel, data + index, size - index, false);
        if (send_byte < 0)
        {
            return send_byte;
        }

        index += send_byte;
        if (index < size)
        {
            rtos_delay_milliseconds(CS2_P2P_TRANSFER_DELAY);
        }
    }

    return (int)index;
}

int ntwk_cs2_video_send_packet(uint8_t *data, uint32_t length, image_format_t video_type)
{
    int send_byte = 0;
    uint32_t size = length;
    int session_ids[NTWK_CS2_MAX_NUM_CONNECTIONS] = {0};
    int session_idx[NTWK_CS2_MAX_NUM_CONNECTIONS] = {0};
    int session_count = 0;
    bool drop_enabled = ntwk_cs2_cong_is_enabled();
    ntwk_cs2_video_pkt_meta_t meta;
#if THROUGHPUT_DEBUG
    uint32_t start_time = 0;
#endif

    if (ntwk_cs2_info == NULL || !ntwk_cs2_info->video_status)
    {
        LOGE("video not ready\n");
        return -1;
    }

    /* 由 drop 策略解析包元信息（首/尾分片、是否关键帧） */
    ntwk_cs2_cong_parse_video_packet(data, length, video_type, &meta);

    rtos_lock_mutex(&ntwk_cs2_info->mutex);
    for (int i = 0; i < NTWK_CS2_MAX_NUM_CONNECTIONS; i++)
    {
        cs2_session_t *session = &ntwk_cs2_info->sessions[i];
        if (ntwk_cs2_session_video_enabled_locked(session))
        {
            session_ids[session_count] = session->session_id;
            session_idx[session_count] = session->index;
            session_count++;
        }
    }
    rtos_unlock_mutex(&ntwk_cs2_info->mutex);

    if (session_count == 0)
    {
        LOGW("video send skip, no subscriber\n");
        return -1;
    }

#if THROUGHPUT_DEBUG
    if (ntwk_cs2_throughput_timer_running()) {
        start_time = rtos_get_time();
    }
#endif

    for (int i = 0; i < session_count; i++)
    {
        uint32_t backlog = 0;

        if (drop_enabled)
        {
            int check_ret = ntwk_cs2_check_channel_backlog(session_ids[i], VIDEO_P2P_CHANNEL, &backlog);

            if (check_ret < 0)
            {
                ntwk_cs2_session_close(session_ids[i]);
                continue;
            }

            /* 逐会话按该会话 CS2 buffer 决定发不发（由 drop 策略裁决） */
            if (!ntwk_cs2_cong_video_admit(session_idx[i], backlog, &meta))
            {
                continue;
            }
        }

        if (drop_enabled)
        {
            /* 已准入：本帧分片写完，不再按水位中途停写（避免半帧），也不 delay 补发 */
            send_byte = ntwk_cs2_p2p_write_internal(session_ids[i], VIDEO_P2P_CHANNEL, data, size, true);
        }
        else
        {
            /* 旧工程未启用 CS2 drop 策略时，保留原始反压：buffer 满则等待写完。 */
            send_byte = ntwk_cs2_p2p_write_all(session_ids[i], VIDEO_P2P_CHANNEL, data, size);
        }

        LOGV("%s send video sid:%d, send:%d, size:%d key:%d first:%d\n",
             __func__, session_ids[i], send_byte, size, meta.is_key, meta.first_frag);

        if (send_byte < 0)
        {
            LOGE("%s send return fd:%d sessionid:%d\n", __func__, send_byte, session_ids[i]);
            ntwk_cs2_cong_mark_truncated(session_idx[i]);
            ntwk_cs2_session_close(session_ids[i]);
            continue;
        }

        if ((uint32_t)send_byte < size)
        {
#if THROUGHPUT_DEBUG
           if (ntwk_cs2_throughput_timer_running()) {
                s_delay_count++;
           }
#endif
            LOGD("%s sid:%d truncated send:%d size:%d, wait key\n",
                 __func__, session_ids[i], send_byte, size);
            ntwk_cs2_cong_mark_truncated(session_idx[i]);
        }
    }

    if (meta.last_frag)
    {
        ntwk_cs2_cong_note_frame_end();
    }

#if THROUGHPUT_DEBUG
    uint32_t end_time = rtos_get_time();

    if (ntwk_cs2_throughput_timer_running()) {
        s_send_video_count++;
        s_send_video_bytes += size;
        s_send_time += end_time - start_time;
    }
#endif

    return (int)size;
}

int ntwk_cs2_audio_send_packet(uint8_t *data, uint32_t length, audio_enc_type_t audio_type)
{
    int send_byte = 0;
    uint32_t size = length;
    int session_ids[NTWK_CS2_MAX_NUM_CONNECTIONS] = {0};
    int session_idx[NTWK_CS2_MAX_NUM_CONNECTIONS] = {0};
    int session_count = 0;
    bool drop_enabled = ntwk_cs2_cong_is_enabled();

    (void)audio_type;

    if (ntwk_cs2_info == NULL || !ntwk_cs2_info->aud_status)
    {
        LOGE("audio not ready\n");
        return -1;
    }

    rtos_lock_mutex(&ntwk_cs2_info->mutex);
    for (int i = 0; i < NTWK_CS2_MAX_NUM_CONNECTIONS; i++)
    {
        cs2_session_t *session = &ntwk_cs2_info->sessions[i];
        if (ntwk_cs2_session_audio_enabled_locked(session))
        {
            session_ids[session_count] = session->session_id;
            session_idx[session_count] = session->index;
            session_count++;
        }
    }
    rtos_unlock_mutex(&ntwk_cs2_info->mutex);

    if (session_count == 0)
    {
        LOGW("audio send skip, no subscriber\n");
        return -1;
    }

    for (int i = 0; i < session_count; i++)
    {
        uint32_t backlog = 0;

        if (drop_enabled)
        {
            int check_ret = ntwk_cs2_check_channel_backlog(session_ids[i], AUD_P2P_CHANNEL, &backlog);

            if (check_ret < 0)
            {
                ntwk_cs2_session_close(session_ids[i]);
                continue;
            }

            if (!ntwk_cs2_cong_audio_admit(session_idx[i], backlog))
            {
                continue;
            }
        }

        if (drop_enabled)
        {
            send_byte = ntwk_cs2_p2p_write_internal(session_ids[i], AUD_P2P_CHANNEL, data, size, true);
        }
        else
        {
            send_byte = ntwk_cs2_p2p_write_all(session_ids[i], AUD_P2P_CHANNEL, data, size);
        }

        if (send_byte < 0)
        {
            LOGE("%s send return fd:%d sessionid:%d\n", __func__, send_byte, session_ids[i]);
            ntwk_cs2_session_close(session_ids[i]);
            continue;
        }

        if ((uint32_t)send_byte < size)
        {
            LOGW("%s partial send sid:%d, send:%d, size:%d, drop\n",
                 __func__, session_ids[i], send_byte, size);
        }
    }

    return (int)size;
}

int ntwk_cs2_p2p_ctrl_send_to(int session_id, uint8_t *data, uint32_t length)
{
    int send_byte = 0;
    uint32_t index = 0;

    if (ntwk_cs2_info == NULL || session_id < 0)
    {
        LOGE("%s invalid session:%d\n", __func__, session_id);
        return -1;
    }

    do
    {
        send_byte = ntwk_cs2_p2p_write(session_id, CMD_P2P_CHANNEL, data + index, length - index);

        if (send_byte < 0)
        {
            LOGE("%s send return fd:%d sessionid:%d\n", __func__, send_byte, session_id);
            rtos_delay_milliseconds(CS2_P2P_TRANSFER_DELAY);
            return -1;
        }

        index += send_byte;

        if (index < length)
        {
            LOGD("%s delay %d, %d\n", __func__, index, length);
            rtos_delay_milliseconds(CS2_P2P_TRANSFER_DELAY);
        }

    }
    while (index < length);


    return index;
}

int ntwk_cs2_p2p_ctrl_send(uint8_t *data, uint32_t length)
{
    return ntwk_cs2_p2p_ctrl_send_to(ntwk_cs2_get_first_session_id(), data, length);
}


static void ntwk_cs2_session_close(int session_id)
{
    time_info_t t1, t2;
    cs2_session_t *session = NULL;
    bool should_close = false;
    bool was_connected = false;
    int active_count = 0;

    memset(&t2, 0, sizeof(t2));

    if (ntwk_cs2_info == NULL || session_id < 0)
    {
        return;
    }

    rtos_lock_mutex(&ntwk_cs2_info->mutex);

    session = ntwk_cs2_find_session_locked(session_id);
    if (session == NULL)
    {
        LOGW("%s session %d already close\n", __func__, session_id);
        goto out;
    }

    if (session->state == CS2_SESSION_CLOSING)
    {
        LOGW("%s session %d is closing\n", __func__, session_id);
        goto out;
    }

    session->state = CS2_SESSION_CLOSING;
    was_connected = session->used;
    should_close = true;

out:
    rtos_unlock_mutex(&ntwk_cs2_info->mutex);

    if (!should_close)
    {
        return;
    }

    cs2_p2p_get_time(&t1);

    PPCS_ForceClose(session_id);// PPCS_Close(SessionID);// 不能多线程对同一个 SessionID 做 PPCS_Close(SessionID)/PPCS_ForceClose(SessionID) 的动作，否则可能导致崩溃。

    cs2_p2p_get_time(&t2);

    LOGD("%s: (%d) done!! t:%d ms\n", __func__, session_id, TU_MS(t1, t2));

    rtos_lock_mutex(&ntwk_cs2_info->mutex);
    session = ntwk_cs2_find_session_locked(session_id);
    if (session != NULL)
    {
        ntwk_cs2_release_session_locked(session);
    }

    active_count = ntwk_cs2_get_active_session_count_locked();
    ntwk_cs2_info->device_connected = (active_count > 0) ? BK_TRUE : BK_FALSE;
    ntwk_cs2_info->video_status = (active_count > 0) ? BK_TRUE : BK_FALSE;
    ntwk_cs2_info->aud_status = (active_count > 0) ? BK_TRUE : BK_FALSE;
    rtos_unlock_mutex(&ntwk_cs2_info->mutex);

    if (was_connected)
    {
        ntwk_msg_event_report(NTWK_TRANS_EVT_DISCONNECTED, session_id, NTWK_TRANS_CHAN_CTRL);
    }

    if (active_count == 0)
    {
        ntwk_msg_event_report(NTWK_TRANS_EVT_STOP, 0, NTWK_TRANS_CHAN_CTRL);

        #if THROUGHPUT_DEBUG
        ntwk_cs2_video_timer_deinit();
        #endif
    }
}

int ntwk_cs2_p2p_interface_init(p2p_cs2_key_t *key)
{
    int ret = 0;
    UINT32 APIVersion = PPCS_GetAPIVersion();
    char VerBuf[64] = {0};

    snprintf(VerBuf, sizeof(VerBuf), "%d.%d.%d.%d",
             (APIVersion & 0xFF000000) >> 24,
             (APIVersion & 0x00FF0000) >> 16,
             (APIVersion & 0x0000FF00) >> 8,
             (APIVersion & 0x000000FF) >> 0);

    if (0 > strncmp(VerBuf, "3.5.0.0", 5))
    {
        LOGD("PPCS P2P API Version: %d.%d.%d.%d\n",
             (APIVersion & 0xFF000000) >> 24,
             (APIVersion & 0x00FF0000) >> 16,
             (APIVersion & 0x0000FF00) >> 8,
             (APIVersion & 0x000000FF) >> 0);
    }
    else
    {
        const char *pVer = PPCS_GetAPIInformation();// PPCS_GetAPIInformation: support by Version >= 3.5.0
        LOGD("PPCS_GetAPIInformation(%u Byte):\n%s\n", (unsigned)strlen(pVer), pVer);
    }

    time_info_t t1, t2;
    os_memset(&t1, 0, sizeof(t1));
    os_memset(&t2, 0, sizeof(t2));

    if (0 <= strncmp(VerBuf, "4.2.0.0", 5)) // PPCS_Initialize JsonString support by Version>=4.2.0
    {
        int MaxNumSess = NTWK_CS2_MAX_NUM_CONNECTIONS; // Max Number Session: 1~256.
        int SessAliveSec = 15; // session timeout close alive: 6~30.


        char InitJsonString[256] = {0};
        snprintf(InitJsonString, sizeof(InitJsonString), "{\"InitString\":\"%s\",\"MaxNumSess\":%d,\"SessAliveSec\":%d}", key->initstring, MaxNumSess, SessAliveSec);
        // st_debug("InitJsonString=%s\n",InitJsonString);
        cs2_p2p_get_time(&t1);
        LOGD("[%s] PPCS_Initialize1(%s) ...\n", t1.date, InitJsonString);


        // 如果Parameter 不是正确的JSON字串则会被当成InitString[:P2PKey]来处理, 如此以兼容旧版.
        ret = PPCS_Initialize((char *)InitJsonString);

        cs2_p2p_get_time(&t2);
        LOGD("[%s] PPCS_Initialize2 len(%d): ret=%d, t:%d ms\n", t2.date, strlen(key->initstring), ret, TU_MS(t1, t2));


        if (ERROR_PPCS_SUCCESSFUL != ret && ERROR_PPCS_ALREADY_INITIALIZED != ret)
        {
            LOGD("[%s] PPCS_Initialize: ret=%d\n", t2.date, ret);
            return 0;
        }
    }
    else
    {
        cs2_p2p_get_time(&t1);
        LOGD("[%s] PPCS_Initialize3(%s) ...\n", t1.date, key->initstring);
        ret = PPCS_Initialize((char *)key->initstring);
        cs2_p2p_get_time(&t2);
        LOGD("[%s] PPCS_Initialize4(%s): ret=%d, t:%d ms\n", t2.date, key->initstring, ret, TU_MS(t1, t2));

        if (ERROR_PPCS_SUCCESSFUL != ret && ERROR_PPCS_ALREADY_INITIALIZED != ret)
        {
            LOGD("[%s] PPCS_Initialize: ret=%d\n", t2.date, ret);
            return 0;
        }
    }

    return 0;
}

void ntwk_cs2_p2p_interface_deinit(void)
{
    int ret = PPCS_DeInitialize();

    if (ERROR_PPCS_SUCCESSFUL != ret)
    {
        LOGE("%s PPCS_DeInitialize: ret=%d %s\n", __func__, ret, get_p2p_error_code_info(ret));
    }

    os_memset(&s_cs2_p2p_networkinfo, 0, sizeof(s_cs2_p2p_networkinfo));
}

static int ntwk_cs2_p2p_audio_receiver(beken_thread_arg_t arg)
{
    int32_t ret = 0;
    cs2_session_t *session = (cs2_session_t *)arg;
    int session_id = (session != NULL) ? session->session_id : CS2_INVALID_SESSION_ID;
    uint8_t *tmp_read_buf = NULL;

    tmp_read_buf = ntwk_malloc(NTWK_CS2_RECV_TMP_BUFF_SIZE);

    if (!tmp_read_buf)
    {
        LOGE("p2p", "%s alloc err\n", __func__);
        return -1;
    }

    if (ntwk_cs2_info && ntwk_cs2_info->aud_sem_inited)
    {
        rtos_set_semaphore(&ntwk_cs2_info->aud_sem);
    }

    while (ntwk_cs2_info && ntwk_cs2_info->is_running && ntwk_cs2_info->aud_running == BK_TRUE)
    {
        INT32 ReadSize = NTWK_CS2_RECV_TMP_BUFF_SIZE;

        ret = PPCS_Read(session_id, AUD_P2P_CHANNEL, (char *)tmp_read_buf, &ReadSize, 1000 * 2);

        if (ReadSize)
        {
            bool is_owner = false;

            rtos_lock_mutex(&ntwk_cs2_info->mutex);
            cs2_session_t *cur_session = ntwk_cs2_find_session_locked(session_id);
            if (cur_session != NULL && cur_session->speaker_owner)
            {
                cur_session->last_rx_tick = rtos_get_time();
                is_owner = true;
            }
            rtos_unlock_mutex(&ntwk_cs2_info->mutex);

            if (is_owner && ntwk_cs2_info->audio_receive_cb)
            {
                LOGV("got audio sid:%d count:%d\n", session_id, ReadSize);
                ntwk_cs2_info->audio_receive_cb(tmp_read_buf, ReadSize);
            }
            else
            {
                LOGV("drop non-owner audio sid:%d count:%d\n", session_id, ReadSize);
            }

            continue;
        }

        if (ret == ERROR_PPCS_TIME_OUT)
        {
            LOGV("got audio data timeout\n");
            continue;
        }

        if (ret < 0 && ERROR_PPCS_TIME_OUT != ret)
        {
            LOGE("%s PPCS_Read sid:%d ret err %d %s\n", __func__, session_id, ret, get_p2p_error_code_info(ret));
            ntwk_cs2_session_close(session_id);
            break;
        }
    }

    if (tmp_read_buf)
    {
        os_free(tmp_read_buf);
    }

    if (ntwk_cs2_info)
    {
        rtos_lock_mutex(&ntwk_cs2_info->mutex);
        cs2_session_t *cur_session = ntwk_cs2_find_session_locked(session_id);
        if (cur_session != NULL)
        {
            cur_session->audio_thread_running = BK_FALSE;
            cur_session->audio_rx_thread = NULL;
        }
        rtos_unlock_mutex(&ntwk_cs2_info->mutex);
    }

    LOGE("audio thread exit sid:%d\n", session_id);

    rtos_delete_thread(NULL);

    return 0;
}

static int ntwk_cs2_p2p_ctrl_receiver(beken_thread_arg_t arg)
{
    int32_t ret = 0;
    cs2_session_t *session = (cs2_session_t *)arg;
    int session_id = (session != NULL) ? session->session_id : CS2_INVALID_SESSION_ID;
    uint8_t *tmp_read_buf = NULL;

    tmp_read_buf = ntwk_malloc(RECV_TMP_CMD_BUFF_SIZE);

    if (!tmp_read_buf)
    {
        LOGE("p2p", "%s alloc err\n", __func__);
        ntwk_cs2_session_close(session_id);
        return -1;
    }

    while (ntwk_cs2_info && ntwk_cs2_info->is_running)
    {
        INT32 ReadSize = RECV_TMP_CMD_BUFF_SIZE;

        ret = PPCS_Read(session_id, CMD_P2P_CHANNEL, (char *)tmp_read_buf, &ReadSize, 1000 * 2);

        if (ReadSize)
        {
            LOGD("got cmd sid:%d count:%d\n", session_id, ReadSize);
            rtos_lock_mutex(&ntwk_cs2_info->mutex);
            cs2_session_t *cur_session = ntwk_cs2_find_session_locked(session_id);
            if (cur_session != NULL)
            {
                cur_session->last_rx_tick = rtos_get_time();
            }
            rtos_unlock_mutex(&ntwk_cs2_info->mutex);

            if (ntwk_cs2_info->ctrl_receive_cb)
            {
                ntwk_cs2_info->ctrl_receive_cb(session_id, tmp_read_buf, ReadSize);
            }
            continue;
        }

        if (ret == ERROR_PPCS_TIME_OUT)
        {
            LOGV("got cmd data timeout, %d\n", ReadSize);
            continue;
        }

        if (ret < 0 && ERROR_PPCS_TIME_OUT != ret)
        {
            LOGD("%s PPCS_Read sid:%d ret err %d %s\n", __func__, session_id, ret, get_p2p_error_code_info(ret));
            break;
        }
    }

    if (tmp_read_buf)
    {
        os_free(tmp_read_buf);
    }

    if (ntwk_cs2_info)
    {
        rtos_lock_mutex(&ntwk_cs2_info->mutex);
        cs2_session_t *cur_session = ntwk_cs2_find_session_locked(session_id);
        if (cur_session != NULL)
        {
            cur_session->ctrl_thread_running = BK_FALSE;
            cur_session->ctrl_rx_thread = NULL;
        }
        rtos_unlock_mutex(&ntwk_cs2_info->mutex);
    }

    ntwk_cs2_session_close(session_id);
    LOGE("ctrl thread exit sid:%d\n", session_id);
    rtos_delete_thread(NULL);

    return 0;
}

static bk_err_t ntwk_cs2_start_session_threads(cs2_session_t *session)
{
    bk_err_t ret = BK_OK;

    if (session == NULL)
    {
        return BK_ERR_PARAM;
    }

    ret = rtos_create_thread(&session->ctrl_rx_thread,
                             4,
                             "cs2_ctrl",
                             (beken_thread_function_t)ntwk_cs2_p2p_ctrl_receiver,
                             1024 * 8,
                             (beken_thread_arg_t)session);
    if (ret != BK_OK)
    {
        LOGE("%s create ctrl thread failed sid:%d ret:%d\n", __func__, session->session_id, ret);
        return ret;
    }
    session->ctrl_thread_running = BK_TRUE;

    if (ntwk_cs2_info->aud_running == BK_TRUE)
    {
        ret = rtos_create_thread(&session->audio_rx_thread,
                                 4,
                                 "cs2_audio",
                                 (beken_thread_function_t)ntwk_cs2_p2p_audio_receiver,
                                 1024 * 4,
                                 (beken_thread_arg_t)session);
        if (ret != BK_OK)
        {
            LOGE("%s create audio thread failed sid:%d ret:%d\n", __func__, session->session_id, ret);
        }
        else
        {
            session->audio_thread_running = BK_TRUE;
        }
    }

    return BK_OK;
}


static int ntwk_cs2_p2p_interface_core(p2p_cs2_key_t *key)
{
    int Repeat = 0;
    int32_t ret = 0;
    int session = -99;
    const unsigned long Total_Times = Repeat;

    (void)Total_Times;
    Repeat = 0;

    ret = PPCS_NetworkDetect(&s_cs2_p2p_networkinfo, 0);
    show_network(s_cs2_p2p_networkinfo);

    // Send CS2 service start event
    ntwk_msg_event_report(NTWK_TRANS_EVT_START, 0, NTWK_TRANS_CHAN_CTRL);

    while (ntwk_cs2_info->is_running)//Repeat < Total_Times)
    {
        int active_count = 0;

        rtos_lock_mutex(&ntwk_cs2_info->mutex);
        active_count = ntwk_cs2_get_active_session_count_locked();
        rtos_unlock_mutex(&ntwk_cs2_info->mutex);

        if (active_count >= NTWK_CS2_MAX_NUM_CONNECTIONS)
        {
            rtos_delay_milliseconds(1000);
            continue;
        }

        Repeat++;

        ret = PPCS_NetworkDetect(&s_cs2_p2p_networkinfo, 0);

        if (ret < 0)
        {
            LOGE("p2p", "%s PPCS_NetworkDetect err %d %s\n", __func__, ret, get_p2p_error_code_info(ret));
            return -1;
        }

        session = cs2_p2p_listen(key->did, key->apilicense, Repeat, &ntwk_cs2_info->is_running);

        if (0 <= session)
        {
            cs2_session_t *new_session = NULL;
            BK_LOGD("p2p", "%s listen Sid %d\n", __func__, session);

            rtos_lock_mutex(&ntwk_cs2_info->mutex);
            new_session = ntwk_cs2_alloc_session_locked(session);
            if (new_session != NULL)
            {
                ntwk_cs2_info->device_connected = BK_TRUE;
                ntwk_cs2_info->video_status = BK_TRUE;
                ntwk_cs2_info->aud_status = BK_TRUE;
            }
            rtos_unlock_mutex(&ntwk_cs2_info->mutex);

            if (new_session == NULL)
            {
                LOGE("%s no free session slot for sid:%d\n", __func__, session);
                PPCS_ForceClose(session);
                rtos_delay_milliseconds(300);
                continue;
            }

            ntwk_cs2_video_timer_init();
            ntwk_msg_event_report(NTWK_TRANS_EVT_CONNECTED, session, NTWK_TRANS_CHAN_CTRL);

            ret = ntwk_cs2_start_session_threads(new_session);
            if (ret != BK_OK)
            {
                ntwk_cs2_session_close(session);
            }

            rtos_delay_milliseconds(300); // 两次 PPCS_Listen 之间需要保持间隔。

            continue;
        }
        else if (ERROR_PPCS_MAX_SESSION == session)
        {
            rtos_delay_milliseconds(1 * 1000);
        }
        else if (session == -1)
        {
            break;
        }
    }

    rtos_lock_mutex(&ntwk_cs2_info->mutex);
    for (int i = 0; i < NTWK_CS2_MAX_NUM_CONNECTIONS; i++)
    {
        if (ntwk_cs2_info->sessions[i].used)
        {
            session = ntwk_cs2_info->sessions[i].session_id;
            rtos_unlock_mutex(&ntwk_cs2_info->mutex);
            ntwk_cs2_session_close(session);
            rtos_lock_mutex(&ntwk_cs2_info->mutex);
        }
    }
    rtos_unlock_mutex(&ntwk_cs2_info->mutex);

    return 0;
}


static void ntwk_cs2_service_main(beken_thread_arg_t arg)
{
    ntwk_cs2_p2p_interface_init(ntwk_cs2_info->cs2_key);

    ntwk_cs2_info->is_running = 1;

    ntwk_cs2_p2p_interface_core(ntwk_cs2_info->cs2_key);

    ntwk_cs2_p2p_interface_deinit();
}

bk_err_t ntwk_cs2_ctrl_chan_start(void *param)
{
    bk_err_t ret;
    p2p_cs2_key_t *p2p_cs2_key = NULL;

    LOGD("%s\n", __func__);

    if (param == NULL)
    {
        LOGE("%s param is NULL\n", __func__);
        return BK_FAIL;
    }

    p2p_cs2_key = (p2p_cs2_key_t *)param;

    if (p2p_cs2_key == NULL
        || p2p_cs2_key->did == NULL
        || p2p_cs2_key->apilicense == NULL
        || p2p_cs2_key->initstring == NULL)
    {
        LOGE("%s p2p_cs2_key null\n", __func__);
        return BK_FAIL;
    }

    if (ntwk_cs2_info == NULL)
    {
        return BK_FAIL;
    }

    ntwk_cs2_info->cs2_key = p2p_cs2_key;

    rtos_init_mutex(&ntwk_cs2_info->tx_lock);

    rtos_init_mutex(&ntwk_cs2_info->mutex);

    ret = rtos_init_semaphore_ex(&ntwk_cs2_info->aud_sem, 1, 0);
    if (ret == BK_OK)
    {
        ntwk_cs2_info->aud_sem_inited = BK_TRUE;
    }

    ret = rtos_create_thread(&ntwk_cs2_info->thd,
                             4,
                             "cs2",
                             (beken_thread_function_t)ntwk_cs2_service_main,
                             1024 * 8,
                             (beken_thread_arg_t)NULL);
    if (ret != BK_OK)
    {
        LOGE("Error: Failed to create cs2 service: %d\n", ret);
        return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t ntwk_cs2_ctrl_chan_stop(void)
{
    LOGD("%s\n", __func__);

    if (ntwk_cs2_info == NULL)
    {
        LOGW("%s: ntwk_cs2_info is already NULL\n", __func__);
        return BK_FAIL;
    }

    // 停止服务运行标志
    ntwk_cs2_info->is_running = BK_FALSE;

    // 停止音频线程运行
    ntwk_cs2_info->aud_running = BK_FALSE;

    // 关闭所有会话
    rtos_lock_mutex(&ntwk_cs2_info->mutex);
    for (int i = 0; i < NTWK_CS2_MAX_NUM_CONNECTIONS; i++)
    {
        if (ntwk_cs2_info->sessions[i].used)
        {
            int session_id = ntwk_cs2_info->sessions[i].session_id;
            rtos_unlock_mutex(&ntwk_cs2_info->mutex);
            ntwk_cs2_session_close(session_id);
            rtos_lock_mutex(&ntwk_cs2_info->mutex);
        }
    }
    rtos_unlock_mutex(&ntwk_cs2_info->mutex);

    rtos_delay_milliseconds(300);

    // 释放音频信号量
    if (ntwk_cs2_info->aud_sem_inited)
    {
        rtos_deinit_semaphore(&ntwk_cs2_info->aud_sem);
        ntwk_cs2_info->aud_sem_inited = BK_FALSE;
    }

    // 释放互斥锁
    rtos_deinit_mutex(&ntwk_cs2_info->mutex);
    rtos_deinit_mutex(&ntwk_cs2_info->tx_lock);

    ntwk_cs2_deinit();
    LOGD("%s: CS2 service deinitialized successfully\n", __func__);
    return BK_OK;
}

bk_err_t ntwk_cs2_init(void)
{
    if (ntwk_cs2_info != NULL)
    {
        LOGW("%s ntwk_cs2_info already initialized\n", __func__);
        return BK_OK;
    }

    ntwk_cs2_info = ntwk_malloc(sizeof(ntwk_cs2_info_t));

    if (ntwk_cs2_info == NULL)
    {
        LOGE("%s ntwk_cs2_info malloc failed\n", __func__);
        return BK_FAIL;
    }

    os_memset(ntwk_cs2_info, 0, sizeof(ntwk_cs2_info_t));
    ntwk_cs2_cong_reset_all();

    return BK_OK;
}

bk_err_t ntwk_cs2_deinit(void)
{
    if (ntwk_cs2_info != NULL)
    {
        os_free(ntwk_cs2_info);
        ntwk_cs2_info = NULL;
    }
    ntwk_cs2_cong_reset_all();

    return BK_OK;
}

bk_err_t ntwk_cs2_audio_chan_start(void *param)
{
    bk_err_t ret;
    int session_ids[NTWK_CS2_MAX_NUM_CONNECTIONS] = {0};
    int session_count = 0;

    LOGD("%s: start\n", __func__);

    if (ntwk_cs2_info == NULL)
    {
        LOGE("%s, ntwk_cs2_info is NULL\n", __func__);
        return BK_FAIL;
    }

    if (ntwk_cs2_info->aud_running == BK_TRUE)
    {
        LOGW("%s already open\n", __func__);
        return BK_OK;
    }

    ntwk_cs2_info->aud_running = BK_TRUE;

    rtos_lock_mutex(&ntwk_cs2_info->mutex);
    for (int i = 0; i < NTWK_CS2_MAX_NUM_CONNECTIONS; i++)
    {
        cs2_session_t *session = &ntwk_cs2_info->sessions[i];
        if (session->used
            && session->state == CS2_SESSION_CONNECTED
            && !session->audio_thread_running)
        {
            session_ids[session_count++] = session->session_id;
        }
    }
    rtos_unlock_mutex(&ntwk_cs2_info->mutex);

    for (int i = 0; i < session_count; i++)
    {
        rtos_lock_mutex(&ntwk_cs2_info->mutex);
        cs2_session_t *session = ntwk_cs2_find_session_locked(session_ids[i]);
        if (session != NULL && !session->audio_thread_running)
        {
            ret = rtos_create_thread(&session->audio_rx_thread,
                                     4,
                                     "cs2_audio",
                                     (beken_thread_function_t)ntwk_cs2_p2p_audio_receiver,
                                     1024 * 4,
                                     (beken_thread_arg_t)session);
            if (ret == BK_OK)
            {
                session->audio_thread_running = BK_TRUE;
            }
        }
        rtos_unlock_mutex(&ntwk_cs2_info->mutex);
    }

    return BK_OK;
}

bk_err_t ntwk_cs2_video_chan_start(void *param)
{
    if (ntwk_cs2_info == NULL)
    {
        LOGE("%s, service null\n", __func__);
        return BK_FAIL;
    }

    LOGD("%s, video channel started\n", __func__);
    return BK_OK;
}

bk_err_t ntwk_cs2_video_chan_stop(void)
{
    return BK_OK;
}

bk_err_t ntwk_cs2_audio_chan_stop(void)
{
    if (ntwk_cs2_info)
    {
        ntwk_cs2_info->aud_running = BK_FALSE;
    }
    return BK_OK;
}

bk_err_t ntwk_cs2_ctrl_register_receive_cb(ntwk_cs2_ctrl_receive_cb_t cb)
{
    if (ntwk_cs2_info == NULL)
    {
        LOGE("%s: Control channel not initialized\n", __func__);
        return BK_FAIL;
    }

    if (cb == NULL)
    {
        LOGE("%s: Invalid callback\n", __func__);
        return BK_ERR_PARAM;
    }

    ntwk_cs2_info->ctrl_receive_cb = cb;
    LOGD("%s: Receive callback registered successfully\n", __func__);

    return BK_OK;
}

bk_err_t ntwk_cs2_set_video_subscribe(int session_id, bool enable)
{
    if (ntwk_cs2_info == NULL)
    {
        return BK_FAIL;
    }

    rtos_lock_mutex(&ntwk_cs2_info->mutex);
    cs2_session_t *session = ntwk_cs2_find_session_locked(session_id);
    if (session == NULL)
    {
        rtos_unlock_mutex(&ntwk_cs2_info->mutex);
        return BK_FAIL;
    }

    session->video_subscribed = enable ? BK_TRUE : BK_FALSE;
    ntwk_cs2_cong_on_subscribe(session->index, enable);
    rtos_unlock_mutex(&ntwk_cs2_info->mutex);

    return BK_OK;
}

bk_err_t ntwk_cs2_set_audio_subscribe(int session_id, bool enable)
{
    if (ntwk_cs2_info == NULL)
    {
        return BK_FAIL;
    }

    rtos_lock_mutex(&ntwk_cs2_info->mutex);
    cs2_session_t *session = ntwk_cs2_find_session_locked(session_id);
    if (session == NULL)
    {
        rtos_unlock_mutex(&ntwk_cs2_info->mutex);
        return BK_FAIL;
    }

    session->audio_subscribed = enable ? BK_TRUE : BK_FALSE;
    if (!enable)
    {
        session->speaker_owner = BK_FALSE;
    }
    rtos_unlock_mutex(&ntwk_cs2_info->mutex);

    return BK_OK;
}

bk_err_t ntwk_cs2_set_speaker_owner(int session_id, bool enable)
{
    if (ntwk_cs2_info == NULL)
    {
        return BK_FAIL;
    }

    rtos_lock_mutex(&ntwk_cs2_info->mutex);
    if (enable)
    {
        for (int i = 0; i < NTWK_CS2_MAX_NUM_CONNECTIONS; i++)
        {
            ntwk_cs2_info->sessions[i].speaker_owner = BK_FALSE;
        }
    }

    cs2_session_t *session = ntwk_cs2_find_session_locked(session_id);
    if (session == NULL)
    {
        rtos_unlock_mutex(&ntwk_cs2_info->mutex);
        return BK_FAIL;
    }

    session->speaker_owner = enable ? BK_TRUE : BK_FALSE;
    rtos_unlock_mutex(&ntwk_cs2_info->mutex);

    return BK_OK;
}

bk_err_t ntwk_cs2_video_register_receive_cb(ntwk_video_receive_cb_t cb)
{
    if (ntwk_cs2_info == NULL)
    {
        LOGE("%s: Video channel not initialized\n", __func__);
        return BK_FAIL;
    }

    if (cb == NULL)
    {
        LOGE("%s: Invalid callback\n", __func__);
        return BK_ERR_PARAM;
    }

    ntwk_cs2_info->video_receive_cb = cb;
    LOGD("%s: Video receive callback registered successfully\n", __func__);

    return BK_OK;
}

bk_err_t ntwk_cs2_audio_register_receive_cb(ntwk_audio_receive_cb_t cb)
{
    if (ntwk_cs2_info == NULL)
    {
        LOGE("%s: Audio channel not initialized\n", __func__);
        return BK_FAIL;
    }

    if (cb == NULL)
    {
        LOGE("%s: Invalid callback\n", __func__);
        return BK_ERR_PARAM;
    }

    ntwk_cs2_info->audio_receive_cb = cb;
    LOGD("%s: Audio receive callback registered successfully\n", __func__);

    return BK_OK;
}





