// Copyright 2025-2026 Beken
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

#ifndef _VIDEO_ENGINE_H_
#define _VIDEO_ENGINE_H_

#include <common/bk_include.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	uint16_t id;
	uint16_t width;
	uint16_t height;
	uint16_t format;
	uint16_t protocol;
	uint16_t rotate;
} camera_parameters_t;

int video_engine_camera_close(void);

int video_engine_start(void);

int video_engine_stop(void);

int video_engine_camera_turn_on(camera_parameters_t *parameters);

int video_engine_transfer_start(void);

int video_engine_transfer_stop(void);

int video_engine_init(void);

int video_engine_deinit(void);

bool video_engine_is_running(void);

#ifdef __cplusplus
}
#endif

#endif /* _VIDEO_ENGINE_H_ */
