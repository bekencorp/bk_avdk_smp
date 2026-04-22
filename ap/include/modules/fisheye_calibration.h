#ifndef FISHEYE_CALIBRATION_H
#define FISHEYE_CALIBRATION_H

#include <stdint.h>

/** TV contour points used with std_map_points / user_map_points (must match calibration). */
#define FISHEYE_MAP_POINTS 7

typedef struct {
    int row, col;
    float x, y;
} grid_point_t;

typedef struct {
    float x, y;
} point_t;

/**
 * Build dense remap tables map_x, map_y (int16, nearest-neighbor indices into source).
 * @param output  Layout: map_x[0 .. ow*oh-1] then map_y[0 .. ow*oh-1], row-major, size 2*ow*oh int16_t.
 * @return 0 on success, -1 on allocation failure.
 */
int fisheye_calibration(
    const grid_point_t *gp,
    uint32_t gpoints,
    const point_t *std_map_points,
    const point_t *user_map_points,
    int input_width,
    int input_height,
    int output_width,
    int output_height,
    int16_t *output);

#endif /* FISHEYE_CORRECT_H */
