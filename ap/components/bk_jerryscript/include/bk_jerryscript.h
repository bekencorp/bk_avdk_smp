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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief JavaScript runtime error codes.
 */
typedef enum {
	BK_JS_OK = 0,          /**< success */
	BK_JS_ERR_INVALID_ARG, /**< invalid argument */
	BK_JS_ERR_NOT_READY,   /**< engine is not running */
	BK_JS_ERR_NO_MEM,      /**< not enough memory for the engine heap */
	BK_JS_ERR_PARSE,       /**< script could not be parsed */
	BK_JS_ERR_EXEC,        /**< script threw an exception */
	BK_JS_ERR_TIMEOUT,     /**< script execution timed out */
	BK_JS_ERR_BUSY,        /**< another script is running */
	BK_JS_ERR_INTERNAL,    /**< internal failure */
} bk_js_err_t;

/**
 * @brief Runtime configuration, zero means "use the Kconfig default".
 */
typedef struct {
	uint32_t heap_size;  /**< engine heap size in bytes, 0 = default */
	uint32_t timeout_ms; /**< default script timeout in ms, 0 = default */
} bk_js_config_t;

/**
 * @brief Engine heap statistics.
 */
typedef struct {
	uint32_t heap_size;       /**< total heap size in bytes */
	uint32_t allocated;       /**< currently allocated bytes */
	uint32_t peak_allocated;  /**< peak allocated bytes */
} bk_js_heap_stat_t;

/**
 * @brief Start the JavaScript runtime.
 *
 * Creates the dedicated JavaScript task and initializes the engine.  Calling it
 * again while the runtime is already running returns BK_JS_OK.
 *
 * @param config runtime configuration, may be NULL for defaults.
 *
 * @return BK_JS_OK on success, an error code otherwise.
 */
bk_js_err_t bk_js_init(const bk_js_config_t *config);

/**
 * @brief Stop the JavaScript runtime and release the engine heap.
 *
 * @return BK_JS_OK on success, BK_JS_ERR_BUSY if a script is running.
 */
bk_js_err_t bk_js_deinit(void);

/**
 * @brief Check whether the runtime is running and the engine is initialized.
 */
bool bk_js_is_ready(void);

/**
 * @brief Run a script synchronously.
 *
 * The function blocks until the script has finished.  The engine is started
 * lazily when needed.  Console output is produced with the global print().
 *
 * @param name       script name used in error messages, may be NULL.
 * @param source     script source, must be valid until the function returns.
 * @param source_len source length in bytes, 0 means strlen(source).
 * @param timeout_ms execution timeout in ms, 0 means the Kconfig default.
 *
 * @return BK_JS_OK if the script ran without exception, an error code otherwise.
 */
bk_js_err_t bk_js_eval(const char *name, const char *source, uint32_t source_len, uint32_t timeout_ms);

/**
 * @brief Run a script file synchronously.
 *
 * A relative path is resolved against the application working directory, an
 * absolute path is used as is.  Requires CONFIG_BK_JERRYSCRIPT_FS.
 *
 * @param path       script path.
 * @param timeout_ms execution timeout in ms, 0 means the Kconfig default.
 *
 * @return BK_JS_OK if the script ran without exception, an error code otherwise.
 */
bk_js_err_t bk_js_eval_file(const char *path, uint32_t timeout_ms);

/**
 * @brief Get engine heap statistics.
 *
 * @param stat output statistics.
 *
 * @return BK_JS_OK on success, BK_JS_ERR_NOT_READY if the engine is not running.
 */
bk_js_err_t bk_js_heap_get(bk_js_heap_stat_t *stat);

/**
 * @brief Convert an error code to a printable string.
 */
const char *bk_js_err_to_str(bk_js_err_t err);

#ifdef __cplusplus
}
#endif
