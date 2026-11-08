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

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/log.h>

#include "bk_cli.h"
#include "cli_section.h"

#include "bk_jerryscript.h"
#include "bk_js_app.h"

#define BK_JS_CLI_CODE_MAX   (160)
#define BK_JS_LOOP_TIMEOUT   (2000)
#define BK_JS_CLI_APP_MAX    (8)

static const char s_js_hello[] =
	"var app = { name: 'BK7259', value: 1 + 2 };\n"
	"print('hello from ' + app.name);\n"
	"print('1 + 2 =', app.value);\n"
	"app.value;";

static const char s_js_loop[] = "var i = 0; while (true) { i++; }";

static void js_cli_help(char *pcWriteBuffer, int xWriteBufferLen)
{
	cmd_printf("usage:\r\n");
	cmd_printf("  js eval <code>   evaluate javascript code\r\n");
	cmd_printf("  js run <path>    run a script file\r\n");
	cmd_printf("  js apps          list applications found under the apps root\r\n");
	cmd_printf("  js start [id]    start an application (default: first one)\r\n");
	cmd_printf("  js stop          stop the running application\r\n");
	cmd_printf("  js status        show runtime and application state\r\n");
	cmd_printf("  js hello         run the bundled example\r\n");
	cmd_printf("  js loop [ms]     run an endless loop (timeout test)\r\n");
	cmd_printf("  js mem           show engine heap usage\r\n");
	cmd_printf("  js deinit        stop the engine\r\n");
}

static int js_cli_join_code(int argc, char **argv, char *buffer, int size)
{
	int length = 0;

	buffer[0] = '\0';

	for (int i = 2; i < argc; i++) {
		int arg_len = (int)os_strlen(argv[i]);

		if (length + arg_len + 2 > size) {
			break;
		}

		if (i > 2) {
			buffer[length++] = ' ';
		}

		os_memcpy(buffer + length, argv[i], (uint32_t)arg_len);
		length += arg_len;
		buffer[length] = '\0';
	}

	return length;
}

static void js_cli_eval(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char code[BK_JS_CLI_CODE_MAX];
	bk_js_err_t err;

	if (js_cli_join_code(argc, argv, code, sizeof(code)) == 0) {
		js_cli_help(pcWriteBuffer, xWriteBufferLen);
		return;
	}

	err = bk_js_eval("eval.js", code, 0, 0);
	if (err != BK_JS_OK) {
		cmd_printf("js: %s\r\n", bk_js_err_to_str(err));
	}
}

static void js_cli_hello(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_js_err_t err;

	(void)argc;
	(void)argv;

	err = bk_js_eval("hello.js", s_js_hello, 0, 0);
	if (err != BK_JS_OK) {
		cmd_printf("js: %s\r\n", bk_js_err_to_str(err));
	}
}

static void js_cli_loop(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t timeout_ms = BK_JS_LOOP_TIMEOUT;
	bk_js_err_t err;

	if (argc > 2) {
		timeout_ms = (uint32_t)os_strtoul(argv[2], NULL, 10);
	}

	err = bk_js_eval("loop.js", s_js_loop, 0, timeout_ms);

	cmd_printf("js loop: %s (timeout %u ms)\r\n", bk_js_err_to_str(err), timeout_ms);
}

static void js_cli_mem(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_js_heap_stat_t stat;
	bk_js_err_t err;
	uint32_t pct;

	(void)argc;
	(void)argv;

	err = bk_js_heap_get(&stat);
	if (err != BK_JS_OK) {
		cmd_printf("js: %s\r\n", bk_js_err_to_str(err));
		return;
	}

	pct = stat.heap_size ? (uint32_t)(((uint64_t)stat.peak_allocated * 100u) / stat.heap_size) : 0;
	cmd_printf("js heap: total=%u used=%u peak=%u (%u%% of total)\r\n",
	           stat.heap_size, stat.allocated, stat.peak_allocated, pct);

	if (pct >= 80) {
		cmd_printf("js heap: peak close to the limit, raise BK_JERRYSCRIPT_HEAP_SIZE_KB\r\n");
	}
}

static void js_cli_deinit(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_js_err_t err;

	(void)argc;
	(void)argv;

	err = bk_js_deinit();
	cmd_printf("js: %s\r\n", bk_js_err_to_str(err));
}

static void js_cli_run(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_js_err_t err;

	(void)pcWriteBuffer;
	(void)xWriteBufferLen;

	if (argc < 3) {
		js_cli_help(pcWriteBuffer, xWriteBufferLen);
		return;
	}

	err = bk_js_eval_file(argv[2], 0);
	if (err != BK_JS_OK) {
		cmd_printf("js run: %s\r\n", bk_js_err_to_str(err));
	}
}

static void js_cli_apps(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_js_app_info_t apps[BK_JS_CLI_APP_MAX];
	uint32_t count;

	(void)pcWriteBuffer;
	(void)xWriteBufferLen;
	(void)argc;
	(void)argv;

	bk_js_init(NULL);

	count = bk_js_app_scan(apps, BK_JS_CLI_APP_MAX);
	if (count == 0) {
		cmd_printf("js: no application found\r\n");
		return;
	}

	for (uint32_t i = 0; i < count; i++) {
		cmd_printf("  %s  %s  %s\r\n", apps[i].id, apps[i].name, apps[i].version);
	}
}

static void js_cli_start(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_js_err_t err;

	(void)pcWriteBuffer;
	(void)xWriteBufferLen;

	if (bk_js_app_is_running()) {
		cmd_printf("js: %s is already running\r\n", bk_js_app_current());
		return;
	}

	err = bk_js_app_start(argc > 2 ? argv[2] : NULL);
	if (err != BK_JS_OK) {
		cmd_printf("js start: %s\r\n", bk_js_err_to_str(err));
		return;
	}

	cmd_printf("js: started %s\r\n", bk_js_app_current());
}

static void js_cli_stop(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_js_err_t err;

	(void)pcWriteBuffer;
	(void)xWriteBufferLen;
	(void)argc;
	(void)argv;

	err = bk_js_app_stop();
	cmd_printf("js: %s\r\n", bk_js_err_to_str(err));
}

static void js_cli_status(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_js_heap_stat_t stat;

	(void)pcWriteBuffer;
	(void)xWriteBufferLen;
	(void)argc;
	(void)argv;

	cmd_printf("js runtime: %s\r\n", bk_js_is_ready() ? "ready" : "not ready");
	cmd_printf("js app    : %s\r\n",
	           bk_js_app_is_running() ? bk_js_app_current() : "(none)");

	if (bk_js_heap_get(&stat) == BK_JS_OK) {
		cmd_printf("js heap   : total=%u used=%u peak=%u\r\n",
		           stat.heap_size, stat.allocated, stat.peak_allocated);
	}
}

static void js_cli_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		js_cli_help(pcWriteBuffer, xWriteBufferLen);
		return;
	}

	if (os_strcmp(argv[1], "eval") == 0) {
		js_cli_eval(pcWriteBuffer, xWriteBufferLen, argc, argv);
	} else if (os_strcmp(argv[1], "run") == 0) {
		js_cli_run(pcWriteBuffer, xWriteBufferLen, argc, argv);
	} else if (os_strcmp(argv[1], "apps") == 0) {
		js_cli_apps(pcWriteBuffer, xWriteBufferLen, argc, argv);
	} else if (os_strcmp(argv[1], "start") == 0) {
		js_cli_start(pcWriteBuffer, xWriteBufferLen, argc, argv);
	} else if (os_strcmp(argv[1], "stop") == 0) {
		js_cli_stop(pcWriteBuffer, xWriteBufferLen, argc, argv);
	} else if (os_strcmp(argv[1], "status") == 0) {
		js_cli_status(pcWriteBuffer, xWriteBufferLen, argc, argv);
	} else if (os_strcmp(argv[1], "hello") == 0) {
		js_cli_hello(pcWriteBuffer, xWriteBufferLen, argc, argv);
	} else if (os_strcmp(argv[1], "loop") == 0) {
		js_cli_loop(pcWriteBuffer, xWriteBufferLen, argc, argv);
	} else if (os_strcmp(argv[1], "mem") == 0) {
		js_cli_mem(pcWriteBuffer, xWriteBufferLen, argc, argv);
	} else if (os_strcmp(argv[1], "deinit") == 0) {
		js_cli_deinit(pcWriteBuffer, xWriteBufferLen, argc, argv);
	} else {
		js_cli_help(pcWriteBuffer, xWriteBufferLen);
	}
}

COMPONENTS_CLI_CMD_EXPORT
static const struct cli_command s_js_commands[] = {
	{"js", "javascript runtime: js eval|run|apps|start|stop|status|mem|deinit", js_cli_command},
};
