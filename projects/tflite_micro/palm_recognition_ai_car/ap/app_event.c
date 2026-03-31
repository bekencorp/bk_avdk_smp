/**
 * @file app_event.c
 * @brief Palm event module: result queue, callback for model, and tracking task.
 *
 * The tracking task reads palm results and drives car left/right and gimbal up/down
 * to keep the palm near the center of the frame. Thresholds define a dead zone to
 * avoid frequent small movements.
 */

#include <common/bk_include.h>
#include <os/os.h>
#include <os/mem.h>
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

/** Delay (ms) after sending a car/gimbal command. */
#define COMMAND_INTERVAL_MS    (80)

/* Palm model output space is 256x256. */
#define PALM_MODEL_IMAGE_WIDTH   (256U)
#define PALM_MODEL_IMAGE_HEIGHT  (256U)

/* UART coordinate packet format:
 * 03 + UID(12B) + CMD(0x51) + LEN(0x000c) + 6*uint16 + CRC16(Modbus, little-endian).
 */
#define COORD_FRAME_START_BYTE      (0x03U)
#define COORD_FRAME_UID_LEN         (12U)
#define COORD_FRAME_CMD_COORD       (0x51U)
#define COORD_FRAME_DATA_LEN        (12U)
#define COORD_FRAME_FIXED_LEN       (1U + COORD_FRAME_UID_LEN + 1U + 2U + COORD_FRAME_DATA_LEN + 2U)

static const uint8_t s_coord_frame_uid[COORD_FRAME_UID_LEN] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static beken_queue_t s_result_queue = NULL;
static beken_thread_t s_tracking_task = NULL;
static palm_tracking_config_t s_tracking_config;

static uint16_t app_crc16_modbus(const uint8_t *data, uint32_t len)
{
    uint16_t crc = 0xFFFF;
    uint32_t i;
    uint32_t j;

    if (data == NULL || len == 0)
    {
        return crc;
    }

    for (i = 0; i < len; ++i)
    {
        crc ^= data[i];
        for (j = 0; j < 8; ++j)
        {
            if ((crc & 0x0001U) != 0U)
            {
                crc = (uint16_t)((crc >> 1) ^ 0xA001U);
            }
            else
            {
                crc = (uint16_t)(crc >> 1);
            }
        }
    }

    return crc;
}

static uint16_t app_clamp_u16(int32_t value, uint16_t min_v, uint16_t max_v)
{
    if (value < (int32_t)min_v)
    {
        return min_v;
    }
    if (value > (int32_t)max_v)
    {
        return max_v;
    }
    return (uint16_t)value;
}

static void app_write_u16_be(uint8_t *buf, uint16_t value)
{
    if (buf == NULL)
    {
        return;
    }
    buf[0] = (uint8_t)((value >> 8) & 0xFFU);
    buf[1] = (uint8_t)(value & 0xFFU);
}

static bk_err_t app_event_send_coord_frame(const palm_result_t *result)
{
    uint8_t frame[COORD_FRAME_FIXED_LEN];
    uint32_t offset = 0;
    uint16_t width = PALM_MODEL_IMAGE_WIDTH;
    uint16_t height = PALM_MODEL_IMAGE_HEIGHT;
    int32_t x1_i;
    int32_t y1_i;
    int32_t x2_i;
    int32_t y2_i;
    uint16_t x1;
    uint16_t y1;
    uint16_t x2;
    uint16_t y2;
    uint16_t crc;
    bk_err_t ret;

    if (result == NULL || result->has_palm == 0)
    {
        return BK_ERR_PARAM;
    }

    x1_i = (int32_t)(result->cx - (result->w * 0.5f));
    y1_i = (int32_t)(result->cy - (result->h * 0.5f));
    x2_i = (int32_t)(result->cx + (result->w * 0.5f));
    y2_i = (int32_t)(result->cy + (result->h * 0.5f));

    x1 = app_clamp_u16(x1_i, 0U, (uint16_t)(width - 1U));
    y1 = app_clamp_u16(y1_i, 0U, (uint16_t)(height - 1U));
    x2 = app_clamp_u16(x2_i, 0U, (uint16_t)(width - 1U));
    y2 = app_clamp_u16(y2_i, 0U, (uint16_t)(height - 1U));

    frame[offset++] = COORD_FRAME_START_BYTE;
    os_memcpy(&frame[offset], s_coord_frame_uid, COORD_FRAME_UID_LEN);
    offset += COORD_FRAME_UID_LEN;
    frame[offset++] = COORD_FRAME_CMD_COORD;
    app_write_u16_be(&frame[offset], COORD_FRAME_DATA_LEN);
    offset += 2U;
    app_write_u16_be(&frame[offset], width);
    offset += 2U;
    app_write_u16_be(&frame[offset], height);
    offset += 2U;
    app_write_u16_be(&frame[offset], x1);
    offset += 2U;
    app_write_u16_be(&frame[offset], y1);
    offset += 2U;
    app_write_u16_be(&frame[offset], x2);
    offset += 2U;
    app_write_u16_be(&frame[offset], y2);
    offset += 2U;

    crc = app_crc16_modbus(frame, offset);
    frame[offset++] = (uint8_t)(crc & 0xFFU);
    frame[offset++] = (uint8_t)((crc >> 8) & 0xFFU);

    if (offset != COORD_FRAME_FIXED_LEN)
    {
        LOGE("coord frame length mismatch, actual=%u expected=%u\n",
             (unsigned int)offset, (unsigned int)COORD_FRAME_FIXED_LEN);
        return BK_FAIL;
    }

    ret = car_control_send_raw(frame, offset);
    if (ret != BK_OK)
    {
        LOGE("send coord frame failed, ret=%d\n", ret);
        return ret;
    }

    LOGI("coord frame sent: img=%ux%u box=(%u,%u)-(%u,%u)\n",
         (unsigned int)width, (unsigned int)height,
         (unsigned int)x1, (unsigned int)y1, (unsigned int)x2, (unsigned int)y2);
    return BK_OK;
}

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
 * @brief Apply one tracking result: move car left/right and gimbal up/down
 *        when palm is outside dead zone so that palm stays near center.
 */
static void app_event_apply_tracking(const palm_result_t *result)
{
    if (result == NULL || !result->has_palm)
    {
        return;
    }

#if defined(CONFIG_PALM_AI_CAR_CONTROL_MODE_SEND_COORD)
    {
        bk_err_t ret = app_event_send_coord_frame(result);
        if (ret != BK_OK)
        {
            LOGW("app_event_send_coord_frame failed, ret=%d\n", ret);
        }
        return;
    }
#endif

    float dx = result->cx - s_tracking_config.center_x;
    float dy = result->cy - s_tracking_config.center_y;

    /* Car left/right: palm left of center -> move car left; palm right -> move right. */
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
    float down_th = s_tracking_config.threshold_y * 0.7f;   /* e.g. 35 -> 24.5 */
    float up_th = s_tracking_config.threshold_y;

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
    }
    else
    {
        s_tracking_config.center_x = DEFAULT_CENTER_X;
        s_tracking_config.center_y = DEFAULT_CENTER_Y;
        s_tracking_config.threshold_x = DEFAULT_THRESHOLD_X;
        s_tracking_config.threshold_y = DEFAULT_THRESHOLD_Y;
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

    LOGI("app_event inited, center=(%.0f,%.0f) threshold=(%.0f,%.0f)\n",
         (double)s_tracking_config.center_x, (double)s_tracking_config.center_y,
         (double)s_tracking_config.threshold_x, (double)s_tracking_config.threshold_y);
    return BK_OK;
}

palm_result_callback_t app_event_get_result_callback(void)
{
    return app_event_result_cb;
}
