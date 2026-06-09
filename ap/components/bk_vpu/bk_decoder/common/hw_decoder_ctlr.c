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

#include "modules/vcdec/vcdec_jpeg_api.h"
#include "modules/vcdec/vcdec_h264_api.h"
#include "modules/vcdec/vcdec_common.h"
#include "driver/int.h"
#include "driver/int_types.h"
#include "sys_driver.h"

#include "hw_decoder_ctlr.h"

#define TAG "hw_dec_ctlr"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#if CONFIG_BK_DECODER
#define HW_DECODER_TASK_PRIO        CONFIG_BK_DECODER_TASK_PRIO
#define HW_DECODER_TASK_STACK_SIZE  CONFIG_BK_DECODER_TASK_SIZE
#else
#define HW_DECODER_TASK_PRIO        6
#define HW_DECODER_TASK_STACK_SIZE  4096
#endif
#define HW_DECODER_QUEUE_SIZE       16

typedef struct decoder_node {
	hw_decoder_type_t type;
	void *decoder_id;
	struct decoder_node *next;
} decoder_node_t;

typedef struct {
	beken_thread_t task;
	beken_queue_t msg_queue;
	beken_mutex_t mutex;

	decoder_node_t *decoder_list;
	uint32_t decoder_count;
	bool hw_initialized;
} hw_decoder_ctlr_t;

static hw_decoder_ctlr_t *g_hw_decoder_ctlr = NULL;

static void decoder_int_register(void)
{
	bk_int_isr_register(INT_SRC_H264D, (int_group_isr_t)&vcdec_isr, NULL);
#if CONFIG_SOC_SMP
	sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_H264D, 1);
#else
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_H264D, 1);
#endif
	bk_int_isr_register(INT_SRC_H264D_PP, (int_group_isr_t)&vcdec_pp_isr, NULL);
#if CONFIG_SOC_SMP
	sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_H264D_PP, 1);
#else
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_H264D_PP, 1);
#endif
}

static void decoder_int_deregister(void)
{
#if CONFIG_SOC_SMP
	sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_H264D, 0);
#else
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_H264D, 0);
#endif
	bk_int_isr_unregister(INT_SRC_H264D);
#if CONFIG_SOC_SMP
	sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_H264D_PP, 0);
#else
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_H264D_PP, 0);
#endif
	bk_int_isr_unregister(INT_SRC_H264D_PP);
}

static avdk_err_t hw_decoder_hw_init(void)
{
	LOGI("Hardware decoder init\r\n");
	decoder_int_register();
	bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_H26D, PM_POWER_MODULE_STATE_ON);
	return AVDK_ERR_OK;
}

static avdk_err_t hw_decoder_hw_deinit(void)
{
	LOGI("Hardware decoder deinit\r\n");
	decoder_int_deregister();
	bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_H26D, PM_POWER_MODULE_STATE_OFF);
	return AVDK_ERR_OK;
}

static void hw_decoder_task(void *arg)
{
	hw_decoder_msg_t msg;
	avdk_err_t ret;

	LOGI("Hardware decoder task started\r\n");

	while (1) {
		if (rtos_pop_from_queue(&g_hw_decoder_ctlr->msg_queue, &msg, BEKEN_WAIT_FOREVER) == kNoErr) {
			if (msg.callback) {
				ret = msg.callback(msg.param);
				if (ret != AVDK_ERR_OK) {
					LOGE("%s %d Callback execution failed: %d\r\n", __func__, __LINE__, ret);
				}
			}
			if (msg.sem) {
				rtos_set_semaphore(msg.sem);
			}
		}
	}
}

static avdk_err_t hw_decoder_ctlr_create(void)
{
	avdk_err_t ret;

	if (g_hw_decoder_ctlr != NULL) {
		return AVDK_ERR_OK;
	}

	g_hw_decoder_ctlr = (hw_decoder_ctlr_t *)os_malloc(sizeof(hw_decoder_ctlr_t));
	if (!g_hw_decoder_ctlr) {
		LOGE("Malloc hw decoder ctlr failed\r\n");
		return AVDK_ERR_NOMEM;
	}

	os_memset(g_hw_decoder_ctlr, 0, sizeof(hw_decoder_ctlr_t));

	ret = rtos_init_mutex(&g_hw_decoder_ctlr->mutex);
	if (ret != kNoErr) {
		LOGE("Create mutex failed\r\n");
		os_free(g_hw_decoder_ctlr);
		g_hw_decoder_ctlr = NULL;
		return AVDK_ERR_NO_RESOURCE;
	}

	ret = rtos_init_queue(&g_hw_decoder_ctlr->msg_queue,
			      "hw_dec_queue",
			      sizeof(hw_decoder_msg_t),
			      HW_DECODER_QUEUE_SIZE);
	if (ret != kNoErr) {
		LOGE("Create queue failed\r\n");
		rtos_deinit_mutex(&g_hw_decoder_ctlr->mutex);
		os_free(g_hw_decoder_ctlr);
		g_hw_decoder_ctlr = NULL;
		return AVDK_ERR_NO_RESOURCE;
	}

	ret = rtos_create_thread(&g_hw_decoder_ctlr->task,
				 HW_DECODER_TASK_PRIO,
				 "hw_decoder",
				 (beken_thread_function_t)hw_decoder_task,
				 HW_DECODER_TASK_STACK_SIZE,
				 NULL);
	if (ret != kNoErr) {
		LOGE("Create task failed\r\n");
		rtos_deinit_queue(&g_hw_decoder_ctlr->msg_queue);
		rtos_deinit_mutex(&g_hw_decoder_ctlr->mutex);
		os_free(g_hw_decoder_ctlr);
		g_hw_decoder_ctlr = NULL;
		return AVDK_ERR_NO_RESOURCE;
	}

	LOGI("Hardware decoder controller created\r\n");
	return AVDK_ERR_OK;
}

static avdk_err_t hw_decoder_ctlr_destroy(void)
{
	if (!g_hw_decoder_ctlr) {
		return AVDK_ERR_OK;
	}

	if (g_hw_decoder_ctlr->task) {
		rtos_delete_thread(&g_hw_decoder_ctlr->task);
		g_hw_decoder_ctlr->task = NULL;
	}

	rtos_deinit_queue(&g_hw_decoder_ctlr->msg_queue);
	rtos_deinit_mutex(&g_hw_decoder_ctlr->mutex);
	os_free(g_hw_decoder_ctlr);
	g_hw_decoder_ctlr = NULL;

	LOGI("Hardware decoder controller destroyed\r\n");
	return AVDK_ERR_OK;
}

avdk_err_t hw_decoder_register(hw_decoder_type_t type, void *decoder_id)
{
	decoder_node_t *node = NULL;
	avdk_err_t ret = AVDK_ERR_OK;

	if (!decoder_id) {
		LOGE("Invalid decoder_id\r\n");
		return AVDK_ERR_INVAL;
	}

	if (!g_hw_decoder_ctlr) {
		ret = hw_decoder_ctlr_create();
		if (ret != AVDK_ERR_OK) {
			return ret;
		}
	}

	rtos_lock_mutex(&g_hw_decoder_ctlr->mutex);

	node = g_hw_decoder_ctlr->decoder_list;
	while (node) {
		if (node->decoder_id == decoder_id) {
			LOGW("Decoder already registered\r\n");
			rtos_unlock_mutex(&g_hw_decoder_ctlr->mutex);
			return AVDK_ERR_OK;
		}
		node = node->next;
	}

	node = (decoder_node_t *)os_malloc(sizeof(decoder_node_t));
	if (!node) {
		LOGE("Malloc decoder node failed\r\n");
		rtos_unlock_mutex(&g_hw_decoder_ctlr->mutex);
		return AVDK_ERR_NOMEM;
	}

	node->type = type;
	node->decoder_id = decoder_id;
	node->next = g_hw_decoder_ctlr->decoder_list;
	g_hw_decoder_ctlr->decoder_list = node;
	g_hw_decoder_ctlr->decoder_count++;

	if (g_hw_decoder_ctlr->decoder_count == 1 && !g_hw_decoder_ctlr->hw_initialized) {
		ret = hw_decoder_hw_init();
		if (ret == AVDK_ERR_OK) {
			g_hw_decoder_ctlr->hw_initialized = true;
		}
	}

	LOGI("Decoder registered, type=%d, count=%d\r\n", type, g_hw_decoder_ctlr->decoder_count);
	rtos_unlock_mutex(&g_hw_decoder_ctlr->mutex);
	return ret;
}

avdk_err_t hw_decoder_unregister(void *decoder_id)
{
	decoder_node_t *node = NULL;
	decoder_node_t *prev = NULL;
	avdk_err_t ret = AVDK_ERR_OK;

	if (!decoder_id || !g_hw_decoder_ctlr) {
		return AVDK_ERR_INVAL;
	}

	rtos_lock_mutex(&g_hw_decoder_ctlr->mutex);

	node = g_hw_decoder_ctlr->decoder_list;
	while (node) {
		if (node->decoder_id == decoder_id) {
			if (prev) {
				prev->next = node->next;
			} else {
				g_hw_decoder_ctlr->decoder_list = node->next;
			}
			os_free(node);
			g_hw_decoder_ctlr->decoder_count--;

			LOGI("Decoder unregistered, count=%d\r\n", g_hw_decoder_ctlr->decoder_count);

			if (g_hw_decoder_ctlr->decoder_count == 0 && g_hw_decoder_ctlr->hw_initialized) {
				ret = hw_decoder_hw_deinit();
				g_hw_decoder_ctlr->hw_initialized = false;
				rtos_unlock_mutex(&g_hw_decoder_ctlr->mutex);
				hw_decoder_ctlr_destroy();
				return ret;
			}
			break;
		}
		prev = node;
		node = node->next;
	}

	rtos_unlock_mutex(&g_hw_decoder_ctlr->mutex);
	return ret;
}

avdk_err_t hw_decoder_send_msg(hw_decoder_msg_t *msg, uintptr_t timeout)
{
	if (!msg || !g_hw_decoder_ctlr) {
		return AVDK_ERR_INVAL;
	}

	if (rtos_push_to_queue(&g_hw_decoder_ctlr->msg_queue, msg, timeout) != kNoErr) {
		LOGE("Push message to queue failed\r\n");
		return AVDK_ERR_GENERIC;
	}
	return AVDK_ERR_OK;
}
