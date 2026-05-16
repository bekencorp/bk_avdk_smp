#include <os/mem.h>
#include <components/log.h>
#include <common/avdk_pixel_types.h>

#include "h264_backpressure_drop.h"

#define TAG "h264_drop"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

typedef struct
{
    uint8_t max_available_buffer_count;
    uint8_t start_available_buffer_count;
    uint8_t stop_available_buffer_count;
    bool policy_enable;
    bool drop_enable;
    bool force_idr_pending;
} ntwk_h264_backpressure_drop_policy_t;

static ntwk_h264_backpressure_drop_policy_t s_h264_drop_policy = {0};

static bool ntwk_h264_backpressure_drop_is_supported(uint8_t max_available_buffer_count)
{
    return (max_available_buffer_count > NTWK_H264_DROP_MIN_AVAILABLE_BUFFER_COUNT);
}

static uint8_t ntwk_h264_backpressure_drop_start_count(uint8_t max_available_buffer_count)
{
    return ntwk_h264_backpressure_drop_is_supported(max_available_buffer_count) ?
           NTWK_H264_DROP_START_AVAILABLE_BUFFER_COUNT : 0;
}

static uint8_t ntwk_h264_backpressure_drop_stop_count(uint8_t max_available_buffer_count)
{
    return ntwk_h264_backpressure_drop_is_supported(max_available_buffer_count) ?
           (max_available_buffer_count - NTWK_H264_DROP_STOP_AVAILABLE_BUFFER_MARGIN) : 0;
}

void ntwk_h264_backpressure_drop_init(uint8_t max_available_buffer_count)
{
    ntwk_h264_backpressure_drop_policy_t *policy = &s_h264_drop_policy;

    os_memset(policy, 0, sizeof(*policy));

    policy->max_available_buffer_count = max_available_buffer_count;
    policy->policy_enable = ntwk_h264_backpressure_drop_is_supported(max_available_buffer_count);
    policy->start_available_buffer_count = ntwk_h264_backpressure_drop_start_count(max_available_buffer_count);
    policy->stop_available_buffer_count = ntwk_h264_backpressure_drop_stop_count(max_available_buffer_count);

    if (policy->policy_enable)
    {
        LOGI("h264 drop policy enabled, max_available_buffer_count:%d, start_available_buffer_count:%d, stop_available_buffer_count:%d\n",
             max_available_buffer_count, policy->start_available_buffer_count, policy->stop_available_buffer_count);
    }
    else
    {
        LOGI("h264 drop policy disabled, max_available_buffer_count:%d\n", max_available_buffer_count);
    }
}

void ntwk_h264_backpressure_drop_reset(void)
{
    ntwk_h264_backpressure_drop_policy_t *policy = &s_h264_drop_policy;

    policy->drop_enable = false;
    policy->force_idr_pending = false;
    LOGI("%s, reset\n", __func__);
}

bool ntwk_h264_backpressure_drop_is_h264_frame(const frame_buffer_t *frame)
{
    return (frame != NULL && frame->fmt == PIXEL_FMT_H264);
}

bool ntwk_h264_backpressure_drop_check(uint8_t available_buffer_count, const frame_buffer_t *frame)
{
    ntwk_h264_backpressure_drop_policy_t *policy = &s_h264_drop_policy;

    if (!policy->policy_enable || !ntwk_h264_backpressure_drop_is_h264_frame(frame))
    {
        return false;
    }

    bool was_dropping = policy->drop_enable;

    if (!policy->drop_enable && available_buffer_count <= policy->start_available_buffer_count)
    {
        policy->drop_enable = true;
    }

    LOGV("h264 drop check, type:%d seq:%d len:%d available_buffer:%d dropping:%d\n",
         frame->h264_type, frame->sequence, frame->length, available_buffer_count, was_dropping);

    if (policy->drop_enable && !was_dropping)
    {
        LOGV("h264 drop start, available_buffer:%d type:%d seq:%d\n",
             available_buffer_count, frame->h264_type, frame->sequence);
    }

    return policy->drop_enable;
}

bool ntwk_h264_backpressure_drop_on_recycle(uint8_t available_buffer_count, const frame_buffer_t *frame)
{
    ntwk_h264_backpressure_drop_policy_t *policy = &s_h264_drop_policy;

    if (!policy->policy_enable || !ntwk_h264_backpressure_drop_is_h264_frame(frame))
    {
        return false;
    }

    if (policy->drop_enable && available_buffer_count >= policy->stop_available_buffer_count)
    {
        policy->drop_enable = false;
        policy->force_idr_pending = true;

        LOGV("h264 drop frame, type:%d seq:%d available_buffer:%d force next IDR\n",
             frame->h264_type, frame->sequence, available_buffer_count);

        return true;
    }

    LOGV("h264 drop frame, type:%d seq:%d available_buffer:%d\n",
         frame->h264_type, frame->sequence, available_buffer_count);

    return false;
}

bool ntwk_h264_backpressure_drop_consume_force_idr(void)
{
    bool force_idr = false;
    ntwk_h264_backpressure_drop_policy_t *policy = &s_h264_drop_policy;

    force_idr = policy->force_idr_pending;
    policy->force_idr_pending = false;

    return force_idr;
}
