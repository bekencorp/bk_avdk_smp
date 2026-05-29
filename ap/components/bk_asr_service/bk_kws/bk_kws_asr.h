// Copyright 2023-2024 Beken
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

#include <os/os.h>

#ifdef __cplusplus
extern "C" {
#endif
// kws words
// {
    // 7 if 'Volume Down_' in f else  #
    // 6 if 'Volume Up_' in f else  #
    // 5 if 'Next song_' in f else  #
    // 4 if 'Stop Play_' in f else  #
    // 3 if 'Play Music_' in f else  #
    // 2 if 'Byebye_' in f else  #
    // 1 if 'Armino_' in f else  #
    // 0 #
// };

typedef enum {
    BK_KWS_NONE        = 0,
    BK_KWS_ARMINO      = 1,
    BK_KWS_BYEBYE      = 2,
    BK_KWS_PLAY_MUSIC  = 3,
    BK_KWS_STOP_PLAY   = 4,
    BK_KWS_NEXT_SONG   = 5,
    BK_KWS_VOLUME_UP   = 6,
    BK_KWS_VOLUME_DOWN = 7,
    BK_KWS_MAX_WORDS,
} bk_kws_word_t;



/**
 * @brief Initialize TFLite ASR service.
 *
 * This function prepares the KWS/ASR runtime and related resources.
 *
 * @return
 *      - 1: initialization succeeded
 *      - Other values: initialization failed
 */
int bk_tflite_asr_init(void);

/**
 * @brief Run one-shot ASR recognition.
 *
 * @param read_buf Input PCM buffer pointer.
 * @param read_size Input PCM buffer size in bytes.
 * @param p1 Output text pointer container (implementation dependent).
 * @param p2 Output score pointer container (implementation dependent).
 *
 * @return
 *      - 1: recognition succeeded
 *      - 0: recognition failed or no valid keyword
 *
 * @note
 *      - The recognized keyword content is returned in `text`
 *        (mapped to `p1` in this wrapper), not by this function's
 *        return value.
 */
int bk_tflite_asr_recog(void *read_buf, uint32_t read_size, void *p1, void *p2);

/**
 * @brief Deinitialize TFLite ASR service.
 *
 * Release runtime resources allocated by @ref bk_tflite_asr_init.
 *
 */
void bk_tflite_asr_deinit(void);



#ifdef __cplusplus
}
#endif

