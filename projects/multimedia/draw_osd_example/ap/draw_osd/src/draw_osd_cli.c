#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <common/bk_include.h>
#include <components/avdk_utils/avdk_error.h>
#include "components/bk_draw_osd.h"
#include "components/bk_display.h"
#include "osd_disp_compat.h"
#include "draw_osd_test.h"
#include "draw_osd_complex_test.h"
#include "blend.h"
#include "cli.h"

#define TAG "draw_osd"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

/* 暴露给 draw_osd_complex_test.c 使用（extern 声明在 draw_osd_complex_test.h 中） */
bk_draw_osd_ctlr_handle_t draw_osd_handle = NULL;
bk_display_ctlr_handle_t  lcd_display_handle = NULL;

/* 分配一帧 RGB565 蓝底背景（OSD_BG_W x OSD_BG_H） */
static frame_buffer_t *osd_alloc_bg_frame(uint16_t color)
{
    uint32_t len = (uint32_t)OSD_BG_W * OSD_BG_H * 2;
    frame_buffer_t *fb = frame_buffer_display_malloc(len);
    if (fb == NULL) {
        return NULL;
    }
    for (uint32_t i = 0; i < len; i += 2) {
        *(uint16_t *)(fb->frame + i) = color;
    }
    fb->fmt    = PIXEL_FMT_RGB565_LE;
    fb->width  = OSD_BG_W;
    fb->height = OSD_BG_H;
    return fb;
}

void cli_draw_osd_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    avdk_err_t ret = AVDK_ERR_UNKNOWN;
    char *msg = NULL;

    if (argc < 2) {
        LOGE("%s, %d: insufficient arguments\n", __func__, __LINE__);
        goto exit;
    }

    if (strcmp(argv[1], "init") == 0)
    {
        osd_ctlr_config_t osd_cfg = {0};
        osd_cfg.blend_assets  = blend_assets;   /**< all assets */
        osd_cfg.blend_info    = blend_info;      /**< real display */
        osd_cfg.draw_in_psram = false;
        ret = bk_draw_osd_new(&draw_osd_handle, &osd_cfg);
        AVDK_GOTO_VOID_ON_FALSE(ret == AVDK_ERR_OK, exit, TAG, "bk_draw_osd_new failed!\n");

        ret = osd_display_open(&lcd_display_handle);
        AVDK_GOTO_VOID_ON_FALSE(ret == AVDK_ERR_OK, exit, TAG, "osd_display_open failed!\n");
        LOGD("osd init success!\n");
    }
    else if (strcmp(argv[1], "deinit") == 0)
    {
        ret = bk_draw_osd_delete(draw_osd_handle);
        draw_osd_handle = NULL;
        AVDK_GOTO_VOID_ON_FALSE(ret == AVDK_ERR_OK, exit, TAG, "bk_draw_osd_delete failed!\n");
        ret = osd_display_close(lcd_display_handle);
        lcd_display_handle = NULL;
        AVDK_GOTO_VOID_ON_FALSE(ret == AVDK_ERR_OK, exit, TAG, "osd_display_close failed!\n");
    }
    else if (strcmp(argv[1], "array") == 0)
    {
        AVDK_RETURN_VOID_ON_FALSE(draw_osd_handle, TAG, "draw_osd_handle is NULL!");
        AVDK_RETURN_VOID_ON_FALSE(lcd_display_handle, TAG, "lcd_display_handle is NULL!");

        frame_buffer_t *bg_frame = osd_alloc_bg_frame(0x0000);
        AVDK_GOTO_VOID_ON_FALSE(bg_frame, exit, TAG, "frame_buffer_display_malloc failed!\n");

        if (argv[2] != NULL && (strcmp(argv[2], "update") == 0 || strcmp(argv[2], "updata") == 0))
        {
            if (argv[3] == NULL) {
                frame_buffer_display_free(bg_frame);
                LOGE("%s, %d: insufficient arguments\n", __func__, __LINE__);
                goto exit;
            }
            ret = bk_draw_osd_add_or_updata(draw_osd_handle, argv[3], argv[4]);
        }
        else if (argv[2] != NULL && strcmp(argv[2], "remove") == 0)
        {
            if (argv[3] == NULL) {
                frame_buffer_display_free(bg_frame);
                LOGE("%s, %d: insufficient arguments\n", __func__, __LINE__);
                goto exit;
            }
            ret = bk_draw_osd_remove(draw_osd_handle, argv[3]);
        }

        osd_bg_info_t bg_info = {0};
        bg_info.frame  = bg_frame;
        bg_info.width  = OSD_BG_W;
        bg_info.height = OSD_BG_H;
        ret = bk_draw_osd_array(draw_osd_handle, &bg_info, NULL);

        ret = osd_display_flush(lcd_display_handle, bg_frame);
        if (ret != AVDK_ERR_OK) {
            LOGE("osd_display_flush failed\n");
        }
    }
    else if (strcmp(argv[1], "get_info") == 0)
    {
        uint32_t is_printf = 1;
        const blend_info_t *resources = NULL;
        uint32_t size = 0;
        if (argc > 2 && strcmp(argv[2], "no_print") == 0) {
            is_printf = 0;
        }
        LOGI("get current draw info (print mode: %s)\n", is_printf ? "open" : "close");
        ret = bk_draw_osd_ioctl(draw_osd_handle, OSD_CTLR_CMD_GET_DRAW_INFO, is_printf, (uint32_t)&resources, (uint32_t)&size);
    }
    else if (strcmp(argv[1], "get_assets") == 0)
    {
        uint32_t is_printf = 1;
        if (argc > 2 && strcmp(argv[2], "no_print") == 0) {
            is_printf = 0;
        }
        LOGI("get all available assets (print mode: %s)\n", is_printf ? "open" : "close");
        ret = bk_draw_osd_ioctl(draw_osd_handle, OSD_CTLR_CMD_GET_ALL_ASSETS, is_printf, 0, 0);
    }
    else if (strcmp(argv[1], "img") == 0)
    {
        AVDK_RETURN_VOID_ON_FALSE(draw_osd_handle, TAG, "draw_osd_handle is NULL!");
        AVDK_RETURN_VOID_ON_FALSE(lcd_display_handle, TAG, "lcd_display_handle is NULL!");

        frame_buffer_t *bg_frame = osd_alloc_bg_frame(0x001f);
        AVDK_GOTO_VOID_ON_FALSE(bg_frame, exit, TAG, "frame_buffer_display_malloc failed!\n");

        osd_bg_info_t bg_info = {0};
        bg_info.frame  = bg_frame;
        bg_info.width  = OSD_BG_W;
        bg_info.height = OSD_BG_H;

        blend_info_t wifi_info = {.name = "wifi", .addr = &img_wifi_rssi0, .content = "wifi0"};
        ret = bk_draw_osd_image(draw_osd_handle, &bg_info, &wifi_info);

        ret = osd_display_flush(lcd_display_handle, bg_frame);
        if (ret != AVDK_ERR_OK) {
            LOGE("osd_display_flush failed\n");
        }
    }
    else if (strcmp(argv[1], "font") == 0)
    {
        AVDK_RETURN_VOID_ON_FALSE(draw_osd_handle, TAG, "draw_osd_handle is NULL!");
        AVDK_RETURN_VOID_ON_FALSE(lcd_display_handle, TAG, "lcd_display_handle is NULL!");

        frame_buffer_t *bg_frame = osd_alloc_bg_frame(0x001f);
        AVDK_GOTO_VOID_ON_FALSE(bg_frame, exit, TAG, "frame_buffer_display_malloc failed!\n");

        osd_bg_info_t bg_info = {0};
        bg_info.frame  = bg_frame;
        bg_info.width  = OSD_BG_W;
        bg_info.height = OSD_BG_H;

        blend_info_t font_info = {.name = "clock", .addr = &font_clock, .content = "12:68"};
        ret = bk_draw_osd_font(draw_osd_handle, &bg_info, &font_info);

        ret = osd_display_flush(lcd_display_handle, bg_frame);
        if (ret != AVDK_ERR_OK) {
            LOGE("osd_display_flush failed\n");
        }
    }
    else if (strcmp(argv[1], "test") == 0)
    {
        ret = osd_complex_test_dispatch(argc, argv);
    }
    else
    {
        LOGE("%s, %d: invalid arguments\n", __func__, __LINE__);
    }

exit:
    if (ret != AVDK_ERR_OK) {
        msg = CLI_CMD_RSP_ERROR;
    } else {
        msg = CLI_CMD_RSP_SUCCEED;
    }

    LOGI("%s ---complete\n", __func__);

    if (pcWriteBuffer != NULL && msg != NULL) {
        os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    }
}

#define CMDS_COUNT  (sizeof(s_draw_osd_test_commands) / sizeof(struct cli_command))

static const struct cli_command s_draw_osd_test_commands[] =
{
    {"osd", "init | array | img | font | get_info | get_assets | test <sub> | deinit", cli_draw_osd_test_cmd},
};

int cli_draw_osd_test_init(void)
{
    return cli_register_commands(s_draw_osd_test_commands, CMDS_COUNT);
}
