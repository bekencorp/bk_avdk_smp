// Copyright 2025 Beken
//
// BK7259 TF-M armino_min: SCA / side-channel-defence no-op weak stubs.
//
// The Beken-patched TF-M spm core/main.c and bl2_main.c unconditionally call a
// set of SCA / sensor helpers (anti-tamper, adc, temp/volt detect, video power)
// that belong to the full-feature SDK driver tree. BK7259 Phase-1 secure boot
// does not use these, so provide weak no-op definitions to keep the secure
// world self-contained. Marked weak so a real driver can override later.

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <common/bk_include.h>
#include <driver/anti_tamper.h>
#include <modules/pm.h>

__attribute__((weak)) bk_err_t bk_anti_tamper_enable(gpio_id_t tx_port, gpio_id_t rx_port)
{
	(void)tx_port;
	(void)rx_port;
	return BK_OK;
}

/* The Beken Dubhe driver (dubhe_driver.c) registers a low-voltage sleep
 * callback and (on the non-TEE_M path) votes the TrustEngine power module.
 * BL2/secure-boot has no PM/sleep framework, so stub these as no-ops. */
__attribute__((weak)) bk_err_t bk_pm_sleep_register_cb(pm_sleep_mode_e sleep_mode,
		pm_dev_id_e dev_id, pm_cb_conf_t *enter_config, pm_cb_conf_t *exit_config)
{
	(void)sleep_mode;
	(void)dev_id;
	(void)enter_config;
	(void)exit_config;
	return BK_OK;
}

__attribute__((weak)) bk_err_t bk_pm_module_vote_power_ctrl(pm_power_module_name_e module,
		pm_power_module_state_e power_state)
{
	(void)module;
	(void)power_state;
	return BK_OK;
}

__attribute__((weak)) bk_err_t bk_anti_tamper_disable(void)
{
	return BK_OK;
}

__attribute__((weak)) bk_err_t bk_adc_driver_init(void)
{
	return BK_OK;
}

__attribute__((weak)) int temp_detect_init(uint32_t init_temperature)
{
	(void)init_temperature;
	return BK_OK;
}

__attribute__((weak)) void temp_sensor_enable(void)
{
}

__attribute__((weak)) void volt_detect_set_config(void)
{
}

__attribute__((weak)) void volt_temp_detect(void)
{
}

__attribute__((weak)) int sys_drv_video_power_en(uint32_t value)
{
	(void)value;
	return 0;
}

/* The Beken-patched secure crypto dispatcher (crypto_init.c) wires every PSA
 * crypto call through a SysTick FIH monitor via register/unregister callbacks.
 * Those are only defined by bk_systick_monitor.c under CONFIG_TFM_SW_FIH, which
 * is off in this boot-first phase. Provide weak no-op pass-through stubs so the
 * dispatcher links; a real SW-FIH build overrides them. */
__attribute__((weak)) bool register_systick_callback(
		int32_t (*func)(void *, size_t, void *, size_t),
		void *a0, size_t a1, void *a2, size_t a3)
{
	(void)func; (void)a0; (void)a1; (void)a2; (void)a3;
	return true;
}

__attribute__((weak)) void unregister_systick_callback(
		int32_t (*func)(void *, size_t, void *, size_t))
{
	(void)func;
}

/* The TF-M SPE runs single-core on BK7259 (SMP NS cores are launched
 * separately). rtos_get_core_id() -> bk_multicore_get_cpu_id() always resolves
 * to the primary core here. */
__attribute__((weak)) uint32_t bk_multicore_get_cpu_id(void)
{
	return 0;
}
