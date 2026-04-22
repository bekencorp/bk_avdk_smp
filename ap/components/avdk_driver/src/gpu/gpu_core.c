#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <string.h>
#include <common/bk_include.h>
#include <components/log.h>
#include <driver/sys_pm.h>
#include "sys_driver.h"
#include <modules/vg_lite_gpu/vg_lite_platform.h>

#define TAG "gpu_core"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


static bool gpu_driver_is_init = false;

void bk_gpu_driver_init(void)
{
    if (gpu_driver_is_init == true)
    {
        LOGW("%s has inited %x\r\n", __func__);
        return;
    }

    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_GPU, PM_POWER_MODULE_STATE_ON);

    sys_drv_gpu_cksel_clkdiv_set(CKSEL_GPU_480M, 0);

    bk_pm_clock_ctrl(PM_CLK_ID_GPU, PM_CLK_CTRL_PWR_UP);

    bk_int_isr_register(INT_SRC_GPU, vg_lite_IRQHandler, NULL);

#if CONFIG_SOC_SMP
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_GPU, 1);
#else
    sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_GPU, 1);
#endif

    gpu_driver_is_init = true;
}

void bk_gpu_driver_deinit(void)
{
    if (gpu_driver_is_init == false)
    {
        LOGW("%s has inited %x\r\n", __func__);
        return;
    }

#if CONFIG_SOC_SMP
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_GPU, 0);
#else
    sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_GPU, 0);
#endif
    bk_int_isr_unregister(INT_SRC_GPU);

    bk_pm_clock_ctrl(PM_CLK_ID_GPU, PM_CLK_CTRL_PWR_DOWN);

    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_GPU, PM_POWER_MODULE_STATE_OFF);

    gpu_driver_is_init = false;
}
