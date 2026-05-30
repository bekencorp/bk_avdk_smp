#pragma once

#include <stdint.h>
#include <components/avdk_utils/avdk_error.h>

#ifdef __cplusplus
extern "C" {
#endif

int cli_h264d_gpu_display_init(void);
avdk_err_t h264d_gpu_display_start(uint32_t max_loops);

#ifdef __cplusplus
}
#endif
