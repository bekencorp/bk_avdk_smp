#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MOTION_DETECT_OK                 (0)
#define MOTION_DETECT_ERR_PARAM          (-1)
#define MOTION_DETECT_ERR_UNALIGNED      (-2)

#define MOTION_DETECT_GROUP_PIXELS       (16U)
#define MOTION_DETECT_REGION_COUNT       (3U)
#define MOTION_DETECT_BLOCK_COUNT        (9U)

typedef struct {
    uint32_t width;
    uint32_t height;
    uint8_t diff_threshold;
    uint32_t count_threshold;
} motion_detect_config_t;

typedef struct {
    uint8_t moving;
    uint32_t total_count;
    uint32_t max_block_index;
    uint32_t max_block_count;
    uint32_t row_counts[MOTION_DETECT_REGION_COUNT];
    uint32_t col_counts[MOTION_DETECT_REGION_COUNT];
    uint32_t block_counts[MOTION_DETECT_BLOCK_COUNT];
} motion_detect_result_t;

uint32_t motion_detect_get_count_buffer_size(uint32_t width, uint32_t height);
int motion_detect_compare_gray(const motion_detect_config_t *config,
                               const uint8_t *new_gray,
                               const uint8_t *old_gray,
                               uint8_t *motion_counts,
                               motion_detect_result_t *result);

#ifdef __cplusplus
}
#endif
