#include "components/avdk_utils/avdk_types.h"
#include "components/avdk_utils/avdk_error.h"
#include "components/bk_decode/bk_jpeg_decode_ctlr.h"
#include "components/bk_video_player/bk_video_player_types.h"
#include "bk_video_player_jpeg_probe.h"


avdk_err_t bk_video_player_probe_jpeg_subsampling(const uint8_t *jpeg_buf,
                                                  uint32_t jpeg_len,
                                                  video_player_jpeg_subsampling_t *out_subsampling)
{
    if (jpeg_buf == NULL || out_subsampling == NULL || jpeg_len == 0)
    {
        return AVDK_ERR_INVAL;
    }

    bk_jpeg_decode_img_info_t img_info = {0};
    img_info.input_stream = (uint8_t *)jpeg_buf;
    img_info.input_stream_length = jpeg_len;

    avdk_err_t jpeg_ret = bk_jpeg_decode_get_img_info(&img_info);
    if (jpeg_ret != AVDK_ERR_OK)
    {
        return jpeg_ret;
    }

    if ((uint32_t)img_info.format >= BK_JPEG_DECODE_IMG_FMT_MAX)
    {
        return AVDK_ERR_INVAL;
    }

    *out_subsampling = (img_info.format == BK_JPEG_DECODE_IMG_FMT_ERR) ? VIDEO_PLAYER_JPEG_SUBSAMPLING_NONE
                                              : (video_player_jpeg_subsampling_t)img_info.format;
    return AVDK_ERR_OK;
}

