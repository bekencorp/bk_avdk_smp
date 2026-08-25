#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <string.h>
#include <common/bk_include.h>
#include <components/log.h>
#include <driver/sys_pm.h>
#include <modules/pm.h>
#include "sys_driver.h"
#include <modules/vg_lite_gpu/vg_lite_platform.h>

#define TAG "gpu_core"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

static uint32_t s_gpu_driver_ref_cnt = 0;
static beken_mutex_t s_gpu_driver_lock = NULL;
static beken_mutex_t s_gpu_global_lock = NULL;

static bk_err_t gpu_driver_lock_init(void)
{
    beken_mutex_t lock = NULL;
    GLOBAL_INT_DECLARATION();

    if (s_gpu_driver_lock != NULL)
    {
        return BK_OK;
    }

    if (rtos_init_mutex(&lock) != BK_OK)
    {
        return BK_FAIL;
    }

    GLOBAL_INT_DISABLE();
    if (s_gpu_driver_lock == NULL)
    {
        s_gpu_driver_lock = lock;
        lock = NULL;
    }
    GLOBAL_INT_RESTORE();

    if (lock != NULL)
    {
        rtos_deinit_mutex(&lock);
    }

    return BK_OK;
}

static bk_err_t gpu_driver_lock(void)
{
    if (rtos_is_in_interrupt_context())
    {
        return BK_FAIL;
    }

    if (gpu_driver_lock_init() != BK_OK)
    {
        return BK_FAIL;
    }

    return rtos_lock_mutex(&s_gpu_driver_lock);
}

void bk_gpu_driver_init(void)
{
    if (gpu_driver_lock() != BK_OK)
    {
        if (!rtos_is_in_interrupt_context())
        {
            LOGE("%s gpu driver lock failed\r\n", __func__);
        }
        return;
    }

    if (s_gpu_driver_ref_cnt > 0)
    {
        s_gpu_driver_ref_cnt++;
        LOGD("%s ref_cnt=%u\r\n", __func__, s_gpu_driver_ref_cnt);
        rtos_unlock_mutex(&s_gpu_driver_lock);
        return;
    }

    if ((s_gpu_global_lock == NULL) && (rtos_init_mutex(&s_gpu_global_lock) != BK_OK))
    {
        rtos_unlock_mutex(&s_gpu_driver_lock);
        LOGE("%s gpu global lock init failed\r\n", __func__);
        return;
    }

    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_GPU, PM_POWER_MODULE_STATE_ON);
    bk_pm_module_vote_cpu_freq(PM_DEV_ID_GPU, PM_CPU_FRQ_480M);

    sys_drv_gpu_cksel_clkdiv_set(CKSEL_GPU_480M, 0);

    bk_pm_clock_ctrl(PM_CLK_ID_GPU, PM_CLK_CTRL_PWR_UP);

    bk_int_isr_register(INT_SRC_GPU, vg_lite_IRQHandler, NULL);

#if CONFIG_SOC_SMP
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_GPU, 1);
#else
    sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_GPU, 1);
#endif

    s_gpu_driver_ref_cnt = 1;
    LOGI("%s done, ref_cnt=%u\r\n", __func__, s_gpu_driver_ref_cnt);
    rtos_unlock_mutex(&s_gpu_driver_lock);
}

void bk_gpu_driver_deinit(void)
{
    if (gpu_driver_lock() != BK_OK)
    {
        if (!rtos_is_in_interrupt_context())
        {
            LOGE("%s gpu driver lock failed\r\n", __func__);
        }
        return;
    }

    if (s_gpu_driver_ref_cnt == 0)
    {
        rtos_unlock_mutex(&s_gpu_driver_lock);
        LOGW("%s has deinit\r\n", __func__);
        return;
    }

    if (s_gpu_driver_ref_cnt > 1)
    {
        s_gpu_driver_ref_cnt--;
        LOGD("%s ref_cnt=%u\r\n", __func__, s_gpu_driver_ref_cnt);
        rtos_unlock_mutex(&s_gpu_driver_lock);
        return;
    }

#if CONFIG_SOC_SMP
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_GPU, 0);
#else
    sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_GPU, 0);
#endif
    bk_int_isr_unregister(INT_SRC_GPU);

    bk_pm_clock_ctrl(PM_CLK_ID_GPU, PM_CLK_CTRL_PWR_DOWN);

    bk_pm_module_vote_cpu_freq(PM_DEV_ID_GPU, PM_CPU_FRQ_DEFAULT);
    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_GPU, PM_POWER_MODULE_STATE_OFF);

    if (s_gpu_global_lock)
    {
        rtos_deinit_mutex(&s_gpu_global_lock);
        s_gpu_global_lock = NULL;
    }

    s_gpu_driver_ref_cnt = 0;
    LOGI("%s done, ref_cnt=%u\r\n", __func__, s_gpu_driver_ref_cnt);
    rtos_unlock_mutex(&s_gpu_driver_lock);
}

bk_err_t bk_gpu_global_lock(void)
{
    if (s_gpu_global_lock == NULL)
    {
        return BK_FAIL;
    }

    return rtos_lock_mutex(&s_gpu_global_lock);
}

bk_err_t bk_gpu_global_unlock(void)
{
    if (s_gpu_global_lock == NULL)
    {
        return BK_FAIL;
    }

    return rtos_unlock_mutex(&s_gpu_global_lock);
}
