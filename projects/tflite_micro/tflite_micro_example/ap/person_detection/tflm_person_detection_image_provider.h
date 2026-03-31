#pragma once

#include "tensorflow/lite/c/common.h"

// Get image data for person detection
// image_width, image_height, channels: image dimensions (96x96x1 for grayscale)
// image_data: output buffer for image data (int8_t format, signed)
// type: image type - 0=not a person, 1=person
TfLiteStatus GetImage(int image_width, int image_height, int channels,
                      int8_t *image_data, int8_t type);

