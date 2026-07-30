#include <stdio.h>
#include <string.h>
#include <common/bk_include.h>
#include <os/os.h>
#include <os/mem.h>
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "lv_vendor.h"
#include "gpu_rotate/lv_gpu_rotate.h"
#include "gpu_core.h"
#if (CONFIG_VG_LITE_GPU)
#include <components/bk_gpu.h>
#endif
#include <modules/vg_lite_gpu/vg_lite.h>

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
static void *s_gpu_handle = NULL;

void lv_gpu_init(uint32_t tess_width, uint32_t tess_height);
void lv_gpu_deinit(void);

static lv_vnd_data_t *lv_vendor_get_data(void)
{
#if CONFIG_LVGL_V8
    lv_disp_t *disp = lv_disp_get_default();
    if ((disp != NULL) && (disp->driver != NULL)) {
#if LV_USE_USER_DATA
        return (lv_vnd_data_t *)disp->driver->user_data;
#endif
    }
    return NULL;
#else
    lv_display_t *disp = lv_display_get_default();
    if (disp == NULL) {
        return NULL;
    }

    return (lv_vnd_data_t *)lv_display_get_user_data(disp);
#endif
}

static bool lv_vendor_rotation_is_valid(rott_angle_t rotation)
{
    return (rotation == ROTATE_NONE) ||
           (rotation == ROTATE_90) ||
           (rotation == ROTATE_180) ||
           (rotation == ROTATE_270);
}

static bool lv_vendor_is_disp_thread(void)
{
    return (g_disp_thread_handle != NULL) &&
           rtos_is_current_thread(&g_disp_thread_handle);
}

static bk_err_t lv_vendor_prepare_rotation_resource(lv_vnd_data_t *vnd_data,
                                                    rott_angle_t rotation)
{
    if (vnd_data == NULL || rotation == ROTATE_NONE ||
        vnd_data->config.output_compress) {
        return BK_OK;
    }

    if (vnd_data->rotate_buffer == NULL) {
        vnd_data->rotate_buffer = lv_vendor_malloc(vnd_data->config.draw_pixel_size);
        if (vnd_data->rotate_buffer == NULL) {
            LOGE("%s lvgl rotate buffer malloc fail!\n", __func__);
            return BK_FAIL;
        }
        LOGI("%s allocate rotate buffer size=%u\n", __func__, vnd_data->config.draw_pixel_size);
    }

#if LV_USE_GPU_ROTATE
    if (!vnd_data->gpu_inited) {
        lv_gpu_init(0, 0);
        vnd_data->gpu_inited = true;
        LOGI("%s init GPU for dynamic rotation\n", __func__);
    }

    lv_gpu_rotate_init(vnd_data);
#endif

    return BK_OK;
}

void *lv_vendor_malloc(size_t size)
{
    return hsram_malloc(size);
}

void *lv_vendor_realloc(void *ptr, size_t size)
{
    return hsram_realloc(ptr, size);
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

#if (CONFIG_GPU)
void lv_gpu_init(uint32_t tess_width, uint32_t tess_height)
{
    bk_gpu_driver_init();

    vg_lite_init(tess_width, tess_height);
}

void lv_gpu_deinit(void)
{
    vg_lite_close();

    bk_gpu_driver_deinit();
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

void lv_vendor_gpu_handle_set(void *gpu_handle)
{
    s_gpu_handle = gpu_handle;
}

bool lv_vendor_gpu_lock(void)
{
#if (CONFIG_VG_LITE_GPU)
    bk_gpu_ctlr_handle_t gpu_handle = (bk_gpu_ctlr_handle_t)s_gpu_handle;

    if (gpu_handle == NULL) {
        return false;
    }

    if (bk_gpu_ioctl(gpu_handle, BK_GPU_IOCTL_LOCK, NULL) != AVDK_ERR_OK) {
        LOGW("%s gpu lock failed\n", __func__);
        return false;
    }

    return true;
#else
    return false;
#endif
}

void lv_vendor_gpu_unlock(bool locked)
{
#if (CONFIG_VG_LITE_GPU)
    bk_gpu_ctlr_handle_t gpu_handle = (bk_gpu_ctlr_handle_t)s_gpu_handle;

    if (!locked) {
        return;
    }

    if (gpu_handle == NULL) {
        LOGW("%s gpu handle is NULL\n", __func__);
        return;
    }

    if (bk_gpu_ioctl(gpu_handle, BK_GPU_IOCTL_UNLOCK, NULL) != AVDK_ERR_OK) {
        LOGW("%s gpu unlock failed\n", __func__);
    }
#else
    (void)locked;
#endif
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

bk_err_t lv_vendor_keypad_send_key(uint32_t key)
{
    return lv_port_keypad_send_key(key);
}

bk_err_t lv_vendor_keypad_send_key_state(uint32_t key, lv_indev_state_t state)
{
    return lv_port_keypad_send_key_state(key, state);
}

void lv_vendor_keypad_reset(void)
{
    lv_port_keypad_reset();
}

lv_indev_t *lv_vendor_keypad_get_indev(void)
{
    return lv_port_keypad_get_indev();
}

lv_group_t *lv_vendor_keypad_get_default_group(void)
{
    return lv_port_keypad_get_default_group();
}

bk_err_t lv_vendor_keypad_set_group(lv_group_t *group)
{
    return lv_port_keypad_set_group(group);
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
    os_memset(vnd_data, 0, sizeof(lv_vnd_data_t));

    if (config) {
        os_memcpy(&vnd_data->config, config, sizeof(lv_vnd_config_t));
    } else {
        LOGE("%s config is NULL\n", __func__);
        lv_vendor_free(vnd_data);
        vnd_data = NULL;
        return BK_FAIL;
    }

    vnd_data->lv_new_frame_flag = true;

#if defined(LV_USE_DRAW_VG_LITE) && LV_USE_DRAW_VG_LITE
    lv_gpu_init(vnd_data->config.width / 4, vnd_data->config.height / 4);
    vnd_data->gpu_inited = true;
#else
    if (vnd_data->config.output_compress ||
        (vnd_data->config.render_mode == RENDER_PARTIAL_MODE &&
         vnd_data->config.rotation != ROTATE_NONE &&
         !vnd_data->config.output_compress && LV_USE_GPU_ROTATE)) {
        lv_gpu_init(0, 0);
        vnd_data->gpu_inited = true;
    }
#endif

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
            vnd_data->config.draw_buf_2_1 = lv_vendor_malloc(vnd_data->config.draw_pixel_size);
#endif
            if (vnd_data->config.draw_buf_2_1 == NULL) {
                LOGE("%s vendor_config.draw_buf_2_1 malloc failed\n", __func__);
                goto fail;
            }
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

        vnd_data->config.draw_buf_2_1 = vnd_data->config.frame_buffer[0];
        vnd_data->config.draw_buf_2_2 = vnd_data->config.frame_buffer[1];

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

    if (vnd_data->gpu_inited) {
        lv_gpu_deinit();
        vnd_data->gpu_inited = false;
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

bk_err_t lv_vendor_set_dynamic_rotation(rott_angle_t rotation)
{
    if (!lv_vendor_initialized) {
        LOGW("%s lvgl vendor is not initialized\n", __func__);
        return BK_FAIL;
    }

    if (!lv_vendor_rotation_is_valid(rotation)) {
        LOGW("%s invalid rotation=%d\n", __func__, rotation);
        return BK_FAIL;
    }

    bool locked = false;
    if (!lv_vendor_is_disp_thread()) {
        lv_vendor_disp_lock();
        locked = true;
    }

#if CONFIG_LVGL_V8
    lv_disp_t *disp = lv_disp_get_default();
#else
    lv_display_t *disp = lv_display_get_default();
#endif
    if (disp == NULL) {
        LOGW("%s display is NULL\n", __func__);
        if (locked) {
            lv_vendor_disp_unlock();
        }
        return BK_FAIL;
    }

    lv_vnd_data_t *vnd_data = lv_vendor_get_data();
    if (vnd_data == NULL || vnd_data->config.render_mode != RENDER_PARTIAL_MODE) {
        LOGW("%s dynamic rotation only supports partial mode\n", __func__);
        if (locked) {
            lv_vendor_disp_unlock();
        }
        return BK_FAIL;
    }

    if (lv_vendor_prepare_rotation_resource(vnd_data, rotation) != BK_OK) {
        LOGE("%s prepare rotation resource failed, rotation=%d\n", __func__, rotation);
        if (locked) {
            lv_vendor_disp_unlock();
        }
        return BK_FAIL;
    }

    vnd_data->config.rotation = rotation;
#if CONFIG_LVGL_V8
    lv_disp_set_rotation(disp, (lv_disp_rot_t)rotation);
    lv_obj_invalidate(lv_scr_act());
#else
    lv_display_set_rotation(disp, (lv_display_rotation_t)rotation);
    lv_obj_invalidate(lv_screen_active());
#endif
    LOGI("%s set rotation=%d\n", __func__, rotation);

    if (locked) {
        lv_vendor_disp_unlock();
    }

    return BK_OK;
}

rott_angle_t lv_vendor_get_rotation(void)
{
    lv_vnd_data_t *vnd_data = lv_vendor_get_data();
    if (vnd_data == NULL) {
        return ROTATE_NONE;
    }

    return vnd_data->config.rotation;
}

void lv_vendor_deinit(void)
{
    bk_err_t ret;
    lv_vnd_data_t *vnd_data = NULL;

    if (lv_vendor_initialized == false) {
        LOGD("%s already deinit\n", __func__);
        return;
    }

#if CONFIG_LVGL_V8
    lv_disp_t *disp = lv_disp_get_default();
    if ((disp != NULL) && (disp->driver != NULL)) {
#if LV_USE_USER_DATA
        vnd_data = (lv_vnd_data_t *)disp->driver->user_data;
#endif
    }
#else
    vnd_data = (lv_vnd_data_t *)lv_display_get_user_data(lv_display_get_default());
#endif

    if (vnd_data == NULL) {
        LOGE("%s vnd_data is NULL\n", __func__);
        return;
    }

    lv_port_disp_deinit(vnd_data);

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

    if (vnd_data->gpu_inited) {
        lv_gpu_deinit();
        vnd_data->gpu_inited = false;
    }

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

#if CONFIG_LVGL_V8
    disp = lv_disp_get_default();
    if ((disp != NULL) && (disp->driver != NULL)) {
#if LV_USE_USER_DATA
        disp->driver->user_data = NULL;
#endif
    }
#else
    lv_display_t *disp = lv_display_get_default();
    if (disp != NULL) {
        lv_display_set_user_data(disp, NULL);
    }
#endif
    os_free(vnd_data);
    vnd_data = NULL;

    lv_vendor_initialized = false;

    LOGD("%s complete\n", __func__);
}

static void lv_task_entry(void *arg)
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

    ret = rtos_create_hsram_thread(&g_disp_thread_handle,
                             CONFIG_LVGL_TASK_PRIORITY,
                             "lvgl",
                             (beken_thread_function_t)lv_task_entry,
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
