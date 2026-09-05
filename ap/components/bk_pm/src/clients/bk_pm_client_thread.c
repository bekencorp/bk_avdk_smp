#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include "driver/pm_ap_core.h"
#include <os/mem.h>
#include "FreeRTOS.h"
#if CONFIG_PM_AP_FAST_BOOT_ENABLE && CONFIG_CPU_HOTPLUG
#include "multicore_driver.h"
#include "sys_types.h"
#include <driver/aon_rtc.h>
#include "sys_sw_regs.h"
#include "cache.h"
#include "bk_arch.h"
#include <driver/dma.h>
#endif

/*=====================DEFINE  SECTION  START=====================*/

#define TAG "pwr_core"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define PM_AP_CORE_STACK_SIZE              (1536)
#define PM_AP_CORE_QUEUE_NUMBER_OF_MESSAGE (30)
#define PM_AP_CORE_THREAD_PRIORITY         (2)

/*=====================DEFINE  SECTION  END=====================*/


/*=====================STRUCT AND ENUM  SECTION  START==========*/
typedef struct
{
    beken_thread_t thd;
    beken_queue_t queue;
}pm_ap_core_info_t;
/*=====================STRUCT AND ENUM  SECTION  END=============*/


/*=====================VARIABLE  SECTION  START==================*/
static pm_ap_core_info_t *s_pm_info = NULL;

/*=====================VARIABLE  SECTION  END===================*/

#if CONFIG_PM_AP_FAST_BOOT_ENABLE && CONFIG_CPU_HOTPLUG
static void pm_ap_suspend_failure_publish(uint32_t recovery_seq)
{
    pm_shared_info_t shared_info = {0};

    /*
     * Publish failure only after rollback has restored AP services. If resume
     * failed, keep CP on the existing timeout path instead of letting it reopen
     * business traffic against a partially restored AP.
     */
    if ((recovery_seq == 0U) || !bk_pm_ap_full_ready_get()) {
        return;
    }

    bk_sys_sw_regs_get_pm_shared_info(&shared_info);
    shared_info.param1 = recovery_seq;
    bk_sys_sw_regs_update_pm_shared_info(&shared_info,
        BK_SYS_SW_REGS_PM_SHARED_INFO_FIELD_PARAM1,
        BK_SYS_SW_REGS_LOCK_DISABLE);
    __DSB();
    flush_dcache((void *)&bk_sys_sw_regs_ptr()->pm_shared_info,
        sizeof(bk_sys_sw_regs_ptr()->pm_shared_info));
    __DSB();
    LOGW("AP fast suspend: rollback ready seq=%u\r\n", recovery_seq);
}
#endif

/*================FUNCTION DECLARATION  SECTION  START==========*/


/*================FUNCTION DECLARATION  SECTION  END===========*/

bk_err_t bk_pm_ap_core_send_msg(pm_ap_core_msg_t *msg)
{
    bk_err_t ret = BK_OK;
	if(msg == NULL || s_pm_info == NULL)
	{
	    LOGE("Pm core send msg error\r\n");
		return BK_FAIL;
	}

    if (s_pm_info->queue)
    {
        ret = rtos_push_to_queue(&s_pm_info->queue, msg, BEKEN_NO_WAIT);

        if (BK_OK != ret)
        {
            LOGE("%s failed[%d]\n", __func__,ret);
            return BK_FAIL;
        }

        return ret;
    }

    return ret;
}

static bk_err_t pm_ap_core_message_handle(void)
{
    bk_err_t ret = BK_OK;
    pm_ap_core_msg_t msg;

    while (1)
    {
        ret = rtos_pop_from_queue(&s_pm_info->queue, &msg, BEKEN_WAIT_FOREVER);
        //LOGD("%s event:%d,param:%d,%d,%d\n", __func__,msg.event,msg.param1,msg.param2,msg.param3);
        if (kNoErr == ret)
        {
            switch (msg.event)
            {
                case PM_AP_CORE_STATE_ENTER_DEEPSLEEP:
                {
                }
                break;
                case PM_AP_CORE_AP_RECOVERY:
                {
#if CONFIG_PM_AP_FAST_BOOT_ENABLE
                    LOGI("AP fast suspend: recovery begin seq=%u\r\n",
                        msg.param1);
#if CONFIG_CPU_HOTPLUG
                    /*
                     * Keep the legacy close callbacks first, then let newly
                     * registered modules quiesce while CPU3 is still online.
                     */
                    bk_pm_ap_full_ready_set(false);
#endif
#endif
                    bk_pm_ap_close_ap_handle_callback();
#if CONFIG_PM_AP_FAST_BOOT_ENABLE
                    LOGI("AP fast suspend: close callbacks ready seq=%u\r\n",
                        msg.param1);
#endif
#if CONFIG_PM_AP_FAST_BOOT_ENABLE && CONFIG_CPU_HOTPLUG
                    ret = bk_pm_ap_fast_suspend_prepare();
                    if (ret != BK_OK) {
                        LOGE("AP fast suspend: module quiesce failed[%d]\r\n", ret);
                        pm_ap_suspend_failure_publish(msg.param1);
                        break;
                    }
                    /*
                     * Modules get the first chance to stop their DMA in
                     * quiesce. Reject the transaction here if any channel is
                     * still enabled, before CPU3 is offlined and hardware is
                     * backed up. This turns an eventual WFI timeout into an
                     * immediate, recoverable suspend failure.
                     */
                    uint32_t dma_busy_mask = bk_dma_check_chn_status();
                    if (dma_busy_mask != 0U) {
                        LOGE("AP fast suspend: DMA still busy mask=0x%x after quiesce\r\n",
                            dma_busy_mask);
                        ret = bk_pm_ap_fast_resume_modules();
                        if (ret != BK_OK) {
                            LOGE("AP fast suspend: DMA busy rollback failed[%d]\r\n",
                                ret);
                        }
                        pm_ap_suspend_failure_publish(msg.param1);
                        break;
                    }
                    /*
                     * Fast resume retains CPU2 only. Run CPU3 hotplug from this
                     * CPU2-pinned PM task before the idle path captures the AP
                     * context. It is unsafe to run the hotplug state machine
                     * from sys_hal_enter_cpu_wfi()'s critical section.
                     */
                    if (bk_cpu_hp_is_online(CPU3_CORE_ID)) {
                        ret = bk_cpu_hp_offline_direct(CPU3_CORE_ID);
                        if (ret != BK_OK) {
                            LOGE("AP fast suspend: CPU3 offline failed[%d]\r\n", ret);
                            (void)bk_pm_ap_fast_resume_modules();
                            pm_ap_suspend_failure_publish(msg.param1);
                            break;
                        } else {
                            LOGI("AP fast suspend: CPU3 offline ready\r\n");
                        }
                    }
                    ret = bk_pm_ap_fast_suspend_backup();
                    if (ret != BK_OK) {
                        LOGE("AP fast suspend: hardware backup failed[%d]\r\n", ret);
                        ret = bk_cpu_hp_online_direct(CPU3_CORE_ID);
                        if (ret == BK_OK) {
                            (void)bk_pm_ap_fast_resume_modules();
                            pm_ap_suspend_failure_publish(msg.param1);
                        } else {
                            LOGE("AP fast suspend rollback: CPU3 online failed[%d]\r\n",
                                ret);
                        }
                        break;
                    }
                    LOGI("AP fast suspend: modules prepared seq=%u\r\n",
                        msg.param1);
#endif
                }
                break;
                case PM_AP_CORE_CPU3_ONLINE:
                {
#if CONFIG_PM_AP_FAST_BOOT_ENABLE && CONFIG_CPU_HOTPLUG
                    ret = bk_pm_ap_fast_restore_hardware();
                    if (ret != BK_OK) {
                        LOGE("AP fast resume: hardware restore failed[%d]\r\n", ret);
                        break;
                    }
                    if (!bk_cpu_hp_is_online(CPU3_CORE_ID)) {
                        ret = bk_cpu_hp_online_direct(CPU3_CORE_ID);
                        if (ret != BK_OK) {
                            LOGE("AP fast resume: CPU3 online failed[%d], stage=%u\r\n",
                                ret, bk_cpu3_fast_resume_stage_get());
                        }
                    }
                    if (!bk_cpu_hp_is_online(CPU3_CORE_ID)) {
                        /*
                         * AP0 already published fast-resume success. Keep CPU3
                         * failure visible without forcing CP into cold fallback.
                         */
                        LOGE("AP fast resume: CPU3 not ready, AP0 remains available\r\n");
                    } else {
                        ret = bk_pm_ap_fast_resume_modules();
                        if (ret != BK_OK) {
                            LOGE("AP fast resume: module resume failed[%d]\r\n", ret);
                        }
                    }
#endif
                }
                break;
#if CONFIG_PM_AP_FAST_BOOT_ENABLE
                case PM_AP_CORE_FAST_SUSPEND_ABORT:
                {
#if CONFIG_CPU_HOTPLUG
                    LOGW("AP fast suspend: abort seq=%u\r\n", msg.param1);
                    if (bk_pm_ap_fast_suspend_is_prepared()) {
                        ret = bk_pm_ap_fast_restore_hardware();
                        if (ret != BK_OK) {
                            LOGE("AP fast suspend abort: hardware restore failed[%d]\r\n",
                                ret);
                            break;
                        }
                    }
                    if (!bk_cpu_hp_is_online(CPU3_CORE_ID)) {
                        ret = bk_cpu_hp_online_direct(CPU3_CORE_ID);
                        if (ret != BK_OK) {
                            LOGE("AP fast suspend abort: CPU3 online failed[%d]\r\n",
                                ret);
                            break;
                        }
                    }
                    ret = bk_pm_ap_fast_resume_modules();
                    if (ret == BK_ERR_STATE) {
                        /*
                         * The close request may still be waiting for
                         * prepare_power_off and therefore has no fast suspend
                         * transaction to restore. Reopen modules prepared by
                         * the Idle probe in this PM task context.
                         */
                        ret = bk_pm_ap_power_prepare_abort();
                        if (ret == BK_OK) {
                            bk_pm_ap_fast_ipc_rx_block_set(false);
                            bk_pm_ap_full_ready_set(true);
                        }
                    } else if (ret == BK_OK) {
                        /* Fast resume already restored callbacks and state. */
                        (void)bk_pm_ap_power_prepare_abort();
                    } else {
                        LOGE("AP fast suspend abort: module resume failed[%d]\r\n",
                            ret);
                    }
#endif
                }
                break;
                case PM_AP_CORE_APP_RESUME:
                {
                    ret = bk_pm_ap_fast_app_resume();
                    if (ret != BK_OK) {
                        LOGE("AP fast resume: app resume callbacks failed[%d]\r\n",
                            ret);
                    } else {
                        LOGI("AP fast resume: app resume callbacks done\r\n");
                    }
                }
                break;
#endif
                case PM_AP_CORE_SLEEP_WAKEUP_NOTIFY:
                {
                    // bk_err_t ret = portYIELD_CORE(1);
                    // while(ret != BK_OK)
                    // {
                    //     ret = portYIELD_CORE(1);
                    //     LOGE("Wakeup cpu2 failed[%d]\r\n",ret);
                    // }

                    bk_pm_ap_system_wakeup_handle_callback(&msg);
                }
                break;
                case PM_AP_CORE_PSRAM_STATE_NOTIFY:
                {
                    if(msg.param1 == PM_AP_PSRAM_POWER_OFF)
                    {
                        bk_pm_ap_psram_power_state_handle_callback(PM_POWER_PSRAM_MODULE_NAME_MAX,msg.param1);
                        //pm_cp1_mailbox_send_data(PM_CP1_PSRAM_MALLOC_STATE_CMD,0x2,0,0);//not need to response when psram power off
                    }
                    else if(msg.param1 == PM_AP_PSRAM_POWER_ON)
                    {

                    }
                    else if(msg.param1 == PM_CP1_PSRAM_MALLOC_STATE_CMD)
                    {
                        uint32_t used_count = bk_psram_heap_get_used_count();
                        pm_cp1_mailbox_send_data(PM_CP1_PSRAM_MALLOC_STATE_CMD,PM_CP1_PSRAM_MALLOC_STATE_CMD,used_count,0);
                    }
                }
                break;
                case PM_AP_CORE_SLEEP_DEMO_HANDLE:
                {
                    if(msg.param1 == PM_MODE_LOW_VOLTAGE)
                    {
                        bk_pm_ap_sleep_mode_set(PM_MODE_DEFAULT);
                        bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP,0x0,0x0);
                    }
                }
                break;
                default:
                    break;
            }
        }
    }

	return  ret;
}

bk_err_t bk_pm_ap_thread_main(void)
{
    bk_err_t ret = BK_OK;

    if (s_pm_info == NULL)
    {
        s_pm_info = os_malloc(sizeof(pm_ap_core_info_t));

        if (s_pm_info == NULL)
        {
            LOGE("%s, malloc pm_info failed\n", __func__);
            goto error;
        }

        os_memset(s_pm_info, 0, sizeof(pm_ap_core_info_t));
    }

    if (s_pm_info->queue != NULL)
    {
        ret = BK_FAIL;
        LOGE("%s, pm_info->queue allready init, exit!\n", __func__);
        goto error;
    }

    if (s_pm_info->thd != NULL)
    {
        ret = BK_FAIL;
        LOGE("%s, pm_info->thd allready init, exit!\n", __func__);
        goto error;
    }

    ret = rtos_init_queue(&s_pm_info->queue,
                          "pm_info->queue",
                          sizeof(pm_ap_core_msg_t),
                          PM_AP_CORE_QUEUE_NUMBER_OF_MESSAGE);

    if (ret != BK_OK)
    {
        LOGE("%s,ceate queue failed\n");
        goto error;
    }

#if CONFIG_SOC_SMP
    ret = rtos_core0_create_thread(&s_pm_info->thd,
                             PM_AP_CORE_THREAD_PRIORITY,/*pm contrl cmd thread priority need higher*/
                             "pm_info->thd",
                             (beken_thread_function_t)pm_ap_core_message_handle,
                             PM_AP_CORE_STACK_SIZE,
                             NULL);
#else
    ret = rtos_create_thread(&s_pm_info->thd,
                             PM_AP_CORE_THREAD_PRIORITY,/*pm contrl cmd thread priority need higher*/
                             "pm_info->thd",
                             (beken_thread_function_t)pm_ap_core_message_handle,
                             PM_AP_CORE_STACK_SIZE,
                             NULL);
#endif
    if (ret != BK_OK)
    {
        LOGE("create thread fail[%d]\n",ret);
        goto error;
    }

    LOGE("%s success\n", __func__);

    return ret;

error:

    LOGE("%s fail\n", __func__);
	if(s_pm_info->thd != NULL)
	{
		rtos_delete_thread(&s_pm_info->thd);
	}
	if(s_pm_info->queue != NULL)
	{
		rtos_deinit_queue(&s_pm_info->queue);
	}
	os_free(s_pm_info);
	return BK_FAIL;
}

