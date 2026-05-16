#pragma once

#include "os/os.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Run one round: test_hands_1 then test_hands_2 embedded inputs; log bbox / score / class_id.
 */
bk_err_t tflm_hand_gesture_detection_run_demo(void);

#ifdef __cplusplus
}
#endif
