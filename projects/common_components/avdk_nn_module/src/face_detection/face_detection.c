#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <driver/int.h>
#include <common/bk_err.h>

#include <avdk_check.h>

#include "avdk_nn_module.h"
#include "cfacedetectcnn.h"
#include "box.h"

#define TAG "face-detection"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

void* handle = NULL;

static int face_detection_init(const avdk_nn_module_t *module)
{
    LOGI("face detection init\r\n");
    rtos_delay_milliseconds(100); //TODO FIXME: remove this

    AVDK_RETURN_ON_FALSE(module, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    handle = os_malloc(facedetectcnn_size());

    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "malloc failed");

    return facedetectcnn_init(handle);
}

static int face_detection_deinit(const avdk_nn_module_t *module)
{
    return -1;
}

static int face_detection_run(const avdk_nn_module_t *module, void* input, uint32_t input_size,
    void* output, uint32_t *output_size)
{
    FaceBox faces[MAX_FACES_COUNT];

    int count = facedetectcnn_run(handle, input, module->width, module->height, 4, (uint8_t *)faces, MAX_FACES_COUNT, 0, 0.5, 0.3);

    if (count > 0)
    {
        box_detection_path_build(faces, count, MAX_FACES_COUNT, 90, module->width, module->height, 1920, 1080);
    }
    else
    {
        box_detection_path_clear();
    }

    LOGI("####### Face Detection: %d #######\n", count);

    return count;
}

static const avdk_nn_module_t face_detection_module = {
    .name = "face_detection",
    .width = 176,
    .height = 96,
    .format = BK_PIXEL_FORMAT_RGB888,
    .init = face_detection_init,
    .deinit = face_detection_deinit,
    .run = face_detection_run,
};

const avdk_nn_module_t *get_face_detection_module(void)
{
    return &face_detection_module;
}