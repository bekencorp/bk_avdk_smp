#include <stdint.h>
#include "hspl/hspl_res_lock.h"
#include "sspl_lock.h"
#include "os/os.h"

#if !CONFIG_HSPL
#define aspl_res_lock bk_sspl_res_lock
#define aspl_res_unlock bk_sspl_res_unlock
#else
#define aspl_res_lock bk_hspl_res_must_lock
#define aspl_res_unlock bk_hspl_res_unlock
#endif

uint32_t bk_aspl_uart_log_enter_critical(void)
{
    uint32_t flags = rtos_disable_int();
    aspl_res_lock(BK_HSPL_RES_UART_LOG);
    return flags;
}

void bk_aspl_uart_log_exit_critical(uint32_t flags)
{
    aspl_res_unlock(BK_HSPL_RES_UART_LOG);
    rtos_enable_int(flags);
}

void bk_aspl_uart_log_lock(void)
{
    aspl_res_lock(BK_HSPL_RES_UART_LOG);
}

void bk_aspl_uart_log_unlock(void)
{
    aspl_res_unlock(BK_HSPL_RES_UART_LOG);
}

uint32_t bk_aspl_sys_sw_regs_enter_critical(void)
{
    uint32_t flags = rtos_disable_int();
    aspl_res_lock(BK_HSPL_RES_SYS_SW_REGS);
    return flags;
}

void bk_aspl_sys_sw_regs_exit_critical(uint32_t flags)
{
    aspl_res_unlock(BK_HSPL_RES_SYS_SW_REGS);
    rtos_enable_int(flags);
}
