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
#include "cli.h"
#include "lcd_example.h"
#include <modules/pm.h>
#include <avdk_check.h>
#include <avdk_error.h>

#define TAG "rgb_lcd"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define DEFAULT_RGB_PANEL_NAME   "st7701sn_rgb_480x854" //"st7282_rgb_480x272"
#define DEFAULT_RGB_FORMAT      BK_PIXEL_FORMAT_RGB565

/* Shared with CLI: power-on display ctx, also used by the switch-format IT case. */
static display_ctx_t s_rgb_disp_ctx;

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

/* IT case: cycle RGB565/ARGB8888 to verify the runtime switch API, then auto-close. */
static void cli_rgb_lcd_switch_format_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    rgb_lcd_switch_format_test(&s_rgb_disp_ctx, DEFAULT_RGB_PANEL_NAME);
}

static const struct cli_command s_rgb_lcd_cli_commands[] =
{
    {"rgb_lcd_switch_format", "switch RGB565/ARGB8888 to verify switch API", cli_rgb_lcd_switch_format_cmd},
};

#define RGB_LCD_CLI_CMDS_COUNT  (sizeof(s_rgb_lcd_cli_commands) / sizeof(struct cli_command))

static int cli_rgb_lcd_example_init(void)
{
    return cli_register_commands(s_rgb_lcd_cli_commands, RGB_LCD_CLI_CMDS_COUNT);
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

    cli_rgb_lcd_example_init();

    /* Power-on IT case: open RGB565, hold ~20s, then auto-close, log [RESULT][PASS] at end. */
    rgb_lcd_rgb565_test(&s_rgb_disp_ctx, DEFAULT_RGB_PANEL_NAME);

    return 0;
}
