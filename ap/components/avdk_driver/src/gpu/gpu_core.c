#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <string.h>
#include <common/bk_include.h>
#include <components/log.h>
#include <driver/sys_pm.h>
#include <modules/pm.h>
#include "sys_driver.h"
#include "spinlock.h"
#include <modules/vg_lite_gpu/vg_lite_platform.h>

#define TAG "gpu_core"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

typedef enum
{
    GPU_DRIVER_STATE_DEINIT = 0,
    GPU_DRIVER_STATE_INITING,
    GPU_DRIVER_STATE_INITED,
    GPU_DRIVER_STATE_DEINITING,
} gpu_driver_state_t;

typedef struct
{
    gpu_driver_state_t state;
    uint32_t ref_cnt;
} gpu_driver_state_ctx_t;

static gpu_driver_state_ctx_t s_gpu_driver_state = {
    .state = GPU_DRIVER_STATE_DEINIT,
    .ref_cnt = 0,
};
static beken_mutex_t s_gpu_global_lock = NULL;
#if CONFIG_SOC_SMP
static SPINLOCK_SECTION volatile spinlock_t s_gpu_driver_state_lock = SPIN_LOCK_INIT;
#endif

static inline uint32_t gpu_driver_state_lock(void)
{
    uint32_t irq_flags;
#if CONFIG_SOC_SMP
    spin_lock_irqsave(&s_gpu_driver_state_lock, irq_flags);
#else
    irq_flags = rtos_disable_int();
#endif
    return irq_flags;
}

static inline void gpu_driver_state_unlock(uint32_t irq_flags)
{
#if CONFIG_SOC_SMP
    spin_unlock_irqrestore(&s_gpu_driver_state_lock, irq_flags);
#else
    rtos_enable_int(irq_flags);
#endif
}

void bk_gpu_driver_init(void)
{
    while (1)
    {
        uint32_t irq_flags = gpu_driver_state_lock();
        if (s_gpu_driver_state.state == GPU_DRIVER_STATE_INITED)
        {
            s_gpu_driver_state.ref_cnt++;
            gpu_driver_state_unlock(irq_flags);
            return;
        }

        if (s_gpu_driver_state.state == GPU_DRIVER_STATE_DEINIT)
        {
            s_gpu_driver_state.state = GPU_DRIVER_STATE_INITING;
            gpu_driver_state_unlock(irq_flags);
            break;
        }

        gpu_driver_state_unlock(irq_flags);
        rtos_delay_milliseconds(1);
    }

    if ((s_gpu_global_lock == NULL) && (rtos_init_mutex(&s_gpu_global_lock) != BK_OK))
    {
        uint32_t irq_flags = gpu_driver_state_lock();
        s_gpu_driver_state.state = GPU_DRIVER_STATE_DEINIT;
        s_gpu_driver_state.ref_cnt = 0;
        gpu_driver_state_unlock(irq_flags);
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

    uint32_t irq_flags = gpu_driver_state_lock();
    s_gpu_driver_state.state = GPU_DRIVER_STATE_INITED;
    s_gpu_driver_state.ref_cnt = 1;
    gpu_driver_state_unlock(irq_flags);
}

void bk_gpu_driver_deinit(void)
{
    while (1)
    {
        uint32_t irq_flags = gpu_driver_state_lock();
        if (s_gpu_driver_state.state == GPU_DRIVER_STATE_DEINIT)
        {
            gpu_driver_state_unlock(irq_flags);
            LOGW("%s has deinit\r\n", __func__);
            return;
        }

        if (s_gpu_driver_state.state == GPU_DRIVER_STATE_INITED)
        {
            if (s_gpu_driver_state.ref_cnt > 1)
            {
                s_gpu_driver_state.ref_cnt--;
                gpu_driver_state_unlock(irq_flags);
                return;
            }

            s_gpu_driver_state.state = GPU_DRIVER_STATE_DEINITING;
            gpu_driver_state_unlock(irq_flags);
            break;
        }

        gpu_driver_state_unlock(irq_flags);
        rtos_delay_milliseconds(1);
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

    uint32_t irq_flags = gpu_driver_state_lock();
    s_gpu_driver_state.state = GPU_DRIVER_STATE_DEINIT;
    s_gpu_driver_state.ref_cnt = 0;
    gpu_driver_state_unlock(irq_flags);
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
