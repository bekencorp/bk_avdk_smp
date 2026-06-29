// bk7259 显示/帧缓冲兼容层实现，详见 osd_disp_compat.h。

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <driver/gpio.h>
#include "gpio_driver.h"
#include <components/bk_frame_buffer.h>
#include <components/bk_display.h>
#include <common/avdk_pixel_types.h>
#include <modules/pm.h>
#include <lcd/lcd_mipi_st7701sn_480x854.h>
#include "osd_disp_compat.h"

#define TAG "osd_disp"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define BL_PIN     GPIO_7
#define RST_PIN    GPIO_60

/* 记录一次 open 创建的全部句柄，供 close 释放 */
typedef struct {
    bk_display_ctlr_handle_t dpu_ctlr_handle;
    bk_display_bus_handle_t  dis_bus_handle;
    bk_avdk_lcd_panel_handle_t panel_handle;
} osd_disp_ctx_t;

static osd_disp_ctx_t s_disp_ctx;

static avdk_err_t osd_lodoen_enable(bool enable)
{
    pm_auxldo_ctrl_cfg_t auxldo_cfg = {0};
    auxldo_cfg.ldo   = AUXLDOS_SEL_1P8V;
    auxldo_cfg.out   = PM_AUXLDO_1P8V_OUT_1P8V;
    auxldo_cfg.user  = PM_AUXLDO_USER_DISPLAY;
    auxldo_cfg.state = enable ? PM_AUXLDO_ENABLE : PM_AUXLDO_DISABLE;
    AVDK_RETURN_ON_ERROR(bk_pm_auxldo_ctrl_vote(&auxldo_cfg), TAG, "display 1p8v ldo vote failed");
    rtos_delay_milliseconds(1);
    return AVDK_ERR_OK;
}

frame_buffer_t *frame_buffer_display_malloc(uint32_t size)
{
    frame_buffer_t *fb = (frame_buffer_t *)os_malloc(sizeof(frame_buffer_t));
    if (fb == NULL) {
        return NULL;
    }
    os_memset(fb, 0, sizeof(*fb));
    fb->frame = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, size);
    if (fb->frame == NULL) {
        os_free(fb);
        return NULL;
    }
    return fb;
}

void frame_buffer_display_free(frame_buffer_t *fb)
{
    if (fb == NULL) {
        return;
    }
    if (fb->frame != NULL) {
        bk_frame_buffer_free(fb->frame);
        fb->frame = NULL;
    }
    os_free(fb);
}

/* 显示驱动用完像素 buffer 后回调，仅释放裸像素内存 */
static avdk_err_t osd_raw_free_cb(void *raw)
{
    if (raw != NULL) {
        bk_frame_buffer_free(raw);
    }
    return AVDK_ERR_OK;
}

avdk_err_t osd_display_flush(bk_display_ctlr_handle_t handle, frame_buffer_t *fb)
{
    if (handle == NULL || fb == NULL) {
        return AVDK_ERR_INVAL;
    }

    uint8_t *raw = fb->frame;
    /* 包装结构体不再需要（OSD 已绘制完成），裸像素 buffer 交给 flush 接管 */
    fb->frame = NULL;
    os_free(fb);

    if (raw == NULL) {
        return AVDK_ERR_INVAL;
    }

    avdk_err_t ret = bk_display_flush(handle, raw, osd_raw_free_cb);
    if (ret != AVDK_ERR_OK) {
        osd_raw_free_cb(raw);
    }
    return ret;
}

avdk_err_t osd_display_open(bk_display_ctlr_handle_t *out_handle)
{
    int ret = AVDK_ERR_GENERIC;
    const bk_display_dsi_panel_t *panel = &lcd_device_st7701sn_mipi_480x854;

    if (out_handle == NULL) {
        return AVDK_ERR_INVAL;
    }
    os_memset(&s_disp_ctx, 0, sizeof(s_disp_ctx));

    bk_display_dpu_config_t dpu_config = {0};
    dpu_config.video.enable     = true;
    dpu_config.video.decompress = false;
    dpu_config.video.format     = BK_PIXEL_FORMAT_RGB565;

    const bk_lcd_panel_config_t panel_config = {
        .reset_pin = RST_PIN,
    };

    AVDK_RETURN_ON_ERROR(osd_lodoen_enable(true), TAG, "ldo enable failed");
    AVDK_GOTO_ON_ERROR(bk_display_dsi_bus_new(&s_disp_ctx.dis_bus_handle, NULL), err, TAG, "dsi bus new err\n");
    AVDK_GOTO_ON_ERROR(bk_lcd_mipi_panel_new(s_disp_ctx.dis_bus_handle, &panel_config, panel, &s_disp_ctx.panel_handle),
                       err, TAG, "create panel err\n");
    AVDK_GOTO_ON_ERROR(bk_display_dpu_ctlr_new(&s_disp_ctx.dpu_ctlr_handle, s_disp_ctx.panel_handle, &dpu_config),
                       err, TAG, "dpu ctlr new err\n");
    AVDK_GOTO_ON_ERROR(bk_display_init(s_disp_ctx.dpu_ctlr_handle), err, TAG, "display init err\n");
    AVDK_GOTO_ON_ERROR(bk_display_open(s_disp_ctx.dpu_ctlr_handle), err, TAG, "display open err\n");

    /* 背光 */
    gpio_dev_unmap(BL_PIN);
    BK_LOG_ON_ERR(bk_gpio_enable_output(BL_PIN));
    BK_LOG_ON_ERR(bk_gpio_pull_up(BL_PIN));
    bk_gpio_set_capacity(BL_PIN, GPIO_DRIVER_CAPACITY_3);
    bk_gpio_set_output_high(BL_PIN);

    *out_handle = s_disp_ctx.dpu_ctlr_handle;
    LOGI("LCD opened: panel=%s %dx%d RGB565\n", panel->name, OSD_BG_W, OSD_BG_H);
    return AVDK_ERR_OK;

err:
    osd_display_close(s_disp_ctx.dpu_ctlr_handle);
    return ret;
}

avdk_err_t osd_display_close(bk_display_ctlr_handle_t handle)
{
    (void)handle;

    if (s_disp_ctx.dpu_ctlr_handle) {
        bk_display_deinit(s_disp_ctx.dpu_ctlr_handle);
        bk_display_delete(s_disp_ctx.dpu_ctlr_handle);
        s_disp_ctx.dpu_ctlr_handle = NULL;
    }
    if (s_disp_ctx.panel_handle) {
        bk_lcd_panel_delete(s_disp_ctx.panel_handle);
        s_disp_ctx.panel_handle = NULL;
    }
    if (s_disp_ctx.dis_bus_handle) {
        bk_display_bus_delete(s_disp_ctx.dis_bus_handle);
        s_disp_ctx.dis_bus_handle = NULL;
    }
    bk_gpio_set_output_low(BL_PIN);
    osd_lodoen_enable(false);
    LOGI("LCD closed\n");
    return AVDK_ERR_OK;
}
