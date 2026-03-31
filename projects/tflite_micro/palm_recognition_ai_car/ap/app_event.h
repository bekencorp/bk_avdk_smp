/**
 * @brief Palm event module: result queue, callback for model, and tracking task.
 *
 * The tracking task reads palm results and then:
 * - in direct-control mode: drives car left/right and gimbal up/down.
 * - in coordinate-send mode: packs and sends bounding box coordinates over UART.
 * Thresholds define a dead zone to avoid frequent small movements in direct mode.
 */

#pragma once

#include <common/bk_include.h>

/*
 * Do not include PalmDetectionModel.h here: it pulls in C++/TensorFlow headers (e.g. cstdarg).
 * app_event.c is compiled as C; define callback type here for C, get it from Palm in C++.
 */
#ifdef __cplusplus
#include "PalmDetectionModel.h"
#else
typedef void (*palm_result_callback_t)(int has_palm, float cx, float cy, float w, float h);
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief One palm detection result (center and size in model coordinates, e.g. 256x256).
 */
typedef struct
{
    int has_palm;   /**< 1 if palm detected, 0 otherwise. */
    float cx;       /**< Center x in model space. */
    float cy;       /**< Center y in model space. */
    float w;        /**< Box width. */
    float h;        /**< Box height. */
    uint64_t timestamp; /**< Timestamp in milliseconds. */
} palm_result_t;

/**
 * @brief Thresholds for palm tracking (dead zone to avoid jitter).
 *
 * Model frame is 256x256; center is (128, 128). Only move car/gimbal when
 * palm center is outside center ± threshold.
 */
typedef struct
{
    float center_x;       /**< Target center x (default 128). */
    float center_y;       /**< Target center y (default 128). */
    float threshold_x;   /**< Min offset in x to move car left/right (default 25). */
    float threshold_y;   /**< Min offset in y to move gimbal up/down (default 25). */
} palm_tracking_config_t;

/**
 * @brief Initialize palm event module: result queue and tracking task.
 *
 * Call once before starting detection. Tracking task behavior depends on build-time mode:
 * direct car control or coordinate packet sending.
 *
 * @param tracking_config NULL to use default thresholds; else use given center and thresholds.
 * @return BK_OK on success.
 */
bk_err_t app_event_init(const palm_tracking_config_t *tracking_config);

/**
 * @brief Get callback to pass to PalmDetectionModel::setResultCallback().
 */
palm_result_callback_t app_event_get_result_callback(void);

#ifdef __cplusplus
}
#endif
