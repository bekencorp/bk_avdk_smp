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

#include <stdbool.h>
#include <stdint.h>

#include "jerryscript.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BK_JS_PATH_MAX   (128)
#define BK_JS_ID_MAX     (40)

typedef struct {
	char id[BK_JS_ID_MAX];
	char name[BK_JS_ID_MAX];
	char version[20];
	char entry[64];
} bk_js_manifest_t;

/* ------------------------------------------------------------------ */
/* Console output, implemented in bk_js_runtime.c                     */
/* ------------------------------------------------------------------ */

void bk_js_console_text(const char *text, uint32_t len, bool newline);

/**
 * Print a value with print()-like formatting.
 */
void bk_js_console_value(jerry_value_t value, bool newline);

/**
 * Print an exception and release it.  The caller must not free the value
 * afterwards.
 */
void bk_js_console_exception(jerry_value_t exception);

/* ------------------------------------------------------------------ */
/* Runtime services used by the bindings                              */
/* ------------------------------------------------------------------ */

void bk_js_post_event(uint32_t slot_id, uint32_t code, bool deleting);

/**
 * Post an event together with the pointer position captured while the LVGL
 * event was generated.  Callbacks run later in the JavaScript task, where the
 * live input device state is no longer meaningful, so the coordinates travel
 * with the message instead.
 */
void bk_js_post_event_touch(uint32_t slot_id, uint32_t code, bool deleting,
                            int32_t x, int32_t y, bool has_point);

/**
 * Pointer position of the event being dispatched, if it carried one.
 */
bool bk_js_event_touch_get(int32_t *x, int32_t *y);

/**
 * Flag the callbacks of a deleted object.  Safe to call from the LVGL thread:
 * the JavaScript task frees the flagged slots on its next loop iteration.
 */
void bk_js_event_mark_dead(uint32_t owner, uint32_t gen);
void bk_js_event_sweep(void);

void bk_js_runtime_request_app_exit(void);

/**
 * Call a script function with the engine fatal guard armed and the default
 * timeout applied.  Must be called from the JavaScript task.
 */
jerry_value_t bk_js_call_guarded(jerry_value_t fn, const jerry_value_t *args, uint32_t count);

/* Event slots, owned by the runtime.  Callbacks are bound to the wrapper
 * generation so that recycled lv_obj_t addresses cannot inherit them. */
uint32_t bk_js_event_add(uint32_t owner, uint32_t gen, uint32_t code, jerry_value_t fn);
void bk_js_event_remove(uint32_t owner, uint32_t gen);
void bk_js_event_clear(void);
void bk_js_event_dispatch(uint32_t slot_id, uint32_t code, bool deleting,
                          int32_t x, int32_t y, bool has_point);

/* Timers, owned by the runtime. */
uint32_t bk_js_timer_add(jerry_value_t fn, uint32_t ms, bool repeat);
void bk_js_timer_clear_id(uint32_t id);
void bk_js_timer_clear_all(void);

/* ------------------------------------------------------------------ */
/* Bindings                                                           */
/* ------------------------------------------------------------------ */

void bk_js_fs_bind(jerry_value_t global);

#if CONFIG_BK_JERRYSCRIPT_UI
void bk_js_ui_bind(jerry_value_t global);
void bk_js_ui_shutdown(void);

/**
 * Serialize JavaScript with LVGL rendering: hold the display mutex around the
 * whole script/timer/event callback executed by the JavaScript task.  Nested
 * calls are counted, so bindings may lock again.  Returns false when the
 * display is not initialized yet.
 */
bool bk_js_ui_lock(void);
void bk_js_ui_unlock(void);
void bk_js_ui_reset_lock(void);
#endif

/* ------------------------------------------------------------------ */
/* Filesystem helpers                                                 */
/* ------------------------------------------------------------------ */

char *bk_js_fs_read(const char *path, uint32_t *out_len);
bool bk_js_fs_exists(const char *path);

/**
 * Mount the application file system on the apps root when it is not mounted
 * yet.  Returns 0 on success.
 */
int bk_js_fs_mount(void);

/**
 * Unmount the application file system if it was mounted by bk_js_fs_mount().
 * A file system mounted by someone else is left untouched.  Returns 0 on
 * success or when nothing had to be done.
 */
int bk_js_fs_unmount(void);

void bk_js_fs_set_root(const char *root);
const char *bk_js_fs_root(void);
void bk_js_fs_set_dir(const char *dir);
const char *bk_js_fs_dir(void);
char *bk_js_fs_resolve(const char *path, char *buf, uint32_t size);
void bk_js_fs_clear_modules(void);
void bk_js_fs_forget_modules(void);

/* ------------------------------------------------------------------ */
/* Application manifest / scanning, implemented in bk_js_app.c        */
/* ------------------------------------------------------------------ */

bool bk_js_manifest_parse(const char *json, bk_js_manifest_t *out);
uint32_t bk_js_scan_apps(const char *root, bk_js_manifest_t *out, uint32_t max);

/**
 * Built-in demo application used when no filesystem application is found.
 */
const char *bk_js_builtin_app_source(void);
const char *bk_js_builtin_app_name(void);

#ifdef __cplusplus
}
#endif
