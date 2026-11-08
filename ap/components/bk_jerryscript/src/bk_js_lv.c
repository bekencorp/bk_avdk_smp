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

/*
 * LVGL bindings for the JavaScript runtime.
 *
 * Two layers are exposed to scripts:
 *
 *   lv.*   low level widget factory, parent is passed explicitly
 *   ui.*   application oriented helpers, parent is the current page
 *
 * Every wrapper object keeps a (slot, generation) pair.  The slot indexes a
 * small table holding the real lv_obj_t.  When LVGL deletes an object the slot
 * is invalidated from the LVGL delete callback, so a stale wrapper can never
 * touch freed memory; it throws "object deleted" instead.
 *
 * All LVGL calls are made from the JavaScript task with the LVGL display lock
 * held.  LVGL input events only enqueue a message, the JerryScript callback is
 * always executed by the JavaScript task.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <os/mem.h>
#include <os/str.h>
#include <components/log.h>

#include "lvgl.h"
#include "lv_vendor.h"

#include "bk_js_internal.h"

#if CONFIG_BK_JERRYSCRIPT_UI

#define BK_JS_TAG            "js.ui"
/* The dashboards build a few hundred objects, the table is small (12 bytes
 * per entry) so keep enough room for several cached pages. */
#define BK_JS_UI_HANDLE_MAX  (256)
#define BK_JS_UI_PAGE_MAX    (8)
/* Must fit the longest LVGL API name ("lv_spinner_set_anim_params"). */
#define BK_JS_UI_NAME_MAX    (48)

typedef enum {
	JS_UI_OBJ = 0,
	JS_UI_BUTTON,
	JS_UI_LABEL,
	JS_UI_IMAGE,
	JS_UI_BAR,
	JS_UI_SLIDER,
	JS_UI_SWITCH,
	JS_UI_ARC,
	JS_UI_BUTTONMATRIX,
	JS_UI_CALENDAR,
	JS_UI_CANVAS,
	JS_UI_CHART,
	JS_UI_CHECKBOX,
	JS_UI_DROPDOWN,
	JS_UI_IMAGEBUTTON,
	JS_UI_KEYBOARD,
	JS_UI_LED,
	JS_UI_LINE,
	JS_UI_LIST,
	JS_UI_MENU,
	JS_UI_ROLLER,
	JS_UI_SCALE,
	JS_UI_SPAN,
	JS_UI_SPINBOX,
	JS_UI_SPINNER,
	JS_UI_TABLE,
	JS_UI_TABVIEW,
	JS_UI_TEXTAREA,
	JS_UI_TILEVIEW,
	JS_UI_WIN,
	JS_UI_MSGBOX,
	JS_UI_ANIMIMG,
	JS_UI_ARCLABEL,
} js_ui_kind_t;

typedef struct {
	lv_obj_t *obj;
	lv_obj_t *label;
	uint32_t gen;
} js_ui_handle_t;

typedef struct {
	char name[BK_JS_UI_NAME_MAX];
	lv_obj_t *obj;
} js_ui_page_t;

static js_ui_handle_t s_handles[BK_JS_UI_HANDLE_MAX];
static js_ui_page_t s_pages[BK_JS_UI_PAGE_MAX];
static uint32_t s_page_count;
static int32_t s_page_stack[BK_JS_UI_PAGE_MAX];
static uint32_t s_page_depth;
static int32_t s_page_current = -1;

/* ------------------------------------------------------------------ */
/* Helpers                                                            */
/* ------------------------------------------------------------------ */

/*
 * The display mutex is a plain (non-recursive) FreeRTOS mutex, while the JS
 * runtime holds it for a whole script/timer/event callback.  Track the nesting
 * depth so the bindings below can keep using js_ui_lock()/js_ui_unlock() even
 * when the runtime already holds the lock.
 */
static int32_t s_ui_lock_depth;

bool bk_js_ui_lock(void)
{
	if (s_ui_lock_depth == 0) {
		if (!lv_vendor_is_initialized()) {
			return false;
		}

		lv_vendor_disp_lock();
	}

	s_ui_lock_depth++;

	return true;
}

void bk_js_ui_unlock(void)
{
	if (s_ui_lock_depth <= 0) {
		return;
	}

	s_ui_lock_depth--;

	if (s_ui_lock_depth == 0) {
		lv_vendor_disp_unlock();
	}
}

static bool js_ui_lock(void)
{
	return bk_js_ui_lock();
}

static void js_ui_unlock(void)
{
	bk_js_ui_unlock();
}

/*
 * JerryScript reports fatal errors (out of memory, engine assert) with a
 * longjmp, which would otherwise skip the unlock in the runtime loop and leave
 * the LVGL task blocked forever.  Release the mutex once and forget the depth.
 */
void bk_js_ui_reset_lock(void)
{
	if (s_ui_lock_depth > 0) {
		s_ui_lock_depth = 0;
		lv_vendor_disp_unlock();
	}
}

static jerry_value_t js_ui_throw(jerry_error_t error_type, const char *message)
{
	return jerry_throw_sz(error_type, message);
}

static jerry_value_t js_ui_not_ready(void)
{
	return js_ui_throw(JERRY_ERROR_COMMON, "display is not initialized");
}

static bool js_value_to_string(jerry_value_t value, char *buf, uint32_t size)
{
	jerry_size_t need;
	jerry_size_t copied;

	if (!jerry_value_is_string(value)) {
		return false;
	}

	need = jerry_string_size(value, JERRY_ENCODING_UTF8);
	if (need + 1 > size) {
		return false;
	}

	copied = jerry_string_to_buffer(value, JERRY_ENCODING_UTF8, (jerry_char_t *)buf,
	                                (jerry_size_t)(size - 1));
	buf[copied] = '\0';

	return true;
}

static bool js_opt_num(jerry_value_t opts, const char *key, int32_t *out)
{
	jerry_value_t value;
	bool found;

	if (!jerry_value_is_object(opts)) {
		return false;
	}

	value = jerry_object_get_sz(opts, key);
	found = jerry_value_is_number(value);

	if (found) {
		*out = (int32_t)jerry_value_as_integer(value);
	}

	jerry_value_free(value);

	return found;
}

/*
 * Colours are 0xRRGGBB.  They are read as double and masked because
 * lv.color()/getStyle() may hand out 0xFFRRGGBB, which does not fit into
 * int32_t and would otherwise saturate to 0x7FFFFFFF (white).
 */
static bool js_opt_color(jerry_value_t opts, const char *key, uint32_t *out)
{
	jerry_value_t value;
	bool found;

	if (!jerry_value_is_object(opts)) {
		return false;
	}

	value = jerry_object_get_sz(opts, key);
	found = jerry_value_is_number(value);

	if (found) {
		*out = ((uint32_t)jerry_value_as_number(value)) & 0xFFFFFFu;
	}

	jerry_value_free(value);

	return found;
}

static bool js_opt_str(jerry_value_t opts, const char *key, char *buf, uint32_t size)
{
	jerry_value_t value;
	bool found;

	if (!jerry_value_is_object(opts)) {
		return false;
	}

	value = jerry_object_get_sz(opts, key);
	found = js_value_to_string(value, buf, size);

	jerry_value_free(value);

	return found;
}

static jerry_value_t js_opt_value(jerry_value_t opts, const char *key)
{
	if (!jerry_value_is_object(opts)) {
		return jerry_undefined();
	}

	return jerry_object_get_sz(opts, key);
}

/*
 * LVGL resolves the driver letter from the first character of the path, so an
 * image source must be absolute.  Relative sources are resolved against the
 * directory of the running application / module.
 */
static void js_ui_resolve_src(const char *src, char *buf, uint32_t size)
{
	if (src[0] == '/') {
		os_strncpy(buf, src, size);
		buf[size - 1] = '\0';
		return;
	}

	bk_js_fs_resolve(src, buf, size);

	if (buf[0] != '/') {
		char tmp[BK_JS_PATH_MAX];

		snprintf(tmp, sizeof(tmp), "/%s", buf);
		os_strncpy(buf, tmp, size);
		buf[size - 1] = '\0';
	}
}

static lv_align_t js_align_from_string(const char *name)
{
	if (strcmp(name, "top_left") == 0) {
		return LV_ALIGN_TOP_LEFT;
	}
	if (strcmp(name, "top_mid") == 0) {
		return LV_ALIGN_TOP_MID;
	}
	if (strcmp(name, "top_right") == 0) {
		return LV_ALIGN_TOP_RIGHT;
	}
	if (strcmp(name, "left_mid") == 0) {
		return LV_ALIGN_LEFT_MID;
	}
	if (strcmp(name, "right_mid") == 0) {
		return LV_ALIGN_RIGHT_MID;
	}
	if (strcmp(name, "bottom_left") == 0) {
		return LV_ALIGN_BOTTOM_LEFT;
	}
	if (strcmp(name, "bottom_mid") == 0) {
		return LV_ALIGN_BOTTOM_MID;
	}
	if (strcmp(name, "bottom_right") == 0) {
		return LV_ALIGN_BOTTOM_RIGHT;
	}

	return LV_ALIGN_CENTER;
}

static lv_event_code_t js_event_from_string(const char *name, bool *found)
{
	static const struct {
		const char *name;
		lv_event_code_t code;
	} map[] = {
		{ "click", LV_EVENT_CLICKED },
		{ "clicked", LV_EVENT_CLICKED },
		{ "pressed", LV_EVENT_PRESSED },
		{ "pressing", LV_EVENT_PRESSING },
		{ "released", LV_EVENT_RELEASED },
		{ "press_lost", LV_EVENT_PRESS_LOST },
		{ "scroll", LV_EVENT_SCROLL },
		{ "long_pressed", LV_EVENT_LONG_PRESSED },
		{ "value_changed", LV_EVENT_VALUE_CHANGED },
		{ "delete", LV_EVENT_DELETE },
		{ "focused", LV_EVENT_FOCUSED },
		{ "defocused", LV_EVENT_DEFOCUSED },
		{ "ready", LV_EVENT_READY },
		{ "cancel", LV_EVENT_CANCEL },
	};

	for (uint32_t i = 0; i < sizeof(map) / sizeof(map[0]); i++) {
		if (strcmp(name, map[i].name) == 0) {
			*found = true;
			return map[i].code;
		}
	}

	*found = false;

	return LV_EVENT_CLICKED;
}

static const lv_font_t *js_font_by_size(int32_t size)
{
	if (size >= 48) {
#if LV_FONT_MONTSERRAT_48
		return &lv_font_montserrat_48;
#else
		return &lv_font_montserrat_32;
#endif
	}
	if (size >= 40) {
#if LV_FONT_MONTSERRAT_40
		return &lv_font_montserrat_40;
#else
		return &lv_font_montserrat_32;
#endif
	}
	if (size >= 32) {
#if LV_FONT_MONTSERRAT_32
		return &lv_font_montserrat_32;
#else
		return &lv_font_montserrat_24;
#endif
	}
	if (size >= 24) {
#if LV_FONT_MONTSERRAT_24
		return &lv_font_montserrat_24;
#else
		return &lv_font_montserrat_14;
#endif
	}
	if (size >= 20) {
#if LV_FONT_MONTSERRAT_20
		return &lv_font_montserrat_20;
#else
		return &lv_font_montserrat_14;
#endif
	}
	if (size >= 18) {
#if LV_FONT_MONTSERRAT_18
		return &lv_font_montserrat_18;
#else
		return &lv_font_montserrat_14;
#endif
	}
	if (size >= 16) {
#if LV_FONT_MONTSERRAT_16
		return &lv_font_montserrat_16;
#else
		return &lv_font_montserrat_14;
#endif
	}
#if LV_FONT_MONTSERRAT_12
	if (size >= 12) {
		return &lv_font_montserrat_12;
	}
#endif

	return LV_FONT_DEFAULT;
}

/* ------------------------------------------------------------------ */
/* Handle table                                                       */
/* ------------------------------------------------------------------ */

static void js_ui_delete_cb(lv_event_t *event)
{
	uint32_t index = (uint32_t)(uintptr_t)lv_event_get_user_data(event);
	lv_obj_t *target = (lv_obj_t *)lv_event_get_target(event);

	if (index < BK_JS_UI_HANDLE_MAX && s_handles[index].obj == target) {
		/* The JavaScript task owns the callbacks, flag them for reclamation. */
		bk_js_event_mark_dead(index, s_handles[index].gen);

		s_handles[index].obj = NULL;
		s_handles[index].label = NULL;
	}

	/* A page may be deleted through its wrapper, drop the page entry too. */
	for (uint32_t i = 0; i < s_page_count; i++) {
		if (s_pages[i].obj == target) {
			s_pages[i].obj = NULL;
		}
	}
}

static int32_t js_ui_handle_alloc(lv_obj_t *obj, lv_obj_t *label)
{
	for (uint32_t i = 0; i < BK_JS_UI_HANDLE_MAX; i++) {
		if (s_handles[i].obj == NULL) {
			s_handles[i].obj = obj;
			s_handles[i].label = label;
			s_handles[i].gen++;
			if (s_handles[i].gen == 0) {
				s_handles[i].gen = 1;
			}

			lv_obj_add_event_cb(obj, js_ui_delete_cb, LV_EVENT_DELETE,
			                    (void *)(uintptr_t)i);

			return (int32_t)i;
		}
	}

	return -1;
}

static int32_t js_ui_handle_find(lv_obj_t *obj)
{
	if (obj == NULL) {
		return -1;
	}

	for (uint32_t i = 0; i < BK_JS_UI_HANDLE_MAX; i++) {
		if (s_handles[i].obj == obj) {
			return (int32_t)i;
		}
	}

	return -1;
}

static js_ui_handle_t *js_ui_this(const jerry_call_info_t *call_info_p)
{
	jerry_value_t index_value;
	jerry_value_t gen_value;
	uint32_t index;
	uint32_t gen;

	if (!jerry_value_is_object(call_info_p->this_value)) {
		return NULL;
	}

	index_value = jerry_object_get_sz(call_info_p->this_value, "__i");
	gen_value = jerry_object_get_sz(call_info_p->this_value, "__g");

	index = jerry_value_is_number(index_value)
	        ? (uint32_t)jerry_value_as_integer(index_value) : UINT32_MAX;
	gen = jerry_value_is_number(gen_value)
	      ? (uint32_t)jerry_value_as_integer(gen_value) : 0;

	jerry_value_free(index_value);
	jerry_value_free(gen_value);

	if (index >= BK_JS_UI_HANDLE_MAX) {
		return NULL;
	}

	if (s_handles[index].obj == NULL || s_handles[index].gen != gen) {
		return NULL;
	}

	return &s_handles[index];
}

static lv_obj_t *js_ui_value_obj(jerry_value_t value)
{
	jerry_value_t index_value;
	jerry_value_t gen_value;
	uint32_t index;
	uint32_t gen;
	lv_obj_t *obj = NULL;

	if (!jerry_value_is_object(value)) {
		return NULL;
	}

	index_value = jerry_object_get_sz(value, "__i");
	gen_value = jerry_object_get_sz(value, "__g");

	index = jerry_value_is_number(index_value)
	        ? (uint32_t)jerry_value_as_integer(index_value) : UINT32_MAX;
	gen = jerry_value_is_number(gen_value)
	      ? (uint32_t)jerry_value_as_integer(gen_value) : 0;

	jerry_value_free(index_value);
	jerry_value_free(gen_value);

	if (index < BK_JS_UI_HANDLE_MAX && s_handles[index].obj != NULL
	    && s_handles[index].gen == gen) {
		obj = s_handles[index].obj;
	}

	return obj;
}

static jerry_value_t js_ui_wrap(int32_t index);

/* ------------------------------------------------------------------ */
/* Methods                                                            */
/* ------------------------------------------------------------------ */

static lv_obj_t *js_ui_get_this(const jerry_call_info_t *call_info_p)
{
	js_ui_handle_t *handle = js_ui_this(call_info_p);

	return handle != NULL ? handle->obj : NULL;
}

static jerry_value_t js_m_set_pos(const jerry_call_info_t *call_info_p,
                                  const jerry_value_t args_p[], const jerry_length_t args_count)
{
	lv_obj_t *obj = js_ui_get_this(call_info_p);

	if (obj == NULL) {
		return js_ui_throw(JERRY_ERROR_TYPE, "object deleted");
	}

	if (args_count < 2) {
		return js_ui_throw(JERRY_ERROR_TYPE, "setPos(x, y)");
	}

	lv_obj_set_pos(obj, (int32_t)jerry_value_as_integer(args_p[0]),
	               (int32_t)jerry_value_as_integer(args_p[1]));

	return jerry_undefined();
}

static jerry_value_t js_m_set_size(const jerry_call_info_t *call_info_p,
                                   const jerry_value_t args_p[], const jerry_length_t args_count)
{
	lv_obj_t *obj = js_ui_get_this(call_info_p);

	if (obj == NULL) {
		return js_ui_throw(JERRY_ERROR_TYPE, "object deleted");
	}

	if (args_count < 2) {
		return js_ui_throw(JERRY_ERROR_TYPE, "setSize(w, h)");
	}

	lv_obj_set_size(obj, (int32_t)jerry_value_as_integer(args_p[0]),
	                (int32_t)jerry_value_as_integer(args_p[1]));

	return jerry_undefined();
}

static jerry_value_t js_m_align(const jerry_call_info_t *call_info_p,
                                const jerry_value_t args_p[], const jerry_length_t args_count)
{
	char name[24];
	lv_obj_t *obj = js_ui_get_this(call_info_p);

	if (obj == NULL) {
		return js_ui_throw(JERRY_ERROR_TYPE, "object deleted");
	}

	if (args_count < 1 || !js_value_to_string(args_p[0], name, sizeof(name))) {
		return js_ui_throw(JERRY_ERROR_TYPE, "align(name, x, y)");
	}

	lv_obj_align(obj, js_align_from_string(name),
	             args_count > 1 ? (int32_t)jerry_value_as_integer(args_p[1]) : 0,
	             args_count > 2 ? (int32_t)jerry_value_as_integer(args_p[2]) : 0);

	return jerry_undefined();
}

static jerry_value_t js_m_center(const jerry_call_info_t *call_info_p,
                                 const jerry_value_t args_p[], const jerry_length_t args_count)
{
	lv_obj_t *obj = js_ui_get_this(call_info_p);

	(void)args_p;
	(void)args_count;

	if (obj == NULL) {
		return js_ui_throw(JERRY_ERROR_TYPE, "object deleted");
	}

	lv_obj_center(obj);

	return jerry_undefined();
}

static jerry_value_t js_m_set_text(const jerry_call_info_t *call_info_p,
                                   const jerry_value_t args_p[], const jerry_length_t args_count)
{
	js_ui_handle_t *handle = js_ui_this(call_info_p);
	jerry_value_t text;
	char buf[128];
	char *heap = NULL;
	const char *str = buf;
	lv_obj_t *obj;

	if (handle == NULL) {
		return js_ui_throw(JERRY_ERROR_TYPE, "object deleted");
	}

	if (args_count < 1) {
		return js_ui_throw(JERRY_ERROR_TYPE, "setText(text)");
	}

	obj = handle->obj;

	text = jerry_value_to_string(args_p[0]);
	if (jerry_value_is_exception(text)) {
		return text;
	}

	if (!js_value_to_string(text, buf, sizeof(buf))) {
		jerry_size_t need = jerry_string_size(text, JERRY_ENCODING_UTF8);

		heap = (char *)os_malloc((uint32_t)need + 1);
		if (heap == NULL) {
			jerry_value_free(text);
			return js_ui_throw(JERRY_ERROR_COMMON, "out of memory");
		}
		need = jerry_string_to_buffer(text, JERRY_ENCODING_UTF8, (jerry_char_t *)heap, need);
		heap[need] = '\0';
		str = heap;
	}

	/* Widgets owning a text property are updated directly, every other
	 * widget gets a child label. */
	if (lv_obj_check_type(obj, &lv_label_class)) {
		lv_label_set_text(obj, str);
		handle->label = obj;
	} else if (lv_obj_check_type(obj, &lv_textarea_class)) {
		lv_textarea_set_text(obj, str);
	} else if (lv_obj_check_type(obj, &lv_checkbox_class)) {
		lv_checkbox_set_text(obj, str);
	} else if (lv_obj_check_type(obj, &lv_arclabel_class)) {
		lv_arclabel_set_text(obj, str);
	} else if (lv_obj_check_type(obj, &lv_dropdown_class)) {
		lv_dropdown_set_options(obj, str);
	} else if (lv_obj_check_type(obj, &lv_roller_class)) {
		lv_roller_set_options(obj, str, LV_ROLLER_MODE_NORMAL);
	} else {
		if (handle->label == NULL) {
			handle->label = lv_label_create(obj);
			lv_obj_center(handle->label);
		}
		lv_label_set_text(handle->label, str);
	}

	if (heap != NULL) {
		os_free(heap);
	}
	jerry_value_free(text);

	return jerry_undefined();
}

static jerry_value_t js_m_set_src(const jerry_call_info_t *call_info_p,
                                  const jerry_value_t args_p[], const jerry_length_t args_count)
{
	char path[BK_JS_PATH_MAX];
	char full[BK_JS_PATH_MAX];
	lv_obj_t *obj = js_ui_get_this(call_info_p);

	if (obj == NULL) {
		return js_ui_throw(JERRY_ERROR_TYPE, "object deleted");
	}

	if (args_count < 1 || !js_value_to_string(args_p[0], path, sizeof(path))) {
		return js_ui_throw(JERRY_ERROR_TYPE, "setSrc(path)");
	}

	js_ui_resolve_src(path, full, sizeof(full));
	lv_image_set_src(obj, full);

	return jerry_undefined();
}

static jerry_value_t js_m_set_value(const jerry_call_info_t *call_info_p,
                                    const jerry_value_t args_p[], const jerry_length_t args_count)
{
	lv_obj_t *obj = js_ui_get_this(call_info_p);
	int32_t value;

	if (obj == NULL) {
		return js_ui_throw(JERRY_ERROR_TYPE, "object deleted");
	}

	if (args_count < 1) {
		return js_ui_throw(JERRY_ERROR_TYPE, "setValue(value)");
	}

	value = (int32_t)jerry_value_as_integer(args_p[0]);

	if (lv_obj_check_type(obj, &lv_slider_class)) {
		lv_slider_set_value(obj, value, LV_ANIM_OFF);
	} else if (lv_obj_check_type(obj, &lv_bar_class)) {
		lv_bar_set_value(obj, value, LV_ANIM_OFF);
	}

	return jerry_undefined();
}

static jerry_value_t js_m_get_value(const jerry_call_info_t *call_info_p,
                                    const jerry_value_t args_p[], const jerry_length_t args_count)
{
	lv_obj_t *obj = js_ui_get_this(call_info_p);
	int32_t value = 0;

	(void)args_p;
	(void)args_count;

	if (obj == NULL) {
		return js_ui_throw(JERRY_ERROR_TYPE, "object deleted");
	}

	if (lv_obj_check_type(obj, &lv_slider_class)) {
		value = lv_slider_get_value(obj);
	} else if (lv_obj_check_type(obj, &lv_bar_class)) {
		value = lv_bar_get_value(obj);
	} else if (lv_obj_check_type(obj, &lv_switch_class)) {
		value = lv_obj_has_state(obj, LV_STATE_CHECKED) ? 1 : 0;
	}

	return jerry_number((double)value);
}

static void js_ui_event_cb(lv_event_t *event)
{
	uint32_t slot = (uint32_t)(uintptr_t)lv_event_get_user_data(event);
	lv_event_code_t code = lv_event_get_code(event);
	lv_indev_t *indev = lv_indev_active();

	/* Snapshot the pointer here: the callback itself runs later in the
	 * JavaScript task, after LVGL has processed further input. */
	if (indev != NULL
	    && (code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING
	        || code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST)) {
		lv_point_t p;

		lv_indev_get_point(indev, &p);
		bk_js_post_event_touch(slot, (uint32_t)code, false, p.x, p.y, true);
		return;
	}

	bk_js_post_event(slot, (uint32_t)code, code == LV_EVENT_DELETE);
}

static jerry_value_t js_m_on(const jerry_call_info_t *call_info_p,
                             const jerry_value_t args_p[], const jerry_length_t args_count)
{
	char name[24];
	bool found = false;
	lv_event_code_t code;
	js_ui_handle_t *handle = js_ui_this(call_info_p);
	uint32_t slot;

	if (handle == NULL) {
		return js_ui_throw(JERRY_ERROR_TYPE, "object deleted");
	}

	if (args_count < 2 || !js_value_to_string(args_p[0], name, sizeof(name))
	    || !jerry_value_is_function(args_p[1])) {
		return js_ui_throw(JERRY_ERROR_TYPE, "on(event, callback)");
	}

	code = js_event_from_string(name, &found);
	if (!found) {
		return js_ui_throw(JERRY_ERROR_TYPE, "unknown event name");
	}

	slot = bk_js_event_add((uint32_t)(handle - s_handles), handle->gen,
	                       (uint32_t)code, args_p[1]);
	if (slot == 0) {
		return js_ui_throw(JERRY_ERROR_COMMON, "no free event slot");
	}

	lv_obj_add_event_cb(handle->obj, js_ui_event_cb, code, (void *)(uintptr_t)slot);

	return jerry_undefined();
}

static jerry_value_t js_m_delete(const jerry_call_info_t *call_info_p,
                                 const jerry_value_t args_p[], const jerry_length_t args_count)
{
	lv_obj_t *obj = js_ui_get_this(call_info_p);

	(void)args_p;
	(void)args_count;

	if (obj == NULL) {
		return jerry_undefined();
	}

	lv_obj_delete(obj);

	return jerry_undefined();
}

static jerry_value_t js_m_show(const jerry_call_info_t *call_info_p,
                               const jerry_value_t args_p[], const jerry_length_t args_count)
{
	lv_obj_t *obj = js_ui_get_this(call_info_p);

	(void)args_p;
	(void)args_count;

	if (obj == NULL) {
		return js_ui_throw(JERRY_ERROR_TYPE, "object deleted");
	}

	lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);

	return jerry_undefined();
}

static jerry_value_t js_m_hide(const jerry_call_info_t *call_info_p,
                               const jerry_value_t args_p[], const jerry_length_t args_count)
{
	lv_obj_t *obj = js_ui_get_this(call_info_p);

	(void)args_p;
	(void)args_count;

	if (obj == NULL) {
		return js_ui_throw(JERRY_ERROR_TYPE, "object deleted");
	}

	lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);

	return jerry_undefined();
}

static jerry_value_t js_m_set_opacity(const jerry_call_info_t *call_info_p,
                                      const jerry_value_t args_p[], const jerry_length_t args_count)
{
	lv_obj_t *obj = js_ui_get_this(call_info_p);

	if (obj == NULL) {
		return js_ui_throw(JERRY_ERROR_TYPE, "object deleted");
	}

	if (args_count < 1) {
		return js_ui_throw(JERRY_ERROR_TYPE, "setOpacity(0..255)");
	}

	lv_obj_set_style_bg_opa(obj, (lv_opa_t)jerry_value_as_integer(args_p[0]), 0);

	return jerry_undefined();
}

static jerry_value_t js_m_set_bg(const jerry_call_info_t *call_info_p,
                                 const jerry_value_t args_p[], const jerry_length_t args_count)
{
	lv_obj_t *obj = js_ui_get_this(call_info_p);

	if (obj == NULL) {
		return js_ui_throw(JERRY_ERROR_TYPE, "object deleted");
	}

	if (args_count < 1) {
		return js_ui_throw(JERRY_ERROR_TYPE, "setBg(color)");
	}

	lv_obj_set_style_bg_color(obj,
	                          lv_color_hex(((uint32_t)jerry_value_as_number(args_p[0])) & 0xFFFFFFu), 0);
	lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);

	return jerry_undefined();
}

static jerry_value_t js_m_set_color(const jerry_call_info_t *call_info_p,
                                    const jerry_value_t args_p[], const jerry_length_t args_count)
{
	js_ui_handle_t *handle = js_ui_this(call_info_p);
	lv_obj_t *target;

	if (handle == NULL) {
		return js_ui_throw(JERRY_ERROR_TYPE, "object deleted");
	}

	if (args_count < 1) {
		return js_ui_throw(JERRY_ERROR_TYPE, "setColor(color)");
	}

	target = handle->label != NULL ? handle->label : handle->obj;
	lv_obj_set_style_text_color(target,
	                            lv_color_hex(((uint32_t)jerry_value_as_number(args_p[0])) & 0xFFFFFFu), 0);

	return jerry_undefined();
}

static jerry_value_t js_m_set_radius(const jerry_call_info_t *call_info_p,
                                     const jerry_value_t args_p[], const jerry_length_t args_count)
{
	lv_obj_t *obj = js_ui_get_this(call_info_p);

	if (obj == NULL) {
		return js_ui_throw(JERRY_ERROR_TYPE, "object deleted");
	}

	if (args_count < 1) {
		return js_ui_throw(JERRY_ERROR_TYPE, "setRadius(radius)");
	}

	lv_obj_set_style_radius(obj, (int32_t)jerry_value_as_integer(args_p[0]), 0);

	return jerry_undefined();
}

static jerry_value_t js_m_set_font(const jerry_call_info_t *call_info_p,
                                   const jerry_value_t args_p[], const jerry_length_t args_count)
{
	js_ui_handle_t *handle = js_ui_this(call_info_p);
	lv_obj_t *target;

	if (handle == NULL) {
		return js_ui_throw(JERRY_ERROR_TYPE, "object deleted");
	}

	if (args_count < 1) {
		return js_ui_throw(JERRY_ERROR_TYPE, "setFont(size)");
	}

	target = handle->label != NULL ? handle->label : handle->obj;
	lv_obj_set_style_text_font(target, js_font_by_size((int32_t)jerry_value_as_integer(args_p[0])), 0);

	return jerry_undefined();
}

static jerry_value_t js_m_set_border(const jerry_call_info_t *call_info_p,
                                     const jerry_value_t args_p[], const jerry_length_t args_count)
{
	lv_obj_t *obj = js_ui_get_this(call_info_p);

	if (obj == NULL) {
		return js_ui_throw(JERRY_ERROR_TYPE, "object deleted");
	}

	if (args_count < 1) {
		return js_ui_throw(JERRY_ERROR_TYPE, "setBorder(width, color)");
	}

	lv_obj_set_style_border_width(obj, (int32_t)jerry_value_as_integer(args_p[0]), 0);

	if (args_count > 1) {
		lv_obj_set_style_border_color(obj,
		                              lv_color_hex(((uint32_t)jerry_value_as_number(args_p[1])) & 0xFFFFFFu), 0);
	}

	return jerry_undefined();
}

static jerry_value_t js_m_clear(const jerry_call_info_t *call_info_p,
                                const jerry_value_t args_p[], const jerry_length_t args_count)
{
	lv_obj_t *obj = js_ui_get_this(call_info_p);

	(void)args_p;
	(void)args_count;

	if (obj == NULL) {
		return js_ui_throw(JERRY_ERROR_TYPE, "object deleted");
	}

	lv_obj_clean(obj);

	return jerry_undefined();
}

typedef struct {
	const char *name;
	jerry_external_handler_t handler;
} js_ui_method_t;

/* ------------------------------------------------------------------ */
/* Generic LVGL access                                                */
/*                                                                    */
/* The sugar methods above cover the common path.  Everything else is  */
/* reachable through lv.call(name, ...) / obj.call(name, ...) and the  */
/* generic style, flag and state helpers, so no LVGL widget is out of  */
/* reach.                                                             */
/* ------------------------------------------------------------------ */

#define JS_LV_ARG_MAX   (5)
#define JS_LV_STR_MAX   (192)

typedef struct {
	const char *name;
	const char *sig;
	char ret;          /* 'v' void, 'i' int, 'b' bool, 'o' obj, 's' string */
	void *fn;
} js_lv_api_t;

/*
 * Signature characters:
 *   o  widget wrapper (lv_obj_t *)
 *   i  integer / enum
 *   b  boolean
 *   s  string
 *   m  image source (const void *, resolved like a model path)
 */
/* The reflection table is generated from the LVGL v9 headers, see
 * bk_js_lv_api_table.h.  Every function listed there can be reached through
 * lv.call(name, ...) or widget.call(name, ...). */
#include "bk_js_lv_api_table.h"

static const js_lv_api_t s_lv_apis[] = {
#define JS_LV_API_ENTRY(n, s, r, f) { n, s, r, (void *)(f) },
	JS_LV_API_LIST(JS_LV_API_ENTRY)
#undef JS_LV_API_ENTRY
};

/* Style property names, colour properties are flagged so that the value can
 * be converted with lv_color_hex(). */
typedef struct {
	const char *name;
	lv_style_prop_t prop;
	bool color;
} js_style_prop_t;

static const js_style_prop_t s_style_props[] = {
	{ "width", LV_STYLE_WIDTH, false },
	{ "height", LV_STYLE_HEIGHT, false },
	{ "length", LV_STYLE_LENGTH, false },
	{ "min_width", LV_STYLE_MIN_WIDTH, false },
	{ "max_width", LV_STYLE_MAX_WIDTH, false },
	{ "min_height", LV_STYLE_MIN_HEIGHT, false },
	{ "max_height", LV_STYLE_MAX_HEIGHT, false },
	{ "x", LV_STYLE_X, false },
	{ "y", LV_STYLE_Y, false },
	{ "align", LV_STYLE_ALIGN, false },
	{ "radius", LV_STYLE_RADIUS, false },
	{ "opa", LV_STYLE_OPA, false },
	{ "opa_layered", LV_STYLE_OPA_LAYERED, false },
	{ "pad_top", LV_STYLE_PAD_TOP, false },
	{ "pad_bottom", LV_STYLE_PAD_BOTTOM, false },
	{ "pad_left", LV_STYLE_PAD_LEFT, false },
	{ "pad_right", LV_STYLE_PAD_RIGHT, false },
	{ "pad_row", LV_STYLE_PAD_ROW, false },
	{ "pad_column", LV_STYLE_PAD_COLUMN, false },
	{ "pad_radial", LV_STYLE_PAD_RADIAL, false },
	{ "margin_top", LV_STYLE_MARGIN_TOP, false },
	{ "margin_bottom", LV_STYLE_MARGIN_BOTTOM, false },
	{ "margin_left", LV_STYLE_MARGIN_LEFT, false },
	{ "margin_right", LV_STYLE_MARGIN_RIGHT, false },
	{ "bg_color", LV_STYLE_BG_COLOR, true },
	{ "bg_opa", LV_STYLE_BG_OPA, false },
	{ "bg_grad_color", LV_STYLE_BG_GRAD_COLOR, true },
	{ "bg_grad_dir", LV_STYLE_BG_GRAD_DIR, false },
	{ "bg_main_stop", LV_STYLE_BG_MAIN_STOP, false },
	{ "bg_grad_stop", LV_STYLE_BG_GRAD_STOP, false },
	{ "bg_image_opa", LV_STYLE_BG_IMAGE_OPA, false },
	{ "bg_image_recolor", LV_STYLE_BG_IMAGE_RECOLOR, true },
	{ "bg_image_recolor_opa", LV_STYLE_BG_IMAGE_RECOLOR_OPA, false },
	{ "bg_image_tiled", LV_STYLE_BG_IMAGE_TILED, false },
	{ "border_width", LV_STYLE_BORDER_WIDTH, false },
	{ "border_color", LV_STYLE_BORDER_COLOR, true },
	{ "border_opa", LV_STYLE_BORDER_OPA, false },
	{ "border_side", LV_STYLE_BORDER_SIDE, false },
	{ "border_post", LV_STYLE_BORDER_POST, false },
	{ "outline_width", LV_STYLE_OUTLINE_WIDTH, false },
	{ "outline_color", LV_STYLE_OUTLINE_COLOR, true },
	{ "outline_opa", LV_STYLE_OUTLINE_OPA, false },
	{ "outline_pad", LV_STYLE_OUTLINE_PAD, false },
	{ "shadow_width", LV_STYLE_SHADOW_WIDTH, false },
	{ "shadow_color", LV_STYLE_SHADOW_COLOR, true },
	{ "shadow_opa", LV_STYLE_SHADOW_OPA, false },
	{ "shadow_offset_x", LV_STYLE_SHADOW_OFFSET_X, false },
	{ "shadow_offset_y", LV_STYLE_SHADOW_OFFSET_Y, false },
	{ "shadow_spread", LV_STYLE_SHADOW_SPREAD, false },
	{ "image_opa", LV_STYLE_IMAGE_OPA, false },
	{ "image_recolor", LV_STYLE_IMAGE_RECOLOR, true },
	{ "image_recolor_opa", LV_STYLE_IMAGE_RECOLOR_OPA, false },
	{ "line_width", LV_STYLE_LINE_WIDTH, false },
	{ "line_color", LV_STYLE_LINE_COLOR, true },
	{ "line_opa", LV_STYLE_LINE_OPA, false },
	{ "line_dash_width", LV_STYLE_LINE_DASH_WIDTH, false },
	{ "line_dash_gap", LV_STYLE_LINE_DASH_GAP, false },
	{ "line_rounded", LV_STYLE_LINE_ROUNDED, false },
	{ "arc_width", LV_STYLE_ARC_WIDTH, false },
	{ "arc_color", LV_STYLE_ARC_COLOR, true },
	{ "arc_opa", LV_STYLE_ARC_OPA, false },
	{ "arc_rounded", LV_STYLE_ARC_ROUNDED, false },
	{ "text_color", LV_STYLE_TEXT_COLOR, true },
	{ "text_opa", LV_STYLE_TEXT_OPA, false },
	{ "text_align", LV_STYLE_TEXT_ALIGN, false },
	{ "text_letter_space", LV_STYLE_TEXT_LETTER_SPACE, false },
	{ "text_line_space", LV_STYLE_TEXT_LINE_SPACE, false },
	{ "text_decor", LV_STYLE_TEXT_DECOR, false },
	{ "clip_corner", LV_STYLE_CLIP_CORNER, false },
	{ "layout", LV_STYLE_LAYOUT, false },
	{ "flex_flow", LV_STYLE_FLEX_FLOW, false },
	{ "flex_main_place", LV_STYLE_FLEX_MAIN_PLACE, false },
	{ "flex_cross_place", LV_STYLE_FLEX_CROSS_PLACE, false },
	{ "flex_track_place", LV_STYLE_FLEX_TRACK_PLACE, false },
	{ "flex_grow", LV_STYLE_FLEX_GROW, false },
	{ "translate_x", LV_STYLE_TRANSLATE_X, false },
	{ "translate_y", LV_STYLE_TRANSLATE_Y, false },
	{ "transform_scale_x", LV_STYLE_TRANSFORM_SCALE_X, false },
	{ "transform_scale_y", LV_STYLE_TRANSFORM_SCALE_Y, false },
	{ "transform_rotation", LV_STYLE_TRANSFORM_ROTATION, false },
	{ "transform_skew_x", LV_STYLE_TRANSFORM_SKEW_X, false },
	{ "transform_skew_y", LV_STYLE_TRANSFORM_SKEW_Y, false },
};

typedef struct {
	const char *name;
	uint32_t value;
} js_name_value_t;

static const js_name_value_t s_parts[] = {
	{ "main", LV_PART_MAIN },
	{ "indicator", LV_PART_INDICATOR },
	{ "knob", LV_PART_KNOB },
	{ "selected", LV_PART_SELECTED },
	{ "items", LV_PART_ITEMS },
	{ "cursor", LV_PART_CURSOR },
	{ "scrollbar", LV_PART_SCROLLBAR },
	{ "any", LV_PART_ANY },
};

static const js_name_value_t s_states[] = {
	{ "default", LV_STATE_DEFAULT },
	{ "checked", LV_STATE_CHECKED },
	{ "focused", LV_STATE_FOCUSED },
	{ "focus_key", LV_STATE_FOCUS_KEY },
	{ "edited", LV_STATE_EDITED },
	{ "hovered", LV_STATE_HOVERED },
	{ "pressed", LV_STATE_PRESSED },
	{ "scrolled", LV_STATE_SCROLLED },
	{ "disabled", LV_STATE_DISABLED },
	{ "user_1", LV_STATE_USER_1 },
	{ "user_2", LV_STATE_USER_2 },
	{ "user_3", LV_STATE_USER_3 },
	{ "user_4", LV_STATE_USER_4 },
};

static const js_name_value_t s_flags[] = {
	{ "hidden", LV_OBJ_FLAG_HIDDEN },
	{ "clickable", LV_OBJ_FLAG_CLICKABLE },
	{ "scrollable", LV_OBJ_FLAG_SCROLLABLE },
	{ "checkable", LV_OBJ_FLAG_CHECKABLE },
	{ "floating", LV_OBJ_FLAG_FLOATING },
	{ "press_lock", LV_OBJ_FLAG_PRESS_LOCK },
	{ "click_focusable", LV_OBJ_FLAG_CLICK_FOCUSABLE },
	{ "ignore_layout", LV_OBJ_FLAG_IGNORE_LAYOUT },
	{ "overflow_visible", LV_OBJ_FLAG_OVERFLOW_VISIBLE },
	{ "event_bubble", LV_OBJ_FLAG_EVENT_BUBBLE },
	{ "gesture_bubble", LV_OBJ_FLAG_GESTURE_BUBBLE },
	{ "adv_hittest", LV_OBJ_FLAG_ADV_HITTEST },
	{ "scroll_elastic", LV_OBJ_FLAG_SCROLL_ELASTIC },
	{ "scroll_momentum", LV_OBJ_FLAG_SCROLL_MOMENTUM },
	{ "scroll_chain", LV_OBJ_FLAG_SCROLL_CHAIN },
	{ "scroll_on_focus", LV_OBJ_FLAG_SCROLL_ON_FOCUS },
	{ "snappable", LV_OBJ_FLAG_SNAPPABLE },
	{ "user_1", LV_OBJ_FLAG_USER_1 },
	{ "user_2", LV_OBJ_FLAG_USER_2 },
	{ "user_3", LV_OBJ_FLAG_USER_3 },
	{ "user_4", LV_OBJ_FLAG_USER_4 },
};

static const js_lv_api_t *js_lv_api_find(const char *name)
{
	for (uint32_t i = 0; i < sizeof(s_lv_apis) / sizeof(s_lv_apis[0]); i++) {
		if (strcmp(s_lv_apis[i].name, name) == 0) {
			return &s_lv_apis[i];
		}
	}

	return NULL;
}

static const js_name_value_t *js_name_find(const js_name_value_t *table, uint32_t count,
                                           const char *name)
{
	for (uint32_t i = 0; i < count; i++) {
		if (strcmp(table[i].name, name) == 0) {
			return &table[i];
		}
	}

	return NULL;
}

/* "main" or "main|checked", the first token is a part, the rest states. */
static uint32_t js_selector_from_string(const char *text)
{
	uint32_t selector = 0;
	char token[BK_JS_UI_NAME_MAX];
	const char *p = text;

	for (;;) {
		uint32_t len = 0;
		const js_name_value_t *entry;

		while (*p != '\0' && *p != '|' && len + 1 < sizeof(token)) {
			token[len++] = *p++;
		}
		token[len] = '\0';

		if (len > 0) {
			entry = js_name_find(s_parts, sizeof(s_parts) / sizeof(s_parts[0]), token);
			if (entry != NULL) {
				selector |= entry->value;
			} else {
				entry = js_name_find(s_states, sizeof(s_states) / sizeof(s_states[0]), token);
				if (entry != NULL) {
					selector |= entry->value;
				}
			}
		}

		if (*p != '|') {
			break;
		}
		p++;
	}

	return selector;
}

static uintptr_t js_lv_raw_call(void *fn, uint32_t count, const uintptr_t *args)
{
	switch (count) {
	case 0:
		return ((uintptr_t (*)(void))fn)();
	case 1:
		return ((uintptr_t (*)(uintptr_t))fn)(args[0]);
	case 2:
		return ((uintptr_t (*)(uintptr_t, uintptr_t))fn)(args[0], args[1]);
	case 3:
		return ((uintptr_t (*)(uintptr_t, uintptr_t, uintptr_t))fn)(args[0], args[1], args[2]);
	case 4:
		return ((uintptr_t (*)(uintptr_t, uintptr_t, uintptr_t, uintptr_t))fn)
		       (args[0], args[1], args[2], args[3]);
	case 5:
		return ((uintptr_t (*)(uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t))fn)
		       (args[0], args[1], args[2], args[3], args[4]);
	default:
		return 0;
	}
}

static jerry_value_t js_lv_result_to_value(const js_lv_api_t *api, uintptr_t raw)
{
	switch (api->ret) {
	case 'i':
		return jerry_number((double)(int32_t)raw);
	case 'b':
		return jerry_boolean(raw != 0);
	case 's':
		return raw != 0 ? jerry_string_sz((const char *)raw) : jerry_null();
	case 'o': {
		lv_obj_t *obj = (lv_obj_t *)raw;
		int32_t index;

		if (obj == NULL) {
			return jerry_null();
		}

		index = js_ui_handle_find(obj);
		if (index < 0) {
			index = js_ui_handle_alloc(obj, NULL);
		}

		return index >= 0 ? js_ui_wrap(index) : jerry_null();
	}
	default:
		return jerry_undefined();
	}
}

/*
 * Scripts create a widget with lv.new(..., {x, y, w, h}) and then call
 * lv_obj_remove_style_all() to drop the theme.  In LVGL v9 width, height, x and
 * y are local style properties, so remove_style_all() would drop the geometry
 * as well and every widget would collapse into the top-left corner.  Keep the
 * geometry that was explicitly set on the object.
 */
static void js_ui_strip_styles_keep_geometry(lv_obj_t *obj)
{
	lv_style_value_t value;
	int32_t w = 0, h = 0, x = 0, y = 0;
	bool has_w = lv_obj_get_local_style_prop(obj, LV_STYLE_WIDTH, &value, 0) == LV_STYLE_RES_FOUND;
	bool has_h = lv_obj_get_local_style_prop(obj, LV_STYLE_HEIGHT, &value, 0) == LV_STYLE_RES_FOUND;
	bool has_x = lv_obj_get_local_style_prop(obj, LV_STYLE_X, &value, 0) == LV_STYLE_RES_FOUND;
	bool has_y = lv_obj_get_local_style_prop(obj, LV_STYLE_Y, &value, 0) == LV_STYLE_RES_FOUND;

	if (has_w) {
		lv_obj_get_local_style_prop(obj, LV_STYLE_WIDTH, &value, 0);
		w = value.num;
	}
	if (has_h) {
		lv_obj_get_local_style_prop(obj, LV_STYLE_HEIGHT, &value, 0);
		h = value.num;
	}
	if (has_x) {
		lv_obj_get_local_style_prop(obj, LV_STYLE_X, &value, 0);
		x = value.num;
	}
	if (has_y) {
		lv_obj_get_local_style_prop(obj, LV_STYLE_Y, &value, 0);
		y = value.num;
	}

	lv_obj_remove_style_all(obj);

	if (has_w) {
		lv_obj_set_width(obj, w);
	}
	if (has_h) {
		lv_obj_set_height(obj, h);
	}
	if (has_x) {
		lv_obj_set_x(obj, x);
	}
	if (has_y) {
		lv_obj_set_y(obj, y);
	}
}

/*
 * Invoke one table entry.  when this_obj is not undefined it is used for the
 * leading 'o' parameter of the signature.
 */
static jerry_value_t js_lv_api_invoke(const js_lv_api_t *api, jerry_value_t this_obj,
                                      const jerry_value_t *args, uint32_t count)
{
	uintptr_t values[JS_LV_ARG_MAX];
	char strings[JS_LV_ARG_MAX][JS_LV_STR_MAX];
	uint32_t sig_len = (uint32_t)strlen(api->sig);
	uint32_t arg_index = 0;
	uintptr_t raw;

	if (sig_len > JS_LV_ARG_MAX) {
		return js_ui_throw(JERRY_ERROR_TYPE, "too many arguments");
	}

	for (uint32_t i = 0; i < sig_len; i++) {
		jerry_value_t value;

		if (i == 0 && !jerry_value_is_undefined(this_obj)) {
			value = this_obj;
		} else {
			if (arg_index >= count) {
				return js_ui_throw(JERRY_ERROR_TYPE, "missing argument");
			}
			value = args[arg_index++];
		}

		switch (api->sig[i]) {
		case 'o': {
			lv_obj_t *obj = js_ui_value_obj(value);

			if (obj == NULL) {
				return js_ui_throw(JERRY_ERROR_TYPE, "argument is not a widget");
			}
			values[i] = (uintptr_t)obj;
			break;
		}
		case 'i':
			values[i] = (uintptr_t)(intptr_t)jerry_value_as_integer(value);
			break;
		case 'b':
			values[i] = jerry_value_is_true(value) ? 1u : 0u;
			break;
		case 's':
			if (!js_value_to_string(value, strings[i], sizeof(strings[i]))) {
				return js_ui_throw(JERRY_ERROR_TYPE, "argument is not a string");
			}
			values[i] = (uintptr_t)strings[i];
			break;
		case 'm':
			if (jerry_value_is_null(value) || jerry_value_is_undefined(value)) {
				values[i] = 0;
			} else {
				char src[BK_JS_PATH_MAX];

				if (!js_value_to_string(value, src, sizeof(src))) {
					return js_ui_throw(JERRY_ERROR_TYPE, "argument is not an image source");
				}
				js_ui_resolve_src(src, strings[i], sizeof(strings[i]));
				values[i] = (uintptr_t)strings[i];
			}
			break;
		default:
			return js_ui_throw(JERRY_ERROR_TYPE, "bad signature");
		}
	}

	if (api->sig[0] == 'o' && strcmp(api->name, "lv_obj_remove_style_all") == 0) {
		js_ui_strip_styles_keep_geometry((lv_obj_t *)values[0]);
		return jerry_undefined();
	}

	raw = js_lv_raw_call(api->fn, sig_len, values);

	return js_lv_result_to_value(api, raw);
}

/* obj.call("lv_bar_set_value", 50, 0) */
static jerry_value_t js_m_call(const jerry_call_info_t *call_info_p,
                               const jerry_value_t args_p[], const jerry_length_t args_count)
{
	char name[BK_JS_UI_NAME_MAX];
	const js_lv_api_t *api;

	if (args_count < 1 || !js_value_to_string(args_p[0], name, sizeof(name))) {
		return js_ui_throw(JERRY_ERROR_TYPE, "call(name, ...)");
	}

	api = js_lv_api_find(name);
	if (api == NULL) {
		return js_ui_throw(JERRY_ERROR_COMMON, "unknown lvgl function");
	}

	return js_lv_api_invoke(api, call_info_p->this_value, &args_p[1], (uint32_t)args_count - 1);
}

/* obj.setStyle("bg_color", 0xff0000) or obj.setStyle("bg_opa", 200, "main") */
static jerry_value_t js_m_set_style(const jerry_call_info_t *call_info_p,
                                    const jerry_value_t args_p[], const jerry_length_t args_count)
{
	lv_obj_t *obj = js_ui_get_this(call_info_p);
	char name[BK_JS_UI_NAME_MAX];
	char selector_text[BK_JS_UI_NAME_MAX];
	uint32_t selector = 0;
	const js_style_prop_t *prop = NULL;
	lv_style_value_t value;
	int32_t number;

	if (obj == NULL) {
		return js_ui_throw(JERRY_ERROR_TYPE, "object deleted");
	}

	if (args_count < 2 || !js_value_to_string(args_p[0], name, sizeof(name))) {
		return js_ui_throw(JERRY_ERROR_TYPE, "setStyle(prop, value, selector)");
	}

	for (uint32_t i = 0; i < sizeof(s_style_props) / sizeof(s_style_props[0]); i++) {
		if (strcmp(s_style_props[i].name, name) == 0) {
			prop = &s_style_props[i];
			break;
		}
	}

	if (prop == NULL) {
		return js_ui_throw(JERRY_ERROR_COMMON, "unknown style property");
	}

	number = (int32_t)jerry_value_as_integer(args_p[1]);

	if (args_count >= 3 && js_value_to_string(args_p[2], selector_text, sizeof(selector_text))) {
		selector = js_selector_from_string(selector_text);
	}

	if (prop->color) {
		value.color = lv_color_hex(((uint32_t)jerry_value_as_number(args_p[1])) & 0xFFFFFFu);
	} else {
		value.num = number;
	}

	lv_obj_set_local_style_prop(obj, prop->prop, value, selector);

	return jerry_undefined();
}

static jerry_value_t js_m_get_style(const jerry_call_info_t *call_info_p,
                                    const jerry_value_t args_p[], const jerry_length_t args_count)
{
	lv_obj_t *obj = js_ui_get_this(call_info_p);
	char name[BK_JS_UI_NAME_MAX];
	char selector_text[BK_JS_UI_NAME_MAX];
	uint32_t selector = 0;
	const js_style_prop_t *prop = NULL;
	lv_style_value_t value;

	if (obj == NULL) {
		return js_ui_throw(JERRY_ERROR_TYPE, "object deleted");
	}

	if (args_count < 1 || !js_value_to_string(args_p[0], name, sizeof(name))) {
		return js_ui_throw(JERRY_ERROR_TYPE, "getStyle(prop, selector)");
	}

	for (uint32_t i = 0; i < sizeof(s_style_props) / sizeof(s_style_props[0]); i++) {
		if (strcmp(s_style_props[i].name, name) == 0) {
			prop = &s_style_props[i];
			break;
		}
	}

	if (prop == NULL) {
		return js_ui_throw(JERRY_ERROR_COMMON, "unknown style property");
	}

	if (args_count >= 2 && js_value_to_string(args_p[1], selector_text, sizeof(selector_text))) {
		selector = js_selector_from_string(selector_text);
	}

	os_memset(&value, 0, sizeof(value));
	lv_obj_get_local_style_prop(obj, prop->prop, &value, selector);

	if (prop->color) {
		return jerry_number((double)(lv_color_to_u32(value.color) & 0xFFFFFFu));
	}

	return jerry_number((double)value.num);
}

/* obj.flag("scrollable", false) */
static jerry_value_t js_m_flag(const jerry_call_info_t *call_info_p,
                               const jerry_value_t args_p[], const jerry_length_t args_count)
{
	lv_obj_t *obj = js_ui_get_this(call_info_p);
	char name[BK_JS_UI_NAME_MAX];
	const js_name_value_t *entry;
	bool enabled = true;

	if (obj == NULL) {
		return js_ui_throw(JERRY_ERROR_TYPE, "object deleted");
	}

	if (args_count < 1 || !js_value_to_string(args_p[0], name, sizeof(name))) {
		return js_ui_throw(JERRY_ERROR_TYPE, "flag(name, enabled)");
	}

	if (args_count >= 2) {
		enabled = jerry_value_is_true(args_p[1]);
	}

	entry = js_name_find(s_flags, sizeof(s_flags) / sizeof(s_flags[0]), name);
	if (entry == NULL) {
		return js_ui_throw(JERRY_ERROR_COMMON, "unknown flag");
	}

	if (enabled) {
		lv_obj_add_flag(obj, (lv_obj_flag_t)entry->value);
	} else {
		lv_obj_remove_flag(obj, (lv_obj_flag_t)entry->value);
	}

	return jerry_undefined();
}

/* obj.state("checked", true) */
static jerry_value_t js_m_state(const jerry_call_info_t *call_info_p,
                                const jerry_value_t args_p[], const jerry_length_t args_count)
{
	lv_obj_t *obj = js_ui_get_this(call_info_p);
	char name[BK_JS_UI_NAME_MAX];
	const js_name_value_t *entry;
	bool enabled = true;

	if (obj == NULL) {
		return js_ui_throw(JERRY_ERROR_TYPE, "object deleted");
	}

	if (args_count < 1 || !js_value_to_string(args_p[0], name, sizeof(name))) {
		return js_ui_throw(JERRY_ERROR_TYPE, "state(name, enabled)");
	}

	if (args_count >= 2) {
		enabled = jerry_value_is_true(args_p[1]);
	}

	entry = js_name_find(s_states, sizeof(s_states) / sizeof(s_states[0]), name);
	if (entry == NULL) {
		return js_ui_throw(JERRY_ERROR_COMMON, "unknown state");
	}

	if (enabled) {
		lv_obj_add_state(obj, (lv_state_t)entry->value);
	} else {
		lv_obj_remove_state(obj, (lv_state_t)entry->value);
	}

	return jerry_undefined();
}

static jerry_value_t js_m_get_child_count(const jerry_call_info_t *call_info_p,
                                          const jerry_value_t args_p[], const jerry_length_t args_count)
{
	lv_obj_t *obj = js_ui_get_this(call_info_p);

	(void)args_p;
	(void)args_count;

	return obj != NULL ? jerry_number((double)lv_obj_get_child_count(obj)) : jerry_number(0);
}

static jerry_value_t js_m_get_child(const jerry_call_info_t *call_info_p,
                                    const jerry_value_t args_p[], const jerry_length_t args_count)
{
	lv_obj_t *obj = js_ui_get_this(call_info_p);
	lv_obj_t *child;
	int32_t index;

	(void)args_count;

	if (obj == NULL) {
		return jerry_null();
	}

	child = lv_obj_get_child(obj, (int32_t)jerry_value_as_integer(args_p[0]));
	if (child == NULL) {
		return jerry_null();
	}

	index = js_ui_handle_find(child);
	if (index < 0) {
		index = js_ui_handle_alloc(child, NULL);
	}

	return index >= 0 ? js_ui_wrap(index) : jerry_null();
}

static jerry_value_t js_m_get_parent(const jerry_call_info_t *call_info_p,
                                     const jerry_value_t args_p[], const jerry_length_t args_count)
{
	lv_obj_t *obj = js_ui_get_this(call_info_p);
	lv_obj_t *parent;
	int32_t index;

	(void)args_p;
	(void)args_count;

	if (obj == NULL) {
		return jerry_null();
	}

	parent = lv_obj_get_parent(obj);
	if (parent == NULL) {
		return jerry_null();
	}

	index = js_ui_handle_find(parent);
	if (index < 0) {
		index = js_ui_handle_alloc(parent, NULL);
	}

	return index >= 0 ? js_ui_wrap(index) : jerry_null();
}

static jerry_value_t js_m_clean(const jerry_call_info_t *call_info_p,
                                const jerry_value_t args_p[], const jerry_length_t args_count)
{
	lv_obj_t *obj = js_ui_get_this(call_info_p);

	(void)args_p;
	(void)args_count;

	if (obj != NULL) {
		lv_obj_clean(obj);
	}

	return jerry_undefined();
}

static jerry_value_t js_m_move_foreground(const jerry_call_info_t *call_info_p,
                                          const jerry_value_t args_p[], const jerry_length_t args_count)
{
	lv_obj_t *obj = js_ui_get_this(call_info_p);

	(void)args_p;
	(void)args_count;

	if (obj != NULL) {
		lv_obj_move_foreground(obj);
	}

	return jerry_undefined();
}

static jerry_value_t js_m_move_background(const jerry_call_info_t *call_info_p,
                                          const jerry_value_t args_p[], const jerry_length_t args_count)
{
	lv_obj_t *obj = js_ui_get_this(call_info_p);

	(void)args_p;
	(void)args_count;

	if (obj != NULL) {
		lv_obj_move_background(obj);
	}

	return jerry_undefined();
}

static jerry_value_t js_m_invalidate(const jerry_call_info_t *call_info_p,
                                     const jerry_value_t args_p[], const jerry_length_t args_count)
{
	lv_obj_t *obj = js_ui_get_this(call_info_p);

	(void)args_p;
	(void)args_count;

	if (obj != NULL) {
		lv_obj_invalidate(obj);
	}

	return jerry_undefined();
}

static const js_ui_method_t s_methods[] = {
    { "setPos", js_m_set_pos },
    { "setSize", js_m_set_size },
    { "align", js_m_align },
    { "center", js_m_center },
    { "setText", js_m_set_text },
    { "setSrc", js_m_set_src },
    { "setValue", js_m_set_value },
    { "getValue", js_m_get_value },
    { "on", js_m_on },
    { "delete", js_m_delete },
    { "show", js_m_show },
    { "hide", js_m_hide },
    { "setOpacity", js_m_set_opacity },
    { "setBg", js_m_set_bg },
    { "setColor", js_m_set_color },
    { "setRadius", js_m_set_radius },
    { "setFont", js_m_set_font },
    { "setBorder", js_m_set_border },
    { "clear", js_m_clear },
    { "call", js_m_call },
    { "setStyle", js_m_set_style },
    { "getStyle", js_m_get_style },
    { "flag", js_m_flag },
    { "state", js_m_state },
    { "getChildCount", js_m_get_child_count },
    { "getChild", js_m_get_child },
    { "getParent", js_m_get_parent },
    { "clean", js_m_clean },
    { "foreground", js_m_move_foreground },
    { "background", js_m_move_background },
    { "invalidate", js_m_invalidate },
};

static jerry_value_t js_ui_wrap(int32_t index)
{
	jerry_value_t wrapper = jerry_object();
	jerry_value_t value;

	value = jerry_number((double)index);
	jerry_value_free(jerry_object_set_sz(wrapper, "__i", value));
	jerry_value_free(value);

	value = jerry_number((double)s_handles[index].gen);
	jerry_value_free(jerry_object_set_sz(wrapper, "__g", value));
	jerry_value_free(value);

	for (uint32_t i = 0; i < sizeof(s_methods) / sizeof(s_methods[0]); i++) {
		jerry_value_t fn = jerry_function_external(s_methods[i].handler);

		jerry_value_free(jerry_object_set_sz(wrapper, s_methods[i].name, fn));
		jerry_value_free(fn);
	}

	return wrapper;
}

/* ------------------------------------------------------------------ */
/* Widget factory                                                     */
/* ------------------------------------------------------------------ */

static lv_obj_t *js_ui_new_object(js_ui_kind_t kind, lv_obj_t *parent)
{
	switch (kind) {
	case JS_UI_BUTTON:
		return lv_button_create(parent);
	case JS_UI_LABEL:
		return lv_label_create(parent);
	case JS_UI_IMAGE:
		return lv_image_create(parent);
	case JS_UI_BAR:
		return lv_bar_create(parent);
	case JS_UI_SLIDER:
		return lv_slider_create(parent);
	case JS_UI_SWITCH:
		return lv_switch_create(parent);
	case JS_UI_ARC:
		return lv_arc_create(parent);
	case JS_UI_BUTTONMATRIX:
		return lv_buttonmatrix_create(parent);
	case JS_UI_CALENDAR:
		return lv_calendar_create(parent);
	case JS_UI_CANVAS:
		return lv_canvas_create(parent);
	case JS_UI_CHART:
		return lv_chart_create(parent);
	case JS_UI_CHECKBOX:
		return lv_checkbox_create(parent);
	case JS_UI_DROPDOWN:
		return lv_dropdown_create(parent);
	case JS_UI_IMAGEBUTTON:
		return lv_imagebutton_create(parent);
	case JS_UI_KEYBOARD:
		return lv_keyboard_create(parent);
	case JS_UI_LED:
		return lv_led_create(parent);
	case JS_UI_LINE:
		return lv_line_create(parent);
	case JS_UI_LIST:
		return lv_list_create(parent);
	case JS_UI_MENU:
		return lv_menu_create(parent);
	case JS_UI_ROLLER:
		return lv_roller_create(parent);
	case JS_UI_SCALE:
		return lv_scale_create(parent);
	case JS_UI_SPAN:
		return lv_spangroup_create(parent);
	case JS_UI_SPINBOX:
		return lv_spinbox_create(parent);
	case JS_UI_SPINNER:
		return lv_spinner_create(parent);
	case JS_UI_TABLE:
		return lv_table_create(parent);
	case JS_UI_TABVIEW:
		return lv_tabview_create(parent);
	case JS_UI_TEXTAREA:
		return lv_textarea_create(parent);
	case JS_UI_TILEVIEW:
		return lv_tileview_create(parent);
	case JS_UI_WIN:
		return lv_win_create(parent);
	case JS_UI_MSGBOX:
		return lv_msgbox_create(parent);
	case JS_UI_ANIMIMG:
		return lv_animimg_create(parent);
	case JS_UI_ARCLABEL:
		return lv_arclabel_create(parent);
	default:
		return lv_obj_create(parent);
	}
}

static jerry_value_t js_ui_create(js_ui_kind_t kind, lv_obj_t *parent,
                                  jerry_value_t opts, jerry_value_t text_arg)
{
	char text[BK_JS_PATH_MAX];
	int32_t x, y, w, h, value;
	uint32_t rgb;
	lv_obj_t *obj;
	lv_obj_t *label = NULL;
	int32_t index;
	jerry_value_t callback;
	jerry_value_t result;

	obj = js_ui_new_object(kind, parent);
	if (obj == NULL) {
		return js_ui_throw(JERRY_ERROR_COMMON, "cannot create widget");
	}

	if (kind == JS_UI_LABEL || kind == JS_UI_IMAGE) {
		lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
	}

	if (kind == JS_UI_IMAGE) {
		lv_obj_set_style_pad_all(obj, 0, 0);
	}

	/* Text is either passed as a string or through the options object. */
	text[0] = '\0';

	if (js_value_to_string(text_arg, text, sizeof(text))) {
		/* text already filled */
	} else if (!js_opt_str(opts, "text", text, sizeof(text))) {
		text[0] = '\0';
	}

	if (kind == JS_UI_LABEL && text[0] != '\0') {
		lv_label_set_text(obj, text);
	} else if (kind == JS_UI_BUTTON && text[0] != '\0') {
		label = lv_label_create(obj);
		lv_label_set_text(label, text);
		lv_obj_center(label);
	} else if (kind == JS_UI_CHECKBOX && text[0] != '\0') {
		lv_checkbox_set_text(obj, text);
	} else if (kind == JS_UI_ARCLABEL && text[0] != '\0') {
		lv_arclabel_set_text(obj, text);
	} else if (kind == JS_UI_TEXTAREA && text[0] != '\0') {
		lv_textarea_set_text(obj, text);
	} else if (kind == JS_UI_DROPDOWN && text[0] != '\0') {
		lv_dropdown_set_options(obj, text);
	} else if (kind == JS_UI_ROLLER && text[0] != '\0') {
		lv_roller_set_options(obj, text, LV_ROLLER_MODE_NORMAL);
	} else if (kind == JS_UI_MSGBOX && text[0] != '\0') {
		lv_msgbox_add_title(obj, text);
	}

	if (js_opt_num(opts, "x", &x) && js_opt_num(opts, "y", &y)) {
		lv_obj_set_pos(obj, x, y);
	} else {
		if (js_opt_num(opts, "x", &x)) {
			lv_obj_set_x(obj, x);
		}
		if (js_opt_num(opts, "y", &y)) {
			lv_obj_set_y(obj, y);
		}
	}

	if (js_opt_num(opts, "w", &w)) {
		lv_obj_set_width(obj, w);
	}
	if (js_opt_num(opts, "h", &h)) {
		lv_obj_set_height(obj, h);
	}

	if (js_opt_num(opts, "radius", &value)) {
		lv_obj_set_style_radius(obj, value, 0);
	}

	if (js_opt_color(opts, "bg", &rgb)) {
		lv_obj_set_style_bg_color(obj, lv_color_hex(rgb), 0);
		lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
	}

	if (js_opt_color(opts, "color", &rgb)) {
		lv_obj_set_style_text_color(label != NULL ? label : obj,
		                            lv_color_hex(rgb), 0);
	}

	if (js_opt_num(opts, "opacity", &value)) {
		lv_obj_set_style_bg_opa(obj, (lv_opa_t)value, 0);
	}

	if (js_opt_num(opts, "border", &value)) {
		lv_obj_set_style_border_width(obj, value, 0);
	}

	if (js_opt_num(opts, "font", &value)) {
		lv_obj_set_style_text_font(label != NULL ? label : obj,
		                           js_font_by_size(value), 0);
	}

	if (js_opt_str(opts, "align", text, sizeof(text))) {
		lv_obj_align(obj, js_align_from_string(text), 0, 0);
	}

	if (kind == JS_UI_IMAGE) {
		char src[BK_JS_PATH_MAX];
		char full[BK_JS_PATH_MAX];

		src[0] = '\0';

		if (!js_value_to_string(text_arg, src, sizeof(src))) {
			js_opt_str(opts, "src", src, sizeof(src));
		}
		if (src[0] != '\0') {
			js_ui_resolve_src(src, full, sizeof(full));
			lv_image_set_src(obj, full);
		}
		if (js_opt_num(opts, "scale", &value)) {
			lv_image_set_scale(obj, (uint32_t)value);
		}
	}

	if ((kind == JS_UI_BAR || kind == JS_UI_SLIDER) && js_opt_num(opts, "value", &value)) {
		if (kind == JS_UI_SLIDER) {
			lv_slider_set_value(obj, value, LV_ANIM_OFF);
		} else {
			lv_bar_set_value(obj, value, LV_ANIM_OFF);
		}
	}

	index = js_ui_handle_alloc(obj, label);
	if (index < 0) {
		lv_obj_delete(obj);
		return js_ui_throw(JERRY_ERROR_COMMON, "too many UI objects");
	}

	callback = js_opt_value(opts, "onClick");
	if (jerry_value_is_function(callback)) {
		uint32_t slot = bk_js_event_add((uint32_t)index, s_handles[index].gen,
		                                (uint32_t)LV_EVENT_CLICKED, callback);

		if (slot != 0) {
			lv_obj_add_event_cb(obj, js_ui_event_cb, LV_EVENT_CLICKED,
			                    (void *)(uintptr_t)slot);
		}
	}
	jerry_value_free(callback);

	result = js_ui_wrap(index);

	return result;
}

/* ------------------------------------------------------------------ */
/* Parent resolution                                                  */
/* ------------------------------------------------------------------ */

static lv_obj_t *js_ui_current_parent(void)
{
	if (s_page_current >= 0 && s_pages[s_page_current].obj != NULL) {
		return s_pages[s_page_current].obj;
	}

	return lv_screen_active();
}

static lv_obj_t *js_ui_parent_arg(const jerry_value_t *args_p, jerry_length_t args_count)
{
	lv_obj_t *obj;

	if (args_count < 1) {
		return lv_screen_active();
	}

	obj = js_ui_value_obj(args_p[0]);

	return obj != NULL ? obj : lv_screen_active();
}

/* ------------------------------------------------------------------ */
/* lv.* bindings                                                      */
/* ------------------------------------------------------------------ */

static jerry_value_t js_lv_create(js_ui_kind_t kind,
                                  const jerry_value_t args_p[], jerry_length_t args_count)
{
	lv_obj_t *parent;
	jerry_value_t opts;
	jerry_value_t text_arg;
	jerry_value_t result;

	if (!js_ui_lock()) {
		return js_ui_not_ready();
	}

	parent = js_ui_parent_arg(args_p, args_count);
	opts = args_count > 1 ? args_p[1] : jerry_undefined();
	text_arg = (args_count > 1 && jerry_value_is_string(args_p[1])) ? args_p[1] : jerry_undefined();

	if (kind == JS_UI_IMAGE && args_count > 1 && jerry_value_is_string(args_p[1])) {
		text_arg = args_p[1];
		opts = jerry_undefined();
	}

	result = js_ui_create(kind, parent, opts, text_arg);

	js_ui_unlock();

	return result;
}

#define JS_LV_FACTORY(name, kind)                                                        \
	static jerry_value_t js_lv_##name(const jerry_call_info_t *call_info_p,          \
	                                  const jerry_value_t args_p[],                  \
	                                  const jerry_length_t args_count)               \
	{                                                                                \
		(void)call_info_p;                                                       \
		return js_lv_create(kind, args_p, args_count);                           \
	}

JS_LV_FACTORY(obj, JS_UI_OBJ)
JS_LV_FACTORY(button, JS_UI_BUTTON)
JS_LV_FACTORY(label, JS_UI_LABEL)
JS_LV_FACTORY(image, JS_UI_IMAGE)
JS_LV_FACTORY(bar, JS_UI_BAR)
JS_LV_FACTORY(slider, JS_UI_SLIDER)
JS_LV_FACTORY(switch, JS_UI_SWITCH)

static jerry_value_t js_lv_screen(const jerry_call_info_t *call_info_p,
                                  const jerry_value_t args_p[], const jerry_length_t args_count)
{
	lv_obj_t *screen;
	int32_t index;
	jerry_value_t result;

	(void)call_info_p;
	(void)args_p;
	(void)args_count;

	if (!js_ui_lock()) {
		return js_ui_not_ready();
	}

	screen = lv_screen_active();
	index = js_ui_handle_find(screen);
	if (index < 0) {
		index = js_ui_handle_alloc(screen, NULL);
	}

	result = index >= 0 ? js_ui_wrap(index) : jerry_null();

	js_ui_unlock();

	return result;
}

/* ------------------------------------------------------------------ */
/* ui.* bindings                                                      */
/* ------------------------------------------------------------------ */

static jerry_value_t js_ui_create_current(js_ui_kind_t kind,
                                          const jerry_value_t args_p[], jerry_length_t args_count)
{
	jerry_value_t opts;
	jerry_value_t text_arg;
	jerry_value_t result;

	if (!js_ui_lock()) {
		return js_ui_not_ready();
	}

	opts = args_count > 0 ? args_p[0] : jerry_undefined();
	text_arg = (args_count > 0 && jerry_value_is_string(args_p[0])) ? args_p[0] : jerry_undefined();

	result = js_ui_create(kind, js_ui_current_parent(), opts, text_arg);

	js_ui_unlock();

	return result;
}

#define JS_UI_FACTORY(name, kind)                                                        \
	static jerry_value_t js_ui_##name(const jerry_call_info_t *call_info_p,          \
	                                  const jerry_value_t args_p[],                  \
	                                  const jerry_length_t args_count)               \
	{                                                                                \
		(void)call_info_p;                                                       \
		return js_ui_create_current(kind, args_p, args_count);                   \
	}

JS_UI_FACTORY(obj, JS_UI_OBJ)
JS_UI_FACTORY(button, JS_UI_BUTTON)
JS_UI_FACTORY(label, JS_UI_LABEL)
JS_UI_FACTORY(image, JS_UI_IMAGE)
JS_UI_FACTORY(bar, JS_UI_BAR)
JS_UI_FACTORY(slider, JS_UI_SLIDER)
JS_UI_FACTORY(switch, JS_UI_SWITCH)

static int32_t js_ui_page_find(const char *name)
{
	for (uint32_t i = 0; i < s_page_count; i++) {
		if (strcmp(s_pages[i].name, name) == 0) {
			return (int32_t)i;
		}
	}

	return -1;
}

static lv_obj_t *js_ui_page_screen_new(void)
{
	lv_obj_t *screen = lv_obj_create(NULL);

	if (screen == NULL) {
		return NULL;
	}

	lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_color(screen, lv_color_hex(0x101318), 0);
	lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
	lv_obj_set_style_border_width(screen, 0, 0);
	lv_obj_set_style_pad_all(screen, 0, 0);

	return screen;
}

static jerry_value_t js_ui_page_wrap(int32_t page_index)
{
	int32_t handle = js_ui_handle_find(s_pages[page_index].obj);

	if (handle < 0) {
		handle = js_ui_handle_alloc(s_pages[page_index].obj, NULL);
	}

	return handle >= 0 ? js_ui_wrap(handle) : jerry_null();
}

static jerry_value_t js_ui_page(const jerry_call_info_t *call_info_p,
                                const jerry_value_t args_p[], const jerry_length_t args_count)
{
	char name[BK_JS_UI_NAME_MAX];
	int32_t index;
	bool created = false;
	jerry_value_t page_wrapper;
	jerry_value_t result;

	(void)call_info_p;

	if (args_count < 1 || !js_value_to_string(args_p[0], name, sizeof(name))) {
		return js_ui_throw(JERRY_ERROR_TYPE, "page(name, builder)");
	}

	if (!js_ui_lock()) {
		return js_ui_not_ready();
	}

	index = js_ui_page_find(name);

	if (index >= 0 && s_pages[index].obj == NULL) {
		/* The page screen was deleted by the application, create it again. */
		s_pages[index].obj = js_ui_page_screen_new();
		if (s_pages[index].obj == NULL) {
			js_ui_unlock();
			return js_ui_throw(JERRY_ERROR_COMMON, "cannot create page");
		}
		created = true;
	} else if (index < 0) {
		lv_obj_t *screen;

		if (s_page_count >= BK_JS_UI_PAGE_MAX) {
			js_ui_unlock();
			return js_ui_throw(JERRY_ERROR_COMMON, "too many pages");
		}

		screen = js_ui_page_screen_new();
		if (screen == NULL) {
			js_ui_unlock();
			return js_ui_throw(JERRY_ERROR_COMMON, "cannot create page");
		}

		index = (int32_t)s_page_count++;
		s_pages[index].obj = screen;
		os_strncpy(s_pages[index].name, name, sizeof(s_pages[index].name));
		s_pages[index].name[sizeof(s_pages[index].name) - 1] = '\0';
		created = true;
	}

	s_page_current = index;

	if (s_page_depth == 0 || s_page_stack[s_page_depth - 1] != index) {
		if (s_page_depth < BK_JS_UI_PAGE_MAX) {
			s_page_stack[s_page_depth++] = index;
		}
	}

	lv_screen_load(s_pages[index].obj);

	page_wrapper = js_ui_page_wrap(index);

	js_ui_unlock();

	/*
	 * The builder runs without the display lock, it calls ui.* which locks.
	 * It only runs when the page is created, so that re-entering a page does
	 * not duplicate widgets or timers.
	 */
	if (created && args_count > 1 && jerry_value_is_function(args_p[1])) {
		result = bk_js_call_guarded(args_p[1], &page_wrapper, 1);

		if (jerry_value_is_exception(result)) {
			jerry_value_free(page_wrapper);
			return result;
		}

		jerry_value_free(result);
	}

	return page_wrapper;
}

static jerry_value_t js_ui_back(const jerry_call_info_t *call_info_p,
                                const jerry_value_t args_p[], const jerry_length_t args_count)
{
	int32_t index;

	(void)call_info_p;
	(void)args_p;
	(void)args_count;

	if (s_page_depth <= 1) {
		return jerry_boolean(false);
	}

	if (!js_ui_lock()) {
		return js_ui_not_ready();
	}

	while (s_page_depth > 1) {
		s_page_depth--;
		index = s_page_stack[s_page_depth - 1];

		/* Skip pages whose screen was deleted by the application. */
		if (s_pages[index].obj != NULL) {
			s_page_current = index;
			lv_screen_load(s_pages[index].obj);
			break;
		}
	}

	js_ui_unlock();

	return jerry_boolean(true);
}

static jerry_value_t js_ui_current(const jerry_call_info_t *call_info_p,
                                   const jerry_value_t args_p[], const jerry_length_t args_count)
{
	(void)call_info_p;
	(void)args_p;
	(void)args_count;

	if (s_page_current < 0) {
		return jerry_string_sz("");
	}

	return jerry_string_sz(s_pages[s_page_current].name);
}

/* ------------------------------------------------------------------ */
/* Generic factory: lv.new() / ui.new()                               */
/* ------------------------------------------------------------------ */

typedef struct {
	const char *name;
	js_ui_kind_t kind;
} js_kind_t;

static const js_kind_t s_kinds[] = {
	{ "obj", JS_UI_OBJ },
	{ "button", JS_UI_BUTTON },
	{ "label", JS_UI_LABEL },
	{ "image", JS_UI_IMAGE },
	{ "bar", JS_UI_BAR },
	{ "slider", JS_UI_SLIDER },
	{ "switch", JS_UI_SWITCH },
	{ "arc", JS_UI_ARC },
	{ "buttonmatrix", JS_UI_BUTTONMATRIX },
	{ "calendar", JS_UI_CALENDAR },
	{ "canvas", JS_UI_CANVAS },
	{ "chart", JS_UI_CHART },
	{ "checkbox", JS_UI_CHECKBOX },
	{ "dropdown", JS_UI_DROPDOWN },
	{ "imagebutton", JS_UI_IMAGEBUTTON },
	{ "keyboard", JS_UI_KEYBOARD },
	{ "led", JS_UI_LED },
	{ "line", JS_UI_LINE },
	{ "list", JS_UI_LIST },
	{ "menu", JS_UI_MENU },
	{ "roller", JS_UI_ROLLER },
	{ "scale", JS_UI_SCALE },
	{ "span", JS_UI_SPAN },
	{ "spinbox", JS_UI_SPINBOX },
	{ "spinner", JS_UI_SPINNER },
	{ "table", JS_UI_TABLE },
	{ "tabview", JS_UI_TABVIEW },
	{ "textarea", JS_UI_TEXTAREA },
	{ "tileview", JS_UI_TILEVIEW },
	{ "win", JS_UI_WIN },
	{ "msgbox", JS_UI_MSGBOX },
	{ "animimg", JS_UI_ANIMIMG },
	{ "arclabel", JS_UI_ARCLABEL },
};

static bool js_kind_from_name(const char *name, js_ui_kind_t *kind)
{
	for (uint32_t i = 0; i < sizeof(s_kinds) / sizeof(s_kinds[0]); i++) {
		if (strcmp(s_kinds[i].name, name) == 0) {
			*kind = s_kinds[i].kind;
			return true;
		}
	}

	return false;
}

/* lv.new("chart", parent, { w: 400, h: 200, text: "..." }) */
static jerry_value_t js_lv_new(const jerry_call_info_t *call_info_p,
                               const jerry_value_t args_p[], const jerry_length_t args_count)
{
	char name[BK_JS_UI_NAME_MAX];
	js_ui_kind_t kind;
	lv_obj_t *parent;
	jerry_value_t opts;
	jerry_value_t text_arg;
	jerry_value_t result;

	(void)call_info_p;

	if (args_count < 1 || !js_value_to_string(args_p[0], name, sizeof(name))) {
		return js_ui_throw(JERRY_ERROR_TYPE, "new(widget, parent, options)");
	}

	if (!js_kind_from_name(name, &kind)) {
		return js_ui_throw(JERRY_ERROR_COMMON, "unknown widget");
	}

	if (!js_ui_lock()) {
		return js_ui_not_ready();
	}

	parent = args_count > 1 ? js_ui_value_obj(args_p[1]) : NULL;
	if (parent == NULL) {
		parent = lv_screen_active();
	}

	opts = args_count > 2 ? args_p[2] : jerry_undefined();
	text_arg = (args_count > 2 && jerry_value_is_string(args_p[2])) ? args_p[2] : jerry_undefined();

	result = js_ui_create(kind, parent, opts, text_arg);

	js_ui_unlock();

	return result;
}

/* lv.call("lv_tabview_add_tab", tabview, "Tab 1") */
static jerry_value_t js_lv_call(const jerry_call_info_t *call_info_p,
                                const jerry_value_t args_p[], const jerry_length_t args_count)
{
	char name[BK_JS_UI_NAME_MAX];
	const js_lv_api_t *api;
	jerry_value_t result;

	(void)call_info_p;

	if (args_count < 1 || !js_value_to_string(args_p[0], name, sizeof(name))) {
		return js_ui_throw(JERRY_ERROR_TYPE, "call(name, ...)");
	}

	api = js_lv_api_find(name);
	if (api == NULL) {
		return js_ui_throw(JERRY_ERROR_COMMON, "unknown lvgl function");
	}

	/* Everything after the name stays in place, the name is skipped. */
	result = js_lv_api_invoke(api, jerry_undefined(), &args_p[1], (uint32_t)args_count - 1);

	return result;
}

/* lv.color(0xff0000) -> 0x00ff0000 independent of the display colour depth. */
static jerry_value_t js_lv_color(const jerry_call_info_t *call_info_p,
                                 const jerry_value_t args_p[], const jerry_length_t args_count)
{
	uint32_t value;

	(void)call_info_p;

	if (args_count < 1) {
		return js_ui_throw(JERRY_ERROR_TYPE, "color(0xrrggbb)");
	}

	value = ((uint32_t)jerry_value_as_number(args_p[0])) & 0xFFFFFFu;

	return jerry_number((double)(lv_color_to_u32(lv_color_hex(value)) & 0xFFFFFFu));
}

/* ------------------------------------------------------------------ */
/* Touch and line helpers                                             */
/* ------------------------------------------------------------------ */

#define BK_JS_LINE_SLOTS   12
#define BK_JS_LINE_POINTS  32

typedef struct {
	lv_obj_t *obj;
	lv_point_precise_t pts[BK_JS_LINE_POINTS];
} js_line_slot_t;

static js_line_slot_t s_line_slots[BK_JS_LINE_SLOTS];

/* lv.touch() -> { x: n, y: n } of the active input device, or null when no
 * touch is in progress.  Intended to be called from a pressed/pressing/
 * released callback so scripts can implement drag and swipe gestures.  The
 * point captured with the event wins over the live device state, because the
 * callback runs after LVGL may already have read a newer sample. */
static jerry_value_t js_lv_touch(const jerry_call_info_t *call_info_p,
                                 const jerry_value_t args_p[], const jerry_length_t args_count)
{
	lv_indev_t *indev;
	lv_point_t p;
	int32_t event_x;
	int32_t event_y;
	bool has_event_point;
	jerry_value_t obj;
	jerry_value_t value;

	(void)call_info_p;
	(void)args_p;
	(void)args_count;

	has_event_point = bk_js_event_touch_get(&event_x, &event_y);
	if (!has_event_point) {
		indev = lv_indev_active();
		if (indev == NULL) {
			return jerry_null();
		}

		lv_indev_get_point(indev, &p);
		event_x = p.x;
		event_y = p.y;
	}

	obj = jerry_object();

	value = jerry_number((double)event_x);
	jerry_value_free(jerry_object_set_sz(obj, "x", value));
	jerry_value_free(value);

	value = jerry_number((double)event_y);
	jerry_value_free(jerry_object_set_sz(obj, "y", value));
	jerry_value_free(value);

	return obj;
}

/* lv.setLinePoints(line, [x0, y0, x1, y1, ...]).  The point array is copied
 * into a slot owned by this binding because lv_line keeps the pointer. */
static jerry_value_t js_lv_set_line_points(const jerry_call_info_t *call_info_p,
                                           const jerry_value_t args_p[],
                                           const jerry_length_t args_count)
{
	lv_obj_t *line;
	jerry_length_t len;
	uint32_t count;
	int32_t slot = -1;

	(void)call_info_p;

	if (args_count < 2 || !jerry_value_is_array(args_p[1])) {
		return js_ui_throw(JERRY_ERROR_TYPE, "setLinePoints(line, [x, y, ...])");
	}

	line = js_ui_value_obj(args_p[0]);
	if (line == NULL) {
		return js_ui_throw(JERRY_ERROR_TYPE, "setLinePoints(line, [x, y, ...])");
	}

	len = jerry_array_length(args_p[1]);
	count = (uint32_t)(len / 2);
	if (count > BK_JS_LINE_POINTS) {
		count = BK_JS_LINE_POINTS;
	}

	for (uint32_t i = 0; i < BK_JS_LINE_SLOTS; i++) {
		if (s_line_slots[i].obj == line) {
			slot = (int32_t)i;
			break;
		}
	}

	if (slot < 0) {
		for (uint32_t i = 0; i < BK_JS_LINE_SLOTS; i++) {
			lv_obj_t *obj = s_line_slots[i].obj;

			if (obj == NULL || !lv_obj_is_valid(obj)) {
				s_line_slots[i].obj = line;
				slot = (int32_t)i;
				break;
			}
		}
	}

	if (slot < 0) {
		return js_ui_throw(JERRY_ERROR_COMMON, "too many line objects");
	}

	for (uint32_t i = 0; i < count; i++) {
		jerry_value_t x = jerry_object_get_index(args_p[1], i * 2);
		jerry_value_t y = jerry_object_get_index(args_p[1], i * 2 + 1);

		s_line_slots[slot].pts[i].x = (lv_value_precise_t)jerry_value_as_integer(x);
		s_line_slots[slot].pts[i].y = (lv_value_precise_t)jerry_value_as_integer(y);

		jerry_value_free(x);
		jerry_value_free(y);
	}

	lv_line_set_points(line, s_line_slots[slot].pts, count);

	return jerry_undefined();
}

/* ui.new("label", { text: "hello" }) creates inside the current page. */
static jerry_value_t js_ui_new(const jerry_call_info_t *call_info_p,
                               const jerry_value_t args_p[], const jerry_length_t args_count)
{
	char name[BK_JS_UI_NAME_MAX];
	js_ui_kind_t kind;
	jerry_value_t opts;
	jerry_value_t text_arg;
	jerry_value_t result;

	(void)call_info_p;

	if (args_count < 1 || !js_value_to_string(args_p[0], name, sizeof(name))) {
		return js_ui_throw(JERRY_ERROR_TYPE, "new(widget, options)");
	}

	if (!js_kind_from_name(name, &kind)) {
		return js_ui_throw(JERRY_ERROR_COMMON, "unknown widget");
	}

	if (!js_ui_lock()) {
		return js_ui_not_ready();
	}

	opts = args_count > 1 ? args_p[1] : jerry_undefined();
	text_arg = (args_count > 1 && jerry_value_is_string(args_p[1])) ? args_p[1] : jerry_undefined();

	result = js_ui_create(kind, js_ui_current_parent(), opts, text_arg);

	js_ui_unlock();

	return result;
}

/* ------------------------------------------------------------------ */
/* Binding entry points                                               */
/* ------------------------------------------------------------------ */

static void js_set_fn(jerry_value_t object, const char *name, jerry_external_handler_t handler)
{
	jerry_value_t fn = jerry_function_external(handler);
	jerry_value_t result = jerry_object_set_sz(object, name, fn);

	jerry_value_free(result);
	jerry_value_free(fn);
}

void bk_js_ui_shutdown(void)
{
	bool locked = false;
	bool has_handles = false;

	for (uint32_t i = 0; i < BK_JS_UI_HANDLE_MAX; i++) {
		if (s_handles[i].obj != NULL) {
			has_handles = true;
			break;
		}
	}

	if (s_page_count == 0 && !has_handles) {
		s_page_depth = 0;
		s_page_current = -1;
		return;
	}

	if (bk_js_ui_lock()) {
		locked = true;

		for (uint32_t i = s_page_count; i > 0; i--) {
			if (s_pages[i - 1].obj != NULL) {
				lv_obj_delete(s_pages[i - 1].obj);
			}
			s_pages[i - 1].obj = NULL;
		}

		/* Objects created on the default screen are not owned by a page. */
		for (uint32_t i = 0; i < BK_JS_UI_HANDLE_MAX; i++) {
			lv_obj_t *obj = s_handles[i].obj;

			if (obj == NULL || obj == lv_obj_get_screen(obj)) {
				continue;
			}
			if (lv_obj_get_parent(obj) == NULL) {
				lv_obj_delete(obj);
			}
		}
	}

	if (locked) {
		bk_js_ui_unlock();
	}

	os_memset(s_pages, 0, sizeof(s_pages));
	os_memset(s_handles, 0, sizeof(s_handles));

	s_page_count = 0;
	s_page_depth = 0;
	s_page_current = -1;
}

void bk_js_ui_bind(jerry_value_t global)
{
	jerry_value_t lv = jerry_object();
	jerry_value_t ui = jerry_object();
	jerry_value_t result;

	js_set_fn(lv, "screen", js_lv_screen);
	js_set_fn(lv, "obj", js_lv_obj);
	js_set_fn(lv, "button", js_lv_button);
	js_set_fn(lv, "label", js_lv_label);
	js_set_fn(lv, "image", js_lv_image);
	js_set_fn(lv, "bar", js_lv_bar);
	js_set_fn(lv, "slider", js_lv_slider);
	js_set_fn(lv, "switch", js_lv_switch);
	js_set_fn(lv, "new", js_lv_new);
	js_set_fn(lv, "call", js_lv_call);
	js_set_fn(lv, "color", js_lv_color);
	js_set_fn(lv, "touch", js_lv_touch);
	js_set_fn(lv, "setLinePoints", js_lv_set_line_points);

	js_set_fn(ui, "page", js_ui_page);
	js_set_fn(ui, "back", js_ui_back);
	js_set_fn(ui, "current", js_ui_current);
	js_set_fn(ui, "obj", js_ui_obj);
	js_set_fn(ui, "button", js_ui_button);
	js_set_fn(ui, "label", js_ui_label);
	js_set_fn(ui, "image", js_ui_image);
	js_set_fn(ui, "bar", js_ui_bar);
	js_set_fn(ui, "slider", js_ui_slider);
	js_set_fn(ui, "switch", js_ui_switch);
	js_set_fn(ui, "new", js_ui_new);

	result = jerry_object_set_sz(global, "lv", lv);
	jerry_value_free(result);
	result = jerry_object_set_sz(global, "ui", ui);
	jerry_value_free(result);

	jerry_value_free(lv);
	jerry_value_free(ui);
}

#endif /* CONFIG_BK_JERRYSCRIPT_UI */
