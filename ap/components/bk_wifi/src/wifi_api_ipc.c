/*
 * Copyright 2020-2025 Beken
 *
 * @file wdrv_api.c 
 * 
 * @brief Beken Wi-Fi Driver Command Control Entry
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 *     http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#include "wifi_api.h"
#include "wdrv_main.h"
#include <common/bk_include.h>
#include <common/bk_err.h>
#include <stdint.h>
#include <stdbool.h>
#include <os/str.h>
#include <os/mem.h>
#include <os/os.h>
#include "net.h"
#include "wdrv_cntrl.h"
#include "wdrv_co_list.h"
#include "wdrv_tx.h"
#include "wifi_api_ipc.h"


// bk_err_t wifi_send_api_cmd_no_arg(uint32_t cmd_id, ...)
// {
//     bk_err_t ret = BK_OK;
//     struct wifi_api_com_req com_req = { 0 };

//     req_no_param.cmd_hdr.cmd_id = cmd_id;
//     req_no_param.cmd_cfm.waitcfm = WDRV_CMD_WAITCFM;

//     wdrv_tx_msg((uint8_t *)&req_no_param, sizeof(req_no_param), &req_no_param.cmd_cfm, &ret);

//     WIFI_LOGD("wifi_send_api_cmd_no_arg cmd_id:%x ret:%d\n", cmd_id, ret);

//     return ret;
// }

// bk_err_t wifi_send_api_cmd_1_param(uint32_t cmd_id, uint32_t param1)
// {
//     bk_err_t ret = BK_OK;
//     struct wifi_api_req_1_agr req_no_arg = { 0 };

//     req_no_arg.cmd_hdr.cmd_id = cmd_id;
//     req_no_arg.cmd_cfm.waitcfm = WDRV_CMD_WAITCFM;
//     req_no_arg.cmd_cfm.waitcfm = WDRV_CMD_WAITCFM;

//     wdrv_tx_msg((uint8_t *)&req_no_arg, sizeof(req_no_arg), &req_no_arg.cmd_cfm, &ret);

//     WIFI_LOGD("wifi_send_api_cmd_no_arg cmd_id:%x ret:%d\n", cmd_id, ret);

//     return ret;
// }

bk_err_t wifi_send_com_api_cmd(uint32_t cmd_id, uint32_t argc, ...)
{
    bk_err_t ret = BK_OK;
    int wdrv_ret = 0;

    struct wifi_api_com_req com_req = { 0 };

    com_req.cmd_hdr.cmd_id = cmd_id;
    com_req.cmd_cfm.waitcfm = WDRV_CMD_WAITCFM;

    if (argc)
    {
        BK_ASSERT (argc <= WIFI_API_IPC_COM_REQ_MAX_ARGC);
        com_req.arg_info.argc = argc;

        va_list args;
        va_start(args, argc);
        for (int i = 0; i < argc; i++)
        {
            com_req.arg_info.args[i] = va_arg(args, uint32_t);
            //WIFI_LOGI("arg[%d]:%x\n", i, com_req.arg_info.args[i]);
        }
        va_end(args);
    }
    wdrv_ret = wdrv_tx_msg((uint8_t *)&com_req, sizeof(com_req), &com_req.cmd_cfm, (uint8_t *)&ret);

    //WIFI_LOGI("wifi_send_com_api_cmd cmd_id:%x argc:%d ret:%d\n", cmd_id, com_req.arg_info.argc, ret);
    if (wdrv_ret < 0)
    {
        //WIFI_LOGI("wifi_send_com_api_cmd FAILED, cmd_id:%x argc:%d ret:%d\n", cmd_id, com_req.arg_info.argc, ret);
        return BK_FAIL;
    }

    return ret;
}