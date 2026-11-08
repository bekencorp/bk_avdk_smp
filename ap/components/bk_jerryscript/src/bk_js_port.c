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
#include <stdint.h>

#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>

#include "jerryscript-port.h"
#include "bk_js_port.h"

#define BK_JS_TAG "js"

#define BK_JS_HEAP_MIN_KB   (64)
#define BK_JS_HEAP_MAX_KB   (512)

jmp_buf bk_js_fatal_jmp;
volatile bool bk_js_guard_armed = false;

static int s_fatal_code = 0;

static uint8_t *s_ctx_raw = NULL;
static uint8_t *s_ctx = NULL;
static bool s_ctx_from_psram = false;
static uint32_t s_heap_kb = BK_JS_HEAP_MAX_KB;

void bk_js_port_set_heap_size(uint32_t heap_size_kb)
{
	if (heap_size_kb >= BK_JS_HEAP_MIN_KB && heap_size_kb <= BK_JS_HEAP_MAX_KB) {
		s_heap_kb = heap_size_kb;
	}
}

int bk_js_port_fatal_code(void)
{
	return s_fatal_code;
}

void jerry_port_init(void)
{
}

void jerry_port_fatal(jerry_fatal_code_t code)
{
	s_fatal_code = (int)code;

	if (bk_js_guard_armed) {
		bk_js_guard_armed = false;
		longjmp(bk_js_fatal_jmp, 1);
	}

	BK_LOGE(BK_JS_TAG, "fatal error %d without recovery point", (int)code);
	for (;;) {
		rtos_delay_milliseconds(1000);
	}
}

void jerry_port_sleep(uint32_t sleep_time)
{
	rtos_delay_milliseconds(sleep_time);
}

size_t jerry_port_context_alloc(size_t context_size)
{
	size_t heap_size = (size_t)s_heap_kb * 1024u;
	size_t offset = (context_size + 7u) & ~(size_t)7u;
	size_t total_size = offset + heap_size;
	uint8_t *raw = NULL;

	/* The context and the heap share one block, so both always come from the
	 * same allocator.  The cacheable PSRAM heap is reserved for the OS task
	 * creation path, therefore only the regular PSRAM heap is used here. */
#if CONFIG_BK_JERRYSCRIPT_HEAP_IN_SRAM
	raw = (uint8_t *)hsram_malloc(total_size + 8u);
	s_ctx_from_psram = false;
#else
	raw = (uint8_t *)psram_malloc(total_size + 8u);
	s_ctx_from_psram = true;
#endif
	if (raw == NULL) {
		BK_LOGE(BK_JS_TAG, "context allocation failed (%u bytes)\n", (unsigned)(total_size + 8u));
		jerry_port_fatal(JERRY_FATAL_OUT_OF_MEMORY);
	}

	s_ctx_raw = raw;
	s_ctx = (uint8_t *)(((uintptr_t)raw + 7u) & ~(uintptr_t)7u);

	BK_LOGI(BK_JS_TAG, "heap %u KB from %s\n", (unsigned)s_heap_kb,
	        s_ctx_from_psram ? "PSRAM" : "HSRAM");

	return total_size;
}

struct jerry_context_t *jerry_port_context_get(void)
{
	return (struct jerry_context_t *)s_ctx;
}

void jerry_port_context_free(void)
{
	if (s_ctx_raw != NULL) {
		if (s_ctx_from_psram) {
			psram_free(s_ctx_raw);
		} else {
			os_free(s_ctx_raw);
		}
		s_ctx_raw = NULL;
		s_ctx = NULL;
	}
}

void jerry_port_log(const char *message_p)
{
	if (message_p != NULL) {
		BK_LOGI(BK_JS_TAG, "%s", message_p);
	}
}

void jerry_port_print_buffer(const jerry_char_t *buffer_p, jerry_size_t buffer_size)
{
	if (buffer_p != NULL && buffer_size > 0) {
		BK_RAW_LOGI(BK_JS_TAG, "%.*s", (int)buffer_size, (const char *)buffer_p);
	}
}

jerry_char_t *jerry_port_line_read(jerry_size_t *out_size_p)
{
	if (out_size_p != NULL) {
		*out_size_p = 0;
	}
	return NULL;
}

void jerry_port_line_free(jerry_char_t *buffer_p)
{
	(void)buffer_p;
}

jerry_char_t *jerry_port_path_normalize(const jerry_char_t *path_p, jerry_size_t path_size)
{
	(void)path_p;
	(void)path_size;
	return NULL;
}

void jerry_port_path_free(jerry_char_t *path_p)
{
	(void)path_p;
}

jerry_size_t jerry_port_path_base(const jerry_char_t *path_p)
{
	jerry_size_t base = 0;

	if (path_p == NULL) {
		return 0;
	}

	for (jerry_size_t i = 0; path_p[i] != 0; i++) {
		if (path_p[i] == '/' || path_p[i] == '\\') {
			base = (jerry_size_t)(i + 1);
		}
	}

	return base;
}

jerry_char_t *jerry_port_source_read(const char *file_name_p, jerry_size_t *out_size_p)
{
	(void)file_name_p;
	if (out_size_p != NULL) {
		*out_size_p = 0;
	}
	return NULL;
}

void jerry_port_source_free(jerry_char_t *buffer_p)
{
	(void)buffer_p;
}

int32_t jerry_port_local_tza(double unix_ms)
{
	(void)unix_ms;
	return 0;
}

double jerry_port_current_time(void)
{
	return (double)rtos_get_time();
}
