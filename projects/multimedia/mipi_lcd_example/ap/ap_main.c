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
// #define DEFAULT_MIPI_PANEL_NAME   "hx8394f_mipi_720x1280"

/** Shared with CLI: lcd_example_flush_thread_stop() then lcd_example_dsi_close() for teardown. */
static display_ctx_t s_mipi_disp_ctx;

#define SYS_ANA_REG_BASE    (0x44010000)
#define LDO_ANA_REG         (0x69)

static void bk_lodoen_enable(void)
{
    uint32_t reg = REG_READ(SYS_ANA_REG_BASE + LDO_ANA_REG * 4);
    reg |= (0xF << 28) | (0x2 << 23) | (0x7 << 19) | (0x7 << 15);
    reg &= ~(0xF << 11);
    reg |= (0x8 << 11);
    REG_WRITE(SYS_ANA_REG_BASE + LDO_ANA_REG * 4, reg);
}

void cli_mipi_lcd_switch_format(const bk_display_pixel_format_config_t *config, const char *name)
{
    bk_pixel_format_t old_format;
    uint8_t old_decompress;

    if (s_mipi_disp_ctx.dis_bus_handle == NULL)
    {
        LOGI("MIPI LCD is off\r\n");
        return;
    }

    old_format = s_mipi_disp_ctx.format;
    old_decompress = s_mipi_disp_ctx.decompress;
    if (lcd_example_flush_thread_stop(&s_mipi_disp_ctx) != AVDK_ERR_OK)
    {
        LOGE("lcd_example_flush_thread_stop failed\r\n");
        return;
    }

    if (bk_display_ioctl(s_mipi_disp_ctx.dpu_ctlr_handle, BK_DISPLAY_IOCTL_DPU_PIXEL_FORMAT, (void *)config) != AVDK_ERR_OK)
    {
        LOGE("bk_display_ioctl runtime switch failed\r\n");
        s_mipi_disp_ctx.format = old_format;
        s_mipi_disp_ctx.decompress = old_decompress;
        lcd_example_flush_thread_start(&s_mipi_disp_ctx);
        return;
    }

    s_mipi_disp_ctx.format = config->format;
    s_mipi_disp_ctx.decompress = config->decompress;
    if (lcd_example_flush_thread_start(&s_mipi_disp_ctx) != AVDK_ERR_OK)
    {
        LOGE("lcd_example_flush_thread_start failed after switch\r\n");
        s_mipi_disp_ctx.format = old_format;
        s_mipi_disp_ctx.decompress = old_decompress;
        return;
    }

    LOGI("MIPI LCD switched to %s\r\n", name);
}

/** Usage: mipi_lcd open | mipi_lcd close | mipi_lcd switch rgb565|rgb888|nv12|argb8888 */
static void cli_mipi_lcd_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    static const bk_display_pixel_format_config_t s_rgb565_runtime_cfg = {
        .format = BK_PIXEL_FORMAT_RGB565,
        .decompress = false,
    };
    static const bk_display_pixel_format_config_t s_rgb888_runtime_cfg = {
        .format = BK_PIXEL_FORMAT_RGB888,
        .decompress = false,
    };
    static const bk_display_pixel_format_config_t s_nv12_runtime_cfg = {
        .format = BK_PIXEL_FORMAT_NV12,
        .decompress = false,
    };
    static const bk_display_pixel_format_config_t s_dec_argb8888_runtime_cfg = {
        .format = BK_PIXEL_FORMAT_ARGB8888,
        .decompress = true,
    };

    if (argc < 2)
    {
        LOGI("usage: mipi_lcd open | mipi_lcd close | mipi_lcd switch rgb565|rgb888|nv12|argb8888\r\n");
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

        if (lcd_example_flush_thread_start(&s_mipi_disp_ctx) != AVDK_ERR_OK)
        {
            LOGE("lcd_example_flush_thread_start failed, closing\r\n");
            lcd_example_dsi_close(&s_mipi_disp_ctx);
            return;
        }
        LOGI("MIPI LCD on (panel %s, ARGB8888)\r\n", DEFAULT_MIPI_PANEL_NAME);
        return;
    }
    else if (os_strcmp(argv[1], "switch") == 0)
    {
        if (argc < 3)
        {
            LOGI("usage: mipi_lcd switch rgb565|rgb888|nv12|argb8888\r\n");
            return;
        }

        if (os_strcmp(argv[2], "argb8888") == 0)
        {
            cli_mipi_lcd_switch_format(&s_dec_argb8888_runtime_cfg, "compressed ARGB8888");
            return;
        }
        if (os_strcmp(argv[2], "rgb565") == 0)
        {
            cli_mipi_lcd_switch_format(&s_rgb565_runtime_cfg, "RGB565");
            return;
        }
        if (os_strcmp(argv[2], "rgb888") == 0)
        {
            cli_mipi_lcd_switch_format(&s_rgb888_runtime_cfg, "RGB888");
            return;
        }
        if (os_strcmp(argv[2], "nv12") == 0)
        {
            cli_mipi_lcd_switch_format(&s_nv12_runtime_cfg, "NV12");
            return;
        }

        LOGI("unknown switch mode \"%s\"\r\n", argv[2]);
        return;
    }
    if (os_strcmp(argv[1], "close") == 0)
    {
        if (s_mipi_disp_ctx.dis_bus_handle == NULL)
        {
            LOGI("MIPI LCD already off\r\n");
            return;
        }

        if (lcd_example_flush_thread_stop(&s_mipi_disp_ctx) != AVDK_ERR_OK)
            LOGE("lcd_example_flush_thread_stop failed\r\n");
        if (lcd_example_dsi_close(&s_mipi_disp_ctx) != AVDK_ERR_OK)
            LOGE("lcd_example_dsi_close failed\r\n");
        return;
    }

    LOGI("unknown arg \"%s\", usage: mipi_lcd open | mipi_lcd close | mipi_lcd switch rgb565|rgb888|nv12|argb8888\r\n", argv[1]);
}

static const struct cli_command s_mipi_lcd_cli_commands[] =
{
    {"mipi_lcd", "mipi_lcd open | mipi_lcd close | mipi_lcd switch rgb565|rgb888|nv12|argb8888", cli_mipi_lcd_cmd},
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
