#include <os/os.h>
#include <os/mem.h>
#include <stdio.h>
#include <common/bk_include.h>
#include <components/log.h>
#include <avdk_error.h>
#include <avdk_check.h>
#include "dpu_reg.h"
 #include "sys_driver.h"
#include "dpu_driver.h"
#include "dpu_core.h"
#include "components/bk_lcd_types.h"
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

void dpu_clk_set(dpu_clk_src_t (clk_src), lcd_clk_t clk)
{
    /****sel 0: 240M, 1: 320M; div 1 ~ 32*****/
    if (clk > LCD_80M || (clk_src == DPU_CLK_SRC_NANENG_DPHY_INTERNAL_DPLL))
    {   
        clk_src = DPU_CLK_SRC_NANENG_DPHY_INTERNAL_DPLL;
    }
    else
    {
        clk_src = DPU_CLK_SRC_320M_480M;
        switch (clk)
        {
            // case LCD_320M:
            //     dpu_clk_sel_div(1, 1);    // 320/1 = 320Mhz
            //     break;

            // case LCD_240M:
            //     dpu_clk_sel_div(0, 1);    // 240/1 = 240Mhz
            //     break;

            // case LCD_160M:
            //     dpu_clk_sel_div(1, 2);    // 320/2 = 160Mhz
            //     break;

            // case LCD_120M:
            //     dpu_clk_sel_div(0, 2);    // 240/2 = 120Mhz
            //     break;

            case LCD_106M:
                dpu_clk_sel_div(1, 3);    // 320/3 = 106.67Mhz (106M)
                break;

            case LCD_80M:
                dpu_clk_sel_div(0, 3);    // 240/3 = 80Mhz
                break;

            case LCD_64M:
                dpu_clk_sel_div(1, 5);    // 320/5 = 64Mhz
                break;

            case LCD_60M:
                dpu_clk_sel_div(0, 4);    // 240/4 = 60Mhz
                break;

            case LCD_53M:
                dpu_clk_sel_div(1, 6);    // 320/6 = 53.33Mhz (53M)
                break;

            case LCD_48M:
                dpu_clk_sel_div(0, 5);    // 240/5 = 48Mhz
                break;

            case LCD_45M:
                dpu_clk_sel_div(1, 7);    // 320/7 = 45.71Mhz (45M)
                break;

            case LCD_40M:
                dpu_clk_sel_div(0, 6);    // 240/6 = 40Mhz
                break;

            case LCD_35M:
                dpu_clk_sel_div(1, 9);    // 320/9 = 35.56Mhz (35M)
                break;

            case LCD_34M:
                dpu_clk_sel_div(0, 7);    // 240/7 = 34.29Mhz (34M)
                break;

            case LCD_32M:
                dpu_clk_sel_div(1, 10);   // 320/10 = 32Mhz
                break;

            case LCD_30M:
                dpu_clk_sel_div(0, 8);    // 240/8 = 30Mhz
                break;

            case LCD_29M:
                dpu_clk_sel_div(1, 11);   // 320/11 = 29.09Mhz (29M)
                break;

            case LCD_26M:
                dpu_clk_sel_div(0, 9);    // 240/9 = 26.67Mhz (26M)
                break;

            case LCD_24M:
                dpu_clk_sel_div(0, 10);   // 240/10 = 24Mhz
                break;

            case LCD_22M:
                dpu_clk_sel_div(1, 14);   // 320/14 = 22.86Mhz (22M)
                break;

            case LCD_21M:
                dpu_clk_sel_div(0, 11);   // 240/11 = 21.82Mhz (21M)
                break;

            case LCD_20M:
                dpu_clk_sel_div(0, 12);   // 240/12 = 20Mhz
                break;

            case LCD_18M:
                dpu_clk_sel_div(0, 13);   // 240/13 = 18.46Mhz (18M)
                break;

            case LCD_17M:
                dpu_clk_sel_div(0, 14);   // 240/14 = 17.14Mhz (17M)
                break;

            case LCD_16M:
                dpu_clk_sel_div(0, 15);   // 240/15 = 16Mhz
                break;

            case LCD_15M:
                dpu_clk_sel_div(0, 16);   // 240/16 = 15Mhz
                break;

            case LCD_14M:
                dpu_clk_sel_div(0, 17);   // 240/17 = 14.12Mhz (14M)
                break;

            case LCD_13M:
                dpu_clk_sel_div(0, 18);   // 240/18 = 13.33Mhz (13M)
                break;

            case LCD_12M:
                dpu_clk_sel_div(0, 20);   // 240/20 = 12Mhz
                break;

            case LCD_11M:
                dpu_clk_sel_div(0, 21);   // 240/21 = 11.43Mhz (11M)
                break;

            case LCD_10M:
                dpu_clk_sel_div(0, 24);   // 240/24 = 10Mhz
                break;

            case LCD_9M:
                dpu_clk_sel_div(0, 26);   // 240/26 = 9.23Mhz (9M)
                break;

            case LCD_8M:
                dpu_clk_sel_div(0, 30);   // 240/30 = 8Mhz
                break;

            case LCD_7M:
                dpu_clk_sel_div(0, 32);   // 240/32 = 7.5Mhz (7M)
                break;
                
            default:
                dpu_clk_sel_div(0, 12);   // 240/12 = 20Mhz default
                bk_printf("%s unknow clk: %d\n", __FUNCTION__, clk);
                break;
        }
    }
    dpu_clk_en(1);
    dpu_clk_src_set(clk_src);
}

void dpu_syc_clk_deinit(void)
{
    dpu_clk_en(0);
}


void dpu_sys_interrupt_init(void)
{
    bk_int_isr_register(INT_SRC_DPU, (int_group_isr_t)dpu_isr, NULL);
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_DPU, 1);
}

void dpu_sys_interrupt_deinit(void)
{
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_DPU, 0);
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
        }
    }
    rtos_set_event_flags(&context->dpu_event_handle, EVENT_BIT_AVAILABLE);
    
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
    dpu_clk_set(dpu_config->dpu_clk_src, dpu_config->dpi_clock_freq_mhz);
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

    if (dpu_config->video.enable)
    {
        layers[DPU_LAYER_VIDEO].layer_enable = true;
        layers[DPU_LAYER_VIDEO].format = dpu_pixel_format_set(dpu_config->video.format);
        layers[DPU_LAYER_VIDEO].decompress_enable = dpu_config->video.decompress;
        layers[DPU_LAYER_VIDEO].blend_enable = false;
        layers[DPU_LAYER_VIDEO].blend_mode = 0;
        layers[DPU_LAYER_VIDEO].width = dpu_config->video_timing.h_size;
        layers[DPU_LAYER_VIDEO].height = dpu_config->video_timing.v_size;
        layers[DPU_LAYER_VIDEO].disp_x = dpu_config->video.disp_x;
        layers[DPU_LAYER_VIDEO].disp_y = dpu_config->video.disp_y;
        layers[DPU_LAYER_VIDEO].disp_w = dpu_config->video.disp_w;
        layers[DPU_LAYER_VIDEO].disp_h = dpu_config->video.disp_h;
    }

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
    
    context->display_dirty = true;

    return ret;
}

bk_err_t dpu_core_deinit(dpu_handle_t *handle)
{
    AVDK_RETURN_ON_FALSE(handle, BK_ERR_NULL_PARAM, TAG, "invalid argument");
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

    if (context->update_frame)
    {
        for (uint8_t layer = 0; layer < DPU_LAYER_MAX; layer++)
        {
            if (context->update_frame[layer])
            {
                LOGV("%s free update frame %p\n", __func__, context->update_frame[layer]);
                context->update_cb[layer](context->update_frame[layer]);
            }
        }
    }
    if (context->display_frame)
    {
        for (uint8_t layer = 0; layer < DPU_LAYER_MAX; layer++)
        {
            if (context->display_frame[layer])
            {
                LOGV("%s free display frame %p\n", __func__, context->display_frame[layer]);
                context->display_cb[layer](context->display_frame[layer]);
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
    bk_err_t ret = BK_OK;
    dpu_context_t *context = (dpu_context_t*)*handle;

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
        context->display_frame[layer] = buff;
        context->display_cb[layer] = cb;
        ret = dpu_frame_commit(layer, buff);
        context->display_dirty = false;
        context->frame_rate[layer]++;
        LOGI("%s %s, cb: %p \n", __func__, (ret == BK_OK) ? "success" : "fail", cb);
    }
    else
    {
        beken_event_flags_t wait_event = 0;

        if (context->update_frame[layer])
        {
            wait_event = rtos_wait_for_event_flags (&context->dpu_event_handle,
                    EVENT_BIT_AVAILABLE, true, WAIT_FOR_ANY_EVENT, BEKEN_WAIT_FOREVER);

            if ((wait_event & EVENT_BIT_AVAILABLE) != EVENT_BIT_AVAILABLE)
            {
                LOGE("%s bit error\n", __func__);
                ret = BK_FAIL;
                goto exit;
            }

            if (context->update_frame[layer])
            {
                LOGE("%s frame error\n", __func__);
                ret = BK_FAIL;
                goto exit;
            }
        }

        context->update_frame[layer] = buff;
        context->update_cb[layer] = cb;
        context->frame_rate[layer]++;
        ret = dpu_frame_update(layer, buff);
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




