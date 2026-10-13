#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include "cli.h"
#include <components/bk_frame_buffer.h>
#include "jpeg_decode_test.h"
#include "media_service.h"

#define TAG "ap_main"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define CMDS_COUNT  (sizeof(s_jpeg_decode_commands) / sizeof(struct cli_command))

static const struct cli_command s_jpeg_decode_commands[] =
{
    // Decode command
    {"jpeg_decode", "jpeg_decode", cli_jpeg_decode_cmd},
};

int cli_jpeg_decode_init(void)
{
    return cli_register_commands(s_jpeg_decode_commands, CMDS_COUNT);
}

int main(void)
{
    bk_init();
    media_service_init();

#if (BK_IPC_UT_TEST)
    bk_ipc_test_init();
#endif

    bk_printf("%s, %d, m55 running...\r\n", __func__, __LINE__);

#ifdef CONFIG_FRAME_BUFFER
    bk_frame_buffer_init();
#endif

    cli_jpeg_decode_init();

    vcdec_jpeg_run_boot_demo();
    return 0;
}
