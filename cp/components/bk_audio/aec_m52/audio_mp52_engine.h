#pragma once

#include <os/mem.h>

#include <common/bk_typedef.h>
#include <common/bk_err.h>

#include <modules/audio_mp52_ipc.h>

#ifdef __cplusplus
extern "C" {
#endif

bk_err_t aec_m52_engine_init(uint32_t fs);
bk_err_t aec_m52_engine_apply_ctrl(const aec_m52_ctrl_cfg_t *ctrl_cfg);
bk_err_t aec_m52_engine_process(aec_m52_slot_desc_t *slot_desc);

#ifdef __cplusplus
}
#endif
