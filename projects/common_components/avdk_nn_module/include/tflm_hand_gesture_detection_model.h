// Tensor layout, arena sizes, and hand gesture (YOLOv8) vela flatbuffer symbols.
// hand_gesture_detect_model_data.cc holds the bytes when CONFIG_TFLM_HAND_GESTURE_DETECTION_V1 is enabled.

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const unsigned char hand_gesture_detection_vela_tflite[];
extern const unsigned int hand_gesture_detection_vela_tflite_len;

#ifdef __cplusplus
}
#endif

/** TFLM tensor arena and Ethos-U scratch; must match vela + runtime. */
#define TFLM_ARENA_SIZE (3 * 1024 * 1024)
#define ETHOSU_SCRATCH_SIZE (130 * 1024)

#ifdef __cplusplus
static constexpr int k_input_h = 320;
static constexpr int k_input_w = 320;
static constexpr int k_input_c = 3;

/** 7 gesture classes; raw output channel = 4 (box) + k_num_classes = 11 */
static constexpr int k_num_classes = 7;
static constexpr int k_num_candidates = 2100;
static constexpr int k_out_channels = 4 + k_num_classes;
#endif
