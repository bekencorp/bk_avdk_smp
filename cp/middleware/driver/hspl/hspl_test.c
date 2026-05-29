// Copyright 2020-2026 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/os.h>
#include "cli.h"
#include "hspl_driver.h"
#include "hspl_res_lock.h"
#include <components/log.h>
#include <stdint.h>
#include <string.h>

#include "ipi_driver.h"
#include "dwt.h"


#define HSPL_TEST_TAG "hspl_test"
#define HSPL_TEST_LOGI(...) BK_LOGI(HSPL_TEST_TAG, ##__VA_ARGS__)
#define HSPL_TEST_LOGD(...) BK_LOGD(HSPL_TEST_TAG, ##__VA_ARGS__)
#define HSPL_TEST_LOGE(...) BK_LOGE(HSPL_TEST_TAG, ##__VA_ARGS__)

static void hspl_timeout_cb(uint8_t channel, void *param)
{
	HSPL_TEST_LOGI("timeout irq: ch=%u param=0x%p\r\n", channel, param);
}

/* Stress test state */
typedef struct {
	volatile bool running;
	volatile bool start_flag;
	volatile uint32_t lock_success;
	volatile uint32_t lock_fail;
	volatile uint32_t unlock_success;
	volatile uint32_t unlock_fail;
	volatile uint32_t timeout_count;
	bk_hspl_id_t hspl_id;
	uint8_t channel;
	uint32_t iterations;
	uint32_t hold_time_ms;
	beken_thread_t thread;
} hspl_stress_test_t;

static hspl_stress_test_t s_stress_test_cpu0 = {0};
static hspl_stress_test_t s_stress_test_cpu2 = {0};

/* TEST-domain IPI events for stress test coordination */
typedef enum {
	HSPL_STRESS_IPI_EVENT_START = 1,
	HSPL_STRESS_IPI_EVENT_STOP,
	HSPL_STRESS_IPI_EVENT_DONE,
	HSPL_STRESS_IPI_EVENT_AUTO,
} hspl_stress_ipi_event_t;

#define HSPL_STRESS_IPI_PAYLOAD_NONE 0

static bk_err_t hspl_stress_ipi_send(ipi_core_id_t target_core, hspl_stress_ipi_event_t event)
{
	return bk_ipi_send_domain(target_core, IPI_DOMAIN_TEST, (uint8_t)event,
		HSPL_STRESS_IPI_PAYLOAD_NONE);
}

static void hspl_stress_test_worker(void *param)
{
	hspl_stress_test_t *test = (hspl_stress_test_t *)param;
	uint32_t core_id = rtos_get_core_id();
	uint32_t start_ms, end_ms;
	uint8_t owner;
	bk_err_t ret;

	HSPL_TEST_LOGI("Stress test worker started on core %u\r\n", core_id);

	/* Wait for start signal */
	while (!test->start_flag) {
		rtos_delay_milliseconds(10);
	}

	HSPL_TEST_LOGI("Core %u: Starting stress test (hspl_id=%u ch=%u iter=%u hold=%ums)\r\n",
	               core_id, test->hspl_id, test->channel, test->iterations, test->hold_time_ms);

	start_ms = rtos_get_time();

	for (uint32_t i = 0; i < test->iterations && test->running; i++) {
		/* Try lock */
		ret = bk_hspl_try_lock(test->hspl_id, test->channel, &owner);
		if (ret == BK_OK) {
			test->lock_success++;
			/* Hold lock for specified time */
			rtos_delay_milliseconds(test->hold_time_ms);
			/* Unlock */
			ret = bk_hspl_unlock(test->hspl_id, test->channel);
			if (ret == BK_OK) {
				test->unlock_success++;
			} else {
				test->unlock_fail++;
				HSPL_TEST_LOGE("Core %u: Unlock failed at iter %u\r\n", core_id, i);
			}
		} else {
			test->lock_fail++;
			if (owner != 0xFF) {
				/* Locked by other core, this is expected in stress test */
			}
		}

		/* Small delay between iterations */
		rtos_delay_milliseconds(1);
	}

	end_ms = rtos_get_time();

	HSPL_TEST_LOGI("Core %u: Stress test completed in %ums\r\n", core_id, end_ms - start_ms);
	HSPL_TEST_LOGI("Core %u: lock_success=%u lock_fail=%u unlock_success=%u unlock_fail=%u\r\n",
	               core_id, test->lock_success, test->lock_fail, test->unlock_success, test->unlock_fail);

	/* Send done signal via IPI */
	if (core_id == CPU0_CORE_ID) {
		hspl_stress_ipi_send(IPI_AP_CORE0, HSPL_STRESS_IPI_EVENT_DONE);
	} else if (core_id == CPU2_CORE_ID) {
		hspl_stress_ipi_send(IPI_CP_CORE0, HSPL_STRESS_IPI_EVENT_DONE);
	}

	test->running = false;
	test->thread = NULL;  /* Clear thread handle */

	/* Properly exit the task */
	rtos_delete_thread(NULL);
}

static void hspl_stress_ipi_callback(ipi_core_id_t core_id, uint32_t value,
	uint8_t src_cpu, uint8_t event, uint16_t payload, void *param)
{
	(void)core_id;
	(void)value;
	(void)src_cpu;
	(void)payload;
	(void)param;
	hspl_stress_test_t *test = NULL;
	uint32_t my_core_id = rtos_get_core_id();

	if (my_core_id == CPU0_CORE_ID) {
		test = &s_stress_test_cpu0;
	} else if (my_core_id == CPU2_CORE_ID) {
		test = &s_stress_test_cpu2;
	} else {
		return;
	}

	if (event == HSPL_STRESS_IPI_EVENT_START) {
		HSPL_TEST_LOGI("Core %u: Received START signal\r\n", my_core_id);
		test->start_flag = true;
	} else if (event == HSPL_STRESS_IPI_EVENT_STOP) {
		HSPL_TEST_LOGI("Core %u: Received STOP signal\r\n", my_core_id);
		test->running = false;
	} else if (event == HSPL_STRESS_IPI_EVENT_AUTO) {
		HSPL_TEST_LOGI("Core %u: Received AUTO signal, starting test\r\n", my_core_id);
		if (test->running) {
			test->start_flag = true;
		}
	} else if (event == HSPL_STRESS_IPI_EVENT_DONE) {
		HSPL_TEST_LOGI("Core %u: Received DONE signal\r\n", my_core_id);
	}
}

static void cli_hspl_help(void)
{
	CLI_LOGD("hspl_driver {init|deinit}\r\n");
	CLI_LOGD("hspl lock {hspl_id} {ch}         - Try lock (hspl_id:0/1 ch:0-15)\r\n");
	CLI_LOGD("hspl unlock {hspl_id} {ch}       - Unlock (hspl_id:0/1 ch:0-15)\r\n");
	CLI_LOGD("hspl state {hspl_id} {ch|all}    - Read STA (hspl_id:0/1)\r\n");
	CLI_LOGD("  compatible: hspl lock {ch} / hspl unlock {ch} / hspl state {ch|all} (default hspl_id=0)\r\n");
	CLI_LOGD("hspl timeout_cfg {hspl_id} {ch} {th} {en} - th: cycles, en: 0/1\r\n");
	CLI_LOGD("hspl timeout_irq {hspl_id} {enable|disable|clear}\r\n");
	CLI_LOGD("hspl raw_sta {hspl_id} {ch}      - Read STA raw\r\n");
	CLI_LOGD("hspl raw_lock {ch}               - Read LOCK raw on HSPL_ID_0 (NOTE: reading LOCK triggers lock attempt)\r\n");
	CLI_LOGD("hspl res_lock {flash|clock|sys_sw_regs|uart_log|os|user1|user2} {timeout_us}\r\n");
	CLI_LOGD("hspl res_must_lock {flash|clock|sys_sw_regs|uart_log|os|user1|user2}\r\n");
	CLI_LOGD("hspl res_unlock {flash|clock|sys_sw_regs|uart_log|os|user1|user2}\r\n");
	CLI_LOGD("hspl stress {hspl_id} {ch} {iter} {hold_ms} - Parallel stress test (CPU0 vs CPU2)\r\n");
	CLI_LOGD("hspl stress_auto {hspl_id} {ch} {iter} {hold_ms} - Auto parallel stress test (auto start on CPU0 & CPU2)\r\n");
	CLI_LOGD("hspl stress_stop                  - Stop stress test\r\n");
	CLI_LOGD("hspl stress_stat                  - Show stress test statistics\r\n");
}

static bool cli_hspl_parse_res(const char *name, bk_hspl_res_t *res)
{
	if (!name || !res) {
		return false;
	}

	if (os_strcmp(name, "flash") == 0) *res = BK_HSPL_RES_FLASH;
	else if (os_strcmp(name, "clock") == 0) *res = BK_HSPL_RES_CLOCK;
	else if (os_strcmp(name, "sys_sw_regs") == 0) *res = BK_HSPL_RES_SYS_SW_REGS;
	else if (os_strcmp(name, "uart_log") == 0) *res = BK_HSPL_RES_UART_LOG;
	else if (os_strcmp(name, "os") == 0) *res = BK_HSPL_RES_OS;
	else if (os_strcmp(name, "user1") == 0) *res = BK_HSPL_RES_USER1;
	else if (os_strcmp(name, "user2") == 0) *res = BK_HSPL_RES_USER2;
	else return false;

	return true;
}

static void cli_hspl_driver_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	(void)pcWriteBuffer;
	(void)xWriteBufferLen;

	if (argc < 2) {
		cli_hspl_help();
		return;
	}

	if (os_strcmp(argv[1], "init") == 0) {
		BK_LOG_ON_ERR(bk_hspl_driver_init());
		BK_LOG_ON_ERR(bk_hspl_register_timeout_callback(BK_HSPL_ID_0,
		                                                hspl_timeout_cb,
		                                                (void *)(uintptr_t)0x4853504C)); /* 'HSPL' */
		CLI_LOGD("HSPL driver initialized\r\n");
	} else if (os_strcmp(argv[1], "deinit") == 0) {
		BK_LOG_ON_ERR(bk_hspl_driver_deinit());
		CLI_LOGD("HSPL driver deinitialized\r\n");
	} else {
		cli_hspl_help();
	}
}

static void cli_hspl_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	(void)pcWriteBuffer;
	(void)xWriteBufferLen;

	if (argc < 2) {
		cli_hspl_help();
		return;
	}

	if (os_strcmp(argv[1], "lock") == 0) {
		bk_hspl_id_t hspl_id = BK_HSPL_ID_0;
		uint8_t ch;

		if (argc < 3) {
			CLI_LOGE("Usage: hspl lock {hspl_id} {ch}\r\n");
			return;
		}

		if (argc >= 4) {
			hspl_id = (bk_hspl_id_t)os_strtoul(argv[2], NULL, 10);
			ch = (uint8_t)os_strtoul(argv[3], NULL, 10);
		} else {
			/* compatible: hspl lock {ch} -> default hspl_id=0 */
			ch = (uint8_t)os_strtoul(argv[2], NULL, 10);
		}

		uint8_t owner = 0xFF;
		bk_err_t ret = bk_hspl_try_lock(hspl_id, ch, &owner);
		if (ret == BK_OK) {
			CLI_LOGD("HSPL lock ok: hspl_id=%u ch=%u\r\n", hspl_id, ch);
		} else {
			CLI_LOGE("HSPL lock fail: hspl_id=%u ch=%u owner=%u\r\n", hspl_id, ch, owner);
		}
	} else if (os_strcmp(argv[1], "unlock") == 0) {
		bk_hspl_id_t hspl_id = BK_HSPL_ID_0;
		uint8_t ch;

		if (argc < 3) {
			CLI_LOGE("Usage: hspl unlock {hspl_id} {ch}\r\n");
			return;
		}

		if (argc >= 4) {
			hspl_id = (bk_hspl_id_t)os_strtoul(argv[2], NULL, 10);
			ch = (uint8_t)os_strtoul(argv[3], NULL, 10);
		} else {
			/* compatible: hspl unlock {ch} -> default hspl_id=0 */
			ch = (uint8_t)os_strtoul(argv[2], NULL, 10);
		}

		bk_err_t ret = bk_hspl_unlock(hspl_id, ch);
		if (ret == BK_OK) {
			CLI_LOGD("HSPL unlock ok: hspl_id=%u ch=%u\r\n", hspl_id, ch);
		} else {
			CLI_LOGE("HSPL unlock fail: hspl_id=%u ch=%u\r\n", hspl_id, ch);
		}
	} else if (os_strcmp(argv[1], "state") == 0) {
		bk_hspl_id_t hspl_id = BK_HSPL_ID_0;
		const char *arg_ch;

		if (argc < 3) {
			CLI_LOGE("Usage: hspl state {hspl_id} {ch|all}\r\n");
			return;
		}

		if (argc >= 4) {
			hspl_id = (bk_hspl_id_t)os_strtoul(argv[2], NULL, 10);
			arg_ch = argv[3];
		} else {
			/* compatible: hspl state {ch|all} -> default hspl_id=0 */
			arg_ch = argv[2];
		}

		if (os_strcmp(arg_ch, "all") == 0) {
			for (uint8_t ch = 0; ch < HSPL_CHANNEL_MAX; ch++) {
				hspl_state_t st = {0};
				if (bk_hspl_get_state(hspl_id, ch, &st) == BK_OK) {
					CLI_LOGD("hspl_id=%u ch=%u locked=%u owner_valid=%u owner_id=%u\r\n",
					         hspl_id, ch, st.locked, st.owner_valid, st.owner_id);
				} else {
					CLI_LOGE("hspl_id=%u ch=%u state read fail\r\n", hspl_id, ch);
				}
			}
		} else {
			uint8_t ch = (uint8_t)os_strtoul(arg_ch, NULL, 10);
			hspl_state_t st = {0};
			if (bk_hspl_get_state(hspl_id, ch, &st) == BK_OK) {
				CLI_LOGD("hspl_id=%u ch=%u locked=%u owner_valid=%u owner_id=%u\r\n",
				         hspl_id, ch, st.locked, st.owner_valid, st.owner_id);
			} else {
				CLI_LOGE("hspl_id=%u ch=%u state read fail\r\n", hspl_id, ch);
			}
		}
	} else if (os_strcmp(argv[1], "timeout_cfg") == 0) {
		if (argc < 6) {
			CLI_LOGE("Usage: hspl timeout_cfg {hspl_id} {ch} {th_cycles} {en}\r\n");
			return;
		}
		bk_hspl_id_t hspl_id = (bk_hspl_id_t)os_strtoul(argv[2], NULL, 10);
		uint8_t ch = (uint8_t)os_strtoul(argv[3], NULL, 10);
		uint32_t th = os_strtoul(argv[4], NULL, 0);
		bool en = (bool)os_strtoul(argv[5], NULL, 10);
		bk_err_t ret = bk_hspl_timeout_config(hspl_id, ch, th, en);
		if (ret == BK_OK) {
			CLI_LOGD("timeout_cfg ok: hspl_id=%u ch=%u th=%u en=%u\r\n", hspl_id, ch, th, en ? 1 : 0);
		} else {
			CLI_LOGE("timeout_cfg fail\r\n");
		}
	} else if (os_strcmp(argv[1], "timeout_irq") == 0) {
		if (argc < 4) {
			CLI_LOGE("Usage: hspl timeout_irq {hspl_id} {enable|disable|clear}\r\n");
			return;
		}
		bk_hspl_id_t hspl_id = (bk_hspl_id_t)os_strtoul(argv[2], NULL, 10);
		if (os_strcmp(argv[3], "enable") == 0) {
			BK_LOG_ON_ERR(bk_hspl_timeout_irq_enable(hspl_id, true));
			CLI_LOGD("timeout_irq enabled\r\n");
		} else if (os_strcmp(argv[3], "disable") == 0) {
			BK_LOG_ON_ERR(bk_hspl_timeout_irq_enable(hspl_id, false));
			CLI_LOGD("timeout_irq disabled\r\n");
		} else if (os_strcmp(argv[3], "clear") == 0) {
			bk_hspl_timeout_irq_clear(hspl_id);
			CLI_LOGD("timeout_irq cleared\r\n");
		} else {
			cli_hspl_help();
		}
	} else if (os_strcmp(argv[1], "raw_sta") == 0) {
		bk_hspl_id_t hspl_id = BK_HSPL_ID_0;
		uint8_t ch;

		if (argc < 3) {
			CLI_LOGE("Usage: hspl raw_sta {hspl_id} {ch}\r\n");
			return;
		}

		if (argc >= 4) {
			hspl_id = (bk_hspl_id_t)os_strtoul(argv[2], NULL, 10);
			ch = (uint8_t)os_strtoul(argv[3], NULL, 10);
		} else {
			ch = (uint8_t)os_strtoul(argv[2], NULL, 10);
		}

		uint32_t v = bk_hspl_read_sta_raw(hspl_id, ch);
		CLI_LOGD("STA hspl_id=%u ch=%u val=0x%08X\r\n", hspl_id, ch, v);
	} else if (os_strcmp(argv[1], "raw_lock") == 0) {
		if (argc < 3) {
			CLI_LOGE("Usage: hspl raw_lock {ch}\r\n");
			return;
		}
		uint8_t ch = (uint8_t)os_strtoul(argv[2], NULL, 10);
		uint32_t v = bk_hspl_read_lock_raw(BK_HSPL_ID_0, ch);
		CLI_LOGD("LOCK[%u]=0x%08X\r\n", ch, v);
	} else if (os_strcmp(argv[1], "res_lock") == 0) {
		if (argc < 4) {
			CLI_LOGE("Usage: hspl res_lock {flash|clock|sys_sw_regs|uart_log|os|user1|user2} {timeout_us}\r\n");
			return;
		}
		bk_hspl_res_t res = BK_HSPL_RES_MAX;
		if (!cli_hspl_parse_res(argv[2], &res)) {
			CLI_LOGE("unknown res\r\n");
			return;
		}
		uint32_t to_us = os_strtoul(argv[3], NULL, 0);
		bk_err_t ret = bk_hspl_res_lock(res, to_us);
		if (ret == BK_OK) {
			uint8_t hspl_id, ch;
			bk_hspl_res_get_map(res, &hspl_id, &ch);
			CLI_LOGD("res_lock ok: %s (hspl_id=%u ch=%u)\r\n", argv[2], hspl_id, ch);
		} else {
			CLI_LOGE("res_lock fail: %d\r\n", ret);
		}
	} else if (os_strcmp(argv[1], "res_must_lock") == 0) {
		if (argc < 3) {
			CLI_LOGE("Usage: hspl res_must_lock {flash|clock|sys_sw_regs|uart_log|os|user1|user2}\r\n");
			return;
		}
		bk_hspl_res_t res = BK_HSPL_RES_MAX;
		if (!cli_hspl_parse_res(argv[2], &res)) {
			CLI_LOGE("unknown res\r\n");
			return;
		}
		bk_err_t ret = bk_hspl_res_must_lock(res);
		if (ret == BK_OK) {
			uint8_t hspl_id, ch;
			bk_hspl_res_get_map(res, &hspl_id, &ch);
			CLI_LOGD("res_must_lock ok: %s (hspl_id=%u ch=%u)\r\n", argv[2], hspl_id, ch);
		} else {
			CLI_LOGE("res_must_lock fail: %d\r\n", ret);
		}
	} else if (os_strcmp(argv[1], "res_unlock") == 0) {
		if (argc < 3) {
			CLI_LOGE("Usage: hspl res_unlock {flash|clock|sys_sw_regs|uart_log|os|user1|user2}\r\n");
			return;
		}
		bk_hspl_res_t res = BK_HSPL_RES_MAX;
		if (!cli_hspl_parse_res(argv[2], &res)) {
			CLI_LOGE("unknown res\r\n");
			return;
		}
		bk_err_t ret = bk_hspl_res_unlock(res);
		if (ret == BK_OK) {
			CLI_LOGD("res_unlock ok: %s\r\n", argv[2]);
		} else {
			CLI_LOGE("res_unlock fail: %d\r\n", ret);
		}
	} else if (os_strcmp(argv[1], "stress") == 0) {
		if (argc < 6) {
			CLI_LOGE("Usage: hspl stress {hspl_id} {ch} {iter} {hold_ms}\r\n");
			CLI_LOGE("  Example: hspl stress 0 0 1000 10\r\n");
			return;
		}
		bk_hspl_id_t hspl_id = (bk_hspl_id_t)os_strtoul(argv[2], NULL, 10);
		uint8_t ch = (uint8_t)os_strtoul(argv[3], NULL, 10);
		uint32_t iter = os_strtoul(argv[4], NULL, 10);
		uint32_t hold_ms = os_strtoul(argv[5], NULL, 10);

		uint32_t my_core_id = rtos_get_core_id();
		hspl_stress_test_t *test = NULL;
		ipi_core_id_t target_core;

		/* Initialize IPI if not already done */
		if (bk_ipi_driver_init() != BK_OK) {
			CLI_LOGE("IPI driver init failed\r\n");
			return;
		}

		/* Register IPI callback for stress test coordination */
		if (my_core_id == CPU0_CORE_ID) {
			test = &s_stress_test_cpu0;
			target_core = IPI_AP_CORE0;
			bk_ipi_register_domain_callback(IPI_DOMAIN_TEST, hspl_stress_ipi_callback, NULL);
			bk_ipi_enable(IPI_CP_CORE0);
		} else if (my_core_id == CPU2_CORE_ID) {
			test = &s_stress_test_cpu2;
			target_core = IPI_CP_CORE0;
			bk_ipi_register_domain_callback(IPI_DOMAIN_TEST, hspl_stress_ipi_callback, NULL);
			bk_ipi_enable(IPI_AP_CORE0);
		} else {
			CLI_LOGE("Stress test only supports CPU0 or CPU2\r\n");
			return;
		}

		/* Stop any running test */
		if (test->running) {
			test->running = false;
			rtos_delay_milliseconds(100);
		}

		/* Initialize test parameters */
		memset((void *)test, 0, sizeof(*test));
		test->hspl_id = hspl_id;
		test->channel = ch;
		test->iterations = iter;
		test->hold_time_ms = hold_ms;
		test->running = true;
		test->start_flag = false;

		CLI_LOGD("Starting stress test on core %u: hspl_id=%u ch=%u iter=%u hold=%ums\r\n",
		         my_core_id, hspl_id, ch, iter, hold_ms);

		/* Create worker thread on current core */
		bk_err_t ret;
		ret = rtos_create_thread(&test->thread, 5, "hspl_stress_cpu0",
								hspl_stress_test_worker, 2048, test);

		if (ret != BK_OK) {
			CLI_LOGE("Failed to create stress test thread: %d\r\n", ret);
			test->running = false;
			return;
		}

		/* Wait a bit for thread to start */
		rtos_delay_milliseconds(50);

		/* Send start signal to other core */
		hspl_stress_ipi_send(target_core, HSPL_STRESS_IPI_EVENT_START);
		test->start_flag = true;

		CLI_LOGD("Stress test started. Use 'hspl stress_stat' to check progress.\r\n");
	} else if (os_strcmp(argv[1], "stress_auto") == 0) {
		/* Auto stress test: automatically start on both CPU0 and CPU2 */
		if (argc < 6) {
			CLI_LOGE("Usage: hspl stress_auto {hspl_id} {ch} {iter} {hold_ms}\r\n");
			CLI_LOGE("  Example: hspl stress_auto 0 0 1000 10\r\n");
			CLI_LOGE("  This will automatically start stress test on CPU0 (CP M52) and CPU2 (AP M55)\r\n");
			return;
		}
		bk_hspl_id_t hspl_id = (bk_hspl_id_t)os_strtoul(argv[2], NULL, 10);
		uint8_t ch = (uint8_t)os_strtoul(argv[3], NULL, 10);
		uint32_t iter = os_strtoul(argv[4], NULL, 10);
		uint32_t hold_ms = os_strtoul(argv[5], NULL, 10);

		/* Validate parameters */
		if (hspl_id >= BK_HSPL_ID_MAX) {
			CLI_LOGE("Invalid hspl_id: %u (must be 0 or 1)\r\n", hspl_id);
			return;
		}
		if (ch >= HSPL_CHANNEL_MAX) {
			CLI_LOGE("Invalid channel: %u (must be 0-%u)\r\n", ch, HSPL_CHANNEL_MAX - 1);
			return;
		}

		uint32_t my_core_id = rtos_get_core_id();
		
		/* Initialize IPI if not already done */
		if (bk_ipi_driver_init() != BK_OK) {
			CLI_LOGE("IPI driver init failed\r\n");
			return;
		}

		CLI_LOGD("Auto stress test: hspl_id=%u ch=%u iter=%u hold=%ums\r\n", hspl_id, ch, iter, hold_ms);
		CLI_LOGD("Current core: %u, will start on CPU0 (CP M52) and CPU2 (AP M55)\r\n", my_core_id);

		/* Setup test parameters for both CPUs */
		hspl_stress_test_t *test_cpu0 = &s_stress_test_cpu0;
		hspl_stress_test_t *test_cpu2 = &s_stress_test_cpu2;

		/* Stop any running tests */
		if (test_cpu0->running) {
			test_cpu0->running = false;
			rtos_delay_milliseconds(100);
		}
		if (test_cpu2->running) {
			test_cpu2->running = false;
			rtos_delay_milliseconds(100);
		}

		/* Initialize test parameters for CPU0 */
		memset((void *)test_cpu0, 0, sizeof(*test_cpu0));
		test_cpu0->hspl_id = hspl_id;
		test_cpu0->channel = ch;
		test_cpu0->iterations = iter;
		test_cpu0->hold_time_ms = hold_ms;
		test_cpu0->running = true;
		test_cpu0->start_flag = false;

		/* Initialize test parameters for CPU2 */
		memset((void *)test_cpu2, 0, sizeof(*test_cpu2));
		test_cpu2->hspl_id = hspl_id;
		test_cpu2->channel = ch;
		test_cpu2->iterations = iter;
		test_cpu2->hold_time_ms = hold_ms;
		test_cpu2->running = true;
		test_cpu2->start_flag = false;

		/* Register IPI callbacks on both CPUs */
		bk_ipi_register_domain_callback(IPI_DOMAIN_TEST, hspl_stress_ipi_callback, NULL);
		bk_ipi_enable(IPI_CP_CORE0);
		bk_ipi_enable(IPI_AP_CORE0);

		/* Create worker threads on both CPUs */
		bk_err_t ret;
		
		/* Start CPU0 worker (if we're on CPU0 or can create thread on CPU0) */
		if (my_core_id == CPU0_CORE_ID) {
			ret = rtos_create_thread(&test_cpu0->thread, 5, "hspl_stress_cpu0",
			                        hspl_stress_test_worker, 2048, test_cpu0);
			if (ret != BK_OK) {
				CLI_LOGE("Failed to create CPU0 stress test thread: %d\r\n", ret);
				test_cpu0->running = false;
			}
		} else {
			/* Send IPI to CPU0 to start its worker */
			CLI_LOGD("Sending IPI to CPU0 to start stress test...\r\n");
			hspl_stress_ipi_send(IPI_CP_CORE0, HSPL_STRESS_IPI_EVENT_AUTO);
		}

		/* Start CPU2 worker (if we're on CPU2 or can create thread on CPU2) */
		if (my_core_id == CPU2_CORE_ID) {
			/* On CPU2, create thread will run on current CPU (CPU2) */
			ret = rtos_create_thread(&test_cpu2->thread, 5, "hspl_stress_cpu2",
			                        hspl_stress_test_worker, 2048, test_cpu2);
			if (ret != BK_OK) {
				CLI_LOGE("Failed to create CPU2 stress test thread: %d\r\n", ret);
				test_cpu2->running = false;
			}
		} else {
			/* Send IPI to CPU2 to start its worker */
			CLI_LOGD("Sending IPI to CPU2 to start stress test...\r\n");
			hspl_stress_ipi_send(IPI_AP_CORE0, HSPL_STRESS_IPI_EVENT_AUTO);
		}

		/* Wait a bit for threads to start */
		rtos_delay_milliseconds(100);

		/* Send start signals to both CPUs */
		CLI_LOGD("Sending START signals to both CPUs...\r\n");
		hspl_stress_ipi_send(IPI_CP_CORE0, HSPL_STRESS_IPI_EVENT_START);
		hspl_stress_ipi_send(IPI_AP_CORE0, HSPL_STRESS_IPI_EVENT_START);
		
		/* Also set start flag locally if we're on CPU0 or CPU2 */
		if (my_core_id == CPU0_CORE_ID) {
			test_cpu0->start_flag = true;
		}
		if (my_core_id == CPU2_CORE_ID) {
			test_cpu2->start_flag = true;
		}

		CLI_LOGD("Auto stress test started on CPU0 and CPU2. Use 'hspl stress_stat' to check progress.\r\n");
		CLI_LOGD("Note: Run 'hspl stress_stat' on CPU0 and CPU2 separately to see each core's statistics.\r\n");
	} else if (os_strcmp(argv[1], "stress_stop") == 0) {
		uint32_t my_core_id = rtos_get_core_id();
		hspl_stress_test_t *test = NULL;
		ipi_core_id_t target_core;

		if (my_core_id == CPU0_CORE_ID) {
			test = &s_stress_test_cpu0;
			target_core = IPI_AP_CORE0;
		} else if (my_core_id == CPU2_CORE_ID) {
			test = &s_stress_test_cpu2;
			target_core = IPI_CP_CORE0;
		} else {
			CLI_LOGE("Stress test only supports CPU0 or CPU2\r\n");
			return;
		}

		if (test->running) {
			test->running = false;
			hspl_stress_ipi_send(target_core, HSPL_STRESS_IPI_EVENT_STOP);
			CLI_LOGD("Stop signal sent. Waiting for threads to finish...\r\n");
			rtos_delay_milliseconds(500);
		} else {
			CLI_LOGD("No stress test running\r\n");
		}
	} else if (os_strcmp(argv[1], "stress_stat") == 0) {
		uint32_t my_core_id = rtos_get_core_id();
		hspl_stress_test_t *test = NULL;

		if (my_core_id == CPU0_CORE_ID) {
			test = &s_stress_test_cpu0;
		} else if (my_core_id == CPU2_CORE_ID) {
			test = &s_stress_test_cpu2;
		} else {
			CLI_LOGE("Stress test only supports CPU0 or CPU2\r\n");
			return;
		}

		CLI_LOGD("=== Stress Test Statistics (Core %u) ===\r\n", my_core_id);
		CLI_LOGD("Running: %s\r\n", test->running ? "Yes" : "No");
		CLI_LOGD("Lock Success: %u\r\n", test->lock_success);
		CLI_LOGD("Lock Fail: %u\r\n", test->lock_fail);
		CLI_LOGD("Unlock Success: %u\r\n", test->unlock_success);
		CLI_LOGD("Unlock Fail: %u\r\n", test->unlock_fail);
		CLI_LOGD("Timeout Count: %u\r\n", test->timeout_count);
		if (test->lock_success + test->lock_fail > 0) {
			uint32_t total = test->lock_success + test->lock_fail;
			CLI_LOGD("Success Rate: %u%%\r\n", (test->lock_success * 100) / total);
		}
	} else {
		cli_hspl_help();
	}
}

#define HSPL_TIME_TEST_COUNT 1000
static void cli_hspl_time_cmd(char *pcWriteBuffer, int xWriteBufferLen,
    int argc, char **argv)
{
    dwt_init_cycle_counter();
    dwt_enable_cycle_counter();

    uint32_t flags = rtos_disable_int();
    uint32_t t_start = dwt_get_cycle_counter_val();
    for (int32_t i = 0; i < HSPL_TIME_TEST_COUNT; i++) {
        bk_hspl_read_lock_raw(BK_HSPL_ID_0, 0);
        bk_hspl_unlock(BK_HSPL_ID_0, 0);
    }
    uint32_t t_end = dwt_get_cycle_counter_val();
    rtos_enable_int(flags);

    HSPL_TEST_LOGI("hspl lock+unlock x%d total cycles: %u\r\n",
                   HSPL_TIME_TEST_COUNT, t_end - t_start);
}


#if CONFIG_HSPL_TEST
DRV_CLI_CMD_EXPORT static const struct cli_command s_hspl_commands[] = {
	{"hspl_driver", "{init|deinit}", cli_hspl_driver_cmd},
	{"hspl", "hspl {lock|unlock|state|timeout_cfg|timeout_irq|raw_sta|raw_lock|res_lock|res_unlock|stress|stress_auto|stress_stop|stress_stat} [...]", cli_hspl_cmd},
	{"hspl_time", "hspl_time", cli_hspl_time_cmd},
};
#endif
