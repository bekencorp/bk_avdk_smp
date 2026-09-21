// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0

#include <common/bk_include.h>
#include <common/bk_err.h>
#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>
#include <components/video_types.h>

#include "cs2_congestion_drop.h"
#include "network_type.h"
#include "ntwk_pack.h"
#include "ntwk_fragmentation.h"
#include "PPCS_cs2_comm.h"

#define TAG "cs2_cong"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

/* 强制 IDR 限频参数：全局丢帧与每会话准入共用。 */
#ifndef CONFIG_NTWK_CS2_FORCE_IDR_MIN_GAP_MS
#define CONFIG_NTWK_CS2_FORCE_IDR_MIN_GAP_MS (500)
#endif
#ifndef CONFIG_NTWK_CS2_FORCE_IDR_MAX_STREAK
#define CONFIG_NTWK_CS2_FORCE_IDR_MAX_STREAK (3)
#endif
#ifndef CONFIG_NTWK_CS2_HEALTHY_FRAMES
#define CONFIG_NTWK_CS2_HEALTHY_FRAMES (25)
#endif

/* ==================== 强制 IDR 仲裁器（编码器级） ==================== */

static bool     s_force_idr_pending;
static uint32_t s_force_idr_streak;
static uint32_t s_last_force_idr_tick;
static uint32_t s_healthy_count;

static void ntwk_cs2_note_force_idr_health(bool healthy)
{
    if (healthy)
    {
        if (s_healthy_count < 0xFFFFFFFFU)
        {
            s_healthy_count++;
        }
        if (s_healthy_count >= CONFIG_NTWK_CS2_HEALTHY_FRAMES)
        {
            s_force_idr_streak = 0;
        }
    }
    else
    {
        s_healthy_count = 0;
    }
}

void ntwk_cs2_request_force_idr(void)
{
    uint32_t now = rtos_get_time();

    if (s_force_idr_streak >= CONFIG_NTWK_CS2_FORCE_IDR_MAX_STREAK)
    {
        return;
    }

    if ((uint32_t)(now - s_last_force_idr_tick) < CONFIG_NTWK_CS2_FORCE_IDR_MIN_GAP_MS)
    {
        return;
    }

    s_force_idr_pending = true;
    s_last_force_idr_tick = now;
    s_force_idr_streak++;
    LOGI("request force idr streak:%u\n", s_force_idr_streak);
}

bool ntwk_cs2_consume_force_idr(void)
{
    bool force_idr = s_force_idr_pending;

    s_force_idr_pending = false;
    return force_idr;
}

static void ntwk_cs2_force_idr_reset(void)
{
    s_force_idr_pending = false;
    s_force_idr_streak = 0;
    s_last_force_idr_tick = 0;
    s_healthy_count = 0;
}

/* ==================== 每会话准入（fanout 发送口） ==================== */
#if CONFIG_NTWK_CS2_CONGESTION_DROP

#define POLICY_MAX_SESSIONS         CONFIG_CS2_MAX_NUM_CONNECTIONS
#define NTWK_CS2_VIDEO_WRITE_THD    CS2_IMG_MAX_TX_BUFFER_THD
#define NTWK_CS2_VIDEO_RESUME_THD   (CS2_IMG_MAX_TX_BUFFER_THD / 2)
#define H264_NAL_TYPE_IDR           (5)

static uint8_t  s_waiting_key[POLICY_MAX_SESSIONS];   /* 1: 跳过了非关键帧，等关键帧干净恢复 */
static uint8_t  s_skip_frame[POLICY_MAX_SESSIONS];    /* 1: 跳过当前帧的剩余分片 */
static uint8_t  s_sending_frame[POLICY_MAX_SESSIONS]; /* 1: 当前帧已准入，继续发剩余分片 */
static uint32_t s_skip_count[POLICY_MAX_SESSIONS];    /* 该会话累计跳帧数（调试统计） */

static bool cong_index_valid(int index)
{
    return (index >= 0 && index < POLICY_MAX_SESSIONS);
}

static bool ntwk_cs2_h264_payload_is_key(const uint8_t *data, uint32_t length)
{
    uint32_t i = 0;

    if (data == NULL || length < 4)
    {
        return false;
    }

    while (i + 3 < length)
    {
        uint32_t sc = 0;

        if (data[i] == 0x00 && data[i + 1] == 0x00 && data[i + 2] == 0x01)
        {
            sc = 3;
        }
        else if ((i + 4 < length)
                 && data[i] == 0x00 && data[i + 1] == 0x00
                 && data[i + 2] == 0x00 && data[i + 3] == 0x01)
        {
            sc = 4;
        }

        if (sc != 0 && (i + sc) < length)
        {
            uint8_t nal_type = data[i + sc] & 0x1F;
            if (nal_type == H264_NAL_TYPE_IDR)
            {
                return true;
            }
            i += sc + 1;
            continue;
        }

        i++;
    }

    return false;
}

void ntwk_cs2_cong_parse_video_packet(uint8_t *data, uint32_t length,
                                      image_format_t video_type,
                                      ntwk_cs2_video_pkt_meta_t *meta)
{
    if (meta == NULL)
    {
        return;
    }

    os_memset(meta, 0, sizeof(*meta));
    meta->valid = true;
    meta->first_frag = true;
    meta->last_frag = true;
    meta->is_key = (video_type != IMAGE_H264);

    if (data == NULL || length < sizeof(ntwk_pack_head_t))
    {
        if (video_type == IMAGE_H264)
        {
            meta->is_key = ntwk_cs2_h264_payload_is_key(data, length);
        }
        return;
    }

    ntwk_pack_head_t *pack = (ntwk_pack_head_t *)data;
    if (pack->magic != CHECK_ENDIAN_UINT16(HEAD_MAGIC_CODE))
    {
        if (video_type == IMAGE_H264)
        {
            meta->is_key = ntwk_cs2_h264_payload_is_key(data, length);
        }
        return;
    }

    uint16_t payload_len = CHECK_ENDIAN_UINT16(pack->length);
    if (payload_len < sizeof(ntwk_fragm_head_t)
        || length < (sizeof(ntwk_pack_head_t) + sizeof(ntwk_fragm_head_t)))
    {
        if (video_type == IMAGE_H264)
        {
            meta->is_key = ntwk_cs2_h264_payload_is_key(pack->payload, payload_len);
        }
        return;
    }

    ntwk_fragm_head_t *frag = (ntwk_fragm_head_t *)pack->payload;
    uint32_t nal_len = payload_len - sizeof(ntwk_fragm_head_t);

    meta->frame_id = frag->id;
    meta->cnt = frag->cnt;
    meta->first_frag = (frag->cnt <= 1);
    meta->last_frag = (frag->eof != 0);
    if (video_type == IMAGE_H264 && meta->first_frag)
    {
        meta->is_key = ntwk_cs2_h264_payload_is_key(frag->data, nal_len);
    }
}

bool ntwk_cs2_cong_video_admit(int index, uint32_t backlog,
                               const ntwk_cs2_video_pkt_meta_t *meta)
{
    bool admit = false;

    if (!cong_index_valid(index) || meta == NULL)
    {
        return false;
    }

    if (meta->first_frag)
    {
        s_skip_frame[index] = 0;
        s_sending_frame[index] = 0;

        if (s_waiting_key[index])
        {
            /* 该会话此前跳过了帧：只有 buffer 排空到低水位、且来的是关键帧，
             * 才干净恢复；期间请求一次强制 IDR，尽快拿到关键帧。 */
            if (backlog <= NTWK_CS2_VIDEO_RESUME_THD)
            {
                ntwk_cs2_request_force_idr();
                if (meta->is_key)
                {
                    s_waiting_key[index] = 0;
                    admit = true;
                }
            }
        }
        else if (backlog < NTWK_CS2_VIDEO_WRITE_THD)
        {
            /* buffer 还吃得下：发。 */
            admit = true;
        }
        else
        {
            /* buffer 吃不下：跳过整帧并置 waiting_key。 */
            s_waiting_key[index] = 1;
            s_skip_count[index]++;
            LOGI("idx:%d skip frame id:%u cnt:%u backlog:%u waiting_key\n",
                 index, meta->frame_id, meta->cnt, backlog);
        }

        s_sending_frame[index] = admit ? 1 : 0;
        s_skip_frame[index] = admit ? 0 : 1;
        return admit;
    }

    /* 后续分片：沿用首片决策，禁止中途改口导致半帧。 */
    if (s_skip_frame[index] || !s_sending_frame[index])
    {
        return false;
    }

    return true;
}

void ntwk_cs2_cong_mark_truncated(int index)
{
    if (cong_index_valid(index))
    {
        s_waiting_key[index] = 1;
        s_skip_frame[index] = 1;
        s_sending_frame[index] = 0;
        s_skip_count[index]++;
    }

    ntwk_cs2_request_force_idr();
}

void ntwk_cs2_cong_note_frame_end(void)
{
    bool any_waiting = false;

    for (int i = 0; i < POLICY_MAX_SESSIONS; i++)
    {
        if (s_waiting_key[i])
        {
            any_waiting = true;
        }
        s_skip_frame[i] = 0;
        s_sending_frame[i] = 0;
    }

    ntwk_cs2_note_force_idr_health(!any_waiting);
}

bool ntwk_cs2_cong_audio_admit(int index, uint32_t backlog)
{
    if (!cong_index_valid(index))
    {
        return false;
    }

    if (backlog > CS2_AUD_MAX_TX_BUFFER_THD)
    {
        LOGV("idx:%d audio drop, backlog:%u\n", index, backlog);
        return false;
    }

    return true;
}

void ntwk_cs2_cong_reset_session(int index)
{
    if (cong_index_valid(index))
    {
        s_waiting_key[index] = 0;
        s_skip_frame[index] = 0;
        s_sending_frame[index] = 0;
        s_skip_count[index] = 0;
    }
}

void ntwk_cs2_cong_on_subscribe(int index, bool enable)
{
    if (!cong_index_valid(index))
    {
        return;
    }

    s_skip_frame[index] = 0;
    s_sending_frame[index] = 0;
    /* 订阅：从 waiting_key 起步，第一帧强制走关键帧恢复路径，避免中途 P 帧花屏。 */
    s_waiting_key[index] = enable ? 1 : 0;
}

#else /* !CONFIG_NTWK_CS2_CONGESTION_DROP */

void ntwk_cs2_cong_parse_video_packet(uint8_t *data, uint32_t length,
                                      image_format_t video_type,
                                      ntwk_cs2_video_pkt_meta_t *meta)
{
    (void)data;
    (void)length;

    if (meta == NULL)
    {
        return;
    }

    os_memset(meta, 0, sizeof(*meta));
    meta->valid = true;
    meta->first_frag = true;
    meta->last_frag = true;
    meta->is_key = (video_type != IMAGE_H264);
}

bool ntwk_cs2_cong_video_admit(int index, uint32_t backlog,
                               const ntwk_cs2_video_pkt_meta_t *meta)
{
    (void)index;
    (void)backlog;
    (void)meta;
    return true;
}

void ntwk_cs2_cong_mark_truncated(int index)
{
    (void)index;
}

bool ntwk_cs2_cong_audio_admit(int index, uint32_t backlog)
{
    (void)index;
    (void)backlog;
    return true;
}

void ntwk_cs2_cong_note_frame_end(void) {}
void ntwk_cs2_cong_reset_session(int index) { (void)index; }
void ntwk_cs2_cong_on_subscribe(int index, bool enable) { (void)index; (void)enable; }

#endif /* CONFIG_NTWK_CS2_CONGESTION_DROP */

bool ntwk_cs2_cong_is_enabled(void)
{
#if CONFIG_NTWK_CS2_CONGESTION_DROP
    return true;
#else
    return false;
#endif
}

void ntwk_cs2_cong_reset_all(void)
{
    ntwk_cs2_force_idr_reset();
#if CONFIG_NTWK_CS2_CONGESTION_DROP
    for (int i = 0; i < POLICY_MAX_SESSIONS; i++)
    {
        ntwk_cs2_cong_reset_session(i);
    }
#endif
}

/* ==================== 全局预投递丢帧（编码输出口） ==================== */
#if CONFIG_NTWK_CS2_CONGESTION_DROP

#include <common/avdk_pixel_types.h>

/* 只声明所需的 CS2 接口，避免把重量级的 ntwk_cs2_service.h（lwip/PPCS）拉进本文件。 */
extern bk_err_t ntwk_cs2_get_video_backlog_range(uint32_t *min_size, uint32_t *max_size);

#ifndef CONFIG_NTWK_CS2_DROP_START_THD
#define CONFIG_NTWK_CS2_DROP_START_THD       (131072)   /* 高水位：积压 >= 它开始丢帧 (128KB) */
#endif
#ifndef CONFIG_NTWK_CS2_DROP_STOP_THD
#define CONFIG_NTWK_CS2_DROP_STOP_THD        (65536)    /* 低水位：积压 <= 它停止丢帧并强制 IDR (64KB) */
#endif

typedef struct
{
    bool     drop_enable;         /* 当前是否处于丢帧状态 */
    uint32_t start_thd;           /* 高水位 */
    uint32_t stop_thd;            /* 低水位 */
} ntwk_cs2_congestion_drop_t;

static ntwk_cs2_congestion_drop_t s_policy;

void ntwk_cs2_congestion_drop_init(void)
{
    os_memset(&s_policy, 0, sizeof(s_policy));
    s_policy.start_thd      = CONFIG_NTWK_CS2_DROP_START_THD;
    s_policy.stop_thd       = CONFIG_NTWK_CS2_DROP_STOP_THD;

    LOGI("init start:%u stop:%u healthy:%u gap:%ums streak:%u\n",
         s_policy.start_thd, s_policy.stop_thd,
         CONFIG_NTWK_CS2_HEALTHY_FRAMES,
         CONFIG_NTWK_CS2_FORCE_IDR_MIN_GAP_MS,
         CONFIG_NTWK_CS2_FORCE_IDR_MAX_STREAK);
}

void ntwk_cs2_congestion_drop_reset(void)
{
    s_policy.drop_enable = false;
}

bool ntwk_cs2_congestion_drop_check(const void *frame)
{
    const frame_buffer_t *fb = (const frame_buffer_t *)frame;
    uint32_t min_backlog = 0;
    uint32_t max_backlog = 0;

    /* 只处理 H264 帧 */
    if (fb == NULL || fb->fmt != PIXEL_FMT_H264)
    {
        return false;
    }

    /* 无订阅会话 / 未连接：无网络积压，视为不拥塞，复位运行态 */
    if (ntwk_cs2_get_video_backlog_range(&min_backlog, &max_backlog) != BK_OK)
    {
        s_policy.drop_enable = false;
        ntwk_cs2_force_idr_reset();
        return false;
    }

    /* 未在丢帧：仅当所有订阅会话都冲到高水位才全局丢帧，慢会话由 fanout waiting_key 隔离 */
    if (!s_policy.drop_enable)
    {
        if (min_backlog >= s_policy.start_thd)
        {
            s_policy.drop_enable = true;
            LOGI("congest start, min:%u max:%u seq:%u\n", min_backlog, max_backlog, fb->sequence);
        }
        else
        {
            return false;   /* 至少一路还能发：放行，由每会话 waiting_key 决定 */
        }
    }

    /* 处于全局丢帧状态：所有会话回落到低水位则退出并请求（限频）强制 IDR */
    if (min_backlog <= s_policy.stop_thd)
    {
        s_policy.drop_enable = false;
        ntwk_cs2_request_force_idr();
        LOGI("congest recover, request force idr min:%u max:%u\n",
             min_backlog, max_backlog);
    }

    return true;   /* 丢弃当前帧（全局拥塞中，或本次为退出丢帧的旧参考帧） */
}

bool ntwk_cs2_congestion_consume_force_idr(void)
{
    return ntwk_cs2_consume_force_idr();
}

#else /* !CONFIG_NTWK_CS2_CONGESTION_DROP */

void ntwk_cs2_congestion_drop_init(void) {}
void ntwk_cs2_congestion_drop_reset(void) {}
bool ntwk_cs2_congestion_drop_check(const void *frame) { (void)frame; return false; }

bool ntwk_cs2_congestion_consume_force_idr(void)
{
    return ntwk_cs2_consume_force_idr();
}

#endif /* CONFIG_NTWK_CS2_CONGESTION_DROP */
