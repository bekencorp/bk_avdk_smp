// Copyright 2024-2025 Beken
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

#ifndef __CSI_CALIB_MEM_H__
#define __CSI_CALIB_MEM_H__

#include <os/mem.h>

/*
 * Shared allocator for CSI sensor ISP calibration copies.
 *
 * Every CSI sensor driver keeps a mutable per-port copy of its ~20 KiB static
 * calibration table (the "<SENSOR>_..._CalibParam_dynamic" object). By default
 * this copy is drawn from the small AP SRAM heap; CONFIG_CSI_CALIB_USE_PSRAM
 * routes it to PSRAM instead, relieving SRAM pressure. This is the single
 * control point shared by all csi_*.c drivers - do not add per-sensor macros.
 */
#if CONFIG_CSI_CALIB_USE_PSRAM
#define CSI_CALIB_MALLOC(size) psram_malloc(size)
#define CSI_CALIB_FREE(ptr)    psram_free(ptr)
#else
#define CSI_CALIB_MALLOC(size) os_malloc(size)
#define CSI_CALIB_FREE(ptr)    os_free(ptr)
#endif

#endif /* __CSI_CALIB_MEM_H__ */
