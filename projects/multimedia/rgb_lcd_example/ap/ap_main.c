#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <driver/psram.h>
#include <avdk_check.h>
#include <avdk_error.h>
#include <components/bk_frame_buffer.h>
#include "sys_driver.h"
#include "media_service.h"
#include <common/avdk_pixel_types.h>
#include "lcd_example.h"
#include <modules/pm.h>
#include <avdk_check.h>
#include <avdk_error.h>

#define TAG "rgb_lcd"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define DEFAULT_RGB_PANEL_NAME   "st7701sn_rgb_480x854" //"st7282_rgb_480x272"
#define DEFAULT_RGB_FORMAT      BK_PIXEL_FORMAT_RGB565

static avdk_err_t bk_lodoen_enable(void)
{
    pm_auxldo_ctrl_cfg_t auxldo_cfg = {0};
    auxldo_cfg.ldo = AUXLDOS_SEL_1P8V; 
    auxldo_cfg.out = PM_AUXLDO_1P8V_OUT_1P8V;
    auxldo_cfg.user = PM_AUXLDO_USER_DISPLAY;
    auxldo_cfg.state = PM_AUXLDO_ENABLE;
    AVDK_RETURN_ON_ERROR(bk_pm_auxldo_ctrl_vote(&auxldo_cfg), TAG, "display 1p8v ldo vote failed");
    return AVDK_ERR_OK;
}


int main(void)
{
    bk_init();
    media_service_init();

    #if (BK_IPC_UT_TEST)
    bk_ipc_test_init();
    #endif
    bk_printf("%s, %d, m55 running...\r\n", __func__, __LINE__);


    bk_printf("lodoen enable start...\r\n");

    bk_lodoen_enable();

    bk_printf("lodoen enable...\r\n");
    #ifdef CONFIG_FRAME_BUFFER
    bk_frame_buffer_init();
    #endif

    /* Power-on display: open RGB panel with panel name and format, then start flush thread. */
    static display_ctx_t s_rgb_disp_ctx;
    os_memset(&s_rgb_disp_ctx, 0, sizeof(s_rgb_disp_ctx));
    if (lcd_example_rgb_open(&s_rgb_disp_ctx, DEFAULT_RGB_PANEL_NAME, DEFAULT_RGB_FORMAT) == AVDK_ERR_OK)
        lcd_example_flush_thread_start(&s_rgb_disp_ctx);

    return 0;
}
