//TEST CODE

#include <os/os.h>
#include <avdk_check.h>
#include "avdk_nn_module.h"
#include <components/bk_frame_buffer.h>

#include "tflm_face_detection.h"
#include "swift_yolo_1xb16_300e_coco_300_model_data.h"

#include "face_detection.h"

#define TAG "tflm-face-detection"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

static void* handle;

static int tflm_face_detection_init(const avdk_nn_module_t *module)
{
    AVDK_RETURN_ON_FALSE(module, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
	return face_detection_init(&handle, (uint8_t*)swift_yolo_1xb16_300e_coco_300_int8_model_data, 900 * 1024);
}

static int tflm_face_detection_deinit(const avdk_nn_module_t *module)
{
    return face_detection_deinit(handle);
}

static int tflm_face_detection_run(const avdk_nn_module_t *module, void* input, uint32_t input_size,
    void* output, uint32_t *output_size)
{
	int ret = face_detection_run(handle, (int8_t*)input, module->width, module->height, 3);

    LOGI("####### Face Detection: %d #######\n", ret);

    return ret;
}

static const avdk_nn_module_t tfm_face_detection_module = {
    .name = "tfm_face_detection",
    .width = 96,
    .height = 96,
    .format = BK_PIXEL_FORMAT_RGB888,
    .init = tflm_face_detection_init,
    .deinit = tflm_face_detection_deinit,
    .run = tflm_face_detection_run,
};

const avdk_nn_module_t *get_tfm_face_detection_module(void)
{
    return &tfm_face_detection_module;
}