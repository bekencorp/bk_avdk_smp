
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


uint32_t bk_aspl_driver_enter_critical(void);
/**
 * @brief Exit critical section for driver (release HSPL lock and restore IRQ)
 */
void bk_aspl_driver_exit_critical(uint32_t flags);


/**
 * @brief Enter critical section for flash (disable IRQ and acquire HSPL lock)
 */
uint32_t bk_aspl_flash_enter_critical(void);

/**
 * @brief Exit critical section for flash (release HSPL lock and restore IRQ)
 */
void bk_aspl_flash_exit_critical(uint32_t flags);

/**
 * @brief Enter critical section for OS (disable IRQ and acquire HSPL lock)
 */
uint32_t bk_aspl_os_enter_critical(void);

/**
 * @brief Exit critical section for OS (release HSPL lock and restore IRQ)
 */
void bk_aspl_os_exit_critical(uint32_t flags);



#ifdef __cplusplus
}
#endif
