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

#include "video_pipeline_test.h"
#include <os/os.h>
#include <frame_buffer.h>

#define TAG "video_pipeline_test_common"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

/**
 * @brief 初始化测试环境
 *
 * @return 0 on success, negative on failure
 */
int video_pipeline_test_env_init(void)
{
    LOGI("Video pipeline test environment initialized successfully\n");
    return BK_OK;
}

/**
 * @brief 清理测试环境
 */
void video_pipeline_test_env_deinit(void)
{
    LOGI("Video pipeline test environment deinitialized\n");
}

/**
 * @brief 打印测试结果
 *
 * @param test_name 测试名称
 * @param result 测试结果
 */
void video_pipeline_test_print_result(const char *test_name, int result)
{
    if (result == BK_OK) {
        LOGI("[%s] Test PASSED\n", test_name);
    } else {
        LOGE("[%s] Test FAILED with result: %d\n", test_name, result);
    }
}