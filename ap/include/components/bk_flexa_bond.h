// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS-IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <os/os.h>
#include <components/avdk_utils/avdk_error.h>
#include <components/bk_decode/bk_jpeg_decode_ctlr.h>
#include <components/bk_encode/bk_h264_encode_ctlr.h>
#include <components/bk_gpu_ctlr.h>

avdk_err_t bk_flexa_mjpegd_h264e_bond_start(void **bond,
					   bk_jpeg_decode_ctlr_handle_t jpeg,
					   bk_h264_encode_ctlr_handle_t h264);
void bk_flexa_mjpegd_h264e_bond_stop(void *bond);

avdk_err_t bk_flexa_mjpegd_gpu_bond_start(void **bond,
					  bk_jpeg_decode_ctlr_handle_t jpeg,
					  bk_gpu_ctlr_handle_t gpu);
void bk_flexa_mjpegd_gpu_bond_stop(void *bond);

avdk_err_t bk_flexa_isp_h264e_bond_start(void **bond, void *isp,
					bk_h264_encode_ctlr_handle_t h264);
void bk_flexa_isp_h264e_bond_stop(void *bond);

avdk_err_t bk_flexa_isp_gpu_bond_start(void **bond, void *isp, bk_gpu_ctlr_handle_t gpu);
void bk_flexa_isp_gpu_bond_stop(void *bond);

#ifdef __cplusplus
}
#endif
