// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

/**
 * Generic 2D detection box used across all NN detection models in this module
 * (faces, palms, gestures, ...).
 *
 *   x, y  : top-left corner    (inclusive), in source-image pixel units
 *   w, h  : box size in pixels (so the bottom-right corner is (x+w, y+h))
 *   score : confidence score (model-specific scale)
 *
 * Coordinates are float so that sub-pixel detection output (e.g. the raw
 * cx/cy/w/h decoded from a model tensor before any rounding) can flow all
 * the way through rotation/scaling/clamping with no precision loss; the
 * final pixel-snap (cast to int) only happens at the very last step inside
 * box_detection_path_build() when the GPU draw command is emitted.
 */
typedef struct {
    float x;
    float y;
    float w;
    float h;
    float score;
} Box;

int  box_detection_path_build(Box *boxes, int count, int buffer_count, int rotate,
                              int src_width, int src_height,
                              int dst_width, int dst_height);
void box_detection_path_clear(void);

#ifdef  __cplusplus
}
#endif//__cplusplus