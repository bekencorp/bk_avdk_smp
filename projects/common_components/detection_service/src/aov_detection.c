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

#include "aov_camera.h"
#include "app_display.h"

#include <components/bk_frame_buffer.h>

#include "tflm_person_detect.h"
#include "face_detection.h"
#include "tflm_face_detection.h"
#include "tflm_mobilefacenet.h"
#include "aov_detection.h"

#include "avdk_nn_module.h"

#define TAG "aov-dt"

#define CMD_CONTAIN(value) cmd_contain(argc, argv, value)


#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


static beken_thread_t aov_thd;
uint8_t *aov_frame = NULL;
uint32_t pixel_size = 0;

static const avdk_nn_module_t *detection_module = NULL;

#include "tensorflow/lite/micro/cortex_m_generic/debug_log_callback.h"

static void debugLogCallback(const char* s){
    BK_LOGI("TFLM", "%s", s);
}

static void aov_detection_thread(void)
{
    bk_err_t ret = BK_OK;
    uint8_t lcd_on = false;
    int detect_th = 0;


    detection_module->init(detection_module);

    RegisterDebugLogCallback(debugLogCallback);

    aov_isp_camera_turn_on(detection_module->width, detection_module->height, detection_module->format);

    while(1)
    {
        if (detection_test_mode && lcd_on == false)
        {
            app_mipi_lcd_turn_on(app_display_board_config_get());
            lcd_on = true;
        }

        ret = aov_isp_camera_frame_get(aov_frame, pixel_size);

        if (ret != BK_OK)
        {
            LOGE("###########%s, %d read frame failed##########\n", __func__, __LINE__);
            //break;
            rtos_delay_milliseconds(1000 * 2);
            continue;
        }

        ret = detection_module->run(detection_module, aov_frame, pixel_size, NULL, NULL);

        if (detection_test_mode)
        {
            rtos_delay_milliseconds(500);
            continue;
        }

        if (ret > 0)
        {
            if (lcd_on == false)
            {
                app_mipi_lcd_turn_on(app_display_board_config_get());
                lcd_on = true;
            }

            detect_th = DETECTION_MAX_RETRY;
            rtos_delay_milliseconds(500);
        }
        else
        {
            detect_th--;
        }

        if (detect_th <= 0)
        {
            break;
        }
    }

    LOGI("############### AOV Sleep ################\n");
    //aov_isp_camera_turn_off();

    //detection_module->deinit(detection_module);

    rtos_delete_thread(NULL);

}

void aov_detection_start(void)
{
    bk_err_t ret = BK_FAIL;

#ifdef CONFIG_FACE_DETECTION_V2
    detection_module = get_face_detection_module();
#elif CONFIG_TFLM_PERSON_DETECTION_V1
    detection_module = get_tfm_person_detection_module();
#elif CONFIG_TFLM_FACE_DETECTION_V1
    detection_module = get_tfm_face_detection_module();
#elif CONFIG_TFLM_MOBILEFACENET_V1
    detection_module = get_tfm_mobilefacenet_module();
#endif

    LOGI("%s %d\n", __func__, __LINE__); rtos_delay_milliseconds(1000);


    if (detection_module == NULL)
    {
        LOGE("detection_module is NULL\n");
        return;
    }

    if (BK_PIXEL_FORMAT_RGB888 == detection_module->format)
    {
        pixel_size = detection_module->width * detection_module->height * 4;
    }
    else
    {
        pixel_size = bk_image_size_get(detection_module->width, detection_module->height, detection_module->format);
    }


    aov_frame = bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, pixel_size);

    if (aov_frame == NULL)
    {
        LOGE("aov_frame malloc fail, size: %d\n", pixel_size);
        return;
    }

    aov_detection_cli_init();

    ret = rtos_create_hsram_thread(&aov_thd,
                             BEKEN_DEFAULT_WORKER_PRIORITY - 1,
                             "aov_thd",
                             (beken_thread_function_t)aov_detection_thread,
                             1024 * 20,
                             NULL);

    if (ret != BK_OK)
    {
        LOGE("create aov_detection_thread fail\n");
    }
}
