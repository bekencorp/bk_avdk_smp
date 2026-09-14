#include "include/isp_display_pipeline.h"

#include <os/mem.h>
#include <os/os.h>
#include <avdk_check.h>
#include <cache.h>
#include <components/bk_gpu.h>
#include <components/bk_gpu_ctlr.h>
#include <components/bk_frame_buffer.h>
#include <components/bk_display.h>
#include <components/bk_isp_camera.h>
#include <components/bk_camera_isp_ctlr.h>
#include <common/avdk_pixel_types.h>
#include <driver/isp_base.h>
#include <components/log.h>

#include "display/display.h"

#define TAG "isp_disp"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define ISP_DISP_GPU_FLEXA_LINES  16

static bk_gpu_ctlr_handle_t s_gpu = NULL;

typedef struct {
    volatile uint8_t running;
    volatile uint8_t gpu_in_use;
    uint8_t display_vc;
    uint16_t width;
    uint16_t height;
    uint32_t frame_size;
    uint32_t last_seq[2];
    bk_isp_camera_vc_mux_handle_t vc_mux_handle;
    bk_isp_camera_vc_mux_frame_ref_t active_frame;
    beken_thread_t thread;
    beken_semaphore_t exit_sem;
    beken_semaphore_t gpu_done_sem;
} isp_disp_route_ctx_t;

typedef struct {
    volatile uint8_t busy;
    void *input_frame;
    void (*input_free)(void *frame, void *args);
    void *input_free_args;
    void (*flexa_line_done)(uint32_t done_lines, void *args);
    void *flexa_line_done_args;
    uint16_t height;
    uint8_t flexa_lines;
} isp_disp_frame_ctx_t;

static isp_disp_route_ctx_t s_disp_route = {0};
static isp_disp_frame_ctx_t s_frame_ctx = {0};

static void *isp_disp_frame_malloc(uint32_t size)
{
    return bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, size);
}

static avdk_err_t isp_disp_frame_free(void *ptr)
{
    bk_frame_buffer_free(ptr);
    return AVDK_ERR_OK;
}

static void isp_disp_flexa_line_done(uint32_t done_lines, void *args)
{
    (void)args;

    if (s_frame_ctx.flexa_line_done != NULL) {
        s_frame_ctx.flexa_line_done(done_lines, s_frame_ctx.flexa_line_done_args);
    }
}

static void isp_disp_frame_complete(void *frame, uint32_t frame_size, void *args)
{
    (void)frame_size;
    (void)args;

    void *dpu = display_get_dpu_handle();
    if (dpu != NULL) {
        if (bk_display_flush(dpu, frame, isp_disp_frame_free) != AVDK_ERR_OK) {
            isp_disp_frame_free(frame);
        }
    } else {
        isp_disp_frame_free(frame);
    }

    if (s_frame_ctx.input_free != NULL && s_frame_ctx.input_frame != NULL) {
        s_frame_ctx.input_free(s_frame_ctx.input_frame, s_frame_ctx.input_free_args);
    }
    s_frame_ctx.input_frame = NULL;
    s_frame_ctx.input_free = NULL;
    s_frame_ctx.input_free_args = NULL;
    s_frame_ctx.busy = 0;
}

static void isp_disp_route_frame_free(void *frame, void *args)
{
    isp_disp_route_ctx_t *ctx = (isp_disp_route_ctx_t *)args;

    (void)frame;
    if (ctx == NULL) {
        return;
    }
    if (ctx->active_frame.frame != NULL && ctx->vc_mux_handle != NULL) {
        (void)bk_isp_camera_vc_mux_release(ctx->vc_mux_handle, &ctx->active_frame);
        os_memset(&ctx->active_frame, 0, sizeof(ctx->active_frame));
    }
    ctx->gpu_in_use = 0;
    if (ctx->gpu_done_sem != NULL) {
        rtos_set_semaphore(&ctx->gpu_done_sem);
    }
}

static void isp_disp_route_task_entry(void *arg)
{
    isp_disp_route_ctx_t *ctx = (isp_disp_route_ctx_t *)arg;

    while (ctx != NULL && ctx->running) {
        uint8_t wait_gpu_done = 0;

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
                if (ctx->gpu_done_sem != NULL) {
                    (void)rtos_get_semaphore(&ctx->gpu_done_sem, BEKEN_NO_WAIT);
                }

                arch_dcache_flush_and_invd_range(frame.frame, ctx->frame_size);

                avdk_err_t gpu_ret = isp_display_gpu_frame_process(frame.frame, frame.sequence,
                                                                 isp_disp_route_frame_free, ctx);
                if (gpu_ret != AVDK_ERR_OK) {
                    ctx->gpu_in_use = 0;
                    if (ctx->active_frame.frame == NULL) {
                        submitted = 1;
                    } else {
                        os_memset(&ctx->active_frame, 0, sizeof(ctx->active_frame));
                    }
                } else {
                    submitted = 1;
                    wait_gpu_done = 1;
                }
            }

            if (!submitted && frame.frame != NULL) {
                (void)bk_isp_camera_vc_mux_release(ctx->vc_mux_handle, &frame);
            }
        }
        if (wait_gpu_done && ctx->gpu_done_sem != NULL) {
            (void)rtos_get_semaphore(&ctx->gpu_done_sem, 1000);
        } else if (ctx->gpu_done_sem != NULL) {
            (void)rtos_get_semaphore(&ctx->gpu_done_sem, 1);
        } else {
            rtos_delay_milliseconds(1);
        }
    }

    if (ctx != NULL) {
        ctx->thread = NULL;
        if (ctx->exit_sem != NULL) {
            rtos_set_semaphore(&ctx->exit_sem);
        }
    }
    rtos_delete_thread(NULL);
}

static void isp_disp_route_stop_locked(isp_disp_route_ctx_t *ctx)
{
    if (ctx == NULL || !ctx->running) {
        return;
    }

    ctx->running = 0;
    if (ctx->gpu_done_sem != NULL) {
        rtos_set_semaphore(&ctx->gpu_done_sem);
    }
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

    if (ctx->exit_sem != NULL) {
        rtos_deinit_semaphore(&ctx->exit_sem);
        ctx->exit_sem = NULL;
    }
    if (ctx->gpu_done_sem != NULL) {
        rtos_deinit_semaphore(&ctx->gpu_done_sem);
        ctx->gpu_done_sem = NULL;
    }

    os_memset(ctx, 0, sizeof(*ctx));
}

bool isp_display_lcd_is_open(void)
{
    return display_get_dpu_handle() != NULL;
}

static avdk_err_t isp_display_gpu_frame_turn_on_internal(uint16_t width, uint16_t height,
                                                         void *src_buffer, uint8_t flexa_buff_cnt,
                                                         void (*flexa_line_done)(uint32_t done_lines, void *args),
                                                         void *flexa_line_done_args)
{
    avdk_err_t ret;
    bk_gpu_ctlr_config_t cfg = {0};

    if (s_gpu != NULL) {
        return AVDK_ERR_BUSY;
    }
    if (src_buffer != NULL && flexa_buff_cnt == 0) {
        return AVDK_ERR_INVAL;
    }

    ret = display_open();
    if (ret != AVDK_ERR_OK) {
        return ret;
    }

    cfg.rotate_degree = 90;
    cfg.src_width = width;
    cfg.src_height = height;
    cfg.dst_width = 1920;
    cfg.dst_height = 1080;
    cfg.src_format = BK_PIXEL_FORMAT_NV12;
    cfg.dst_format = BK_PIXEL_FORMAT_ARGB8888;
    cfg.compress = true;
    cfg.scale = true;
    cfg.flexa = true;
    cfg.flexa_lines = ISP_DISP_GPU_FLEXA_LINES;
    cfg.flexa_buff_cnt = (src_buffer != NULL) ?
                          flexa_buff_cnt :
                          ((height + cfg.flexa_lines - 1) / cfg.flexa_lines);
    cfg.src_buffer = (uint8_t *)src_buffer;
    cfg.frame_malloc = isp_disp_frame_malloc;
    cfg.frame_free = isp_disp_frame_free;
    if (flexa_line_done != NULL) {
        cfg.flexa_line_done = isp_disp_flexa_line_done;
    }
    cfg.frame_done = isp_disp_frame_complete;

    ret = bk_gpu_ctlr_new(&s_gpu, &cfg);
    if (ret != AVDK_ERR_OK) {
        goto err_disp;
    }
    ret = bk_gpu_init(s_gpu);
    if (ret != AVDK_ERR_OK) {
        goto err_gpu;
    }
    ret = bk_gpu_open(s_gpu);
    if (ret != AVDK_ERR_OK) {
        goto err_deinit;
    }
    if (flexa_line_done != NULL) {
        bool per_flexa = true;
        ret = bk_gpu_ioctl(s_gpu, BK_GPU_IOCTL_SET_OSD_BY_FLEXA, &per_flexa);
        if (ret != AVDK_ERR_OK) {
            goto err_close;
        }
    }

    os_memset(&s_frame_ctx, 0, sizeof(s_frame_ctx));
    s_frame_ctx.height = height;
    s_frame_ctx.flexa_lines = cfg.flexa_lines;
    s_frame_ctx.flexa_line_done = flexa_line_done;
    s_frame_ctx.flexa_line_done_args = flexa_line_done_args;
    LOGI("frame GPU on: %ux%u -> 1920x1080\n", width, height);
    return AVDK_ERR_OK;

err_close:
    (void)bk_gpu_close(s_gpu);
err_deinit:
    (void)bk_gpu_deinit(s_gpu);
err_gpu:
    (void)bk_gpu_delete(s_gpu);
    s_gpu = NULL;
err_disp:
    (void)display_close();
    return ret;
}

avdk_err_t isp_display_gpu_frame_turn_on(uint16_t width, uint16_t height)
{
    return isp_display_gpu_frame_turn_on_internal(width, height, NULL, 0, NULL, NULL);
}

avdk_err_t isp_display_gpu_frame_turn_on_with_flexa_src(uint16_t width, uint16_t height,
                                                        void *src_buffer, uint8_t flexa_buff_cnt,
                                                        void (*flexa_line_done)(uint32_t done_lines, void *args),
                                                        void *flexa_line_done_args)
{
    return isp_display_gpu_frame_turn_on_internal(width, height, src_buffer, flexa_buff_cnt,
                                                 flexa_line_done, flexa_line_done_args);
}

avdk_err_t isp_display_gpu_frame_turn_off(void)
{
    if (s_disp_route.running) {
        return AVDK_ERR_BUSY;
    }
    if (s_gpu != NULL) {
        avdk_err_t ret = bk_gpu_close(s_gpu);
        if (ret != AVDK_ERR_OK) {
            return ret;
        }
        (void)bk_gpu_deinit(s_gpu);
        (void)bk_gpu_delete(s_gpu);
        s_gpu = NULL;
    }
    os_memset(&s_frame_ctx, 0, sizeof(s_frame_ctx));
    (void)display_close();
    return AVDK_ERR_OK;
}

avdk_err_t isp_display_gpu_frame_process(void *frame, uint32_t sequence,
                                         void (*input_free)(void *frame, void *args), void *args)
{
    avdk_err_t ret;

    ret = isp_display_gpu_frame_prepare(frame, input_free, args);
    if (ret != AVDK_ERR_OK) {
        return ret;
    }

    ret = bk_gpu_ioctl(s_gpu, BK_GPU_IOCTL_FLEXA_ADDR_MAPPING, frame);
    if (ret != AVDK_ERR_OK) {
        goto fail;
    }

    ret = isp_display_gpu_frame_lines_ready(sequence, 1);
    if (ret != AVDK_ERR_OK) {
        goto fail;
    }

    ret = isp_display_gpu_frame_lines_ready(sequence,
                                           (s_frame_ctx.height + s_frame_ctx.flexa_lines - 1) /
                                           s_frame_ctx.flexa_lines);
    if (ret != AVDK_ERR_OK) {
        goto fail;
    }
    return AVDK_ERR_OK;

fail:
    isp_display_gpu_frame_abort();
    return ret;
}

avdk_err_t isp_display_gpu_frame_prepare(void *frame,
                                         void (*input_free)(void *frame, void *args), void *args)
{
    if (s_gpu == NULL || frame == NULL) {
        return AVDK_ERR_INVAL;
    }
    if (s_frame_ctx.busy) {
        return AVDK_ERR_BUSY;
    }

    s_frame_ctx.busy = 1;
    s_frame_ctx.input_frame = frame;
    s_frame_ctx.input_free = input_free;
    s_frame_ctx.input_free_args = args;
    return AVDK_ERR_OK;
}

avdk_err_t isp_display_gpu_frame_lines_ready(uint32_t sequence, uint32_t line_cnt)
{
    if (s_gpu == NULL || !s_frame_ctx.busy || line_cnt == 0) {
        return AVDK_ERR_INVAL;
    }

    bk_gpu_isp_flexa_event_t event = {
        .frame_seq = sequence,
        .line_cnt = line_cnt,
    };

    return bk_gpu_ioctl(s_gpu, BK_GPU_IOCTL_ISP_FLEXA_READY, &event);
}

void isp_display_gpu_frame_abort(void)
{
    if (s_gpu != NULL) {
        (void)bk_gpu_ioctl(s_gpu, BK_GPU_IOCTL_SET_NOTIFY, NULL);
    }

    if (s_frame_ctx.input_free != NULL && s_frame_ctx.input_frame != NULL) {
        s_frame_ctx.input_free(s_frame_ctx.input_frame, s_frame_ctx.input_free_args);
    }
    s_frame_ctx.input_frame = NULL;
    s_frame_ctx.input_free = NULL;
    s_frame_ctx.input_free_args = NULL;
    s_frame_ctx.busy = 0;
}

bool isp_display_gpu_frame_busy(void)
{
    return s_frame_ctx.busy != 0;
}

bool isp_display_pipeline_is_running(void)
{
    return s_disp_route.running != 0;
}

avdk_err_t isp_display_pipeline_start(bk_isp_camera_ctlr_handle_t camera_handle,
                                      uint16_t src_w, uint16_t src_h)
{
    avdk_err_t ret;
    isp_disp_route_ctx_t *ctx = &s_disp_route;

    if (isp_display_pipeline_is_running()) {
        return AVDK_ERR_OK;
    }
    AVDK_RETURN_ON_FALSE(camera_handle != NULL, AVDK_ERR_INVAL, TAG, "camera null");

    os_memset(ctx, 0, sizeof(*ctx));
    ctx->width = src_w;
    ctx->height = src_h;
    ctx->frame_size = (uint32_t)src_w * (uint32_t)src_h * 3U / 2U;
    ctx->display_vc = 0;

    ret = rtos_init_semaphore(&ctx->exit_sem, 1);
    if (ret != BK_OK) {
        return ret;
    }
    ret = rtos_init_semaphore(&ctx->gpu_done_sem, 1);
    if (ret != BK_OK) {
        goto err_sem;
    }
    (void)rtos_get_semaphore(&ctx->gpu_done_sem, BEKEN_NO_WAIT);

    ret = bk_camera_isp_vc_mux_new(&ctx->vc_mux_handle, camera_handle);
    if (ret != AVDK_ERR_OK) {
        goto err_sem;
    }

    bk_isp_camera_vc_mux_config_t mux_cfg = {
        .channel = ISP_MP_CHN_ID,
        .discard_frames = 0,
        .frame_size = ctx->frame_size,
        .width = src_w,
        .height = src_h,
        .runtime_vc_switch = 0,
    };
    ret = bk_isp_camera_vc_mux_start(ctx->vc_mux_handle, &mux_cfg);
    if (ret != AVDK_ERR_OK) {
        goto err_mux;
    }

    ret = bk_isp_camera_vc_mux_vc_enable(ctx->vc_mux_handle, 0, 0);
    if (ret != AVDK_ERR_OK) {
        LOGW("vc0 enable failed %d\n", ret);
    }

    ret = isp_display_gpu_frame_turn_on(src_w, src_h);
    if (ret != AVDK_ERR_OK) {
        goto err_mux_stop;
    }

    ctx->running = 1;
    ret = rtos_create_thread(&ctx->thread,
                             (BEKEN_DEFAULT_WORKER_PRIORITY > 1) ? (BEKEN_DEFAULT_WORKER_PRIORITY - 1)
                                                                 : BEKEN_DEFAULT_WORKER_PRIORITY,
                             "isp_disp",
                             (beken_thread_function_t)isp_disp_route_task_entry,
                             4096,
                             ctx);
    if (ret != BK_OK) {
        ctx->running = 0;
        goto err_gpu;
    }

    LOGI("isp display pipeline started: %ux%u frame GPU (doorbell aligned)\n", src_w, src_h);
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
err_sem:
    if (ctx->gpu_done_sem) {
        rtos_deinit_semaphore(&ctx->gpu_done_sem);
        ctx->gpu_done_sem = NULL;
    }
    if (ctx->exit_sem) {
        rtos_deinit_semaphore(&ctx->exit_sem);
        ctx->exit_sem = NULL;
    }
    os_memset(ctx, 0, sizeof(*ctx));
    return ret;
}

avdk_err_t isp_display_pipeline_stop(void)
{
    isp_disp_route_stop_locked(&s_disp_route);
    LOGI("isp display pipeline stopped\n");
    return AVDK_ERR_OK;
}
