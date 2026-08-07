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
#include "bk_lcd_panel_commands.h"   /* LCD_CMD_DISPON / LCD_CMD_DISPOFF */
#include <driver/mipi_dsi.h>         /* mipi_dsi_panel_set_pattern (VPG bring-up test) */


#define TAG "mipi_lcd"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

/* Default MIPI panel (change here to switch panel). */
#define DEFAULT_MIPI_PANEL_NAME   "hx8399c_mipi_1080x1920"
// #define DEFAULT_MIPI_PANEL_NAME   "hx8394f_mipi_720x1280"
// #define DEFAULT_MIPI_PANEL_NAME   "ek79007ad_mipi_1024x600"
// #define DEFAULT_MIPI_PANEL_NAME   "er68576b_mipi_720x1280"

/** Shared with CLI: lcd_example_flush_thread_stop() then lcd_example_dsi_close() for teardown. */
static display_ctx_t s_mipi_disp_ctx;

void cli_mipi_lcd_switch_format(const bk_display_pixel_format_config_t *config, const char *name)
{
    if (s_mipi_disp_ctx.dis_bus_handle == NULL)
    {
        LOGI("MIPI LCD is off\r\n");
        return;
    }

    const bool flush_was_running = (s_mipi_disp_ctx.thread != NULL);

    if (mipi_lcd_do_switch(&s_mipi_disp_ctx, config) == AVDK_ERR_OK)
    {
        if (!flush_was_running)
            LOGI("MIPI LCD display started (%s)\r\n", name);
        else
            LOGI("MIPI LCD switched to %s\r\n", name);
    }
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
        LOGI("usage: mipi_lcd open | close | switch rgb565|rgb888|nv12|argb8888 | read_id | disp on|off | sleep in|out | vpg v|h|ber|off\r\n");
        LOGI("  open: init panel only (no flush); switch rgb565|argb8888 starts display\r\n");
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
        LOGI("MIPI LCD on (panel %s, no flush; use 'mipi_lcd switch rgb565|argb8888' to display)\r\n",
             DEFAULT_MIPI_PANEL_NAME);
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
    else if (os_strcmp(argv[1], "read_id") == 0)
    {
        uint32_t lcd_id = 0;

        if (s_mipi_disp_ctx.panel_handle == NULL)
        {
            LOGI("MIPI LCD is off\r\n");
            return;
        }
        if (bk_lcd_panel_read_id(s_mipi_disp_ctx.panel_handle, &lcd_id) == BK_OK)
            LOGI("LCD read ID: 0x%06x\r\n", lcd_id);
        else
            LOGE("LCD read ID failed\r\n");
        return;
    }
    else if (os_strcmp(argv[1], "disp") == 0)
    {
        if (s_mipi_disp_ctx.dis_bus_handle == NULL)
        {
            LOGI("MIPI LCD is off\r\n");
            return;
        }
        if (argc < 3)
        {
            LOGI("usage: mipi_lcd disp on|off\r\n");
            return;
        }
        bool on = (os_strcmp(argv[2], "on") == 0);
#if 1
        /* ioctl path: takes dpu_ctlr_lock and (for sleep) enforces the DCS
         * post-command delay. Preferred for product code. */
        if (bk_display_ioctl(s_mipi_disp_ctx.dpu_ctlr_handle,
                             BK_DISPLAY_IOCTL_PANEL_DISP_ON_OFF, &on) == AVDK_ERR_OK)
            LOGI("panel DISP %s\r\n", on ? "ON" : "OFF");
        else
            LOGE("panel DISP %s failed\r\n", on ? "ON" : "OFF");
#else
        /* Direct panel tx_param: sends the DCS write straight down the DSI
         * command channel (no DPU lock, no delay). Safe while streaming because
         * the host inserts it in the LP blanking period (VID_MODE_CFG.lp_cmd_en). */
        if (bk_lcd_panel_tx_param(s_mipi_disp_ctx.panel_handle,
                                  on ? LCD_CMD_DISPON : LCD_CMD_DISPOFF, NULL, 0) == BK_OK)
            LOGI("panel DISP %s\r\n", on ? "ON" : "OFF");
        else
            LOGE("panel DISP %s failed\r\n", on ? "ON" : "OFF");
#endif
        return;
    }
    else if (os_strcmp(argv[1], "sleep") == 0)
    {
        if (s_mipi_disp_ctx.dis_bus_handle == NULL)
        {
            LOGI("MIPI LCD is off\r\n");
            return;
        }
        if (argc < 3)
        {
            LOGI("usage: mipi_lcd sleep in|out\r\n");
            return;
        }
        bool sleep = (os_strcmp(argv[2], "in") == 0);
        if (bk_display_ioctl(s_mipi_disp_ctx.dpu_ctlr_handle,
                             BK_DISPLAY_IOCTL_PANEL_SLEEP, &sleep) == AVDK_ERR_OK)
            LOGI("panel SLEEP %s\r\n", sleep ? "IN" : "OUT");
        else
            LOGE("panel SLEEP %s failed\r\n", sleep ? "IN" : "OUT");
        return;
    }
    else if (os_strcmp(argv[1], "vpg") == 0)
    {
        /* DSI-host internal video pattern generator: streams color bars over HS
         * independent of the DPU feed. If bars show -> HS link + panel OK, the
         * problem is the DPU/DPI feed. If nothing shows -> HS link/panel dead. */
        if (s_mipi_disp_ctx.dis_bus_handle == NULL)
        {
            LOGI("MIPI LCD is off, run 'mipi_lcd open' first\r\n");
            return;
        }
        mipi_dsi_pattern_type_t pat = MIPI_DSI_PATTERN_BAR_VERTICAL;
        if (argc >= 3)
        {
            if (os_strcmp(argv[2], "off") == 0)        pat = MIPI_DSI_PATTERN_NONE;
            else if (os_strcmp(argv[2], "h") == 0)     pat = MIPI_DSI_PATTERN_BAR_HORIZONTAL;
            else if (os_strcmp(argv[2], "v") == 0)     pat = MIPI_DSI_PATTERN_BAR_VERTICAL;
            else if (os_strcmp(argv[2], "ber") == 0)   pat = MIPI_DSI_PATTERN_BER_VERTICAL;
        }
        if (mipi_dsi_panel_set_pattern(pat) == BK_OK)
            LOGI("VPG set: %d (usage: mipi_lcd vpg v|h|ber|off)\r\n", (int)pat);
        else
            LOGE("VPG set failed\r\n");
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

    LOGI("unknown arg \"%s\", usage: mipi_lcd open | close | switch rgb565|rgb888|nv12|argb8888 | read_id | disp on|off | sleep in|out\r\n", argv[1]);
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
    {"mipi_lcd", "open (no flush) | close | switch rgb565|argb8888 starts display | read_id | vpg", cli_mipi_lcd_cmd},
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

    /* Power-on IT case: open ARGB8888, flush ~30s, auto-close, log [RESULT][PASS]. */
    mipi_lcd_argb8888_test(&s_mipi_disp_ctx, DEFAULT_MIPI_PANEL_NAME);

    return 0;
}
