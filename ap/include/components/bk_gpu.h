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
avdk_err_t bk_gpu_ioctl(bk_gpu_ctlr_handle_t handle, uint32_t cmd, void *arg);
avdk_err_t bk_gpu_delete(bk_gpu_ctlr_handle_t handle);
avdk_err_t bk_gpu_draw_path_clear(bk_gpu_ctlr_handle_t handle);
avdk_err_t bk_gpu_draw_path_build(bk_gpu_ctlr_handle_t handle, bk_gpu_draw_path_set_t *path_set);
avdk_err_t bk_gpu_blit_set(bk_gpu_ctlr_handle_t handle, void *src_buffer, bk_gpu_blit_config_t *blit_config);
avdk_err_t bk_gpu_blit_clear(bk_gpu_ctlr_handle_t handle);
avdk_err_t bk_gpu_ioctl(bk_gpu_ctlr_handle_t handle, uint32_t cmd, void *args);

#ifdef __cplusplus
}
#endif

