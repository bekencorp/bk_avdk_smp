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

#define BK_TFLITE_ASR_MODEL_WAKEUP 0
#define BK_TFLITE_ASR_MODEL_CMDS   1

typedef struct {
    int (*open)(const char *path, void **handle);
    int (*read)(void *handle, uint8_t *buf, uint32_t size, uint32_t *read_size);
    int (*size)(void *handle, uint32_t *file_size);
    int (*close)(void *handle);
} bk_tflite_asr_model_file_ops_t;

int bk_tflite_asr_register_model_file_ops(const bk_tflite_asr_model_file_ops_t *ops);
int bk_tflite_asr_set_model_from_file(int model_id, const char *path);
int bk_tflite_asr_set_model_from_array(int model_id);
int bk_tflite_asr_switch_model(int model_id);

typedef enum {
    BK_KWS_NONE = 0,
    BK_KWS_ARMINO,
    BK_KWS_BYEBYE,
    BK_KWS_JINRUBIAODING,
    BK_KWS_WANCHENGBIAODING,
    BK_KWS_DAKASHEXIANG,
    BK_KWS_GUANBISHEXIANG,
    BK_KWS_SHANGXIADUNQI,
    BK_KWS_ZUOYOUYAOBAI,
    BK_KWS_QIANHOUBAIDONG,
    BK_KWS_YAOTOUHUANGNAO,
    BK_KWS_SHENLANYAO,
    BK_KWS_DAZHAOHU,
    BK_KWS_NAOYANGYANG,
    BK_KWS_ZUOZHUANWAN,
    BK_KWS_YOUZHUANWAN,
    BK_KWS_QIANJIN,
    BK_KWS_HOUTUI,
    BK_KWS_ZUOXIA,
    BK_KWS_AONAO,
    BK_KWS_BAOBAO,
    BK_KWS_SAJIAO,
    BK_KWS_YOUYONG,
    BK_KWS_SHENGQI,
    BK_KWS_QIQIU,
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

