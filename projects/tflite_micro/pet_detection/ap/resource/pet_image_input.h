#pragma once

#include <stdint.h>
#include "tflm_pet_detection_model.h"

// Postprocess / letterbox parameters tied to embedded inputs in pet_image_input_1.cc,
// pet_image_input_2.cc (see generation *_output.txt). Update when resource tensors change.
#ifdef __cplusplus
static constexpr float k_conf_threshold = 0.3f;
static constexpr float k_iou_threshold = 0.45f;

static constexpr int k_cat_orig_w = 500;
static constexpr int k_cat_orig_h = 374;
static constexpr int k_dog_orig_w = 499;
static constexpr int k_dog_orig_h = 375;

static constexpr const char *k_class_names[k_num_classes] = {"cat", "dog"};
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern const unsigned char cat_200_model_input[];
extern const unsigned int cat_200_model_input_len;
extern const unsigned char dog_204_model_input[];
extern const unsigned int dog_204_model_input_len;

#ifdef __cplusplus
}
#endif
