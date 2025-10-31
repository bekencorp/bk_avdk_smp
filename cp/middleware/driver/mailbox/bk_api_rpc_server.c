// Copyright 2025 Beken
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

#include <os/mem.h>
#include <common/bk_err.h>
#include "bk_api_rpc.h"
#include "bk_api_ipc.h"



#define TAG "bk_rpc"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)



uint32_t bk_api_rpc_callback(uint8_t *data, uint32_t size, void *param, ipc_obj_t ipc_obj);

BK_IPC_CHANNEL_DEF(bk_api_rpc);
BK_IPC_CHANNEL_REGISTER(bk_api_rpc, IPC_ROUTE_CPU0_CPU1, bk_api_rpc_callback, NULL, NULL);

bk_err_t bk_api_rpc_init(void)
{
    return BK_OK;
}

uint32_t bk_api_rpc_callback(uint8_t *data, uint32_t size, void *param, ipc_obj_t ipc_obj)
{
    bk_err_t ret = BK_FAIL;

    if (data == NULL || size < sizeof(uint16_t)) {
        LOGE("%s invalid parameters\n", __func__);
        return BK_ERR_PARAM;
    }

    uint16_t event = *((uint16_t *)(data));

    switch (event)
    {
        case BK_API_RPC_EVENT_GET_CHIP_UID:
        {
            if (size < sizeof(bk_api_rpc_get_uid_msg_t)) {
                LOGE("%s invalid message size: %d, expected: %d\n", __func__, size, sizeof(bk_api_rpc_get_uid_msg_t));
                return BK_ERR_PARAM;
            }

            bk_api_rpc_get_uid_msg_t *proxy = (bk_api_rpc_get_uid_msg_t *)data;

            if (proxy->out_para_size != BK_UID_SIZE) {
                LOGE("%s invalid out_para_size: %d, expected: 32\n", __func__, proxy->out_para_size);
                return BK_ERR_PARAM;
            }

            unsigned char uid[BK_UID_SIZE] = {0};
            ret = bk_uid_get_data(uid);
            if (ret != BK_OK)
            {
                LOGE("%s get uid failed\n", __func__);
                return ret;
            }
            memcpy(proxy->out_para, uid, BK_UID_SIZE);
        }
        break;

        default:
            LOGW("%s not found this event:%d\n", __func__, event);
            break;
    }

    return ret;
}




