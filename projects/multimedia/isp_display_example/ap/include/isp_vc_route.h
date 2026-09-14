#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <avdk_error.h>

#ifdef __cplusplus
extern "C" {
#endif

bool isp_vc_route_is_active(void);

avdk_err_t isp_vc_route_turn_on(uint16_t sensor_w, uint16_t sensor_h, uint16_t fps,
                                uint16_t isp_w, uint16_t isp_h, uint8_t default_vc);
avdk_err_t isp_vc_route_turn_off(void);

avdk_err_t isp_vc_route_vc_enable(uint8_t vc, uint8_t discard_frames);
avdk_err_t isp_vc_route_vc_disable(uint8_t vc);
avdk_err_t isp_vc_route_select(uint8_t vc);

#ifdef __cplusplus
}
#endif
