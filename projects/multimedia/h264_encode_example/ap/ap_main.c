
#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include "cli.h"
#include <components/bk_frame_buffer.h>
#include "h264_encode_test.h"
#include "h264_encode_stress.h"
#include "h264_encode_time_statisticsi.h"
#include "media_service.h"

#define SYS_ANA_REG_BASE    (0x44010000)
#define LDO_ANA_REG         (0x69)

#define TAG "ap_main"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

extern void cli_mjpeg_encode_stress_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern void cli_mjpeg_flexa_dump_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

#define CMDS_COUNT  (sizeof(s_h264_encode_commands) / sizeof(struct cli_command))

static const struct cli_command s_h264_encode_commands[] =
{
    // Encode command
    {"h264_encode", "h264_encode", cli_h264_encode_cmd},
    // Legacy H264 encoder pressure test
    {"h264_encode_stress", "h264 encode pressure test", cli_h264_encode_stress_cmd},
    {"h264_encode_time_statisticsi", "frame|sw_flexa [n] GPIO32/33 timing", cli_h264_encode_time_statisticsi_cmd},

    {"mjpeg_encode_stress", "mjpeg encode stress", cli_mjpeg_encode_stress_cmd},
    {"mjpeg_flexa_dump", "mjpeg flexa encode+dump one frame", cli_mjpeg_flexa_dump_cmd},
};

int cli_h264_encode_init(void)
{
    return cli_register_commands(s_h264_encode_commands, CMDS_COUNT);
}

static void bk_auxldo_enable(void)
{
    uint32_t reg = REG_READ(SYS_ANA_REG_BASE + LDO_ANA_REG * 4);
    reg |= (0xF << 28) | (0x2 << 23) | (0x7 << 19) | (0x7 << 15);
    reg &= ~(0xF << 11);
    reg |= (0x8 << 11);
    REG_WRITE(SYS_ANA_REG_BASE + LDO_ANA_REG * 4, reg);
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

    bk_auxldo_enable();

    bk_printf("lodoen enable...\r\n");
#ifdef CONFIG_FRAME_BUFFER
    bk_frame_buffer_init();
#endif

    cli_h264_encode_init();
#ifdef CONFIG_BK_ENCODER
    vcenc_h264_run_boot_demo();
#endif
    return 0;
}
