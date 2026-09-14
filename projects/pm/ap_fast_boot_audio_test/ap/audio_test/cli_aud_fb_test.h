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

/**
 * Unified CLI for AP fast-boot + audio service smoke test.
 *
 *   aud_fb service voice|asr|player init [args...]
 *   aud_fb service voice|asr|player stop     (keep cfg, auto resume)
 *   aud_fb service voice|asr|player deinit   (clear cfg, no resume)
 *   aud_fb service status
 *   aud_fb auto_ap_off on|off
 *   aud_fb ap off
 *
 * stop is the main cold-fast-boot test: tear objects, keep cfg.
 * deinit is a true off: tear objects and clear cfg so app_resume skips it.
 * Multiple services may run together. When the last service stops/deinits
 * (and auto_ap_off=on), AP requests CP via customer IPC; CP votes AP OFF.
 * Bring AP back with CP `ap_fast_boot on`.
 */
int cli_aud_fb_test_init(void);

#ifdef __cplusplus
}
#endif
