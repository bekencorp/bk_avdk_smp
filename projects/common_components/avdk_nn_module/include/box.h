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

// Face detection box structure
typedef struct {
    float score;  // confidence score
    short xmin;   // left top x
    short ymin;   // left top y
    short xmax;   // right bottom x
    short ymax;   // right bottom y
    short lm[0];  // if landmark enabled, elements of lm will be FACE_LANDMARK_POINTS * 2
} FaceBox;

typedef struct {
    int x1, y1, x2, y2;
    float score;
} Box;

int box_detection_path_build(FaceBox *faces, int count, int buffer_count, int rotate, int src_width, int src_height, int dst_width, int dst_height);
void box_detection_path_clear(void);

#ifdef  __cplusplus
}
#endif//__cplusplus