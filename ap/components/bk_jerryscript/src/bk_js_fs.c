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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <os/mem.h>
#include <os/os.h>
#include <os/str.h>
#include <components/log.h>

#include "bk_js_internal.h"

#define BK_JS_TAG "js"

#if CONFIG_BK_JERRYSCRIPT_FS

#include <fcntl.h>
#include <stdlib.h>

#include "bk_vfs.h"
#include "bk_filesystem.h"
#include "bk_partition.h"
#include "bk_std_header.h"
#include "driver/flash_partition.h"

/* A single application or module is not expected to be bigger than this. */
#define BK_JS_FILE_MAX_SIZE   (512u * 1024u)
#define BK_JS_MODULE_MAX      (8)

typedef struct {
	bool used;
	char path[BK_JS_PATH_MAX];
	jerry_value_t exports;
} js_module_t;

static char s_root[BK_JS_PATH_MAX] = "/apps";
static char s_dir[BK_JS_PATH_MAX] = "";
static js_module_t s_modules[BK_JS_MODULE_MAX];

static void js_trim_trailing_slash(char *path)
{
	uint32_t len = (uint32_t)os_strlen(path);

	while (len > 1 && path[len - 1] == '/') {
		path[--len] = '\0';
	}
}

static const char *js_join(const char *dir, const char *path, char *buf, uint32_t size)
{
	if (path == NULL) {
		buf[0] = '\0';
		return buf;
	}

	if (path[0] == '/' || dir == NULL || dir[0] == '\0') {
		snprintf(buf, size, "%s", path);
	} else {
		snprintf(buf, size, "%s/%s", dir, path);
	}

	return buf;
}

static void js_dirname(const char *path, char *buf, uint32_t size)
{
	const char *slash = NULL;

	for (const char *p = path; *p != '\0'; p++) {
		if (*p == '/') {
			slash = p;
		}
	}

	if (slash == NULL) {
		buf[0] = '\0';
		return;
	}

	uint32_t len = (uint32_t)(slash - path);

	if (len == 0) {
		len = 1;
	}
	if (len >= size) {
		len = size - 1;
	}

	os_memcpy(buf, path, len);
	buf[len] = '\0';
}

char *bk_js_fs_resolve(const char *path, char *buf, uint32_t size)
{
	return (char *)js_join(s_dir, path, buf, size);
}

void bk_js_fs_set_root(const char *root)
{
	if (root == NULL || root[0] == '\0') {
		os_strncpy(s_root, "/apps", sizeof(s_root));
	} else {
		os_strncpy(s_root, root, sizeof(s_root));
	}

	s_root[sizeof(s_root) - 1] = '\0';
	js_trim_trailing_slash(s_root);
}

const char *bk_js_fs_root(void)
{
	return s_root;
}

#if CONFIG_BK_JERRYSCRIPT_FS_AUTO_MOUNT

/* Set only when this component performed the mount, so a file system that was
 * already mounted by someone else is never unmounted behind its back. */
static bool s_mounted_here;

int bk_js_fs_mount(void)
{
	static struct bk_little_fs_partition s_part;
	bk_logic_partition_t *info;
	struct stat st;

	/* The application mounted the same file system already. */
	if (bk_vfs_stat(s_root, &st) == 0) {
		return 0;
	}

	info = bk_flash_partition_get_info(BK_PARTITION_USR_CONFIG);
	if (info == NULL) {
		BK_LOGE(BK_JS_TAG, "usr_config partition not found\n");
		return -1;
	}

	os_memset(&s_part, 0, sizeof(s_part));
	s_part.part_type = LFS_FLASH;
	s_part.mount_path = s_root;
	s_part.part_flash.start_addr = info->partition_start_addr;
	s_part.part_flash.size = info->partition_length;

	if (bk_lfs_mount(&s_part, 1) != 0) {
		BK_LOGE(BK_JS_TAG, "littlefs mount on %s failed\n", s_root);
		return -1;
	}

	s_mounted_here = true;

	BK_LOGI(BK_JS_TAG, "mounted %s on usr_config (0x%x, %u bytes)\n",
	        s_root, info->partition_start_addr, info->partition_length);

	{
		DIR *dir = bk_vfs_opendir(s_root);
		struct dirent *entry;

		if (dir == NULL) {
			BK_LOGE(BK_JS_TAG, "cannot open directory %s\n", s_root);
		} else {
			while ((entry = bk_vfs_readdir(dir)) != NULL) {
				if (entry->d_name[0] == '.') {
					continue;
				}
				BK_LOGD(BK_JS_TAG, "  %s\n", entry->d_name);
			}
			bk_vfs_closedir(dir);
		}
	}

	return 0;
}

int bk_js_fs_unmount(void)
{
	int ret;

	if (!s_mounted_here) {
		return 0;
	}

	ret = bk_vfs_umount(s_root);
	if (ret != 0) {
		BK_LOGE(BK_JS_TAG, "littlefs unmount on %s failed: %d\n", s_root, ret);
		return ret;
	}

	s_mounted_here = false;

	BK_LOGI(BK_JS_TAG, "unmounted %s\n", s_root);

	return 0;
}

#else

int bk_js_fs_mount(void)
{
	return 0;
}

int bk_js_fs_unmount(void)
{
	return 0;
}

#endif

void bk_js_fs_set_dir(const char *dir)
{
	if (dir == NULL) {
		s_dir[0] = '\0';
		return;
	}

	os_strncpy(s_dir, dir, sizeof(s_dir));
	s_dir[sizeof(s_dir) - 1] = '\0';
	js_trim_trailing_slash(s_dir);

	if (os_strcmp(s_dir, "/") == 0) {
		s_dir[0] = '\0';
	}
}

const char *bk_js_fs_dir(void)
{
	return s_dir;
}

char *bk_js_fs_read(const char *path, uint32_t *out_len)
{
	char full[BK_JS_PATH_MAX];
	char *buffer;
	uint32_t capacity;
	uint32_t file_size = 0;	/* 0 means "unknown, grow on demand" */
	uint32_t length = 0;
	struct stat st;
	int fd;

	if (out_len != NULL) {
		*out_len = 0;
	}

	if (path == NULL || path[0] == '\0') {
		return NULL;
	}

	bk_js_fs_resolve(path, full, sizeof(full));

	fd = bk_vfs_open(full, O_RDONLY);
	if (fd < 0) {
		BK_LOGE(BK_JS_TAG, "cannot open %s\n", full);
		return NULL;
	}

	/* Ask the file system for the exact size and allocate once.  The script
	 * buffer comes from the PSRAM heap: the SRAM heap is only 160 KB and is
	 * shared with the rest of the system, so a 64 KB script (or 128 KB with the
	 * old doubling growth) could not be allocated there once the engine is up. */
	if (bk_vfs_stat(full, &st) == 0 && st.st_size > 0) {
		if ((uint32_t)st.st_size > BK_JS_FILE_MAX_SIZE) {
			BK_LOGE(BK_JS_TAG, "%s is too large (%u bytes)\n", full, (unsigned)st.st_size);
			bk_vfs_close(fd);
			return NULL;
		}
		file_size = (uint32_t)st.st_size;
		capacity = file_size;
	} else {
		capacity = 4096;
	}

	buffer = (char *)psram_malloc(capacity + 1);
	if (buffer == NULL) {
		BK_LOGE(BK_JS_TAG, "no psram for %s (%u bytes, %u free)\n", full,
		        (unsigned)(capacity + 1), (unsigned)rtos_get_psram_free_heap_size());
		bk_vfs_close(fd);
		return NULL;
	}

	for (;;) {
		uint32_t want;
		int count;

		if (file_size != 0) {
			if (length >= file_size) {
				break;
			}
			want = file_size - length;
		} else {
			if (length >= capacity) {
				uint32_t new_capacity;
				char *new_buffer;

				if (capacity >= BK_JS_FILE_MAX_SIZE) {
					break;
				}

				new_capacity = capacity * 2;
				if (new_capacity > BK_JS_FILE_MAX_SIZE) {
					new_capacity = BK_JS_FILE_MAX_SIZE;
				}

				new_buffer = (char *)psram_realloc(buffer, new_capacity + 1);
				if (new_buffer == NULL) {
					BK_LOGE(BK_JS_TAG, "no psram to grow %s to %u bytes\n",
					        full, (unsigned)(new_capacity + 1));
					os_free(buffer);
					bk_vfs_close(fd);
					return NULL;
				}

				buffer = new_buffer;
				capacity = new_capacity;
			}
			want = capacity - length;
		}

		count = (int)bk_vfs_read(fd, buffer + length, want);
		if (count <= 0) {
			break;
		}

		length += (uint32_t)count;
	}

	bk_vfs_close(fd);

	buffer[length] = '\0';

	BK_LOGD(BK_JS_TAG, "read %s: %u bytes\n", full, length);

	if (out_len != NULL) {
		*out_len = length;
	}

	return buffer;
}

bool bk_js_fs_exists(const char *path)
{
	char full[BK_JS_PATH_MAX];
	struct stat st;

	if (path == NULL || path[0] == '\0') {
		return false;
	}

	bk_js_fs_resolve(path, full, sizeof(full));

	return bk_vfs_stat(full, &st) == 0;
}

void bk_js_fs_clear_modules(void)
{
	for (uint32_t i = 0; i < BK_JS_MODULE_MAX; i++) {
		if (s_modules[i].used) {
			jerry_value_free(s_modules[i].exports);
			s_modules[i].exports = jerry_undefined();
			s_modules[i].used = false;
			s_modules[i].path[0] = '\0';
		}
	}
}

void bk_js_fs_forget_modules(void)
{
	os_memset(s_modules, 0, sizeof(s_modules));
}

static js_module_t *js_module_find(const char *path)
{
	for (uint32_t i = 0; i < BK_JS_MODULE_MAX; i++) {
		if (s_modules[i].used && os_strcmp(s_modules[i].path, path) == 0) {
			return &s_modules[i];
		}
	}

	return NULL;
}

static js_module_t *js_module_alloc(const char *path)
{
	for (uint32_t i = 0; i < BK_JS_MODULE_MAX; i++) {
		if (!s_modules[i].used) {
			s_modules[i].used = true;
			os_strncpy(s_modules[i].path, path, sizeof(s_modules[i].path));
			s_modules[i].path[sizeof(s_modules[i].path) - 1] = '\0';
			s_modules[i].exports = jerry_undefined();
			return &s_modules[i];
		}
	}

	return NULL;
}

static bool js_arg_string(jerry_value_t value, char *buf, uint32_t size)
{
	jerry_value_t str;
	jerry_size_t need;
	jerry_size_t copied;

	if (!jerry_value_is_string(value)) {
		return false;
	}

	str = value;
	need = jerry_string_size(str, JERRY_ENCODING_UTF8);
	if (need + 1 > size) {
		return false;
	}

	copied = jerry_string_to_buffer(str, JERRY_ENCODING_UTF8, (jerry_char_t *)buf,
	                                (jerry_size_t)(size - 1));
	buf[copied] = '\0';

	return true;
}

static jerry_value_t js_throw(const char *message)
{
	return jerry_throw_sz(JERRY_ERROR_COMMON, message);
}

static jerry_value_t js_fs_read_handler(const jerry_call_info_t *call_info_p,
                                        const jerry_value_t args_p[],
                                        const jerry_length_t args_count)
{
	char path[BK_JS_PATH_MAX];
	uint32_t length = 0;
	char *data;
	jerry_value_t result;

	(void)call_info_p;

	if (args_count < 1 || !js_arg_string(args_p[0], path, sizeof(path))) {
		return jerry_throw_sz(JERRY_ERROR_TYPE, "fs.read(path) expects a string");
	}

	data = bk_js_fs_read(path, &length);
	if (data == NULL) {
		return jerry_null();
	}

	result = jerry_string((const jerry_char_t *)data, (jerry_size_t)length, JERRY_ENCODING_UTF8);
	os_free(data);

	return result;
}

static jerry_value_t js_fs_exists_handler(const jerry_call_info_t *call_info_p,
                                          const jerry_value_t args_p[],
                                          const jerry_length_t args_count)
{
	char path[BK_JS_PATH_MAX];

	(void)call_info_p;

	if (args_count < 1 || !js_arg_string(args_p[0], path, sizeof(path))) {
		return jerry_throw_sz(JERRY_ERROR_TYPE, "fs.exists(path) expects a string");
	}

	return jerry_boolean(bk_js_fs_exists(path));
}

static jerry_value_t js_fs_write_handler(const jerry_call_info_t *call_info_p,
                                         const jerry_value_t args_p[],
                                         const jerry_length_t args_count)
{
	char path[BK_JS_PATH_MAX];
	char full[BK_JS_PATH_MAX];
	jerry_value_t text;
	jerry_size_t need;
	jerry_size_t written;
	char *data;
	int fd;
	bool ok;

	(void)call_info_p;

	if (args_count < 2 || !js_arg_string(args_p[0], path, sizeof(path))) {
		return jerry_throw_sz(JERRY_ERROR_TYPE, "fs.write(path, text) expects a string path");
	}

	text = jerry_value_to_string(args_p[1]);
	if (jerry_value_is_exception(text)) {
		return text;
	}

	need = jerry_string_size(text, JERRY_ENCODING_UTF8);
	data = (char *)os_malloc((uint32_t)need + 1);
	if (data == NULL) {
		jerry_value_free(text);
		return jerry_throw_sz(JERRY_ERROR_COMMON, "out of memory");
	}

	written = jerry_string_to_buffer(text, JERRY_ENCODING_UTF8, (jerry_char_t *)data, need);
	data[written] = '\0';
	jerry_value_free(text);

	bk_js_fs_resolve(path, full, sizeof(full));

	fd = bk_vfs_open(full, O_WRONLY | O_CREAT | O_TRUNC);
	if (fd < 0) {
		os_free(data);
		return jerry_boolean(false);
	}

	ok = bk_vfs_write(fd, data, written) == (ssize_t)written;
	bk_vfs_close(fd);
	os_free(data);

	return jerry_boolean(ok);
}

static jerry_value_t js_fs_list_handler(const jerry_call_info_t *call_info_p,
                                        const jerry_value_t args_p[],
                                        const jerry_length_t args_count)
{
	char path[BK_JS_PATH_MAX];
	char full[BK_JS_PATH_MAX];
	jerry_value_t array;
	const char *dir_path = ".";
	DIR *dir;
	struct dirent *entry;
	uint32_t index = 0;

	(void)call_info_p;

	if (args_count >= 1 && js_arg_string(args_p[0], path, sizeof(path))) {
		dir_path = path;
	}

	bk_js_fs_resolve(dir_path, full, sizeof(full));

	dir = bk_vfs_opendir(full);
	if (dir == NULL) {
		return jerry_array(0);
	}

	array = jerry_array(0);

	while ((entry = bk_vfs_readdir(dir)) != NULL) {
		jerry_value_t name;
		jerry_value_t set_result;

		if (os_strcmp(entry->d_name, ".") == 0 || os_strcmp(entry->d_name, "..") == 0) {
			continue;
		}

		name = jerry_string_sz(entry->d_name);
		set_result = jerry_object_set_index(array, index++, name);
		jerry_value_free(set_result);
		jerry_value_free(name);
	}

	bk_vfs_closedir(dir);

	return array;
}

static jerry_value_t js_require_handler(const jerry_call_info_t *call_info_p,
                                        const jerry_value_t args_p[],
                                        const jerry_length_t args_count)
{
	char name[BK_JS_PATH_MAX];
	char resolved[BK_JS_PATH_MAX];
	char module_dir[BK_JS_PATH_MAX];
	char save_dir[BK_JS_PATH_MAX];
	char *source;
	char *wrapped;
	uint32_t source_len = 0;
	int wrapped_len;
	jerry_value_t function;
	jerry_value_t module;
	jerry_value_t exports;
	jerry_value_t require_fn;
	jerry_value_t argv[3];
	jerry_value_t call_result;
	jerry_value_t final_exports;
	js_module_t *cached;
	js_module_t *slot;
	bool has_extension = false;
	const char *base = name;

	(void)call_info_p;

	if (args_count < 1 || !js_arg_string(args_p[0], name, sizeof(name))) {
		return jerry_throw_sz(JERRY_ERROR_TYPE, "require(path) expects a string");
	}

	for (const char *p = name; *p != '\0'; p++) {
		if (*p == '/') {
			base = p + 1;
		}
	}

	for (const char *p = base; *p != '\0'; p++) {
		if (*p == '.') {
			has_extension = true;
			break;
		}
	}

	if (!has_extension) {
		uint32_t len = (uint32_t)os_strlen(name);

		if (len + 4 < sizeof(name)) {
			name[len] = '.';
			name[len + 1] = 'j';
			name[len + 2] = 's';
			name[len + 3] = '\0';
		}
	}

	bk_js_fs_resolve(name, resolved, sizeof(resolved));

	cached = js_module_find(resolved);
	if (cached != NULL) {
		return jerry_value_copy(cached->exports);
	}

	slot = js_module_alloc(resolved);
	if (slot == NULL) {
		return js_throw("too many modules");
	}

	source = bk_js_fs_read(resolved, &source_len);
	if (source == NULL) {
		slot->used = false;
		slot->path[0] = '\0';
		return js_throw("module not found");
	}

	/* Keep the wrapped copy of the module source in PSRAM as well: it is a full
	 * duplicate of the source and may be tens of KB, which does not fit in the
	 * small SRAM heap alongside the rest of the system. */
	wrapped_len = (int)source_len + 64;
	wrapped = (char *)psram_malloc((uint32_t)wrapped_len);
	if (wrapped == NULL) {
		os_free(source);
		slot->used = false;
		return js_throw("out of memory");
	}

	wrapped_len = snprintf(wrapped, (size_t)wrapped_len,
	                       "(function(module, exports, require){%s\n})", source);
	os_free(source);

	function = jerry_eval((const jerry_char_t *)wrapped, (size_t)wrapped_len, JERRY_PARSE_NO_OPTS);
	os_free(wrapped);

	if (jerry_value_is_exception(function)) {
		slot->used = false;
		slot->path[0] = '\0';
		return function;
	}

	module = jerry_object();
	exports = jerry_object();
	{
		jerry_value_t set_result = jerry_object_set_sz(module, "exports", exports);

		jerry_value_free(set_result);
	}

	require_fn = jerry_function_external(js_require_handler);
	argv[0] = module;
	argv[1] = exports;
	argv[2] = require_fn;

	js_dirname(resolved, module_dir, sizeof(module_dir));
	os_strncpy(save_dir, s_dir, sizeof(save_dir));
	save_dir[sizeof(save_dir) - 1] = '\0';
	bk_js_fs_set_dir(module_dir);

	call_result = jerry_call(function, jerry_undefined(), argv, 3);

	bk_js_fs_set_dir(save_dir);

	jerry_value_free(require_fn);
	jerry_value_free(function);
	jerry_value_free(exports);

	if (jerry_value_is_exception(call_result)) {
		jerry_value_free(module);
		slot->used = false;
		slot->path[0] = '\0';
		return call_result;
	}

	jerry_value_free(call_result);

	final_exports = jerry_object_get_sz(module, "exports");
	jerry_value_free(module);

	if (jerry_value_is_exception(final_exports)) {
		slot->used = false;
		slot->path[0] = '\0';
		return final_exports;
	}

	slot->exports = jerry_value_copy(final_exports);

	return final_exports;
}

void bk_js_fs_bind(jerry_value_t global)
{
	jerry_value_t fs = jerry_object();
	jerry_value_t fn;
	jerry_value_t set_result;

	fn = jerry_function_external(js_fs_read_handler);
	set_result = jerry_object_set_sz(fs, "read", fn);
	jerry_value_free(set_result);
	jerry_value_free(fn);

	fn = jerry_function_external(js_fs_exists_handler);
	set_result = jerry_object_set_sz(fs, "exists", fn);
	jerry_value_free(set_result);
	jerry_value_free(fn);

	fn = jerry_function_external(js_fs_write_handler);
	set_result = jerry_object_set_sz(fs, "write", fn);
	jerry_value_free(set_result);
	jerry_value_free(fn);

	fn = jerry_function_external(js_fs_list_handler);
	set_result = jerry_object_set_sz(fs, "list", fn);
	jerry_value_free(set_result);
	jerry_value_free(fn);

	set_result = jerry_object_set_sz(global, "fs", fs);
	jerry_value_free(set_result);
	jerry_value_free(fs);

	fn = jerry_function_external(js_require_handler);
	set_result = jerry_object_set_sz(global, "require", fn);
	jerry_value_free(set_result);
	jerry_value_free(fn);
}

#else /* !CONFIG_BK_JERRYSCRIPT_FS */

void bk_js_fs_set_root(const char *root)
{
	(void)root;
}

const char *bk_js_fs_root(void)
{
	return "/apps";
}

int bk_js_fs_mount(void)
{
	return 0;
}

void bk_js_fs_set_dir(const char *dir)
{
	(void)dir;
}

const char *bk_js_fs_dir(void)
{
	return "";
}

char *bk_js_fs_resolve(const char *path, char *buf, uint32_t size)
{
	if (path == NULL) {
		path = "";
	}

	os_strncpy(buf, path, size);
	buf[size - 1] = '\0';

	return buf;
}

char *bk_js_fs_read(const char *path, uint32_t *out_len)
{
	(void)path;
	if (out_len != NULL) {
		*out_len = 0;
	}
	return NULL;
}

bool bk_js_fs_exists(const char *path)
{
	(void)path;
	return false;
}

void bk_js_fs_clear_modules(void)
{
}

void bk_js_fs_forget_modules(void)
{
}

void bk_js_fs_bind(jerry_value_t global)
{
	(void)global;
}

#endif /* CONFIG_BK_JERRYSCRIPT_FS */
