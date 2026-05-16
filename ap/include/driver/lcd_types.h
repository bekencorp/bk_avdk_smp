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
#include "driver/lcd_qspi_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Rectangular region descriptor used by the partial-display path of the
 *        SPI and QSPI LCD drivers (::bk_lcd_spi_partial_display() and
 *        ::bk_lcd_qspi_partial_display()).
 *
 * Coordinates are inclusive end-points in pixels and follow the panel's
 * native column/row addressing (i.e. matched by the column / page address-set
 * commands the driver IC accepts).
 */
typedef struct {
    uint16_t x_start;
    uint16_t y_start;
    uint16_t x_end;
    uint16_t y_end;
} lcd_display_area_t;

#ifdef __cplusplus
}
#endif
