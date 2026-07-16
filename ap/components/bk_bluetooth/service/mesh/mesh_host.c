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

#include <os/os.h>
#include <components/log.h>
#include <components/system.h>

#include "bluetooth_internal.h"

#define TAG "mesh_host"

extern int bk_mesh_hci_ipc_driver_init(void);
extern int bk_mesh_hci_ipc_driver_deinit(void);
extern void freertos_zephyr_port_init(void);
extern int init_mem_slab_module(void);
extern void zephyr_ble_mesh_init(void *arg);
extern int zephyr_ble_mesh_deinit(void);

static uint8_t mesh_host_init_finish(int32_t reason)
{
    BK_LOGI(TAG, "mesh host init finish reason=%d\r\n", reason);
    return 0;
}

int bluetooth_host_init(void)
{
    int ret;

    freertos_zephyr_port_init();

    ret = bk_mesh_hci_ipc_driver_init();
    if (ret)
    {
        BK_LOGW(TAG, "mesh hci ipc driver init failed ret=%d\r\n", ret);
        return ret;
    }

    ret = init_mem_slab_module();
    if (ret)
    {
        BK_LOGW(TAG, "mesh mem slab init failed ret=%d\r\n", ret);
        return ret;
    }

    zephyr_ble_mesh_init((void *)mesh_host_init_finish);
    return BK_OK;
}

int bluetooth_host_deinit(void)
{
    int ret;

    ret = zephyr_ble_mesh_deinit();
    if (ret)
    {
        BK_LOGW(TAG, "mesh host thread deinit failed ret=%d\r\n", ret);
        return ret;
    }

    ret = bk_mesh_hci_ipc_driver_deinit();
    if (ret)
    {
        BK_LOGW(TAG, "mesh hci ipc driver deinit failed ret=%d\r\n", ret);
        return ret;
    }

    return BK_OK;
}

int bluetooth_get_mac(uint8_t *mac)
{
    if (!mac)
    {
        return BK_ERR_NULL_PARAM;
    }

    return bk_get_mac(mac, MAC_TYPE_BLUETOOTH);
}
