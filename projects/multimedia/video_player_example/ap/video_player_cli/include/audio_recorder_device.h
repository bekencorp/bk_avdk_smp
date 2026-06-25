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

#ifndef _AUDIO_RECORDER_DEVICE_H_
#define _AUDIO_RECORDER_DEVICE_H_

#include <common/bk_include.h>
#include <components/avdk_utils/avdk_error.h>

// Reuse recorder audio format definitions.
// Note: audio_recorder_device encodes from mic PCM into the configured format when supported.
#include "bk_video_recorder_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Audio recorder device handle
typedef void *audio_recorder_device_handle_t;

// Audio recorder device configuration
typedef struct
{
    uint32_t audio_channels;   // Audio channels (0 = no audio)
    uint32_t audio_rate;       // Audio sample rate in Hz
    uint32_t audio_bits;       // Bits per sample
    uint32_t audio_format;     // Audio format (VIDEO_RECORD_AUDIO_FORMAT_PCM, VIDEO_RECORD_AUDIO_FORMAT_AAC, etc.)
} audio_recorder_device_cfg_t;

avdk_err_t audio_recorder_device_init(const audio_recorder_device_cfg_t *cfg, audio_recorder_device_handle_t *handle);
avdk_err_t audio_recorder_device_deinit(audio_recorder_device_handle_t handle);
avdk_err_t audio_recorder_device_start(audio_recorder_device_handle_t handle);
avdk_err_t audio_recorder_device_stop(audio_recorder_device_handle_t handle);
avdk_err_t audio_recorder_device_read(audio_recorder_device_handle_t handle, uint8_t *buffer, uint32_t buffer_size, uint32_t *data_len);

#ifdef __cplusplus
}
#endif

#endif /* _AUDIO_RECORDER_DEVICE_H_ */
