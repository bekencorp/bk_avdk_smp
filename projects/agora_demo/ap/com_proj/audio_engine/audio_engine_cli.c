#include "audio_engine.h"
#include <os/os.h>
#include <common/bk_include.h>
#include <common/bk_err.h>
#include "cli.h"

#define TAG "aude_cli"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define AUD_ENGINE_CMD_CNT   (sizeof(s_aud_engine_commands) / sizeof(struct cli_command))
static void bk_aud_engine_cli_help(void)
{
    LOGI("aude {start|stop}\n");
}

static void bk_aud_engine_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc < 2)
    {
        goto cmd_fail;
    }

    /* audio test */
    if (os_strcmp(argv[1], "start") == 0)
    {
        LOGI("start audio engine\r\n");
        if (os_strcmp(argv[2], "g722") == 0)
        {
            #if CONFIG_VOICE_SERVICE_G722_ENCODER && CONFIG_VOICE_SERVICE_G722_DECODER
            //extern int audio_engine_example_g722(void);
            //audio_engine_example_g722();
            #endif
        }
    }
    else if (os_strcmp(argv[1], "stop") == 0)
    {
        LOGI("stop audio engine\r\n");
        //bk_byte_rtc_stop(volc_room_info);
    }
    else if (os_strcmp(argv[1], "prompt_tone") == 0)
    {
        if (argc < 3)
        {
            goto cmd_fail;
        }

        LOGI("prompt_test audio engine\r\n");
#if CONFIG_APP_EVT
        extern bk_err_t app_event_send_msg(uint32_t event, uint32_t param);
        app_event_send_msg(os_strtoul(argv[2], NULL, 10), 0);
#endif
    }
    else
    {
        goto cmd_fail;
    }

    return;

cmd_fail:
    bk_aud_engine_cli_help();
}
static const struct cli_command s_aud_engine_commands[] =
{
    {"aude", "aude debug ...", bk_aud_engine_test_cmd},
};

int bk_aud_engine_cli_init(void)
{
    return cli_register_commands(s_aud_engine_commands, AUD_ENGINE_CMD_CNT);
}


