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

#include <stdint.h>
#include <common/bk_err.h>

#ifdef __cplusplus
extern "C" {
#endif


void bk_gpu_driver_init(void);

void bk_gpu_driver_deinit(void);

/* Apply VG-Lite runtime mem config (base/cmd/tess) and return the contiguous
 * heap size the caller must allocate. If CONFIG_VG_LITE_GPU_TESS_WIDTH/HEIGHT
 * are non-zero, the heap is always sized for that maximum and a larger tess
 * request fails (returns 0). tess 0 x 0 disables the tess buffer only when
 * those Kconfig values are 0. */
uint32_t bk_gpu_vg_lite_apply_mem_config(uint32_t tess_width, uint32_t tess_height);

/* Global VG-Lite hardware lock shared by all GPU users. */
bk_err_t bk_gpu_global_lock(void);

bk_err_t bk_gpu_global_unlock(void);


#ifdef __cplusplus
}
#endif
