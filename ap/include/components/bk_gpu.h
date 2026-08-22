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

#include "bk_gpu_types.h"

#ifdef __cplusplus
extern "C" {
#endif


avdk_err_t bk_gpu_init(bk_gpu_ctlr_handle_t handle);
avdk_err_t bk_gpu_deinit(bk_gpu_ctlr_handle_t handle);
avdk_err_t bk_gpu_open(bk_gpu_ctlr_handle_t handle);
avdk_err_t bk_gpu_close(bk_gpu_ctlr_handle_t handle);
avdk_err_t bk_gpu_ioctl(bk_gpu_ctlr_handle_t handle, uint32_t cmd, void *args);
avdk_err_t bk_gpu_delete(bk_gpu_ctlr_handle_t handle);
avdk_err_t bk_gpu_draw_path_clear(bk_gpu_ctlr_handle_t handle);
avdk_err_t bk_gpu_draw_path_build(bk_gpu_ctlr_handle_t handle, bk_gpu_draw_path_set_t *path_set);

/**
 * @brief Set a single-layer blit overlay on the GPU output.
 *
 * Queues @p src_buffer to be composited onto every finished GPU output frame
 * at blit_config->dst_x/y with the given crop/format/rotation/blend, using
 * blit_config->osd_slot as the layer index. The GPU takes ownership of
 * @p src_buffer until blit_config->free() runs (on replace, clear, or delete).
 *
 * @note bk_gpu_blit_set(), bk_gpu_layer_* and bk_draw_osd all share the single
 *       overlay owned by the controller and can be mixed on one controller;
 *       the controller composites every active layer onto each output frame.
 *
 * @return AVDK_ERR_OK on success; AVDK_ERR_INVAL on bad args.
 */
avdk_err_t bk_gpu_blit_set(bk_gpu_ctlr_handle_t handle, void *src_buffer, bk_gpu_blit_config_t *blit_config);

/**
 * @brief Remove all blit slots set via bk_gpu_blit_set().
 *
 * Releases every slot's source buffer (via its free callback). Safe to call
 * when nothing was ever set.
 *
 * @return AVDK_ERR_OK on success; AVDK_ERR_INVAL if handle is NULL.
 */
avdk_err_t bk_gpu_blit_clear(bk_gpu_ctlr_handle_t handle);

/**
 * @brief Remove a single blit slot set via bk_gpu_blit_set().
 *
 * Releases that slot's source buffer (via its free callback) and drops the
 * layer from the composition. Other slots are untouched. Safe to call on a
 * slot that was never set.
 *
 * @param handle GPU controller.
 * @param slot   Slot index [0, BK_GPU_BLIT_SLOT_MAX).
 * @return AVDK_ERR_OK on success; AVDK_ERR_INVAL on bad args.
 */
avdk_err_t bk_gpu_blit_clear_slot(bk_gpu_ctlr_handle_t handle, uint8_t slot);

#ifdef __cplusplus
}
#endif

