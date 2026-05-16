#pragma once

#include <stdint.h>
#include "tflm_hand_gesture_detection_model.h"

// Postprocess / letterbox parameters tied to embedded inputs in hand_gesture_image_input_1.cc,
// hand_gesture_image_input_2.cc (see generation *_output.txt). Update when resource tensors change.
#ifdef __cplusplus
static constexpr float k_conf_threshold = 0.45f;
static constexpr float k_iou_threshold = 0.7f;

static constexpr int k_test_hands_1_orig_w = 384;
static constexpr int k_test_hands_1_orig_h = 384;
static constexpr int k_test_hands_2_orig_w = 384;
static constexpr int k_test_hands_2_orig_h = 512;

static constexpr const char *k_class_names[k_num_classes] = {
    "c0", "c1", "c2", "c3", "c4", "c5", "c6",
};
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern const unsigned char test_hands_1_model_input[];
extern const unsigned int test_hands_1_model_input_len;
extern const unsigned char test_hands_2_model_input[];
extern const unsigned int test_hands_2_model_input_len;

#ifdef __cplusplus
}
#endif
