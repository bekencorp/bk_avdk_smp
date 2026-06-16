#include <common/bk_err.h>
#include <common/avdk_pixel_types.h>
#include <components/log.h>

#include "network_transfer.h"
#include "h264e_stream_priv.h"

#define TAG "h264e_stream_push"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

int h264e_stream_session_send_h264(uint8_t *data, uint32_t length)
{
    h264e_stream_session_ctx_t *ctx = h264e_stream_session_get_ctx();
    if (data == NULL || length == 0)
    {
        return -1;
    }

    if (!ctx->initialized || !ctx->started)
    {
        LOGE("demo is not started\n");
        return -1;
    }
    
    return ntwk_trans_video_send(data, length, IMAGE_H264);
}