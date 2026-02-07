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

#ifndef _AUDIO_PLAYER_DEVICE_H_
#define _AUDIO_PLAYER_DEVICE_H_

#include <common/bk_include.h>
#include <components/avdk_utils/avdk_error.h>
#include <components/bk_video_player/bk_video_player_types.h>

#ifdef __cplusplus
extern "C" {
#endif

// Audio player device handle
typedef void *audio_player_device_handle_t;

// Audio player device configuration
typedef struct
{
    uint32_t channels;        // Audio channels
    uint32_t sample_rate;    // Sample rate in Hz
    uint32_t bits_per_sample; // Bits per sample
    video_player_audio_format_t format; // Audio format
    uint32_t frame_size;     // Frame size in bytes
} audio_player_device_cfg_t;

/**
 * @brief Initialize audio player device
 *
 * @param cfg Configuration parameters
 * @param handle Output handle pointer
 * @return avdk_err_t
 *         - AVDK_ERR_OK: success
 *         - AVDK_ERR_INVAL: invalid parameter
 *         - AVDK_ERR_NOMEM: memory allocation failed
 *         - AVDK_ERR_HWERROR: hardware error
 */
avdk_err_t audio_player_device_init(const audio_player_device_cfg_t *cfg, audio_player_device_handle_t *handle);

/**
 * @brief Deinitialize audio player device
 *
 * @param handle Device handle
 * @return avdk_err_t
 *         - AVDK_ERR_OK: success
 *         - AVDK_ERR_INVAL: invalid handle
 */
avdk_err_t audio_player_device_deinit(audio_player_device_handle_t handle);

/**
 * @brief Start audio player device
 *
 * @param handle Device handle
 * @return avdk_err_t
 *         - AVDK_ERR_OK: success
 *         - AVDK_ERR_INVAL: invalid handle
 *         - AVDK_ERR_HWERROR: hardware error
 */
avdk_err_t audio_player_device_start(audio_player_device_handle_t handle);

/**
 * @brief Stop audio player device
 *
 * @param handle Device handle
 * @return avdk_err_t
 *         - AVDK_ERR_OK: success
 *         - AVDK_ERR_INVAL: invalid handle
 */
avdk_err_t audio_player_device_stop(audio_player_device_handle_t handle);

/**
 * @brief Write PCM data to audio player device
 *
 * @param handle Device handle
 * @param data PCM data buffer
 * @param len Data length in bytes
 * @return int32_t
 *         - >= 0: number of bytes written
 *         - < 0: error code
 */
int32_t audio_player_device_write_frame_data(audio_player_device_handle_t handle, const char *data, uint32_t len);

/**
 * @brief Set volume (0-100)
 *
 * @param handle Device handle
 * @param volume Volume level (0-100)
 * @return avdk_err_t
 *         - AVDK_ERR_OK: success
 *         - AVDK_ERR_INVAL: invalid handle or volume
 *         - AVDK_ERR_HWERROR: hardware error
 */
avdk_err_t audio_player_device_set_volume(audio_player_device_handle_t handle, uint8_t volume);

/**
 * @brief Set mute state
 *
 * @param handle Device handle
 * @param mute true to mute, false to unmute
 * @return avdk_err_t
 *         - AVDK_ERR_OK: success
 *         - AVDK_ERR_INVAL: invalid handle
 *         - AVDK_ERR_HWERROR: hardware error
 */
avdk_err_t audio_player_device_set_mute(audio_player_device_handle_t handle, bool mute);

#ifdef __cplusplus
}
#endif

#endif /* _AUDIO_PLAYER_DEVICE_H_ */

