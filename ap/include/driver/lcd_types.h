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

/*
 * NOTE: All legacy LCD types previously declared in this file have been
 * removed (they were wrapped in `#if 0` and unused). This header is now a
 * thin forwarding stub kept only for source-level backward compatibility:
 * it pulls in the QSPI/MCU-related typedefs so existing
 * `#include <driver/lcd_types.h>` lines keep compiling.
 *
 * New code SHOULD include the concrete headers directly:
 *   - `driver/lcd_qspi_types.h` for QSPI / SPI panel types
 *   - `components/bk_lcd_types.h` for `bk_lcd_panel_t` / panel device entry
 *   - `components/bk_display_types.h` for the new display framework types
 *
 * This stub will be removed entirely once all `#include` sites have been
 * migrated (planned C2 header-reorganization commit).
 */

#include "driver/lcd_qspi_types.h"
