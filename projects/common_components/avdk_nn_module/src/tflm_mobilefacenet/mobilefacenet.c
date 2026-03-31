//TEST CODE

#include <os/os.h>
#include <avdk_check.h>
#include "avdk_nn_module.h"
#include <components/bk_frame_buffer.h>

#include "mobilefacenet_int8_96x112_model_data.h"
#include "mobilefacenet_int8_112x112_model_data.h"
#include "mobilefacenet.h"
#include "face_image_data.h"

#define TAG "tflm-face-detection"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

static void* handle;
static int8_t *detection_frame = NULL;

static int tflm_mobilefacenet_init(const avdk_nn_module_t *module)
{
    AVDK_RETURN_ON_FALSE(module, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

	detection_frame = (int8_t*)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, module->width * module->height * 3);
	if (detection_frame == NULL)
	{
		LOGE("detection_frame malloc failed\n");
		return AVDK_ERR_GENERIC;
	}

	return mobilefacenet_init(&handle, (uint8_t*)mobilefacenet_int8_96x112_data, 1024 * 1024 * 4) ? AVDK_ERR_GENERIC : AVDK_ERR_OK;
}

static int tflm_mobilefacenet_deinit(const avdk_nn_module_t *module)
{
    return mobilefacenet_deinit(handle);
}

static int tflm_mobilefacenet_run(const avdk_nn_module_t *module, void* input, uint32_t input_size,
    void* output, uint32_t *output_size)
{
#if 0
	uint8_t *src_frame = (uint8_t*)input;

	for (int i = 0;i < module->width * module->height; i++)
	{
		detection_frame[i * 3 + 0] = (int8_t)src_frame[i * 4 + 2] - 128;
		detection_frame[i * 3 + 1] = (int8_t)src_frame[i * 4 + 1] - 128;
		detection_frame[i * 3 + 2] = (int8_t)src_frame[i * 4 + 0] - 128;
	}

	int ret = mobilefacenet_run(handle, detection_frame, module->width, module->height, 3);

#else

	LOGI("face_a_01_rgb\n");

	for (int i = 0;i < module->width * module->height; i++)
	{
		detection_frame[i * 3 + 0] = (int8_t)face_a_01_rgb[i * 3 + 0] - 128;
		detection_frame[i * 3 + 1] = (int8_t)face_a_01_rgb[i * 3 + 1] - 128;
		detection_frame[i * 3 + 2] = (int8_t)face_a_01_rgb[i * 3 + 2] - 128;
	}

	int ret = mobilefacenet_run(handle, (int8_t*)detection_frame, module->width, module->height, 3);


	LOGI("face_a_02_rgb\n");

	for (int i = 0;i < module->width * module->height; i++)
	{
		detection_frame[i * 3 + 0] = (int8_t)face_a_02_rgb[i * 3 + 0] - 128;
		detection_frame[i * 3 + 1] = (int8_t)face_a_02_rgb[i * 3 + 1] - 128;
		detection_frame[i * 3 + 2] = (int8_t)face_a_02_rgb[i * 3 + 2] - 128;
	}

	ret = mobilefacenet_run(handle, (int8_t*)detection_frame, module->width, module->height, 3);

	LOGI("face_b_01_rgb\n");

	for (int i = 0;i < module->width * module->height; i++)
	{
		detection_frame[i * 3 + 0] = (int8_t)face_b_01_rgb[i * 3 + 0] - 128;
		detection_frame[i * 3 + 1] = (int8_t)face_b_01_rgb[i * 3 + 1] - 128;
		detection_frame[i * 3 + 2] = (int8_t)face_b_01_rgb[i * 3 + 2] - 128;
	}

	ret = mobilefacenet_run(handle, (int8_t*)detection_frame, module->width, module->height, 3);
#endif
    LOGI("####### Face Detection: %d #######\n", ret);

    return ret;
}

static const avdk_nn_module_t tfm_mobilefacenet_module = {
    .name = "tfm_mobilefacenet",
    .width = 96,
    .height = 122,
    .format = BK_PIXEL_FORMAT_RGB888,
    .init = tflm_mobilefacenet_init,
    .deinit = tflm_mobilefacenet_deinit,
    .run = tflm_mobilefacenet_run,
};

const avdk_nn_module_t *get_tfm_mobilefacenet_module(void)
{
    return &tfm_mobilefacenet_module;
}