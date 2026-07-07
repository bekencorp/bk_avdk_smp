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

#include "modules/vcdec/vcdec_types.h"

#ifdef __cplusplus
extern "C" {
#endif

vcdec_pp_out_format_e bk_decode_pp_map_out_format(uint32_t fmt);
uint32_t bk_decode_pp_output_size(uint32_t fmt, uint16_t width, uint16_t height);
uint32_t bk_decode_pp_flexa_rb_size(uint16_t width, uint16_t segment_height_mb,
				    uint8_t segment_num);

#ifdef __cplusplus
}
#endif
