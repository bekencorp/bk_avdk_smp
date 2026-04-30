#pragma once

#include "os/os.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Run one round: infer cat.200 input, then dog.204 input; log bbox / score / class_id.
 * Intended to be called in a loop from ap_main (same pattern as tflite_micro_example).
 */
bk_err_t tflm_pet_detection_run_demo(void);

#ifdef __cplusplus
}
#endif
