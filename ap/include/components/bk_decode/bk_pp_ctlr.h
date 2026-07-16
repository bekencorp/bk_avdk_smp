// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "bk_pp_types.h"

/**
 * @brief Create a standalone PP (post-processor) controller
 * @param handle Output pointer that receives the PP controller handle
 * @param config PP controller configuration
 * @return AVDK error code
 */
avdk_err_t bk_pp_ctlr_new(bk_pp_ctlr_handle_t *handle, bk_pp_config_t *config);

/**
 * @brief Initialize the PP controller (bring up the shared decoder/PP power
 *        domain + IRQ and create the underlying PP instance)
 * @param handle PP controller handle
 * @return AVDK error code
 */
avdk_err_t bk_pp_init(bk_pp_ctlr_handle_t handle);

/**
 * @brief Open the PP controller for processing
 * @param handle PP controller handle
 * @return AVDK error code
 */
avdk_err_t bk_pp_open(bk_pp_ctlr_handle_t handle);

/**
 * @brief Run one blocking PP operation (NV12 scale and/or format conversion)
 * @param handle PP controller handle
 * @param req Input / output buffer descriptors for this operation
 * @return AVDK error code
 */
avdk_err_t bk_pp_process(bk_pp_ctlr_handle_t handle, const bk_pp_process_req_t *req);

/**
 * @brief Close the PP controller
 * @param handle PP controller handle
 * @return AVDK error code
 */
avdk_err_t bk_pp_close(bk_pp_ctlr_handle_t handle);

/**
 * @brief Deinitialize the PP controller (tear down the PP instance and release
 *        the decoder/PP power domain + IRQ)
 * @param handle PP controller handle
 * @return AVDK error code
 */
avdk_err_t bk_pp_deinit(bk_pp_ctlr_handle_t handle);

/**
 * @brief Delete the PP controller
 * @param handle PP controller handle
 * @return AVDK error code
 */
avdk_err_t bk_pp_delete(bk_pp_ctlr_handle_t handle);

#ifdef __cplusplus
}
#endif
