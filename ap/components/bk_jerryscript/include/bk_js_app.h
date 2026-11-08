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

#include "bk_jerryscript.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BK_JS_APP_ID_MAX      (40)
#define BK_JS_APP_NAME_MAX    (40)
#define BK_JS_APP_VERSION_MAX (20)
#define BK_JS_APP_ROOT_MAX    (128)

/**
 * @brief Application description, read from <apps_root>/<id>/manifest.json.
 */
typedef struct {
	char id[BK_JS_APP_ID_MAX];
	char name[BK_JS_APP_NAME_MAX];
	char version[BK_JS_APP_VERSION_MAX];
} bk_js_app_info_t;

/**
 * @brief Application runtime configuration, zero means "use the Kconfig default".
 */
typedef struct {
	const char *apps_root;   /**< directory containing one sub-directory per app */
	const char *default_app; /**< app id started by auto_start, NULL = first found */
	uint32_t heap_size;      /**< engine heap in bytes, 0 = default */
	uint32_t timeout_ms;     /**< default script timeout in ms, 0 = default */
	bool auto_start;         /**< start an application right after init */
} bk_js_runtime_config_t;

/**
 * @brief Start the application runtime task and initialize the JS engine.
 *
 * When @p config is NULL the Kconfig defaults are used.
 *
 * @return BK_JS_OK on success, an error code otherwise.
 */
bk_js_err_t bk_js_runtime_init(const bk_js_runtime_config_t *config);

/**
 * @brief Stop the running application, the JS engine and the runtime task.
 */
bk_js_err_t bk_js_runtime_deinit(void);

/**
 * @brief Scan the applications root directory.
 *
 * @param infos output array, may be NULL when only the count is needed.
 * @param max   capacity of @p infos.
 *
 * @return number of applications found.
 */
uint32_t bk_js_app_scan(bk_js_app_info_t *infos, uint32_t max);

/**
 * @brief Start an application.
 *
 * @param app_id application id (sub-directory name), NULL or "" starts the
 *               first application found under the root directory.
 *
 * @return BK_JS_OK on success, BK_JS_ERR_BUSY when an app is already running.
 */
bk_js_err_t bk_js_app_start(const char *app_id);

/**
 * @brief Stop the running application and tear down its VM.
 */
bk_js_err_t bk_js_app_stop(void);

/**
 * @brief Check whether an application is running.
 */
bool bk_js_app_is_running(void);

/**
 * @brief Get the id of the running application, or "" when none is running.
 */
const char *bk_js_app_current(void);

#ifdef __cplusplus
}
#endif
