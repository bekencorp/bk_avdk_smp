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
#include <os/mem.h>
#include <components/log.h>
#include <components/avdk_utils/avdk_error.h>
#include <components/bk_frame_buffer.h>

#include "driver/int.h"
#include "driver/sys_pm.h"

#include "sys_driver.h"

#include "hw_encoder_ctlr.h"

#define TAG "hw_enc_ctlr"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define HW_ENCODER_TASK_PRIO        5
#define HW_ENCODER_TASK_STACK_SIZE  (4 * 1024)
#define HW_ENCODER_QUEUE_SIZE       16

// 编码器注册信息
typedef struct encoder_node {
    hw_encoder_type_t type;
    void *encoder_id;
    struct encoder_node *next;
} encoder_node_t;

// 硬件编码器控制器（单例）
typedef struct {
    beken_thread_t task;
    beken_queue_t msg_queue;
    beken_mutex_t mutex;
    
    encoder_node_t *encoder_list;  // 注册的编码器链表
    uint32_t encoder_count;        // 注册的编码器数量
    bool hw_initialized;           // 硬件是否已初始化
} hw_encoder_ctlr_t;

// TODO FIX
#define REG_SYS_BASE_ADDR  0x48000000

extern void h264_vcenc_memalloc_register(void* (*pmalloc)(size_t), void (*pfree)(void*));
extern void h264_vcenc_isr(void);

static hw_encoder_ctlr_t *g_hw_encoder_ctlr = NULL;

#ifdef CONFIG_H264_ENCODER_USE_OS_MALLOC
static void* encoder_malloc(size_t size) {
    return os_malloc(size);
}

static void encode_free(void* pbuf){
    os_free(pbuf);
}
#else
void* encoder_malloc(size_t size) {
    return bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, size);
}

void encode_free(void* pbuf){
    bk_frame_buffer_free(pbuf);
}
#endif // CONFIG_H264_ENCODER_USE_OS_MALLOC

static void encoder_int_isr()
{
    h264_vcenc_isr();
}

static void encoder_int_register(void)
{
    bk_int_isr_register(INT_SRC_H26E, (int_group_isr_t)&encoder_int_isr, NULL);
#if CONFIG_SOC_SMP
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_H26E, 1);
#else
    sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_H26E, 1);
#endif
}

static void encoder_int_deregister(void)
{
#if CONFIG_SOC_SMP
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_H26E, 0);
#else
    sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_H26E, 0);
#endif
    bk_int_isr_unregister(INT_SRC_H26E);
}

// 硬件初始化（根据实际硬件实现）
static avdk_err_t hw_encoder_hw_init(void)
{
    LOGI("Hardware encoder init\r\n");

    // h264e pwd enable
    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_H26E, PM_POWER_MODULE_STATE_ON);

    // h264e clock sel
    sys_drv_h265_cksel_clkdiv_set(CKSEL_H265_160M, 1);

    // h264e clock enable
    bk_pm_clock_ctrl(PM_CLK_ID_H26E, PM_CLK_CTRL_PWR_UP);

    encoder_int_register();
    h264_vcenc_memalloc_register(encoder_malloc, encode_free);

    return AVDK_ERR_OK;
}

// 硬件反初始化
static avdk_err_t hw_encoder_hw_deinit(void)
{
    LOGI("Hardware encoder deinit\r\n");

    encoder_int_deregister();

    // h264e clock disable
    bk_pm_clock_ctrl(PM_CLK_ID_H26E, PM_CLK_CTRL_PWR_DOWN);

    // h264e pwd disable
    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_H26E, PM_POWER_MODULE_STATE_OFF);

    return AVDK_ERR_OK;
}

// 硬件控制器任务
static void hw_encoder_task(void *arg)
{
    hw_encoder_msg_t msg;
    avdk_err_t ret;
    
    LOGI("Hardware encoder task started\r\n");
    
    while (1) {
        if (rtos_pop_from_queue(&g_hw_encoder_ctlr->msg_queue, &msg, BEKEN_WAIT_FOREVER) == kNoErr) {
            // 执行回调函数
            if (msg.callback) {
                ret = msg.callback(msg.param);
                if (ret != AVDK_ERR_OK) {
                    LOGE("Callback execution failed: %d\r\n", ret);
                }
            }
            
            // 如果有信号量，通知完成
            if (msg.sem) {
                rtos_set_semaphore(msg.sem);
            }
        }
    }
}

// 创建硬件控制器
static avdk_err_t hw_encoder_ctlr_create(void)
{
    avdk_err_t ret;
    
    if (g_hw_encoder_ctlr != NULL) {
        return AVDK_ERR_OK;  // 已创建
    }
    
    g_hw_encoder_ctlr = (hw_encoder_ctlr_t *)os_malloc(sizeof(hw_encoder_ctlr_t));
    if (!g_hw_encoder_ctlr) {
        LOGE("Malloc hw encoder ctlr failed\r\n");
        return AVDK_ERR_NOMEM;
    }
    
    os_memset(g_hw_encoder_ctlr, 0, sizeof(hw_encoder_ctlr_t));
    
    // 创建互斥锁
    ret = rtos_init_mutex(&g_hw_encoder_ctlr->mutex);
    if (ret != kNoErr) {
        LOGE("Create mutex failed\r\n");
        os_free(g_hw_encoder_ctlr);
        g_hw_encoder_ctlr = NULL;
        return AVDK_ERR_NO_RESOURCE;
    }
    
    // 创建消息队列
    ret = rtos_init_queue(&g_hw_encoder_ctlr->msg_queue, 
                          "hw_enc_queue",
                          sizeof(hw_encoder_msg_t),
                          HW_ENCODER_QUEUE_SIZE);
    if (ret != kNoErr) {
        LOGE("Create queue failed\r\n");
        rtos_deinit_mutex(&g_hw_encoder_ctlr->mutex);
        os_free(g_hw_encoder_ctlr);
        g_hw_encoder_ctlr = NULL;
        return AVDK_ERR_NO_RESOURCE;
    }
    
    // 创建任务
    ret = rtos_create_thread(&g_hw_encoder_ctlr->task,
                            HW_ENCODER_TASK_PRIO,
                            "hw_encoder",
                            (beken_thread_function_t)hw_encoder_task,
                            HW_ENCODER_TASK_STACK_SIZE,
                            NULL);
    if (ret != kNoErr) {
        LOGE("Create task failed\r\n");
        rtos_deinit_queue(&g_hw_encoder_ctlr->msg_queue);
        rtos_deinit_mutex(&g_hw_encoder_ctlr->mutex);
        os_free(g_hw_encoder_ctlr);
        g_hw_encoder_ctlr = NULL;
        return AVDK_ERR_NO_RESOURCE;
    }
    
    LOGI("Hardware encoder controller created\r\n");
    return AVDK_ERR_OK;
}

// 销毁硬件控制器
static avdk_err_t hw_encoder_ctlr_destroy(void)
{
    if (!g_hw_encoder_ctlr) {
        return AVDK_ERR_OK;
    }
    
    // 删除任务
    if (g_hw_encoder_ctlr->task) {
        rtos_delete_thread(&g_hw_encoder_ctlr->task);
        g_hw_encoder_ctlr->task = NULL;
    }
    
    // 删除队列
    rtos_deinit_queue(&g_hw_encoder_ctlr->msg_queue);
    
    // 删除互斥锁
    rtos_deinit_mutex(&g_hw_encoder_ctlr->mutex);
    
    // 释放内存
    os_free(g_hw_encoder_ctlr);
    g_hw_encoder_ctlr = NULL;
    
    LOGI("Hardware encoder controller destroyed\r\n");
    return AVDK_ERR_OK;
}

avdk_err_t hw_encoder_register(hw_encoder_type_t type, void *encoder_id)
{
    encoder_node_t *node = NULL;
    avdk_err_t ret = AVDK_ERR_OK;
    
    if (!encoder_id) {
        LOGE("Invalid encoder_id\r\n");
        return AVDK_ERR_INVAL;
    }
    
    // 创建控制器（如果还未创建）
    if (!g_hw_encoder_ctlr) {
        ret = hw_encoder_ctlr_create();
        if (ret != AVDK_ERR_OK) {
            return ret;
        }
    }
    
    rtos_lock_mutex(&g_hw_encoder_ctlr->mutex);
    
    // 检查是否已注册
    node = g_hw_encoder_ctlr->encoder_list;
    while (node) {
        if (node->encoder_id == encoder_id) {
            LOGW("Encoder already registered\r\n");
            rtos_unlock_mutex(&g_hw_encoder_ctlr->mutex);
            return AVDK_ERR_OK;
        }
        node = node->next;
    }
    
    // 创建新节点
    node = (encoder_node_t *)os_malloc(sizeof(encoder_node_t));
    if (!node) {
        LOGE("Malloc encoder node failed\r\n");
        rtos_unlock_mutex(&g_hw_encoder_ctlr->mutex);
        return AVDK_ERR_NOMEM;
    }
    
    node->type = type;
    node->encoder_id = encoder_id;
    node->next = g_hw_encoder_ctlr->encoder_list;
    g_hw_encoder_ctlr->encoder_list = node;
    g_hw_encoder_ctlr->encoder_count++;
    
    // 如果是第一个注册的编码器，初始化硬件
    if (g_hw_encoder_ctlr->encoder_count == 1 && !g_hw_encoder_ctlr->hw_initialized) {
        ret = hw_encoder_hw_init();
        if (ret == AVDK_ERR_OK) {
            g_hw_encoder_ctlr->hw_initialized = true;
        }
    }
    
    LOGI("Encoder registered, type=%d, count=%d\r\n", type, g_hw_encoder_ctlr->encoder_count);
    
    rtos_unlock_mutex(&g_hw_encoder_ctlr->mutex);
    
    return ret;
}

avdk_err_t hw_encoder_unregister(void *encoder_id)
{
    encoder_node_t *node = NULL;
    encoder_node_t *prev = NULL;
    avdk_err_t ret = AVDK_ERR_OK;
    
    if (!encoder_id || !g_hw_encoder_ctlr) {
        return AVDK_ERR_INVAL;
    }
    
    rtos_lock_mutex(&g_hw_encoder_ctlr->mutex);
    
    // 查找并删除节点
    node = g_hw_encoder_ctlr->encoder_list;
    while (node) {
        if (node->encoder_id == encoder_id) {
            if (prev) {
                prev->next = node->next;
            } else {
                g_hw_encoder_ctlr->encoder_list = node->next;
            }
            
            os_free(node);
            g_hw_encoder_ctlr->encoder_count--;
            
            LOGI("Encoder unregistered, count=%d\r\n", g_hw_encoder_ctlr->encoder_count);
            
            // 如果所有编码器都注销了，反初始化硬件
            if (g_hw_encoder_ctlr->encoder_count == 0 && g_hw_encoder_ctlr->hw_initialized) {
                ret = hw_encoder_hw_deinit();
                g_hw_encoder_ctlr->hw_initialized = false;
                
                // 销毁控制器
                rtos_unlock_mutex(&g_hw_encoder_ctlr->mutex);
                hw_encoder_ctlr_destroy();
                return ret;
            }
            
            break;
        }
        prev = node;
        node = node->next;
    }
    
    rtos_unlock_mutex(&g_hw_encoder_ctlr->mutex);
    
    return ret;
}

avdk_err_t hw_encoder_send_msg(hw_encoder_msg_t *msg, uintptr_t timeout)
{
    if (!msg || !g_hw_encoder_ctlr) {
        return AVDK_ERR_INVAL;
    }
    
    if (rtos_push_to_queue(&g_hw_encoder_ctlr->msg_queue, msg, timeout) != kNoErr) {
        LOGE("Push message to queue failed\r\n");
        return AVDK_ERR_GENERIC;
    }
    return AVDK_ERR_OK;
}

