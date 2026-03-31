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

#include "avdk_nn_module.h"

typedef enum {
    GESTURE_ROCK = 0,
    GESTURE_PAPER,
    GESTURE_SCISSORS,
    GESTURE_NONE,   /**< No valid gesture detected */
    GESTURE_MAX,    /**< Sentinel value, must be the last */
} gesture_result_t;

const avdk_nn_module_t *get_tflm_gesture_detection_module(void);