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

#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/log.h>

#include "jerryscript.h"
#include "jerryscript-port.h"

#include "bk_jerryscript.h"
#include "bk_js_app.h"
#include "bk_js_port.h"
#include "bk_js_internal.h"

#define BK_JS_TAG "js"

/* Console output is printed in small chunks to stay below the log buffer size. */
#define BK_JS_CONSOLE_CHUNK   (128)

/* Number of backward branches between two timeout checks. */
#define BK_JS_HALT_INTERVAL   (64)

/* Touch driven UIs generate bursts of pressed/pressing/released and scroll
 * events, so keep enough room to avoid dropping the closing release. */
#define BK_JS_QUEUE_LEN       (16)
#define BK_JS_TIMER_MAX       (16)
#define BK_JS_EVENT_MAX       (48)

typedef enum {
	JS_MSG_EVAL = 1,
	JS_MSG_EVAL_FILE,
	JS_MSG_APP_START,
	JS_MSG_APP_STOP,
	JS_MSG_EVENT,
	JS_MSG_EXIT,
} js_msg_type_t;

typedef struct {
	uint8_t type;
	uint8_t deleting;
	uint8_t has_point;
	uint8_t reserved;
	uint32_t id;
	uint32_t code;
	uint32_t timeout_ms;
	const char *source;
	uint32_t source_len;
	int16_t point_x;
	int16_t point_y;
	char text[BK_JS_PATH_MAX];
} js_msg_t;

typedef struct {
	uint32_t deadline;
	bool timed_out;
	bool error_valid;
	jerry_value_t error;
} js_halt_t;

typedef struct {
	bool used;
	uint32_t id;
	uint32_t due;
	uint32_t interval;
	bool repeat;
	jerry_value_t fn;
} js_timer_t;

typedef struct {
	bool used;
	bool dead;
	uint32_t id;
	uint32_t owner;
	uint32_t gen;
	uint32_t code;
	jerry_value_t fn;
} js_event_t;

static beken_thread_t s_task = NULL;
static beken_queue_t s_queue = NULL;
static beken_semaphore_t s_ready_sem = NULL;
static beken_semaphore_t s_done_sem = NULL;
static beken_semaphore_t s_exit_sem = NULL;

static volatile bool s_engine_up = false;
static volatile bool s_engine_broken = false;
static volatile bool s_running = false;
static volatile bool s_app_running = false;
static volatile bool s_app_exit = false;
static volatile bk_js_err_t s_job_result = BK_JS_ERR_INTERNAL;

static uint32_t s_heap_kb = CONFIG_BK_JERRYSCRIPT_HEAP_SIZE_KB;
static uint32_t s_default_timeout_ms = CONFIG_BK_JERRYSCRIPT_DEFAULT_TIMEOUT_MS;

/* Pointer position of the event being dispatched, only valid while a
 * JavaScript callback for that event is running in the JavaScript task. */
static bool s_event_touch_valid = false;
static int32_t s_event_touch_x = 0;
static int32_t s_event_touch_y = 0;

static char s_app_root[BK_JS_PATH_MAX] = CONFIG_BK_JERRYSCRIPT_APPS_ROOT;
static char s_default_app[BK_JS_ID_MAX] = "";
static char s_app_id[BK_JS_ID_MAX] = "";
static char s_app_name[BK_JS_ID_MAX] = "";
static char s_app_version[20] = "";
static char s_app_dir[BK_JS_PATH_MAX] = "";

static js_timer_t s_timers[BK_JS_TIMER_MAX];
static uint32_t s_timer_next_id = 1;

static js_event_t s_events[BK_JS_EVENT_MAX];
static uint32_t s_event_next_id = 1;

/* ------------------------------------------------------------------ */
/* Console                                                            */
/* ------------------------------------------------------------------ */

void bk_js_console_text(const char *text, uint32_t len, bool newline)
{
	char chunk[BK_JS_CONSOLE_CHUNK + 1];
	uint32_t offset = 0;

	if (len == 0) {
		if (newline) {
			BK_RAW_LOGI(BK_JS_TAG, "\n");
		}
		return;
	}

	while (offset < len) {
		uint32_t count = len - offset;

		if (count > BK_JS_CONSOLE_CHUNK) {
			count = BK_JS_CONSOLE_CHUNK;
		}

		os_memcpy(chunk, text + offset, count);
		chunk[count] = '\0';

		if (newline && (offset + count) >= len) {
			BK_RAW_LOGI(BK_JS_TAG, "%s\n", chunk);
		} else {
			BK_RAW_LOGI(BK_JS_TAG, "%s", chunk);
		}

		offset += count;
	}
}

static void js_console_string(jerry_value_t value, bool newline)
{
	char stack_buffer[BK_JS_CONSOLE_CHUNK + 32];
	jerry_char_t *buffer = (jerry_char_t *)stack_buffer;
	bool on_heap = false;
	jerry_size_t need;
	jerry_size_t size;
	jerry_size_t copied;

	need = jerry_string_size(value, JERRY_ENCODING_UTF8);
	if (need == 0) {
		bk_js_console_text("", 0, newline);
		return;
	}

	if (need >= (jerry_size_t)sizeof(stack_buffer)) {
		buffer = (jerry_char_t *)os_malloc((uint32_t)need);
		on_heap = (buffer != NULL);
		if (!on_heap) {
			buffer = (jerry_char_t *)stack_buffer;
		}
	}

	size = on_heap ? need : (jerry_size_t)(sizeof(stack_buffer) - 1);
	copied = jerry_string_to_buffer(value, JERRY_ENCODING_UTF8, buffer, size);

	bk_js_console_text((const char *)buffer, (uint32_t)copied, newline);

	if (on_heap) {
		os_free(buffer);
	}
}

void bk_js_console_value(jerry_value_t value, bool newline)
{
	jerry_value_t text = jerry_value_to_string(value);

	if (jerry_value_is_exception(text)) {
		jerry_value_free(text);
		bk_js_console_text("[exception]", 11, newline);
		return;
	}

	js_console_string(text, newline);
	jerry_value_free(text);
}

void bk_js_console_exception(jerry_value_t exception)
{
	jerry_value_t value = jerry_exception_value(exception, true);

	bk_js_console_value(value, true);
	jerry_value_free(value);
}

static jerry_value_t js_print_handler(const jerry_call_info_t *call_info_p,
                                      const jerry_value_t args_p[],
                                      const jerry_length_t args_count)
{
	(void)call_info_p;

	for (jerry_length_t i = 0; i < args_count; i++) {
		if (i > 0) {
			bk_js_console_text(" ", 1, false);
		}
		bk_js_console_value(args_p[i], i + 1 == args_count);
	}

	if (args_count == 0) {
		bk_js_console_text("", 0, true);
	}

	return jerry_undefined();
}

/* ------------------------------------------------------------------ */
/* Object helpers                                                     */
/* ------------------------------------------------------------------ */

static void js_object_set_str(jerry_value_t object, const char *key, const char *value)
{
	jerry_value_t str = jerry_string_sz(value);
	jerry_value_t result = jerry_object_set_sz(object, key, str);

	jerry_value_free(result);
	jerry_value_free(str);
}

static void js_object_set_fn(jerry_value_t object, const char *key, jerry_external_handler_t handler)
{
	jerry_value_t fn = jerry_function_external(handler);
	jerry_value_t result = jerry_object_set_sz(object, key, fn);

	jerry_value_free(result);
	jerry_value_free(fn);
}

/* ------------------------------------------------------------------ */
/* Timers                                                             */
/* ------------------------------------------------------------------ */

static uint32_t js_now(void)
{
	return (uint32_t)rtos_get_time();
}

static void js_timers_forget(void)
{
	os_memset(s_timers, 0, sizeof(s_timers));
}

uint32_t bk_js_timer_add(jerry_value_t fn, uint32_t ms, bool repeat)
{
	js_timer_t *slot = NULL;

	if (!jerry_value_is_function(fn)) {
		return 0;
	}

	for (uint32_t i = 0; i < BK_JS_TIMER_MAX; i++) {
		if (!s_timers[i].used) {
			slot = &s_timers[i];
			break;
		}
	}

	if (slot == NULL) {
		return 0;
	}

	if (ms == 0) {
		ms = 1;
	}

	slot->used = true;
	slot->id = s_timer_next_id++;
	slot->due = js_now() + ms;
	slot->interval = ms;
	slot->repeat = repeat;
	slot->fn = jerry_value_copy(fn);

	return slot->id;
}

void bk_js_timer_clear_id(uint32_t id)
{
	for (uint32_t i = 0; i < BK_JS_TIMER_MAX; i++) {
		if (s_timers[i].used && s_timers[i].id == id) {
			s_timers[i].used = false;
			jerry_value_free(s_timers[i].fn);
			s_timers[i].fn = jerry_undefined();
			return;
		}
	}
}

void bk_js_timer_clear_all(void)
{
	for (uint32_t i = 0; i < BK_JS_TIMER_MAX; i++) {
		if (s_timers[i].used) {
			s_timers[i].used = false;
			jerry_value_free(s_timers[i].fn);
			s_timers[i].fn = jerry_undefined();
		}
	}
}

static uint32_t js_timer_wait(void)
{
	uint32_t now = js_now();
	uint32_t wait = BEKEN_WAIT_FOREVER;

	for (uint32_t i = 0; i < BK_JS_TIMER_MAX; i++) {
		uint32_t delta;

		if (!s_timers[i].used) {
			continue;
		}

		if ((int32_t)(now - s_timers[i].due) >= 0) {
			return 0;
		}

		delta = s_timers[i].due - now;
		if (delta < wait) {
			wait = delta;
		}
	}

	return wait;
}

static void js_timers_run(void)
{
	uint32_t now = js_now();

	for (uint32_t i = 0; i < BK_JS_TIMER_MAX; i++) {
		js_timer_t *timer = &s_timers[i];
		jerry_value_t result;
		uint32_t id;

		if (!timer->used || (int32_t)(now - timer->due) < 0) {
			continue;
		}

		id = timer->id;
		result = bk_js_call_guarded(timer->fn, NULL, 0);

		if (jerry_value_is_exception(result)) {
			bk_js_console_exception(result);
		} else {
			jerry_value_free(result);
		}

		if (!s_engine_up) {
			return;
		}

		/* The callback may have cleared this timer or started a new one. */
		if (!timer->used || timer->id != id) {
			continue;
		}

		if (timer->repeat) {
			timer->due += timer->interval;
			if ((int32_t)(js_now() - timer->due) >= 0) {
				timer->due = js_now() + timer->interval;
			}
		} else {
			timer->used = false;
			jerry_value_free(timer->fn);
			timer->fn = jerry_undefined();
		}
	}
}

/* ------------------------------------------------------------------ */
/* Event slots                                                        */
/* ------------------------------------------------------------------ */

static void js_events_forget(void)
{
	os_memset(s_events, 0, sizeof(s_events));
}

/*
 * Events are owned by a (handle index, generation) pair instead of the raw
 * lv_obj_t pointer.  LVGL recycles addresses, so a delete message that is
 * still queued while the script keeps allocating can otherwise match a brand
 * new object that landed on the same address and silently drop its callback.
 */
uint32_t bk_js_event_add(uint32_t owner, uint32_t gen, uint32_t code, jerry_value_t fn)
{
	js_event_t *slot = NULL;

	if (!jerry_value_is_function(fn)) {
		return 0;
	}

	for (uint32_t i = 0; i < BK_JS_EVENT_MAX; i++) {
		if (!s_events[i].used) {
			slot = &s_events[i];
			break;
		}
	}

	if (slot == NULL) {
		return 0;
	}

	slot->used = true;
	slot->dead = false;
	slot->id = s_event_next_id++;
	slot->owner = owner;
	slot->gen = gen;
	slot->code = code;
	slot->fn = jerry_value_copy(fn);

	return slot->id;
}

static void js_event_remove_at(js_event_t *slot)
{
	slot->used = false;
	slot->dead = false;
	jerry_value_free(slot->fn);
	slot->fn = jerry_undefined();
	slot->owner = 0;
	slot->gen = 0;
}

/*
 * Called from the LVGL thread when an object is deleted.  Freeing the jerry
 * value here would race with the JavaScript task, so only flag the slot; the
 * JavaScript task reclaims it on its next loop iteration.
 */
void bk_js_event_mark_dead(uint32_t owner, uint32_t gen)
{
	for (uint32_t i = 0; i < BK_JS_EVENT_MAX; i++) {
		if (s_events[i].used && s_events[i].owner == owner && s_events[i].gen == gen) {
			s_events[i].dead = true;
		}
	}
}

void bk_js_event_sweep(void)
{
	for (uint32_t i = 0; i < BK_JS_EVENT_MAX; i++) {
		if (s_events[i].used && s_events[i].dead) {
			js_event_remove_at(&s_events[i]);
		}
	}
}

void bk_js_event_remove(uint32_t owner, uint32_t gen)
{
	for (uint32_t i = 0; i < BK_JS_EVENT_MAX; i++) {
		if (s_events[i].used && s_events[i].owner == owner && s_events[i].gen == gen) {
			js_event_remove_at(&s_events[i]);
		}
	}
}

void bk_js_event_clear(void)
{
	for (uint32_t i = 0; i < BK_JS_EVENT_MAX; i++) {
		if (s_events[i].used) {
			js_event_remove_at(&s_events[i]);
		}
	}
}

void bk_js_event_dispatch(uint32_t slot_id, uint32_t code, bool deleting,
                          int32_t x, int32_t y, bool has_point)
{
	js_event_t *slot = NULL;

	for (uint32_t i = 0; i < BK_JS_EVENT_MAX; i++) {
		if (s_events[i].used && !s_events[i].dead && s_events[i].id == slot_id) {
			slot = &s_events[i];
			break;
		}
	}

	if (slot == NULL) {
		return;
	}

	jerry_value_t event = jerry_object();
	jerry_value_t code_value = jerry_number((double)code);
	jerry_value_t set_result = jerry_object_set_sz(event, "code", code_value);
	jerry_value_t args[1];
	jerry_value_t result;

	jerry_value_free(set_result);
	jerry_value_free(code_value);

	args[0] = event;
	s_event_touch_valid = has_point;
	s_event_touch_x = x;
	s_event_touch_y = y;
	result = bk_js_call_guarded(slot->fn, args, 1);
	s_event_touch_valid = false;

	if (jerry_value_is_exception(result)) {
		bk_js_console_exception(result);
	} else {
		jerry_value_free(result);
	}

	jerry_value_free(event);

	if (deleting) {
		bk_js_event_remove(slot->owner, slot->gen);
	}
}

void bk_js_post_event(uint32_t slot_id, uint32_t code, bool deleting)
{
	bk_js_post_event_touch(slot_id, code, deleting, 0, 0, false);
}

void bk_js_post_event_touch(uint32_t slot_id, uint32_t code, bool deleting,
                            int32_t x, int32_t y, bool has_point)
{
	js_msg_t msg;

	os_memset(&msg, 0, sizeof(msg));
	msg.type = JS_MSG_EVENT;
	msg.id = slot_id;
	msg.code = code;
	msg.deleting = deleting ? 1 : 0;
	msg.has_point = has_point ? 1 : 0;
	msg.point_x = (int16_t)x;
	msg.point_y = (int16_t)y;

	if (s_queue != NULL) {
		rtos_push_to_queue(&s_queue, &msg, 0);
	}
}

bool bk_js_event_touch_get(int32_t *x, int32_t *y)
{
	if (!s_event_touch_valid) {
		return false;
	}

	if (x != NULL) {
		*x = s_event_touch_x;
	}
	if (y != NULL) {
		*y = s_event_touch_y;
	}

	return true;
}

/* ------------------------------------------------------------------ */
/* Engine lifecycle                                                   */
/* ------------------------------------------------------------------ */

static jerry_value_t js_app_exit_handler(const jerry_call_info_t *call_info_p,
                                         const jerry_value_t args_p[],
                                         const jerry_length_t args_count)
{
	(void)call_info_p;
	(void)args_p;
	(void)args_count;

	bk_js_runtime_request_app_exit();

	return jerry_undefined();
}

static void js_bind_app_object(jerry_value_t realm)
{
	jerry_value_t app = jerry_object();
	jerry_value_t result;

	js_object_set_str(app, "id", "");
	js_object_set_str(app, "name", "");
	js_object_set_str(app, "version", "");
	js_object_set_str(app, "dir", "");
	js_object_set_fn(app, "log", js_print_handler);
	js_object_set_fn(app, "exit", js_app_exit_handler);

	result = jerry_object_set_sz(realm, "app", app);
	jerry_value_free(result);
	jerry_value_free(app);
}

static jerry_value_t js_timer_set_timeout(const jerry_call_info_t *call_info_p,
                                          const jerry_value_t args_p[],
                                          const jerry_length_t args_count,
                                          bool repeat)
{
	uint32_t id;

	(void)call_info_p;

	if (args_count < 2 || !jerry_value_is_function(args_p[0])) {
		return jerry_throw_sz(JERRY_ERROR_TYPE, "timer expects (function, milliseconds)");
	}

	id = bk_js_timer_add(args_p[0], (uint32_t)jerry_value_as_integer(args_p[1]), repeat);
	if (id == 0) {
		return jerry_throw_sz(JERRY_ERROR_COMMON, "no free timer");
	}

	return jerry_number((double)id);
}

static jerry_value_t js_timer_set_timeout_handler(const jerry_call_info_t *call_info_p,
                                                  const jerry_value_t args_p[],
                                                  const jerry_length_t args_count)
{
	return js_timer_set_timeout(call_info_p, args_p, args_count, false);
}

static jerry_value_t js_timer_set_interval_handler(const jerry_call_info_t *call_info_p,
                                                   const jerry_value_t args_p[],
                                                   const jerry_length_t args_count)
{
	return js_timer_set_timeout(call_info_p, args_p, args_count, true);
}

static jerry_value_t js_timer_clear_handler(const jerry_call_info_t *call_info_p,
                                            const jerry_value_t args_p[],
                                            const jerry_length_t args_count)
{
	(void)call_info_p;

	if (args_count >= 1 && jerry_value_is_number(args_p[0])) {
		bk_js_timer_clear_id((uint32_t)jerry_value_as_integer(args_p[0]));
	}

	return jerry_undefined();
}

static jerry_value_t js_timer_now_handler(const jerry_call_info_t *call_info_p,
                                          const jerry_value_t args_p[],
                                          const jerry_length_t args_count)
{
	(void)call_info_p;
	(void)args_p;
	(void)args_count;

	return jerry_number((double)js_now());
}

static void js_bind_globals(void)
{
	jerry_value_t realm = jerry_current_realm();
	jerry_value_t console;
	jerry_value_t timer;
	jerry_value_t fn;
	jerry_value_t result;

	fn = jerry_function_external(js_print_handler);
	result = jerry_object_set_sz(realm, "print", fn);
	jerry_value_free(result);
	jerry_value_free(fn);

	console = jerry_object();
	js_object_set_fn(console, "log", js_print_handler);
	js_object_set_fn(console, "warn", js_print_handler);
	js_object_set_fn(console, "error", js_print_handler);
	js_object_set_fn(console, "info", js_print_handler);
	result = jerry_object_set_sz(realm, "console", console);
	jerry_value_free(result);
	jerry_value_free(console);

	timer = jerry_object();
	js_object_set_fn(timer, "setTimeout", js_timer_set_timeout_handler);
	js_object_set_fn(timer, "setInterval", js_timer_set_interval_handler);
	js_object_set_fn(timer, "clear", js_timer_clear_handler);
	js_object_set_fn(timer, "now", js_timer_now_handler);
	result = jerry_object_set_sz(realm, "timer", timer);
	jerry_value_free(result);
	jerry_value_free(timer);

	js_bind_app_object(realm);

	bk_js_fs_bind(realm);

#if CONFIG_BK_JERRYSCRIPT_UI
	bk_js_ui_bind(realm);
#endif

	jerry_value_free(realm);
}

static bk_js_err_t js_engine_start(void)
{
	bk_js_port_set_heap_size(s_heap_kb);

	if (setjmp(bk_js_fatal_jmp) != 0) {
		bk_js_guard_armed = false;
		BK_LOGE(BK_JS_TAG, "engine init failed, fatal code %d", bk_js_port_fatal_code());
		jerry_port_context_free();
		return BK_JS_ERR_NO_MEM;
	}

	bk_js_guard_armed = true;

#if CONFIG_BK_JERRYSCRIPT_UI
	bk_js_ui_shutdown();
#endif

	jerry_init(JERRY_INIT_EMPTY);
	js_bind_globals();

	bk_js_guard_armed = false;
	s_engine_up = true;
	s_engine_broken = false;

	return BK_JS_OK;
}

static void js_engine_stop(void)
{
	if (s_engine_up) {
		jerry_halt_handler(0, NULL, NULL);
		jerry_cleanup();
		s_engine_up = false;
	} else {
		jerry_port_context_free();
	}
}

static void js_engine_abort(void)
{
	jerry_port_context_free();

	s_engine_up = false;
	s_engine_broken = true;
	s_app_running = false;
	s_app_exit = false;

	js_timers_forget();
	js_events_forget();
	bk_js_fs_forget_modules();

	s_app_id[0] = '\0';
	s_app_name[0] = '\0';
	s_app_version[0] = '\0';
	s_app_dir[0] = '\0';
}

/* ------------------------------------------------------------------ */
/* Script execution                                                   */
/* ------------------------------------------------------------------ */

static jerry_value_t js_halt_cb(void *user_p)
{
	js_halt_t *halt = (js_halt_t *)user_p;

	if (!halt->error_valid && (int32_t)(rtos_get_time() - halt->deadline) >= 0) {
		halt->timed_out = true;
		halt->error = jerry_throw_sz(JERRY_ERROR_COMMON, "script execution timed out");
		halt->error_valid = true;
		return halt->error;
	}

	if (halt->error_valid) {
		return halt->error;
	}

	return jerry_undefined();
}

jerry_value_t bk_js_call_guarded(jerry_value_t fn, const jerry_value_t *args, uint32_t count)
{
	js_halt_t halt;
	jerry_value_t result;

	if (!s_engine_up) {
		return jerry_undefined();
	}

	if (setjmp(bk_js_fatal_jmp) != 0) {
		bk_js_guard_armed = false;
		BK_LOGE(BK_JS_TAG, "engine fatal error %d", bk_js_port_fatal_code());
#if CONFIG_BK_JERRYSCRIPT_UI
		/* The longjmp skipped the runtime loop's unlock; give LVGL back. */
		bk_js_ui_reset_lock();
#endif
		js_engine_abort();
		return jerry_undefined();
	}

	bk_js_guard_armed = true;

	/* Callbacks (events, timers) get the same timeout as a script. */
	halt.deadline = js_now() + s_default_timeout_ms;
	halt.timed_out = false;
	halt.error_valid = false;
	halt.error = jerry_undefined();

	jerry_halt_handler(BK_JS_HALT_INTERVAL, js_halt_cb, &halt);

	result = jerry_call(fn, jerry_undefined(), args, (jerry_size_t)count);

	jerry_halt_handler(0, NULL, NULL);
	bk_js_guard_armed = false;

	return result;
}

static bk_js_err_t js_run_source(const char *name, const char *source, uint32_t source_len,
                                 uint32_t timeout_ms, bool print_result, jerry_value_t *result_out)
{
	js_halt_t halt;
	bk_js_err_t result = BK_JS_OK;
	jerry_value_t program;
	jerry_parse_options_t options;

	if (result_out != NULL) {
		*result_out = jerry_undefined();
	}

	if (!s_engine_up) {
		bk_js_err_t err = js_engine_start();

		if (err != BK_JS_OK) {
			return err;
		}
	}

	if (setjmp(bk_js_fatal_jmp) != 0) {
		bk_js_guard_armed = false;
		BK_LOGE(BK_JS_TAG, "engine fatal error %d, heap released", bk_js_port_fatal_code());
#if CONFIG_BK_JERRYSCRIPT_UI
		/* The longjmp skipped the runtime loop's unlock; give LVGL back. */
		bk_js_ui_reset_lock();
#endif
		js_engine_abort();
		return BK_JS_ERR_INTERNAL;
	}

	bk_js_guard_armed = true;

	halt.deadline = js_now() + timeout_ms;
	halt.timed_out = false;
	halt.error_valid = false;
	halt.error = jerry_undefined();

	jerry_halt_handler(BK_JS_HALT_INTERVAL, js_halt_cb, &halt);

	os_memset(&options, 0, sizeof(options));
	options.options = JERRY_PARSE_HAS_SOURCE_NAME;
	options.source_name = jerry_string_sz(name != NULL ? name : "script.js");

	program = jerry_parse((const jerry_char_t *)source, source_len, &options);
	jerry_value_free(options.source_name);

	if (jerry_value_is_exception(program)) {
		bk_js_console_exception(program);
		result = BK_JS_ERR_PARSE;
	} else {
		jerry_value_t run_result = jerry_run(program);

		if (jerry_value_is_exception(run_result)) {
			halt.timed_out = halt.error_valid;
			bk_js_console_exception(run_result);
			result = halt.timed_out ? BK_JS_ERR_TIMEOUT : BK_JS_ERR_EXEC;
		} else {
			if (print_result && !jerry_value_is_undefined(run_result)) {
				bk_js_console_value(run_result, true);
			}

			if (result_out != NULL) {
				*result_out = run_result;
			} else {
				jerry_value_free(run_result);
			}
		}

		jerry_value_free(program);
	}

	jerry_halt_handler(0, NULL, NULL);
	bk_js_guard_armed = false;

	if (result != BK_JS_OK && result_out != NULL) {
		*result_out = jerry_undefined();
	}

	return result;
}

/* ------------------------------------------------------------------ */
/* Application lifecycle                                              */
/* ------------------------------------------------------------------ */

void bk_js_runtime_request_app_exit(void)
{
	s_app_exit = true;
}

static void js_set_app_metadata(const char *id, const char *name, const char *version, const char *dir)
{
	jerry_value_t realm;
	jerry_value_t app;

	os_strncpy(s_app_id, id != NULL ? id : "", sizeof(s_app_id));
	s_app_id[sizeof(s_app_id) - 1] = '\0';
	os_strncpy(s_app_name, name != NULL ? name : "", sizeof(s_app_name));
	s_app_name[sizeof(s_app_name) - 1] = '\0';
	os_strncpy(s_app_version, version != NULL ? version : "", sizeof(s_app_version));
	s_app_version[sizeof(s_app_version) - 1] = '\0';
	os_strncpy(s_app_dir, dir != NULL ? dir : "", sizeof(s_app_dir));
	s_app_dir[sizeof(s_app_dir) - 1] = '\0';

	bk_js_fs_set_dir(s_app_dir);

	realm = jerry_current_realm();
	app = jerry_object_get_sz(realm, "app");

	if (jerry_value_is_object(app)) {
		js_object_set_str(app, "id", s_app_id);
		js_object_set_str(app, "name", s_app_name);
		js_object_set_str(app, "version", s_app_version);
		js_object_set_str(app, "dir", s_app_dir);
	}

	jerry_value_free(app);
	jerry_value_free(realm);
}

static void js_call_on_exit(void)
{
	jerry_value_t realm;
	jerry_value_t callback;
	jerry_value_t result;

	if (!s_engine_up) {
		return;
	}

	realm = jerry_current_realm();
	callback = jerry_object_get_sz(realm, "onExit");

	if (jerry_value_is_function(callback)) {
		result = bk_js_call_guarded(callback, NULL, 0);
		if (jerry_value_is_exception(result)) {
			bk_js_console_exception(result);
		} else {
			jerry_value_free(result);
		}
	}

	jerry_value_free(callback);
	jerry_value_free(realm);
}

static void js_teardown(void)
{
	js_call_on_exit();

#if CONFIG_BK_JERRYSCRIPT_UI
	bk_js_ui_shutdown();
#endif

	bk_js_timer_clear_all();
	bk_js_event_clear();
	bk_js_fs_clear_modules();

	s_app_running = false;
	s_app_exit = false;

	/* Peak heap tells whether BK_JERRYSCRIPT_HEAP_SIZE_KB is sized right. */
	{
		bk_js_heap_stat_t stat;

		if (bk_js_heap_get(&stat) == BK_JS_OK && stat.heap_size != 0) {
			BK_LOGI(BK_JS_TAG, "heap peak %u/%u KB (%u%%)",
			        (unsigned)(stat.peak_allocated / 1024u),
			        (unsigned)(stat.heap_size / 1024u),
			        (unsigned)(((uint64_t)stat.peak_allocated * 100u) / stat.heap_size));
		}
	}

	js_engine_stop();

	s_app_id[0] = '\0';
	s_app_name[0] = '\0';
	s_app_version[0] = '\0';
	s_app_dir[0] = '\0';
	bk_js_fs_set_dir("");
}

static void js_derive_id(const char *dir, char *id, uint32_t size)
{
	const char *base = dir;
	const char *p;

	for (p = dir; *p != '\0'; p++) {
		if (*p == '/') {
			base = p + 1;
		}
	}

	os_strncpy(id, base, size);
	id[size - 1] = '\0';
}

static bk_js_err_t js_app_start_impl(const char *requested_id)
{
	char dir[BK_JS_PATH_MAX] = "";
	char manifest_path[BK_JS_PATH_MAX];
	char entry_path[BK_JS_PATH_MAX] = "";
	char *manifest = NULL;
	char *source = NULL;
	uint32_t source_len = 0;
	uint32_t manifest_len = 0;
	bk_js_manifest_t manifest_info;
	bool builtin = false;
	bk_js_err_t err;

	if (s_app_running) {
		return BK_JS_ERR_BUSY;
	}

	if (!s_engine_up) {
		err = js_engine_start();
		if (err != BK_JS_OK) {
			return err;
		}
	}

	os_memset(&manifest_info, 0, sizeof(manifest_info));

	if (requested_id != NULL && requested_id[0] != '\0') {
		if (requested_id[0] == '/') {
			snprintf(dir, sizeof(dir), "%s", requested_id);
		} else {
			snprintf(dir, sizeof(dir), "%s/%s", s_app_root, requested_id);
		}
	} else {
		bk_js_manifest_t apps[8];
		uint32_t count = bk_js_scan_apps(s_app_root, apps, 8);

		if (count > 0) {
			manifest_info = apps[0];
			if (apps[0].id[0] != '\0') {
				snprintf(dir, sizeof(dir), "%s/%s", s_app_root, apps[0].id);
			}
		} else if (s_default_app[0] != '\0') {
			snprintf(dir, sizeof(dir), "%s/%s", s_app_root, s_default_app);
		}
	}

	if (dir[0] != '\0') {
		snprintf(manifest_path, sizeof(manifest_path), "%s/manifest.json", dir);
		manifest = bk_js_fs_read(manifest_path, &manifest_len);

		if (manifest != NULL) {
			manifest[manifest_len] = '\0';
			bk_js_manifest_parse(manifest, &manifest_info);
			os_free(manifest);
		}

		snprintf(entry_path, sizeof(entry_path), "%s/%s", dir,
		         manifest_info.entry[0] != '\0' ? manifest_info.entry : "main.js");
		source = bk_js_fs_read(entry_path, &source_len);
	}

	if (source == NULL) {
#if CONFIG_BK_JERRYSCRIPT_BUILTIN_DEMO
		BK_LOGW(BK_JS_TAG, "no application found under %s, using the built-in demo", s_app_root);
		builtin = true;
		os_strncpy(manifest_info.id, "builtin", sizeof(manifest_info.id));
		os_strncpy(manifest_info.name, bk_js_builtin_app_name(), sizeof(manifest_info.name));
		os_strncpy(manifest_info.version, "1.0.0", sizeof(manifest_info.version));
		dir[0] = '\0';
		source = (char *)bk_js_builtin_app_source();
		source_len = (uint32_t)os_strlen(source);
#else
		BK_LOGE(BK_JS_TAG, "application %s not found (entry '%s')\n",
		        dir[0] != '\0' ? dir : s_app_root,
		        entry_path[0] != '\0' ? entry_path : "main.js");
		return BK_JS_ERR_PARSE;
#endif
	}

	if (manifest_info.id[0] == '\0') {
		js_derive_id(dir, manifest_info.id, sizeof(manifest_info.id));
	}

	js_set_app_metadata(manifest_info.id, manifest_info.name, manifest_info.version, dir);

	s_app_running = true;
	s_app_exit = false;

	err = js_run_source(entry_path[0] != '\0' ? entry_path : "main.js",
	                    source, source_len, s_default_timeout_ms, false, NULL);

	if (!builtin) {
		os_free(source);
	}

	if (err != BK_JS_OK) {
		BK_LOGE(BK_JS_TAG, "application %s failed: %s\n", manifest_info.id, bk_js_err_to_str(err));
		js_teardown();
		return err;
	}

	BK_LOGI(BK_JS_TAG, "application started: %s (%s)\n", s_app_id, s_app_name);

	return BK_JS_OK;
}

static bk_js_err_t js_app_stop_impl(void)
{
	if (!s_app_running && !s_engine_up) {
		return BK_JS_OK;
	}

	js_teardown();

	return BK_JS_OK;
}

/* ------------------------------------------------------------------ */
/* Task loop                                                          */
/* ------------------------------------------------------------------ */

static bool js_handle_msg(const js_msg_t *msg)
{
	bool blocking = false;

	switch (msg->type) {
	case JS_MSG_EVAL:
		s_running = true;
		s_job_result = js_run_source("eval.js", msg->source, msg->source_len,
		                             msg->timeout_ms, true, NULL);
		blocking = true;
		break;

	case JS_MSG_EVAL_FILE: {
		uint32_t len = 0;
		char *source = bk_js_fs_read(msg->text, &len);

		s_running = true;

		if (source == NULL) {
			s_job_result = BK_JS_ERR_PARSE;
		} else {
			s_job_result = js_run_source(msg->text, source, len,
			                             msg->timeout_ms, true, NULL);
			os_free(source);
		}

		blocking = true;
		break;
	}

	case JS_MSG_APP_START:
		s_job_result = js_app_start_impl(msg->text);
		blocking = true;
		break;

	case JS_MSG_APP_STOP:
		s_job_result = js_app_stop_impl();
		blocking = true;
		break;

	case JS_MSG_EVENT:
		bk_js_event_dispatch(msg->id, msg->code, msg->deleting != 0,
		                     msg->point_x, msg->point_y, msg->has_point != 0);
		break;

	default:
		break;
	}

	return blocking;
}

static void js_task_entry(beken_thread_arg_t arg)
{
	(void)arg;

	if (js_engine_start() != BK_JS_OK) {
		BK_LOGE(BK_JS_TAG, "engine not ready, will retry on demand");
	}

	rtos_set_semaphore(&s_ready_sem);

	for (;;) {
		js_msg_t msg;
		uint32_t wait = js_timer_wait();
		bool blocking = false;
		bool has_msg = false;
		bool locked = false;

		if (rtos_pop_from_queue(&s_queue, &msg, wait) != kNoErr) {
			os_memset(&msg, 0, sizeof(msg));
		} else if (msg.type == JS_MSG_EXIT) {
			break;
		} else {
			has_msg = true;
		}

		/*
		 * Everything that may touch LVGL runs here, and only here, so hold the
		 * display mutex for the whole callback.  Without it a timer callback can
		 * invalidate a widget while the LVGL task is rendering, which trips
		 * lv_inv_area()'s rendering_in_progress assertion.
		 */
		locked = bk_js_ui_lock();

		if (has_msg) {
			blocking = js_handle_msg(&msg);

			if (blocking) {
				s_running = false;
				rtos_set_semaphore(&s_done_sem);
			}
		}

		js_timers_run();

		/* Reclaim event slots of objects deleted during the callbacks above. */
		if (s_engine_up) {
			bk_js_event_sweep();
		}

		if (s_app_exit && s_engine_up) {
			js_teardown();
		}

		if (locked) {
			bk_js_ui_unlock();
		}
	}

	{
		bool locked = bk_js_ui_lock();

		js_teardown();

		if (locked) {
			bk_js_ui_unlock();
		}
	}

	rtos_set_semaphore(&s_exit_sem);
	rtos_delete_thread(NULL);
}

/* ------------------------------------------------------------------ */
/* Public API                                                         */
/* ------------------------------------------------------------------ */

static void js_runtime_release_handles(void)
{
	if (s_queue != NULL) {
		rtos_deinit_queue(&s_queue);
		s_queue = NULL;
	}

	if (s_ready_sem != NULL) {
		rtos_deinit_semaphore(&s_ready_sem);
		s_ready_sem = NULL;
	}

	if (s_done_sem != NULL) {
		rtos_deinit_semaphore(&s_done_sem);
		s_done_sem = NULL;
	}

	if (s_exit_sem != NULL) {
		rtos_deinit_semaphore(&s_exit_sem);
		s_exit_sem = NULL;
	}
}

static bk_js_err_t js_runtime_create(const bk_js_runtime_config_t *config)
{
	uint32_t heap_bytes = (uint32_t)CONFIG_BK_JERRYSCRIPT_HEAP_SIZE_KB * 1024u;
	uint32_t timeout_ms = CONFIG_BK_JERRYSCRIPT_DEFAULT_TIMEOUT_MS;
	js_msg_t msg;

	if (s_task != NULL) {
		return s_engine_up ? BK_JS_OK : BK_JS_ERR_NO_MEM;
	}

	s_engine_broken = false;

	if (config != NULL) {
		if (config->heap_size != 0) {
			heap_bytes = config->heap_size;
		}
		if (config->timeout_ms != 0) {
			timeout_ms = config->timeout_ms;
		}
		if (config->apps_root != NULL && config->apps_root[0] != '\0') {
			os_strncpy(s_app_root, config->apps_root, sizeof(s_app_root));
			s_app_root[sizeof(s_app_root) - 1] = '\0';
		}
		if (config->default_app != NULL) {
			os_strncpy(s_default_app, config->default_app, sizeof(s_default_app));
			s_default_app[sizeof(s_default_app) - 1] = '\0';
		}
	}

	if (heap_bytes < (64u * 1024u) || heap_bytes > (512u * 1024u)) {
		return BK_JS_ERR_INVALID_ARG;
	}

	s_heap_kb = heap_bytes / 1024u;
	s_default_timeout_ms = timeout_ms;

	bk_js_fs_set_root(s_app_root);

#if CONFIG_BK_JERRYSCRIPT_FS_AUTO_MOUNT
	/* Mount the application file system before the engine starts.  A missing or
	 * unformatted image is not fatal: scripts compiled into the firmware
	 * (bk_js_eval) still work, only applications loaded from the file system
	 * become unavailable. */
	if (bk_js_fs_mount() != 0) {
		BK_LOGW(BK_JS_TAG, "file system not mounted on %s, fs applications unavailable\n",
		        s_app_root);
	}
#endif

	if (rtos_init_semaphore(&s_ready_sem, 1) != kNoErr) {
		goto fail;
	}
	if (rtos_init_semaphore(&s_done_sem, 1) != kNoErr) {
		goto fail;
	}
	if (rtos_init_semaphore(&s_exit_sem, 1) != kNoErr) {
		goto fail;
	}
	if (rtos_init_queue(&s_queue, "js_queue", sizeof(js_msg_t), BK_JS_QUEUE_LEN) != kNoErr) {
		goto fail;
	}

	if (rtos_create_thread(&s_task, BEKEN_DEFAULT_WORKER_PRIORITY, "js_task", js_task_entry,
	                       CONFIG_BK_JERRYSCRIPT_TASK_STACK_SIZE, NULL) != kNoErr) {
		s_task = NULL;
		goto fail;
	}

	rtos_get_semaphore(&s_ready_sem, BEKEN_WAIT_FOREVER);

	if (!s_engine_up) {
		BK_LOGE(BK_JS_TAG, "engine init failed\n");

		/* The task is still running (it retries the engine on demand), so stop
		 * it and release everything instead of leaving a half-initialised
		 * runtime that can never be initialised again. */
		os_memset(&msg, 0, sizeof(msg));
		msg.type = JS_MSG_EXIT;
		if (rtos_push_to_queue(&s_queue, &msg, BEKEN_WAIT_FOREVER) == kNoErr) {
			rtos_get_semaphore(&s_exit_sem, BEKEN_WAIT_FOREVER);
		}

		s_task = NULL;
		s_engine_up = false;
		js_runtime_release_handles();

#if CONFIG_BK_JERRYSCRIPT_FS_AUTO_MOUNT
		bk_js_fs_unmount();
#endif

		return BK_JS_ERR_NO_MEM;
	}

	if (config != NULL && config->auto_start) {
		return bk_js_app_start(config->default_app);
	}

	return BK_JS_OK;

fail:
	js_runtime_release_handles();

#if CONFIG_BK_JERRYSCRIPT_FS_AUTO_MOUNT
	bk_js_fs_unmount();
#endif

	return BK_JS_ERR_INTERNAL;
}

bk_js_err_t bk_js_runtime_init(const bk_js_runtime_config_t *config)
{
	return js_runtime_create(config);
}

bk_js_err_t bk_js_runtime_deinit(void)
{
	return bk_js_deinit();
}

bk_js_err_t bk_js_init(const bk_js_config_t *config)
{
	bk_js_runtime_config_t runtime_config;

	os_memset(&runtime_config, 0, sizeof(runtime_config));

	if (config != NULL) {
		runtime_config.heap_size = config->heap_size;
		runtime_config.timeout_ms = config->timeout_ms;
	}

	return js_runtime_create(&runtime_config);
}

bk_js_err_t bk_js_deinit(void)
{
	js_msg_t msg;

	if (s_task == NULL) {
		return BK_JS_OK;
	}

	if (s_app_running) {
		bk_js_app_stop();
	}

	if (s_running) {
		return BK_JS_ERR_BUSY;
	}

	os_memset(&msg, 0, sizeof(msg));
	msg.type = JS_MSG_EXIT;

	if (rtos_push_to_queue(&s_queue, &msg, BEKEN_WAIT_FOREVER) != kNoErr) {
		return BK_JS_ERR_INTERNAL;
	}

	rtos_get_semaphore(&s_exit_sem, BEKEN_WAIT_FOREVER);

	s_task = NULL;
	s_engine_up = false;

	js_runtime_release_handles();

#if CONFIG_BK_JERRYSCRIPT_FS_AUTO_MOUNT
	bk_js_fs_unmount();
#endif

	return BK_JS_OK;
}

bool bk_js_is_ready(void)
{
	return s_engine_up && !s_engine_broken;
}

bk_js_err_t bk_js_eval(const char *name, const char *source, uint32_t source_len, uint32_t timeout_ms)
{
	js_msg_t msg;
	bk_js_err_t err;

	(void)name;

	if (source == NULL) {
		return BK_JS_ERR_INVALID_ARG;
	}

	if (s_running || s_app_running) {
		return BK_JS_ERR_BUSY;
	}

	err = bk_js_init(NULL);
	if (err != BK_JS_OK && err != BK_JS_ERR_NO_MEM) {
		return err;
	}

	if (source_len == 0) {
		source_len = (uint32_t)os_strlen(source);
	}

	os_memset(&msg, 0, sizeof(msg));
	msg.type = JS_MSG_EVAL;
	msg.source = source;
	msg.source_len = source_len;
	msg.timeout_ms = (timeout_ms != 0) ? timeout_ms : s_default_timeout_ms;

	s_running = true;
	s_job_result = BK_JS_ERR_INTERNAL;

	if (rtos_push_to_queue(&s_queue, &msg, BEKEN_WAIT_FOREVER) != kNoErr) {
		s_running = false;
		return BK_JS_ERR_INTERNAL;
	}

	rtos_get_semaphore(&s_done_sem, BEKEN_WAIT_FOREVER);
	s_running = false;

	return s_job_result;
}

bk_js_err_t bk_js_eval_file(const char *path, uint32_t timeout_ms)
{
	js_msg_t msg;
	bk_js_err_t err;

	if (path == NULL || path[0] == '\0') {
		return BK_JS_ERR_INVALID_ARG;
	}

	if (s_running || s_app_running) {
		return BK_JS_ERR_BUSY;
	}

	err = bk_js_init(NULL);
	if (err != BK_JS_OK && err != BK_JS_ERR_NO_MEM) {
		return err;
	}

	os_memset(&msg, 0, sizeof(msg));
	msg.type = JS_MSG_EVAL_FILE;
	msg.timeout_ms = (timeout_ms != 0) ? timeout_ms : s_default_timeout_ms;
	os_strncpy(msg.text, path, sizeof(msg.text));
	msg.text[sizeof(msg.text) - 1] = '\0';

	s_running = true;
	s_job_result = BK_JS_ERR_INTERNAL;

	if (rtos_push_to_queue(&s_queue, &msg, BEKEN_WAIT_FOREVER) != kNoErr) {
		s_running = false;
		return BK_JS_ERR_INTERNAL;
	}

	rtos_get_semaphore(&s_done_sem, BEKEN_WAIT_FOREVER);
	s_running = false;

	return s_job_result;
}

bk_js_err_t bk_js_app_start(const char *app_id)
{
	js_msg_t msg;
	bk_js_err_t err;

	if (s_running) {
		return BK_JS_ERR_BUSY;
	}

	err = bk_js_init(NULL);
	if (err != BK_JS_OK && err != BK_JS_ERR_NO_MEM) {
		return err;
	}

	os_memset(&msg, 0, sizeof(msg));
	msg.type = JS_MSG_APP_START;

	if (app_id != NULL) {
		os_strncpy(msg.text, app_id, sizeof(msg.text));
		msg.text[sizeof(msg.text) - 1] = '\0';
	}

	s_running = true;
	s_job_result = BK_JS_ERR_INTERNAL;

	if (rtos_push_to_queue(&s_queue, &msg, BEKEN_WAIT_FOREVER) != kNoErr) {
		s_running = false;
		return BK_JS_ERR_INTERNAL;
	}

	rtos_get_semaphore(&s_done_sem, BEKEN_WAIT_FOREVER);
	s_running = false;

	return s_job_result;
}

bk_js_err_t bk_js_app_stop(void)
{
	js_msg_t msg;

	if (!s_app_running) {
		return BK_JS_OK;
	}

	os_memset(&msg, 0, sizeof(msg));
	msg.type = JS_MSG_APP_STOP;

	s_running = true;
	s_job_result = BK_JS_ERR_INTERNAL;

	if (rtos_push_to_queue(&s_queue, &msg, BEKEN_WAIT_FOREVER) != kNoErr) {
		s_running = false;
		return BK_JS_ERR_INTERNAL;
	}

	rtos_get_semaphore(&s_done_sem, BEKEN_WAIT_FOREVER);
	s_running = false;

	return s_job_result;
}

bool bk_js_app_is_running(void)
{
	return s_app_running;
}

const char *bk_js_app_current(void)
{
	return s_app_id;
}

bk_js_err_t bk_js_heap_get(bk_js_heap_stat_t *stat)
{
	jerry_heap_stats_t stats;

	if (stat == NULL) {
		return BK_JS_ERR_INVALID_ARG;
	}

	if (!s_engine_up) {
		return BK_JS_ERR_NOT_READY;
	}

	if (!jerry_heap_stats(&stats)) {
		return BK_JS_ERR_NOT_READY;
	}

	stat->heap_size = (uint32_t)stats.size;
	stat->allocated = (uint32_t)stats.allocated_bytes;
	stat->peak_allocated = (uint32_t)stats.peak_allocated_bytes;

	return BK_JS_OK;
}

const char *bk_js_err_to_str(bk_js_err_t err)
{
	switch (err) {
	case BK_JS_OK:
		return "ok";
	case BK_JS_ERR_INVALID_ARG:
		return "invalid argument";
	case BK_JS_ERR_NOT_READY:
		return "engine not ready";
	case BK_JS_ERR_NO_MEM:
		return "out of memory";
	case BK_JS_ERR_PARSE:
		return "parse error";
	case BK_JS_ERR_EXEC:
		return "script error";
	case BK_JS_ERR_TIMEOUT:
		return "timeout";
	case BK_JS_ERR_BUSY:
		return "busy";
	case BK_JS_ERR_INTERNAL:
		return "internal error";
	default:
		return "unknown error";
	}
}
