#include <os/os.h>
#include <os/mem.h>
#include <stddef.h>
#include <stdint.h>
#include <common/bk_include.h>
#include <components/log.h>
#include <avdk_error.h>
#include <avdk_check.h>
#include "dpu_reg.h"
 #include "sys_driver.h"
#include "dpu_driver.h"
#include "dpu_core.h"
#include "components/bk_lcd_panel.h"
#include <components/bk_hardware_ram.h>
#include "sys_ahbp_reg.h"
#include <int_types_impl.h>
#include "avdk_monitor.h"
#include "driver/sys_pm.h"
#define TAG "dpu_core"

#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#if CONFIG_DPU_FLUSH_TIMER_DEBUG
/* DPU flush debug timer interval in seconds */
#define DPU_FLUSH_TIMER_INTERVAL (5)
static void dpu_flush_timer_handle(void *param);
#endif

#define CHECK_DPU_HANDLE(handle) \
    do { \
        if((handle) == NULL) {\
            LOGE("%s %d handle is NULL\r\n", __func__, __LINE__);\
            return BK_FAIL; \
        } \
    } while(0); \

void sys_drv_int_enable_temp(uint32 param);
void sys_drv_int_disable_temp(uint32 param);


/* dpu_dcnano_dpi_clkgate
*  0:enable clock gate, 1: disable clock gate.
*/
void dpu_dpi_clkgate(uint8_t enable)
{
    dcregBeClockGaterDisable &= ~(1 << 12);
    dcregBeClockGaterDisable |= (!!enable) << 12;  // bit:12, 0: clkgate not disable, dpi clk disable; 1: clkgate disable, dpi clk enable
}

/****sel 0: 240M, 1: 320M; div 1 ~ 32 (div-1 range: 0~31, masked by 0x1f)*****/
void dpu_clk_sel_div(uint32_t sel, uint32_t div)
{
    // uint32_t reg= REG_READ(SYS_AHBP_REG9_ADDR);

    // reg &= ~(0x3F << 19);
    // reg |= ((!!sel) << 19) | (((div - 1) & 0x1f) << 20);

    // REG_WRITE(SYS_AHBP_REG9_ADDR, reg);
    sys_drv_dpu_cksel_clkdiv_set(sel, (div-1)&0x1f);
}



void dpu_clk_en(uint32_t enable)
{
    // uint32_t reg = REG_READ(SYS_AHBP_REGA_ADDR);

    // reg &= ~(1 << 14);
    // reg |= (!!enable) << 14;

    // REG_WRITE(SYS_AHBP_REGA_ADDR, reg);
    bk_pm_clock_ctrl(PM_CLK_ID_DPU, enable ? CLK_PWR_CTRL_PWR_UP : CLK_PWR_CTRL_PWR_DOWN);
}

void dpu_clk_src_set(dpu_clk_src_t clk_src)
{
    dcreg_DPU_Beken_01 = 0x00000002 + clk_src;  // bit:0,  0: dpu clk from 320M/480M; 1: dpu clk from naneng-DPHY internal dpll
}

typedef struct {
    uint8_t sel;
    uint8_t div;
} dpu_sysclk_div_pair_t;

/* Matches legacy ::dpu_clk_sel_div ladder: sel 0 = 240 MHz root, sel 1 = 320 MHz root. */
static const dpu_sysclk_div_pair_t s_dpu_sysclk_pairs[] = {
    {1, 1}, {0, 1}, {1, 2}, {0, 2}, {1, 3}, {0, 3},
    {1, 5}, {0, 4}, {1, 6}, {0, 5}, {1, 7}, {0, 6},
    {1, 9}, {0, 7}, {1, 10}, {0, 8}, {1, 11}, {0, 9},
    {0, 10}, {1, 14}, {0, 11}, {0, 12}, {0, 13}, {0, 14},
    {0, 15}, {0, 16}, {0, 17}, {0, 18}, {0, 20}, {0, 21},
    {0, 24}, {0, 26}, {0, 30}, {0, 32},
};

static uint64_t dpu_sysclk_pair_hz(uint32_t sel, uint32_t div)
{
    const uint64_t root_hz = (sel != 0u) ? 320000000ULL : 240000000ULL;
    return root_hz / (uint64_t)div;
}

void dpu_clk_set(dpu_clk_src_t clk_src, uint32_t pixel_clock_hz)
{
    if (clk_src == DPU_CLK_SRC_SYSCLK) {
        uint32_t req_hz = pixel_clock_hz;

        if (req_hz == 0u) {
            LOGW("%s: pixel_clock_hz=0, using 20 MHz default\n", __func__);
            req_hz = 20000000u;
        }

        size_t best_i = 0;
        uint64_t best_hz = 0;
        uint64_t best_diff = UINT64_MAX;

        for (size_t i = 0; i < sizeof(s_dpu_sysclk_pairs) / sizeof(s_dpu_sysclk_pairs[0]); i++) {
            uint64_t hz = dpu_sysclk_pair_hz(s_dpu_sysclk_pairs[i].sel, s_dpu_sysclk_pairs[i].div);
            uint64_t diff = (hz > (uint64_t)req_hz) ? (hz - (uint64_t)req_hz)
                                                    : ((uint64_t)req_hz - hz);
            if (diff < best_diff) {
                best_diff = diff;
                best_i = i;
                best_hz = hz;
            }
        }

        if (best_diff != 0u) {
            const uint64_t warn_abs = 100000ULL;
            const uint64_t warn_rel = (uint64_t)req_hz / 200ULL;

            if (best_diff > warn_abs && best_diff > warn_rel) {
                LOGW("%s: requested %u Hz, applied ~%llu Hz (nearest SYSCLK divider ladder)\n",
                     __func__, (unsigned)req_hz, (unsigned long long)best_hz);
            }
        }

        dpu_clk_sel_div((uint32_t)s_dpu_sysclk_pairs[best_i].sel,
                        (uint32_t)s_dpu_sysclk_pairs[best_i].div);
        dpu_clk_src_set(0);
    } else {
        dpu_clk_src_set(1);
    }
    dpu_clk_en(1);
}

void dpu_syc_clk_deinit(void)
{
    dpu_clk_en(0);
}


void dpu_sys_interrupt_init(void)
{
    bk_int_isr_register(INT_SRC_DPU, (int_group_isr_t)dpu_isr, NULL);
#if CONFIG_SOC_SMP
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_DPU, 1);
#else
    sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_DPU, 1);
#endif
}

void dpu_sys_interrupt_deinit(void)
{
#if CONFIG_SOC_SMP
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_DPU, 0);
#else
    sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_DPU, 0);
#endif
    bk_int_isr_unregister(INT_SRC_DPU);
}

static uint16_t dpu_pixel_format_set(bk_pixel_format_t bk_format)
{
    uint16_t viv_fomat = 18; // vivARGB8565

    switch(bk_format)
    {
        case BK_PIXEL_FORMAT_RGB565:
            viv_fomat = 16;  // vivRGB565
        break;

        case BK_PIXEL_FORMAT_BGR565:
            viv_fomat = 17;  // vivBGR565
        break;

        case BK_PIXEL_FORMAT_ARGB8565:
            viv_fomat = 18;  // vivARGB8565
        break;
        
        case BK_PIXEL_FORMAT_ABGR8565:
            viv_fomat = 19;  // vivABGR8565
        break;

        case BK_PIXEL_FORMAT_RGBA5658:
            viv_fomat = 20;  // vivRGBA5658
        break;
        
        case BK_PIXEL_FORMAT_BGRA5658:
            viv_fomat = 21;  // vivBGRA5658
        break;

        case BK_PIXEL_FORMAT_RGB888:
            viv_fomat = 22;  // vivRGB888
        break;

        case BK_PIXEL_FORMAT_BGR888:
            viv_fomat = 23;  // vivBGR888
        break;

        case BK_PIXEL_FORMAT_ARGB8888:
            viv_fomat = 24;  // vivARGB8888
        break;

        case BK_PIXEL_FORMAT_ABGR8888:
            viv_fomat = 25;  // vivABGR8888
        break;

        case BK_PIXEL_FORMAT_RGBA8888:
            viv_fomat = 26;  // vivRGBA8888
        break;

        case BK_PIXEL_FORMAT_BGRA8888:
            viv_fomat = 27;  // vivBGRA8888
        break;

        case BK_PIXEL_FORMAT_NV12:
            viv_fomat = 259; // vivNV12
        break;

        case BK_PIXEL_FORMAT_NV21:
            viv_fomat = 266; // vivNV21
        break;

        default:
            bk_printf("UNKNOW FORMAT %d\n", bk_format);
        break;
    }

    // bk_printf("%s: %d\n", __FUNCTION__, viv_fomat);
    return viv_fomat;
}

static bool dpu_runtime_format_supported(const bk_display_pixel_format_config_t *config)
{
    if (config == NULL)
        return false;

    if (config->decompress)
        return config->format == BK_PIXEL_FORMAT_ARGB8888;

    return config->format == BK_PIXEL_FORMAT_RGB565 ||
           config->format == BK_PIXEL_FORMAT_RGB888 ||
           config->format == BK_PIXEL_FORMAT_NV12;
}

static void dpu_fill_video_layer_config(const dpu_config_t *dpu_config, layer_config *layer)
{
    os_memset(layer, 0, sizeof(*layer));

    if (!dpu_config->video.enable)
    {
        return;
    }

    layer->layer_enable = true;
    layer->format = dpu_pixel_format_set(dpu_config->video.format);
    layer->decompress_enable = dpu_config->video.decompress;
    layer->blend_enable = false;
    layer->blend_mode = 0;
    layer->width = dpu_config->video_timing.h_size;
    layer->height = dpu_config->video_timing.v_size;
    layer->disp_x = dpu_config->video.disp_x;
    layer->disp_y = dpu_config->video.disp_y;
    layer->disp_w = dpu_config->video.disp_w;
    layer->disp_h = dpu_config->video.disp_h;
}

static int dpu_flush_complete_handle(void *param)
{
    AVDK_RETURN_ON_FALSE(param, BK_ERR_NULL_PARAM, TAG, "invalid argument");
    dpu_context_t * context = (dpu_context_t *)param;
    LOGV("%s, %d\n", __func__, __LINE__);
    context->refresh_rate++;
    AVDK_MONITOR_DPU_ISR_PLUS();
    DPU_VIDEO_ISR_START();

    for(uint8_t layer = 0; layer < DPU_LAYER_MAX; layer++)
    {
        if (context->update_frame[layer])
        {
            if (context->display_cb[layer])
            {
                context->display_cb[layer](context->display_frame[layer]);
            }
            context->display_frame[layer] = context->update_frame[layer];
            context->display_cb[layer] = context->update_cb[layer];
            context->update_frame[layer] = NULL;
            context->update_cb[layer] = NULL;
            if (layer == DPU_LAYER_VIDEO)
            {
                DPU_VIDEO_FRAME_END();
            }
            rtos_set_event_flags(&context->dpu_event_handle, EVENT_BIT_AVAILABLE);
        }
    }
    DPU_VIDEO_ISR_END();
    return BK_OK;
}

#if CONFIG_DPU_FLUSH_TIMER_DEBUG
static void dpu_flush_timer_handle(void *param)
{
    dpu_context_t *context = (dpu_context_t *)param;
    if (context == NULL)
    {
        return;
    }

    // Skip first statistics to initialize snapshots
    if (context->debug.first_statistics)
    {
        context->debug.last_refresh_rate = context->refresh_rate;
        for (uint8_t layer = 0; layer < DPU_LAYER_MAX; layer++)
        {
            context->debug.last_frame_rate[layer] = context->frame_rate[layer];
        }
        context->debug.first_statistics = false;
        return;
    }

    // Calculate and log refresh rate per second
    uint32_t refresh_delta = context->refresh_rate - context->debug.last_refresh_rate;
    context->debug.refresh_rps = (float)refresh_delta / DPU_FLUSH_TIMER_INTERVAL;
    context->debug.last_refresh_rate = context->refresh_rate;
    LOGI("dpu_flush: rps:%.1f refresh:%u\n", context->debug.refresh_rps, context->refresh_rate);

    // Calculate and log FPS for each layer
    for (uint8_t layer = 0; layer < DPU_LAYER_MAX; layer++)
    {
        uint32_t frame_delta = context->frame_rate[layer] - context->debug.last_frame_rate[layer];
        context->debug.layer_fps[layer] = (float)frame_delta / DPU_FLUSH_TIMER_INTERVAL;
        context->debug.last_frame_rate[layer] = context->frame_rate[layer];
        LOGI(" layer%u:%.1f total:%u\n", layer, context->debug.layer_fps[layer], context->frame_rate[layer]);
    }
}
#endif
bk_err_t dpu_core_init(dpu_config_t * dpu_config, dpu_handle_t *handle)
{
    bk_err_t ret = BK_OK;

    if (*handle != NULL)
    {
        LOGI("%s %d handle is not NULL %x\r\n", __func__, __LINE__, *handle);
        return BK_FAIL;
    }

    dpu_context_t *context = (dpu_context_t*)os_zalloc(sizeof(dpu_context_t));;
    AVDK_GOTO_ON_FALSE(context, BK_ERR_NO_MEM, err, TAG, "no memory for DPI contxet");
    os_memset(context, 0, sizeof(dpu_context_t));

    rtos_init_mutex(&context->flush_mutex);
    if (ret != BK_OK)
    {
        LOGE("%s init mutex failed\n", __func__);
    }
    ret = rtos_init_event_flags(&context->dpu_event_handle);
    if (ret != BK_OK)
    {
        LOGE("%s init event failed\n", __func__);
        goto err;
    }
    dpu_sys_interrupt_init();
    LOGI("%s clk_src: %s, pixel_clock_hz: %u\n", __func__,
         (dpu_config->dpu_clk_src == DPU_CLK_SRC_DPHY_DPLL) ? "DPU_CLK_SRC_DPHY_DPLL" : "DPU_CLK_SRC_SYSCLK",
         (unsigned)dpu_config->pixel_clock_hz);
    dpu_clk_set(dpu_config->dpu_clk_src, dpu_config->pixel_clock_hz);
    uint32_t viv_dc_get_dc_core_len(void);
    uint32_t dpu_buffer_len = viv_dc_get_dc_core_len();
    void viv_dc_set_dc_core(uint8_t *buffer);
    viv_dc_set_dc_core(bk_get_dpu_buffer(dpu_buffer_len));

    ret = dpu_frame_init();
    if (ret != BK_OK)
    {
        LOGE("%s dpu_frame_init failed\n", __func__);
        goto err;
    }
    dpu_dpi_clkgate(0);

    //context->pixel_format = dpu_config->video.format;
    context->h_pixels = dpu_config->video_timing.h_size;
    context->v_pixels = dpu_config->video_timing.v_size;
    context->current_config = *dpu_config;
    //context->dpu_decompress_en = dpu_config->video.decompress;
    //context->dpu_blend_en = dpu_config->graphic.enable;
    //context->dpu_blend_mode = dpu_config->graphic.blend_mode;

    LOGI("%s, %dx%d, %d, %d, %d, %d, %d, %d\n", __func__, dpu_config->video_timing.h_size, dpu_config->video_timing.v_size, dpu_config->video_timing.hsync_pulse_width, dpu_config->video_timing.hsync_back_porch, dpu_config->video_timing.hsync_front_porch, dpu_config->video_timing.vsync_pulse_width, dpu_config->video_timing.vsync_back_porch, dpu_config->video_timing.vsync_front_porch);

    dpu_frame_display_config(dpu_config->video_timing.h_size,
                             dpu_config->video_timing.v_size,
                             dpu_config->video_timing.hsync_pulse_width,
                             dpu_config->video_timing.hsync_back_porch,
                             dpu_config->video_timing.hsync_front_porch,
                             dpu_config->video_timing.vsync_pulse_width,
                             dpu_config->video_timing.vsync_back_porch,
                             dpu_config->video_timing.vsync_front_porch
                            );

    dpu_frame_flush_complete_register(dpu_flush_complete_handle, (void *)context);
    context->draw = dpu_core_flush;

#if CONFIG_DPU_FLUSH_TIMER_DEBUG
    /* Launch periodic timer to report refresh and frame rates */
    if (rtos_init_timer(&context->debug.timer, DPU_FLUSH_TIMER_INTERVAL * 1000, dpu_flush_timer_handle, context) != BK_OK)
    {
        LOGE("%s init timer failed\n", __func__);
    }
    else
    {
        if (rtos_start_timer(&context->debug.timer) != BK_OK)
        {
            LOGE("%s start timer failed\n", __func__);
            rtos_deinit_timer(&context->debug.timer);
        }
        else
        {
            context->debug.timer_started = true;
        }
    }
#endif

    *handle = context;
    LOGI("%s dpu_core_init success\n", __func__);

    return BK_OK;

err:   
    LOGI("%s, %d ERROR ",__func__, __LINE__);
    if (context)
    {
        os_free(context);
    }
    return ret;
    
}

bk_err_t dpu_core_layer_config(dpu_config_t * dpu_config, dpu_handle_t *handle)
{
    bk_err_t ret = BK_OK;
    dpu_context_t *context = (dpu_context_t*)*handle;
    layer_config layers[2] = {0};

    dpu_fill_video_layer_config(dpu_config, &layers[DPU_LAYER_VIDEO]);

    if (dpu_config->graphic.enable)
    {
        layers[DPU_LAYER_GRAPHIC].layer_enable = true;
        layers[DPU_LAYER_GRAPHIC].format = dpu_pixel_format_set(dpu_config->graphic.format);
        layers[DPU_LAYER_GRAPHIC].decompress_enable = false;
        layers[DPU_LAYER_GRAPHIC].blend_enable = true;
        layers[DPU_LAYER_GRAPHIC].blend_mode = dpu_config->graphic.blend_mode;
        layers[DPU_LAYER_GRAPHIC].width = dpu_config->video_timing.h_size;
        layers[DPU_LAYER_GRAPHIC].height = dpu_config->video_timing.v_size;
        layers[DPU_LAYER_GRAPHIC].disp_x = dpu_config->graphic.disp_x;
        layers[DPU_LAYER_GRAPHIC].disp_y = dpu_config->graphic.disp_y;
        layers[DPU_LAYER_GRAPHIC].disp_w = dpu_config->graphic.disp_w;
        layers[DPU_LAYER_GRAPHIC].disp_h = dpu_config->graphic.disp_h;
    }

    dpu_frame_layer_config(layers);
    context->current_config = *dpu_config;
    context->display_dirty = true;

    return ret;
}

bk_err_t dpu_core_runtime_switch(dpu_handle_t *handle, const bk_display_pixel_format_config_t *config)
{
    dpu_context_t *context;
    beken_event_flags_t wait_event = 0;
    layer_config video_layer = {0};
    bk_err_t ret = BK_OK;

    AVDK_RETURN_ON_FALSE(handle && config, BK_ERR_NULL_PARAM, TAG, "invalid argument");

    context = (dpu_context_t *)*handle;
    AVDK_RETURN_ON_FALSE(context, BK_ERR_NULL_PARAM, TAG, "invalid handle");
    AVDK_RETURN_ON_FALSE(dpu_runtime_format_supported(config),
                         BK_ERR_PARAM, TAG, "runtime switch only supports RGB565, RGB888, NV12 and compressed ARGB8888");

    rtos_lock_mutex(&context->flush_mutex);

    while (context->update_frame[DPU_LAYER_VIDEO])
    {
        wait_event = rtos_wait_for_event_flags(&context->dpu_event_handle,
                EVENT_BIT_AVAILABLE, true, WAIT_FOR_ANY_EVENT, BEKEN_WAIT_FOREVER);

        if ((wait_event & EVENT_BIT_AVAILABLE) != EVENT_BIT_AVAILABLE || context->update_frame[DPU_LAYER_VIDEO])
        {
            LOGE("%s wait pending frame failed\n", __func__);
            ret = BK_FAIL;
            goto exit;
        }
    }

    context->current_config.video.format = config->format;
    context->current_config.video.decompress = config->decompress;
    dpu_fill_video_layer_config(&context->current_config, &video_layer);

    ret = dpu_frame_switch_video_config(&video_layer);
    if (ret == BK_OK)
    {
        context->display_dirty = true;
    }

exit:
    rtos_unlock_mutex(&context->flush_mutex);
    return ret;
}

bk_err_t dpu_core_deinit(dpu_handle_t *handle)
{
    AVDK_RETURN_ON_FALSE(handle, BK_ERR_NULL_PARAM, TAG, "invalid argument");
    AVDK_RETURN_ON_FALSE(*handle, BK_ERR_NULL_PARAM, TAG, "invalid handle");
    dpu_context_t *context = (dpu_context_t*)*handle;

    dpu_frame_deinit();

    dpu_sys_interrupt_deinit();

#if CONFIG_DPU_FLUSH_TIMER_DEBUG
    if (context->debug.timer_started)
    {
        if (rtos_stop_timer(&context->debug.timer) != BK_OK)
        {
            LOGW("%s stop timer failed\n", __func__);
        }
        if (rtos_deinit_timer(&context->debug.timer) != BK_OK)
        {
            LOGW("%s deinit timer failed\n", __func__);
        }
        context->debug.timer_started = false;
    }
#endif

    //dpu_syc_clk_deinit();

    if (context->dpu_event_handle)
    {
        rtos_deinit_event_flags(&context->dpu_event_handle);
    }
    {
        GLOBAL_INT_DECLARATION();
        for (uint8_t layer = 0; layer < DPU_LAYER_MAX; layer++)
        {
            void *uf = NULL;
            flush_free_cb_t ucb = NULL;
            void *df = NULL;
            flush_free_cb_t dcb = NULL;

            GLOBAL_INT_DISABLE();
            uf = context->update_frame[layer];
            ucb = context->update_cb[layer];
            df = context->display_frame[layer];
            dcb = context->display_cb[layer];
            context->update_frame[layer] = NULL;
            context->update_cb[layer] = NULL;
            context->display_frame[layer] = NULL;
            context->display_cb[layer] = NULL;
            GLOBAL_INT_RESTORE();

            if (uf && ucb)
            {
                LOGV("%s free update frame %p\n", __func__, uf);
                ucb(uf);
            }
            if (df && dcb && df != uf)
            {
                LOGV("%s free display frame %p\n", __func__, df);
                dcb(df);
            }
        }
    }

    if (context->flush_mutex)
    {
        rtos_deinit_mutex(&context->flush_mutex);
    }   

    os_free(context);
    *handle = NULL;

    LOGI("%s\n", __func__);
    return BK_OK;
}

bk_err_t dpu_core_flush_stop(dpu_handle_t *handle)
{
    AVDK_RETURN_ON_FALSE(handle, BK_ERR_NULL_PARAM, TAG, "invalid argument");
    dpu_frame_trigger(0);

    return BK_OK;
}

bk_err_t dpu_core_flush_restart(dpu_handle_t *handle)
{
    AVDK_RETURN_ON_FALSE(handle, BK_ERR_NULL_PARAM, TAG, "invalid argument");
    dpu_frame_trigger(1);

    return BK_OK;
}


bk_err_t dpu_core_flush(dpu_handle_t *handle, dpu_layer_t layer, void *buff, flush_free_cb_t cb)
{
    AVDK_RETURN_ON_FALSE(handle, BK_ERR_NULL_PARAM, TAG, "invalid argument");
    AVDK_RETURN_ON_FALSE(*handle, BK_ERR_NULL_PARAM, TAG, "invalid handle");
    bk_err_t ret = BK_OK;
    dpu_context_t *context = (dpu_context_t*)*handle;
    void *old_display_frame = NULL;
    flush_free_cb_t old_display_cb = NULL;

    /* Validate layer index before accessing layer-specific resources */
    if (layer >= DPU_LAYER_MAX)
    {
        LOGE("%s invalid layer:%d\n", __func__, layer);
        return BK_ERR_PARAM;
    }

    if (layer == DPU_LAYER_VIDEO)
    {
        DPU_VIDEO_FRAME_START();
        AVDK_MONITOR_DPU_FPS_PLUS();
    }

    rtos_lock_mutex(&context->flush_mutex);
    if ((!context->display_frame[layer]) || (context->display_dirty == true))
    {
        old_display_frame = context->display_frame[layer];
        old_display_cb = context->display_cb[layer];
        ret = dpu_frame_commit(layer, buff);
        if (ret == BK_OK)
        {
            context->display_frame[layer] = buff;
            context->display_cb[layer] = cb;
            context->display_dirty = false;
            context->frame_rate[layer]++;
            if (old_display_frame && old_display_cb && old_display_frame != buff)
            {
                old_display_cb(old_display_frame);
            }
            LOGI("%s commit success, cb: %p, buff: %p \n", __func__, cb, buff);
        }
        else if (cb)
        {
            cb(buff);
            LOGI("%s commit fail, cb: %p, buff: %p \n", __func__, cb, buff);
        }
    }
    else
    {
        beken_event_flags_t wait_event = 0;

        while (context->update_frame[layer])
        {
            wait_event = rtos_wait_for_event_flags (&context->dpu_event_handle,
                    EVENT_BIT_AVAILABLE, true, WAIT_FOR_ANY_EVENT, BEKEN_WAIT_FOREVER);

            if ((wait_event & EVENT_BIT_AVAILABLE) != EVENT_BIT_AVAILABLE)
            {
                LOGE("%s bit error\n", __func__);
                ret = BK_FAIL;
                goto exit;
            }
        }

        context->update_frame[layer] = buff;
        context->update_cb[layer] = cb;
        context->frame_rate[layer]++;
        ret = dpu_frame_update(layer, buff);
        if (ret != BK_OK)
        {
            context->update_frame[layer] = NULL;
            context->update_cb[layer] = NULL;
            if (cb)
            {
                cb(buff);
            }
        }
    }
exit:
    rtos_unlock_mutex(&context->flush_mutex);
    return ret;
}

uint32_t dpu_core_get_flush_addr(dpu_handle_t *handle)
{
    AVDK_RETURN_ON_FALSE(handle, BK_ERR_NULL_PARAM, TAG, "invalid argument");
    return dpu_frame_get_layer_address(0);
}




