#pragma once

#include <components/avdk_utils/avdk_error.h>

#ifdef __cplusplus
extern "C" {
#endif

avdk_err_t pp_dpu_open(void);
void pp_dpu_close(void);
avdk_err_t pp_dpu_flush(void *frame, avdk_err_t (*free_cb)(void *args));
uint16_t pp_dpu_width(void);
uint16_t pp_dpu_height(void);

#ifdef __cplusplus
}
#endif
