#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <driver/int.h>
#include <common/bk_err.h>

#include <driver/pwr_clk.h>
#include "sys_driver.h"

#include <components/shell_task.h>
#include "cli.h"

#include <driver/mipi_csi.h>
#include <components/bk_isp_camera.h>

#include "aov_camera.h"
#include "app_display.h"


#include "tflm_person_detect.h"
#include "tflm_face_detection.h"
#define TAG "aov-dt"

#include "aov_detection.h"


#define CMD_CONTAIN(value) cmd_contain(argc, argv, value)


#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

uint8_t detection_test_mode = true;

void argb_to_rgb(argb_pixel_t *argb_image, rgb_pixel_t *rgb_image, int width, int height)
{
    int i;
    for (i = 0; i < width * height; i++) {
        // 将ARGB图像的每个像素点转换成RGB图像的每个像素点
#if 1
        rgb_image[i].r = argb_image[i].r - 128;
        rgb_image[i].g = argb_image[i].g - 128;
        rgb_image[i].b = argb_image[i].b - 128;
#else
        rgb_image[i].r = argb_image[i].r;
        rgb_image[i].g = argb_image[i].g;
        rgb_image[i].b = argb_image[i].b;
#endif
    }
}

void aov_detection_shutdown(void)
{
    LOGI("%s\n", __func__);
    rtos_delay_milliseconds(500);
}

void aov_detection_test(uint8_t enable)
{
    LOGI("%s : %d\n", __func__, enable);
    detection_test_mode = enable;
}


void cli_avdk_doorbell_aov_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    //for (int i = 0; i < argc; i++)
    //{
    //    os_printf("%d: %s\n", i, argv[i]);
    //}

    if (argc == 3 && os_strcmp(argv[1], "test") == 0)
    {
        if (os_strcmp(argv[2], "0") == 0)
        {
            aov_detection_test(0);
        }
        else if (os_strcmp(argv[2], "1") == 0)
        {
            aov_detection_test(1);
        }
    }
}

static const struct cli_command s_aov_commands[] =
{
    {"aov", "aov...", cli_avdk_doorbell_aov_cmd},
};

#define AOV_TEST_CMD_CNT  (sizeof(s_aov_commands) / sizeof(struct cli_command))

void aov_detection_cli_init(void)
{
    cli_register_commands(s_aov_commands, AOV_TEST_CMD_CNT);
}

