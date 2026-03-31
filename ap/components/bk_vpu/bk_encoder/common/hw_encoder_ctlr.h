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

#include <components/avdk_utils/avdk_error.h>
#include <os/os.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HW_ENCODER_TYPE_H264,
    HW_ENCODER_TYPE_JPEG,
} hw_encoder_type_t;

typedef enum {
    HW_ENCODER_MSG_ENCODE,
    HW_ENCODER_MSG_CONFIG,
    HW_ENCODER_MSG_RESET,
} hw_encoder_msg_type_t;

// 硬件编码器消息回调函数
typedef avdk_err_t (*hw_encoder_msg_cb_t)(void *param);

// 硬件编码器消息
typedef struct {
    hw_encoder_msg_type_t type;
    hw_encoder_msg_cb_t callback;
    void *param;
    beken_semaphore_t *sem;  // 用于同步等待
} hw_encoder_msg_t;

/**
 * @brief 注册编码器到硬件控制器
 * 
 * @param type 编码器类型
 * @param encoder_id 编码器ID（用于标识）
 * 
 * @return avdk_err_t 
 */
avdk_err_t hw_encoder_register(hw_encoder_type_t type, void *encoder_id);

/**
 * @brief 从硬件控制器注销编码器
 * 
 * @param encoder_id 编码器ID
 * 
 * @return avdk_err_t 
 */
avdk_err_t hw_encoder_unregister(void *encoder_id);

/**
 * @brief 发送消息给硬件控制器
 * 
 * @param msg 消息结构
 * @param timeout 超时时间
 * 
 * @return avdk_err_t 
 */
avdk_err_t hw_encoder_send_msg(hw_encoder_msg_t *msg, uintptr_t timeout);

#ifdef __cplusplus
}
#endif

