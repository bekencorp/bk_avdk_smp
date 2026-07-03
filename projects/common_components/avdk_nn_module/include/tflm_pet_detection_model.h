// Tensor layout, arena sizes, and embedded pet (YOLOv8n) vela flatbuffer symbols.
// pet_detect_model_data.cc holds the bytes when CONFIG_TFLM_PET_DETECTION_V1 is enabled.

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const unsigned char pet_detection_vela_tflite[];
extern const unsigned int pet_detection_vela_tflite_len;

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

static constexpr int k_num_classes = 2;
static constexpr int k_num_candidates = 2100;
static constexpr int k_out_channels = 4 + k_num_classes;
#endif
