#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "components/bk_encode/bk_h264_encode_ctlr.h"
#include "modules/vcenc/vcenc_h264_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int crf;
    uint32_t picture_rc;
    uint32_t ctb_rc;
    uint32_t block_rc_size;
    uint32_t picture_skip;
    int qp_hdr;
    uint32_t qp_min_pb;
    uint32_t qp_max_pb;
    uint32_t qp_min_i;
    uint32_t qp_max_i;
    uint32_t bit_per_second;
    uint32_t cpb_max_rate;
    uint32_t filler_data;
    uint32_t hrd;
    uint32_t hrd_cpb_size;
    uint32_t bitrate_window;
    int intra_qp_delta;
    uint32_t fixed_intra_qp;
    int bit_var_range_i;
    int bit_var_range_p;
    int bit_var_range_b;
    int tol_moving_bit_rate;
    int monitor_frames;
    int target_pic_size;
    int smooth_psnr_in_gop;
    uint32_t static_scene_i_bit_percent;
    uint32_t rc_qp_delta_range;
    uint32_t rc_base_mb_complexity;
    int pic_qp_delta_min;
    int pic_qp_delta_max;
    int long_term_qp_delta;
    int vbr;
    uint32_t rc_mode;
    float tol_ctb_rc_inter;
    float tol_ctb_rc_intra;
    int tol_rc_underflow;
    uint32_t max_i_prop;
    uint32_t min_i_prop;
    int change_pos;
    int ctb_rc_row_qp_step;
    int ctb_rc_row_qp_delta_range;
    uint32_t ctb_rc_qp_delta_reverse;
    uint32_t frame_rate_num;
    uint32_t frame_rate_denom;
    uint32_t hie_qp_delta_enable;
} bk_h264_encode_vcenc_rate_ctrl_t;

#define H264_ENCODE_IOCTL_SET_VCENC_RATE_CTRL_PRIV (0x1000U)
#define H264_ENCODE_IOCTL_GET_VCENC_RATE_CTRL_PRIV (0x1001U)

avdk_err_t h264e_stream_encode_set_vcenc_rate_ctrl(bk_h264_encode_ctlr_handle_t handle,
                                                   bk_h264_encode_vcenc_rate_ctrl_t *rate_ctrl);
avdk_err_t h264e_stream_encode_get_vcenc_rate_ctrl(bk_h264_encode_ctlr_handle_t handle,
                                                   bk_h264_encode_vcenc_rate_ctrl_t *rate_ctrl);
avdk_err_t h264_encode_set_vcenc_rate_ctrl_common(h264_enc_param_t *enc_param,
                                                  bool encoder_inited,
                                                  bk_h264_encode_vcenc_rate_ctrl_t *rate_ctrl);
avdk_err_t h264_encode_get_vcenc_rate_ctrl_common(h264_enc_param_t *enc_param,
                                                  bool encoder_inited,
                                                  bk_h264_encode_vcenc_rate_ctrl_t *rate_ctrl);

#ifdef __cplusplus
}
#endif
