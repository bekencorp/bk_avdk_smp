/*
 * BK7259 AP fast-boot I2C driver PM integration demo
 *
 * REFERENCE ONLY: this file is intentionally not added to any CMake source
 * list and is not compiled. Driver owners should integrate the pattern into
 * the real i2c_driver.c instead of exporting these hooks to applications.
 *
 * Design goals:
 *  - Applications keep using normal bk_i2c_* APIs across AP fast resume.
 *  - Only I2C instances successfully initialized by an application take part
 *    in suspend/resume.
 *  - SRAM-retained software objects (mutexes, semaphores and init flags) are
 *    not created twice.
 *  - Volatile hardware state is reset and rebuilt from retained configuration
 *    instead of restoring stale FIFO/BUSY/IRQ transaction state.
 */

#include <common/bk_include.h>
#include <driver/i2c.h>
#include <modules/pm.h>
#include <os/os.h>

#if CONFIG_PM_AP_FAST_BOOT_ENABLE && CONFIG_I2C

#define I2C_FAST_QUIESCE_TIMEOUT_MS (20U)

typedef struct {
	uint32_t active_mask;
	volatile uint32_t busy_count[I2C_ID_MAX];
	volatile bool accepting[I2C_ID_MAX];
	i2c_config_t config[I2C_ID_MAX];
	volatile bool restore_required;
	bool pm_registered;
} i2c_fast_demo_context_t;

/*
 * This context is retained in AP SRAM. In a real integration these members
 * should normally be added to the existing private I2C driver context.
 */
static i2c_fast_demo_context_t s_i2c_fast_demo;

/*
 * Replace the body with existing private I2C driver primitives. This function
 * runs from resume(), where interrupts and RTOS services are available.
 *
 * Required order:
 *  1. enable power/clock and restore pinmux;
 *  2. reset the I2C controller;
 *  3. apply the retained baud/address configuration;
 *  4. reset FIFO, software transaction state and stale pending IRQs;
 *  5. enable the peripheral IRQ.
 *
 * Do not recreate retained mutexes/semaphores and do not restore BUSY, FIFO,
 * interrupt-status or an interrupted transaction.
 */
static bk_err_t i2c_demo_hw_reinit_from_cache(i2c_id_t id,
	const i2c_config_t *config)
{
	(void)id;
	(void)config;

	/* Example integration inside i2c_driver.c:
	 *     i2c_clock_enable(id);
	 *     i2c_init_gpio(id);
	 *     i2c_hal_soft_reset(&s_i2c[id].hal);
	 *     i2c_hal_configure(&s_i2c[id].hal, config);
	 *     i2c_transtate_reset(id);
	 *     i2c_interrupt_enable(id);
	 */
	return BK_OK;
}

/*
 * Wrap every public transfer entry with enter/exit. The second accepting
 * check closes the race where quiesce starts immediately after the first
 * check.
 */
static bk_err_t i2c_demo_transfer_enter(i2c_id_t id)
{
	if ((id >= I2C_ID_MAX) ||
		!(__atomic_load_n(&s_i2c_fast_demo.active_mask,
			__ATOMIC_ACQUIRE) & BIT(id)) ||
		!__atomic_load_n(&s_i2c_fast_demo.accepting[id],
			__ATOMIC_ACQUIRE)) {
		return BK_ERR_BUSY;
	}

	__atomic_add_fetch(&s_i2c_fast_demo.busy_count[id], 1,
		__ATOMIC_ACQ_REL);
	if (!__atomic_load_n(&s_i2c_fast_demo.accepting[id],
		__ATOMIC_ACQUIRE)) {
		__atomic_sub_fetch(&s_i2c_fast_demo.busy_count[id], 1,
			__ATOMIC_ACQ_REL);
		return BK_ERR_BUSY;
	}
	return BK_OK;
}

static void i2c_demo_transfer_exit(i2c_id_t id)
{
	__atomic_sub_fetch(&s_i2c_fast_demo.busy_count[id], 1,
		__ATOMIC_RELEASE);
}

static bk_err_t i2c_demo_fast_quiesce(void *arg)
{
	i2c_fast_demo_context_t *ctx = arg;
	uint32_t active = __atomic_load_n(&ctx->active_mask,
		__ATOMIC_ACQUIRE);
	uint32_t start_ms;

	for (i2c_id_t id = I2C_ID_0; id < I2C_ID_MAX; id++) {
		if (active & BIT(id)) {
			__atomic_store_n(&ctx->accepting[id], false,
				__ATOMIC_RELEASE);
		}
	}

	start_ms = rtos_get_time();
	for (;;) {
		bool busy = false;

		for (i2c_id_t id = I2C_ID_0; id < I2C_ID_MAX; id++) {
			if ((active & BIT(id)) &&
				__atomic_load_n(&ctx->busy_count[id],
					__ATOMIC_ACQUIRE)) {
				busy = true;
				break;
			}
		}
		if (!busy) {
			return BK_OK;
		}
		if ((rtos_get_time() - start_ms) >=
			I2C_FAST_QUIESCE_TIMEOUT_MS) {
			for (i2c_id_t id = I2C_ID_0;
				id < I2C_ID_MAX; id++) {
				if (active & BIT(id)) {
					__atomic_store_n(&ctx->accepting[id],
						true, __ATOMIC_RELEASE);
				}
			}
			return BK_ERR_TIMEOUT;
		}
		rtos_delay_milliseconds(1);
	}
}

/*
 * backup/restore execute with CPU3 offline and CPU2 interrupts disabled.
 * This demo only records a retained marker; it deliberately performs no
 * logging, allocation, locking, delay or complete driver reinitialization.
 */
static bk_err_t i2c_demo_fast_backup(void *arg)
{
	i2c_fast_demo_context_t *ctx = arg;

	ctx->restore_required = true;
	__DMB();
	return BK_OK;
}

static bk_err_t i2c_demo_fast_restore(void *arg)
{
	(void)arg;

	/*
	 * Keep this callback for the PM framework's backup/restore pairing and
	 * dependency ordering. Hardware is rebuilt safely in resume().
	 */
	return BK_OK;
}

static bk_err_t i2c_demo_fast_resume(void *arg)
{
	i2c_fast_demo_context_t *ctx = arg;
	uint32_t active = __atomic_load_n(&ctx->active_mask,
		__ATOMIC_ACQUIRE);
	bk_err_t ret;

	/*
	 * restore_required is false when suspend is cancelled before backup, so
	 * rollback only reopens submissions and does not reset live hardware.
	 */
	if (ctx->restore_required) {
		for (i2c_id_t id = I2C_ID_0; id < I2C_ID_MAX; id++) {
			if (!(active & BIT(id))) {
				continue;
			}
			ret = i2c_demo_hw_reinit_from_cache(id,
				&ctx->config[id]);
			if (ret != BK_OK) {
				return ret;
			}
		}
		ctx->restore_required = false;
	}

	for (i2c_id_t id = I2C_ID_0; id < I2C_ID_MAX; id++) {
		if (active & BIT(id)) {
			__atomic_store_n(&ctx->accepting[id], true,
				__ATOMIC_RELEASE);
		}
	}
	return BK_OK;
}

static const pm_ap_fast_pm_ops_t s_i2c_demo_fast_ops = {
	.name = "i2c",
	.quiesce = i2c_demo_fast_quiesce,
	.backup = i2c_demo_fast_backup,
	.restore = i2c_demo_fast_restore,
	.resume = i2c_demo_fast_resume,
	.arg = &s_i2c_fast_demo,
	.priority = PM_AP_FAST_PRIORITY_PERIPHERAL,
};

/*
 * Call after validating id/config but before exposing a successfully
 * initialized instance to other tasks. Register only on first actual use.
 */
static bk_err_t i2c_demo_prepare_init(i2c_id_t id,
	const i2c_config_t *config)
{
	bk_err_t ret;

	if (!s_i2c_fast_demo.pm_registered) {
		ret = bk_pm_ap_fast_ops_register(&s_i2c_demo_fast_ops);
		if (ret != BK_OK) {
			return ret;
		}
		s_i2c_fast_demo.pm_registered = true;
	}

	s_i2c_fast_demo.config[id] = *config;
	__atomic_store_n(&s_i2c_fast_demo.accepting[id], true,
		__ATOMIC_RELEASE);
	__atomic_or_fetch(&s_i2c_fast_demo.active_mask, BIT(id),
		__ATOMIC_RELEASE);
	return BK_OK;
}

/* Call if the following real hardware initialization fails. */
static void i2c_demo_init_rollback(i2c_id_t id)
{
	__atomic_store_n(&s_i2c_fast_demo.accepting[id], false,
		__ATOMIC_RELEASE);
	__atomic_and_fetch(&s_i2c_fast_demo.active_mask, ~BIT(id),
		__ATOMIC_RELEASE);
}

/*
 * Call from bk_i2c_deinit() after stopping transfers and before destroying
 * per-instance software resources. Keeping one empty registered callback
 * avoids unregister-versus-suspend races; active_mask == 0 makes it a no-op.
 */
static void i2c_demo_on_deinit(i2c_id_t id)
{
	__atomic_store_n(&s_i2c_fast_demo.accepting[id], false,
		__ATOMIC_RELEASE);
	__atomic_and_fetch(&s_i2c_fast_demo.active_mask, ~BIT(id),
		__ATOMIC_RELEASE);
}

/*
 * Real driver integration sketch:
 *
 * bk_i2c_init(id, config):
 *     ret = i2c_demo_prepare_init(id, config);
 *     if (ret != BK_OK)
 *         return ret;
 *     ret = existing_i2c_hardware_init(id, config);
 *     if (ret != BK_OK)
 *         i2c_demo_init_rollback(id);
 *
 * bk_i2c_master_read/write(...):
 *     ret = i2c_demo_transfer_enter(id);
 *     if (ret != BK_OK)
 *         return ret;
 *     ret = existing_transfer(...);
 *     i2c_demo_transfer_exit(id);
 *
 * bk_i2c_deinit(id):
 *     existing_wait_idle_and_hardware_deinit(id);
 *     i2c_demo_on_deinit(id);
 */

#endif /* CONFIG_PM_AP_FAST_BOOT_ENABLE && CONFIG_I2C */
