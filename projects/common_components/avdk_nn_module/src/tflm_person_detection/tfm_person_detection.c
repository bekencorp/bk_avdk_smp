#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <driver/int.h>
#include <common/bk_err.h>

#include <avdk_check.h>
#include "avdk_nn_module.h"

#include "tflite-micro.h"
#include "tflite-micro.dat"

#include "tflm_person_detect.h"


static void* handle;


#define TAG "tfm-person-detection"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


static int tflm_person_detection_init(const avdk_nn_module_t *module)
{
    AVDK_RETURN_ON_FALSE(module, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    return person_detection_init(&handle, (uint8_t*)g_person_detection_model_data, 100 * 1024);
}

static int tflm_person_detection_deinit(const avdk_nn_module_t *module)
{
    return person_detection_deinit(handle);
}

static int tflm_person_detection_run(const avdk_nn_module_t *module, void* input, uint32_t input_size,
    void* output, uint32_t *output_size)
{
    int ret = person_detection_run(handle, (uint8_t*)input);

    LOGI("####### Person Detection: %d #######\n", ret);

    return ret;
}

static const avdk_nn_module_t tfm_person_detection_module = {
    .name = "tfm_person_detection",
    .width = 96,
    .height = 96,
    .format = BK_PIXEL_FORMAT_NV12,
    .init = tflm_person_detection_init,
    .deinit = tflm_person_detection_deinit,
    .run = tflm_person_detection_run,
};

const avdk_nn_module_t *get_tfm_person_detection_module(void)
{
    return &tfm_person_detection_module;
}