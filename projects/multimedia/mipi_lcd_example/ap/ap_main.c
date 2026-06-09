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


#define TAG "mipi_lcd"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

/* Default MIPI panel (change here to switch panel). */
 #define DEFAULT_MIPI_PANEL_NAME   "hx8399c_mipi_1080x1920"
// #define DEFAULT_MIPI_PANEL_NAME   "hx8394f_mipi_720x1280"

/** Shared with CLI: lcd_example_flush_thread_stop() then lcd_example_dsi_close() for teardown. */
static display_ctx_t s_mipi_disp_ctx;

void cli_mipi_lcd_switch_format(const bk_display_pixel_format_config_t *config, const char *name)
{
    if (s_mipi_disp_ctx.dis_bus_handle == NULL)
    {
        LOGI("MIPI LCD is off\r\n");
        return;
    }

    if (mipi_lcd_do_switch(&s_mipi_disp_ctx, config) == AVDK_ERR_OK)
        LOGI("MIPI LCD switched to %s\r\n", name);
    else
        LOGE("MIPI LCD switch to %s failed\r\n", name);
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

/* IT case: cycle RGB565/RGB888/ARGB8888 to verify the runtime switch API, then auto-close. */
static void cli_mipi_lcd_switch_format_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    mipi_lcd_switch_format_test(&s_mipi_disp_ctx, DEFAULT_MIPI_PANEL_NAME);
}

/* On/off stress test: lcd_stress on_off <time_ms> loops open->flush->close(power down); lcd_stress stop ends it. */
static void cli_lcd_stress_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc >= 2 && os_strcmp(argv[1], "stop") == 0)
    {
        lcd_stress_stop();
        return;
    }
    if (argc >= 3 && os_strcmp(argv[1], "on_off") == 0)
    {
        uint32_t on_ms = (uint32_t)os_strtoul(argv[2], NULL, 10);
        lcd_stress_on_off_start(DEFAULT_MIPI_PANEL_NAME, on_ms);
        return;
    }
    LOGI("usage: lcd_stress on_off <time_ms> | lcd_stress stop\r\n");
}

static const struct cli_command s_mipi_lcd_cli_commands[] =
{
    {"mipi_lcd", "mipi_lcd open | mipi_lcd close | mipi_lcd switch rgb565|rgb888|nv12|argb8888", cli_mipi_lcd_cmd},
    {"mipi_lcd_switch_format", "switch RGB565/RGB888/ARGB8888 to verify switch API", cli_mipi_lcd_switch_format_cmd},
    {"lcd_stress", "lcd_stress on_off <time_ms> | lcd_stress stop", cli_lcd_stress_cmd},
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


    // bk_lodoen_enable();
    #ifdef CONFIG_FRAME_BUFFER
    bk_frame_buffer_init();
    #endif

    cli_mipi_lcd_example_init();

    /* Power-on IT case: open ARGB8888, log [RESULT][PASS], hold ~30s, then auto-close. */
    mipi_lcd_argb8888_test(&s_mipi_disp_ctx, DEFAULT_MIPI_PANEL_NAME);

    return 0;
}
