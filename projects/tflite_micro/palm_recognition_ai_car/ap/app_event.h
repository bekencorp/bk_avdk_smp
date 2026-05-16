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

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Stable C-level callback type the car/gimbal business layer wants to receive.
 *
 * NOTE: This is *not* the type PalmDetectionModel uses anymore. The model now
 * fires `boxDetectionCallbackT(Box *boxes, int count)` (see AvdkDetectionModel.h),
 * with `boxes[0].score` carrying `has_palm`. The C++ side (ap_main.cc) is
 * expected to install a small adapter that converts the new Box-based callback
 * into this old `(has_palm, cx, cy, w, h)` form, so app_event.c (which is
 * compiled as C and only cares about palm tracking, not the Box struct layout)
 * does not need to change.
 *
 * Defined unconditionally for both C and C++ so the same prototype is visible
 * everywhere; do NOT pull in PalmDetectionModel.h from this header (it drags
 * in TFLite C++ headers which would break C compilation of app_event.c).
 */
typedef void (*palm_result_callback_t)(int has_palm, float cx, float cy, float w, float h);

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
 * @brief Get the C-level result callback.
 *
 * The C++ caller (ap_main.cc) wraps this in a small adapter and installs it
 * via `model->setBoxDetectionCallback(...)`. See ap_main.cc for the adapter.
 */
palm_result_callback_t app_event_get_result_callback(void);

#ifdef __cplusplus
}
#endif
