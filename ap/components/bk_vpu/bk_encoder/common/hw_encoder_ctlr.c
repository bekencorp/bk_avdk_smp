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

#include "modules/vcenc/vcenc_common.h"
#include "modules/vcenc/vcenc_h264_api.h"
#include "modules/vcenc/vcenc_jpeg_api.h"

#define TAG "hw_enc_ctlr"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define HW_ENCODER_TASK_STACK_SIZE  (4 * 1024)
#define HW_ENCODER_QUEUE_SIZE       16

/* Registered encoder list node */
typedef struct encoder_node {
    hw_encoder_type_t type;
    void *encoder_id;
    struct encoder_node *next;
} encoder_node_t;

/*
 * Hardware encoder controller singleton.
 *
 * Note: codec dispatch is now performed by the unified vcenc_isr (see
 * vcenc/common/src/vcenc_isr_common.c) which uses the active vcenc_common_t
 * pointer plus a HWIF_ENC_MODE sanity check, so this controller no longer
 * tracks an active_encoder_type.
 */
typedef struct {
	beken_thread_t task;
	beken_queue_t msg_queue;
	beken_mutex_t mutex;

	encoder_node_t *encoder_list;
	uint32_t encoder_count;
	bool hw_initialized;
} hw_encoder_ctlr_t;

// TODO FIX
#define REG_SYS_BASE_ADDR  0x48000000

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

static void encoder_int_register(void)
{
    bk_int_isr_register(INT_SRC_H26E, (int_group_isr_t)&vcenc_isr, NULL);
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

/* Hardware bring-up (clock/power/IRQ and VCENC alloc hooks) */
static avdk_err_t hw_encoder_hw_init(void)
{
    LOGI("Hardware encoder init\r\n");

    // h264e pwd enable
    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_H26E, PM_POWER_MODULE_STATE_ON);

    // h264e clock sel
#ifdef CONFIG_ENCODER_H264_CLK_240M
    sys_drv_h265_cksel_clkdiv_set(CKSEL_H265_240M, 0);
#else
    sys_drv_h265_cksel_clkdiv_set(CKSEL_H265_160M, 1);
#endif

    // h264e clock enable
    bk_pm_clock_ctrl(PM_CLK_ID_H26E, PM_CLK_CTRL_PWR_UP);

	encoder_int_register();
	vcenc_h264_memalloc_register(encoder_malloc, encode_free);
	vcenc_jpeg_memalloc_register(encoder_malloc, encode_free);

	return AVDK_ERR_OK;
}

/* Hardware shutdown */
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

/* Drop all pending queue messages; wake waiters blocked on msg.sem */
static void hw_encoder_ctlr_flush_queue(void)
{
    hw_encoder_msg_t msg;

    while (rtos_pop_from_queue(&g_hw_encoder_ctlr->msg_queue, &msg, 0) == kNoErr) {
        if (msg.sem) {
            rtos_set_semaphore(msg.sem);
        }
    }
}

/* Worker task: dequeue and run encode callbacks */
static void hw_encoder_task(void *arg)
{
    hw_encoder_msg_t msg;
    avdk_err_t ret;

    LOGI("Hardware encoder task started\r\n");

    while (1) {
        if (rtos_pop_from_queue(&g_hw_encoder_ctlr->msg_queue, &msg, BEKEN_WAIT_FOREVER) == kNoErr) {
            if (msg.type == HW_ENCODER_MSG_EXIT) {
                hw_encoder_ctlr_flush_queue();
                rtos_delete_thread(NULL);
                return;
            }

			if (msg.callback) {
				ret = msg.callback(msg.param);
				if (ret != AVDK_ERR_OK) {
					LOGE("%s %d Callback execution failed: %d\r\n", __func__, __LINE__, ret);
				}
			}

            /* Optional completion semaphore */
            if (msg.sem) {
                rtos_set_semaphore(msg.sem);
            }
        }
    }
}

/* Create singleton controller (mutex, queue, task) */
static avdk_err_t hw_encoder_ctlr_create(void)
{
    avdk_err_t ret;

    if (g_hw_encoder_ctlr != NULL) {
        return AVDK_ERR_OK;  /* already created */
    }

    g_hw_encoder_ctlr = (hw_encoder_ctlr_t *)os_malloc(sizeof(hw_encoder_ctlr_t));
    if (!g_hw_encoder_ctlr) {
        LOGE("Malloc hw encoder ctlr failed\r\n");
        return AVDK_ERR_NOMEM;
    }

	os_memset(g_hw_encoder_ctlr, 0, sizeof(hw_encoder_ctlr_t));

    ret = rtos_init_mutex(&g_hw_encoder_ctlr->mutex);
    if (ret != kNoErr) {
        LOGE("Create mutex failed\r\n");
        os_free(g_hw_encoder_ctlr);
        g_hw_encoder_ctlr = NULL;
        return AVDK_ERR_NO_RESOURCE;
    }

    /* Message queue */
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

    /* Worker thread */
    ret = rtos_create_hsram_thread(&g_hw_encoder_ctlr->task,
                            CONFIG_BK_ENCODER_HW_TASK_PRIORITY,
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

/* Tear down singleton controller */
static avdk_err_t hw_encoder_ctlr_destroy(void)
{
    if (!g_hw_encoder_ctlr) {
        return AVDK_ERR_OK;
    }

    /* Stop worker: flush pending msgs, notify exit, then wait for thread termination */
    if (g_hw_encoder_ctlr->task) {
        hw_encoder_msg_t exit_msg = {
            .type = HW_ENCODER_MSG_EXIT,
        };

        hw_encoder_ctlr_flush_queue();

        if (rtos_push_to_queue(&g_hw_encoder_ctlr->msg_queue, &exit_msg, BEKEN_WAIT_FOREVER) != kNoErr) {
            LOGE("Push exit message failed\r\n");
        }
        rtos_thread_join(&g_hw_encoder_ctlr->task);
        g_hw_encoder_ctlr->task = NULL;
    }

    hw_encoder_ctlr_flush_queue();
    rtos_deinit_queue(&g_hw_encoder_ctlr->msg_queue);

    rtos_deinit_mutex(&g_hw_encoder_ctlr->mutex);

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

    /* Lazily create controller on first register */
    if (!g_hw_encoder_ctlr) {
        ret = hw_encoder_ctlr_create();
        if (ret != AVDK_ERR_OK) {
            return ret;
        }
    }

    rtos_lock_mutex(&g_hw_encoder_ctlr->mutex);

    /* Reject duplicate registration */
    node = g_hw_encoder_ctlr->encoder_list;
    while (node) {
        if (node->encoder_id == encoder_id) {
            LOGW("Encoder already registered\r\n");
            rtos_unlock_mutex(&g_hw_encoder_ctlr->mutex);
            return AVDK_ERR_OK;
        }
        node = node->next;
    }

    /* Prepend new list node */
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

    /* First client: power up VCENC and register allocators/ISR */
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

    /* Unlink matching node */
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

            /* Last client: power down and destroy controller */
            if (g_hw_encoder_ctlr->encoder_count == 0 && g_hw_encoder_ctlr->hw_initialized) {
                ret = hw_encoder_hw_deinit();
                g_hw_encoder_ctlr->hw_initialized = false;

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
	if (!msg || !g_hw_encoder_ctlr)
		return AVDK_ERR_INVAL;

	if (rtos_push_to_queue(&g_hw_encoder_ctlr->msg_queue, msg, timeout) != kNoErr) {
		LOGE("Push message to queue failed\r\n");
		return AVDK_ERR_GENERIC;
	}
	return AVDK_ERR_OK;
}
