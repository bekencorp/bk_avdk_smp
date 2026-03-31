
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t bk_aspl_uart_log_enter_critical(void);
void bk_aspl_uart_log_exit_critical(uint32_t flags);
void bk_aspl_uart_log_lock(void);
void bk_aspl_uart_log_unlock(void);

uint32_t bk_aspl_sys_sw_regs_enter_critical(void);
void bk_aspl_sys_sw_regs_exit_critical(uint32_t flags);

#ifdef __cplusplus
}
#endif
