// Copyright 2025-2026 Beken
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

#include <modules/private/veri_isp/vsi_comm_video.h>
#include <common/avdk_pixel_types.h>

PIXEL_FORMAT_E isp_camera_format_convert(bk_pixel_format_t bk_format);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

