// Copyright 2020-2025 Beken
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

#include <common/bk_include.h>
#include <stdio.h>
#include <string.h>
#include <components/log.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>

#include "components/bluetooth/bk_ble.h"
#include "components/bluetooth/bk_dm_bluetooth.h"
#include "modules/pm.h"
#include "ble_adv.h"

#define TAG "ble_adv"
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)

#define BLE_ADV_CMD_TIMEOUT_MS      4000
#define BLE_ADV_LOCAL_NAME          "bk7259_ble_test"

#ifndef UNKNOW_ACT_IDX
#define UNKNOW_ACT_IDX              0xFFU
#endif

static beken_semaphore_t s_ble_adv_sema = NULL;
static volatile ble_err_t s_ble_adv_cmd_status = BK_ERR_BLE_SUCCESS;
static bool s_ble_adv_inited = false;

static void ble_adv_cmd_cb(ble_cmd_t cmd, ble_cmd_param_t *param)
{
    s_ble_adv_cmd_status = param->status;

    switch (cmd)
    {
        case BLE_CREATE_ADV:
        case BLE_SET_ADV_DATA:
        case BLE_SET_RSP_DATA:
        case BLE_START_ADV:
        case BLE_STOP_ADV:
        case BLE_DELETE_ADV:
            if (s_ble_adv_sema != NULL)
            {
                rtos_set_semaphore(&s_ble_adv_sema);
            }
            break;

        default:
            break;
    }
}

static void ble_adv_notice_cb(ble_notice_t notice, void *param)
{
    switch (notice)
    {
        case BLE_5_CONNECT_EVENT:
            LOGI("ble connected\n");
            break;

        case BLE_5_DISCONNECT_EVENT:
            LOGI("ble disconnected\n");
            break;

        default:
            break;
    }
}

/* Build the advertising data: Flags + Complete Local Name. */
static uint16_t ble_adv_build_data(uint8_t *adv_data, uint16_t max_len)
{
    uint16_t idx = 0;
    uint8_t name_len = (uint8_t)os_strlen(BLE_ADV_LOCAL_NAME);

    /* AD structure: Flags (LE General Discoverable, BR/EDR not supported). */
    adv_data[idx++] = 0x02;
    adv_data[idx++] = 0x01;
    adv_data[idx++] = 0x06;

    /* AD structure: Complete Local Name. */
    adv_data[idx++] = name_len + 1;
    adv_data[idx++] = 0x09;
    os_memcpy(&adv_data[idx], BLE_ADV_LOCAL_NAME, name_len);
    idx += name_len;

    (void)max_len;
    return idx;
}

static int ble_adv_start(void)
{
    ble_adv_param_t adv_param;
    uint8_t adv_data[31] = {0};
    uint16_t adv_len = 0;
    uint8_t actv_idx = 0;
    int ret = BK_FAIL;

    actv_idx = bk_ble_get_idle_actv_idx_handle();
    if (actv_idx == UNKNOW_ACT_IDX)
    {
        LOGW("no idle activity index\n");
        return BK_FAIL;
    }

    /* set adv parameters */
    os_memset(&adv_param, 0, sizeof(ble_adv_param_t));
    adv_param.chnl_map = 7;
    adv_param.adv_intv_min = 120;
    adv_param.adv_intv_max = 160;
    adv_param.own_addr_type = OWN_ADDR_TYPE_PUBLIC_ADDR;
    adv_param.adv_type = 0;
    adv_param.adv_prop = 3;
    adv_param.prim_phy = 1;
    adv_param.second_phy = 1;

    ret = bk_ble_create_advertising(actv_idx, &adv_param, ble_adv_cmd_cb);
    if (ret != BK_ERR_BLE_SUCCESS)
    {
        LOGW("config adv parameters failed %d\n", ret);
        goto error;
    }

    ret = rtos_get_semaphore(&s_ble_adv_sema, BLE_ADV_CMD_TIMEOUT_MS);
    if (ret != BK_OK)
    {
        LOGW("wait semaphore failed at %d, %d\n", ret, __LINE__);
        goto error;
    }
    LOGD("create adv success\n");

    /* set adv data */
    adv_len = ble_adv_build_data(adv_data, sizeof(adv_data));

    ret = bk_ble_set_adv_data(actv_idx, adv_data, adv_len, ble_adv_cmd_cb);
    if (ret != BK_ERR_BLE_SUCCESS)
    {
        LOGW("set adv data failed %d\n", ret);
        goto error;
    }

    ret = rtos_get_semaphore(&s_ble_adv_sema, BLE_ADV_CMD_TIMEOUT_MS);
    if (ret != BK_OK)
    {
        LOGW("wait semaphore failed at %d, %d\n", ret, __LINE__);
        goto error;
    }
    LOGD("set adv data success\n");

    /* start adv */
    ret = bk_ble_start_advertising(actv_idx, 0, ble_adv_cmd_cb);
    if (ret != BK_ERR_BLE_SUCCESS)
    {
        LOGW("start adv failed %d\n", ret);
        goto error;
    }

    ret = rtos_get_semaphore(&s_ble_adv_sema, BLE_ADV_CMD_TIMEOUT_MS);
    if (ret != BK_OK)
    {
        LOGW("wait semaphore failed at %d, %d\n", ret, __LINE__);
        goto error;
    }
    LOGD("start adv success, name: %s\n", BLE_ADV_LOCAL_NAME);

    return ret;

error:
    return BK_FAIL;
}

int ble_adv_init(void)
{
    int ret = BK_OK;

    if (s_ble_adv_inited)
    {
        return BK_OK;
    }

    if (s_ble_adv_sema == NULL)
    {
        ret = rtos_init_semaphore(&s_ble_adv_sema, 1);
        if (ret != BK_OK)
        {
            LOGE("init semaphore failed %d\n", ret);
            return ret;
        }
    }

    bk_ble_set_notice_cb(ble_adv_notice_cb);

    s_ble_adv_inited = true;

#if CONFIG_BLUETOOTH_SUPPORT_AP_PWD_ALL
    if (bk_pm_ap_first_boot_get())
#endif
    {

        ret = ble_adv_start();
        if (ret != BK_OK)
        {
            LOGE("start adv failed %d\n", ret);
            return ret;
        }
    }
    return BK_OK;
}