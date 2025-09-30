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
#include <stdint.h>
#include <os/os.h>
#include <string.h>
#include <os/mem.h>
#include <components/avdk_utils/avdk_error.h>
#include <driver/dma2d.h>
#include "bk_dma2d_ctlr.h"
#ifdef CONFIG_FREERTOS_SMP
#include "spinlock.h"
#endif

#define TAG "dma2d_ctlr"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)


#ifdef CONFIG_FREERTOS_SMP
static SPINLOCK_SECTION volatile spinlock_t dma2d_spin_lock = SPIN_LOCK_INIT;
#endif
extern uint32_t  platform_is_in_interrupt_context(void);
// Add global static variables at the top of the file to implement singleton and reference counting
static private_dma2d_ctlr_t *g_dma2d_controller = NULL;
static uint32_t g_dma2d_ref_count = 0;


static inline uint32_t dma2d_enter_critical()
{
    uint32_t flags = rtos_disable_int();

#ifdef CONFIG_FREERTOS_SMP
   spin_lock(&dma2d_spin_lock);
#endif // CONFIG_FREERTOS_SMP

   return flags;
}

static inline void dma2d_exit_critical(uint32_t flags)
{
#ifdef CONFIG_FREERTOS_SMP
   spin_unlock(&dma2d_spin_lock);
#endif // CONFIG_FREERTOS_SMP

   rtos_enable_int(flags);
}

// Private function declarations
static avdk_err_t dma2d_ctlr_fill_sync(bk_dma2d_ctlr_t *controller, dma2d_fill_config_t *config);
static avdk_err_t dma2d_ctlr_blend_sync(bk_dma2d_ctlr_t *controller, dma2d_blend_config_t *config);
static avdk_err_t dma2d_ctlr_memcpy_sync(bk_dma2d_ctlr_t *controller, dma2d_memcpy_config_t *config);
static avdk_err_t dma2d_ctlr_pixel_conversion_sync(bk_dma2d_ctlr_t *controller, dma2d_pfc_memcpy_config_t *config);

static void dma2d_ctlr_trans_complete_isr(void *args)
{
    if (!args) {
        LOGE("ISR called with NULL args\n");
        return;
    }
    private_dma2d_ctlr_t *controller = (private_dma2d_ctlr_t *)args;
    dma2d_msg_type_t current_op = controller->context.current_operation;
    dma2d_trans_status_t status = DMA2D_TRANSFER_COMPLETE;
    // only call the callback function of the current operation type
    switch (current_op) {
        case DMA2D_FILL_REQUEST:
            if (controller->fill_config.transfer_complete_cb) {
                controller->fill_config.transfer_complete_cb(status, controller->fill_config.user_data);
            }
            break;
        case DMA2D_BLEND_REQUEST:
            if (controller->blend_config.transfer_complete_cb) {
                controller->blend_config.transfer_complete_cb(status, controller->blend_config.user_data);
            }
            break;
        case DMA2D_MEMCPY_REQUEST:
            if (controller->memcpy_config.transfer_complete_cb) {
                controller->memcpy_config.transfer_complete_cb(status, controller->memcpy_config.user_data);
            }
            break;
        case DMA2D_PFC_MEMCPY_REQUEST:
            if (controller->pfc_memcpy_config.transfer_complete_cb) {
                controller->pfc_memcpy_config.transfer_complete_cb(status, controller->pfc_memcpy_config.user_data);
            }
            break;
        default:
            LOGD("dma2d_ctlr_trans_complete_isr: unknown operation type\n");
            break;
    }
    rtos_set_semaphore(&controller->context.dma2d_sem);

    // clear current operation type
    controller->context.current_operation = (dma2d_msg_type_t)0;
}

static void dma2d_ctlr_trans_error_isr(void *args)
{
    dma2d_trans_status_t status = DMA2D_TRANSFER_ERROR; // transfer error status
    LOGD("dma2d_ctlr_trans_error_isr \n");
    private_dma2d_ctlr_t *controller = (private_dma2d_ctlr_t *)args;
    dma2d_msg_type_t current_op = controller->context.current_operation;
    // only call the callback function of the current operation type
    switch (current_op) {
        case DMA2D_FILL_REQUEST:
            if (controller->fill_config.transfer_complete_cb) {
                controller->fill_config.transfer_complete_cb(status, controller->fill_config.user_data);
            }
            break;
        case DMA2D_BLEND_REQUEST:
            if (controller->blend_config.transfer_complete_cb) {
                controller->blend_config.transfer_complete_cb(status, controller->blend_config.user_data);
            }
            break;
        case DMA2D_MEMCPY_REQUEST:
            if (controller->memcpy_config.transfer_complete_cb) {
                controller->memcpy_config.transfer_complete_cb(status, controller->memcpy_config.user_data);
            }
            break;
        case DMA2D_PFC_MEMCPY_REQUEST:
            if (controller->pfc_memcpy_config.transfer_complete_cb) {
                controller->pfc_memcpy_config.transfer_complete_cb(status, controller->pfc_memcpy_config.user_data);
            }
            break;
        default:
            LOGD("dma2d_ctlr_trans_error_isr: unknown operation type\n");
            break;
    }
    rtos_set_semaphore(&controller->context.dma2d_sem);
    controller->context.current_operation = (dma2d_msg_type_t)0;
}

static void dma2d_ctlr_config_error(void *args)
{
    LOGD("dma2d_ctlr_config_error \n");
    dma2d_ctlr_trans_error_isr(args);
}

static bk_err_t dma2d_ctlr_task_send_msg(dma2d_ctlr_context_t *context, uint8_t type, uint32_t param, uint32_t param2)
{
    int ret = BK_FAIL;
    dma2d_msg_t msg;
    uint32_t isr_context = platform_is_in_interrupt_context();

    if (context && context->task_running)
    {
        msg.event = type;
        msg.param = param;
        msg.param2 = param2;

        if (!isr_context)
        {
            rtos_lock_mutex(&context->lock);
        }

        if (context->task_running)
        {
            ret = rtos_push_to_queue(&context->queue, &msg, BEKEN_WAIT_FOREVER);

            if (ret != AVDK_ERR_OK)
            {
                LOGE("%s push failed\n", __func__);
            }
        }

        if (!isr_context)
        {
            rtos_unlock_mutex(&context->lock);
        }
    }

    return ret;
}


static void dma2d_ctlr_task_entry(beken_thread_arg_t arg)
{
    dma2d_ctlr_context_t *context = (dma2d_ctlr_context_t *)arg;
    context->task_running = true;
    rtos_set_semaphore(&context->sem);

    while (context->task_running)
    {
        dma2d_msg_t msg;
        int ret = rtos_pop_from_queue(&context->queue, &msg, BEKEN_WAIT_FOREVER);
        if (ret == AVDK_ERR_OK)
        {
            switch (msg.event)
            {
                case DMA2D_FILL_REQUEST:
                {
                    private_dma2d_ctlr_t *dma2d_ctlr = (private_dma2d_ctlr_t *)msg.param;
                    dma2d_ctlr_fill_sync((bk_dma2d_ctlr_t *)&dma2d_ctlr->ops, &dma2d_ctlr->fill_config);
                    break;
                }

                case DMA2D_MEMCPY_REQUEST:
                {
                    private_dma2d_ctlr_t *dma2d_ctlr = (private_dma2d_ctlr_t *)msg.param;
                    dma2d_ctlr_memcpy_sync((bk_dma2d_ctlr_t *)&dma2d_ctlr->ops, (dma2d_memcpy_config_t *)&dma2d_ctlr->memcpy_config);
                    break;
                }

                case DMA2D_PFC_MEMCPY_REQUEST:
                {
                    private_dma2d_ctlr_t *dma2d_ctlr = (private_dma2d_ctlr_t *)msg.param;
                    dma2d_ctlr_pixel_conversion_sync((bk_dma2d_ctlr_t *)&dma2d_ctlr->ops, (dma2d_pfc_memcpy_config_t *)&dma2d_ctlr->pfc_memcpy_config);;
                }
                break;

                case DMA2D_BLEND_REQUEST:
                {
                    private_dma2d_ctlr_t *dma2d_ctlr = (private_dma2d_ctlr_t *)msg.param;
                    dma2d_ctlr_blend_sync((bk_dma2d_ctlr_t *)&dma2d_ctlr->ops, (dma2d_blend_config_t *)&dma2d_ctlr->blend_config);
                }
                break;

                case DMA2D_EXIT:
                {
                    rtos_lock_mutex(&context->lock);
                    context->task_running = false;
                    rtos_unlock_mutex(&context->lock);
                }
                goto exit;
            }
        }
    }
exit:
    context->task = NULL;
    rtos_set_semaphore(&context->sem);
    rtos_delete_thread(NULL);
}

static bk_err_t dma2d_ctlr_task_start(dma2d_ctlr_context_t *context)
{
    int ret = AVDK_ERR_OK;

    ret = rtos_init_queue(&context->queue,
                          "dma2d_queue",
                          sizeof(dma2d_msg_t),
                          6);
    AVDK_RETURN_ON_ERROR(ret, TAG, "dma2d_queue init failed \n");

    ret = rtos_create_thread(&context->task,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "dma2d_task",
                             (beken_thread_function_t)dma2d_ctlr_task_entry,
                             512,
                             (beken_thread_arg_t)context);
    AVDK_RETURN_ON_ERROR(ret, TAG, "dma2d_task init failed \n");

    ret = rtos_get_semaphore(&context->sem, BEKEN_NEVER_TIMEOUT);
    AVDK_RETURN_ON_ERROR(ret, TAG, "sem get failed \n");

    return ret;
}


static avdk_err_t dma2d_ctlr_open(bk_dma2d_ctlr_t *controller)
{
    private_dma2d_ctlr_t *dma2d_ctlr = __containerof(controller, private_dma2d_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(dma2d_ctlr, AVDK_ERR_INVAL, TAG, "control is NULL \n");

    dma2d_ctlr_context_t *context = &dma2d_ctlr->context;

    // init dma2d driver
    avdk_err_t ret = bk_dma2d_driver_init();
    AVDK_RETURN_ON_ERROR(ret, TAG, "dma2d driver init failed \n");

    bk_dma2d_int_enable(DMA2D_CFG_ERROR | DMA2D_TRANS_ERROR | DMA2D_TRANS_COMPLETE ,1); 
    bk_dma2d_register_int_callback_isr(DMA2D_TRANS_COMPLETE_ISR, dma2d_ctlr_trans_complete_isr, dma2d_ctlr);
    bk_dma2d_register_int_callback_isr(DMA2D_TRANS_ERROR_ISR, dma2d_ctlr_trans_error_isr, dma2d_ctlr);
    bk_dma2d_register_int_callback_isr(DMA2D_CFG_ERROR_ISR, dma2d_ctlr_config_error, dma2d_ctlr);

    ret = rtos_init_semaphore(&context->sem, 1);
    AVDK_RETURN_ON_ERROR(ret, TAG, "disp_sem init failed \n");

    ret = rtos_init_semaphore(&context->dma2d_sem, 1);
    AVDK_RETURN_ON_ERROR(ret, TAG, "disp_sem init failed \n");

    ret = rtos_init_mutex(&context->lock);
    AVDK_RETURN_ON_ERROR(ret, TAG, "lock init failed \n");

    ret = dma2d_ctlr_task_start(context);
    AVDK_RETURN_ON_ERROR(ret, TAG, "dma2d ctlr task init failed \n");
    return ret;
}


static bk_err_t dma2d_ctlr_task_stop(dma2d_ctlr_context_t *context)
{
    bk_err_t ret = AVDK_ERR_OK;
    
    if (!context || context->task_running == false)
    {
        LOGD("%s already stop\n", __func__);
        return ret;
    }

   ret = dma2d_ctlr_task_send_msg(context, DMA2D_EXIT, 0, 0);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s send exit message failed: %d\n", __func__, ret);
    }
    ret = rtos_get_semaphore(&context->sem, BEKEN_NEVER_TIMEOUT);
    if (AVDK_ERR_OK != ret)
    {
        LOGE("%s dma2d_ctlr_sem get failed\n", __func__);
    }
    if (context->queue)
    {
        rtos_deinit_queue(&context->queue);
        context->queue = NULL;
    }
    LOGD("%s complete\n", __func__);
    return ret;
}


/**
 * @brief Close DMA2D controller
 * @param controller DMA2D controller handle
 * @return Operation result, AVDK_ERR_OK indicates success
 */
static avdk_err_t dma2d_ctlr_close(bk_dma2d_ctlr_t *controller)
{
    private_dma2d_ctlr_t *dma2d_ctlr = __containerof(controller, private_dma2d_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(dma2d_ctlr, AVDK_ERR_INVAL, TAG, "control is NULL \n");
    
    // enter critical section
    uint32_t flags = dma2d_enter_critical();
    
    // check if it is the last user
    bool is_last_user = (g_dma2d_ref_count == 1);
    if (is_last_user) {
        LOGD(TAG, "dma2d controller is last user, ref count is %d \n", g_dma2d_ref_count);
    }
    
    // exit critical section
    dma2d_exit_critical(flags);
    
    // only close the dma2d controller if it is the last user
    if (is_last_user) {
        dma2d_ctlr_task_stop(&dma2d_ctlr->context);
        
        // deinit dma2d driver
        bk_dma2d_driver_deinit();
        bk_dma2d_register_int_callback_isr(DMA2D_TRANS_COMPLETE_ISR, NULL, NULL);
        bk_dma2d_register_int_callback_isr(DMA2D_TRANS_ERROR_ISR, NULL, NULL);
        bk_dma2d_register_int_callback_isr(DMA2D_CFG_ERROR_ISR, NULL, NULL);
        
        if (dma2d_ctlr->context.lock) {
            rtos_deinit_mutex(&dma2d_ctlr->context.lock);
            dma2d_ctlr->context.lock = NULL;
        }
        if (dma2d_ctlr->context.sem) {
            rtos_deinit_semaphore(&dma2d_ctlr->context.sem);
            dma2d_ctlr->context.sem = NULL;
        }
        if (dma2d_ctlr->context.dma2d_sem) {
            rtos_deinit_semaphore(&dma2d_ctlr->context.dma2d_sem);
            dma2d_ctlr->context.dma2d_sem = NULL;
        }
        
        LOGD(TAG, "dma2d controller closed completely \n");
    } else {
        LOGD(TAG, "dma2d controller partially closed, still in use by other modules \n");
    }
    
    return AVDK_ERR_OK;
}

/**
 * @brief Fill operation implementation
 * @param controller DMA2D controller handle
 * @param config Fill configuration
 * @return Operation result, AVDK_ERR_OK indicates success
 */
static avdk_err_t dma2d_ctlr_fill_sync(bk_dma2d_ctlr_t *controller, dma2d_fill_config_t *config)
{
    private_dma2d_ctlr_t *dma2d_ctlr = __containerof(controller, private_dma2d_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(dma2d_ctlr, AVDK_ERR_INVAL, TAG, "control is NULL \n");
    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, "config is NULL \n");

    //LOCK
    rtos_lock_mutex(&dma2d_ctlr->context.lock);
    // set current operation type to fill operation
    dma2d_ctlr->context.current_operation = DMA2D_FILL_REQUEST;

    dma2d_ctlr->fill_config = *config;

    // Execute fill operation
    avdk_err_t ret = dma2d_fill(&dma2d_ctlr->fill_config.fill);
    if (ret != AVDK_ERR_OK) {
        LOGE("dma2d fill failed\n");
        rtos_unlock_mutex(&dma2d_ctlr->context.lock);
        return AVDK_ERR_HWERROR;
    }

    ret = bk_dma2d_start_transfer();
    if (ret != AVDK_ERR_OK) {
        LOGE("dma2d start transfer failed\n");
        rtos_unlock_mutex(&dma2d_ctlr->context.lock);
        return AVDK_ERR_HWERROR;
    }

    // get semaphore to wait for transfer completion
    ret = rtos_get_semaphore(&dma2d_ctlr->context.dma2d_sem, BEKEN_NEVER_TIMEOUT);

    if (ret != AVDK_ERR_OK) {
        LOGE( "dma2d sem get failed\n");
        rtos_unlock_mutex(&dma2d_ctlr->context.lock);
        return AVDK_ERR_HWERROR;
    }

    //UNLOCK
    rtos_unlock_mutex(&dma2d_ctlr->context.lock);
    return AVDK_ERR_OK;
}

static avdk_err_t dma2d_ctlr_fill(bk_dma2d_ctlr_t *controller, dma2d_fill_config_t *config)
{
    private_dma2d_ctlr_t *dma2d_ctlr = __containerof(controller, private_dma2d_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(dma2d_ctlr, AVDK_ERR_INVAL, TAG, "control is NULL \n");
    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, "config is NULL \n");

    if (config->is_sync)
    {
        return dma2d_ctlr_fill_sync(controller, config);
    }
    else
    {
        dma2d_ctlr->fill_config = *config;
        dma2d_ctlr_task_send_msg(&dma2d_ctlr->context, DMA2D_FILL_REQUEST, (uint32_t)dma2d_ctlr, 0);
    }
    return AVDK_ERR_OK;
}

static avdk_err_t dma2d_ctlr_memcpy_sync(bk_dma2d_ctlr_t *controller, dma2d_memcpy_config_t *config)
{
    private_dma2d_ctlr_t *dma2d_ctlr = __containerof(controller, private_dma2d_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(dma2d_ctlr, AVDK_ERR_INVAL, TAG, "control is NULL \n");
    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, "config is NULL \n");

    // lock mutex to protect configuration process
    rtos_lock_mutex(&dma2d_ctlr->context.lock);
    // set current operation type to memcpy operation
    dma2d_ctlr->context.current_operation = DMA2D_MEMCPY_REQUEST;

    dma2d_ctlr->memcpy_config = *config;
    dma2d_ctlr->memcpy_config.memcpy.mode = DMA2D_M2M;

    // execute memcpy operation
    bk_dma2d_memcpy_or_pixel_convert(&dma2d_ctlr->memcpy_config.memcpy);

    avdk_err_t ret = bk_dma2d_start_transfer();
    if (ret != AVDK_ERR_OK) {
        LOGE( "dma2d start transfer failed\n");
        rtos_unlock_mutex(&dma2d_ctlr->context.lock);
        return AVDK_ERR_HWERROR;
    }

    // get semaphore to wait for transfer completion
    ret = rtos_get_semaphore(&dma2d_ctlr->context.dma2d_sem, BEKEN_NEVER_TIMEOUT);
    if (ret != AVDK_ERR_OK) {
        LOGE( "dma2d semaphore get failed: %d\n", ret);
        rtos_unlock_mutex(&dma2d_ctlr->context.lock);
        return AVDK_ERR_HWERROR;
    }

    rtos_unlock_mutex(&dma2d_ctlr->context.lock);
    return AVDK_ERR_OK;
}
/**
 * @brief Memcpy operation implementation
 * @param controller DMA2D controller handle
 * @param config Memcpy configuration
 * @return Operation result, AVDK_ERR_OK indicates success
 */
static avdk_err_t dma2d_ctlr_memcpy(bk_dma2d_ctlr_t *controller, dma2d_memcpy_config_t *config)
{
    private_dma2d_ctlr_t *dma2d_ctlr = __containerof(controller, private_dma2d_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(dma2d_ctlr, AVDK_ERR_INVAL, TAG, "control is NULL \n");
    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, "config is NULL \n");

   if (config->is_sync)
    {
        return dma2d_ctlr_memcpy_sync(controller, config);
    }
    else
    {
        dma2d_ctlr->memcpy_config = *config;
        return dma2d_ctlr_task_send_msg(&dma2d_ctlr->context, DMA2D_MEMCPY_REQUEST, (uint32_t)dma2d_ctlr, 0);
    }
}


/**
 * @brief Pixel conversion operation implementation
 * @param controller DMA2D controller handle
 * @param config Pixel conversion configuration
 * @return Operation result, AVDK_ERR_OK indicates success
 */
static avdk_err_t dma2d_ctlr_pixel_conversion_sync(bk_dma2d_ctlr_t *controller, dma2d_pfc_memcpy_config_t *config)
{
    private_dma2d_ctlr_t *dma2d_ctlr = __containerof(controller, private_dma2d_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(dma2d_ctlr, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, "config is NULL");

    // set current operation type to pixel conversion operation
    dma2d_ctlr->context.current_operation = DMA2D_PFC_MEMCPY_REQUEST;
    // lock mutex to protect configuration process
    rtos_lock_mutex(&dma2d_ctlr->context.lock);

    dma2d_ctlr->pfc_memcpy_config = *config;
    dma2d_ctlr->pfc_memcpy_config.pfc.mode = DMA2D_M2M_PFC; 
    bk_dma2d_memcpy_or_pixel_convert(&dma2d_ctlr->pfc_memcpy_config.pfc);

    avdk_err_t ret = bk_dma2d_start_transfer();
    if (ret != AVDK_ERR_OK) {
        LOGE( "dma2d start transfer failed\n");
        rtos_unlock_mutex(&dma2d_ctlr->context.lock);
        return AVDK_ERR_HWERROR;
    }

    // get semaphore to wait for transfer completion
    ret = rtos_get_semaphore(&dma2d_ctlr->context.dma2d_sem, BEKEN_NEVER_TIMEOUT);
    if (ret != AVDK_ERR_OK) {
        LOGE( "dma2d semaphore get failed: %d\n", ret);
        rtos_unlock_mutex(&dma2d_ctlr->context.lock);
        return AVDK_ERR_HWERROR;
    }

    rtos_unlock_mutex(&dma2d_ctlr->context.lock);
    return AVDK_ERR_OK;
}
/**
 * @brief Pixel conversion operation implementation
 * @param controller DMA2D controller handle
 * @param config Pixel conversion configuration
 * @return Operation result, AVDK_ERR_OK indicates success
 */
static avdk_err_t dma2d_ctlr_pixel_conversion(bk_dma2d_ctlr_t *controller, dma2d_pfc_memcpy_config_t *config)
{
    private_dma2d_ctlr_t *dma2d_ctlr = __containerof(controller, private_dma2d_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(dma2d_ctlr, AVDK_ERR_INVAL, TAG, "control is NULL \n");

    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, "config is NULL \n");
    //AVDK_RETURN_ON_FALSE(config->pfc_memcpy, AVDK_ERR_INVAL, TAG, "pfc_memcpy is NULL");
    //AVDK_RETURN_ON_FALSE(config->callbacks, AVDK_ERR_INVAL, TAG, "callbacks is NULL");

   if (config->is_sync)
    {
        return dma2d_ctlr_pixel_conversion_sync(controller, config);
    }
    else
    {
        dma2d_ctlr->pfc_memcpy_config = *config;
        return dma2d_ctlr_task_send_msg(&dma2d_ctlr->context, DMA2D_PFC_MEMCPY_REQUEST,  (uint32_t)dma2d_ctlr, 0);
    }
}

static avdk_err_t dma2d_ctlr_blend_sync(bk_dma2d_ctlr_t *controller, dma2d_blend_config_t *config)
{
    private_dma2d_ctlr_t *dma2d_ctlr = __containerof(controller, private_dma2d_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(dma2d_ctlr, AVDK_ERR_INVAL, TAG, "control is NULL \n");

    // set current operation type to blend operation
    dma2d_ctlr->context.current_operation = DMA2D_BLEND_REQUEST;
    // lock mutex to protect configuration process
    rtos_lock_mutex(&dma2d_ctlr->context.lock);
    dma2d_ctlr->blend_config = *config;

    // Execute blend operation
    avdk_err_t ret = bk_dma2d_offset_blend(&dma2d_ctlr->blend_config.blend);
    if (ret != AVDK_ERR_OK) {
        LOGE( "dma2d blend failed \n");
        rtos_unlock_mutex(&dma2d_ctlr->context.lock);
        return AVDK_ERR_HWERROR;
    }
    ret = bk_dma2d_start_transfer();
    if (ret != AVDK_ERR_OK) {
        LOGE( "dma2d start transfer failed\n");
        rtos_unlock_mutex(&dma2d_ctlr->context.lock);
        return AVDK_ERR_HWERROR;
    }

    // get semaphore to wait for transfer completion
    ret = rtos_get_semaphore(&dma2d_ctlr->context.dma2d_sem, BEKEN_NEVER_TIMEOUT);
    if (ret != AVDK_ERR_OK) {
        LOGE( "dma2d semaphore get failed: %d\n", ret);
        rtos_unlock_mutex(&dma2d_ctlr->context.lock);
        return AVDK_ERR_HWERROR;
    }
    rtos_unlock_mutex(&dma2d_ctlr->context.lock);
    return AVDK_ERR_OK;
}
/**
 * @brief Blend operation implementation
 * @param controller DMA2D controller handle
 * @param config Blend configuration
 * @return Operation result, AVDK_ERR_OK indicates success
 */
static avdk_err_t dma2d_ctlr_blend(bk_dma2d_ctlr_t *controller, dma2d_blend_config_t *config)
{
    private_dma2d_ctlr_t *dma2d_ctlr = __containerof(controller, private_dma2d_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(dma2d_ctlr, AVDK_ERR_INVAL, TAG, "control is NULL \n");
    //AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, "config is NULL");
    //AVDK_RETURN_ON_FALSE(config->blend, AVDK_ERR_INVAL, TAG, "blend is NULL");
    if (config->is_sync)
    {
        return dma2d_ctlr_blend_sync(controller, config);
    }
    else
    {
        dma2d_ctlr->blend_config = *config;
        return dma2d_ctlr_task_send_msg(&dma2d_ctlr->context, DMA2D_BLEND_REQUEST,  (uint32_t)dma2d_ctlr, 0);
    }
}

/**
 * @brief Delete DMA2D controller
 * @param controller DMA2D controller handle
 * @return Operation result, AVDK_ERR_OK indicates success
 */
static avdk_err_t dma2d_ctlr_delete(bk_dma2d_ctlr_t *controller)
{
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL \n");

    uint32_t flags = dma2d_enter_critical();

    // make sure the controller is the singleton instance
    if (controller != &(g_dma2d_controller->ops)) {
        dma2d_exit_critical(flags);
        return AVDK_ERR_INVAL;
    }

    // decrease reference count
    if (g_dma2d_ref_count > 0) {
        g_dma2d_ref_count--;
        LOGD(TAG, "dma2d controller ref count decreased to %d \n", g_dma2d_ref_count);
    }

    // only release memory when reference count is 0
    if (g_dma2d_ref_count == 0) {
        // release memory
        os_free(g_dma2d_controller);
        g_dma2d_controller = NULL;
        LOGD(TAG, "delete dma2d controller success \n");
    }

    // exit critical section
    dma2d_exit_critical(flags);

    return AVDK_ERR_OK;
}

/**
 * @brief IO control operation implementation
 * @param controller DMA2D controller handle
 * @param ioctl_cmd IO control command
 * @param param1 Command parameter 1
 * @param param2 Command parameter 2
 * @param param3 Command parameter 3
 * @return Operation result, AVDK_ERR_OK indicates success
 */
static avdk_err_t dma2d_ctlr_ioctl(bk_dma2d_ctlr_t *controller, uint32_t ioctl_cmd, uint32_t param1, uint32_t param2, uint32_t param3)
{
    private_dma2d_ctlr_t *dma2d_ctlr = __containerof(controller, private_dma2d_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(dma2d_ctlr, AVDK_ERR_INVAL, TAG, "control is NULL");

    // handle ioctl command
    switch (ioctl_cmd) {
        case DMA2D_IOCTL_SET_SWRESRT:
            // reset dma2d controller
            bk_dma2d_soft_reset();
            break;
        case DMA2D_IOCTL_SUSPEND:
            // suspend dma2d controller
            bk_driver_dma2d_suspend(1);
            break;
        case DMA2D_IOCTL_RESUME:
            // resume dma2d controller
            bk_driver_dma2d_suspend(0);
            break;
        case DMA2D_IOCTL_TRANS_ABORT:
            // abort dma2d transfer
            bk_driver_dma2d_trans_abort();
            break;
        default:
            LOGE( "unsupported ioctl cmd: %d \n", ioctl_cmd);
            return AVDK_ERR_UNSUPPORTED;
    }
    return AVDK_ERR_OK;
}


/**
 * @brief Create DMA2D controller
 * @param handle Output parameter, used to store the created DMA2D controller handle
 * @return Operation result, AVDK_ERR_OK indicates success
 */
avdk_err_t bk_dma2d_ctlr_new(bk_dma2d_ctlr_handle_t *handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    
    // create dma2d controller
    uint32_t flags = dma2d_enter_critical();
    
    if (g_dma2d_controller == NULL) {
        // create dma2d controller
        private_dma2d_ctlr_t *controller = os_malloc(sizeof(private_dma2d_ctlr_t));
        AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
        os_memset(controller, 0, sizeof(private_dma2d_ctlr_t));

        // set operation functions
        controller->ops.open = dma2d_ctlr_open;
        controller->ops.fill = dma2d_ctlr_fill;
        controller->ops.memcpy = dma2d_ctlr_memcpy;
        controller->ops.pfc_memcpy = dma2d_ctlr_pixel_conversion;
        controller->ops.blend = dma2d_ctlr_blend;
        controller->ops.delete = dma2d_ctlr_delete;
        controller->ops.ioctl = dma2d_ctlr_ioctl;
        controller->ops.close = dma2d_ctlr_close;
        
        g_dma2d_controller = controller;
        LOGI(TAG, "create dma2d controller success \n");
    }

    // increase reference count
    g_dma2d_ref_count++;
    LOGD(TAG, "dma2d controller ref count increased to %d\n", g_dma2d_ref_count);

    // return created controller handle
    *handle = &(g_dma2d_controller->ops);

    // exit critical section
    dma2d_exit_critical(flags);

    return AVDK_ERR_OK;
}

bk_dma2d_ctlr_handle_t bk_dma2d_ctlr_get(void)
{
    if (g_dma2d_controller)
        return &(g_dma2d_controller->ops);
    else
        return NULL;
}