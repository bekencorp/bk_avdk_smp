#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include "cli.h"
#include <components/bk_frame_buffer.h>
#include "h264_decode_test.h"
#include "h264_decode_flexa_test.h"
#include "h264_decode_stress.h"
#include "vcdec_h264_driver_test.h"
#include "media_service.h"

#define SYS_ANA_REG_BASE    (0x44010000)
#define LDO_ANA_REG         (0x69)

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
    {"h264_decode_flexa", "h264 decode flexa test", cli_h264_decode_flexa_cmd},
    {"h264_decode_stress", "h264 decode pressure test", cli_h264_decode_stress_cmd},
    {"vcdec_h264_driver", "vcdec h264 direct-register test", cli_vcdec_h264_driver_cmd},
};

int cli_h264_decode_init(void)
{
    return cli_register_commands(s_h264_decode_commands, CMDS_COUNT);
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

    cli_h264_decode_init();
#ifdef CONFIG_BK_DECODER
    vcdec_h264_run_boot_demo();
#endif
    return 0;
}
