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

typedef struct
{
    uint8_t  enable_debug_pin;
    uint16_t width;
    uint16_t height;
    uint16_t input_format;
    uint16_t clk_io; // csi input clk io
    uint32_t clk; // clk value
} mipi_csi_config_t;

typedef struct
{
    uint8_t  state;
    uint8_t  enable_debug_pin;
    uint16_t width;
    uint16_t height;
    uint16_t input_format;
} mipi_csi_control_t;

typedef void *mipi_csi_handle_t;

#define MIPI_CSI_DEFAULT_CONFIG() {    \
    .enable_debug_pin = 1,        \
    .width = 1920,        \
    .height = 1088,        \
    .input_format = PIXEL_FORMAT_RAW10,    \
    .clk_io = 59,        \
    .clk = 20000000,        \
}


#ifdef __cplusplus
}
#endif
