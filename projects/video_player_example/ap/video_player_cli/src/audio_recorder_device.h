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
#include "components/bk_video_recorder_types.h"

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
    uint32_t audio_bits;       // Audio bits per sample
    uint32_t audio_format;     // Audio format (VIDEO_RECORD_AUDIO_FORMAT_PCM, VIDEO_RECORD_AUDIO_FORMAT_AAC, etc.)
} audio_recorder_device_cfg_t;

/**
 * @brief Initialize audio recorder device
 *
 * @param cfg Configuration parameters
 * @param handle Output handle pointer
 * @return avdk_err_t
 *         - AVDK_ERR_OK: success
 *         - AVDK_ERR_INVAL: invalid parameter
 *         - AVDK_ERR_NOMEM: memory allocation failed
 *         - AVDK_ERR_HWERROR: hardware error
 */
avdk_err_t audio_recorder_device_init(const audio_recorder_device_cfg_t *cfg, audio_recorder_device_handle_t *handle);

/**
 * @brief Deinitialize audio recorder device
 *
 * @param handle Device handle
 * @return avdk_err_t
 *         - AVDK_ERR_OK: success
 *         - AVDK_ERR_INVAL: invalid handle
 */
avdk_err_t audio_recorder_device_deinit(audio_recorder_device_handle_t handle);

/**
 * @brief Start audio recorder device
 *
 * @param handle Device handle
 * @return avdk_err_t
 *         - AVDK_ERR_OK: success
 *         - AVDK_ERR_INVAL: invalid handle
 *         - AVDK_ERR_HWERROR: hardware error
 */
avdk_err_t audio_recorder_device_start(audio_recorder_device_handle_t handle);

/**
 * @brief Stop audio recorder device
 *
 * @param handle Device handle
 * @return avdk_err_t
 *         - AVDK_ERR_OK: success
 *         - AVDK_ERR_INVAL: invalid handle
 */
avdk_err_t audio_recorder_device_stop(audio_recorder_device_handle_t handle);

/**
 * @brief Read audio data from recorder device
 *
 * @param handle Device handle
 * @param buffer Output buffer to store audio data
 * @param buffer_size Buffer size in bytes
 * @param data_len Output parameter, actual data length read
 * @return avdk_err_t
 *         - AVDK_ERR_OK: success, data_len contains actual bytes read
 *         - AVDK_ERR_INVAL: invalid parameter
 *         - AVDK_ERR_NODATA: no data available
 */
avdk_err_t audio_recorder_device_read(audio_recorder_device_handle_t handle, uint8_t *buffer, uint32_t buffer_size, uint32_t *data_len);

#ifdef __cplusplus
}
#endif

#endif /* _AUDIO_RECORDER_DEVICE_H_ */

