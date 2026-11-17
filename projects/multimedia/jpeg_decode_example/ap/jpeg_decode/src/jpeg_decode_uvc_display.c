#include <os/os.h>
#include <os/mem.h>

#include <avdk_error.h>
#include <components/log.h>
#include <avdk_check.h>
#include <components/bk_frame_buffer.h>
#include <common/avdk_pixel_types.h>

#include <components/usb_types.h>
#include <components/bk_uvc_camera.h>
#include <components/usbh_hub_multiple_classes_api.h>
#include <components/bk_flexa_bond.h>

#include <components/bk_decode/bk_jpeg_decode_ctlr.h>
#include <components/bk_decode/bk_jpeg_decode_types.h>

#include <components/bk_display.h>
#include <components/bk_gpu.h>
#include <components/bk_gpu_ctlr.h>
#include <components/bk_gpu_types.h>
#include <lcd/lcd_mipi_hx8399c_1080x1920.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"
#include "cache.h"

#include "jpeg_decode_test.h"

#define TAG "jpegd_uvc_disp"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define JPEGU_UVC_PORT              (1)
#define JPEGU_UVC_WIDTH             (1920)
#define JPEGU_UVC_HEIGHT            (1080)
#define JPEGU_UVC_FPS               (30)
#define JPEGU_UVC_CONNECT_TIMEOUT   (4000)

#define JPEGU_CROP_X                (0)
#define JPEGU_CROP_Y                (0)
#define JPEGU_CROP_WIDTH            (1280)
#define JPEGU_CROP_HEIGHT           (720)

#define JPEGU_OUT_ALIGN_SIZE        (16)

#define JPEGU_FLEXA_LINES           (16)
#define JPEGU_FLEXA_SEGMENT_CNT     (3)

#define JPEGU_GUARD_ALIGN           (64U)
#define JPEGU_GUARD_PATTERN         (0x5AU)

#define JPEGU_GPU_DST_WIDTH         (1920)
#define JPEGU_GPU_DST_HEIGHT        (1080)

#define JPEGU_FRAME_COUNT           (5)
#ifdef CONFIG_UVC_FRAME_SIZE
#define JPEGU_FRAME_SIZE            (CONFIG_UVC_FRAME_SIZE)
#else
#define JPEGU_FRAME_SIZE            (1024 * 200)
#endif

#define JPEGU_DECODE_TASK_STACK     (1024 * 4)

#define JPEGU_PANEL_RESET_PIN       (GPIO_60)
#define JPEGU_PANEL_BACKLIGHT_PIN   (GPIO_7)

typedef struct {
    beken_queue_t ready_queue;
    beken_queue_t free_queue;
    beken_mutex_t mutex;
    uint8_t enable;
} jpegu_frame_que_t;

typedef struct {
    uint8_t task_running;
    uint8_t flexa_segment_cnt;
    uint16_t width;
    uint16_t height;
    uint8_t *flexa_buffer;
    uint8_t *flexa_buffer_base;
    uint32_t flexa_buffer_size;
    uint32_t guard_size;
    uint8_t guard_corrupt_reported;
    bk_jpeg_decode_ctlr_handle_t decode_handle;
    beken_semaphore_t decode_sem;
    beken_thread_t decode_thread;
} jpegu_decode_ctx_t;

typedef struct {
    uint8_t opened;
    bk_display_ctlr_handle_t dpu_handle;
    bk_display_bus_handle_t bus_handle;
    bk_avdk_lcd_panel_handle_t panel_handle;
} jpegu_display_ctx_t;

static jpegu_frame_que_t *s_jpegu_frame_que = NULL;
static jpegu_decode_ctx_t s_jpegu_decode = {0};
static jpegu_display_ctx_t s_jpegu_display = {0};
static bk_gpu_ctlr_handle_t s_jpegu_gpu = NULL;
static void *s_jpegu_gpu_bond = NULL;
static bk_uvc_ctlr_handle_t s_jpegu_uvc = NULL;
static beken_semaphore_t s_jpegu_uvc_connect_sem = NULL;
static uint8_t s_jpegu_pipeline_running = 0;

typedef struct {
    uint8_t crop;
} jpegu_param_t;

static jpegu_param_t s_jpegu_param = {1};

static void jpegu_get_crop_size(uint16_t *width, uint16_t *height)
{
    *width = s_jpegu_param.crop ? (uint16_t)JPEGU_CROP_WIDTH : (uint16_t)JPEGU_UVC_WIDTH;
    *height = s_jpegu_param.crop ? (uint16_t)JPEGU_CROP_HEIGHT : (uint16_t)JPEGU_UVC_HEIGHT;
}

static void jpegu_get_out_size(uint16_t *width, uint16_t *height)
{
    jpegu_get_crop_size(width, height);
}

static void *jpegu_hsram_aligned_malloc(uint32_t alignment, uint32_t size)
{
    void *raw = NULL;
    uintptr_t start = 0;
    uintptr_t aligned = 0;
    uint32_t total = 0;

    if (alignment < (uint32_t)sizeof(void *)) {
        alignment = (uint32_t)sizeof(void *);
    }

    if ((alignment & (alignment - 1U)) != 0U) {
        return NULL;
    }

    total = size + alignment - 1U + (uint32_t)sizeof(void *);
    raw = hsram_malloc(total);
    if (raw == NULL) {
        return NULL;
    }

    start = (uintptr_t)raw + sizeof(void *);
    aligned = (start + (alignment - 1U)) & ~((uintptr_t)alignment - 1U);
    ((void **)aligned)[-1] = raw;

    return (void *)aligned;
}

static void jpegu_hsram_aligned_free(void *ptr)
{
    if (ptr == NULL) {
        return;
    }

    os_free(((void **)ptr)[-1]);
}

static void jpegu_flexa_guard_prepare(void)
{
    if (s_jpegu_decode.flexa_buffer_base == NULL || s_jpegu_decode.guard_size == 0U) {
        return;
    }

    os_memset(s_jpegu_decode.flexa_buffer_base, JPEGU_GUARD_PATTERN, s_jpegu_decode.guard_size);
    bk_dcache_clean_for_producer(s_jpegu_decode.flexa_buffer_base, s_jpegu_decode.guard_size);
}

static void jpegu_flexa_guard_check(void)
{
    uint32_t i = 0;

    if (s_jpegu_decode.flexa_buffer_base == NULL || s_jpegu_decode.guard_size == 0U ||
        s_jpegu_decode.guard_corrupt_reported) {
        return;
    }

    bk_dcache_invalidate_for_consumer(s_jpegu_decode.flexa_buffer_base, s_jpegu_decode.guard_size);
    for (i = 0; i < s_jpegu_decode.guard_size; i++) {
        if (s_jpegu_decode.flexa_buffer_base[i] != JPEGU_GUARD_PATTERN) {
            LOGE("%s, %d, flexa decode guard row corrupted, offset:%u value:0x%02x\n",
                 __func__, __LINE__, i, s_jpegu_decode.flexa_buffer_base[i]);
            s_jpegu_decode.guard_corrupt_reported = 1;
            break;
        }
    }
}

static avdk_err_t jpegu_frame_que_init(void)
{
    avdk_err_t ret = AVDK_ERR_OK;

    if (s_jpegu_frame_que != NULL) {
        return AVDK_ERR_OK;
    }

    s_jpegu_frame_que = (jpegu_frame_que_t *)os_malloc(sizeof(jpegu_frame_que_t));
    if (s_jpegu_frame_que == NULL) {
        LOGE("%s, %d, malloc frame queue failed\n", __func__, __LINE__);
        return AVDK_ERR_NOMEM;
    }
    os_memset(s_jpegu_frame_que, 0, sizeof(jpegu_frame_que_t));

    ret = rtos_init_queue(&s_jpegu_frame_que->ready_queue, "jpegu_ready_que", sizeof(void *), JPEGU_FRAME_COUNT);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, init ready queue failed\n", __func__, __LINE__);
        return ret;
    }

    ret = rtos_init_queue(&s_jpegu_frame_que->free_queue, "jpegu_free_que", sizeof(void *), JPEGU_FRAME_COUNT);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, init free queue failed\n", __func__, __LINE__);
        return ret;
    }

    ret = rtos_init_mutex(&s_jpegu_frame_que->mutex);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, init frame queue mutex failed\n", __func__, __LINE__);
        return ret;
    }

    for (uint8_t i = 0; i < JPEGU_FRAME_COUNT; i++) {
        frame_buffer_t *frame = (frame_buffer_t *)os_malloc(sizeof(frame_buffer_t));
        uint8_t *data = NULL;

        if (frame == NULL) {
            LOGE("%s, %d, malloc frame failed\n", __func__, __LINE__);
            return AVDK_ERR_NOMEM;
        }
        os_memset(frame, 0, sizeof(frame_buffer_t));

        data = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, JPEGU_FRAME_SIZE);
        if (data == NULL) {
            LOGE("%s, %d, malloc frame data failed\n", __func__, __LINE__);
            os_free(frame);
            return AVDK_ERR_NOMEM;
        }

        frame->frame = data;
        frame->size = JPEGU_FRAME_SIZE;

        ret = rtos_push_to_queue(&s_jpegu_frame_que->free_queue, &frame, BEKEN_NO_WAIT);
        if (ret != AVDK_ERR_OK) {
            LOGE("%s, %d, push frame to free queue failed\n", __func__, __LINE__);
            bk_frame_buffer_free(data);
            os_free(frame);
            return ret;
        }
    }

    s_jpegu_frame_que->enable = 1;
    return AVDK_ERR_OK;
}

static avdk_err_t jpegu_frame_que_deinit(void)
{
    jpegu_frame_que_t *que = s_jpegu_frame_que;
    frame_buffer_t *frame = NULL;

    if (que == NULL) {
        return AVDK_ERR_OK;
    }

    que->enable = 0;

    while (!rtos_is_queue_empty(&que->free_queue)) {
        if (rtos_pop_from_queue(&que->free_queue, &frame, BEKEN_NO_WAIT) != AVDK_ERR_OK) {
            break;
        }
        if (frame->frame != NULL) {
            bk_frame_buffer_free(frame->frame);
        }
        os_free(frame);
    }

    while (!rtos_is_queue_empty(&que->ready_queue)) {
        if (rtos_pop_from_queue(&que->ready_queue, &frame, BEKEN_NO_WAIT) != AVDK_ERR_OK) {
            break;
        }
        if (frame->frame != NULL) {
            bk_frame_buffer_free(frame->frame);
        }
        os_free(frame);
    }

    rtos_deinit_queue(&que->ready_queue);
    rtos_deinit_queue(&que->free_queue);
    rtos_deinit_mutex(&que->mutex);
    os_free(que);
    s_jpegu_frame_que = NULL;

    return AVDK_ERR_OK;
}

static avdk_err_t jpegu_ready_frame_push(frame_buffer_t *frame)
{
    avdk_err_t ret = AVDK_ERR_GENERIC;
    jpegu_frame_que_t *que = s_jpegu_frame_que;

    if (que == NULL || que->enable == 0) {
        return ret;
    }

    rtos_lock_mutex(&que->mutex);
    ret = rtos_push_to_queue(&que->ready_queue, &frame, BEKEN_NO_WAIT);
    rtos_unlock_mutex(&que->mutex);

    return ret;
}

static avdk_err_t jpegu_free_frame_push(frame_buffer_t *frame)
{
    avdk_err_t ret = AVDK_ERR_GENERIC;
    jpegu_frame_que_t *que = s_jpegu_frame_que;

    if (que == NULL || que->enable == 0) {
        return ret;
    }

    rtos_lock_mutex(&que->mutex);
    ret = rtos_push_to_queue(&que->free_queue, &frame, BEKEN_NO_WAIT);
    rtos_unlock_mutex(&que->mutex);

    return ret;
}

static frame_buffer_t *jpegu_ready_frame_pop(uint32_t timeout)
{
    frame_buffer_t *frame = NULL;
    jpegu_frame_que_t *que = s_jpegu_frame_que;

    if (que == NULL || que->enable == 0) {
        return NULL;
    }

    if (rtos_pop_from_queue(&que->ready_queue, &frame, timeout) != AVDK_ERR_OK) {
        return NULL;
    }

    return frame;
}

static frame_buffer_t *jpegu_free_frame_pop(void)
{
    frame_buffer_t *frame = NULL;
    jpegu_frame_que_t *que = s_jpegu_frame_que;

    if (que == NULL || que->enable == 0) {
        return NULL;
    }

    rtos_lock_mutex(&que->mutex);
    if (rtos_pop_from_queue(&que->free_queue, &frame, BEKEN_NO_WAIT) != AVDK_ERR_OK) {
        rtos_pop_from_queue(&que->ready_queue, &frame, BEKEN_NO_WAIT);
    }
    rtos_unlock_mutex(&que->mutex);

    return frame;
}

static frame_buffer_t *jpegu_uvc_frame_malloc(bk_image_format_t format, uint32_t size)
{
    (void)format;
    (void)size;

    return jpegu_free_frame_pop();
}

static void jpegu_uvc_frame_complete(uint8_t port, bk_image_format_t format, frame_buffer_t *frame, int result)
{
    (void)port;
    (void)format;

    if (frame == NULL) {
        return;
    }

    if (result == AVDK_ERR_OK) {
        if (jpegu_ready_frame_push(frame) != AVDK_ERR_OK) {
            jpegu_free_frame_push(frame);
        }
    } else {
        jpegu_free_frame_push(frame);
    }
}

static void jpegu_uvc_state_callback(uvc_state_t state, void *user_data)
{
    LOGI("%s, state:%d, user_data:%p\n", __func__, state, user_data);
}

static const bk_uvc_callback_t s_jpegu_uvc_cbs = {
    .frame_malloc = jpegu_uvc_frame_malloc,
    .frame_complete = jpegu_uvc_frame_complete,
    .state_change_cb = jpegu_uvc_state_callback,
    .user_data = NULL,
};

static void jpegu_uvc_connect_callback(bk_usb_hub_port_info *port_info, void *arg)
{
    beken_semaphore_t sem = (beken_semaphore_t)arg;

    if (port_info == NULL) {
        LOGE("%s, port_info is NULL\n", __func__);
        return;
    }

    if (sem != NULL) {
        rtos_set_semaphore(&sem);
    }
}

static avdk_err_t jpegu_uvc_power_on(uint32_t timeout)
{
    avdk_err_t ret = AVDK_ERR_OK;
    bk_usb_hub_port_info *port_info = NULL;

    ret = rtos_init_semaphore(&s_jpegu_uvc_connect_sem, 1);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, init connect semaphore failed\n", __func__, __LINE__);
        return ret;
    }

    rtos_get_semaphore(&s_jpegu_uvc_connect_sem, BEKEN_NO_WAIT);

    for (uint8_t port = 1; port <= UVC_PORT_MAX; port++) {
        bk_usbh_hub_port_register_connect_callback(port, USB_UVC_DEVICE, jpegu_uvc_connect_callback, s_jpegu_uvc_connect_sem);
        bk_usbh_hub_multiple_devices_power_on(USB_HOST_MODE, port, USB_UVC_DEVICE);
    }

    ret = bk_usbh_hub_port_check_device(JPEGU_UVC_PORT, USB_UVC_DEVICE, &port_info);
    if (ret != AVDK_ERR_OK) {
        ret = rtos_get_semaphore(&s_jpegu_uvc_connect_sem, timeout);
    }

    return ret;
}

static avdk_err_t jpegu_uvc_power_off(void)
{
    for (uint8_t port = 1; port <= UVC_PORT_MAX; port++) {
        bk_usbh_hub_port_register_connect_callback(port, USB_UVC_DEVICE, NULL, NULL);
        bk_usbh_hub_multiple_devices_power_down(USB_HOST_MODE, port, USB_UVC_DEVICE);
    }

    if (s_jpegu_uvc_connect_sem != NULL) {
        rtos_deinit_semaphore(&s_jpegu_uvc_connect_sem);
        s_jpegu_uvc_connect_sem = NULL;
    }

    return AVDK_ERR_OK;
}

static avdk_err_t jpegu_uvc_check_mjpeg_config(bk_uvc_device_brief_info_t *device_param, bk_cam_uvc_config_t *user_config)
{
    uint8_t frame_num = device_param->all_frame.mjpeg_frame_num;
    uint8_t resolution_flag = false;
    uint8_t fps_flag = false;

    for (uint8_t index = 0; index < frame_num; index++) {
        bk_uvc_frame *frame_info = &device_param->all_frame.mjpeg_frame[index];

        LOGD("mjpeg %ux%u index:%d\n", frame_info->width, frame_info->height, frame_info->index);

        if (frame_info->width != user_config->width || frame_info->height != user_config->height) {
            continue;
        }

        resolution_flag = true;

        for (int i = 0; i < frame_info->fps_num; i++) {
            if (frame_info->fps[i] == user_config->fps) {
                fps_flag = true;
            }
        }

        if (fps_flag == false && frame_info->fps_num > 0) {
            user_config->fps = frame_info->fps[0];
            fps_flag = true;
        }

        break;
    }

    if (resolution_flag && fps_flag) {
        return AVDK_ERR_OK;
    }

    return AVDK_ERR_UNSUPPORTED;
}

static avdk_err_t jpegu_uvc_checkout_port_info(bk_cam_uvc_config_t *user_config)
{
    bk_usb_hub_port_info *port_info = NULL;
    bk_uvc_device_brief_info_t *device_param = NULL;
    avdk_err_t ret = AVDK_ERR_OK;

    ret = bk_usbh_hub_port_check_device(user_config->port, USB_UVC_DEVICE, &port_info);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, check device on port:%d failed\n", __func__, __LINE__, user_config->port);
        return ret;
    }

    device_param = (bk_uvc_device_brief_info_t *)port_info->usb_device_param;
    if (device_param == NULL) {
        LOGE("%s, %d, device param is NULL\n", __func__, __LINE__);
        return AVDK_ERR_INVAL;
    }

    LOGI("port:%d vid:0x%x pid:0x%x, format:%d %ux%u@%u\n", user_config->port,
         device_param->vendor_id, device_param->product_id,
         user_config->format, user_config->width, user_config->height, (unsigned)user_config->fps);

    return jpegu_uvc_check_mjpeg_config(device_param, user_config);
}

static bk_uvc_ctlr_handle_t jpegu_uvc_turn_on(void)
{
    bk_cam_uvc_config_t config;
    bk_uvc_ctlr_handle_t handle = NULL;
    avdk_err_t ret = AVDK_ERR_OK;

    os_memset(&config, 0, sizeof(config));
    config.port = JPEGU_UVC_PORT;
    config.width = JPEGU_UVC_WIDTH;
    config.height = JPEGU_UVC_HEIGHT;
    config.fps = JPEGU_UVC_FPS;
    config.format = BK_IMAGE_FORMAT_MJPEG;

    ret = jpegu_uvc_power_on(JPEGU_UVC_CONNECT_TIMEOUT);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, uvc power on failed\n", __func__, __LINE__);
        goto exit;
    }

    ret = jpegu_uvc_checkout_port_info(&config);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, checkout port info failed, mjpeg %ux%u@%u not supported\n", __func__, __LINE__,
             JPEGU_UVC_WIDTH, JPEGU_UVC_HEIGHT, JPEGU_UVC_FPS);
        goto exit;
    }

    ret = bk_uvc_ctrl_new(&handle, &s_jpegu_uvc_cbs);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, uvc ctrl new failed\n", __func__, __LINE__);
        goto exit;
    }

    ret = bk_uvc_init(handle);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, uvc init failed\n", __func__, __LINE__);
        goto exit;
    }

    ret = bk_uvc_open(handle, &config);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, uvc open failed\n", __func__, __LINE__);
        goto exit;
    }

    LOGI("%s uvc mjpeg %ux%u@%u ok\n", __func__, JPEGU_UVC_WIDTH, JPEGU_UVC_HEIGHT, JPEGU_UVC_FPS);
    return handle;

exit:
    if (handle != NULL) {
        (void)bk_uvc_deinit(handle);
        (void)bk_uvc_delete(handle);
    }
    (void)jpegu_uvc_power_off();
    return NULL;
}

static avdk_err_t jpegu_uvc_turn_off(bk_uvc_ctlr_handle_t handle)
{
    if (handle == NULL) {
        return AVDK_ERR_OK;
    }

    (void)bk_uvc_close(handle);
    (void)bk_uvc_deinit(handle);
    (void)bk_uvc_delete(handle);
    (void)jpegu_uvc_power_off();

    return AVDK_ERR_OK;
}

static int value_arr[128] = {0};
static int value_count = 0;
static void jpegu_decode_frame_done_cb(int status, void *args)
{
    (void)args;

    value_arr[value_count++] = -1;
    if (status != BK_OK) {
        LOGD("%s, decode failed, status=%d\n", __func__, status);
    }
}

static void jpegu_decode_flexa_done_cb(uint32_t wr_cnt, void *args)
{
    value_arr[value_count ++] = wr_cnt;
    (void)wr_cnt;
    (void)args;
}

static void jpegu_decode_thread_entry(void *arg)
{
    jpegu_decode_ctx_t *ctx = (jpegu_decode_ctx_t *)arg;
    frame_buffer_t *frame = NULL;
    bk_jpeg_decode_input_t in;

    ctx->task_running = 1;
    rtos_set_semaphore(&ctx->decode_sem);

    while (ctx->task_running) {
        frame = jpegu_ready_frame_pop(2000);
        if (frame == NULL) {
            rtos_delay_milliseconds(10);
            continue;
        }

        os_memset(&in, 0, sizeof(in));
        in.stream = frame->frame;
        in.stream_len = frame->length;
        in.out_buffer = ctx->flexa_buffer;
        in.out_buffer_size = ctx->flexa_buffer_size;

        value_count = 0;
        os_memset(value_arr, 0, 128);
        jpegu_flexa_guard_prepare();
        avdk_err_t ret = bk_jpeg_decode_frame(ctx->decode_handle, &in);
        jpegu_flexa_guard_check();
        if (ret != AVDK_ERR_OK) {
            LOGD("%s, decode failed, ret=%d\n", __func__, ret);
        }

        // for (int i = 0 ; i < value_count; i ++)
        // {
        //     LOGE("%s %d %d %d\n", __func__, __LINE__, i, value_arr[i]);
        // }
        jpegu_free_frame_push(frame);
        frame = NULL;
    }

    if (frame != NULL) {
        jpegu_free_frame_push(frame);
    }

    ctx->decode_thread = NULL;
    rtos_set_semaphore(&ctx->decode_sem);
    rtos_delete_thread(NULL);
}

static avdk_err_t jpegu_decode_open(void)
{
    bk_jpeg_decode_flexa_config_t cfg = DEFAULT_JPEG_DECODE_FLEXA_CONFIG;
    bk_jpeg_decode_pp_config_t pp_cfg;
    uint16_t crop_w = 0;
    uint16_t crop_h = 0;
    uint16_t out_w = 0;
    uint16_t out_h = 0;
    avdk_err_t ret = AVDK_ERR_OK;

    if (s_jpegu_decode.decode_handle != NULL) {
        return AVDK_ERR_OK;
    }

    jpegu_get_crop_size(&crop_w, &crop_h);
    jpegu_get_out_size(&out_w, &out_h);

    s_jpegu_decode.width = out_w;
    s_jpegu_decode.height = out_h;
    s_jpegu_decode.flexa_segment_cnt = JPEGU_FLEXA_SEGMENT_CNT;
    s_jpegu_decode.flexa_buffer_size = bk_image_size_get(out_w,
                                                         JPEGU_FLEXA_LINES * JPEGU_FLEXA_SEGMENT_CNT,
                                                         BK_PIXEL_FORMAT_NV12);
    s_jpegu_decode.guard_size = ((uint32_t)out_w + JPEGU_GUARD_ALIGN - 1U) & ~(JPEGU_GUARD_ALIGN - 1U);
    s_jpegu_decode.flexa_buffer_base = (uint8_t *)jpegu_hsram_aligned_malloc(64,
                                                                             s_jpegu_decode.flexa_buffer_size + s_jpegu_decode.guard_size);
    if (s_jpegu_decode.flexa_buffer_base == NULL) {
        LOGE("%s, %d, alloc flexa buffer %u failed\n", __func__, __LINE__,
             s_jpegu_decode.flexa_buffer_size + s_jpegu_decode.guard_size);
        return AVDK_ERR_NOMEM;
    }

    s_jpegu_decode.flexa_buffer = s_jpegu_decode.flexa_buffer_base + s_jpegu_decode.guard_size;
    s_jpegu_decode.guard_corrupt_reported = 0;
    jpegu_flexa_guard_prepare();
    LOGD("%s, %d, flexa buffer:%p, ring buffer:%p, guard:%u, size:%u\n", __func__, __LINE__,
         s_jpegu_decode.flexa_buffer_base, s_jpegu_decode.flexa_buffer,
         s_jpegu_decode.guard_size, s_jpegu_decode.flexa_buffer_size);

    cfg.out_width = out_w;
    cfg.out_height = out_h;
    cfg.out_format = BK_PIXEL_FORMAT_NV12;
    cfg.segment_height = JPEGU_FLEXA_LINES / 16;
    cfg.segment_number = JPEGU_FLEXA_SEGMENT_CNT;
    cfg.frame_done_cb = jpegu_decode_frame_done_cb;
    cfg.frame_done_args = NULL;
    cfg.flexa_done_cb = jpegu_decode_flexa_done_cb;
    cfg.flexa_done_args = NULL;

    ret = bk_jpeg_decode_flexa_ctlr_new(&s_jpegu_decode.decode_handle, &cfg);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, flexa ctlr new failed\n", __func__, __LINE__);
        goto out;
    }

    ret = bk_jpeg_decode_init(s_jpegu_decode.decode_handle);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, jpeg decode init failed\n", __func__, __LINE__);
        goto out;
    }

    ret = bk_jpeg_decode_open(s_jpegu_decode.decode_handle);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, jpeg decode open failed\n", __func__, __LINE__);
        goto out;
    }

    os_memset(&pp_cfg, 0, sizeof(pp_cfg));
    pp_cfg.out_width = out_w;
    pp_cfg.out_height = out_h;
    pp_cfg.crop_x = JPEGU_CROP_X;
    pp_cfg.crop_y = JPEGU_CROP_Y;
    pp_cfg.crop_width = crop_w;
    pp_cfg.crop_height = crop_h;

    ret = bk_jpeg_decode_ioctl(s_jpegu_decode.decode_handle, BK_JPEG_DECODE_IOCTL_SET_PP, &pp_cfg);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, set pp crop %ux%u -> %ux%u failed, ret=%d\n", __func__, __LINE__, crop_w, crop_h, out_w, out_h, ret);
        goto out;
    }

    ret = rtos_init_semaphore(&s_jpegu_decode.decode_sem, 1);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, init decode semaphore failed\n", __func__, __LINE__);
        goto out;
    }

    ret = rtos_create_thread(&s_jpegu_decode.decode_thread,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "jpegu_decode",
                             (beken_thread_function_t)jpegu_decode_thread_entry,
                             JPEGU_DECODE_TASK_STACK,
                             &s_jpegu_decode);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, create decode thread failed\n", __func__, __LINE__);
        goto out;
    }

    rtos_get_semaphore(&s_jpegu_decode.decode_sem, BEKEN_WAIT_FOREVER);

    LOGI("%s decode flexa %ux%u, pp crop %ux%u -> %ux%u, ring %u x %u lines ok\n", __func__,
         out_w, out_h, crop_w, crop_h, out_w, out_h,
         s_jpegu_decode.flexa_segment_cnt, JPEGU_FLEXA_LINES);
    return AVDK_ERR_OK;

out:
    if (s_jpegu_decode.decode_handle != NULL) {
        (void)bk_jpeg_decode_close(s_jpegu_decode.decode_handle);
        (void)bk_jpeg_decode_deinit(s_jpegu_decode.decode_handle);
        (void)bk_jpeg_decode_delete(s_jpegu_decode.decode_handle);
        s_jpegu_decode.decode_handle = NULL;
    }

    if (s_jpegu_decode.decode_sem != NULL) {
        rtos_deinit_semaphore(&s_jpegu_decode.decode_sem);
        s_jpegu_decode.decode_sem = NULL;
    }

    if (s_jpegu_decode.flexa_buffer_base != NULL) {
        jpegu_hsram_aligned_free(s_jpegu_decode.flexa_buffer_base);
        s_jpegu_decode.flexa_buffer_base = NULL;
        s_jpegu_decode.flexa_buffer = NULL;
    }

    return ret;
}

static avdk_err_t jpegu_decode_close(void)
{
    if (s_jpegu_decode.decode_handle == NULL && s_jpegu_decode.decode_sem == NULL) {
        return AVDK_ERR_OK;
    }

    s_jpegu_decode.task_running = 0;
    if (s_jpegu_decode.decode_sem != NULL && s_jpegu_decode.decode_thread != NULL) {
        rtos_get_semaphore(&s_jpegu_decode.decode_sem, BEKEN_WAIT_FOREVER);
    }

    if (s_jpegu_decode.decode_handle != NULL) {
        (void)bk_jpeg_decode_close(s_jpegu_decode.decode_handle);
        (void)bk_jpeg_decode_deinit(s_jpegu_decode.decode_handle);
        (void)bk_jpeg_decode_delete(s_jpegu_decode.decode_handle);
        s_jpegu_decode.decode_handle = NULL;
    }

    if (s_jpegu_decode.decode_sem != NULL) {
        rtos_deinit_semaphore(&s_jpegu_decode.decode_sem);
        s_jpegu_decode.decode_sem = NULL;
    }

    if (s_jpegu_decode.flexa_buffer_base != NULL) {
        jpegu_hsram_aligned_free(s_jpegu_decode.flexa_buffer_base);
        s_jpegu_decode.flexa_buffer_base = NULL;
        s_jpegu_decode.flexa_buffer = NULL;
    }

    os_memset(&s_jpegu_decode, 0, sizeof(s_jpegu_decode));

    return AVDK_ERR_OK;
}

static void jpegu_display_backlight_on(void)
{
    gpio_dev_unmap(JPEGU_PANEL_BACKLIGHT_PIN);
    BK_LOG_ON_ERR(bk_gpio_enable_output(JPEGU_PANEL_BACKLIGHT_PIN));
    BK_LOG_ON_ERR(bk_gpio_pull_up(JPEGU_PANEL_BACKLIGHT_PIN));
    bk_gpio_set_capacity(JPEGU_PANEL_BACKLIGHT_PIN, GPIO_DRIVER_CAPACITY_3);
    bk_gpio_set_output_high(JPEGU_PANEL_BACKLIGHT_PIN);
}

static avdk_err_t jpegu_display_turn_on(void)
{
    const bk_display_dsi_panel_t *panel = &lcd_device_hx8399c_mipi_1080x1920;
    bk_display_dpu_config_t dpu_cfg;
    bk_lcd_panel_config_t panel_cfg;
    avdk_err_t ret = AVDK_ERR_OK;

    if (s_jpegu_display.opened) {
        return AVDK_ERR_OK;
    }

    os_memset(&dpu_cfg, 0, sizeof(dpu_cfg));
    dpu_cfg.video.enable = true;
    dpu_cfg.video.decompress = true;
    dpu_cfg.video.format = BK_PIXEL_FORMAT_ARGB8888;

    os_memset(&panel_cfg, 0, sizeof(panel_cfg));
    panel_cfg.reset_pin = JPEGU_PANEL_RESET_PIN;

    ret = bk_display_dsi_bus_new(&s_jpegu_display.bus_handle, NULL);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, dsi bus new failed, ret=%d\n", __func__, __LINE__, ret);
        return ret;
    }

    ret = bk_lcd_mipi_panel_new(s_jpegu_display.bus_handle, &panel_cfg, panel, &s_jpegu_display.panel_handle);
    if (ret != BK_OK) {
        LOGE("%s, %d, mipi panel new failed, ret=%d\n", __func__, __LINE__, ret);
        goto error;
    }

    ret = bk_display_dpu_ctlr_new(&s_jpegu_display.dpu_handle, s_jpegu_display.panel_handle, &dpu_cfg);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, dpu ctlr new failed, ret=%d\n", __func__, __LINE__, ret);
        goto error;
    }

    ret = bk_display_init(s_jpegu_display.dpu_handle);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, display init failed, ret=%d\n", __func__, __LINE__, ret);
        goto error_deinit;
    }

    ret = bk_display_open(s_jpegu_display.dpu_handle);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, display open failed, ret=%d\n", __func__, __LINE__, ret);
        goto error_deinit;
    }

    jpegu_display_backlight_on();
    s_jpegu_display.opened = 1;

    LOGI("%s panel %s ok\n", __func__, panel->name);
    return AVDK_ERR_OK;

error_deinit:
    (void)bk_display_deinit(s_jpegu_display.dpu_handle);
    (void)bk_display_delete(s_jpegu_display.dpu_handle);
    s_jpegu_display.dpu_handle = NULL;
error:
    if (s_jpegu_display.panel_handle != NULL) {
        (void)bk_lcd_panel_delete(s_jpegu_display.panel_handle);
        s_jpegu_display.panel_handle = NULL;
    }

    if (s_jpegu_display.bus_handle != NULL) {
        (void)bk_display_bus_delete(s_jpegu_display.bus_handle);
        s_jpegu_display.bus_handle = NULL;
    }

    return ret;
}

static avdk_err_t jpegu_display_turn_off(void)
{
    if (s_jpegu_display.dpu_handle != NULL) {
        (void)bk_display_close(s_jpegu_display.dpu_handle);
        (void)bk_display_deinit(s_jpegu_display.dpu_handle);
        (void)bk_display_delete(s_jpegu_display.dpu_handle);
        s_jpegu_display.dpu_handle = NULL;
    }

    if (s_jpegu_display.panel_handle != NULL) {
        (void)bk_lcd_panel_delete(s_jpegu_display.panel_handle);
        s_jpegu_display.panel_handle = NULL;
    }

    if (s_jpegu_display.bus_handle != NULL) {
        (void)bk_display_bus_delete(s_jpegu_display.bus_handle);
        s_jpegu_display.bus_handle = NULL;
    }

    s_jpegu_display.opened = 0;

    return AVDK_ERR_OK;
}

static void *jpegu_gpu_frame_malloc(uint32_t size)
{
    void *frame = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, size);

    if (frame == NULL) {
        LOGE("%s, gpu frame malloc %u failed\n", __func__, size);
    }

    return frame;
}

static avdk_err_t jpegu_gpu_frame_free(void *ptr)
{
    if (ptr != NULL) {
        bk_frame_buffer_free(ptr);
    }

    return AVDK_ERR_OK;
}

static avdk_err_t jpegu_display_frame_release(void *args)
{
    return jpegu_gpu_frame_free(args);
}

static void jpegu_gpu_frame_done(void *frame, uint32_t frame_size, void *args)
{
    (void)frame_size;
    (void)args;

    if (!s_jpegu_display.opened || s_jpegu_display.dpu_handle == NULL) {
        (void)jpegu_gpu_frame_free(frame);
        return;
    }

    if (bk_display_flush(s_jpegu_display.dpu_handle, frame, jpegu_display_frame_release) != AVDK_ERR_OK) {
        LOGD("%s, display flush failed, drop frame\n", __func__);
        (void)jpegu_gpu_frame_free(frame);
    }
}

static avdk_err_t jpegu_gpu_open(void)
{
    bk_gpu_ctlr_config_t cfg;
    uint16_t out_w = 0;
    uint16_t out_h = 0;
    uint16_t src_h_aligned = 0;
    avdk_err_t ret = AVDK_ERR_OK;

    if (s_jpegu_gpu != NULL) {
        return AVDK_ERR_OK;
    }

    if (s_jpegu_decode.flexa_buffer == NULL) {
        LOGE("%s, %d, flexa buffer is NULL\n", __func__, __LINE__);
        return AVDK_ERR_INVAL;
    }

    jpegu_get_out_size(&out_w, &out_h);
    src_h_aligned = (uint16_t)((out_h + JPEGU_OUT_ALIGN_SIZE - 1) & ~(JPEGU_OUT_ALIGN_SIZE - 1));

    os_memset(&cfg, 0, sizeof(cfg));
    cfg.rotate_degree = 90;
    cfg.src_width = out_w;
    cfg.src_height = src_h_aligned;
    cfg.dst_width = JPEGU_GPU_DST_WIDTH;
    cfg.dst_height = JPEGU_GPU_DST_HEIGHT;
    cfg.src_format = BK_PIXEL_FORMAT_NV12;
    cfg.dst_format = BK_PIXEL_FORMAT_ARGB8888;
    cfg.compress = true;
    cfg.scale = true;
    cfg.flexa = true;
    cfg.flexa_lines = JPEGU_FLEXA_LINES;
    cfg.flexa_buff_cnt = s_jpegu_decode.flexa_segment_cnt;
    cfg.src_buffer = s_jpegu_decode.flexa_buffer;
    cfg.frame_malloc = jpegu_gpu_frame_malloc;
    cfg.frame_free = jpegu_gpu_frame_free;
    cfg.frame_done = jpegu_gpu_frame_done;
    cfg.frame_done_args = NULL;

    ret = bk_gpu_ctlr_new(&s_jpegu_gpu, &cfg);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, gpu ctlr new failed, ret=%d\n", __func__, __LINE__, ret);
        return ret;
    }

    ret = bk_gpu_init(s_jpegu_gpu);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, gpu init failed, ret=%d\n", __func__, __LINE__, ret);
        goto error;
    }

    ret = bk_gpu_open(s_jpegu_gpu);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, gpu open failed, ret=%d\n", __func__, __LINE__, ret);
        goto error;
    }

    LOGI("%s gpu nv12 %ux%u -> argb8888 %ux%u rotate 90 scale on ok\n", __func__,
         out_w, src_h_aligned, cfg.dst_width, cfg.dst_height);
    return AVDK_ERR_OK;

error:
    (void)bk_gpu_close(s_jpegu_gpu);
    (void)bk_gpu_deinit(s_jpegu_gpu);
    (void)bk_gpu_delete(s_jpegu_gpu);
    s_jpegu_gpu = NULL;

    return ret;
}

static void jpegu_gpu_close(void)
{
    if (s_jpegu_gpu != NULL) {
        (void)bk_gpu_close(s_jpegu_gpu);
        (void)bk_gpu_deinit(s_jpegu_gpu);
        (void)bk_gpu_delete(s_jpegu_gpu);
        s_jpegu_gpu = NULL;
    }
}

static void jpegu_pipeline_close(void)
{
    if (s_jpegu_uvc != NULL) {
        (void)jpegu_uvc_turn_off(s_jpegu_uvc);
        s_jpegu_uvc = NULL;
    }

    if (s_jpegu_gpu_bond != NULL) {
        bk_flexa_mjpegd_gpu_bond_stop(s_jpegu_gpu_bond);
        s_jpegu_gpu_bond = NULL;
    }

    jpegu_gpu_close();
    (void)jpegu_display_turn_off();
    (void)jpegu_decode_close();
    (void)jpegu_frame_que_deinit();

    s_jpegu_pipeline_running = 0;
}

void jpeg_decode_uvc_gpu_display_set_param(uint8_t crop)
{
    s_jpegu_param.crop = crop ? 1 : 0;

    LOGI("%s crop=%u\n", __func__, s_jpegu_param.crop);
}

void jpeg_decode_uvc_gpu_display_stop(void)
{
    if (!s_jpegu_pipeline_running) {
        LOGW("%s, pipeline not running\n", __func__);
        return;
    }

    jpegu_pipeline_close();
    LOGI("%s pipeline close ok\n", __func__);
}

void jpeg_decode_uvc_gpu_display_test(void)
{
    uint16_t crop_w = 0;
    uint16_t crop_h = 0;
    uint16_t out_w = 0;
    uint16_t out_h = 0;
    avdk_err_t ret = AVDK_ERR_OK;

    if (s_jpegu_pipeline_running) {
        LOGW("%s, pipeline already running\n", __func__);
        return;
    }

    jpegu_get_crop_size(&crop_w, &crop_h);
    jpegu_get_out_size(&out_w, &out_h);

    LOGI("%s uvc mjpeg %ux%u -> pp crop %ux%u -> out %ux%u -> gpu nv12 rotate 90 scale off\n", __func__,
         JPEGU_UVC_WIDTH, JPEGU_UVC_HEIGHT, crop_w, crop_h, out_w, out_h);

    ret = jpegu_frame_que_init();
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, frame queue init failed, ret=%d\n", __func__, ret);
        return;
    }

    ret = jpegu_decode_open();
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, decode open failed, ret=%d\n", __func__, ret);
        (void)jpegu_frame_que_deinit();
        return;
    }

    ret = jpegu_display_turn_on();
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, display open failed, ret=%d\n", __func__, ret);
        goto error;
    }

    ret = jpegu_gpu_open();
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, gpu open failed, ret=%d\n", __func__, ret);
        goto error;
    }

    ret = bk_flexa_mjpegd_gpu_bond_start(&s_jpegu_gpu_bond, s_jpegu_decode.decode_handle, s_jpegu_gpu);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, mjpegd gpu bond start failed, ret=%d\n", __func__, ret);
        goto error;
    }

    s_jpegu_uvc = jpegu_uvc_turn_on();
    if (s_jpegu_uvc == NULL) {
        LOGE("%s, uvc turn on failed\n", __func__);
        goto error;
    }

    s_jpegu_pipeline_running = 1;
    LOGI("%s pipeline open ok\n", __func__);
    return;

error:
    jpegu_pipeline_close();
}
