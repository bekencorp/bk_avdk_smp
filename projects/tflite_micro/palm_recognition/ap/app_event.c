/**
 * @file app_event.c
 * @brief Palm event module: result queue, callback for model, and tracking task.
 *
 * The tracking task reads palm results and drives car left/right, forward/backward,
 * and gimbal up/down to keep the palm near the center of the frame with appropriate size.
 * Thresholds define a dead zone to avoid frequent small movements.
 */

#include <common/bk_include.h>
#include <os/os.h>
#include "app_event.h"
#include "car_control.h"

#if CONFIG_AON_RTC || CONFIG_ANA_RTC
#include <driver/aon_rtc.h>
#include <driver/aon_rtc_types.h>
#endif

#define TAG "app-event"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define PALM_RESULT_QUEUE_LEN   (60)
#define PALM_RESULT_ITEM_SIZE   (sizeof(palm_result_t))

/** Default center and thresholds (model space 256x256). */
#define DEFAULT_CENTER_X       (128.f)
#define DEFAULT_CENTER_Y       (128.f)
/* Base horizontal threshold. Right move will be slightly more sensitive. */
#define DEFAULT_THRESHOLD_X    (25.f)
/* Vertical threshold: larger dead zone to reduce gimbal movement. */
#define DEFAULT_THRESHOLD_Y    (35.f)
/* Palm box area thresholds: min area triggers forward, max area triggers backward. */
#define DEFAULT_MIN_AREA       (850.f)   /* Min area (w * h) to trigger forward movement */
#define DEFAULT_MAX_AREA       (3700.f)  /* Max area (w * h) to trigger backward movement */

/** Delay (ms) after sending a car/gimbal command. */
#define COMMAND_INTERVAL_MS    (80)

static beken_queue_t s_result_queue = NULL;
static beken_thread_t s_tracking_task = NULL;
static palm_tracking_config_t s_tracking_config;

/**
 * @brief Callback registered with PalmDetectionModel; pushes result to queue.
 */
static void app_event_result_cb(int has_palm, float cx, float cy, float w, float h)
{
    if (s_result_queue == NULL)
    {
        return;
    }

    palm_result_t result = {
        .has_palm = has_palm,
        .cx = cx,
        .cy = cy,
        .w = w,
        .h = h,
    };

    bk_err_t ret = rtos_push_to_queue(&s_result_queue, &result, BEKEN_NO_WAIT);
    if (ret != BK_OK)
    {
        LOGW("palm result queue push failed, ret=%d\n", ret);
    }
}

/**
 * @brief Apply one tracking result: move car left/right, forward/backward, and gimbal up/down
 *        when palm is outside dead zone so that palm stays near center with appropriate size.
 */
static void app_event_apply_tracking(const palm_result_t *result)
{
    if (result == NULL || !result->has_palm)
    {
        return;
    }

    float dx = result->cx - s_tracking_config.center_x;
    float dy = result->cy - s_tracking_config.center_y;
    float palm_area = result->w * result->h;

    /* Car left/right: palm left of center -> move car right; palm right -> move car left. */
    if (dx < -s_tracking_config.threshold_x)
    {
        if (car_control_move_right() == BK_OK)
        {
            LOGI("car_control_move_right\n");
            rtos_delay_milliseconds(COMMAND_INTERVAL_MS);
        }
    }
    else if (dx > s_tracking_config.threshold_x)
    {
        if (car_control_move_left() == BK_OK)
        {
            LOGI("car_control_move_left\n");
            rtos_delay_milliseconds(COMMAND_INTERVAL_MS);
        }
    }

    /* Gimbal up/down: palm above center -> gimbal up; palm below -> gimbal down.
     * Use asymmetric thresholds: make gimbal up a bit more sensitive than down.
     */
    float up_th = s_tracking_config.threshold_y * 0.7f;   /* e.g. 35 -> 24.5 */
    float down_th = s_tracking_config.threshold_y;

    if (dy < -up_th)
    {
        if (car_control_gimbal_up() == BK_OK)
        {
            LOGI("car_control_gimbal_up\n");
            rtos_delay_milliseconds(COMMAND_INTERVAL_MS);
        }
    }
    else if (dy > down_th)
    {
        if (car_control_gimbal_down() == BK_OK)
        {
            LOGI("car_control_gimbal_down\n");
            rtos_delay_milliseconds(COMMAND_INTERVAL_MS);
        }
    }

    /* Car forward/backward: control based on palm box area.
     * If area is too small, move forward to get closer.
     * If area is too large, move backward to get farther.
     */
    if (palm_area < s_tracking_config.min_area)
    {
        if (car_control_move_forward() == BK_OK)
        {
            LOGI("car_control_move_forward, area=%.0f < min=%.0f\n",
                 (double)palm_area, (double)s_tracking_config.min_area);
            rtos_delay_milliseconds(COMMAND_INTERVAL_MS);
        }
    }
    else if (palm_area > s_tracking_config.max_area)
    {
        if (car_control_move_backward() == BK_OK)
        {
            LOGI("car_control_move_backward, area=%.0f > max=%.0f\n",
                 (double)palm_area, (double)s_tracking_config.max_area);
            rtos_delay_milliseconds(COMMAND_INTERVAL_MS);
        }
    }
}

/**
 * @brief Tracking task: read palm results from queue and drive car/gimbal.
 */
static void app_event_tracking_task(beken_thread_arg_t arg)
{
    palm_result_t result;

    (void)arg;

    while (s_result_queue != NULL)
    {
        bk_err_t ret = rtos_pop_from_queue(&s_result_queue, &result, BEKEN_WAIT_FOREVER);
        if (ret == BK_OK)
        {
            app_event_apply_tracking(&result);
        }
    }

    s_tracking_task = NULL;
}

bk_err_t app_event_init(const palm_tracking_config_t *tracking_config)
{
    if (s_result_queue != NULL)
    {
        LOGW("app_event already inited\n");
        return BK_OK;
    }

    if (tracking_config != NULL)
    {
        s_tracking_config.center_x = tracking_config->center_x;
        s_tracking_config.center_y = tracking_config->center_y;
        s_tracking_config.threshold_x = tracking_config->threshold_x;
        s_tracking_config.threshold_y = tracking_config->threshold_y;
        s_tracking_config.min_area = tracking_config->min_area;
        s_tracking_config.max_area = tracking_config->max_area;
    }
    else
    {
        s_tracking_config.center_x = DEFAULT_CENTER_X;
        s_tracking_config.center_y = DEFAULT_CENTER_Y;
        s_tracking_config.threshold_x = DEFAULT_THRESHOLD_X;
        s_tracking_config.threshold_y = DEFAULT_THRESHOLD_Y;
        s_tracking_config.min_area = DEFAULT_MIN_AREA;
        s_tracking_config.max_area = DEFAULT_MAX_AREA;
    }

    bk_err_t ret = rtos_init_queue(&s_result_queue,
                                  "palm_result",
                                  PALM_RESULT_ITEM_SIZE,
                                  PALM_RESULT_QUEUE_LEN);
    if (ret != BK_OK)
    {
        LOGE("rtos_init_queue palm_result failed %d\n", ret);
        return ret;
    }

    ret = rtos_create_thread(&s_tracking_task,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "palm_track",
                             (beken_thread_function_t)app_event_tracking_task,
                             2048,
                             NULL);
    if (ret != BK_OK)
    {
        LOGE("rtos_create_thread palm_track failed %d\n", ret);
        rtos_deinit_queue(&s_result_queue);
        s_result_queue = NULL;
        return ret;
    }

    LOGI("app_event inited, center=(%.0f,%.0f) threshold=(%.0f,%.0f) area_range=(%.0f,%.0f)\n",
         (double)s_tracking_config.center_x, (double)s_tracking_config.center_y,
         (double)s_tracking_config.threshold_x, (double)s_tracking_config.threshold_y,
         (double)s_tracking_config.min_area, (double)s_tracking_config.max_area);
    return BK_OK;
}

palm_result_callback_t app_event_get_result_callback(void)
{
    return app_event_result_cb;
}
