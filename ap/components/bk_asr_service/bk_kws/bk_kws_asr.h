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

void bk_kws_init(void *arg);
int  bk_tflite_ASR_Recog(short *buf, int buf_len, const char **text, float *score,int16_t *result);

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

#ifdef __cplusplus
}
#endif

