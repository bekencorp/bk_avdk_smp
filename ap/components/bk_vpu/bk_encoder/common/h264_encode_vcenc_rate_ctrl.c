#include <components/log.h>
#include <components/avdk_utils/avdk_error.h>

#include "h264_encode_vcenc_rate_ctrl_priv.h"
#include "modules/vcenc/vcenc_h264_api.h"

#define TAG "h264_encode_vcenc_rc"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

static void h264_encode_vcenc_rate_ctrl_to_bk(const vcenc_rate_ctrl_t *src,
                                              bk_h264_encode_vcenc_rate_ctrl_t *dst)
{
    dst->crf = src->crf;
    dst->picture_rc = src->picture_rc;
    dst->ctb_rc = src->ctb_rc;
    dst->block_rc_size = src->block_rc_size;
    dst->picture_skip = src->picture_skip;
    dst->qp_hdr = src->qp_hdr;
    dst->qp_min_pb = src->qp_min_pb;
    dst->qp_max_pb = src->qp_max_pb;
    dst->qp_min_i = src->qp_min_i;
    dst->qp_max_i = src->qp_max_i;
    dst->bit_per_second = src->bit_per_second;
    dst->cpb_max_rate = src->cpb_max_rate;
    dst->filler_data = src->filler_data;
    dst->hrd = src->hrd;
    dst->hrd_cpb_size = src->hrd_cpb_size;
    dst->bitrate_window = src->bitrate_window;
    dst->intra_qp_delta = src->intra_qp_delta;
    dst->fixed_intra_qp = src->fixed_intra_qp;
    dst->bit_var_range_i = src->bit_var_range_i;
    dst->bit_var_range_p = src->bit_var_range_p;
    dst->bit_var_range_b = src->bit_var_range_b;
    dst->tol_moving_bit_rate = src->tol_moving_bit_rate;
    dst->monitor_frames = src->monitor_frames;
    dst->target_pic_size = src->target_pic_size;
    dst->smooth_psnr_in_gop = src->smooth_psnr_in_gop;
    dst->static_scene_i_bit_percent = src->static_scene_i_bit_percent;
    dst->rc_qp_delta_range = src->rc_qp_delta_range;
    dst->rc_base_mb_complexity = src->rc_base_mb_complexity;
    dst->pic_qp_delta_min = src->pic_qp_delta_min;
    dst->pic_qp_delta_max = src->pic_qp_delta_max;
    dst->long_term_qp_delta = src->long_term_qp_delta;
    dst->vbr = src->vbr;
    dst->rc_mode = src->rc_mode;
    dst->tol_ctb_rc_inter = src->tol_ctb_rc_inter;
    dst->tol_ctb_rc_intra = src->tol_ctb_rc_intra;
    dst->tol_rc_underflow = src->tol_rc_underflow;
    dst->max_i_prop = src->max_i_prop;
    dst->min_i_prop = src->min_i_prop;
    dst->change_pos = src->change_pos;
    dst->ctb_rc_row_qp_step = src->ctb_rc_row_qp_step;
    dst->ctb_rc_row_qp_delta_range = src->ctb_rc_row_qp_delta_range;
    dst->ctb_rc_qp_delta_reverse = src->ctb_rc_qp_delta_reverse;
    dst->frame_rate_num = src->frame_rate_num;
    dst->frame_rate_denom = src->frame_rate_denom;
    dst->hie_qp_delta_enable = src->hie_qp_delta_enable;
}

static void h264_encode_bk_rate_ctrl_to_vcenc(const bk_h264_encode_vcenc_rate_ctrl_t *src,
                                              vcenc_rate_ctrl_t *dst)
{
    dst->crf = src->crf;
    dst->picture_rc = src->picture_rc;
    dst->ctb_rc = src->ctb_rc;
    dst->block_rc_size = src->block_rc_size;
    dst->picture_skip = src->picture_skip;
    dst->qp_hdr = src->qp_hdr;
    dst->qp_min_pb = src->qp_min_pb;
    dst->qp_max_pb = src->qp_max_pb;
    dst->qp_min_i = src->qp_min_i;
    dst->qp_max_i = src->qp_max_i;
    dst->bit_per_second = src->bit_per_second;
    dst->cpb_max_rate = src->cpb_max_rate;
    dst->filler_data = src->filler_data;
    dst->hrd = src->hrd;
    dst->hrd_cpb_size = src->hrd_cpb_size;
    dst->bitrate_window = src->bitrate_window;
    dst->intra_qp_delta = src->intra_qp_delta;
    dst->fixed_intra_qp = src->fixed_intra_qp;
    dst->bit_var_range_i = src->bit_var_range_i;
    dst->bit_var_range_p = src->bit_var_range_p;
    dst->bit_var_range_b = src->bit_var_range_b;
    dst->tol_moving_bit_rate = src->tol_moving_bit_rate;
    dst->monitor_frames = src->monitor_frames;
    dst->target_pic_size = src->target_pic_size;
    dst->smooth_psnr_in_gop = src->smooth_psnr_in_gop;
    dst->static_scene_i_bit_percent = src->static_scene_i_bit_percent;
    dst->rc_qp_delta_range = src->rc_qp_delta_range;
    dst->rc_base_mb_complexity = src->rc_base_mb_complexity;
    dst->pic_qp_delta_min = src->pic_qp_delta_min;
    dst->pic_qp_delta_max = src->pic_qp_delta_max;
    dst->long_term_qp_delta = src->long_term_qp_delta;
    dst->vbr = src->vbr;
    dst->rc_mode = src->rc_mode;
    dst->tol_ctb_rc_inter = src->tol_ctb_rc_inter;
    dst->tol_ctb_rc_intra = src->tol_ctb_rc_intra;
    dst->tol_rc_underflow = src->tol_rc_underflow;
    dst->max_i_prop = src->max_i_prop;
    dst->min_i_prop = src->min_i_prop;
    dst->change_pos = src->change_pos;
    dst->ctb_rc_row_qp_step = src->ctb_rc_row_qp_step;
    dst->ctb_rc_row_qp_delta_range = src->ctb_rc_row_qp_delta_range;
    dst->ctb_rc_qp_delta_reverse = src->ctb_rc_qp_delta_reverse;
    dst->frame_rate_num = src->frame_rate_num;
    dst->frame_rate_denom = src->frame_rate_denom;
    dst->hie_qp_delta_enable = src->hie_qp_delta_enable;
}

static bool h264_encode_vcenc_rate_ctrl_is_valid(const bk_h264_encode_vcenc_rate_ctrl_t *rate_ctrl)
{
    return rate_ctrl->qp_min_i <= 51 && rate_ctrl->qp_max_i <= 51 &&
           rate_ctrl->qp_min_pb <= 51 && rate_ctrl->qp_max_pb <= 51 &&
           rate_ctrl->qp_hdr <= 51 && rate_ctrl->qp_hdr >= -1 &&
           rate_ctrl->fixed_intra_qp <= 51 &&
           (!rate_ctrl->qp_min_i || !rate_ctrl->qp_max_i || rate_ctrl->qp_min_i <= rate_ctrl->qp_max_i) &&
           (!rate_ctrl->qp_min_pb || !rate_ctrl->qp_max_pb || rate_ctrl->qp_min_pb <= rate_ctrl->qp_max_pb);
}

avdk_err_t h264_encode_set_vcenc_rate_ctrl_common(h264_enc_param_t *enc_param,
                                                  bool encoder_inited,
                                                  bk_h264_encode_vcenc_rate_ctrl_t *rate_ctrl)
{
    vcenc_rate_ctrl_t vcenc_rc = {0};

    if (rate_ctrl == NULL || enc_param == NULL) {
        LOGE("vcenc rate_ctrl arg is NULL\r\n");
        return AVDK_ERR_INVAL;
    }
    if (!encoder_inited) {
        LOGE("h264 encoder is not opened\r\n");
        return AVDK_ERR_INVAL;
    }
    if (!h264_encode_vcenc_rate_ctrl_is_valid(rate_ctrl)) {
        LOGE("invalid vcenc rate_ctrl qp range\r\n");
        return AVDK_ERR_INVAL;
    }

    h264_encode_bk_rate_ctrl_to_vcenc(rate_ctrl, &vcenc_rc);
    if (vcenc_h264_set_rate_ctrl(enc_param, &vcenc_rc) != VCENC_OK) {
        LOGE("vcenc_h264_set_rate_ctrl failed\r\n");
        return AVDK_ERR_GENERIC;
    }
    return AVDK_ERR_OK;
}

avdk_err_t h264_encode_get_vcenc_rate_ctrl_common(h264_enc_param_t *enc_param,
                                                  bool encoder_inited,
                                                  bk_h264_encode_vcenc_rate_ctrl_t *rate_ctrl)
{
    vcenc_rate_ctrl_t vcenc_rc;

    if (rate_ctrl == NULL || enc_param == NULL) {
        LOGE("vcenc rate_ctrl arg is NULL\r\n");
        return AVDK_ERR_INVAL;
    }
    if (!encoder_inited) {
        LOGE("h264 encoder is not opened\r\n");
        return AVDK_ERR_INVAL;
    }
    if (vcenc_h264_get_rate_ctrl(enc_param, &vcenc_rc) != VCENC_OK) {
        LOGE("vcenc_h264_get_rate_ctrl failed\r\n");
        return AVDK_ERR_GENERIC;
    }

    h264_encode_vcenc_rate_ctrl_to_bk(&vcenc_rc, rate_ctrl);
    return AVDK_ERR_OK;
}
