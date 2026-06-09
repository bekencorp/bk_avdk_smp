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

uint32_t bk_aspl_driver_enter_critical(void)
{
	uint32_t flags = rtos_disable_int();
	aspl_res_lock(BK_HSPL_RES_SYS);
	return flags;
}

void bk_aspl_driver_exit_critical(uint32_t flags)
{
	aspl_res_unlock(BK_HSPL_RES_SYS);
	rtos_enable_int(flags);
}

uint32_t bk_aspl_flash_enter_critical(void)
{
	uint32_t flags = rtos_disable_int();
	aspl_res_lock(BK_HSPL_RES_FLASH);
	return flags;
}

void bk_aspl_flash_exit_critical(uint32_t flags)
{
	aspl_res_unlock(BK_HSPL_RES_FLASH);
	rtos_enable_int(flags);
}

uint32_t bk_aspl_os_enter_critical(void)
{
	uint32_t flags = rtos_disable_int();
	aspl_res_lock(BK_HSPL_RES_OS);
	return flags;
}

void bk_aspl_os_exit_critical(uint32_t flags)
{
	aspl_res_unlock(BK_HSPL_RES_OS);
	rtos_enable_int(flags);
}


void bk_aspl_sys_sw_regs_lock(void)
{
	aspl_res_lock(BK_HSPL_RES_SYS_SW_REGS);
}

void bk_aspl_sys_sw_regs_unlock(void)
{
	aspl_res_unlock(BK_HSPL_RES_SYS_SW_REGS);
}

uint32_t bk_aspl_vdec_enter_critical(void)
{
	uint32_t flags = rtos_disable_int();
	aspl_res_lock(BK_HSPL_RES_VDEC);
	return flags;
}

void bk_aspl_vdec_exit_critical(uint32_t flags)
{
	aspl_res_unlock(BK_HSPL_RES_VDEC);
	rtos_enable_int(flags);
}

uint32_t bk_aspl_venc_enter_critical(void)
{
	uint32_t flags = rtos_disable_int();
	aspl_res_lock(BK_HSPL_RES_VENC);
	return flags;
}

void bk_aspl_venc_exit_critical(uint32_t flags)
{
	aspl_res_unlock(BK_HSPL_RES_VENC);
	rtos_enable_int(flags);
}

uint32_t bk_aspl_isp_enter_critical(void)
{
	uint32_t flags = rtos_disable_int();
	aspl_res_lock(BK_HSPL_RES_ISP);
	return flags;
}

void bk_aspl_isp_exit_critical(uint32_t flags)
{
	aspl_res_unlock(BK_HSPL_RES_ISP);
	rtos_enable_int(flags);
}

uint32_t bk_aspl_npu_enter_critical(void)
{
	uint32_t flags = rtos_disable_int();
	aspl_res_lock(BK_HSPL_RES_NPU);
	return flags;
}

void bk_aspl_npu_exit_critical(uint32_t flags)
{
	aspl_res_unlock(BK_HSPL_RES_NPU);
	rtos_enable_int(flags);
}