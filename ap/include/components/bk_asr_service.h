#pragma once

#include <components/bk_asr_service_types.h>

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus


aud_asr_handle_t bk_aud_asr_init(aud_asr_cfg_t *cfg);
bk_err_t bk_aud_asr_deinit(aud_asr_handle_t aud_asr_handle);
bk_err_t bk_aud_asr_start(aud_asr_handle_t aud_asr_handle);
bk_err_t bk_aud_asr_stop(aud_asr_handle_t aud_asr_handle);

#if CONFIG_AUD_PM_FAST_COLD
bk_err_t bk_aud_asr_pm_save_cfg(const aud_asr_cfg_t *cfg);
aud_asr_handle_t bk_aud_asr_pm_get_handle(void);
void bk_aud_asr_pm_clear(void);
#endif


#ifdef  __cplusplus
}
#endif//__cplusplus

/**
 * @}
 */