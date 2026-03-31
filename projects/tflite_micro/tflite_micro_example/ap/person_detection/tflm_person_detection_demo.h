#pragma once

#include "os/os.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Run the TFLM Person Detection demo.
 * This function performs person detection (person/no person) on test images.
 *
 * @return bk_err_t BK_OK on success, error code otherwise
 */
bk_err_t tflm_person_detection_run_demo(void);

#ifdef __cplusplus
}
#endif

