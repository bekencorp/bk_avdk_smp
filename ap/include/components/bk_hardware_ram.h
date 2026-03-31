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

#if CONFIG_ISP_MAX_WIDTH == 1280
#define ISP_MAX_WIDTH				(1280)
#elif CONFIG_ISP_MAX_WIDTH == 1920
#define ISP_MAX_WIDTH				(1920)
#elif CONFIG_ISP_MAX_WIDTH == 2560
#define ISP_MAX_WIDTH				(2560)
#else
#define ISP_MAX_WIDTH				(1920)
#endif

#if CONFIG_GPU_MAX_WIDTH == 1280
#define GPU_MAX_WIDTH				(1280)
#elif CONFIG_GPU_MAX_WIDTH == 1920
#define GPU_MAX_WIDTH				(1920)
#elif CONFIG_GPU_MAX_WIDTH == 2560
#define GPU_MAX_WIDTH				(2560)
#else
#define GPU_MAX_WIDTH				(1920)
#endif

#define ISP_BUFFER_SIZE				(((ISP_MAX_WIDTH * 16 * 3 / 2 * 3 + 64 + 63) / 64) * 64)
#define GPU_BUFFER_SIZE				(((CONFIG_VG_LITE_GPU_CONTIGUOUS_MEM_SZ + 63) / 64) * 64)
#define GPU_PINGPONG_BUFFER_SIZE	(((GPU_MAX_WIDTH * 16 * 2 + 64 + 63) / 64) * 64)
//#define DPU_BUFFER_SIZE				(((32560 + 63) / 64) * 64)
//#define H264E_BUFFER_SIZE			(((57664 + 63) / 64) * 64)

//#define ALL_MEDIA_SRAM_SIZE (ISP_BUFFER_SIZE + GPU_BUFFER_SIZE + GPU_PINGPONG_BUFFER_SIZE + DPU_BUFFER_SIZE + H264E_BUFFER_SIZE)

void *bk_get_isp_flexa_buffer(uint32_t size);
void *bk_get_gpu_flexa_buffer(uint32_t size);
void *bk_get_dpu_buffer(uint32_t size);
void *bk_get_gpu_output_buffer(uint32_t size);

#ifdef __cplusplus
}
#endif


