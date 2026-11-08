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
#include <os/str.h>

#include "bk_js_internal.h"
#include "bk_js_app.h"

#if CONFIG_BK_JERRYSCRIPT_FS
#include "bk_vfs.h"
#include "bk_std_header.h"
#include "bk_posix.h"
#endif

static const char *s_builtin_app =
	"function buildHome() {\n"
	"  ui.label({ text: 'BK7259 JS Runtime', x: 40, y: 70, h: 40, font: 24, color: 0xffffff });\n"
	"  ui.label({ text: 'JerryScript + LVGL', x: 40, y: 120, h: 24, color: 0x9aa7b8 });\n"
	"  ui.button({ text: 'Open counter', x: 40, y: 200, w: 320, h: 72, radius: 16,\n"
	"              bg: 0x2f6fed, color: 0xffffff,\n"
	"              onClick: function () { ui.page('counter', buildCounter); } });\n"
	"  ui.button({ text: 'About', x: 40, y: 300, w: 320, h: 72, radius: 16,\n"
	"              bg: 0x2a2f3a, color: 0xffffff,\n"
	"              onClick: function () { ui.page('about', buildAbout); } });\n"
	"}\n"
	"function buildCounter() {\n"
	"  var value = 0;\n"
	"  var label = ui.label({ text: '0', x: 40, y: 80, h: 60, font: 24, color: 0x00e08a });\n"
	"  var id = timer.setInterval(function () {\n"
	"    value = value + 1;\n"
	"    label.setText('' + value);\n"
	"  }, 1000);\n"
	"  ui.button({ text: 'Reset', x: 40, y: 200, w: 240, h: 64, radius: 16,\n"
	"              bg: 0x2a2f3a, color: 0xffffff,\n"
	"              onClick: function () { value = 0; label.setText('0'); } });\n"
	"  ui.button({ text: 'Back', x: 40, y: 300, w: 240, h: 64, radius: 16,\n"
	"              bg: 0x2a2f3a, color: 0xffffff,\n"
	"              onClick: function () { timer.clear(id); ui.back(); } });\n"
	"}\n"
	"function buildAbout() {\n"
	"  ui.label({ text: 'Generic JavaScript runtime', x: 40, y: 70, h: 36, font: 24, color: 0xffffff });\n"
	"  ui.label({ text: 'Apps are loaded from /apps/<id>/manifest.json', x: 40, y: 120, h: 24, color: 0x9aa7b8 });\n"
	"  ui.button({ text: 'Back', x: 40, y: 220, w: 240, h: 64, radius: 16,\n"
	"              bg: 0x2a2f3a, color: 0xffffff,\n"
	"              onClick: function () { ui.back(); } });\n"
	"}\n"
	"ui.page('home', buildHome);\n";

const char *bk_js_builtin_app_source(void)
{
	return s_builtin_app;
}

const char *bk_js_builtin_app_name(void)
{
	return "Built-in demo";
}

static const char *json_find_value(const char *json, const char *key)
{
	char pattern[48];
	const char *found;
	const char *p;

	snprintf(pattern, sizeof(pattern), "\"%s\"", key);

	found = strstr(json, pattern);
	if (found == NULL) {
		return NULL;
	}

	p = found + os_strlen(pattern);

	while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
		p++;
	}

	if (*p != ':') {
		return NULL;
	}

	p++;

	while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
		p++;
	}

	return p;
}

static bool json_get_string(const char *json, const char *key, char *out, uint32_t size)
{
	const char *p = json_find_value(json, key);
	uint32_t index = 0;

	if (p == NULL || *p != '"') {
		return false;
	}

	p++;

	while (*p != '\0' && *p != '"' && index + 1 < size) {
		if (*p == '\\' && p[1] != '\0') {
			p++;
		}
		out[index++] = *p++;
	}

	out[index] = '\0';

	return index > 0;
}

bool bk_js_manifest_parse(const char *json, bk_js_manifest_t *out)
{
	if (json == NULL || out == NULL) {
		return false;
	}

	os_memset(out, 0, sizeof(*out));

	if (!json_get_string(json, "id", out->id, sizeof(out->id))) {
		return false;
	}

	if (!json_get_string(json, "name", out->name, sizeof(out->name))) {
		os_strncpy(out->name, out->id, sizeof(out->name));
		out->name[sizeof(out->name) - 1] = '\0';
	}

	if (!json_get_string(json, "version", out->version, sizeof(out->version))) {
		os_strncpy(out->version, "1.0.0", sizeof(out->version));
		out->version[sizeof(out->version) - 1] = '\0';
	}

	if (!json_get_string(json, "entry", out->entry, sizeof(out->entry))) {
		os_strncpy(out->entry, "main.js", sizeof(out->entry));
		out->entry[sizeof(out->entry) - 1] = '\0';
	}

	return true;
}

uint32_t bk_js_scan_apps(const char *root, bk_js_manifest_t *out, uint32_t max)
{
#if CONFIG_BK_JERRYSCRIPT_FS
	char root_full[BK_JS_PATH_MAX];
	DIR *dir;
	struct dirent *entry;
	uint32_t count = 0;

	if (root == NULL || out == NULL || max == 0) {
		return 0;
	}

	bk_js_fs_resolve(root, root_full, sizeof(root_full));

	dir = bk_vfs_opendir(root_full);
	if (dir == NULL) {
		return 0;
	}

	while (count < max && (entry = bk_vfs_readdir(dir)) != NULL) {
		char manifest_path[BK_JS_PATH_MAX];
		char fallback_id[BK_JS_ID_MAX];
		char *json;
		uint32_t json_len = 0;

		if (entry->d_name[0] == '.') {
			continue;
		}

		snprintf(manifest_path, sizeof(manifest_path), "%s/%s/manifest.json",
		         root_full, entry->d_name);

		json = bk_js_fs_read(manifest_path, &json_len);
		if (json == NULL) {
			continue;
		}

		json[json_len] = '\0';

		if (!bk_js_manifest_parse(json, &out[count])) {
			os_strncpy(fallback_id, entry->d_name, sizeof(fallback_id));
			fallback_id[sizeof(fallback_id) - 1] = '\0';
			os_memset(&out[count], 0, sizeof(out[count]));
			os_strncpy(out[count].id, fallback_id, sizeof(out[count].id));
			os_strncpy(out[count].name, fallback_id, sizeof(out[count].name));
			os_strncpy(out[count].version, "1.0.0", sizeof(out[count].version));
			os_strncpy(out[count].entry, "main.js", sizeof(out[count].entry));
		}

		os_free(json);
		count++;
	}

	bk_vfs_closedir(dir);

	return count;
#else
	(void)root;
	(void)out;
	(void)max;

	return 0;
#endif
}

uint32_t bk_js_app_scan(bk_js_app_info_t *infos, uint32_t max)
{
	bk_js_manifest_t manifests[8];
	uint32_t count;
	uint32_t i;

	if (infos == NULL || max == 0) {
		return 0;
	}

	if (max > 8) {
		max = 8;
	}

	count = bk_js_scan_apps(bk_js_fs_root(), manifests, max);

	for (i = 0; i < count; i++) {
		os_memset(&infos[i], 0, sizeof(infos[i]));
		os_strncpy(infos[i].id, manifests[i].id, sizeof(infos[i].id));
		os_strncpy(infos[i].name, manifests[i].name, sizeof(infos[i].name));
		os_strncpy(infos[i].version, manifests[i].version, sizeof(infos[i].version));
	}

	return count;
}
