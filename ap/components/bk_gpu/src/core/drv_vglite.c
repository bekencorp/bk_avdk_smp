#include <stddef.h>
#include <stdint.h>

#include <os/os.h>
#include <driver/int.h>
#include "sys_driver.h"
#include <driver/sys_pm.h>
#include "drv_vglite.h"
#include "vg_lite.h"
#include "vg_lite_hw.h"
#include "vg_lite_platform.h"


#define TAG "drv_vglite"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


void vg_lite_IRQHandler(void);
void int_handler_gpu(void)
{
    bk_printf("gpu isr\r\n");
    vg_lite_IRQHandler();
}


// int gpu_bsp_init(bk_lcd_panel_t* panel)
int gpu_bsp_init(uint32_t tess_width, uint32_t tess_heigth)
{
    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_GPU, PM_POWER_MODULE_STATE_ON);

    sys_drv_gpu_cksel_clkdiv_set(CKSEL_GPU_480M, 0);

    bk_pm_clock_ctrl(PM_CLK_ID_GPU, PM_CLK_CTRL_PWR_UP);

    bk_int_isr_register(INT_SRC_GPU, vg_lite_IRQHandler, NULL);
#if CONFIG_SOC_SMP
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_GPU, 1);
#else
    sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_GPU, 1);
#endif

    return vg_lite_init(tess_width, tess_heigth);
}

int gpu_bsp_terminate(void)
{
    vg_lite_close();

#if CONFIG_SOC_SMP
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_GPU, 0);
#else
    sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_GPU, 0);
#endif
    bk_int_isr_unregister(INT_SRC_GPU);

    bk_pm_clock_ctrl(PM_CLK_ID_GPU, PM_CLK_CTRL_PWR_DOWN);
    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_GPU, PM_POWER_MODULE_STATE_OFF);

    return 0;
}