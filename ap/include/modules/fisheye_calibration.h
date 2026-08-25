#ifndef FISHEYE_CALIBRATION_H
#define FISHEYE_CALIBRATION_H

#include <stdint.h>

/** TV contour points used by the camera-intrinsics fisheye calibration path. */
#define FISHEYE_MAP_POINTS 7

typedef struct {
    float x, y;
} point_t;

typedef struct {
    double camera_matrix[9];              /* Row-major 3x3 K matrix. */
    double distortion_coeffs[4];          /* OpenCV fisheye k1..k4. */
    double distortion_coeffs_fallback[4]; /* Used when refined residual is too large. */
} fisheye_camera_params_t;

typedef struct {
    uint8_t used_fallback;
    float source_max_error_px;
    float valid_coverage;
    uint16_t projected_output[FISHEYE_MAP_POINTS][2];
    double homography[9]; /* Row-major normalized-camera to output transform. */
} fisheye_calibration_result_t;

/**
 * Build dense remap tables map_x, map_y from camera K/D and seven TV landmarks.
 * @param output  Layout: map_x[0 .. ow*oh-1] then map_y[0 .. ow*oh-1], row-major, size 2*ow*oh int16_t.
 * @param result  Optional debug/result info; may be NULL.
 * @return 0 on success, -1 on allocation failure.
 */
int fisheye_calibration(
    const fisheye_camera_params_t *camera,
    const point_t tv_points[FISHEYE_MAP_POINTS],
    int input_width,
    int input_height,
    int output_width,
    int output_height,
    int16_t *output,
    fisheye_calibration_result_t *result);

#endif /* FISHEYE_CALIBRATION_H */
