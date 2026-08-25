// Copyright 2020-2022 Beken
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

#include <stdio.h>
#include <string.h>

#include <os/os.h>
#include "mb_ipc_cmd.h"
#include <modules/pm.h>
#include "bk_wdt.h"
#include <wdt_driver.h>
#if CONFIG_SLAVE_HEART_BEAT_USE_IPI
#include <driver/ipi_driver.h>
#endif

/* P0-2: the AP-memory trap dump is offloaded from the mailbox RX callback to
 * this (highest-priority) task so the IPC ACK returns promptly. */
extern void bk_coredump_dump_ap_memory_for_trap(void);
extern volatile uint32_t g_ap_dump_flag;
/* Defined later in this file; the strong override of the weak stub in
 * mb_ipc_cmd.c. Forward-declared for the mb_ipc_task() call site below. */
void mb_ipc_dump_notify(u32 cpu_id, u32 dump);

#define MOD_TAG		"hrt"
#define BEKEN_HEARTBEAT_PRIORITY 0

#if (CONFIG_CPU_CNT > 1)

/* define the code section will be compiled. */
#define MASTER_HB_TASK


#if !defined(MASTER_HB_TASK)
void mb_ipc_reset_notify(u32 cpu_id, u32 power_on)
{
	(void)cpu_id;
	(void)power_on;
}
int mb_ipc_cpu_is_power_on(u32 cpu_id)
{
	(void)cpu_id;

	return 0;
}
int mb_ipc_cpu_is_power_off(u32 cpu_id)
{
	(void)cpu_id;

	return 1;
}
#endif

#if defined(MASTER_HB_TASK)

#include <os/rtos_ext.h>

#define MB_IPC_START_CORE_FLAG		0x01
#define MB_IPC_STOP_CORE_FLAG		0x02
#define MB_IPC_POWER_UP_FLAG		0x04
#define MB_IPC_HEARTBEAT_FLAG		0x08
#define MB_IPC_AP_DUMP_FLAG			0x10

#define MB_IPC_ALL_FLAGS			(MB_IPC_START_CORE_FLAG | MB_IPC_STOP_CORE_FLAG | MB_IPC_POWER_UP_FLAG | MB_IPC_HEARTBEAT_FLAG | MB_IPC_AP_DUMP_FLAG)

#define MB_IPC_HEARTBEAT_TIME       2000   /* slave sends heartbeat every 2s */
#define MB_IPC_HEARTBEAT_IPI_EVENT_POWER_UP     1
#define MB_IPC_HEARTBEAT_IPI_EVENT_HEARTBEAT    2
#if CONFIG_WDT_EN
#define HB_TIMEOUT_MS               CONFIG_INT_WDT_PERIOD_MS
#else
#define HB_TIMEOUT_MS               (MB_IPC_HEARTBEAT_TIME * 3)  /* 6s: allow 3 missed heartbeats */
#endif

enum
{
	CORE_POWER_OFF = 0,
	CORE_STARTING,
	CORE_POWER_ON,
};
typedef enum
{
	MB_IPC_ENTER_LV = 0,
	MB_IPC_EXIT_LV,
	MB_IPC_WORKING,
}mb_ipc_work_state_e;

static rtos_event_ext_t             mb_ipc_heart_event;
static u32                          cpu_x_heartbeat_timestamp = 0;
static volatile u8                  cpu_x_state = CORE_POWER_OFF;
static volatile u8                  cpu_x_id = 0xFF;   /* invalid ID, */
static volatile u8                  cpu_x_dump = 0;
static volatile u8                  cpu_x_dump_pending_id = 0xFF;
static volatile u8                  cpu_x_heartbeat_timeout = 0;
static volatile mb_ipc_work_state_e s_mb_ipc_work_state = MB_IPC_WORKING;

void mb_ipc_heartbeat_notify(u32 cpu_id);
void mb_ipc_power_on_notify(u32 cpu_id);

extern void start_cpu1_core(void);
extern void stop_cpu1_core(void);
extern void start_cpu2_core(void);
extern void stop_cpu2_core(void);


int bk_ipc_heartbeat_is_timeout(void)
{
	return cpu_x_heartbeat_timeout;
}

void bk_ipc_heartbeat_get_status(u8 *state, u32 *last_ts, u32 *cur_ts, u8 *cpu_id)
{
	if(state)  *state  = cpu_x_state;
	if(last_ts) *last_ts = cpu_x_heartbeat_timestamp;
	if(cur_ts)  *cur_ts  = (u32)rtos_get_time();
	if(cpu_id)  *cpu_id  = cpu_x_id;
}

static int ipc_heartbeat_timeout(void)
{
	u32   cur_time;

	cur_time = (u32)rtos_get_time();

	if(cpu_x_state == CORE_POWER_OFF)
	{
		return 0;
	}
	if((cpu_x_state == CORE_STARTING) || (cpu_x_dump != 0))
	{
		cpu_x_heartbeat_timestamp = cur_time;
		return 0;
	}

	if(cur_time >= cpu_x_heartbeat_timestamp)
	{
		cur_time -= cpu_x_heartbeat_timestamp;
	}
	else
	{
		cur_time += (~(cpu_x_heartbeat_timestamp)) + 1;  // wrap around.
	}

	if(cur_time < HB_TIMEOUT_MS)
	{
		cpu_x_heartbeat_timestamp = (u32)rtos_get_time();
		return 0;
	}

	if (bk_pm_module_lv_sleep_state_get(PM_DEV_ID_DEFAULT))
	{
		cpu_x_heartbeat_timestamp = (u32)rtos_get_time();
		bk_pm_module_lv_sleep_state_clear(PM_DEV_ID_DEFAULT);
		return 0;
	}

	if(s_mb_ipc_work_state == MB_IPC_EXIT_LV)
	{
		cpu_x_heartbeat_timestamp = (u32)rtos_get_time();
		s_mb_ipc_work_state = MB_IPC_WORKING;
		return 0;
	}

	return 1;
}

static void restart_cpu_x(void)
{
	if(cpu_x_id == 1)
	{
		stop_cpu1_core();
		rtos_delay_milliseconds(6);
		start_cpu1_core();
		return;
	}
	
	if(cpu_x_id == 2)
	{
		stop_cpu2_core();
		rtos_delay_milliseconds(6);
		start_cpu2_core();
		return;
	}
}

static int check_cpu_id_ok(u32 cpu_id)
{
	if(cpu_x_id == 0xFF)
	{
		cpu_x_id = cpu_id;
		return 1;
	}

	if(cpu_x_id != cpu_id)
	{
		BK_LOGE(MOD_TAG, "can't manage multiple cpus!\r\n");
		return 0;
	}

	return 1;
}

#if CONFIG_SLAVE_HEART_BEAT_USE_IPI
static void mb_ipc_heartbeat_ipi_callback(ipi_core_id_t core_id, uint32_t value,
	uint8_t src_cpu, uint8_t event, uint16_t payload, void *param)
{
	u32 cpu_id = payload;

	(void)core_id;
	(void)value;
	(void)src_cpu;
	(void)param;

	if (cpu_id == 0) {
		cpu_id = src_cpu;
	}

	if (event == MB_IPC_HEARTBEAT_IPI_EVENT_POWER_UP) {
		mb_ipc_power_on_notify(cpu_id);
	} else if (event == MB_IPC_HEARTBEAT_IPI_EVENT_HEARTBEAT) {
		if (cpu_x_state == CORE_POWER_OFF) {
			mb_ipc_power_on_notify(cpu_id);
		}
		mb_ipc_heartbeat_notify(cpu_id);
	}
}
#endif

static bk_err_t mb_ipc_exit_lv(uint64_t sleep_time, void *args)
{
	cpu_x_heartbeat_timestamp = (u32)rtos_get_time();
	s_mb_ipc_work_state = MB_IPC_EXIT_LV;
#if CONFIG_PM_AP_FAST_BOOT_ENABLE && CONFIG_CP_HANG_DUMP_BY_AP
	extern void bk_cp_hang_debug_heartbeat_lv_exit(void);
	bk_cp_hang_debug_heartbeat_lv_exit();
#endif
	return BK_OK;
}
static bk_err_t mb_ipc_enter_lv(uint64_t sleep_time, void *args)
{
#if CONFIG_PM_AP_FAST_BOOT_ENABLE && CONFIG_CP_HANG_DUMP_BY_AP
	extern void bk_cp_hang_debug_heartbeat_lv_enter(void);
	bk_cp_hang_debug_heartbeat_lv_enter();
#endif
	return BK_OK;
}
static bk_err_t mb_ipc_init_lv_callback()
{
	pm_cb_conf_t enter_config;
	enter_config.cb = (pm_cb)mb_ipc_enter_lv;
	enter_config.args = NULL;

	pm_cb_conf_t exit_config;
	exit_config.cb = (pm_cb)mb_ipc_exit_lv;
	exit_config.args = NULL;

	bk_pm_sleep_register_cb(PM_MODE_LOW_VOLTAGE, PM_DEV_ID_MAILBOX, &enter_config, &exit_config);
	return BK_OK;
}
static void mb_ipc_task( void *para )
{
	bk_err_t	ret_val;
	u32    events;
	u32    check_time = BEKEN_WAIT_FOREVER;

	mb_ipc_init_lv_callback();

	ret_val = rtos_init_event_ex(&mb_ipc_heart_event);

	if(ret_val != BK_OK)
	{
		rtos_delete_thread(NULL);
		return;
	}

#if CONFIG_SLAVE_HEART_BEAT_USE_IPI
	ret_val = bk_ipi_register_domain_callback(IPI_DOMAIN_HEARTBEAT,
		mb_ipc_heartbeat_ipi_callback, NULL);
	if(ret_val != BK_OK)
	{
		BK_LOGE(MOD_TAG, "register heartbeat IPI callback failed: %d\r\n", ret_val);
		rtos_delete_thread(NULL);
		return;
	}
#endif

	while(1)
	{
		events = rtos_wait_event_ex(&mb_ipc_heart_event, MB_IPC_ALL_FLAGS, true, check_time);

		if(events == 0)
		{
			// timeout, so check heartbeat.
			events = MB_IPC_HEARTBEAT_FLAG;
		}

		if(events & MB_IPC_AP_DUMP_FLAG)
		{
			/* P0-2: run the multi-second AP-memory trap dump here (highest
			 * priority task) instead of in the mailbox RX callback, so the IPC
			 * ACK returned promptly and the AP's bounded handoff wait sees a
			 * healthy CP. The board is reset when the dump completes; this does
			 * not return. */
			bk_coredump_dump_ap_memory_for_trap();
			g_ap_dump_flag = 0;
			mb_ipc_dump_notify(cpu_x_dump_pending_id, 0);
			bk_wdt_force_reboot();
		}

		if(events & MB_IPC_STOP_CORE_FLAG)  // process this event at first!!!!
		{
			if(cpu_x_state == CORE_POWER_OFF)
			{
				events = 0;  // clear all events.
			}
		}
		
		if(events & MB_IPC_START_CORE_FLAG)
		{
			u8   retry_cnt = 0;

			while(cpu_x_state == CORE_STARTING)
			{
				ipc_heartbeat_timeout();
				
				if(events & MB_IPC_POWER_UP_FLAG)
				{
					if(cpu_x_state == CORE_STARTING)
					{
						cpu_x_state = CORE_POWER_ON;
						break;  // cpu1 power on. 
					}
				}
				else
				{
					if( retry_cnt > 3)
					{
						BK_DUMP_OUT("IPC retry to start core%d, retry_cnt:%d\r\n", cpu_x_id, retry_cnt);
						// restart_cpu_x();
						break;
					}
					else
					{
						events = rtos_wait_event_ex(&mb_ipc_heart_event, MB_IPC_POWER_UP_FLAG, true, 2000);//2s
					}
				}

				retry_cnt++;
			}

			// discard this event when not in CORE_STARTING state.
		}
		
		if(events & MB_IPC_HEARTBEAT_FLAG)
		{
			if(ipc_heartbeat_timeout())
			{
				BK_LOGE(MOD_TAG, "IPC[%d]heartbeat timeout %d,%d\r\n",cpu_x_id,cpu_x_heartbeat_timestamp,(u32)rtos_get_time());
				/*when cpu1 heartbeat timeout, then system reboot*/
				cpu_x_heartbeat_timeout = 1;
				BK_ASSERT(false);
			}
		}

		if(cpu_x_state == CORE_POWER_OFF)
		{
			check_time = BEKEN_WAIT_FOREVER;
		}
		else
		{
			check_time = HB_TIMEOUT_MS;
		}
	}
}

void mb_ipc_reset_notify(u32 cpu_id, u32 power_on)
{
	if(check_cpu_id_ok(cpu_id) == 0)
	{
		return;
	}
	
	if(power_on)
	{
		if(cpu_x_state != CORE_POWER_ON)
		{
			cpu_x_state = CORE_STARTING;
			rtos_set_event_ex(&mb_ipc_heart_event, MB_IPC_START_CORE_FLAG);
		}
	}
	else
	{
		cpu_x_state = CORE_POWER_OFF;
		rtos_set_event_ex(&mb_ipc_heart_event, MB_IPC_STOP_CORE_FLAG);
	}
}

void mb_ipc_heartbeat_notify(u32 cpu_id)
{
	if(check_cpu_id_ok(cpu_id) == 0)
	{
		return;
	}

#if CONFIG_PM_AP_FAST_BOOT_ENABLE
	/*
	 * A fast-resumed AP continues in the existing heartbeat task, so its
	 * one-shot POWER_UP indication at task entry is not sent again. A regular
	 * heartbeat received while STARTING is equivalent proof that the AP is
	 * alive and must complete the start handshake.
	 */
	if(cpu_x_state == CORE_STARTING)
	{
		rtos_set_event_ex(&mb_ipc_heart_event, MB_IPC_POWER_UP_FLAG);
	}
#endif
	rtos_set_event_ex(&mb_ipc_heart_event, MB_IPC_HEARTBEAT_FLAG);
}

void mb_ipc_power_on_notify(u32 cpu_id)
{
	if(check_cpu_id_ok(cpu_id) == 0)
	{
		return;
	}

	if(cpu_x_state == CORE_POWER_OFF)
	{
		cpu_x_state = CORE_STARTING;
		rtos_set_event_ex(&mb_ipc_heart_event, MB_IPC_START_CORE_FLAG);
	}

	rtos_set_event_ex(&mb_ipc_heart_event, MB_IPC_POWER_UP_FLAG);
}

void mb_ipc_dump_notify(u32 cpu_id, u32 dump)
{
	if(check_cpu_id_ok(cpu_id) == 0)
	{
		return;
	}
	
	cpu_x_dump = (dump != 0);
}

/* P0-2: called from the mailbox RX handler (IPC_AP_TRAP_HANDLE_END). Instead of
 * dumping AP memory synchronously in the RX callback (which delayed the IPC ACK
 * by multiple seconds), just wake this task to perform the dump + reboot. */
void mb_ipc_ap_dump_notify(u32 cpu_id)
{
	if(check_cpu_id_ok(cpu_id) == 0)
	{
		return;
	}

	cpu_x_dump_pending_id = (u8)cpu_id;
	rtos_set_event_ex(&mb_ipc_heart_event, MB_IPC_AP_DUMP_FLAG);
}

int mb_ipc_cpu_is_power_on(u32 cpu_id)
{
	if((cpu_x_id == 0xFF) || (cpu_x_id != cpu_id))
	{
		return 0;
	}
	
	if(cpu_x_state == CORE_POWER_ON)
	{
		return 1;
	}

	return 0;
}

int mb_ipc_cpu_is_power_off(u32 cpu_id)
{
	if((cpu_x_id == 0xFF) || (cpu_x_id != cpu_id))
	{
		return 1;
	}
	
	if(cpu_x_state == CORE_POWER_OFF)
	{
		return 1;
	}

	return 0;
}

#endif

#if defined(SLAVE_HB_TASK)

#define MB_IPC_HEARTBEAT_TIME		2000

static void mb_ipc_task( void *para )
{
	ipc_send_power_up();

	while(1)
	{
		rtos_delay_milliseconds(MB_IPC_HEARTBEAT_TIME);
		ipc_send_heart_beat(0);
	}
}

#endif

bk_err_t mb_ipc_heartbeat_init(void)
{
	bk_err_t	ret_val = BK_FAIL;

#if defined(MASTER_HB_TASK) || defined(SLAVE_HB_TASK)
	ret_val = rtos_create_thread(NULL, BEKEN_HEARTBEAT_PRIORITY, "heartbeat", mb_ipc_task, 1024, 0);
#endif

	if(ret_val != BK_OK)
	{
		BK_LOGE(MOD_TAG, "heartbeat task failed at line %d: %d\r\n", __LINE__, ret_val);
	}

	return ret_val;	
}

#endif

