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

#include "cli.h"

#include <os/os.h>
#include <driver/psram.h>
#include <driver/aon_rtc.h>
#if CONFIG_TRNG_SUPPORT
#include <driver/trng.h>
#endif
#include "bk_general_dma.h"
#include <driver/dma.h>
#include <driver/hpdma.h>
#include "soc/mapping.h"
#include "psram_hal.h"
#include "cache.h"
//#include "bk_sensor_internal.h"

#if (CONFIG_PSRAM_AUTO_DETECT)
#include "bk_ef.h"
#endif

#include "ram_regions.h"

#ifndef CONFIG_PSRAM_TEST_CPU_ADDR
#define CONFIG_PSRAM_TEST_CPU_ADDR       0x60000000
#define CONFIG_PSRAM_TEST_CPU_SIZE       0x00400000
#endif
#ifndef CONFIG_PSRAM_TEST_CPU_DMA_ADDR
#define CONFIG_PSRAM_TEST_CPU_DMA_ADDR   0x60C00000
#define CONFIG_PSRAM_TEST_CPU_DMA_SIZE   0x00100000
#endif
#ifndef CONFIG_PSRAM_TEST_TASK_ADDR
#define CONFIG_PSRAM_TEST_TASK_ADDR      0x60400000
#define CONFIG_PSRAM_TEST_TASK_SIZE      0x00200000
#endif

/* PSRAM test DMA: 1 = HPDMA (default), 0 = GDMA (general_dma). See Kconfig PSRAM_TEST_USE_HPDMA. */
#if !defined(CONFIG_PSRAM_TEST_USE_HPDMA)
#define CONFIG_PSRAM_TEST_USE_HPDMA 1
#endif
/* Use GDMA path only when GDMA chosen and GENERAL_DMA is linked; else use HPDMA to avoid undefined reference. */
#if (CONFIG_PSRAM_TEST_USE_HPDMA) || !(defined(CONFIG_GENERAL_DMA) && CONFIG_GENERAL_DMA)
#define PSRAM_TEST_USE_HPDMA_EFF 1
#else
#define PSRAM_TEST_USE_HPDMA_EFF 0
#endif

#define TEST_PSRAM_ACCURACY     1

#define HARD_NUMBER 0xA55AA55A

#define write_data(addr,val)                 *((volatile uint32_t *)(addr)) = val
#define read_data(addr,val)                  val = *((volatile uint32_t *)(addr))
#define get_addr_data(addr)                  *((volatile uint32_t *)(addr))

#if (CONFIG_ARCH_RISCV)
extern u64 riscv_get_mtimer(void);
#endif

extern bk_err_t bk_psram_init_with_para(uint32_t psram_clk,uint32_t psram_vol);

static void cli_psram_help(void)
{
	CLI_LOGD("psram_test start <cpu|conexist|continue_write|dma|calibrate|new> 0 [delay_ms] [silent 0|1] [data_type] [psram_id 0|1]\r\n");
	CLI_LOGD("psram_test stop\r\n");
	CLI_LOGD("  - psram_id: 0=PSRAM0(base=SOC_PSRAM_DATA_BASE), 1=PSRAM1(base=SOC_PSRAM1_DATA_BASE if defined, else fallback to SOC_QSPI0_DATA_BASE)\r\n");
	CLI_LOGD("psram_test_ext cpu_dma_verify [half_size_kb]  -- CPU write + DMA copy + CPU verify, runs until stop (region: 0x%08x, %uKB)\r\n",
		CONFIG_PSRAM_TEST_CPU_DMA_ADDR, CONFIG_PSRAM_TEST_CPU_DMA_SIZE / 1024);
	CLI_LOGD("psram_test_ext cpu_dma_verify_stop            -- stop cpu_dma_verify\r\n");
	CLI_LOGD("psram_test_ext stack_stress_start             -- PSRAM-stack parent+10 workers, runs until stop\r\n");
	CLI_LOGD("psram_test_ext stack_stress_stop              -- stop stack_stress test\r\n");
	CLI_LOGD("psram_test_ext psram_soak_start [soak_ms]     -- write-soak-readback bit-flip test (region: 0x%08x, %uKB, default soak=30000ms)\r\n",
		CONFIG_PSRAM_TEST_TASK_ADDR, CONFIG_PSRAM_TEST_TASK_SIZE / 1024);
	CLI_LOGD("psram_test_ext psram_soak_stop                -- stop psram_soak test\r\n");
	CLI_LOGD("psram_test_ext psram_rapid_start              -- fast write-then-read pattern verify (region: 0x%08x, %uKB)\r\n",
		CONFIG_PSRAM_TEST_TASK_ADDR, CONFIG_PSRAM_TEST_TASK_SIZE / 1024);
	CLI_LOGD("psram_test_ext psram_rapid_stop               -- stop psram_rapid test\r\n");
}

static inline uint32_t psram_test_get_base_addr(uint32_t psram_id)
{
	if (psram_id == 0) {
#if defined(SOC_PSRAM0_DATA_BASE)
		return SOC_PSRAM0_DATA_BASE;
#else
		return SOC_PSRAM_DATA_BASE;
#endif
	}

	/* psram_id == 1 */
#if defined(SOC_PSRAM1_DATA_BASE)
	return SOC_PSRAM1_DATA_BASE;
#elif defined(SOC_QSPI0_DATA_BASE)
	/* Some SOCs map the second PSRAM-like region at QSPI0 data base */
	return SOC_QSPI0_DATA_BASE;
#else
	return 0;
#endif
}

static inline uint32_t psram_test_get_size(uint32_t psram_id)
{
	(void)psram_id;
#if defined(SOC_QSPI0_DATA_SIZE)
	if (psram_id == 1) {
		return SOC_QSPI0_DATA_SIZE;
	}
#endif
	return SOC_PSRAM_DATA_SIZE;
}

#define PSRAM_TEST_LEN               (1024 * 4)
beken_thread_t  psram_thread_hdl = NULL;

typedef struct {
	uint8_t test_running;
	uint8_t test_mode;
	uint8_t cacheable;
	uint8_t dma_channel;
	bool silent_mode;
	uint8_t psram_id;
	uint32_t start_addr;
	uint32_t test_size;
	uint32_t psram_total_size;
	uint32_t length;
	uint32_t *data;
	uint32_t delay_time;
	uint32_t id;
	uint32_t data_type;
} psram_debug_t;

psram_debug_t * psram_debug = NULL;

static uint64_t bk_get_current_timer(void)
{
	uint64_t timer = 0;

#if (CONFIG_AON_RTC || CONFIG_ANA_RTC)
	timer = bk_aon_rtc_get_us();
#endif

	return timer;
}

static uint64_t bk_get_spend_time_us(uint64_t before, uint64_t after)
{
	uint64_t spend_time = 0;

	if (after == 0 || before >= after)
	{
		spend_time = 0;
		return spend_time;
	}

#ifdef CONFIG_ARCH_RISCV
	spend_time = (after - before) / 26;
#else // CONFIG_ARCH_RISCV

#if (CONFIG_AON_RTC || CONFIG_ANA_RTC)
	spend_time = after - before;
#endif

#endif // CONFIG_ARCH_RISCV

	return spend_time;
}


static uint32_t s_cpu_test_pass_count = 0;
static uint32_t s_cpu_test_fail_count = 0;
static uint64_t s_cpu_test_last_status_time = 0;

static void psram_cpu_write_test(void)
{
	uint32_t i = 0;
	uint32_t error_num = 0;
	uint32_t value = 0;
	uint32_t base_addr = CONFIG_PSRAM_TEST_CPU_ADDR;
	uint32_t test_len = CONFIG_PSRAM_TEST_CPU_SIZE;

#if TEST_PSRAM_ACCURACY
	uint32_t j = 0;
	for (j = 0; j < test_len / psram_debug->length; j ++)
	{
		for (i = 0; i < psram_debug->length; i += 4)
			write_data((base_addr + i + (j << 15)), psram_debug->data[(i >> 2) & 0x1FFF]);
	}
#else
	for (i = 0; i < test_len; i +=4)
		write_data(base_addr + i, 0x11223344);
#endif

	for (i = 0; i < test_len / 4; i++)
		read_data((base_addr + i * 0x4), value);

#if TEST_PSRAM_ACCURACY
	for (i = 0; i < test_len / 4; i++)
	{
		value = get_addr_data(base_addr + i * 0x4);
		if (value != (psram_debug->data[i & 0x1FFF]))
		{
			if (error_num < 10)
				CLI_LOGE("cpu_test ERR @%08x: got %08x expect %08x xor %08x\r\n",
					base_addr + i * 0x4, value, psram_debug->data[i & 0x1FFF],
					value ^ psram_debug->data[i & 0x1FFF]);
			error_num++;
		}
	}
#endif

	if (error_num > 0) {
		s_cpu_test_fail_count++;
		CLI_LOGE("cpu_test FAIL: error_num=%u (pass=%u, fail=%u)\r\n",
			error_num, s_cpu_test_pass_count, s_cpu_test_fail_count);
	} else {
		s_cpu_test_pass_count++;
	}

	uint64_t now = bk_get_current_timer();
	if (now - s_cpu_test_last_status_time >= 5000000) {
		CLI_LOGD("cpu_test running [%08x-%08x]: pass=%u, fail=%u\r\n",
			base_addr, base_addr + test_len, s_cpu_test_pass_count, s_cpu_test_fail_count);
		s_cpu_test_last_status_time = now;
	}

	rtos_delay_milliseconds(psram_debug->delay_time);
}

/*
 * PSRAM test DMA selection: CONFIG_PSRAM_TEST_USE_HPDMA=y use HPDMA, else use GDMA (general_dma).
 * GDMA path is used only when GENERAL_DMA is linked (PSRAM_TEST_USE_HPDMA_EFF); else HPDMA to avoid undefined refs.
 */
#if (PSRAM_TEST_USE_HPDMA_EFF)
/* Copy PSRAM to RAM using HPDMA (same as hpdma_driver/hpdma_test), for write-through test */
static bk_err_t dma_copy_psram_to_ram(void *ram_dst, const void *psram_src, uint32_t len)
{
	return bk_hpdma_memcpy(ram_dst, psram_src, len);
}

extern bk_err_t hpdma_memcpy_by_chnl(void *out, const void *in, uint32_t len, hpdma_id_t cpy_chnl);

static bk_err_t psram_dma_memcpy_by_chnl(void *out, const void *in, uint32_t len, dma_id_t cpy_chnl)
{
	return hpdma_memcpy_by_chnl(out, in, len, (hpdma_id_t)cpy_chnl);
}
#else
/* Copy PSRAM to RAM using GDMA (general_dma dma_memcpy), for write-through test */
static bk_err_t dma_copy_psram_to_ram(void *ram_dst, const void *psram_src, uint32_t len)
{
	return dma_memcpy(ram_dst, psram_src, len);
}

/* GDMA copy by channel (dma_memcpy_by_chnl from bk_general_dma) */
static bk_err_t psram_dma_memcpy_by_chnl(void *out, const void *in, uint32_t len, dma_id_t cpy_chnl)
{
	return dma_memcpy_by_chnl(out, in, len, cpy_chnl);
}
#endif

static void psram_dma_write_test(void)
{
	uint32_t i = 0;
	uint32_t error_num = 0;
	uint64_t rate = 0;
	uint64_t timer0, timer1;
	uint32_t total_time = 0;
	uint32_t value = 0;
	uint32_t base_addr = psram_test_get_base_addr(psram_debug->psram_id);

#if (CONFIG_PSRAM_APS128XXO_OB9)
	uint32_t test_len = 1024 * 1024 * 16;
#elif (CONFIG_PSRAM_W955D8MKY_5J)
	uint32_t test_len = 1024 * 1024 * 4;
#else //CONFIG_PSRAM_APS6408L_O
	uint32_t test_len = 1024 * 1024 * 8;
#endif

	uint32_t region_size = psram_test_get_size(psram_debug->psram_id);
	if (region_size && test_len > region_size) {
		test_len = region_size;
	}

	CLI_LOGD("begin write %08x-%08x test\r\n", base_addr, base_addr + test_len);

	timer0 = bk_get_current_timer();

	for (i = 0; i < test_len / psram_debug->length; i++)
	{
		psram_dma_memcpy_by_chnl((void *)(base_addr + i * psram_debug->length), psram_debug->data, psram_debug->length, psram_debug->dma_channel);
	}

	timer1 = bk_get_current_timer();
	if (timer1 > timer0)
	{
		total_time = bk_get_spend_time_us(timer0, timer1);
		rate = (test_len) * 8 / total_time;
		CLI_LOGD("finish write, use time: %ld ms, write_rate:%d Mbps\r\n", (uint32_t)(total_time / 1000), rate);
	}

	CLI_LOGD("begin read %08x-%08x test\r\n", base_addr, base_addr + test_len);

	timer0 = bk_get_current_timer();

	for (i = 0; i < test_len / 4; i++)
		read_data((base_addr + i * 0x4), value);

	timer1 = bk_get_current_timer();
	if (timer1 > timer0)
	{
		total_time = bk_get_spend_time_us(timer0, timer1);
		rate = ((uint64_t)test_len) * 8/ total_time;
		CLI_LOGD("finish read, use time: %ld ms, read_rate:%d Mbps\r\n", (uint32_t)(total_time / 1000), rate);
	}

	for (i = 0; i < test_len / psram_debug->length; i++)
	{
		for (uint32_t k = 0; k < psram_debug->length / 4; k++)
		{
			value = get_addr_data(base_addr + i * psram_debug->length + k * 0x4);

			if (value != psram_debug->data[k])
			{
				CLI_LOGD("==========%08x %08x %08x=======\n", value, psram_debug->data[k], value^psram_debug->data[k]);
				error_num++;
			}
		}
	}

	CLI_LOGD("finish compare, error_num: %ld, corr_rate: %ld.\r\n", error_num, ((test_len / 4 - error_num) * 100 / (test_len / 4)));

	rtos_delay_milliseconds(psram_debug->delay_time);

}

static void psram_write_continue_test(void)
{
	uint32_t i = 0;
	uint32_t error_num = 0;
	uint64_t rate = 0;
	uint64_t timer0, timer1;
	uint32_t total_time = 0;
	uint32_t value = 0;
	uint32_t base_addr = psram_test_get_base_addr(psram_debug->psram_id);

#if (CONFIG_PSRAM_APS128XXO_OB9)
	uint32_t test_len = 1024 * 1024 * 16;
#elif (CONFIG_PSRAM_W955D8MKY_5J)
	uint32_t test_len = 1024 * 1024 * 4;
#else //CONFIG_PSRAM_APS6408L_O
	uint32_t test_len = 1024 * 1024 * 8;
#endif

	uint32_t region_size = psram_test_get_size(psram_debug->psram_id);
	if (region_size && test_len > region_size) {
		test_len = region_size;
	}

	CLI_LOGD("begin write %08x-%08x test\r\n", base_addr, base_addr + test_len);
	timer0 = bk_get_current_timer();
	for (i = 0; i < test_len; i += psram_debug->length)
	{
		bk_psram_memcpy((uint8_t *)(base_addr + i), (uint8_t *)&psram_debug->data[0], psram_debug->length);
	}

	timer1 = bk_get_current_timer();
	if (timer1 > timer0)
	{
		total_time = bk_get_spend_time_us(timer0, timer1);
		rate = ((uint64_t)test_len) * 1000000 / total_time;
		CLI_LOGD("finish write, use time: %ld ms, write_rate:%d%ld byte/s\r\n", (uint32_t)(total_time / 1000),
			(uint32_t)(rate >> 32), (uint32_t)(rate & 0xFFFFFFFF));
	}


	CLI_LOGD("begin read %08x-%08x test\r\n", base_addr, base_addr + test_len);
	timer0 = bk_get_current_timer();
	for (i = 0; i < test_len / 4; i++)
		read_data((base_addr + i * 0x4), value);

	timer1 = bk_get_current_timer();
	if (timer1 > timer0)
	{
		total_time = bk_get_spend_time_us(timer0, timer1);
		rate = ((uint64_t)test_len) * 1000000 / total_time;
		CLI_LOGD("finish read, use time: %ld ms, read_rate:%d%ld byte/s\r\n", (uint32_t)(total_time / 1000),
			(uint32_t)(rate >> 32), (uint32_t)(rate & 0xFFFFFFFF));
	}

	for (i = 0; i < test_len / psram_debug->length; i++)
	{
		for (uint32_t k = 0; k < psram_debug->length / 4; k++)
		{
			value = get_addr_data(base_addr + i * psram_debug->length + k * 0x4);

			if (value != psram_debug->data[k])
			{
				error_num++;
			}
		}
	}

	CLI_LOGD("finish compare, error_num: %ld, corr_rate: %ld.\r\n", error_num, (test_len / 4 - error_num) * 100 / (test_len / 4));

	rtos_delay_milliseconds(psram_debug->delay_time);

}

static void psram_test_unit(void)
{
}

//TODO fix/refactoring it during v5 verification
static uint32_t psram_calibrate_read_write_test(uint32_t start_address, uint32_t test_len)
{
	uint32_t error_num = 0;
	uint32_t value = 0;
	uint32_t i = 0;

	for (i = 0; i < test_len / 4; i++)
		write_data((start_address + i * 0x4), 0x11111111 + i);

	for (i = 0; i < test_len / 4; i++)
	{
		value = get_addr_data(start_address + i * 0x4);
		if (value != (0x11111111 + i))
			error_num++;
	}

	return error_num;
}

// static void psram_calibrate_test(void)
// {
// #if CONFIG_PSRAM_CALIBRATE
// 	static uint32_t s_temperature = 0;
// 	uint32_t cur_temperature = -1;
// 	uint32_t err_cnt = 0;
// 	// uint32_t v = 0;

// 	if (BK_OK != temp_detect_get_temperature(&cur_temperature)) {
// 		CLI_LOGE("failed to get temperature\r\n");
// 		return;
// 	}

// 	uint32_t diff = (cur_temperature > s_temperature) ? (cur_temperature - s_temperature) : (s_temperature - cur_temperature);
// 	if (diff > 100 ) { //TODO give a reasonable value
// 		uint32_t base_addr = psram_test_get_base_addr(psram_debug->psram_id);
// 		for (int i = 0; i < 63; i++) {
// 			uint32_t v = (i & 7) | ((i & ~7) << 3);
// 			REG_WRITE(0x46080000 + (5 << 2), v);
// 			err_cnt = psram_calibrate_read_write_test(base_addr, 10240);
// 			if (err_cnt) {
// 				BK_LOGD(NULL, "%d %d %d %d %d\r\n", cur_temperature, (i & 7), (i >> 6) & 3, (i >> 8) & 3, err_cnt);
// 			}
// 		}
// 	}
// #endif
// }

#if CONFIG_PSRAM_CALIBRATE
#define	PSRAM_7239_REG5_ADDR	(0x45040000 + 0x5 * 4)
typedef struct {
	uint8_t ds0_delay_t;
	// uint32_t ds1_delay_t; //16line
	uint8_t clks_iodrv_t;
	uint8_t dats_iodrv_t;
} psram_dly_drv;

static void psram_7239_set_reg5_value(uint32_t value)
{
	write_data(PSRAM_7239_REG5_ADDR, value);
}

static void psram_calib_delay(volatile uint32_t times)
{
	while(times--);
}

static int psram_7239n_calibration(uint32_t start_addr, uint32_t test_len)
{
	uint32_t i,k,err,val;
	uint8_t ds0_delay = 0, ds1_delay = 0, clks_iodrv = 0, dats_iodrv = 0;
	psram_dly_drv *psram_param =  (psram_dly_drv *)os_malloc(512 * sizeof(psram_dly_drv));
	// psram_dly_drv psram_param[512] = {0};
	uint32_t param_ptr_min = 0, param_ptr_max = 0;

	test_len /= 4;

	CLI_LOGI("Start Psram Calib...\r\n");

	// set traversal range
	for(ds0_delay = 0; ds0_delay <= 7; ds0_delay ++)
	{
		for(clks_iodrv = 0; clks_iodrv <= 7; clks_iodrv ++)
		{
			for(dats_iodrv = 0; dats_iodrv <= 7; dats_iodrv ++)
			{
				psram_7239_set_reg5_value((((uint32_t)ds0_delay)<<0) + (((uint32_t)ds1_delay)<<3) + (((uint32_t)clks_iodrv)<<6) + (((uint32_t)dats_iodrv)<<9) + (1 << 12));
				psram_calib_delay(100);

				err = 0; k = 1;

				while(k--)
				{
					for (i = 0; i < test_len; i++)
					{
						write_data((start_addr + i * 0x4), 0x11111111 + i);
					}
					psram_calib_delay(100);
					for (i = 0; i < test_len; i++)
					{
						val = get_addr_data(start_addr + i * 0x4);
						if ((val != (0x11111111 + i)))
						{
							err = 1;
						}
					}
				}

				if(err == 0)
				{
					printf("calib success: ds0_delay=%d,clks_iodrv=%d,dats_iodrv=%d\n",ds0_delay,clks_iodrv,dats_iodrv);
					psram_param[param_ptr_max].ds0_delay_t =  ds0_delay;
					psram_param[param_ptr_max].clks_iodrv_t =  clks_iodrv;
					psram_param[param_ptr_max].dats_iodrv_t =  dats_iodrv;
					param_ptr_max = param_ptr_max + 1;
				}
			}
		}
	}

	//no successful parameter
	if(param_ptr_max == 0)
		return 1;//err

	//find mid parameter
	uint32_t param_min,param_max;
	char flag;
	param_ptr_max = param_ptr_max -1;

	flag = 1;
	param_min = psram_param[param_ptr_min].ds0_delay_t;
	param_max = psram_param[param_ptr_max].ds0_delay_t;
	while(flag){
		if((param_max == param_min) || (param_ptr_max == param_ptr_min))
		{
			ds0_delay = param_min;
			flag = 0;
		}
		else if((param_max - param_min == 1) || (param_ptr_max - param_ptr_min == 1))
		{
			ds0_delay = param_min;
			flag = 0;

			while(psram_param[param_ptr_max].ds0_delay_t != param_min)
			{
				param_ptr_max--;
			}
			param_max = psram_param[param_ptr_max].ds0_delay_t;
		}
		else
		{
			while(psram_param[param_ptr_min].ds0_delay_t == param_min)
			{
				param_ptr_min++;
			}
			param_min = psram_param[param_ptr_min].ds0_delay_t;

			while(psram_param[param_ptr_max].ds0_delay_t == param_max)
			{
				param_ptr_max--;
			}
			param_max = psram_param[param_ptr_max].ds0_delay_t;
		}
	}
	// //printf("ds0 param_min:%d,param_ptr_min:%d,param_max:%d,param_ptr_max:%d\n",param_min,param_ptr_min,param_max,param_ptr_max);
	for(k=param_ptr_min;k<=param_ptr_max;k=k+1)
		printf("ds0 calib success[%u]: ds0_delay=%u,clks_iodrv=%u,dats_iodrv=%u\n",k,
		psram_param[k].ds0_delay_t,psram_param[k].clks_iodrv_t,psram_param[k].dats_iodrv_t);

	flag = 1;
	param_min = psram_param[param_ptr_min].clks_iodrv_t;
	param_max = psram_param[param_ptr_max].clks_iodrv_t;
	while(flag)
	{
		if((param_max == param_min) || (param_ptr_max == param_ptr_min))
		{
			clks_iodrv = param_max;
			flag = 0;
		}
		else if((param_max - param_min == 1) || (param_ptr_max - param_ptr_min == 1))
		{
			clks_iodrv = param_max;
			flag = 0;

			while(psram_param[param_ptr_min].clks_iodrv_t != param_max)
			{
				param_ptr_min++;
			}
			param_min = psram_param[param_ptr_min].clks_iodrv_t;
		}
		else
		{
			while(psram_param[param_ptr_min].clks_iodrv_t == param_min)
			{
				param_ptr_min++;
			}
			param_min = psram_param[param_ptr_min].clks_iodrv_t;

			while(psram_param[param_ptr_max].clks_iodrv_t == param_max)
			{
				param_ptr_max--;
			}
			param_max = psram_param[param_ptr_max].clks_iodrv_t;
		}
	}
	//printf("clks param_min:%d,param_ptr_min:%d,param_max:%d,param_ptr_max:%d\n",param_min,param_ptr_min,param_max,param_ptr_max);
	for(k=param_ptr_min;k<=param_ptr_max;k=k+1)
		printf("clks calib success[%u]: ds0_delay=%u,clks_iodrv=%u,dats_iodrv=%u\n",k,
		psram_param[k].ds0_delay_t,psram_param[k].clks_iodrv_t,psram_param[k].dats_iodrv_t);

	flag = 1;
	param_min = psram_param[param_ptr_min].dats_iodrv_t;
	param_max = psram_param[param_ptr_max].dats_iodrv_t;
	while(flag)
	{
		if((param_max == param_min) || (param_ptr_max == param_ptr_min))
		{
			dats_iodrv = param_min;
			flag = 0;
		}
		else if((param_max - param_min == 1) || (param_ptr_max - param_ptr_min == 1))
		{
			dats_iodrv = param_min;
			flag = 0;

			while(psram_param[param_ptr_max].dats_iodrv_t != param_min)
			{
				param_ptr_max--;
			}
			param_max = psram_param[param_ptr_max].dats_iodrv_t;
		}
		else
		{
			while(psram_param[param_ptr_min].dats_iodrv_t == param_min)
			{
				param_ptr_min++;
			}
			param_min = psram_param[param_ptr_min].dats_iodrv_t;

			while(psram_param[param_ptr_max].dats_iodrv_t == param_max)
			{
				param_ptr_max--;
			}
			param_max = psram_param[param_ptr_max].dats_iodrv_t;
		}
	}

	CLI_LOGI("calib result: ds0_delay = %d, clks_iodrv = %d, dats_iodrv = %d\n",ds0_delay,clks_iodrv,dats_iodrv);
	psram_7239_set_reg5_value((((uint32_t)ds0_delay)<<0) + (((uint32_t)ds1_delay)<<3) + (((uint32_t)clks_iodrv)<<6) + (((uint32_t)dats_iodrv)<<9) + (1 << 12));
	os_free(psram_param);
	return 0;
}

// int psram_7239n_calibrate(uint32_t calib_en, uint32_t start_addr, uint32_t test_len)
// {
// 	uint32_t i,val;
// 	uint32_t err = 0;
// 	uint32_t cur_temperature;

// 	test_len /= 4;

// 	//initialized parameters
// 	//if passed,use the current parameters and no longer calibrate to save time
// 	psram_7239_set_reg5_value((3<<0) + (0<<3) + (2<<6) + (2<<8));

// 	for (i = 0; i < test_len; i++)
// 	{
// 		write_data((start_addr + i * 0x4), 0x11111111 + i);
// 	}
// 	psram_calib_delay(100);
// 	for (i = 0; i < test_len; i++)
// 	{
// 		val = get_addr_data(start_addr + i * 0x4);
// 		if (val != (0x11111111 + i))
// 		{
// 			err = 1;
// 		}
// 	}

// 	//test initialized parameters fail and enable calibration
// 	if(calib_en && err)
// 	{
// 		if (BK_OK != temp_detect_get_temperature(&cur_temperature))
// 		{
// 			CLI_LOGE("failed to get temperature\r\n");
// 		}

// 		test_len *= 4;

// 		if( BK_OK == psram_7239n_calibration(start_addr, test_len) )
// 		{
// 			return 0;
// 		}
// 		else
// 		{
// 			return 1;
// 		}

// 	}
// 	else if(err)
// 	{
// 		return 1;
// 	}
// 	else
// 	{
// 		return 0;
// 	}
// }

static void psram_7239n_calibrate_test(void)
{
	uint32_t start_addr = psram_test_get_base_addr(psram_debug->psram_id);
	uint32_t test_len = 1024 * 1024 * 8;

	if( BK_OK == psram_7239n_calibration(start_addr, test_len) )
	{
	}
	else
	{
	}

	rtos_delay_milliseconds(psram_debug->delay_time);
}
#endif
static volatile uint32_t psram_7239n_total = 0;
static volatile uint32_t psram_7239n_pass = 0;
static volatile uint32_t psram_7239n_fail = 0;
static volatile uint32_t psram_7239n_unaligned_offset = 0;

static void psram_write_test_new(void)
{
	uint32_t i = 0;
	uint32_t j = 0;
	uint32_t k = 0;
	uint32_t error_num = 0;
	uint32_t value = 0;
	uint32_t base = psram_test_get_base_addr(psram_debug->psram_id);
	uint32_t base_addr = base;

//4Byte aligned
#if (CONFIG_PSRAM_APS6408L_O)
	uint32_t test_len = 1024 * 1024 * 8;
#elif (CONFIG_PSRAM_W955D8MKY_5J)
	uint32_t test_len = 1024 * 1024 * 4;
#else //CONFIG_PSRAM_APS128XXO_OB9
	uint32_t test_len = 1024 * 1024 * 16;
#endif
	psram_debug->length = 1024 * 32;

	uint32_t region_size = psram_test_get_size(psram_debug->psram_id);
	if (region_size && test_len > region_size) {
		test_len = region_size;
	}

	// CLI_LOGD("begin write %08x-%08x 4Byte unaligned test\r\n", base_addr, base_addr + test_len);

	for (j = 0; j < test_len / psram_debug->length; j ++)
	{
		for (i = 0; i < psram_debug->length; i += 4)
			write_data((base_addr + i + (j << 15)), psram_debug->data[(i >> 2) & 0x1FFF]);
	}

	// CLI_LOGD("begin read %08x-%08x test\r\n", base_addr, base_addr + test_len);

	for (i = 0; i < test_len / 4; i++)
	{
		value = get_addr_data(base_addr + i * 0x4);
		if (value != (psram_debug->data[i & 0x1FFF]))
		{
			CLI_LOGD("4Byte align err: addr %08x, val %08x %08x %08x\n", base_addr + i * 0x4, value, psram_debug->data[i & 0x1FFF], value^psram_debug->data[i & 0x1FFF]);
			error_num++;
		}
	}

	// CLI_LOGD("finish compare, error_num: %ld, corr_rate: %ld.\r\n", error_num, ((test_len / 4 - error_num) * 100 / (test_len / 4)));

//4Byte unaligned
	if(psram_7239n_unaligned_offset >= 1024 * 1024 * 5)
		psram_7239n_unaligned_offset = 1;
	else
		psram_7239n_unaligned_offset ++;

	base_addr = base + psram_7239n_unaligned_offset;
	test_len = 1024 * 1024 * 2;
	psram_debug->length = 1024 * 16;

	// CLI_LOGD("begin write %08x-%08x 4Byte unaligned test\r\n", base_addr, base_addr + test_len);

	for (j = 0; j < test_len / psram_debug->length; j ++)
	{
		for (i = 0; i < psram_debug->length; i += 4)
			write_data((base_addr + i + (j << 14)), psram_debug->data[(i >> 2) & 0xFFF]);
	}

	for (i = 0; i < test_len / 4; i++)
	{
		value = get_addr_data(base_addr + i * 0x4);
		if (value != (psram_debug->data[i & 0xFFF]))
		{
			CLI_LOGD("4Byte unlign1 err: addr %08x, val %08x %08x %08x\n", base_addr + i * 0x4, value, psram_debug->data[i & 0xFFF], value^psram_debug->data[i & 0xFFF]);
			error_num++;
		}
	}

	// CLI_LOGD("finish compare, error_num: %ld, corr_rate: %ld.\r\n", error_num, ((test_len / 4 - error_num) * 100 / (test_len / 4)));


//1Byte
	base_addr = base;
#if (CONFIG_PSRAM_APS6408L_O)
	test_len = 1024 * 1024 * 8;
#elif (CONFIG_PSRAM_W955D8MKY_5J)
	test_len = 1024 * 1024 * 4;
#else //CONFIG_PSRAM_APS128XXO_OB9
	test_len = 1024 * 1024 * 16;
#endif
	psram_debug->length = 1024 * 32;

	// CLI_LOGD("begin write %08x-%08x 1Byte test\r\n", base_addr, base_addr + test_len);

	for (i = 0; i < test_len; i += psram_debug->length)
	{
		bk_psram_memcpy((uint8_t *)(base_addr + i), (uint8_t *)&psram_debug->data[0], psram_debug->length);
	}

	// CLI_LOGD("begin read %08x-%08x test\r\n", base_addr, base_addr + test_len);

	for (i = 0; i < test_len / psram_debug->length; i++)
	{
		for (k = 0; k < psram_debug->length / 4; k++)
		{
			value = get_addr_data(base_addr + i * psram_debug->length + k * 0x4);

			if (value != psram_debug->data[k])
			{
				CLI_LOGD("1Byte err: addr %08x, val %08x %08x %08x\n", base_addr + i * psram_debug->length + k * 0x4, value, psram_debug->data[k], value^psram_debug->data[k]);
				error_num++;
			}
		}
	}

	// CLI_LOGD("finish compare, error_num: %ld, corr_rate: %ld.\r\n", error_num, (test_len / 4 - error_num) * 100 / (test_len / 4));


//psram_malloc
#if (CONFIG_PSRAM_APS6408L_O)
	test_len = 1024 * 1024 * 8 - 512;
#elif (CONFIG_PSRAM_W955D8MKY_5J)
	test_len = 1024 * 1024 * 4 - 512;
#else //CONFIG_PSRAM_APS128XXO_OB9
	test_len = 1024 * 1024 * 16 - 512;
#endif

	// CLI_LOGD("Malloc Test\r\n");

	uint8_t *data = (uint8_t *)psram_malloc(test_len);
	if (data == NULL)
	{
		CLI_LOGD("psram malloc error!\r\n");
		return;
	}

	for(i = 0; i < test_len; i += 4)
	{
		write_data((uint32_t *)&data[i], 0x11111111 + (i >> 2));
	}

	for (i = 0; i < test_len; i += 4)
	{
		if (*(uint32_t *)&data[i] != 0x11111111 + (i >> 2))
		{
			CLI_LOGD("Malloc Err %08x %08x\r\n", *(uint32_t *)&data[i], 0x11111111 + (i >> 2));
			error_num++;
		}
	}

	psram_free(data);

	// CLI_LOGD("Malloc finish compare, error_num: %ld, corr_rate: %ld.\r\n", error_num, (test_len / 4 - error_num) * 100 / (test_len / 4));

	if(error_num)
	{
		psram_7239n_total++;
		psram_7239n_fail++;
	}
	else
	{
		psram_7239n_total++;
		psram_7239n_pass++;
	}

	CLI_LOGD("Finish %d tests, PASS times: %d, FAIL times: %d\r\n", psram_7239n_total, psram_7239n_pass, psram_7239n_fail);

	rtos_delay_milliseconds(psram_debug->delay_time);

}

static void psram_test_main(void)
{
	if (psram_debug->test_mode == 0) {
		s_cpu_test_pass_count = 0;
		s_cpu_test_fail_count = 0;
		s_cpu_test_last_status_time = bk_get_current_timer();
		CLI_LOGD("cpu_test started: region [%08x-%08x] (%uKB)\r\n",
			CONFIG_PSRAM_TEST_CPU_ADDR,
			CONFIG_PSRAM_TEST_CPU_ADDR + CONFIG_PSRAM_TEST_CPU_SIZE,
			CONFIG_PSRAM_TEST_CPU_SIZE / 1024);
	}

	while (psram_debug->test_running)
	{
		if (psram_debug->test_mode == 0)
		{
			psram_cpu_write_test();
		} else if (psram_debug->test_mode == 1) {
			psram_test_unit();
		} else if (psram_debug->test_mode == 2) {
			psram_write_continue_test();
		} else if (psram_debug->test_mode == 3) {
			psram_dma_write_test();
		} else if (psram_debug->test_mode == 5) {
			psram_write_test_new();
		} else {
			//psram_calibrate_test();
		}
	}

	CLI_LOGD("psram_test task exit\n");

	if (psram_debug)
	{
		if (psram_debug->data)
		{
			os_free(psram_debug->data);
			psram_debug->data = NULL;
		}

#if (PSRAM_TEST_USE_HPDMA_EFF)
		bk_hpdma_free(HPDMA_DEV_DTCM, (hpdma_id_t)psram_debug->dma_channel);
#else
		bk_dma_free(DMA_DEV_DTCM, (dma_id_t)psram_debug->dma_channel);
#endif

		os_free(psram_debug);
		psram_debug = NULL;
	}

	psram_thread_hdl = NULL;
	rtos_delete_thread(NULL);
}

static bk_err_t psram_task_init(void)
{
	bk_err_t ret = BK_OK;

	if (!psram_thread_hdl)
	{
		ret = rtos_create_thread(&psram_thread_hdl,
								 4,
								 "psram_debug",
								 (beken_thread_function_t)psram_test_main,
								 4 * 1024,
								 (beken_thread_arg_t)NULL);
		if (ret != BK_OK)
		{
			psram_thread_hdl = NULL;
			CLI_LOGE("Error: Failed to create psram test task: %d\r\n", ret);
			return BK_ERR_NOT_INIT;
		}

		return BK_OK;
	}
	else
		return BK_OK;
}

static void cli_psram_cmd_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = NULL;

	if (os_strcmp(argv[1], "start") == 0)
	{
		bk_psram_init();

		psram_debug = (psram_debug_t *)os_malloc(sizeof(psram_debug_t));

		if (psram_debug == NULL)
		{
			CLI_LOGE("psram test malloc failed!\r\n");
			msg = CLI_CMD_RSP_ERROR;
			os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
			return;
		}

		os_memset(psram_debug, 0, sizeof(psram_debug_t));

		psram_debug->test_running = 1;

		/* Optional: select PSRAM0/PSRAM1, default PSRAM0.
		 * Command format:
		 *   psram_test start <cpu|conexist|continue_write|dma|calibrate|new> <cacheable 0|1> [delay_ms] [silent 0|1] [data_type] [psram_id 0|1]
		 */
		psram_debug->psram_id = 0;
		if (argc >= 8) {
			psram_debug->psram_id = (uint8_t)os_strtoul(argv[7], NULL, 10);
		}
		if (psram_debug->psram_id > 1) {
			psram_debug->psram_id = 0;
		}
		if (psram_test_get_base_addr(psram_debug->psram_id) == 0) {
			CLI_LOGW("psram_id %d base addr invalid, fallback to psram_id 0\r\n", psram_debug->psram_id);
			psram_debug->psram_id = 0;
		}

		if (os_strcmp(argv[2], "cpu") == 0)
		{
			psram_debug->test_mode = 0;
		}
		else if (os_strcmp(argv[2], "conexist") == 0)
		{
			psram_debug->test_mode = 1;
		}
		else if (os_strcmp(argv[2], "continue_write") == 0)
		{
			psram_debug->test_mode = 2;
		} else if (os_strcmp(argv[2], "dma") == 0) {
			psram_debug->test_mode = 3;
#if (PSRAM_TEST_USE_HPDMA_EFF)
			psram_debug->dma_channel = (uint8_t)bk_hpdma_alloc(HPDMA_DEV_DTCM);
#else
			psram_debug->dma_channel = (uint8_t)bk_dma_alloc(DMA_DEV_DTCM);
#endif
		} else if (os_strcmp(argv[2], "calibrate") == 0) {
			psram_debug->test_mode = 4;
		} else if (os_strcmp(argv[2], "new") == 0) {
			psram_debug->test_mode = 5;
		}
		// else if (os_strcmp(argv[2], "scan") == 0) {
		// 	if(argc < 8)
		// 	{
		// 		msg = CLI_CMD_RSP_ERROR;
		// 		CLI_LOGE("psram test cmd should have <clk> <vol> <data_type> <start_addr> <size>!\r\n");
		// 		os_free(psram_debug);
		// 		psram_debug = NULL;
		// 		return;
		// 	}
		// 	else
		// 	{
		// 		psram_debug->test_mode = 6;
		// 		uint32_t psram_clk = os_strtoul(argv[5], NULL, 10);
		// 		uint32_t psram_vol = os_strtoul(argv[6], NULL, 10);
		// 		psram_debug->data_type = os_strtoul(argv[7], NULL, 10);
		// 		bk_psram_init_with_para(psram_clk,psram_vol);
		// 		if(argc > 8)
		// 		{
		// 			psram_debug->start_addr = os_strtoul(argv[8], NULL, 16);
		// 			psram_debug->test_size = os_strtoul(argv[9], NULL, 16);
		// 		}
		// 		psram_debug->id = bk_psram_get_psram_id();
		// 		psram_debug->psram_total_size = bk_psram_get_psram_data_length(psram_debug->id);
		// 	}

		// }

		if(argc >= 7)
		{
			psram_debug->data_type = os_strtoul(argv[6], NULL, 10);
		}

		if (psram_debug->data == NULL)
		{
			psram_debug->length = 1024 * 32;
			psram_debug->data = (uint32_t *)os_malloc(psram_debug->length);
			if (psram_debug->data == NULL)
			{
				CLI_LOGE("malloc error!\r\n");
				os_free(psram_debug);
				psram_debug = NULL;
				return;
			}
		}

		for (int i = 0; i < psram_debug->length / 4; i++)
		{
			if(psram_debug->data_type == 1)
			{
#if CONFIG_TRNG_SUPPORT
				psram_debug->data[i] = bk_rand() + i;
#endif
			}
			else if (psram_debug->data_type == 2)
			{
				psram_debug->data[i] = 0xA55AA55A + i + (i << 8) + (i << 16) + (i << 24);
			}
		}

		if (os_strcmp(argv[3], "1") == 0)
		{
			psram_debug->cacheable = 1;
		}
		else
		{
			psram_debug->cacheable = 0;
		}

		psram_debug->delay_time = 500;

		if (argc >= 5)
		{
			psram_debug->delay_time = os_strtoul(argv[4], NULL, 10);

			if (psram_debug->delay_time == 0)
				psram_debug->delay_time = 500;
		}

		if(argc >= 6)
		{
			psram_debug->silent_mode = os_strtoul(argv[5], NULL, 10);
		}

		if (psram_task_init() != kNoErr)
		{
			CLI_LOGE("psram test failed!\r\n");
			msg = CLI_CMD_RSP_ERROR;
			os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
			return;
		}

		CLI_LOGD("psram test start success!\r\n");
		msg = CLI_CMD_RSP_SUCCEED;
	}
	else if (os_strcmp(argv[1], "stop") == 0)
	{
		if (psram_thread_hdl)
		{
			psram_debug->test_running = 0;
		}

		while (psram_thread_hdl)
		{
			rtos_delay_milliseconds(10);
		}

		bk_psram_deinit();
		CLI_LOGD("psram test stop success!\r\n");
		msg = CLI_CMD_RSP_SUCCEED;
	}
	#if CONFIG_PSRAM
	else if (os_strcmp(argv[1], "strcat") == 0)
	{
		uint8_t *data = psram_malloc(20);
		if (data == NULL)
		{
			CLI_LOGD("psram malloc error!\r\n");
			msg = CLI_CMD_RSP_ERROR;
			os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
			return;
		}
		os_memset(data, 0, 20);
		bk_psram_strcat((char *)data, (char *)argv[2]);
		bk_psram_strcat((char *)data, (char *)argv[2]);
		bk_psram_strcat((char *)data, (char *)argv[2]);
		psram_free(data);
		msg = CLI_CMD_RSP_SUCCEED;
	}
	#endif
	else
	{
		cli_psram_help();
		msg = CLI_CMD_RSP_ERROR;
	}

	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

static uint32_t test_frame_strip(uint8_t *src, uint32_t size)
{
	uint8_t sram_tmp[16] = {0x71, 0xfb, 0x84, 0x1f, 0x53, 0x5a, 0xd9, 0xd9, 0x8e, 0xd2, 0x76, 0x3f, 0xff, 0xff, 0xff, 0xd9};

	static uint8_t flag = 0;
	flag ++;
	sram_tmp[0] += flag;

	{
		BK_LOGD(NULL, "1====>>>> %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %p %d\n",
			src[size - 16], src[size - 15], src[size - 14], src[size - 13],
			src[size - 12], src[size - 11], src[size - 10], src[size - 9],
			src[size - 8], src[size - 7], src[size - 6], src[size - 5],
			src[size - 4], src[size - 3], src[size - 2], src[size - 1], src, size);
		bk_psram_word_memcpy(src, sram_tmp, 16);

		BK_LOGD(NULL, "2===>>> %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %p %d\n",
			src[size - 16], src[size - 15], src[size - 14], src[size - 13],
			src[size - 12], src[size - 11], src[size - 10], src[size - 9],
			src[size - 8], src[size - 7], src[size - 6], src[size - 5],
			src[size - 4], src[size - 3], src[size - 2], src[size - 1], src, size);

#if (CONFIG_DCACHE)
		arch_dcache_flush_and_invd_range(src, 16);
#endif

		BK_LOGD(NULL, "3==>> %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %p %d\n",
			src[size - 16], src[size - 15], src[size - 14], src[size - 13],
			src[size - 12], src[size - 11], src[size - 10], src[size - 9],
			src[size - 8], src[size - 7], src[size - 6], src[size - 5],
			src[size - 4], src[size - 3], src[size - 2], src[size - 1], src, size);

		bk_psram_word_memcpy(src, sram_tmp, 16);

		BK_LOGD(NULL, "4=> %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %p %d\n",
			src[size - 16], src[size - 15], src[size - 14], src[size - 13],
			src[size - 12], src[size - 11], src[size - 10], src[size - 9],
			src[size - 8], src[size - 7], src[size - 6], src[size - 5],
			src[size - 4], src[size - 3], src[size - 2], src[size - 1], src, size);
	}

	return size;
}


void cli_test_psram_cache_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if CONFIG_DEBUG_VERSION
	uint32_t address, size;
	BK_LOGD(NULL, "cli_test_psram_cache_cmd\r\n");
	if (argc >= 3)
	{
		address = strtoll(argv[1], NULL, 16);
		size = strtoll(argv[2], NULL, 16);
		BK_LOGD(NULL, "test psram cache, address: 0x%08X size: 0x%08X\r\n", address, 16);

		test_frame_strip((uint8_t *)address, size);
	} else {
		BK_LOGD(NULL, "psram_cache <addr> <size>\r\n");
	}
#endif
}

#if (CONFIG_MPC)
#include <driver/mpc.h>

#define BUFFER_SIZE         (34)
#define TEST_VALUE_START    0x41

static void fill_buffer(uint8_t *pBuffer, uint32_t uwBufferLenght, uint32_t uwOffset)
{
	uint32_t tmpIndex = 0;

	/* Put in global buffer different values */
	for (tmpIndex = 0; tmpIndex < uwBufferLenght; tmpIndex++ ) {
		pBuffer[tmpIndex] = tmpIndex + uwOffset;
	}
}

static void cli_psram_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int i;
	uint8_t *test_addr_sec = NULL;
	char *msg = NULL;
	uint8_t psram_tx_buffer[BUFFER_SIZE] = {0};
	uint8_t psram_rx_buffer[BUFFER_SIZE] = {0};

	fill_buffer(psram_tx_buffer, BUFFER_SIZE, TEST_VALUE_START);

	/*set first block non-sec and second block sec*/
	bk_mpc_driver_init();
	bk_mpc_set_secure_attribute(MPC_DEV_PSRAM, 0, 1, MPC_BLOCK_NON_SECURE);
	bk_mpc_set_secure_attribute(MPC_DEV_PSRAM, bk_mpc_get_block_size(MPC_DEV_PSRAM), 1, MPC_BLOCK_SECURE);

	test_addr_sec = (uint8_t *)(SOC_PSRAM_DATA_ADDR_SEC + bk_mpc_get_block_size(MPC_DEV_PSRAM));
	bk_psram_memcpy(test_addr_sec, psram_tx_buffer, BUFFER_SIZE);
	bk_psram_memread(test_addr_sec, psram_rx_buffer, BUFFER_SIZE);

	for (i = 0; i < BUFFER_SIZE; i++) {
		BK_LOGD(NULL, "%02x ", psram_rx_buffer[i]);
	}
	BK_LOGD(NULL, "\r\n");
	msg = CLI_CMD_RSP_SUCCEED;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}
#endif


#if (CONFIG_PSRAM_WRITE_THROUGH)
/* Background task: continuous CPU write (pattern increments per round) + DMA read for write-through test */
typedef struct {
	uint32_t start_addr;
	uint32_t size;
	psram_write_through_area_t area;
	uint8_t psram_id;
	volatile uint32_t running;
} wt_test_ctx_t;
static wt_test_ctx_t s_wt_ctx = {0};
static beken_thread_t s_wt_task_hdl = NULL;

static void psram_wt_task_main(void *arg)
{
	uint32_t start_addr = s_wt_ctx.start_addr;
	uint32_t size = s_wt_ctx.size;
	uint32_t *ram_buf = (uint32_t *)os_malloc(size);
	if (!ram_buf) {
		CLI_LOGE("wt task: malloc ram buf failed\r\n");
		s_wt_task_hdl = NULL;
		rtos_delete_thread(NULL);
		return;
	}
	uint32_t round = 0;
	while (s_wt_ctx.running) {
		uint32_t i;
		uint32_t base = 0xA55A0000 + (round << 16);
		for (i = 0; i < size / 4; i++)
			write_data(start_addr + i * 4, base + i);
		if (dma_copy_psram_to_ram(ram_buf, (const void *)start_addr, size) != BK_OK) {
			CLI_LOGE("wt task: DMA memcpy failed round=%lu\r\n", (unsigned long)round);
			break;
		}
		for (i = 0; i < size / 4; i++) {
			if (ram_buf[i] != (base + i)) {
				CLI_LOGE("wt task: round=%lu [%lu] expect=0x%lx actual=0x%lx (DMA read mismatch)\r\n",
					(unsigned long)round, (unsigned long)i,
					(unsigned long)(base + i), (unsigned long)ram_buf[i]);
				break;
			}
		}
		round++;
		rtos_delay_milliseconds(10);
	}
	os_free(ram_buf);
	s_wt_task_hdl = NULL;
	rtos_delete_thread(NULL);
}

/* Background task: loop wt_verify_512k or wt_verify_8k; print only on error (max 10). Stop via wt_verify_stop. */
#define WTV_MODE_512K  0
#define WTV_MODE_8K    1
typedef struct {
	volatile uint32_t running;
	uint8_t mode;       /* WTV_MODE_512K or WTV_MODE_8K */
	uint32_t start_addr;
	uint32_t size_a;    /* 512*1024 or 8*1024 */
	uint32_t total_size; /* 1M or 16K */
	void *malloc_base;
	psram_write_through_area_t area;
	uint8_t psram_id;
} wt_verify_ctx_t;
static wt_verify_ctx_t s_wt_verify_ctx = {0};
static beken_thread_t s_wt_verify_task_hdl = NULL;

static void psram_wt_verify_task_main(void *arg)
{
	uint32_t start_addr = s_wt_verify_ctx.start_addr;
	uint32_t size_a = s_wt_verify_ctx.size_a;
	uint32_t total_size = s_wt_verify_ctx.total_size;
	uint32_t base_pattern = 0xA55A0000;
	const uint32_t max_print_errors = 10;
	const char *name = (s_wt_verify_ctx.mode == WTV_MODE_512K) ? "wt_verify_512k" : "wt_verify_8k";

	while (s_wt_verify_ctx.running) {
		uint32_t i;
		bk_err_t dma_ret = BK_OK;
		uint32_t mismatch = 0;
		uint32_t err_count = 0;

#if (CONFIG_DCACHE)
		arch_dcache_flush_and_invd_range((uint8_t *)start_addr, total_size);
#endif
		for (i = 0; i < size_a / 4; i++)
			write_data(start_addr + i * 4, base_pattern + i);

#if (PSRAM_TEST_USE_HPDMA_EFF)
		if (s_wt_verify_ctx.mode == WTV_MODE_512K) {
			uint32_t offset = 0;
			while (offset < size_a && dma_ret == BK_OK) {
				uint32_t chunk = size_a - offset;
				if (chunk > 65535u)
					chunk = 65535u;
				dma_ret = bk_hpdma_memcpy((void *)(start_addr + size_a + offset),
					(const void *)(start_addr + offset), chunk);
				offset += chunk;
			}
		} else
			dma_ret = bk_hpdma_memcpy((void *)(start_addr + size_a), (const void *)start_addr, size_a);
#else
		dma_ret = dma_memcpy((void *)(start_addr + size_a), (const void *)start_addr, size_a);
#endif
		if (dma_ret != BK_OK) {
			CLI_LOGE("%s: DMA copy failed\r\n", name);
			break;
		}

#if (CONFIG_DCACHE)
		arch_dcache_invd_range((uint8_t *)(start_addr + size_a), size_a);
#endif
		for (i = 0; i < size_a / 4; i++) {
			uint32_t val = get_addr_data(start_addr + size_a + i * 4);
			if (val != (base_pattern + i)) {
				mismatch = 1;
				err_count++;
				if (err_count <= max_print_errors)
					CLI_LOGE("%s err[%u]: offset=0x%lx addr_B=0x%lx expect=0x%lx actual=0x%lx\r\n",
						name, (unsigned)err_count, (unsigned long)(i * 4),
						(unsigned long)(start_addr + size_a + i * 4),
						(unsigned long)(base_pattern + i), (unsigned long)val);
			}
		}
		if (mismatch)
			CLI_LOGE("%s FAIL: total %lu errors (first %u printed)\r\n", name, (unsigned long)err_count, (unsigned)max_print_errors);

		rtos_delay_milliseconds(10);
	}

	bk_psram_disable_write_through(s_wt_verify_ctx.area);
	bk_psram_free_write_through_channel(s_wt_verify_ctx.area);
	if (s_wt_verify_ctx.malloc_base != NULL) {
		psram_free(s_wt_verify_ctx.malloc_base);
		s_wt_verify_ctx.malloc_base = NULL;
	}
	s_wt_verify_task_hdl = NULL;
	rtos_delete_thread(NULL);
}
#endif

static void cli_psram_cmd_handle_ext(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t addr = SOC_PSRAM_DATA_BASE;
	uint32_t i = 0;
	uint32_t length = 512;
	char *msg = NULL;

	if (argc < 2)
	{
		msg = CLI_CMD_RSP_ERROR;
		goto out;
	}

	if (os_strcmp(argv[1], "init") == 0)
	{
		/*init psram*/
		bk_psram_init();
		msg = CLI_CMD_RSP_SUCCEED;
	}
	else if (os_strcmp(argv[1], "delete_flash") == 0)
	{
#if (CONFIG_PSRAM_AUTO_DETECT)
		bk_set_env_enhance(PSRAM_CHIP_ID, NULL, 0);
		msg = CLI_CMD_RSP_SUCCEED;
#else
		msg = CLI_CMD_RSP_ERROR;
#endif
	}
	else if (os_strcmp(argv[1], "clk") == 0)
	{
		uint16_t clk = os_strtoul(argv[2], NULL, 10);

		switch (clk)
		{
			case 80:
				bk_psram_set_clk(PSRAM_80M);
				break;

			case 120:
				bk_psram_set_clk(PSRAM_120M);
				break;

			case 160:
				bk_psram_set_clk(PSRAM_160M);
				break;

			case 240:
				bk_psram_set_clk(PSRAM_240M);
				break;

			default:
				CLI_LOGE("can not support this clk!\r\n");
				break;
		}

		msg = CLI_CMD_RSP_SUCCEED;
	}
	else if (os_strcmp(argv[1], "byte") == 0)
	{
		uint8_t  value = 0;
		if (argc < 4)
		{
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}

		addr = os_strtoul(argv[2], NULL, 16);
		length = os_strtoul(argv[3], NULL, 10);
		if (length & 0x3)
		{
			length = ((length >> 2) + 1) << 2;
		}

		if (addr > 0x60800000 || addr < SOC_PSRAM_DATA_BASE || length == 0)
		{
			msg = CLI_CMD_RSP_ERROR;
		}
		else
		{
			for (i = 0; i < length; i++)
			{
				*((volatile uint8_t *)addr + i) = i;
			}

			for (i = 0; i < length; i++)
			{
				value = *((volatile uint8_t *)addr + i);

				if (value != (i % 256))
				{
					CLI_LOGD("index:%d, value:%d\r\n", i, value);
					break;
				}
			}

			if (i < length)
			{
				msg = CLI_CMD_RSP_ERROR;
			}
			else
			{
				msg = CLI_CMD_RSP_SUCCEED;
			}
		}
	}
	else if (os_strcmp(argv[1], "word") == 0)
	{
		if (argc < 4)
		{
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}

		addr = os_strtoul(argv[2], NULL, 16);
		length = os_strtoul(argv[3], NULL, 10);
		if (length & 0x3)
		{
			length = ((length >> 2) + 1) << 2;
		}

		if (addr > 0x60800000 || addr < SOC_PSRAM_DATA_BASE || length == 0)
		{
			msg = CLI_CMD_RSP_ERROR;
		}
		else
		{
			for (i = 0; i < length; i += 4)
			{
				*(((volatile uint32_t *)(addr + i))) = (i << 24) + (i << 16) + (i << 8) + i + 0x11223344;
			}
		}

		msg = CLI_CMD_RSP_SUCCEED;
	}
	else if (os_strcmp(argv[1], "rewrite") == 0)
	{
		if (argc < 3)
		{
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}

		addr = os_strtoul(argv[2], NULL, 16);
		length = 20;

		if (addr > 0x60800000 || addr < SOC_PSRAM_DATA_BASE || length == 0)
		{
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}

		for (i = 0; i < length; i += 4)
		{
			*(((volatile uint32_t *)(addr + i))) = 0x11111111 * i + 0x11223344;
		}

		for (i = 0; i < length * 100; i++)
		{
			*((volatile uint32_t *)addr) = 0x44332211;
		}

		for (i = 0; i < length; i ++)
		{
			CLI_LOGD("0x%08x\r\n", *((volatile uint32_t *)(addr + i * 4)));
		}

		msg = CLI_CMD_RSP_SUCCEED;
	}
	else if (os_strcmp(argv[1], "timer_check") == 0)
	{
		/* Verify AON RTC: delay 100ms, measure with RTC (us). Expect ~100000 us. */
#if (CONFIG_AON_RTC || CONFIG_ANA_RTC)
		uint64_t t0_rtc = bk_aon_rtc_get_us();
		rtos_delay_milliseconds(100);
		uint64_t t1_rtc = bk_aon_rtc_get_us();
		uint64_t rtc_us = (t1_rtc > t0_rtc) ? (t1_rtc - t0_rtc) : 0;
		CLI_LOGD("timer_check: 100ms delay => RTC delta=%lu us (expect ~100000 us)\r\n",
			(unsigned long)rtc_us);
		msg = CLI_CMD_RSP_SUCCEED;
#else
		CLI_LOGE("timer_check: need CONFIG_AON_RTC or CONFIG_ANA_RTC\r\n");
		msg = CLI_CMD_RSP_ERROR;
#endif
	}
	else if (os_strcmp(argv[1], "speed") == 0)
	{
		/* PSRAM read/write speed test: psram_malloc 1MB, measure throughput in MB/s.
		 * When AON RTC is available, use it for timing (crystal-based us); else use mtimer (ticks/26). */
#define PSRAM_SPEED_TEST_SIZE (1024 * 1024)
		void *buf = NULL;
		uint32_t size = PSRAM_SPEED_TEST_SIZE;
		uint64_t t0, t1, write_us, read_us;
		uint32_t nw;
		volatile uint32_t read_sink;

		buf = psram_malloc(size);
		if (buf == NULL) {
			CLI_LOGE("speed: psram_malloc(1M) failed\r\n");
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}

#if (CONFIG_AON_RTC || CONFIG_ANA_RTC)
		/* Use AON RTC (32K crystal) for us, no dependency on mtimer/26 */
		t0 = bk_aon_rtc_get_us();
		for (nw = 0; nw < size / 4; nw++)
			((volatile uint32_t *)buf)[nw] = (uint32_t)nw;
		t1 = bk_aon_rtc_get_us();
		write_us = (t1 > t0) ? (t1 - t0) : 1;

		read_sink = 0;
		t0 = bk_aon_rtc_get_us();
		for (nw = 0; nw < size / 4; nw++)
			read_sink += ((volatile uint32_t *)buf)[nw];
		t1 = bk_aon_rtc_get_us();
		read_us = (t1 > t0) ? (t1 - t0) : 1;
#else
		/* Fallback: mtimer ticks, convert with /26 */
		t0 = bk_get_current_timer();
		for (nw = 0; nw < size / 4; nw++)
			((volatile uint32_t *)buf)[nw] = (uint32_t)nw;
		t1 = bk_get_current_timer();
		write_us = bk_get_spend_time_us(t0, t1);

		read_sink = 0;
		t0 = bk_get_current_timer();
		for (nw = 0; nw < size / 4; nw++)
			read_sink += ((volatile uint32_t *)buf)[nw];
		t1 = bk_get_current_timer();
		read_us = bk_get_spend_time_us(t0, t1);
#endif

		psram_free(buf);
#undef PSRAM_SPEED_TEST_SIZE

		if (write_us == 0) write_us = 1;
		if (read_us == 0) read_us = 1;
		/* MB/s * 100 = size * 100 / us (integer only) */
		{
			uint64_t w100 = (uint64_t)size * 100ULL / write_us;
			uint64_t r100 = (uint64_t)size * 100ULL / read_us;
			CLI_LOGD("PSRAM speed (1MB, psram_malloc): write %lu.%02lu MB/s, read %lu.%02lu MB/s\r\n",
				(unsigned long)(w100 / 100), (unsigned long)(w100 % 100),
				(unsigned long)(r100 / 100), (unsigned long)(r100 % 100));
		}
		(void)read_sink;
		msg = CLI_CMD_RSP_SUCCEED;
	}
	else if (os_strcmp(argv[1], "speed_dma") == 0)
	{
		/* PSRAM throughput via HPDMA: alloc 1M total, copy 512K from first half to second half in chunks (max 65535 per transfer), report MB/s. */
#if (PSRAM_TEST_USE_HPDMA_EFF)
#define PSRAM_SPEED_DMA_TOTAL  (1024 * 1024)   /* 1M total buffer */
#define PSRAM_SPEED_DMA_COPY   (512 * 1024)    /* 512K copy size */
#define PSRAM_SPEED_DMA_CHUNK  65535u
		void *buf = NULL;
		uint32_t copy_size = PSRAM_SPEED_DMA_COPY;
		uint64_t t0, t1, copy_us;
		uint32_t offset;
		bk_err_t dma_ret = BK_OK;

		buf = psram_malloc(PSRAM_SPEED_DMA_TOTAL);
		if (buf == NULL) {
			CLI_LOGE("speed_dma: psram_malloc(1M) failed\r\n");
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}

#if (CONFIG_AON_RTC || CONFIG_ANA_RTC)
		t0 = bk_aon_rtc_get_us();
		offset = 0;
		while (offset < copy_size && dma_ret == BK_OK) {
			uint32_t chunk = copy_size - offset;
			if (chunk > PSRAM_SPEED_DMA_CHUNK)
				chunk = PSRAM_SPEED_DMA_CHUNK;
			dma_ret = bk_hpdma_memcpy((void *)((uint8_t *)buf + copy_size + offset),
				(const void *)((uint8_t *)buf + offset), chunk);
			offset += chunk;
		}
		t1 = bk_aon_rtc_get_us();
#else
		t0 = bk_get_current_timer();
		offset = 0;
		while (offset < copy_size && dma_ret == BK_OK) {
			uint32_t chunk = copy_size - offset;
			if (chunk > PSRAM_SPEED_DMA_CHUNK)
				chunk = PSRAM_SPEED_DMA_CHUNK;
			dma_ret = bk_hpdma_memcpy((void *)((uint8_t *)buf + copy_size + offset),
				(const void *)((uint8_t *)buf + offset), chunk);
			offset += chunk;
		}
		t1 = bk_get_current_timer();
		copy_us = bk_get_spend_time_us(t0, t1);
#endif

		psram_free(buf);
#undef PSRAM_SPEED_DMA_CHUNK
#undef PSRAM_SPEED_DMA_COPY
#undef PSRAM_SPEED_DMA_TOTAL

		if (dma_ret != BK_OK) {
			CLI_LOGE("speed_dma: HPDMA copy failed\r\n");
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}
#if (CONFIG_AON_RTC || CONFIG_ANA_RTC)
		copy_us = (t1 > t0) ? (t1 - t0) : 1;
#endif
		if (copy_us == 0) copy_us = 1;
		{
			uint64_t c100 = (uint64_t)copy_size * 100ULL / copy_us;
			CLI_LOGD("PSRAM speed_dma (512KB HPDMA copy, 1M buf PSRAM->PSRAM): %lu.%02lu MB/s\r\n",
				(unsigned long)(c100 / 100), (unsigned long)(c100 % 100));
		}
		msg = CLI_CMD_RSP_SUCCEED;
#else
		CLI_LOGE("speed_dma: need HPDMA (CONFIG_PSRAM_TEST_USE_HPDMA)\r\n");
		msg = CLI_CMD_RSP_ERROR;
#endif
	}
	else if (os_strcmp(argv[1], "read") == 0)
	{
		if (argc < 4)
		{
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}

		addr = os_strtoul(argv[2], NULL, 16);
		length = os_strtoul(argv[3], NULL, 10);
		if (length & 0x3)
		{
			length = ((length >> 2) + 1) << 2;
		}

		if (length == 0)
		{
			msg = CLI_CMD_RSP_ERROR;
		}
		else
		{
			uint8_t *src = (uint8_t *)addr;

			for (i = 0; i < length; i++)
			{
				if ((i % 32) == 0)
				{
					CLI_LOGD("\r\n");
				}

				CLI_LOGD("%02x ", *(src + i));
			}
			CLI_LOGD("\r\n");
			msg = CLI_CMD_RSP_SUCCEED;
		}
	}
	else if (os_strcmp(argv[1], "cache") == 0)
	{
		if (argc < 4)
		{
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}

		addr = os_strtoul(argv[2], NULL, 16);
		length = os_strtoul(argv[3], NULL, 10);
		if (length & 0x3)
		{
			length = ((length >> 2) + 1) << 2;
		}

		if (length == 0)
		{
			msg = CLI_CMD_RSP_ERROR;
		}
		else
		{
#if (CONFIG_DCACHE)
			arch_dcache_flush_and_invd_range((uint8_t *)addr, length);
#endif
			msg = CLI_CMD_RSP_SUCCEED;
		}
	}
	else if (os_strcmp(argv[1], "cover_write") == 0)
	{
		int ret = 0;
		uint32_t start = 0, end = 0;
		uint8_t enable = 0;
		uint8_t id = 0;

		if (argc < 4)
		{
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}

		id = os_strtoul(argv[2], NULL, 10);

		if (os_strcmp(argv[3], "1") == 0)
		{
			if (argc < 6)
			{
				msg = CLI_CMD_RSP_ERROR;
				goto out;
			}
			enable = 1;
			start = os_strtoul(argv[4], NULL, 16);
			end = os_strtoul(argv[5], NULL, 16);
		}

		else
			enable = 0;

		if (enable)
			ret = bk_psram_enable_write_through(id, start, end);
		else
			ret = bk_psram_disable_write_through(id);

		if (ret == BK_OK)
			msg = CLI_CMD_RSP_SUCCEED;
		else
			msg = CLI_CMD_RSP_ERROR;
	}
	else if (os_strcmp(argv[1], "deinit") == 0)
	{
		/*init psram*/
		bk_psram_deinit();
		msg = CLI_CMD_RSP_SUCCEED;
	}
#if (CONFIG_PSRAM_WRITE_THROUGH)
	else if (os_strcmp(argv[1], "wt_start") == 0)
	{
		/* psram_test_ext wt_start <start_addr_hex> <size> [psram_id]
		 * Start a task that continuously: CPU write (pattern increments each round), DMA read, compare.
		 * start_addr/size must be 64B aligned. psram_id: 0=PSRAM0, 1=PSRAM1.
		 */
		uint32_t start_addr, size, psram_id = 0;
		psram_write_through_area_t area;

		if (argc < 4) {
			CLI_LOGE("usage: psram_test_ext wt_start <start_addr_hex> <size> [psram_id]\r\n");
			CLI_LOGE("  start_addr/size: 64B aligned. psram_id: 0|1\r\n");
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}
		if (s_wt_task_hdl != NULL) {
			CLI_LOGE("wt test task already running, call wt_stop first\r\n");
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}

		start_addr = os_strtoul(argv[2], NULL, 16);
		size = os_strtoul(argv[3], NULL, 0);  /* 0: accept decimal or 0x hex */
		if (argc >= 5)
			psram_id = (uint8_t)os_strtoul(argv[4], NULL, 10);

		if ((start_addr & 63) || (size & 63) || size == 0) {
			CLI_LOGE("start_addr and size must be 64-byte aligned\r\n");
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}
		if (psram_id > 1) {
			CLI_LOGE("psram_id must be 0 or 1\r\n");
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}

		area = bk_psram_alloc_write_through_channel_with_psram_id(psram_id);
		if (area >= PSRAM_WRITE_THROUGH_AREA_COUNT) {
			CLI_LOGE("alloc write-through channel failed\r\n");
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}
		if (bk_psram_enable_write_through(area, start_addr, start_addr + size) != BK_OK) {
			CLI_LOGE("enable write-through failed\r\n");
			bk_psram_free_write_through_channel(area);
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}

		s_wt_ctx.start_addr = start_addr;
		s_wt_ctx.size = size;
		s_wt_ctx.area = area;
		s_wt_ctx.psram_id = psram_id;
		s_wt_ctx.running = 1;

		if (rtos_create_thread(&s_wt_task_hdl, 4, "psram_wt",
			(beken_thread_function_t)psram_wt_task_main, 4 * 1024, (beken_thread_arg_t)NULL) != BK_OK) {
			CLI_LOGE("wt task create failed\r\n");
			bk_psram_disable_write_through(area);
			bk_psram_free_write_through_channel(area);
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}
#if (CONFIG_PSRAM_INTERLEAVE)
		CLI_LOGD("wt test task started: start=0x%08lx size=%lu (interleave)\r\n",
			(unsigned long)start_addr, (unsigned long)size);
#else
		CLI_LOGD("wt test task started: start=0x%08lx size=%lu psram_id=%u\r\n",
			(unsigned long)start_addr, (unsigned long)size, (unsigned int)psram_id);
#endif
		msg = CLI_CMD_RSP_SUCCEED;
	}
	else if (os_strcmp(argv[1], "wt_verify_512k") == 0)
	{
		/* Start background task: loop 512K write/copy/verify (psram_malloc 1M). Print only on error (max 10). Stop with wt_verify_stop. */
#define WT_VERIFY_512K_SIZE  (512 * 1024)
#define WT_VERIFY_1M_SIZE    (1024 * 1024)
		uint32_t start_addr;
		uint8_t psram_id = 0;

		if (s_wt_task_hdl != NULL || s_wt_verify_task_hdl != NULL) {
			CLI_LOGE("wt or wt_verify task already running, call wt_stop or wt_verify_stop first\r\n");
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}

		s_wt_verify_ctx.malloc_base = psram_malloc(WT_VERIFY_1M_SIZE + 64);
		if (s_wt_verify_ctx.malloc_base == NULL) {
			CLI_LOGE("wt_verify_512k: psram_malloc(1M+64) failed\r\n");
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}
		start_addr = (uint32_t)(((uintptr_t)s_wt_verify_ctx.malloc_base + 63) & ~(uintptr_t)63);

#if defined(SOC_PSRAM1_DATA_BASE)
		psram_id = (start_addr >= SOC_PSRAM1_DATA_BASE) ? 1 : 0;
#endif
		if (argc >= 3)
			psram_id = (uint8_t)os_strtoul(argv[2], NULL, 10);
		if (psram_id > 1) {
			CLI_LOGE("psram_id must be 0 or 1\r\n");
			psram_free(s_wt_verify_ctx.malloc_base);
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}

		s_wt_verify_ctx.area = bk_psram_alloc_write_through_channel_with_psram_id(psram_id);
		if (s_wt_verify_ctx.area >= PSRAM_WRITE_THROUGH_AREA_COUNT) {
			CLI_LOGE("alloc write-through channel failed\r\n");
			psram_free(s_wt_verify_ctx.malloc_base);
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}
		if (bk_psram_enable_write_through(s_wt_verify_ctx.area, start_addr, start_addr + WT_VERIFY_1M_SIZE) != BK_OK) {
			CLI_LOGE("enable write-through failed\r\n");
			bk_psram_free_write_through_channel(s_wt_verify_ctx.area);
			psram_free(s_wt_verify_ctx.malloc_base);
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}

		s_wt_verify_ctx.running = 1;
		s_wt_verify_ctx.mode = WTV_MODE_512K;
		s_wt_verify_ctx.start_addr = start_addr;
		s_wt_verify_ctx.size_a = WT_VERIFY_512K_SIZE;
		s_wt_verify_ctx.total_size = WT_VERIFY_1M_SIZE;
		s_wt_verify_ctx.psram_id = psram_id;
		if (rtos_create_thread(&s_wt_verify_task_hdl, 4, "psram_wt_verify",
			(beken_thread_function_t)psram_wt_verify_task_main, 4 * 1024, (beken_thread_arg_t)NULL) != BK_OK) {
			CLI_LOGE("wt_verify_512k: create task failed\r\n");
			bk_psram_disable_write_through(s_wt_verify_ctx.area);
			bk_psram_free_write_through_channel(s_wt_verify_ctx.area);
			psram_free(s_wt_verify_ctx.malloc_base);
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}
		CLI_LOGD("wt_verify_512k task started (loop), stop with wt_verify_stop\r\n");
		msg = CLI_CMD_RSP_SUCCEED;
#undef WT_VERIFY_512K_SIZE
#undef WT_VERIFY_1M_SIZE
	}
	else if (os_strcmp(argv[1], "wt_verify_8k") == 0)
	{
		/* Start background task: loop 8K write/copy/verify (psram_malloc 16K). Print only on error (max 10). Stop with wt_verify_stop. */
#define WT_VERIFY_8K_SIZE   (8 * 1024)
#define WT_VERIFY_16K_SIZE (16 * 1024)
		uint32_t start_addr;
		uint8_t psram_id = 0;

		if (s_wt_task_hdl != NULL || s_wt_verify_task_hdl != NULL) {
			CLI_LOGE("wt or wt_verify task already running, call wt_stop or wt_verify_stop first\r\n");
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}

		s_wt_verify_ctx.malloc_base = psram_malloc(WT_VERIFY_16K_SIZE + 64);
		if (s_wt_verify_ctx.malloc_base == NULL) {
			CLI_LOGE("wt_verify_8k: psram_malloc(16K+64) failed\r\n");
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}
		start_addr = (uint32_t)(((uintptr_t)s_wt_verify_ctx.malloc_base + 63) & ~(uintptr_t)63);

#if defined(SOC_PSRAM1_DATA_BASE)
		psram_id = (start_addr >= SOC_PSRAM1_DATA_BASE) ? 1 : 0;
#endif
		if (argc >= 3)
			psram_id = (uint8_t)os_strtoul(argv[2], NULL, 10);
		if (psram_id > 1) {
			CLI_LOGE("psram_id must be 0 or 1\r\n");
			psram_free(s_wt_verify_ctx.malloc_base);
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}

		s_wt_verify_ctx.area = bk_psram_alloc_write_through_channel_with_psram_id(psram_id);
		if (s_wt_verify_ctx.area >= PSRAM_WRITE_THROUGH_AREA_COUNT) {
			CLI_LOGE("alloc write-through channel failed\r\n");
			psram_free(s_wt_verify_ctx.malloc_base);
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}
		if (bk_psram_enable_write_through(s_wt_verify_ctx.area, start_addr, start_addr + WT_VERIFY_16K_SIZE) != BK_OK) {
			CLI_LOGE("enable write-through failed\r\n");
			bk_psram_free_write_through_channel(s_wt_verify_ctx.area);
			psram_free(s_wt_verify_ctx.malloc_base);
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}

		s_wt_verify_ctx.running = 1;
		s_wt_verify_ctx.mode = WTV_MODE_8K;
		s_wt_verify_ctx.start_addr = start_addr;
		s_wt_verify_ctx.size_a = WT_VERIFY_8K_SIZE;
		s_wt_verify_ctx.total_size = WT_VERIFY_16K_SIZE;
		s_wt_verify_ctx.psram_id = psram_id;
		if (rtos_create_thread(&s_wt_verify_task_hdl, 4, "psram_wt_verify",
			(beken_thread_function_t)psram_wt_verify_task_main, 4 * 1024, (beken_thread_arg_t)NULL) != BK_OK) {
			CLI_LOGE("wt_verify_8k: create task failed\r\n");
			bk_psram_disable_write_through(s_wt_verify_ctx.area);
			bk_psram_free_write_through_channel(s_wt_verify_ctx.area);
			psram_free(s_wt_verify_ctx.malloc_base);
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}
		CLI_LOGD("wt_verify_8k task started (loop), stop with wt_verify_stop\r\n");
		msg = CLI_CMD_RSP_SUCCEED;
#undef WT_VERIFY_8K_SIZE
#undef WT_VERIFY_16K_SIZE
	}
	else if (os_strcmp(argv[1], "wt_verify_stop") == 0)
	{
		if (s_wt_verify_task_hdl == NULL) {
			CLI_LOGD("wt_verify task not running\r\n");
			msg = CLI_CMD_RSP_SUCCEED;
			goto out;
		}
		s_wt_verify_ctx.running = 0;
		while (s_wt_verify_task_hdl != NULL)
			rtos_delay_milliseconds(10);
		os_memset(&s_wt_verify_ctx, 0, sizeof(s_wt_verify_ctx));
		CLI_LOGD("wt_verify task stopped, write-through disabled, channel and psram_malloc freed\r\n");
		msg = CLI_CMD_RSP_SUCCEED;
	}
	else if (os_strcmp(argv[1], "wt_stop") == 0)
	{
		/* Stop the wt test task, disable write-through, free channel and resources. */
		if (s_wt_task_hdl == NULL) {
			CLI_LOGD("wt test task not running\r\n");
			msg = CLI_CMD_RSP_SUCCEED;
			goto out;
		}
		s_wt_ctx.running = 0;
		while (s_wt_task_hdl != NULL)
			rtos_delay_milliseconds(10);
		bk_psram_disable_write_through(s_wt_ctx.area);
		bk_psram_free_write_through_channel(s_wt_ctx.area);
		os_memset(&s_wt_ctx, 0, sizeof(s_wt_ctx));
		CLI_LOGD("wt test task stopped, write-through disabled, channel freed\r\n");
		msg = CLI_CMD_RSP_SUCCEED;
	}
#endif
	else if (os_strcmp(argv[1], "cpu_dma_verify") == 0)
	{
		extern void psram_cdv_start(uint32_t half_size_kb);
		uint32_t half_kb = 0;
		if (argc >= 3)
			half_kb = os_strtoul(argv[2], NULL, 10);
		psram_cdv_start(half_kb);
		msg = CLI_CMD_RSP_SUCCEED;
	}
	else if (os_strcmp(argv[1], "cpu_dma_verify_stop") == 0)
	{
		extern void psram_cdv_stop(void);
		psram_cdv_stop();
		msg = CLI_CMD_RSP_SUCCEED;
	}
	else if (os_strcmp(argv[1], "stack_stress_start") == 0)
	{
		extern void psram_stack_stress_start(void);
		psram_stack_stress_start();
		msg = CLI_CMD_RSP_SUCCEED;
	}
	else if (os_strcmp(argv[1], "stack_stress_stop") == 0)
	{
		extern void psram_stack_stress_stop(void);
		psram_stack_stress_stop();
		msg = CLI_CMD_RSP_SUCCEED;
	}
	else if (os_strcmp(argv[1], "psram_soak_start") == 0)
	{
		extern void psram_soak_start(uint32_t soak_ms);
		uint32_t soak_ms = 30000;
		if (argc >= 3)
			soak_ms = os_strtoul(argv[2], NULL, 10);
		psram_soak_start(soak_ms);
		msg = CLI_CMD_RSP_SUCCEED;
	}
	else if (os_strcmp(argv[1], "psram_soak_stop") == 0)
	{
		extern void psram_soak_stop(void);
		psram_soak_stop();
		msg = CLI_CMD_RSP_SUCCEED;
	}
	else if (os_strcmp(argv[1], "psram_rapid_start") == 0)
	{
		extern void psram_rapid_start(void);
		psram_rapid_start();
		msg = CLI_CMD_RSP_SUCCEED;
	}
	else if (os_strcmp(argv[1], "psram_rapid_stop") == 0)
	{
		extern void psram_rapid_stop(void);
		psram_rapid_stop();
		msg = CLI_CMD_RSP_SUCCEED;
	}

out:
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}


/* ========== cpu_dma_verify: background task ========== */

typedef struct {
	volatile uint8_t running;
	uint32_t half_size;
	volatile uint32_t pass_count;
	volatile uint32_t fail_count;
	beken_thread_t task_hdl;
} cdv_ctx_t;

static cdv_ctx_t s_cdv_ctx = {0};

#define CDV_DMA_CHUNK 65535u

static void cdv_task_main(void *arg)
{
	uint32_t half_size = s_cdv_ctx.half_size;
	uint32_t src_base = CONFIG_PSRAM_TEST_CPU_DMA_ADDR;
	uint32_t dst_base = src_base + half_size;
	uint64_t last_status_time = bk_get_current_timer();

	CLI_LOGD("cpu_dma_verify started: src=[%08x-%08x] dst=[%08x-%08x] half=%uKB\r\n",
		src_base, src_base + half_size, dst_base, dst_base + half_size, half_size / 1024);

	while (s_cdv_ctx.running) {
		uint32_t error_num = 0;
		bk_err_t dma_ret = BK_OK;

		for (uint32_t idx = 0; idx < half_size / 4; idx++)
			write_data(src_base + idx * 4, (uint32_t)(idx * 0x10004001 + 0xA5A5A5A5));

		{
			uint32_t offset = 0;
			while (offset < half_size && dma_ret == BK_OK) {
				uint32_t chunk = half_size - offset;
				if (chunk > CDV_DMA_CHUNK)
					chunk = CDV_DMA_CHUNK;
#if (PSRAM_TEST_USE_HPDMA_EFF)
				dma_ret = bk_hpdma_memcpy((void *)(dst_base + offset),
					(const void *)(src_base + offset), chunk);
#else
				dma_ret = dma_memcpy((void *)(dst_base + offset),
					(const void *)(src_base + offset), chunk);
#endif
				offset += chunk;
			}
		}

		if (dma_ret != BK_OK) {
			s_cdv_ctx.fail_count++;
			CLI_LOGE("cpu_dma_verify: DMA copy failed (%d), fail=%u\r\n", dma_ret, s_cdv_ctx.fail_count);
			rtos_delay_milliseconds(100);
			continue;
		}

		for (uint32_t idx = 0; idx < half_size / 4; idx++) {
			uint32_t expect = (uint32_t)(idx * 0x10004001 + 0xA5A5A5A5);
			uint32_t actual = get_addr_data(dst_base + idx * 4);
			if (actual != expect) {
				error_num++;
				if (error_num <= 10)
					CLI_LOGE("cpu_dma_verify ERR @%08x: got %08x expect %08x xor %08x\r\n",
						dst_base + idx * 4, actual, expect, actual ^ expect);
			}
		}

		if (error_num > 0) {
			s_cdv_ctx.fail_count++;
			CLI_LOGE("cpu_dma_verify FAIL: error_num=%u (pass=%u, fail=%u)\r\n",
				error_num, s_cdv_ctx.pass_count, s_cdv_ctx.fail_count);
		} else {
			s_cdv_ctx.pass_count++;
		}

		uint64_t now = bk_get_current_timer();
		if (now - last_status_time >= 5000000) {
			CLI_LOGD("cpu_dma_verify running [%08x-%08x]: pass=%u, fail=%u\r\n",
				CONFIG_PSRAM_TEST_CPU_DMA_ADDR,
				CONFIG_PSRAM_TEST_CPU_DMA_ADDR + CONFIG_PSRAM_TEST_CPU_DMA_SIZE,
				s_cdv_ctx.pass_count, s_cdv_ctx.fail_count);
			last_status_time = now;
		}

		rtos_delay_milliseconds(10);
	}

	CLI_LOGD("cpu_dma_verify stopped: pass=%u, fail=%u\r\n", s_cdv_ctx.pass_count, s_cdv_ctx.fail_count);
	s_cdv_ctx.task_hdl = NULL;
	rtos_delete_thread(NULL);
}

void psram_cdv_start(uint32_t half_size_kb)
{
	if (s_cdv_ctx.running) {
		CLI_LOGD("cpu_dma_verify already running\r\n");
		return;
	}
	os_memset(&s_cdv_ctx, 0, sizeof(s_cdv_ctx));

	uint32_t max_half = CONFIG_PSRAM_TEST_CPU_DMA_SIZE / 2;
	uint32_t half = max_half;
	if (half_size_kb > 0 && (half_size_kb * 1024) <= max_half)
		half = half_size_kb * 1024;

	s_cdv_ctx.half_size = half;
	s_cdv_ctx.running = 1;

	bk_err_t ret = rtos_create_thread(&s_cdv_ctx.task_hdl,
		4, "cdv_task",
		(beken_thread_function_t)cdv_task_main,
		4 * 1024,
		(beken_thread_arg_t)NULL);
	if (ret != BK_OK) {
		CLI_LOGE("cpu_dma_verify: create task failed %d\r\n", ret);
		s_cdv_ctx.running = 0;
		s_cdv_ctx.task_hdl = NULL;
	}
}

void psram_cdv_stop(void)
{
	if (!s_cdv_ctx.running) {
		CLI_LOGD("cpu_dma_verify not running\r\n");
		return;
	}
	s_cdv_ctx.running = 0;
	while (s_cdv_ctx.task_hdl != NULL)
		rtos_delay_milliseconds(10);
	CLI_LOGD("cpu_dma_verify stopped: pass=%u, fail=%u\r\n", s_cdv_ctx.pass_count, s_cdv_ctx.fail_count);
}


/* ========== stack_stress: PSRAM-stack parent + 10 PSRAM-stack workers ========== */

#define STACK_STRESS_WORKER_CNT     10
#define STACK_STRESS_LOCAL_BUF_SZ   (4 * 1024)
#define STACK_STRESS_WORKER_STACK   (48 * 1024)
#define STACK_STRESS_PARENT_STACK   (16 * 1024)

typedef struct {
	volatile uint8_t running;
	volatile uint32_t error_count;
	volatile uint32_t completed_rounds;
	volatile uint8_t worker_done[STACK_STRESS_WORKER_CNT];
	beken_thread_t parent_hdl;
} stack_stress_ctx_t;

static stack_stress_ctx_t s_ss_ctx = {0};

static void stack_stress_worker(void *arg)
{
	uint32_t packed = (uint32_t)(uintptr_t)arg;
	uint32_t slot_id = packed & 0xFF;
	uint32_t worker_id = packed >> 8;
	uint8_t buf_a[STACK_STRESS_LOCAL_BUF_SZ];
	uint8_t buf_b[STACK_STRESS_LOCAL_BUF_SZ];

	for (uint32_t i = 0; i < STACK_STRESS_LOCAL_BUF_SZ; i++)
		buf_a[i] = (uint8_t)(i + worker_id * 37 + 0x5A);

	os_memcpy(buf_b, buf_a, STACK_STRESS_LOCAL_BUF_SZ);

	for (uint32_t i = 0; i < STACK_STRESS_LOCAL_BUF_SZ; i++) {
		if (buf_a[i] != buf_b[i]) {
			s_ss_ctx.error_count++;
			CLI_LOGE("worker%u: mismatch @%u: %02x vs %02x\r\n",
				worker_id, i, buf_a[i], buf_b[i]);
			break;
		}
	}

	if (slot_id < STACK_STRESS_WORKER_CNT)
		s_ss_ctx.worker_done[slot_id] = 1;

	while (1)
		rtos_delay_milliseconds(1000);
}

static uint32_t ss_simple_rand(uint32_t *seed)
{
	*seed = (*seed) * 1103515245u + 12345u;
	return (*seed >> 16) & 0x7FFF;
}

static int ss_generate_cd_sequence(uint8_t *seq, uint32_t *seed)
{
	for (int retry = 0; retry < 200; retry++) {
		uint8_t tmp[STACK_STRESS_WORKER_CNT * 2];
		for (int i = 0; i < STACK_STRESS_WORKER_CNT; i++) {
			tmp[i] = 'C';
			tmp[i + STACK_STRESS_WORKER_CNT] = 'D';
		}
		int n = STACK_STRESS_WORKER_CNT * 2;
		for (int i = n - 1; i > 0; i--) {
			int j = ss_simple_rand(seed) % (i + 1);
			uint8_t t = tmp[i]; tmp[i] = tmp[j]; tmp[j] = t;
		}
		int depth = 0, valid = 1;
		for (int i = 0; i < n; i++) {
			if (tmp[i] == 'C') depth++;
			else depth--;
			if (depth < 0) { valid = 0; break; }
		}
		if (valid && depth == 0) {
			os_memcpy(seq, tmp, n);
			return 0;
		}
	}
	return -1;
}

static void stack_stress_parent(void *arg)
{
	beken_thread_t workers[STACK_STRESS_WORKER_CNT] = {NULL};
	uint32_t seed = (uint32_t)bk_get_current_timer() ^ 0xDEADBEEF;
	uint8_t cd_seq[STACK_STRESS_WORKER_CNT * 2];
	int total_steps = STACK_STRESS_WORKER_CNT * 2;
	uint64_t last_status_time = bk_get_current_timer();

	CLI_LOGD("stack_stress started: task_region [%08x-%08x] (%uKB), worker_stack=%uKB, local_buf=%uKB\r\n",
		CONFIG_PSRAM_TEST_TASK_ADDR,
		CONFIG_PSRAM_TEST_TASK_ADDR + CONFIG_PSRAM_TEST_TASK_SIZE,
		CONFIG_PSRAM_TEST_TASK_SIZE / 1024,
		STACK_STRESS_WORKER_STACK / 1024,
		STACK_STRESS_LOCAL_BUF_SZ / 1024);

	while (s_ss_ctx.running) {
		if (ss_generate_cd_sequence(cd_seq, &seed) != 0) {
			CLI_LOGE("stack_stress: failed to generate CD sequence\r\n");
			rtos_delay_milliseconds(100);
			continue;
		}

		os_memset((void *)s_ss_ctx.worker_done, 0, sizeof(s_ss_ctx.worker_done));

		int create_idx = 0, destroy_idx = 0;
		for (int step = 0; step < total_steps && s_ss_ctx.running; step++) {
			if (cd_seq[step] == 'C') {
				int slot = -1;
				for (int s = 0; s < STACK_STRESS_WORKER_CNT; s++) {
					if (workers[s] == NULL) { slot = s; break; }
				}
				if (slot < 0) {
					CLI_LOGE("stack_stress: no free slot (bug)\r\n");
					break;
				}
				char name[12];
				s_ss_ctx.worker_done[slot] = 0;
				os_snprintf(name, sizeof(name), "ss_w%d", slot);
				uint32_t packed_arg = ((s_ss_ctx.completed_rounds * STACK_STRESS_WORKER_CNT + slot) << 8) | (slot & 0xFF);
				bk_err_t ret = rtos_create_psram_thread(&workers[slot],
					5, name,
					(beken_thread_function_t)stack_stress_worker,
					STACK_STRESS_WORKER_STACK,
					(beken_thread_arg_t)(uintptr_t)packed_arg);
				if (ret != BK_OK) {
					CLI_LOGE("stack_stress: create %s failed %d\r\n", name, ret);
					workers[slot] = NULL;
				} else {
					CLI_LOGD("stack_stress: create %s (round=%u)\r\n", name, s_ss_ctx.completed_rounds);
				}
				create_idx++;
				rtos_delay_milliseconds(50 + ss_simple_rand(&seed) % 100);
			} else {
				int slot = -1;
				for (int s = 0; s < STACK_STRESS_WORKER_CNT; s++) {
					if (workers[s] != NULL) { slot = s; break; }
				}
				if (slot < 0) {
					CLI_LOGE("stack_stress: no active slot (bug)\r\n");
					break;
				}
				while (!s_ss_ctx.worker_done[slot])
					rtos_delay_milliseconds(10);
				CLI_LOGD("stack_stress: destroy ss_w%d (round=%u)\r\n", slot, s_ss_ctx.completed_rounds);
				rtos_delete_thread(&workers[slot]);
				workers[slot] = NULL;
				destroy_idx++;
			}
		}

		for (int s = 0; s < STACK_STRESS_WORKER_CNT; s++) {
			if (workers[s] != NULL) {
				while (!s_ss_ctx.worker_done[s])
					rtos_delay_milliseconds(10);
				CLI_LOGD("stack_stress: cleanup ss_w%d\r\n", s);
				rtos_delete_thread(&workers[s]);
				workers[s] = NULL;
			}
		}

		s_ss_ctx.completed_rounds++;

		uint64_t now = bk_get_current_timer();
		if (now - last_status_time >= 5000000) {
			CLI_LOGD("stack_stress running [%08x-%08x]: rounds=%u, errors=%u\r\n",
				CONFIG_PSRAM_TEST_TASK_ADDR,
				CONFIG_PSRAM_TEST_TASK_ADDR + CONFIG_PSRAM_TEST_TASK_SIZE,
				s_ss_ctx.completed_rounds, s_ss_ctx.error_count);
			last_status_time = now;
		}
	}

	CLI_LOGD("stack_stress stopped: rounds=%u, errors=%u\r\n",
		s_ss_ctx.completed_rounds, s_ss_ctx.error_count);
	s_ss_ctx.parent_hdl = NULL;
	rtos_delete_thread(NULL);
}

void psram_stack_stress_start(void)
{
	if (s_ss_ctx.running) {
		CLI_LOGD("stack_stress already running\r\n");
		return;
	}
	os_memset(&s_ss_ctx, 0, sizeof(s_ss_ctx));
	s_ss_ctx.running = 1;

	bk_err_t ret = rtos_create_psram_thread(&s_ss_ctx.parent_hdl,
		4, "ss_parent",
		(beken_thread_function_t)stack_stress_parent,
		STACK_STRESS_PARENT_STACK,
		(beken_thread_arg_t)NULL);
	if (ret != BK_OK) {
		CLI_LOGE("stack_stress: create parent failed %d\r\n", ret);
		s_ss_ctx.running = 0;
		s_ss_ctx.parent_hdl = NULL;
	}
}

void psram_stack_stress_stop(void)
{
	if (!s_ss_ctx.running) {
		CLI_LOGD("stack_stress not running\r\n");
		return;
	}
	s_ss_ctx.running = 0;
	while (s_ss_ctx.parent_hdl != NULL)
		rtos_delay_milliseconds(10);
	CLI_LOGD("stack_stress stopped: rounds=%u, errors=%u\r\n",
		s_ss_ctx.completed_rounds, s_ss_ctx.error_count);
}


/* ========== psram_soak: write-soak-readback bit-flip detection ========== */

static const uint32_t s_soak_patterns[] = {
	0x55555555, 0xAAAAAAAA,
	0x00000000, 0xFFFFFFFF,
	0x12345678, 0xA5A5A5A5,
	0x0F0F0F0F, 0xF0F0F0F0,
};
#define SOAK_PATTERN_CNT  (sizeof(s_soak_patterns) / sizeof(s_soak_patterns[0]))
#define SOAK_TASK_STACK   (8 * 1024)

typedef struct {
	volatile uint8_t running;
	volatile uint32_t pass_count;
	volatile uint32_t fail_count;
	volatile uint32_t bitflip_words;
	uint32_t soak_ms;
	beken_thread_t task_hdl;
} soak_ctx_t;

static soak_ctx_t s_soak_ctx = {0};

static void soak_task_main(void *arg)
{
	uint32_t base_addr = CONFIG_PSRAM_TEST_TASK_ADDR;
	uint32_t region_size = CONFIG_PSRAM_TEST_TASK_SIZE;
	uint32_t words = region_size / 4;
	volatile uint32_t *p32 = (volatile uint32_t *)base_addr;
	uint32_t soak_ms = s_soak_ctx.soak_ms;
	uint32_t pat_idx = 0;
	uint64_t last_status_time = bk_get_current_timer();

	CLI_LOGD("psram_soak started: region [%08x-%08x] (%uKB), soak_ms=%u, patterns=%u\r\n",
		base_addr, base_addr + region_size, region_size / 1024, soak_ms, SOAK_PATTERN_CNT);

	while (s_soak_ctx.running) {
		uint32_t pat = s_soak_patterns[pat_idx % SOAK_PATTERN_CNT];

		for (uint32_t i = 0; i < words; i++)
			p32[i] = pat ^ i;

		rtos_delay_milliseconds(soak_ms);

		uint32_t error_num = 0;
		for (uint32_t i = 0; i < words; i++) {
			uint32_t actual = p32[i];
			uint32_t expect = pat ^ i;
			if (actual != expect) {
				error_num++;
				if (error_num <= 10)
					CLI_LOGE("soak bit-flip @%08x: got %08x expect %08x xor %08x (pat[%u]=%08x)\r\n",
						base_addr + i * 4, actual, expect, actual ^ expect, pat_idx % SOAK_PATTERN_CNT, pat);
			}
		}

		if (error_num > 0) {
			s_soak_ctx.fail_count++;
			s_soak_ctx.bitflip_words += error_num;
			CLI_LOGE("soak FAIL pat[%u]=%08x: %u words flipped (total_pass=%u, total_fail=%u, total_flip=%u)\r\n",
				pat_idx % SOAK_PATTERN_CNT, pat, error_num,
				s_soak_ctx.pass_count, s_soak_ctx.fail_count, s_soak_ctx.bitflip_words);
		} else {
			s_soak_ctx.pass_count++;
		}

		uint64_t now = bk_get_current_timer();
		if (now - last_status_time >= 5000000) {
			CLI_LOGD("psram_soak running [%08x-%08x]: pat[%u]=%08x, pass=%u, fail=%u, flip_words=%u\r\n",
				base_addr, base_addr + region_size,
				pat_idx % SOAK_PATTERN_CNT, pat,
				s_soak_ctx.pass_count, s_soak_ctx.fail_count, s_soak_ctx.bitflip_words);
			last_status_time = now;
		}

		pat_idx++;
	}

	CLI_LOGD("psram_soak stopped: pass=%u, fail=%u, flip_words=%u\r\n",
		s_soak_ctx.pass_count, s_soak_ctx.fail_count, s_soak_ctx.bitflip_words);
	s_soak_ctx.task_hdl = NULL;
	rtos_delete_thread(NULL);
}

void psram_soak_start(uint32_t soak_ms)
{
	if (s_soak_ctx.running) {
		CLI_LOGD("psram_soak already running\r\n");
		return;
	}
	os_memset(&s_soak_ctx, 0, sizeof(s_soak_ctx));
	s_soak_ctx.running = 1;
	s_soak_ctx.soak_ms = soak_ms ? soak_ms : 30000;

	bk_err_t ret = rtos_create_thread(&s_soak_ctx.task_hdl,
		5, "soak_task",
		(beken_thread_function_t)soak_task_main,
		SOAK_TASK_STACK,
		(beken_thread_arg_t)NULL);
	if (ret != BK_OK) {
		CLI_LOGE("psram_soak: create task failed %d\r\n", ret);
		s_soak_ctx.running = 0;
		s_soak_ctx.task_hdl = NULL;
	}
}

void psram_soak_stop(void)
{
	if (!s_soak_ctx.running) {
		CLI_LOGD("psram_soak not running\r\n");
		return;
	}
	s_soak_ctx.running = 0;
	while (s_soak_ctx.task_hdl != NULL)
		rtos_delay_milliseconds(10);
	CLI_LOGD("psram_soak stopped: pass=%u, fail=%u, flip_words=%u\r\n",
		s_soak_ctx.pass_count, s_soak_ctx.fail_count, s_soak_ctx.bitflip_words);
}


/* ========== psram_rapid: fast write-then-read pattern verify ========== */

#define RAPID_TASK_STACK  (8 * 1024)

typedef struct {
	volatile uint8_t running;
	volatile uint32_t pass_count;
	volatile uint32_t fail_count;
	volatile uint32_t bitflip_words;
	beken_thread_t task_hdl;
} rapid_ctx_t;

static rapid_ctx_t s_rapid_ctx = {0};

static void rapid_task_main(void *arg)
{
	uint32_t base_addr = CONFIG_PSRAM_TEST_TASK_ADDR;
	uint32_t region_size = CONFIG_PSRAM_TEST_TASK_SIZE;
	uint32_t words = region_size / 4;
	volatile uint32_t *p32 = (volatile uint32_t *)base_addr;
	uint32_t pat_idx = 0;
	uint64_t last_status_time = bk_get_current_timer();

	CLI_LOGD("psram_rapid started: region [%08x-%08x] (%uKB), patterns=%u\r\n",
		base_addr, base_addr + region_size, region_size / 1024, SOAK_PATTERN_CNT);

	while (s_rapid_ctx.running) {
		uint32_t pat = s_soak_patterns[pat_idx % SOAK_PATTERN_CNT];

		for (uint32_t i = 0; i < words; i++)
			p32[i] = pat ^ i;

		uint32_t error_num = 0;
		for (uint32_t i = 0; i < words; i++) {
			uint32_t actual = p32[i];
			uint32_t expect = pat ^ i;
			if (actual != expect) {
				error_num++;
				if (error_num <= 10)
					CLI_LOGE("rapid err @%08x: got %08x expect %08x xor %08x (pat[%u]=%08x)\r\n",
						base_addr + i * 4, actual, expect, actual ^ expect,
						pat_idx % SOAK_PATTERN_CNT, pat);
			}
		}

		if (error_num > 0) {
			s_rapid_ctx.fail_count++;
			s_rapid_ctx.bitflip_words += error_num;
			CLI_LOGE("rapid FAIL pat[%u]=%08x: %u words err (pass=%u, fail=%u, total_err=%u)\r\n",
				pat_idx % SOAK_PATTERN_CNT, pat, error_num,
				s_rapid_ctx.pass_count, s_rapid_ctx.fail_count, s_rapid_ctx.bitflip_words);
		} else {
			s_rapid_ctx.pass_count++;
		}

		uint64_t now = bk_get_current_timer();
		if (now - last_status_time >= 5000000) {
			CLI_LOGD("psram_rapid running [%08x-%08x]: pat[%u]=%08x, pass=%u, fail=%u, err_words=%u\r\n",
				base_addr, base_addr + region_size,
				pat_idx % SOAK_PATTERN_CNT, pat,
				s_rapid_ctx.pass_count, s_rapid_ctx.fail_count, s_rapid_ctx.bitflip_words);
			last_status_time = now;
		}

		pat_idx++;

		/* Yield one tick so the IDLE task can run and feed task_wdt; otherwise
		 * this busy loop starves IDLE on its core and triggers task_wdt assert
		 * after CONFIG_TASK_WDT_PERIOD_MS. */
		rtos_delay_milliseconds(1);
	}

	CLI_LOGD("psram_rapid stopped: pass=%u, fail=%u, err_words=%u\r\n",
		s_rapid_ctx.pass_count, s_rapid_ctx.fail_count, s_rapid_ctx.bitflip_words);
	s_rapid_ctx.task_hdl = NULL;
	rtos_delete_thread(NULL);
}

void psram_rapid_start(void)
{
	if (s_rapid_ctx.running) {
		CLI_LOGD("psram_rapid already running\r\n");
		return;
	}
	if (s_soak_ctx.running) {
		CLI_LOGE("psram_rapid: cannot start while psram_soak is running (shared region)\r\n");
		return;
	}
	os_memset(&s_rapid_ctx, 0, sizeof(s_rapid_ctx));
	s_rapid_ctx.running = 1;

	bk_err_t ret = rtos_create_thread(&s_rapid_ctx.task_hdl,
		5, "rapid_task",
		(beken_thread_function_t)rapid_task_main,
		RAPID_TASK_STACK,
		(beken_thread_arg_t)NULL);
	if (ret != BK_OK) {
		CLI_LOGE("psram_rapid: create task failed %d\r\n", ret);
		s_rapid_ctx.running = 0;
		s_rapid_ctx.task_hdl = NULL;
	}
}

void psram_rapid_stop(void)
{
	if (!s_rapid_ctx.running) {
		CLI_LOGD("psram_rapid not running\r\n");
		return;
	}
	s_rapid_ctx.running = 0;
	while (s_rapid_ctx.task_hdl != NULL)
		rtos_delay_milliseconds(10);
	CLI_LOGD("psram_rapid stopped: pass=%u, fail=%u, err_words=%u\r\n",
		s_rapid_ctx.pass_count, s_rapid_ctx.fail_count, s_rapid_ctx.bitflip_words);
}


beken_thread_t  psram_task_hdl = NULL;

static void psram_task_main(void)
{
	while (1) {
		rtos_delay_milliseconds(3000);
		CLI_LOGD("psram_task_main is running.\r\n");
	}

	psram_task_hdl = NULL;
	rtos_delete_thread(NULL);
}

static void cli_create_psram_task_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;

	if (!psram_task_hdl)
	{
		ret = rtos_create_psram_thread(&psram_task_hdl,
								 4,
								 "psram_task",
								 (beken_thread_function_t)psram_task_main,
								 4 * 1024,
								 (beken_thread_arg_t)NULL);
		if (ret != BK_OK)
		{
			psram_task_hdl = NULL;
			CLI_LOGE("Error: Failed to create psram test task: %d\r\n", ret);
			return;
		}

		return;
	}
}

static void cli_delete_psram_task_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if(psram_task_hdl) {
		rtos_delete_thread(&psram_task_hdl);
		psram_task_hdl = NULL;
	}
}


#define PSRAM_CNT (sizeof(s_psram_commands) / sizeof(struct cli_command))
DRV_CLI_CMD_EXPORT static const struct cli_command s_psram_commands[] = {
	{"psram_test_ext", "init|byte|word|rewrite|read|speed|speed_dma|timer_check|deinit|wt_start|wt_verify_8k|wt_verify_512k|wt_verify_stop|wt_stop|cpu_dma_verify|cpu_dma_verify_stop|stack_stress_start|stack_stress_stop|psram_soak_start|psram_soak_stop|psram_rapid_start|psram_rapid_stop", cli_psram_cmd_handle_ext},
	{"psram_test", "start|stop", cli_psram_cmd_handle},
	{"psram_cache", "psram_cache <addr> <size>", cli_test_psram_cache_cmd},
#if (CONFIG_MPC)
	{"psram_mpc", "", cli_psram_test},
#endif
	{"psram_task_create", "create task on psram", cli_create_psram_task_handle},
	{"psram_task_delete", "delete task on psram", cli_delete_psram_task_handle},
};

int cli_psram_init(void)
{
	return cli_register_module_test_feature(s_psram_commands, PSRAM_CNT);
}
// eof



