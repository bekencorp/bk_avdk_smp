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

#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Jump buffer used to recover from jerry_port_fatal().  setjmp() must be
 * executed in the frame that is expected to survive the fatal error, so it
 * cannot be wrapped by a helper function.
 */
extern jmp_buf bk_js_fatal_jmp;

/**
 * True while the code calling into the engine is inside a guarded region.
 */
extern volatile bool bk_js_guard_armed;

/**
 * @brief Select the engine heap size used by jerry_port_context_alloc().
 *
 * @param heap_size_kb heap size in KB, must be in the [64, 512] range.
 */
void bk_js_port_set_heap_size(uint32_t heap_size_kb);

/**
 * @brief Get the fatal code captured by the last jerry_port_fatal() call.
 */
int bk_js_port_fatal_code(void);

#ifdef __cplusplus
}
#endif
