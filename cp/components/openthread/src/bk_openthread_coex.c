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

#include "stdint.h"

#include "sys_ll.h"

#include "bk_rf_internal.h"

#include "bk_openthread.h"
#include "openthread/instance.h"
#include "openthread/thread.h"

#if CONFIG_OT_TRIP_COEX_EN
#define TAG "ot_coex"
#define BK_OT_RF_PREEMPTED_LIMIT    2
static uint8_t uOtRfPreemptCnt = 0;

uint8_t bk_ot_task_type = RF_TASK_TYPE_THREAD_DISABLED;
uint8_t bk_ot_rf_priority = RF_PRIORITY_THREAD_NORMAL;

uint8_t bk_ot_get_thread_task_type(void)
{
    return bk_ot_task_type;
}
uint8_t bk_ot_set_thread_task_type(uint8_t tskType)
{
    if((tskType > RF_TASK_TYPE_THREAD_INIT) && (tskType < RF_TASK_TYPE_THREAD_END))
        bk_ot_task_type = tskType;
    return bk_ot_task_type;
}
/**
 * @brief openthread is during attaching progress
 * 
 * @return true :attaching process false: attaching process completed
 */
bool bk_ot_coex_is_attaching_process(void)
{
    if(bk_ot_get_single_instance() == NULL)
    {
        BK_LOGE(TAG, "%s ot not initial\n", __func__);
        return false;
    }
    if(!otIp6IsEnabled(bk_ot_get_single_instance()))
    {
        bk_ot_set_thread_task_type(RF_TASK_TYPE_THREAD_DISABLED);
        return false;
    }
    
    otDeviceRole role = otThreadGetDeviceRole(bk_ot_get_single_instance());
    if(role < OT_DEVICE_ROLE_CHILD)
    {
        bk_ot_set_thread_task_type(RF_TASK_TYPE_THREAD_ATTACHING);
        return false;
    }
    else
    {
        bk_ot_set_thread_task_type(RF_TASK_TYPE_THREAD_CONNECTED);
        return false;
    }
}


bool bk_ot_rf_is_thread(void)
{
    return (3 == sys_ll_get_cpu_storage_connect_op_select_rf_source());
}
/**
 * @brief counter rf preempted for tx
 * 
 */
void bk_ot_rf_counter_preempted(void)
{
    if(3 != sys_ll_get_cpu_storage_connect_op_select_rf_source())
    {
        if(uOtRfPreemptCnt < BK_OT_RF_PREEMPTED_LIMIT)
            uOtRfPreemptCnt++;
        else
            uOtRfPreemptCnt = BK_OT_RF_PREEMPTED_LIMIT;
        bk_ot_set_thread_task_type(RF_TASK_TYPE_THREAD_RETRY);
    }
    else
    {
        uOtRfPreemptCnt = 0;
    }
}
/**
 * @brief thread rf is preempted
 * 
 * @return true :is preempted
 * @return false :not preempted
 */
bool bk_ot_coex_is_preempted(void)
{
    if(uOtRfPreemptCnt >= BK_OT_RF_PREEMPTED_LIMIT)
        return true;
    else
        return false;
}

/**
 * @brief update rf level
 * 
 * @return true :high
 * @return false :low
 */
bool bk_ot_update_rf_lvl(void)
{
    if(bk_ot_coex_is_preempted() || bk_ot_coex_is_attaching_process())
        return true;
    else
    {
        bk_ot_set_thread_task_type(RF_TASK_TYPE_THREAD_CONNECTED);
        return false;
    }
}

uint8_t bk_ot_update_rf_priority(uint8_t uPrio)
{
    bk_ot_rf_priority = uPrio;
    return bk_ot_rf_priority;
}
uint8_t bk_ot_get_rf_priority(void)
{
    return bk_ot_rf_priority;
}
#endif