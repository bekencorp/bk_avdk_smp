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
#include <components/bk_hardware_ram.h>
#include <components/bk_isp_camera.h>
#include <components/bk_camera_isp_ctlr.h>
#include <isp_camera_ctlr.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include <driver/hpdma.h>
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
#define VC_ROUTE_FLEXA_LINES 16
#define VC_ROUTE_FLEXA_BUFF_CNT 3
#define VC_ROUTE_FLEXA_ALIGN 64U
#define VC_ROUTE_HPDMA_TIMEOUT_MS 1000

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
    uint8_t *flexa_ring_raw;
    uint8_t *flexa_ring;
    uint32_t flexa_ring_size;
    uint8_t flexa_lines;
    uint8_t flexa_buff_cnt;
    uint16_t input_blocks;
    uint16_t output_height_aligned;
    volatile uint32_t gpu_output_blocks_done;
    hpdma_id_t input_dma;
    void *input_dma_link;
    beken_semaphore_t input_dma_sem;
    beken_semaphore_t gpu_block_sem;
    beken_thread_t thread;
    beken_semaphore_t exit_sem;
    beken_semaphore_t gpu_done_sem;
} isp_vc_route_ctx_t;

static isp_vc_route_ctx_t *s_vc_route = NULL;

static void vc_route_display_frame_free(void *frame, void *args);

static inline uintptr_t vc_route_align_up(uintptr_t addr, uint32_t align)
{
    return (addr + (align - 1U)) & ~((uintptr_t)align - 1U);
}

static inline uint32_t vc_route_min_u32(uint32_t a, uint32_t b)
{
    return (a < b) ? a : b;
}

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

static void vc_route_input_dma_finish_cb(hpdma_id_t hpdma_id, void *user_data)
{
    (void)hpdma_id;
    if (user_data != NULL) {
        rtos_set_semaphore((beken_semaphore_t *)user_data);
    }
}

static void vc_route_gpu_flexa_line_done(uint32_t done_lines, void *args)
{
    isp_vc_route_ctx_t *ctx = (isp_vc_route_ctx_t *)args;

    if (ctx == NULL) {
        return;
    }
    ctx->gpu_output_blocks_done = done_lines;
    if (ctx->gpu_block_sem != NULL) {
        rtos_set_semaphore(&ctx->gpu_block_sem);
    }
}

static void vc_route_input_dma_sem_drain(isp_vc_route_ctx_t *ctx)
{
    if (ctx == NULL || ctx->input_dma_sem == NULL) {
        return;
    }

    while (rtos_get_semaphore(&ctx->input_dma_sem, BEKEN_NO_WAIT) == BK_OK) {
    }
}

static avdk_err_t vc_route_input_dma_transfer(isp_vc_route_ctx_t *ctx,
                                              hpdma_link_config_t *cfg,
                                              uint32_t cfg_cnt)
{
    bk_err_t ret;

    if (ctx == NULL || ctx->input_dma >= HPDMA_ID_MAX ||
        ctx->input_dma_link == NULL || ctx->input_dma_sem == NULL ||
        cfg == NULL || cfg_cnt == 0) {
        return AVDK_ERR_INVAL;
    }

    vc_route_input_dma_sem_drain(ctx);
    ret = bk_hpdma_link_set_descs(ctx->input_dma_link, cfg, cfg_cnt);
    if (ret != BK_OK) {
        LOGE("input hpdma set desc failed %d\n", ret);
        return AVDK_ERR_GENERIC;
    }
    ret = bk_hpdma_link_transfer(ctx->input_dma, ctx->input_dma_link);
    if (ret != BK_OK) {
        LOGE("input hpdma start failed %d\n", ret);
        return AVDK_ERR_GENERIC;
    }
    ret = rtos_get_semaphore(&ctx->input_dma_sem, VC_ROUTE_HPDMA_TIMEOUT_MS);
    if (ret != BK_OK) {
        LOGE("input hpdma timeout, dma_id=%d ret=%d\n", ctx->input_dma, ret);
        return AVDK_ERR_TIMEOUT;
    }

    return AVDK_ERR_OK;
}

static avdk_err_t vc_route_feeder_copy_block(isp_vc_route_ctx_t *ctx,
                                             const bk_isp_camera_vc_mux_frame_ref_t *frame,
                                             uint32_t block)
{
    uint32_t block_start_line;
    uint32_t copy_lines;
    uint32_t uv_lines;
    uint32_t slot;
    uint8_t *src_y;
    uint8_t *src_uv;
    uint8_t *dst_y;
    uint8_t *dst_uv;
    hpdma_link_config_t cfg[2];
    uint32_t cfg_cnt = 0;

    if (ctx == NULL || frame == NULL || frame->frame == NULL || block == 0) {
        return AVDK_ERR_INVAL;
    }

    block_start_line = (block - 1U) * ctx->flexa_lines;
    if (block_start_line >= ctx->height) {
        return AVDK_ERR_INVAL;
    }

    copy_lines = vc_route_min_u32(ctx->flexa_lines, ctx->height - block_start_line);
    uv_lines = (copy_lines + 1U) / 2U;
    slot = (block - 1U) % ctx->flexa_buff_cnt;

    src_y = frame->frame + block_start_line * ctx->width;
    src_uv = frame->frame + ((uint32_t)ctx->width * ctx->height) +
             ((block_start_line / 2U) * ctx->width);
    dst_y = ctx->flexa_ring + slot * ((uint32_t)ctx->width * ctx->flexa_lines);
    dst_uv = ctx->flexa_ring + ((uint32_t)ctx->width * ctx->flexa_lines * ctx->flexa_buff_cnt) +
             slot * ((uint32_t)ctx->width * (ctx->flexa_lines / 2U));

    arch_dcache_flush_and_invd_range(src_y, (uint32_t)ctx->width * copy_lines);
    arch_dcache_flush_and_invd_range(src_uv, (uint32_t)ctx->width * uv_lines);

    os_memset(cfg, 0, sizeof(cfg));
    cfg[cfg_cnt].src_addr = (uint32_t)(uintptr_t)src_y;
    cfg[cfg_cnt].dst_addr = (uint32_t)(uintptr_t)dst_y;
    cfg[cfg_cnt].src_xsize = ctx->width;
    cfg[cfg_cnt].dst_xsize = ctx->width;
    cfg[cfg_cnt].src_ysize = copy_lines;
    cfg[cfg_cnt].dst_ysize = copy_lines;
    cfg[cfg_cnt].finish_int_en = (uv_lines == 0) ? 1 : 0;
    cfg_cnt++;

    if (uv_lines > 0) {
        cfg[cfg_cnt].src_addr = (uint32_t)(uintptr_t)src_uv;
        cfg[cfg_cnt].dst_addr = (uint32_t)(uintptr_t)dst_uv;
        cfg[cfg_cnt].src_xsize = ctx->width;
        cfg[cfg_cnt].dst_xsize = ctx->width;
        cfg[cfg_cnt].src_ysize = uv_lines;
        cfg[cfg_cnt].dst_ysize = uv_lines;
        cfg[cfg_cnt].finish_int_en = 1;
        cfg_cnt++;
    }

    return vc_route_input_dma_transfer(ctx, cfg, cfg_cnt);
}

static uint32_t vc_route_required_output_blocks(isp_vc_route_ctx_t *ctx, uint32_t consumed_src_blocks)
{
    if (ctx == NULL || ctx->height == 0) {
        return 0;
    }

    return (consumed_src_blocks * ctx->output_height_aligned + ctx->height - 1U) / ctx->height;
}

static avdk_err_t vc_route_wait_ring_slot(isp_vc_route_ctx_t *ctx, uint32_t block)
{
    uint32_t consumed_src_blocks;
    uint32_t required_output_blocks;

    if (ctx == NULL) {
        return AVDK_ERR_INVAL;
    }
    if (block <= ctx->flexa_buff_cnt) {
        return AVDK_ERR_OK;
    }

    consumed_src_blocks = block - ctx->flexa_buff_cnt;
    required_output_blocks = vc_route_required_output_blocks(ctx, consumed_src_blocks);
    while (ctx->running && ctx->gpu_output_blocks_done < required_output_blocks) {
        if (rtos_get_semaphore(&ctx->gpu_block_sem, VC_ROUTE_HPDMA_TIMEOUT_MS) != BK_OK) {
            LOGE("wait gpu block timeout, block=%u need_out=%u done_out=%u\n",
                 block, required_output_blocks, ctx->gpu_output_blocks_done);
            return AVDK_ERR_TIMEOUT;
        }
    }

    return ctx->running ? AVDK_ERR_OK : AVDK_ERR_GENERIC;
}

static avdk_err_t vc_route_feeder_init(isp_vc_route_ctx_t *ctx)
{
    bk_err_t ret;
    uintptr_t raw;

    if (ctx == NULL) {
        return AVDK_ERR_INVAL;
    }

    ctx->flexa_lines = VC_ROUTE_FLEXA_LINES;
    ctx->flexa_buff_cnt = VC_ROUTE_FLEXA_BUFF_CNT;
    ctx->input_blocks = (ctx->height + ctx->flexa_lines - 1U) / ctx->flexa_lines;
    ctx->output_height_aligned = (1080U + 15U) & ~15U;
    ctx->flexa_ring_size = (uint32_t)ctx->width * ctx->flexa_lines *
                           ctx->flexa_buff_cnt * 3U / 2U;
    ctx->input_dma = HPDMA_ID_MAX;

    ctx->flexa_ring_raw = (uint8_t *)bk_get_isp_flexa_buffer(ctx->flexa_ring_size + VC_ROUTE_FLEXA_ALIGN);
    if (ctx->flexa_ring_raw == NULL) {
        LOGE("alloc input flexa ring failed, size=%u\n", ctx->flexa_ring_size);
        return AVDK_ERR_NOMEM;
    }
    raw = vc_route_align_up((uintptr_t)ctx->flexa_ring_raw, VC_ROUTE_FLEXA_ALIGN);
    ctx->flexa_ring = (uint8_t *)raw;
    os_memset(ctx->flexa_ring, 0, ctx->flexa_ring_size);
    arch_dcache_flush_and_invd_range(ctx->flexa_ring, ctx->flexa_ring_size);

    ret = rtos_init_semaphore(&ctx->input_dma_sem, 1);
    if (ret != BK_OK) {
        goto fail;
    }
    ret = rtos_init_semaphore(&ctx->gpu_block_sem, 1);
    if (ret != BK_OK) {
        goto fail;
    }

    ctx->input_dma_link = bk_hpdma_link_init(2);
    if (ctx->input_dma_link == NULL) {
        LOGE("input hpdma link init failed\n");
        goto fail;
    }
    ctx->input_dma = bk_hpdma_alloc(HPDMA_DEV_DTCM);
    if (ctx->input_dma >= HPDMA_ID_MAX) {
        LOGE("input hpdma alloc failed\n");
        ctx->input_dma = HPDMA_ID_MAX;
        goto fail;
    }
    (void)bk_hpdma_set_dest_burst_len(ctx->input_dma, HPDMA_BURST_LEN_INC16);
    (void)bk_hpdma_set_src_burst_len(ctx->input_dma, HPDMA_BURST_LEN_INC16);
    (void)bk_hpdma_register_isr(ctx->input_dma, NULL, NULL,
                                vc_route_input_dma_finish_cb, &ctx->input_dma_sem);
    (void)bk_hpdma_enable_finish_interrupt(ctx->input_dma);

    LOGI("vc flexa feeder ready: ring=%p size=%u lines=%u cnt=%u blocks=%u dma=%d\n",
         ctx->flexa_ring, ctx->flexa_ring_size, ctx->flexa_lines,
         ctx->flexa_buff_cnt, ctx->input_blocks, ctx->input_dma);
    return AVDK_ERR_OK;

fail:
    if (ctx->input_dma < HPDMA_ID_MAX) {
        (void)bk_hpdma_register_isr(ctx->input_dma, NULL, NULL, NULL, NULL);
        (void)bk_hpdma_free(HPDMA_DEV_DTCM, ctx->input_dma);
        ctx->input_dma = HPDMA_ID_MAX;
    }
    if (ctx->input_dma_link != NULL) {
        bk_hpdma_link_deinit(ctx->input_dma_link);
        ctx->input_dma_link = NULL;
    }
    if (ctx->gpu_block_sem != NULL) {
        rtos_deinit_semaphore(&ctx->gpu_block_sem);
        ctx->gpu_block_sem = NULL;
    }
    if (ctx->input_dma_sem != NULL) {
        rtos_deinit_semaphore(&ctx->input_dma_sem);
        ctx->input_dma_sem = NULL;
    }
    if (ctx->flexa_ring_raw != NULL) {
        hsram_free(ctx->flexa_ring_raw);
        ctx->flexa_ring_raw = NULL;
        ctx->flexa_ring = NULL;
    }
    return AVDK_ERR_GENERIC;
}

static void vc_route_feeder_deinit(isp_vc_route_ctx_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    if (ctx->input_dma < HPDMA_ID_MAX) {
        (void)bk_hpdma_disable_finish_interrupt(ctx->input_dma);
        (void)bk_hpdma_register_isr(ctx->input_dma, NULL, NULL, NULL, NULL);
        (void)bk_hpdma_free(HPDMA_DEV_DTCM, ctx->input_dma);
        ctx->input_dma = HPDMA_ID_MAX;
    }
    if (ctx->input_dma_link != NULL) {
        bk_hpdma_link_deinit(ctx->input_dma_link);
        ctx->input_dma_link = NULL;
    }
    if (ctx->gpu_block_sem != NULL) {
        rtos_deinit_semaphore(&ctx->gpu_block_sem);
        ctx->gpu_block_sem = NULL;
    }
    if (ctx->input_dma_sem != NULL) {
        rtos_deinit_semaphore(&ctx->input_dma_sem);
        ctx->input_dma_sem = NULL;
    }
    if (ctx->flexa_ring_raw != NULL) {
        hsram_free(ctx->flexa_ring_raw);
        ctx->flexa_ring_raw = NULL;
        ctx->flexa_ring = NULL;
    }
}

static avdk_err_t vc_route_flexa_frame_process(isp_vc_route_ctx_t *ctx,
                                               const bk_isp_camera_vc_mux_frame_ref_t *frame)
{
    avdk_err_t ret;

    if (ctx == NULL || frame == NULL || frame->frame == NULL || ctx->flexa_ring == NULL) {
        return AVDK_ERR_INVAL;
    }

    ctx->gpu_output_blocks_done = 0;
    ret = isp_display_gpu_frame_prepare(frame->frame, vc_route_display_frame_free, ctx);
    if (ret != AVDK_ERR_OK) {
        return ret;
    }

    for (uint32_t block = 1; block <= ctx->input_blocks; block++) {
        ret = vc_route_wait_ring_slot(ctx, block);
        if (ret != AVDK_ERR_OK) {
            goto fail;
        }

        ret = vc_route_feeder_copy_block(ctx, frame, block);
        if (ret != AVDK_ERR_OK) {
            goto fail;
        }

        ret = isp_display_gpu_frame_lines_ready(frame->sequence, block);
        if (ret != AVDK_ERR_OK) {
            goto fail;
        }
    }

    return AVDK_ERR_OK;

fail:
    isp_display_gpu_frame_abort();
    return ret;
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
    if (ctx->gpu_done_sem != NULL) {
        rtos_set_semaphore(&ctx->gpu_done_sem);
    }
}

static void vc_route_task_entry(void *arg)
{
    isp_vc_route_ctx_t *ctx = (isp_vc_route_ctx_t *)arg;

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

                avdk_err_t gpu_ret = vc_route_flexa_frame_process(ctx, &frame);
                if (gpu_ret != AVDK_ERR_OK) {
                    ctx->gpu_in_use = 0;
                    if (ctx->active_frame.frame == NULL) {
                        submitted = 1;
                    } else {
                        (void)bk_isp_camera_vc_mux_release(ctx->vc_mux_handle, &ctx->active_frame);
                        os_memset(&ctx->active_frame, 0, sizeof(ctx->active_frame));
                        submitted = 1;
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
    ret = rtos_init_semaphore(&ctx->gpu_done_sem, 1);
    if (ret != BK_OK) {
        goto err_sem;
    }
    (void)rtos_get_semaphore(&ctx->gpu_done_sem, BEKEN_NO_WAIT);

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

    ret = vc_route_feeder_init(ctx);
    if (ret != AVDK_ERR_OK) {
        goto err_mux_stop;
    }

    ret = isp_display_gpu_frame_turn_on_with_flexa_src(isp_w, isp_h,
                                                       ctx->flexa_ring,
                                                       ctx->flexa_buff_cnt,
                                                       vc_route_gpu_flexa_line_done,
                                                       ctx);
    if (ret != AVDK_ERR_OK) {
        goto err_feeder;
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
err_feeder:
    vc_route_feeder_deinit(ctx);
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
    if (ctx->gpu_done_sem) {
        rtos_deinit_semaphore(&ctx->gpu_done_sem);
    }
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
    if (ctx->gpu_done_sem != NULL) {
        rtos_set_semaphore(&ctx->gpu_done_sem);
    }
    if (ctx->thread != NULL && ctx->exit_sem != NULL) {
        (void)rtos_get_semaphore(&ctx->exit_sem, 1000);
    }

    (void)isp_display_gpu_frame_turn_off();
    vc_route_feeder_deinit(ctx);

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
    if (ctx->gpu_done_sem != NULL) {
        rtos_deinit_semaphore(&ctx->gpu_done_sem);
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
