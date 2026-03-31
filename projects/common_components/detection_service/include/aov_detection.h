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

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
#if 0
    uint8_t a;
    uint8_t r;
    uint8_t g;
    uint8_t b;
#else
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t a;
#endif
} __attribute__((packed)) argb_pixel_t;

typedef struct {
#if 0
    uint8_t r;
    uint8_t g;
    uint8_t b;
#else
    uint8_t b;
    uint8_t g;
    uint8_t r;
#endif
} __attribute__((packed)) rgb_pixel_t;


#define DETECTION_MAX_RETRY (8)

extern uint8_t detection_test_mode;

void argb_to_rgb(argb_pixel_t *argb_image, rgb_pixel_t *rgb_image, int width, int height);

void aov_detection_shutdown(void);

void aov_detection_cli_init(void);

void aov_detection_start(void);
void aov_detection_test(uint8_t enable);

#ifdef __cplusplus
}
#endif

