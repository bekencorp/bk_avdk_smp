#include <stdio.h>
#include <string.h>
#include <common/bk_include.h>
#include <os/os.h>
#include <os/mem.h>
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "lv_vendor.h"
// #include "frame_buffer.h"

#define TAG "lvgl"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


static beken_thread_t g_disp_thread_handle;
static beken_mutex_t g_disp_mutex = NULL;
static beken_semaphore_t lvgl_sem = NULL;
static beken_queue_t lvgl_frame_queue = NULL;
static u8 lvgl_task_state = STATE_INIT;
static bool lv_vendor_initialized = false;


void *lv_vendor_malloc(size_t size)
{
    return hsram_malloc(size);
}

void lv_vendor_free(void *ptr)
{
    os_free(ptr);
}

#if LV_USE_LOG
static void lv_log_print(lv_log_level_t level, const char * buf)
{
    bk_printf_raw(level, NULL, buf);
}
#endif

#if CONFIG_LVGL_V9
static uint32_t lv_tick_get_callback(void)
{
    return rtos_get_time();
}
#endif

void lv_vendor_disp_lock(void)
{
    rtos_lock_mutex(&g_disp_mutex);
}

void lv_vendor_disp_unlock(void)
{
    rtos_unlock_mutex(&g_disp_mutex);
}

void *lv_vendor_get_ready_frame_buffer(void)
{
    bk_err_t ret = BK_OK;
    void *frame = NULL;
    lv_frame_msg_t msg;

    ret = rtos_pop_from_queue(&lvgl_frame_queue, &msg, BEKEN_WAIT_FOREVER);
    if (ret == BK_OK) {
        frame = (uint8_t *)msg.param0;
    }

    return frame;
}

void lv_vendor_set_ready_frame_buffer(void *frame_buffer)
{
    bk_err_t ret;
    lv_frame_msg_t msg;

    if (frame_buffer == NULL) {
        LOGE("%s frame_buffer is NULL\n", __func__);
        return;
    }

    msg.param0 = (uint32_t)frame_buffer;
    msg.param1 = 0;
    ret = rtos_push_to_queue(&lvgl_frame_queue, &msg, BEKEN_WAIT_FOREVER);
    if (ret != BK_OK) {
        LOGE("%s lvgl_frame_queue push failed\n", __func__);
        return;
    }
}

bk_err_t lv_vendor_init(lv_vnd_config_t *config)
{
    bk_err_t ret;

    if (lv_vendor_initialized) {
        LOGD("%s already init\n", __func__);
        return BK_OK;
    }

    lv_vnd_data_t *vnd_data = (lv_vnd_data_t *)lv_vendor_malloc(sizeof(lv_vnd_data_t));

    if (vnd_data == NULL) {
        LOGE("%s vnd_data malloc failed\n", __func__);
        return BK_FAIL;
    }

    os_memcpy(&vnd_data->config, config, sizeof(lv_vnd_config_t));

    ret = rtos_init_mutex(&g_disp_mutex);
    if (BK_OK != ret) {
        LOGE("%s g_disp_mutex init failed\n", __func__);
        goto fail;
    }

    ret = rtos_init_semaphore_ex(&lvgl_sem, 1, 0);
    if (BK_OK != ret) {
        LOGE("%s lvgl_sem init failed\n", __func__);
        goto fail;
    }

    ret = rtos_init_queue(&lvgl_frame_queue,
                          "lvgl_queue",
                          sizeof(lv_frame_msg_t),
                          15);
    if (ret != BK_OK) {
        LOGE("%s, init lvgl_frame_queue failed\r\n", __func__);
        goto fail;
    }

    for (int i = 0; i < CONFIG_LVGL_FRAME_BUFFER_NUM; i++) {
        vnd_data->config.frame_buffer[i] = config->frame_buffer[i];
        // lvgl_frame_buffer_init(vendor_config.frame_buffer[i]);
        lv_vendor_set_ready_frame_buffer(vnd_data->config.frame_buffer[i]);
    }

    if (config->render_mode == RENDER_PARTIAL_MODE) {
        if (config->draw_pixel_size != 0) {
            LOGW("%s !!!The draw_pixel_size is customized instead of default config\n", __func__);
            vnd_data->config.draw_pixel_size = config->draw_pixel_size;
        } else {
#if CONFIG_LVGL_V8
            vnd_data->config.draw_pixel_size = config->width * config->height / 10;
#else
            vnd_data->config.draw_pixel_size = config->width * config->height / 10 * sizeof(bk_color_t);
#endif
        }

        if (config->draw_buf_2_1 == NULL) {
#if CONFIG_LVGL_V8
            vnd_data->config.draw_buf_2_1 = lv_vendor_malloc(vnd_data->config.draw_pixel_size * sizeof(bk_color_t));
#else
#if CONFIG_LV_USE_DRAW_VG_LITE
            vnd_data->config.draw_buf_2_1 = lv_vendor_malloc(vnd_data->config.draw_pixel_size + 64);
#else
            vnd_data->config.draw_buf_2_1 = lv_vendor_malloc(vnd_data->config.draw_pixel_size);
#endif
#endif
            if (vnd_data->config.draw_buf_2_1 == NULL) {
                LOGE("%s vendor_config.draw_buf_2_1 malloc failed\n", __func__);
                goto fail;
            }
#if CONFIG_LV_USE_DRAW_VG_LITE
            vnd_data->config.draw_buf_2_1 = (void *)(((uintptr_t)vnd_data->config.draw_buf_2_1 + 63) & ~63);
#endif
        } else {
            LOGW("%s !!!The draw_buf_2_1 is customized instead of default config\n", __func__);
            vnd_data->config.draw_buf_2_1 = config->draw_buf_2_1;
        }

        if (config->draw_buf_2_2) {
            LOGW("%s !!!The draw_buf_2_2 is customized instead of default config. It usually not be used\n", __func__);
            vnd_data->config.draw_buf_2_2 = config->draw_buf_2_2;
        } else {
            vnd_data->config.draw_buf_2_2 = NULL;
        }
    } else {
#if CONFIG_LVGL_V8
        vnd_data->config.draw_pixel_size = config->width * config->height;
#else
        vnd_data->config.draw_pixel_size = config->width * config->height * sizeof(bk_color_t);
#endif
        if (config->draw_buf_2_1 || config->draw_buf_2_2) {
            LOGW("%s !!!Please do not config draw_buf_2_1 and draw_buf_2_2 in the direct mode or full mode, config is invalid!\n", __func__);
        }

        if (CONFIG_LVGL_FRAME_BUFFER_NUM < 2) {
            LOGE("%s !!!Please config at least 2 frame buffer in the direct mode or full mode, config is error\n", __func__);
            goto fail;
        }

    #if CONFIG_LV_USE_DRAW_VG_LITE
        vnd_data->config.draw_buf_2_1 = (void *)(((uintptr_t)vnd_data->config.frame_buffer[0] + 63) & ~63);
        vnd_data->config.draw_buf_2_2 = (void *)(((uintptr_t)vnd_data->config.frame_buffer[1] + 63) & ~63);
    #else
        vnd_data->config.draw_buf_2_1 = vnd_data->config.frame_buffer[0];
        vnd_data->config.draw_buf_2_2 = vnd_data->config.frame_buffer[1];
    #endif

        if (config->rotation != ROTATE_NONE) {
            LOGW("%s Direct mode and full mode don't support rotation because of very low frame rate\r\n", __func__);
        }
    }

    lv_init();

    bk_lv_port_disp_init(vnd_data);

    lv_port_indev_init();

#if LV_USE_LOG
    lv_log_register_print_cb(lv_log_print);
#endif

#if CONFIG_LVGL_V9
    lv_tick_set_cb(lv_tick_get_callback);
#endif

    lv_vendor_initialized = true;

    LOGD("%s complete\n", __func__);

    return BK_OK;

fail:
    if (g_disp_mutex) {
        rtos_deinit_mutex(&g_disp_mutex);
        g_disp_mutex = NULL;
    }

    if (lvgl_sem) {
        rtos_deinit_semaphore(&lvgl_sem);
        lvgl_sem = NULL;
    }

    if (lvgl_frame_queue) {
        rtos_deinit_queue(&lvgl_frame_queue);
        lvgl_frame_queue = NULL;
    }

    if (config->render_mode == RENDER_PARTIAL_MODE) {
        if (config->draw_buf_2_1 == NULL && vnd_data->config.draw_buf_2_1) {
            os_free(vnd_data->config.draw_buf_2_1);
            vnd_data->config.draw_buf_2_1 = NULL;
        }

        if (config->draw_buf_2_2 == NULL && vnd_data->config.draw_buf_2_2) {
            os_free(vnd_data->config.draw_buf_2_2);
            vnd_data->config.draw_buf_2_2 = NULL;
        }
    } else {
        vnd_data->config.draw_buf_2_1 = NULL;
        vnd_data->config.draw_buf_2_2 = NULL;
    }

    lv_vendor_initialized = false;

    return BK_FAIL;
}

void lv_vendor_deinit(void)
{
    bk_err_t ret;
    lv_vnd_data_t *vnd_data = (lv_vnd_data_t *)lv_display_get_user_data(lv_display_get_default());

    if (vnd_data == NULL) {
        LOGE("%s vnd_data malloc failed\n", __func__);
        return;
    }

    if (lv_vendor_initialized == false) {
        LOGD("%s already deinit\n", __func__);
        return;
    }

    lv_port_disp_deinit();

    lv_port_indev_deinit();

#if LV_USE_LOG
    lv_log_register_print_cb(NULL);
#endif

#if CONFIG_LVGL_V9
    lv_tick_set_cb(NULL);
#endif

    ret = rtos_deinit_mutex(&g_disp_mutex);
    if (BK_OK != ret) {
        LOGE("%s g_disp_mutex deinit failed\n", __func__);
        return;
    }
    g_disp_mutex = NULL;

    ret = rtos_deinit_semaphore(&lvgl_sem);
    if (BK_OK != ret) {
        LOGE("%s lvgl_sem deinit failed\n", __func__);
        return;
    }
    lvgl_sem = NULL;

    ret = rtos_deinit_queue(&lvgl_frame_queue);
    if (BK_OK != ret) {
        LOGE("%s lvgl_frame_queue deinit failed\n", __func__);
        return;
    }
    lvgl_frame_queue = NULL;

    if (vnd_data->config.render_mode == RENDER_PARTIAL_MODE) {
        if (vnd_data->config.draw_buf_2_1) {
            os_free(vnd_data->config.draw_buf_2_1);
            vnd_data->config.draw_buf_2_1 = NULL;
        }

        if (vnd_data->config.draw_buf_2_2) {
            os_free(vnd_data->config.draw_buf_2_2);
            vnd_data->config.draw_buf_2_2 = NULL;
        }
    } else {
        vnd_data->config.draw_buf_2_1 = NULL;
        vnd_data->config.draw_buf_2_2 = NULL;
    }


    lv_display_set_user_data(lv_display_get_default(), NULL);
    os_free(vnd_data);
    vnd_data = NULL;

    lv_vendor_initialized = false;

    LOGD("%s complete\n", __func__);
}

static void lv_tast_entry(void *arg)
{
    uint32_t sleep_time;

    lvgl_task_state = STATE_RUNNING;
    rtos_set_semaphore(&lvgl_sem);

    while(lvgl_task_state == STATE_RUNNING) {
        lv_vendor_disp_lock();
        sleep_time = lv_task_handler();
        lv_vendor_disp_unlock();
#if CONFIG_LVGL_TASK_SLEEP_TIME_CUSTOMIZE
        sleep_time = CONFIG_LVGL_TASK_SLEEP_TIME;
#else
        if (sleep_time > 500) {
            sleep_time = 500;
        } else if (sleep_time < 4) {
            sleep_time = 4;
        }
#endif
        rtos_delay_milliseconds(sleep_time);
    }

    rtos_set_semaphore(&lvgl_sem);

    rtos_delete_thread(NULL);
}

void lv_vendor_start(void)
{
    bk_err_t ret;

    if (lvgl_task_state == STATE_RUNNING) {
        LOGD("%s already start\n", __func__);
        return;
    }

    ret = rtos_create_sram_thread(&g_disp_thread_handle,
                             CONFIG_LVGL_TASK_PRIORITY,
                             "lvgl",
                             (beken_thread_function_t)lv_tast_entry,
                             CONFIG_LVGL_TASK_STACK_SIZE,
                             (beken_thread_arg_t)0);
    if (BK_OK != ret) {
        LOGE("%s lvgl task create failed\n", __func__);
        return;
    }

    ret = rtos_get_semaphore(&lvgl_sem, BEKEN_NEVER_TIMEOUT);
    if (BK_OK != ret) {
        LOGE("%s lvgl_sem get failed\n", __func__);
        return;
    }

    LOGD("%s complete\n", __func__);
}

void lv_vendor_stop(void)
{
    bk_err_t ret;

    if (lvgl_task_state == STATE_STOP) {
        LOGD("%s already stop\n", __func__);
        return;
    }

    lvgl_task_state = STATE_STOP;

    ret = rtos_get_semaphore(&lvgl_sem, BEKEN_NEVER_TIMEOUT);
    if (BK_OK != ret) {
        LOGE("%s lvgl_sem get failed\n", __func__);
        return;
    }

    LOGD("%s complete\n", __func__);
}
