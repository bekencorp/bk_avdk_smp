#include "include/isp_vc_route.h"
#include "include/isp_display_pipeline.h"

#include <os/mem.h>
#include <os/os.h>
#include <avdk_check.h>
#include <cache.h>
#include <common/avdk_pixel_types.h>
#include <components/bk_camera_sensor.h>
#include <components/bk_camera_bus.h>
#include <components/bk_camera_configs.h>
#include <components/bk_isp_camera.h>
#include <components/bk_camera_isp_ctlr.h>
#include <isp_camera_ctlr.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include <driver/isp_base.h>
#include <driver/mipi_csi.h>
#include <modules/pm.h>
#include <components/log.h>

#define TAG "isp_vc"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define VC_ROUTE_PIN_SCL    GPIO_69
#define VC_ROUTE_PIN_SDA    GPIO_70
#define VC_ROUTE_PIN_RESET  GPIO_71
#define VC_ROUTE_I2C_ID     1
#define VC_ROUTE_CAMERA_OPEN_RETRY 3

typedef struct {
    volatile uint8_t running;
    volatile uint8_t gpu_in_use;
    uint8_t display_vc;
    uint16_t width;
    uint16_t height;
    uint32_t frame_size;
    uint32_t last_seq[2];
    bk_camera_bus_t *bus;
    bk_camera_sensor_handle_t sensor_handle;
    bk_isp_camera_ctlr_handle_t camera_ctlr_handle;
    bk_isp_camera_vc_mux_handle_t vc_mux_handle;
    bk_isp_camera_vc_mux_frame_ref_t active_frame;
    beken_thread_t thread;
    beken_semaphore_t exit_sem;
} isp_vc_route_ctx_t;

static isp_vc_route_ctx_t *s_vc_route = NULL;

static avdk_err_t vc_route_camera_power(bool enable)
{
    int ldo_en = enable ? PM_AUXLDO_ENABLE : PM_AUXLDO_DISABLE;
    pm_auxldo_ctrl_cfg_t cfg = {0};

    cfg.ldo = AUXLDOS_SEL_1P8V;
    cfg.out = PM_AUXLDO_1P8V_OUT_1P8V;
    cfg.user = PM_AUXLDO_USER_CAMERA;
    cfg.state = ldo_en;
    AVDK_RETURN_ON_ERROR(bk_pm_auxldo_ctrl_vote(&cfg), TAG, "camera 1p8v ldo vote failed");

    cfg = (pm_auxldo_ctrl_cfg_t){0};
    cfg.ldo = AUXLDOS_SEL_1P2V;
    cfg.out = PM_AUXLDO_1P2V_OUT_1P2V;
    cfg.user = PM_AUXLDO_USER_CAMERA;
    cfg.state = ldo_en;
    AVDK_RETURN_ON_ERROR(bk_pm_auxldo_ctrl_vote(&cfg), TAG, "camera 1p2v ldo vote failed");
    rtos_delay_milliseconds(1);
    return AVDK_ERR_OK;
}

static void vc_route_display_frame_free(void *frame, void *args)
{
    isp_vc_route_ctx_t *ctx = (isp_vc_route_ctx_t *)args;

    (void)frame;
    if (ctx == NULL) {
        return;
    }
    if (ctx->active_frame.frame != NULL && ctx->vc_mux_handle != NULL) {
        (void)bk_isp_camera_vc_mux_release(ctx->vc_mux_handle, &ctx->active_frame);
        os_memset(&ctx->active_frame, 0, sizeof(ctx->active_frame));
    }
    ctx->gpu_in_use = 0;
}

static void vc_route_task_entry(void *arg)
{
    isp_vc_route_ctx_t *ctx = (isp_vc_route_ctx_t *)arg;

    while (ctx != NULL && ctx->running) {
        if (isp_display_lcd_is_open() && !ctx->gpu_in_use && !isp_display_gpu_frame_busy()
            && ctx->vc_mux_handle != NULL) {
            uint8_t display_vc = ctx->display_vc & 0x01;
            bk_isp_camera_vc_mux_frame_ref_t frame = { .vc = display_vc };
            avdk_err_t frame_ret = bk_isp_camera_vc_mux_peek(ctx->vc_mux_handle, &frame);
            uint8_t submitted = 0;

            if (frame_ret == AVDK_ERR_OK
                && frame.frame_size >= ctx->frame_size
                && frame.sequence != ctx->last_seq[display_vc]) {
                ctx->last_seq[display_vc] = frame.sequence;
                ctx->active_frame = frame;
                ctx->gpu_in_use = 1;

                arch_dcache_flush_and_invd_range(frame.frame, ctx->frame_size);

                avdk_err_t gpu_ret = isp_display_gpu_frame_process(frame.frame, frame.sequence,
                                                                 vc_route_display_frame_free, ctx);
                if (gpu_ret != AVDK_ERR_OK) {
                    ctx->gpu_in_use = 0;
                    if (ctx->active_frame.frame == NULL) {
                        submitted = 1;
                    } else {
                        os_memset(&ctx->active_frame, 0, sizeof(ctx->active_frame));
                    }
                } else {
                    submitted = 1;
                }
            }

            if (!submitted && frame.frame != NULL) {
                (void)bk_isp_camera_vc_mux_release(ctx->vc_mux_handle, &frame);
            }
        }
        rtos_delay_milliseconds(10);
    }

    if (ctx != NULL) {
        ctx->thread = NULL;
        if (ctx->exit_sem != NULL) {
            rtos_set_semaphore(&ctx->exit_sem);
        }
    }
    rtos_delete_thread(NULL);
}

static avdk_err_t vc_route_camera_open_once(isp_vc_route_ctx_t *ctx, uint16_t w, uint16_t h, uint16_t fps)
{
    avdk_err_t ret;
    bk_camera_bus_config_t bus_config = (bk_camera_bus_config_t)CSI_CAM_BUS_I2C1_8BIT_2000TIMEOUT();
    bk_camera_sensor_config_t sensor_config = {
        .pin_reset = VC_ROUTE_PIN_RESET,
        .pin_pwdn = 0xFF,
    };

    bus_config.pin_scl = VC_ROUTE_PIN_SCL;
    bus_config.pin_sda = VC_ROUTE_PIN_SDA;
    bus_config.i2c_id = VC_ROUTE_I2C_ID;
    bus_config.pin_xclk = BK_CAMERA_PIN_INVALID;

    AVDK_RETURN_ON_ERROR(vc_route_camera_power(true), TAG, "camera power on failed");

    ctx->bus = bk_camera_bus_new(&bus_config);
    AVDK_RETURN_ON_FALSE(ctx->bus, AVDK_ERR_GENERIC, TAG, "bus new failed");

    ret = bk_camera_bus_enable(ctx->bus);
    if (ret != AVDK_ERR_OK) {
        goto err;
    }

    sensor_config.bus = ctx->bus;
    ctx->sensor_handle = bk_camera_sensor_auto_detect(&sensor_config, CSI_CAMERA_PORT);
    if (ctx->sensor_handle == NULL) {
        ret = AVDK_ERR_NODEV;
        goto err;
    }

    bk_isp_camera_ctlr_config_t isp_cfg = CAM_CSI_DEFAULT_RAW10_CONFIG(w, h, fps);
    {
        bk_camera_sensor_format_array_t fmt_arr = {0};
        if (bk_camera_sensor_query_support_formats(ctx->sensor_handle, &fmt_arr) == AVDK_ERR_OK
            && fmt_arr.size > 0) {
            uint32_t i;
            isp_cfg.input_pixel_fmt = fmt_arr.format_array[0].output_pixel_fmt;
            for (i = 0; i < fmt_arr.size; i++) {
                if (fmt_arr.format_array[i].width == w
                    && fmt_arr.format_array[i].height == h
                    && fmt_arr.format_array[i].fps == fps) {
                    isp_cfg.input_pixel_fmt = fmt_arr.format_array[i].output_pixel_fmt;
                    break;
                }
            }
        }
    }

    const void *sensor_object = bk_camera_sensor_get_sensor_object(ctx->sensor_handle);
    if (sensor_object == NULL) {
        ret = AVDK_ERR_GENERIC;
        goto err;
    }
    isp_cfg.sensor_object = sensor_object;

    /* doorbell: sensor stream before ISP port init */
    ret = bk_camera_sensor_init(ctx->sensor_handle);
    if (ret != AVDK_ERR_OK) {
        goto err;
    }
    bk_camera_sensor_format_t fmt = { .width = w, .height = h, .fps = fps };
    ret = bk_camera_sensor_set_format(ctx->sensor_handle, &fmt);
    if (ret != AVDK_ERR_OK) {
        goto err;
    }

    ret = bk_camera_isp_ctlr_new(&ctx->camera_ctlr_handle);
    if (ret != AVDK_ERR_OK) {
        goto err;
    }
    ret = bk_isp_camera_dev_init(ctx->camera_ctlr_handle);
    if (ret != AVDK_ERR_OK) {
        goto err;
    }
    ret = bk_isp_camera_port_init(ctx->camera_ctlr_handle, &isp_cfg);
    if (ret != AVDK_ERR_OK) {
        goto err;
    }

    bk_isp_camera_channel_config_t inst = CAM_MP_NV12_RB_INSTANCE_CONFIG(w, h);
    inst.port_id = ISP_MP_CHN_ID;
    inst.enable_flexa = 0;
    inst.work_mode = 0;
    inst.format = BK_PIXEL_FORMAT_NV12;
    inst.buf_cnt = 7;

    ret = bk_isp_camera_channel_open(ctx->camera_ctlr_handle, ISP_MP_CHN_ID, &inst);
    if (ret != AVDK_ERR_OK) {
        goto err;
    }

    LOGI("vc route camera open ok: %ux%u@%u buf_cnt=7\n", w, h, fps);
    return AVDK_ERR_OK;

err:
    if (ctx->camera_ctlr_handle) {
        (void)bk_isp_camera_deinit(ctx->camera_ctlr_handle);
        (void)bk_isp_camera_delete(ctx->camera_ctlr_handle);
        ctx->camera_ctlr_handle = NULL;
    }
    if (ctx->sensor_handle) {
        bk_camera_sensor_destroy(ctx->sensor_handle);
        ctx->sensor_handle = NULL;
    }
    if (ctx->bus) {
        bk_camera_bus_disable(ctx->bus);
        bk_camera_bus_delete(ctx->bus);
        ctx->bus = NULL;
    }
    (void)vc_route_camera_power(false);
    return ret;
}

static avdk_err_t vc_route_camera_open(isp_vc_route_ctx_t *ctx, uint16_t w, uint16_t h, uint16_t fps)
{
    avdk_err_t ret = AVDK_ERR_GENERIC;

    for (int att = 1; att <= VC_ROUTE_CAMERA_OPEN_RETRY; att++) {
        ret = vc_route_camera_open_once(ctx, w, h, fps);
        if (ret == AVDK_ERR_OK) {
            if (att > 1) {
                LOGW("vc route camera open ok after full retry att=%d\n", att);
            }
            return AVDK_ERR_OK;
        }

        LOGW("vc route camera open failed att=%d/%d ret=%d, retry full sensor/csi open\n",
             att, VC_ROUTE_CAMERA_OPEN_RETRY, ret);
        rtos_delay_milliseconds(30);
    }

    return ret;
}

static void vc_route_camera_close(isp_vc_route_ctx_t *ctx)
{
    if (ctx == NULL) {
        return;
    }
    if (ctx->camera_ctlr_handle) {
        if (bk_isp_camera_channel_state_get(ctx->camera_ctlr_handle, ISP_MP_CHN_ID) == ISP_CHANNEL_STATE_TURN_ON) {
            (void)bk_isp_camera_channel_close(ctx->camera_ctlr_handle, ISP_MP_CHN_ID);
        }
        (void)bk_isp_camera_deinit(ctx->camera_ctlr_handle);
        (void)bk_isp_camera_delete(ctx->camera_ctlr_handle);
        ctx->camera_ctlr_handle = NULL;
    }
    if (ctx->sensor_handle) {
        bk_camera_sensor_destroy(ctx->sensor_handle);
        ctx->sensor_handle = NULL;
    }
    if (ctx->bus) {
        bk_camera_bus_disable(ctx->bus);
        bk_camera_bus_delete(ctx->bus);
        ctx->bus = NULL;
    }
    (void)vc_route_camera_power(false);
}

bool isp_vc_route_is_active(void)
{
    return s_vc_route != NULL;
}

avdk_err_t isp_vc_route_turn_on(uint16_t sensor_w, uint16_t sensor_h, uint16_t fps,
                                uint16_t isp_w, uint16_t isp_h, uint8_t default_vc)
{
    avdk_err_t ret;
    isp_vc_route_ctx_t *ctx = NULL;

    AVDK_RETURN_ON_FALSE(!isp_vc_route_is_active(), AVDK_ERR_BUSY, TAG, "vc route already on");
    AVDK_RETURN_ON_FALSE(default_vc < 2, AVDK_ERR_INVAL, TAG, "default vc invalid");

    if (isp_w == 0) {
        isp_w = sensor_w;
    }
    if (isp_h == 0) {
        isp_h = sensor_h;
    }

    ctx = os_malloc(sizeof(*ctx));
    AVDK_RETURN_ON_FALSE(ctx, AVDK_ERR_NOMEM, TAG, "ctx malloc failed");
    os_memset(ctx, 0, sizeof(*ctx));
    ctx->width = isp_w;
    ctx->height = isp_h;
    ctx->frame_size = (uint32_t)isp_w * (uint32_t)isp_h * 3U / 2U;
    ctx->display_vc = default_vc;

    ret = rtos_init_semaphore(&ctx->exit_sem, 1);
    if (ret != BK_OK) {
        goto err_free;
    }

    bk_mipi_csi_set_default_vc(0);

    ret = vc_route_camera_open(ctx, sensor_w, sensor_h, fps);
    if (ret != AVDK_ERR_OK) {
        goto err_sem;
    }

    ret = bk_camera_isp_vc_mux_new(&ctx->vc_mux_handle, ctx->camera_ctlr_handle);
    if (ret != AVDK_ERR_OK) {
        goto err_cam;
    }

    bk_isp_camera_vc_mux_config_t mux_cfg = {
        .channel = ISP_MP_CHN_ID,
        .discard_frames = 0,
        .frame_size = ctx->frame_size,
        .width = isp_w,
        .height = isp_h,
        .runtime_vc_switch = 1,
    };
    ret = bk_isp_camera_vc_mux_start(ctx->vc_mux_handle, &mux_cfg);
    if (ret != AVDK_ERR_OK) {
        goto err_mux;
    }

    ret = isp_display_gpu_frame_turn_on(isp_w, isp_h);
    if (ret != AVDK_ERR_OK) {
        goto err_mux_stop;
    }

    ctx->running = 1;
    s_vc_route = ctx;

    ret = rtos_create_thread(&ctx->thread,
                             (BEKEN_DEFAULT_WORKER_PRIORITY > 1) ? (BEKEN_DEFAULT_WORKER_PRIORITY - 1)
                                                                 : BEKEN_DEFAULT_WORKER_PRIORITY,
                             "vc_route",
                             (beken_thread_function_t)vc_route_task_entry,
                             4096,
                             ctx);
    if (ret != BK_OK) {
        s_vc_route = NULL;
        ctx->running = 0;
        goto err_gpu;
    }

    ret = isp_vc_route_vc_enable(default_vc, 0);
    if (ret != AVDK_ERR_OK) {
        LOGW("default vc%u enable failed %d\n", default_vc, ret);
    }

    LOGI("vc route on: display v%u, %ux%u, use vc_route_vc / vc_route_enable to switch\n",
         default_vc, isp_w, isp_h);
    return AVDK_ERR_OK;

err_gpu:
    (void)isp_display_gpu_frame_turn_off();
err_mux_stop:
    (void)bk_isp_camera_vc_mux_stop(ctx->vc_mux_handle);
err_mux:
    if (ctx->vc_mux_handle) {
        (void)bk_isp_camera_vc_mux_delete(ctx->vc_mux_handle);
        ctx->vc_mux_handle = NULL;
    }
err_cam:
    vc_route_camera_close(ctx);
err_sem:
    if (ctx->exit_sem) {
        rtos_deinit_semaphore(&ctx->exit_sem);
    }
err_free:
    os_free(ctx);
    return ret;
}

avdk_err_t isp_vc_route_turn_off(void)
{
    isp_vc_route_ctx_t *ctx = s_vc_route;

    if (ctx == NULL) {
        return AVDK_ERR_OK;
    }

    ctx->running = 0;
    if (ctx->thread != NULL && ctx->exit_sem != NULL) {
        (void)rtos_get_semaphore(&ctx->exit_sem, 1000);
    }

    (void)isp_display_gpu_frame_turn_off();

    if (ctx->active_frame.frame != NULL && ctx->vc_mux_handle != NULL) {
        (void)bk_isp_camera_vc_mux_release(ctx->vc_mux_handle, &ctx->active_frame);
        os_memset(&ctx->active_frame, 0, sizeof(ctx->active_frame));
    }

    if (ctx->vc_mux_handle != NULL) {
        (void)bk_isp_camera_vc_mux_stop(ctx->vc_mux_handle);
        (void)bk_isp_camera_vc_mux_delete(ctx->vc_mux_handle);
        ctx->vc_mux_handle = NULL;
    }

    vc_route_camera_close(ctx);
    s_vc_route = NULL;

    if (ctx->exit_sem != NULL) {
        rtos_deinit_semaphore(&ctx->exit_sem);
    }
    os_free(ctx);
    LOGI("vc route off\n");
    return AVDK_ERR_OK;
}

avdk_err_t isp_vc_route_vc_enable(uint8_t vc, uint8_t discard_frames)
{
    AVDK_RETURN_ON_FALSE(s_vc_route != NULL, AVDK_ERR_INVAL, TAG, "vc route not on");
    AVDK_RETURN_ON_FALSE(s_vc_route->vc_mux_handle != NULL, AVDK_ERR_INVAL, TAG, "mux null");
    AVDK_RETURN_ON_FALSE(vc < 2, AVDK_ERR_INVAL, TAG, "vc invalid");
    return bk_isp_camera_vc_mux_vc_enable(s_vc_route->vc_mux_handle, vc, discard_frames);
}

avdk_err_t isp_vc_route_vc_disable(uint8_t vc)
{
    AVDK_RETURN_ON_FALSE(s_vc_route != NULL, AVDK_ERR_INVAL, TAG, "vc route not on");
    AVDK_RETURN_ON_FALSE(s_vc_route->vc_mux_handle != NULL, AVDK_ERR_INVAL, TAG, "mux null");
    AVDK_RETURN_ON_FALSE(vc < 2, AVDK_ERR_INVAL, TAG, "vc invalid");
    return bk_isp_camera_vc_mux_vc_disable(s_vc_route->vc_mux_handle, vc);
}

avdk_err_t isp_vc_route_select(uint8_t vc)
{
    AVDK_RETURN_ON_FALSE(vc < 2, AVDK_ERR_INVAL, TAG, "vc invalid");
    AVDK_RETURN_ON_FALSE(s_vc_route != NULL, AVDK_ERR_INVAL, TAG, "vc route not on");

    s_vc_route->display_vc = vc;
    s_vc_route->last_seq[vc] = 0;
    LOGI("vc route display v%u\n", vc);
    return AVDK_ERR_OK;
}
