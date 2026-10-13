#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include "os/str.h"
#include "cli.h"
#include <components/bk_frame_buffer.h>
#include "h264_decode_test.h"
#include "vcdec_h264_test_common.h"
#include "media_service.h"

#define TAG "ap_main"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define CMDS_COUNT  (sizeof(s_h264_decode_commands) / sizeof(struct cli_command))

static const struct cli_command s_h264_decode_commands[] =
{
    // Decode command
    {"h264_decode", "h264_decode", cli_h264_decode_cmd},
};

int cli_h264_decode_init(void)
{
    return cli_register_commands(s_h264_decode_commands, CMDS_COUNT);
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

#if CONFIG_H264_DECODE_DPB_COVER
    h264_decode_cover_set_mode(H264_DECODE_COVER_POOL);
#endif

    cli_h264_decode_init();

#if VCDEC_H264_BOOT_DEMO_ENABLE
    vcdec_h264_run_boot_demo();
#endif

    return 0;
}
