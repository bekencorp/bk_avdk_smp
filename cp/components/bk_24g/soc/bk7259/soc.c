#include "sdkconfig.h"
#include "os/os.h"
#include <components/log.h>
#include "driver/int.h"
#include "sys_driver.h"
#include "bk_uart.h"
#include "gpio_driver.h"
#include <driver/int_types.h>
#include <driver/hal/hal_int_types.h>
#include "bk_drv_model.h"
#include <modules/pm.h>
#include <driver/pwr_clk.h>
#include <components/system.h>
#include <driver/uart.h>
#include "arch_interrupt.h"
#include <driver/gpio.h>
#include <os/mem.h>
#include <os/str.h>
#include <string.h>
#include "aon_pmu_hal.h"
//#include "bk_cal_ex.h"
#include "bk_rf_internal.h"
#include "driver/ckmn.h"
#include "bk_24g_private.h"

static bk_err_t int_isr_register_wrapper(void *isr, void *arg)
{
    return bk_int_isr_register(INT_SRC_BK24, (int_group_isr_t)isr, arg);
}

static bk_err_t int_isr_unregister_wrapper(void)
{
    return bk_int_isr_unregister(INT_SRC_BK24);
}

static bk_err_t int_ctrl_wrapper(bool en)
{
    return sys_drv_set_int_en(0, INT_SRC_BK24, en);
}

static bk_err_t power_ctrl_wrapper(uint8_t power_state)
{
    pm_power_module_state_e state = (power_state ? PM_POWER_MODULE_STATE_ON : PM_POWER_MODULE_STATE_OFF);

    BK_LOGI(NULL, "%s power_state %d\n", __func__, power_state);

    return bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_BK24, state);
}

static bk_err_t clock_ctrl_wrapper(uint8_t clock_state)
{
    pm_dev_clk_pwr_e state = (clock_state ? PM_CLK_CTRL_PWR_UP : PM_CLK_CTRL_PWR_DOWN);

    BK_LOGI(NULL, "%s clock_state %d\n", __func__, clock_state);

    if (clock_state)
    {
        BK_LOGW(NULL, "%s bluetooth and bk2.4g can not be enabled at the same time !!!!\n", __func__);
        bk_pm_clock_ctrl(PM_CLK_ID_BTDM, PM_CLK_CTRL_PWR_DOWN);
    }

    if (clock_state)
    {
        return bk_pm_clock_ctrl(PM_CLK_ID_BK24, state) ||
               bk_pm_clock_ctrl(PM_CLK_ID_XVR, state) ||
               bk_pm_clock_ctrl(PM_CLK_ID_WLSS, state) ||
               bk_pm_clock_ctrl(PM_CLK_ID_RF, state);
    }
    else
    {
        return bk_pm_clock_ctrl(PM_CLK_ID_BK24, state);
        // bk_pm_clock_ctrl(PM_CLK_ID_XVR, state);
        // bk_pm_clock_ctrl(PM_CLK_ID_WLSS, state);
        // bk_pm_clock_ctrl(PM_CLK_ID_RF, state);
    }
}

static void vote_rf_ctrl_wrapper(uint8_t cmd)
{
    rf_module_vote_ctrl(cmd, 1 << 1); //RF_BY_BLE_BIT
}

static void set_pwr_table_wrapper(void)
{
    extern uint8_t manual_cal_get_ble_pwr_idx(uint8_t channel);
    uint8_t table1_index = manual_cal_get_ble_pwr_idx(19);

    extern UINT32 manual_cal_txpwr_tab_ready_in_flash(void);
    uint32_t is_cali = manual_cal_txpwr_tab_ready_in_flash();

    BK_LOGI(NULL, "%s cali_ready_status:0x%x, ble_cali_staus: %d\n", __func__, is_cali, (is_cali & 0x08));

    extern void ble_cal_set_txpwr(uint8_t idx);
    ble_cal_set_txpwr(table1_index);
}

static bk_err_t delay_milliseconds_wrapper(uint32_t num_ms)
{
    return rtos_delay_milliseconds(num_ms);
}

static void delay_us_wrapper(uint32_t us)
{
    extern void bk_delay_us(UINT32 us);
    bk_delay_us(us);
}

static void *malloc_wrapper(size_t size)
{
    return os_malloc(size);
}

static void free_wrapper(void *ptr)
{
    os_free(ptr);
}

static const struct bk_24g_osi_funcs_t s_bk_24g_osi_funcs =
{
    ._int_isr_register_wrapper = int_isr_register_wrapper,
    ._int_isr_unregister_wrapper = int_isr_unregister_wrapper,
    ._int_ctrl_wrapper = int_ctrl_wrapper,
    ._power_ctrl_wrapper = power_ctrl_wrapper,
    ._clock_ctrl_wrapper = clock_ctrl_wrapper,
    ._vote_rf_ctrl_wrapper = vote_rf_ctrl_wrapper,
    ._set_pwr_table_wrapper = set_pwr_table_wrapper,
    ._delay_milliseconds = delay_milliseconds_wrapper,
    ._delay_us = delay_us_wrapper,
    ._malloc = malloc_wrapper,
    ._free = free_wrapper,
    ._log = bk_printf_ext,
};

int bk_24g_os_adapter_init(void)
{
    bk_err_t ret = BK_OK;

    if (bk24_os_adapter_init((void *)&s_bk_24g_osi_funcs) != 0)
    {
        return BK_FAIL;
    }

    return ret;
}
