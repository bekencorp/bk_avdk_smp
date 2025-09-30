#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include "jpeg_data.h"
#include <media_service.h>
#include <driver/pwr_clk.h>
#include "cli.h"
#include "frame_buffer.h"
#include "jpeg_decode_test.h"

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
    // Regular test command
    {"jpeg_decode_regular_test", "hardware_test/software_test", cli_jpeg_decode_regular_test_cmd},
    // Abnormal test command
    {"jpeg_decode_error_test", "input_buffer_null/invalid_input_data/output_buffer_small", cli_jpeg_decode_error_test_cmd},
};

int cli_jpeg_decode_init(void)
{
    return cli_register_commands(s_jpeg_decode_commands, CMDS_COUNT);
}

int main(void)
{
    bk_init();
    media_service_init();

    bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_VIDP_JPEG_DE, PM_POWER_MODULE_STATE_ON);
    bk_pm_module_vote_cpu_freq(PM_DEV_ID_DECODER, PM_CPU_FRQ_480M);

    cli_jpeg_decode_init();
    return 0;
}
