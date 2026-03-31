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
#include <os/str.h>
#include "cli.h"
#include "lcd_example.h"
#include <components/bk_display.h>

#define TAG "mipi_lcd_ap"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

/* Default MIPI panel (change here to switch panel). */
#define DEFAULT_MIPI_PANEL_NAME   "hx8399c_mipi_1080x1920"

/** Shared with CLI: stop flush thread + DSI/panel teardown via lcd_example_dsi_close(). */
static display_ctx_t s_mipi_disp_ctx;

#define SYS_ANA_REG_BASE    (0x44010000)
#define LDO_ANA_REG         (0x69)

#define SYS_M55_BASE_ADDR    (0x48000000)
#define SYS_GPIO_BASE_ADDR    (0x44000400)


static void bk_lodoen_enable(void)
{
    uint32_t reg = REG_READ(SYS_ANA_REG_BASE + LDO_ANA_REG * 4);
    reg |= (0xF << 28) | (0x2 << 23) | (0x7 << 19) | (0x7 << 15);
    reg &= ~(0xF << 11);
    reg |= (0x8 << 11);
    REG_WRITE(SYS_ANA_REG_BASE + LDO_ANA_REG * 4, reg);
}

/** Usage: mipi_lcd open | mipi_lcd close */
static void cli_mipi_lcd_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc < 2)
    {
        LOGI("usage: mipi_lcd open | mipi_lcd close | mipi_lcd open stress_test\r\n");
        return;
    }

    if (os_strcmp(argv[1], "open") == 0)
    {
        if (s_mipi_disp_ctx.dis_bus_handle != NULL)
        {
            LOGI("MIPI LCD already on\r\n");
            return;
        }

        os_memset(&s_mipi_disp_ctx, 0, sizeof(s_mipi_disp_ctx));
        if (lcd_example_dsi_open(&s_mipi_disp_ctx, DEFAULT_MIPI_PANEL_NAME, BK_PIXEL_FORMAT_ARGB8888) != AVDK_ERR_OK)
        {
            LOGE("lcd_example_dsi_open failed\r\n");
            return;
        }
        if (os_strcmp(argv[2], "stress_test") == 0)
        {
            bk_display_ioctl(s_mipi_disp_ctx.dpu_ctlr_handle, BK_DISPLAY_IOCTL_DPU_PIXEL_CLK, (void *)LCD_96M);
        }
        if (lcd_example_flush_thread_start(&s_mipi_disp_ctx) != AVDK_ERR_OK)
        {
            LOGE("lcd_example_flush_thread_start failed, closing\r\n");
            lcd_example_dsi_close(&s_mipi_disp_ctx);
            return;
        }
        LOGI("MIPI LCD on (panel %s, ARGB8888)\r\n", DEFAULT_MIPI_PANEL_NAME);
        return;
    }

    if (os_strcmp(argv[1], "close") == 0)
    {
        if (s_mipi_disp_ctx.dis_bus_handle == NULL)
        {
            LOGI("MIPI LCD already off\r\n");
            return;
        }

        if (lcd_example_dsi_close(&s_mipi_disp_ctx) != AVDK_ERR_OK)
            LOGE("lcd_example_dsi_close failed\r\n");
        return;
    }

    LOGI("unknown arg \"%s\", usage: mipi_lcd open | mipi_lcd close\r\n", argv[1]);
}

static const struct cli_command s_mipi_lcd_cli_commands[] =
{
    {"mipi_lcd", "mipi_lcd open | mipi_lcd close | mipi_lcd open stress_test", cli_mipi_lcd_cmd},
};

#define MIPI_LCD_CLI_CMDS_COUNT  (sizeof(s_mipi_lcd_cli_commands) / sizeof(struct cli_command))

static int cli_mipi_lcd_example_init(void)
{
    return cli_register_commands(s_mipi_lcd_cli_commands, MIPI_LCD_CLI_CMDS_COUNT);
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

    cli_mipi_lcd_example_init();

    /* Power-on display: open default MIPI panel with ARGB8888 and start flush thread. */
    os_memset(&s_mipi_disp_ctx, 0, sizeof(s_mipi_disp_ctx));
    if (lcd_example_dsi_open(&s_mipi_disp_ctx, DEFAULT_MIPI_PANEL_NAME, BK_PIXEL_FORMAT_ARGB8888) == AVDK_ERR_OK)
        lcd_example_flush_thread_start(&s_mipi_disp_ctx);

    return 0;
}
