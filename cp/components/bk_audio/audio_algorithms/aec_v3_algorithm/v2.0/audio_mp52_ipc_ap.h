#pragma once

#include <common/bk_typedef.h>
#include <common/bk_err.h>
#include <stdint.h>
#include <modules/audio_mp52_ipc.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aec_m52_proxy aec_m52_proxy_t;
typedef struct {
    uint8_t test;
    int16_t spcnt;
    int16_t dcnt;
    int32_t dc;
    int32_t mic_max;
    int32_t vad_hr;
    int32_t phs_cur;
} aec_m52_feedback_t;

aec_m52_proxy_t *aec_m52_proxy_create(const aec_m52_ctrl_cfg_t *ctrl_cfg,
                                      uint32_t ref_bytes,
                                      uint32_t mic_bytes,
                                      uint32_t out_bytes);

void aec_m52_proxy_destroy(aec_m52_proxy_t *proxy);

bk_err_t aec_m52_proxy_copy_last_output(aec_m52_proxy_t *proxy, int16_t *out, uint32_t out_bytes);
bk_err_t aec_m52_proxy_copy_last_feedback(aec_m52_proxy_t *proxy, aec_m52_feedback_t *feedback);
bk_err_t aec_m52_proxy_copy_last_ecout(aec_m52_proxy_t *proxy, uint8_t *ecout, uint32_t ecout_bytes);
bk_err_t aec_m52_proxy_submit(aec_m52_proxy_t *proxy, const int16_t *ref, const int16_t *mic);
bk_err_t aec_m52_proxy_update_ctrl(aec_m52_proxy_t *proxy, const aec_m52_ctrl_cfg_t *ctrl_cfg);

#ifdef __cplusplus
}
#endif
