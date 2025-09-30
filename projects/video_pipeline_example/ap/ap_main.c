#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include "jpeg_data.h"
#include <media_service.h>
#include <driver/pwr_clk.h>
#include "cli.h"
#include "frame_buffer.h"
#include "components/bk_video_pipeline/bk_video_pipeline.h"
#include "video_pipeline_test.h"

#define TAG "ap_main"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define CMDS_COUNT  (sizeof(s_video_pipeline_commands) / sizeof(struct cli_command))

static const struct cli_command s_video_pipeline_commands[] =
{
    //测试命令
    {"video_pipeline", "video_pipeline", cli_video_pipeline_cmd},
    //常规测试命令
    {"video_pipeline_regular_test", "hardware_rotate/software_rotate/h264_encode", cli_video_pipeline_regular_test_cmd},
    //异常测试命令
    {"video_pipeline_error_test", "null_handle/invalid_config/no_callback", cli_video_pipeline_error_test_cmd},
};

int cli_video_pipeline_init(void)
{
    return cli_register_commands(s_video_pipeline_commands, CMDS_COUNT);
}

int main(void)
{
    bk_init();
    media_service_init();

    bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_VIDP_JPEG_DE, PM_POWER_MODULE_STATE_ON);
    bk_pm_module_vote_cpu_freq(PM_DEV_ID_DECODER, PM_CPU_FRQ_480M);

    cli_video_pipeline_init();
    LOGI("Video pipeline example initialized. Type 'video_pipeline' for help.\n");
    return 0;
}
