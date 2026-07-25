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
#include <components/bk_platform.h>
#include "bk_general_dma.h"
#include <driver/dma.h>
#include "soc/mapping.h"
#include "psram_hal.h"
#include "cache.h"
#include "bk_sensor_internal.h"
#include "aon_pmu_ll.h"
#include "sys_ll.h"
#include "sys_ahbp_ll.h"
#include "sys_driver.h"
#include "bk_misc.h"

#if (CONFIG_PSRAM_AUTO_DETECT)
#include "bk_ef.h"
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
	CLI_LOGD("psram_test {start|stop}\n");
}

#define PSRAM_TEST_LEN               (1024 * 4)
#define PSRAM_TEST_MAX_INSTANCE      (2)

static beken_thread_t  psram_thread_hdl[PSRAM_TEST_MAX_INSTANCE] = {NULL};

typedef struct {
	uint8_t test_running;
	uint8_t test_mode;
	uint8_t cacheable;
	uint8_t dma_channel;
	bool silent_mode;
	uint8_t psram_id; /* 0: PSRAM0, 1: PSRAM1 */
	uint32_t start_addr;
	uint32_t test_size;
	uint32_t psram_total_size;
	uint32_t length;
	uint32_t *data;
	uint32_t delay_time;
	uint32_t id;
	uint32_t data_type;
} psram_debug_t;

static psram_debug_t *psram_debug[PSRAM_TEST_MAX_INSTANCE] = {NULL};

static inline uint32_t psram_test_get_base_addr(uint8_t psram_id)
{
	if (psram_id == 0) {
#if defined(SOC_PSRAM0_DATA_BASE)
		return SOC_PSRAM0_DATA_BASE;
#else
		return SOC_PSRAM_DATA_BASE;
#endif
	}

#if defined(SOC_PSRAM1_DATA_BASE)
	return SOC_PSRAM1_DATA_BASE;
#else
	/* If SOC doesn't define PSRAM1, fallback to PSRAM0 base */
	return SOC_PSRAM_DATA_BASE;
#endif
}

static uint64_t bk_get_current_timer(void)
{
	uint64_t timer = 0;

#ifdef CONFIG_ARCH_RISCV
	timer = riscv_get_mtimer();// tick
#else // CONFIG_ARCH_RISCV

#if (CONFIG_AON_RTC || CONFIG_ANA_RTC)
	timer = bk_aon_rtc_get_us();
#endif

#endif // CONFIG_ARCH_RISCV

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

static void psram_scan_test(psram_debug_t *ctx)
{
	/* NOTE: scan test implementation was commented out historically.
	 * Provide a minimal implementation to avoid implicit declaration error
	 * when PSRAM test mode == 6.
	 */
	CLI_LOGW("psram_scan_test is not enabled, skip.\r\n");
	rtos_delay_milliseconds(ctx ? ctx->delay_time : 1);
}

static void psram_cpu_write_test(psram_debug_t *ctx)
{
	uint32_t i = 0;
#if TEST_PSRAM_ACCURACY
	uint32_t j = 0;
	uint32_t error_num = 0;
#endif

	uint64_t rate = 0;
	uint64_t timer0, timer1;
	uint64_t total_time = 0;
	uint32_t value = 0;
	uint32_t base_addr = psram_test_get_base_addr(ctx->psram_id);
	bool silent_mode = ctx->silent_mode;

#if (CONFIG_PSRAM_APS128XXO_OB9)
	uint32_t test_len = 1024 * 1024 * 16;
#elif (CONFIG_PSRAM_W955D8MKY_5J)
	uint32_t test_len = 1024 * 1024 * 4;
#else //CONFIG_PSRAM_APS6408L_O
	uint32_t test_len = 1024 * 1024 * 8;
#endif

	if(!silent_mode)
		CLI_LOGD("begin write %08x-%08x test\r\n", base_addr, base_addr + test_len);

	timer0 = bk_get_current_timer();

#if TEST_PSRAM_ACCURACY
	for (j = 0; j < test_len / ctx->length; j ++)
	{
		for (i = 0; i < ctx->length; i += 4)
			write_data((base_addr + i + (j << 15)), ctx->data[(i >> 2) & 0x1FFF]);
	}
#else
	for (i = 0; i < test_len; i +=4)
	{
		write_data(base_addr + i, 0x11223344);
	}
#endif

	timer1 = bk_get_current_timer();

	if (timer1 > timer0)
	{
		total_time = bk_get_spend_time_us(timer0, timer1);
		rate = ((uint64_t)test_len) * 1000000 / total_time;
		if(!silent_mode)
		{
			CLI_LOGD("finish write, use time: %ld ms, write_rate:%ld%ld byte/s\r\n", (uint32_t)(total_time / 1000),
			(uint32_t)(rate >> 32), (uint32_t)(rate & 0xFFFFFFFF));
		}
	}

	if(!silent_mode)
		CLI_LOGD("begin read %08x-%08x test\r\n", base_addr, base_addr + test_len);

	timer0 = bk_get_current_timer();

	for (i = 0; i < test_len / 4; i++)
		read_data((base_addr + i * 0x4), value);

	timer1 = bk_get_current_timer();
	if (timer1 > timer0)
	{
		total_time = bk_get_spend_time_us(timer0, timer1);
		rate = ((uint64_t)test_len) * 1000000 / total_time;
		if(!silent_mode)
		{
			CLI_LOGD("finish read, use time: %ld ms, read_rate:%d%ld byte/s\r\n", (uint32_t)(total_time / 1000),
			(uint32_t)(rate >> 32), (uint32_t)(rate & 0xFFFFFFFF));
		}
	}

#if TEST_PSRAM_ACCURACY
	for (i = 0; i < test_len / 4; i++)
	{
		value = get_addr_data(base_addr + i * 0x4);
		if (value != (ctx->data[i & 0x1FFF]))
		{
			if(!silent_mode)
			{
				CLI_LOGD("==========%08x %08x %08x %08x=======\n",base_addr + i * 0x4, value, ctx->data[i & 0x1FFF], value^ctx->data[i & 0x1FFF]);
			}
			error_num++;
		}
	}
	if(!silent_mode)
	{
		CLI_LOGD("finish compare, error_num: %ld, corr_rate: %ld.\r\n", error_num, ((test_len / 4 - error_num) * 100 / (test_len / 4)));
	}
#endif

	for (i = 0; i < test_len; i +=4)
	{
		write_data(base_addr + i, 0x0);
	}

	rtos_delay_milliseconds(ctx->delay_time);

}

static bk_err_t psram_dma_memcpy_by_chnl(psram_debug_t *ctx, void *out, const void *in, uint32_t len, dma_id_t cpy_chnl)
{
	dma_config_t dma_config = {0};

	dma_config.mode = DMA_WORK_MODE_SINGLE;
	dma_config.chan_prio = 0;

	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.src.start_addr = (uint32_t)in;
	dma_config.src.end_addr = (uint32_t)(in + len);

	dma_config.dst.dev = DMA_DEV_DTCM;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.dst.start_addr = (uint32_t)out;
	dma_config.dst.end_addr = (uint32_t)(out + len);


	bk_dma_init(cpy_chnl, &dma_config);
	bk_dma_set_transfer_len(cpy_chnl, len);
#if (CONFIG_SPE)

	if (ctx && ctx->cacheable)
	{
		bk_dma_set_src_burst_len(cpy_chnl, BURST_LEN_INC8);
		bk_dma_set_dest_burst_len(cpy_chnl, BURST_LEN_INC8);
	}

	bk_dma_set_src_sec_attr(cpy_chnl, DMA_ATTR_SEC);
	bk_dma_set_dest_sec_attr(cpy_chnl, DMA_ATTR_SEC);
#endif
	bk_dma_start(cpy_chnl);

	BK_WHILE(bk_dma_get_enable_status(cpy_chnl));

	return BK_OK;
}

static void psram_dma_write_test(psram_debug_t *ctx)
{
	uint32_t i = 0;
	uint32_t error_num = 0;
	uint64_t rate = 0;
	uint64_t timer0, timer1;
	uint32_t total_time = 0;
	uint32_t value = 0;
	uint32_t base_addr = psram_test_get_base_addr(ctx->psram_id);

#if (CONFIG_PSRAM_APS128XXO_OB9)
	uint32_t test_len = 1024 * 1024 * 16;
#elif (CONFIG_PSRAM_W955D8MKY_5J)
	uint32_t test_len = 1024 * 1024 * 4;
#else //CONFIG_PSRAM_APS6408L_O
	uint32_t test_len = 1024 * 1024 * 8;
#endif

	CLI_LOGD("begin write %08x-%08x test\r\n", base_addr, base_addr + test_len);

	timer0 = bk_get_current_timer();

	for (i = 0; i < test_len / ctx->length; i++)
	{
		psram_dma_memcpy_by_chnl(ctx, (void *)(base_addr + i * ctx->length), ctx->data, ctx->length, ctx->dma_channel);
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

	for (i = 0; i < test_len / ctx->length; i++)
	{
		for (uint32_t k = 0; k < ctx->length / 4; k++)
		{
			value = get_addr_data(base_addr + i * ctx->length + k * 0x4);

			if (value != ctx->data[k])
			{
				CLI_LOGD("==========%08x %08x %08x=======\n", value, ctx->data[k], value^ctx->data[k]);
				error_num++;
			}
		}
	}

	CLI_LOGD("finish compare, error_num: %ld, corr_rate: %ld.\r\n", error_num, ((test_len / 4 - error_num) * 100 / (test_len / 4)));

	rtos_delay_milliseconds(ctx->delay_time);

}

static void psram_write_continue_test(psram_debug_t *ctx)
{
	uint32_t i = 0;
	uint32_t error_num = 0;
	uint64_t rate = 0;
	uint64_t timer0, timer1;
	uint32_t total_time = 0;
	uint32_t value = 0;
	uint32_t base_addr = psram_test_get_base_addr(ctx->psram_id);

#if (CONFIG_PSRAM_APS128XXO_OB9)
	uint32_t test_len = 1024 * 1024 * 16;
#elif (CONFIG_PSRAM_W955D8MKY_5J)
	uint32_t test_len = 1024 * 1024 * 4;
#else //CONFIG_PSRAM_APS6408L_O
	uint32_t test_len = 1024 * 1024 * 8;
#endif

	CLI_LOGD("begin write %08x-%08x test\r\n", base_addr, base_addr + test_len);
	timer0 = bk_get_current_timer();
	for (i = 0; i < test_len; i += ctx->length)
	{
		bk_psram_memcpy((uint8_t *)(base_addr + i), (uint8_t *)&ctx->data[0], ctx->length);
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

	for (i = 0; i < test_len / ctx->length; i++)
	{
		for (uint32_t k = 0; k < ctx->length / 4; k++)
		{
			value = get_addr_data(base_addr + i * ctx->length + k * 0x4);

			if (value != ctx->data[k])
			{
				error_num++;
			}
		}
	}

	CLI_LOGD("finish compare, error_num: %ld, corr_rate: %ld.\r\n", error_num, (test_len / 4 - error_num) * 100 / (test_len / 4));

	rtos_delay_milliseconds(ctx->delay_time);

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

static void psram_calibrate_test(psram_debug_t *ctx)
{
#if CONFIG_PSRAM_CALIBRATE
	static uint32_t s_temperature = 0;
	uint32_t cur_temperature = -1;
	uint32_t err_cnt = 0;
	// uint32_t v = 0;
	uint32_t base_addr = ctx ? psram_test_get_base_addr(ctx->psram_id) : SOC_PSRAM_DATA_BASE;

	if (BK_OK != temp_detect_get_temperature(&cur_temperature)) {
		CLI_LOGE("failed to get temperature\r\n");
		return;
	}

	uint32_t diff = (cur_temperature > s_temperature) ? (cur_temperature - s_temperature) : (s_temperature - cur_temperature);
	if (diff > 100 ) { //TODO give a reasonable value
		for (int i = 0; i < 63; i++) {
			uint32_t v = (i & 7) | ((i & ~7) << 3);
			REG_WRITE(SOC_PSRAM_CAL_REG_BASE + (5 << 2), v);
			err_cnt = psram_calibrate_read_write_test(base_addr, 10240);
			if (err_cnt) {
				BK_LOGD(NULL, "%d %d %d %d %d\r\n", cur_temperature, (i & 7), (i >> 6) & 3, (i >> 8) & 3, err_cnt);
			}
		}
	}
#endif
}

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

int psram_7239n_calibrate(uint32_t calib_en, uint32_t start_addr, uint32_t test_len)
{
	uint32_t i,val;
	uint32_t err = 0;
	uint32_t cur_temperature;

	test_len /= 4;

	//initialized parameters
	//if passed,use the current parameters and no longer calibrate to save time
	psram_7239_set_reg5_value((3<<0) + (0<<3) + (2<<6) + (2<<8));

	for (i = 0; i < test_len; i++)
	{
		write_data((start_addr + i * 0x4), 0x11111111 + i);
	}
	psram_calib_delay(100);
	for (i = 0; i < test_len; i++)
	{
		val = get_addr_data(start_addr + i * 0x4);
		if (val != (0x11111111 + i))
		{
			err = 1;
		}
	}

	//test initialized parameters fail and enable calibration
	if(calib_en && err)
	{
		if (BK_OK != temp_detect_get_temperature(&cur_temperature))
		{
			CLI_LOGE("failed to get temperature\r\n");
		}

		test_len *= 4;

		if( BK_OK == psram_7239n_calibration(start_addr, test_len) )
		{
			return 0;
		}
		else
		{
			return 1;
		}

	}
	else if(err)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

static void psram_7239n_calibrate_test(psram_debug_t *ctx)
{
	uint32_t start_addr = ctx ? psram_test_get_base_addr(ctx->psram_id) : SOC_PSRAM_DATA_BASE;
	uint32_t test_len = 1024 * 1024 * 8;

	if( BK_OK == psram_7239n_calibration(start_addr, test_len) )
	{
	}
	else
	{
	}

	rtos_delay_milliseconds(ctx ? ctx->delay_time : 1);
}
#endif
static volatile uint32_t psram_7239n_total = 0;
static volatile uint32_t psram_7239n_pass = 0;
static volatile uint32_t psram_7239n_fail = 0;
static volatile uint32_t psram_7239n_unaligned_offset = 0;

static void psram_write_test_new(psram_debug_t *ctx)
{
	uint32_t i = 0;
	uint32_t j = 0;
	uint32_t k = 0;
	uint32_t error_num = 0;
	uint32_t value = 0;
	uint32_t base = ctx ? psram_test_get_base_addr(ctx->psram_id) : SOC_PSRAM_DATA_BASE;
	uint32_t base_addr = base;

//4Byte aligned
#if (CONFIG_PSRAM_APS6408L_O)
	uint32_t test_len = 1024 * 1024 * 8;
#elif (CONFIG_PSRAM_W955D8MKY_5J)
	uint32_t test_len = 1024 * 1024 * 4;
#else //CONFIG_PSRAM_APS128XXO_OB9
	uint32_t test_len = 1024 * 1024 * 16;
#endif
	ctx->length = 1024 * 32;

	// CLI_LOGD("begin write %08x-%08x 4Byte unaligned test\r\n", base_addr, base_addr + test_len);

	for (j = 0; j < test_len / ctx->length; j ++)
	{
		for (i = 0; i < ctx->length; i += 4)
			write_data((base_addr + i + (j << 15)), ctx->data[(i >> 2) & 0x1FFF]);
	}

	// CLI_LOGD("begin read %08x-%08x test\r\n", base_addr, base_addr + test_len);

	for (i = 0; i < test_len / 4; i++)
	{
		value = get_addr_data(base_addr + i * 0x4);
		if (value != (ctx->data[i & 0x1FFF]))
		{
			CLI_LOGD("4Byte align err: addr %08x, val %08x %08x %08x\n", base_addr + i * 0x4, value, ctx->data[i & 0x1FFF], value^ctx->data[i & 0x1FFF]);
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
	ctx->length = 1024 * 16;

	// CLI_LOGD("begin write %08x-%08x 4Byte unaligned test\r\n", base_addr, base_addr + test_len);

	for (j = 0; j < test_len / ctx->length; j ++)
	{
		for (i = 0; i < ctx->length; i += 4)
			write_data((base_addr + i + (j << 14)), ctx->data[(i >> 2) & 0xFFF]);
	}

	for (i = 0; i < test_len / 4; i++)
	{
		value = get_addr_data(base_addr + i * 0x4);
		if (value != (ctx->data[i & 0xFFF]))
		{
			CLI_LOGD("4Byte unlign1 err: addr %08x, val %08x %08x %08x\n", base_addr + i * 0x4, value, ctx->data[i & 0xFFF], value^ctx->data[i & 0xFFF]);
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
	ctx->length = 1024 * 32;

	// CLI_LOGD("begin write %08x-%08x 1Byte test\r\n", base_addr, base_addr + test_len);

	for (i = 0; i < test_len; i += ctx->length)
	{
		bk_psram_memcpy((uint8_t *)(base_addr + i), (uint8_t *)&ctx->data[0], ctx->length);
	}

	// CLI_LOGD("begin read %08x-%08x test\r\n", base_addr, base_addr + test_len);

	for (i = 0; i < test_len / ctx->length; i++)
	{
		for (k = 0; k < ctx->length / 4; k++)
		{
			value = get_addr_data(base_addr + i * ctx->length + k * 0x4);

			if (value != ctx->data[k])
			{
				CLI_LOGD("1Byte err: addr %08x, val %08x %08x %08x\n", base_addr + i * ctx->length + k * 0x4, value, ctx->data[k], value^ctx->data[k]);
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

	rtos_delay_milliseconds(ctx ? ctx->delay_time : 1);

}

static void psram_test_main(beken_thread_arg_t arg)
{
	psram_debug_t *ctx = (psram_debug_t *)arg;
	uint8_t psram_id = ctx ? ctx->psram_id : 0;

	while (ctx && ctx->test_running)
	{
		if (ctx->test_mode == 0)
		{
			psram_cpu_write_test(ctx);
		} else if (ctx->test_mode == 1) {
			psram_test_unit();
		} else if (ctx->test_mode == 2) {
			psram_write_continue_test(ctx);
		} else if (ctx->test_mode == 3) {
			psram_dma_write_test(ctx);
		} else if (ctx->test_mode == 4) {
			psram_calibrate_test(ctx);
		} else if (ctx->test_mode == 5) {
			psram_write_test_new(ctx);
		} else if (ctx->test_mode == 6) {
			psram_scan_test(ctx);
		} else {
			psram_calibrate_test(ctx);
		}
	}

	CLI_LOGD("psram_test task exit\n");

	if (ctx)
	{
		if (ctx->data)
		{
			os_free(ctx->data);
			ctx->data = NULL;
		}

		if (ctx->dma_channel < DMA_ID_MAX) {
			bk_dma_free(DMA_DEV_DTCM, ctx->dma_channel);
		}

		os_free(ctx);
		psram_debug[psram_id] = NULL;
	}

	psram_thread_hdl[psram_id] = NULL;
	rtos_delete_thread(NULL);
}

static bk_err_t psram_task_init(psram_debug_t *ctx)
{
	bk_err_t ret = BK_OK;
	uint8_t psram_id = ctx ? ctx->psram_id : 0;
	char name[16] = {0};

	if (psram_id >= PSRAM_TEST_MAX_INSTANCE) {
		return BK_ERR_PARAM;
	}

	if (!psram_thread_hdl[psram_id])
	{
		os_snprintf(name, sizeof(name), "psram_dbg%d", psram_id);
		ret = rtos_create_thread(&psram_thread_hdl[psram_id],
								 4,
								 name,
								 (beken_thread_function_t)psram_test_main,
								 4 * 1024,
								 (beken_thread_arg_t)ctx);
		if (ret != BK_OK)
		{
			psram_thread_hdl[psram_id] = NULL;
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
		/* Optional: select PSRAM0/PSRAM1, default PSRAM0.
		 * Reuse the existing argv layout and append [psram_id] at the end.
		 * Example:
		 *   psram_test start cpu 0 500 0 1 1   -> psram_id=1
		 */
		uint8_t psram_id = 0;
		if (argc >= 8) {
			psram_id = (uint8_t)os_strtoul(argv[7], NULL, 10);
		}
		if (psram_id > 1) {
			psram_id = 0;
		}

		bk_psram_init_with_id((psram_id_t)psram_id);

		if (psram_thread_hdl[psram_id] || psram_debug[psram_id]) {
			CLI_LOGE("psram_test instance %d is already running\r\n", psram_id);
			msg = CLI_CMD_RSP_ERROR;
			os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
			return;
		}

		psram_debug[psram_id] = (psram_debug_t *)os_malloc(sizeof(psram_debug_t));

		if (psram_debug[psram_id] == NULL)
		{
			CLI_LOGE("psram test malloc failed!\r\n");
			msg = CLI_CMD_RSP_ERROR;
			os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
			return;
		}

		os_memset(psram_debug[psram_id], 0, sizeof(psram_debug_t));
		psram_debug_t *ctx = psram_debug[psram_id];
		ctx->dma_channel = DMA_ID_MAX;

		ctx->test_running = 1;
		ctx->psram_id = psram_id;

		if (os_strcmp(argv[2], "cpu") == 0)
		{
			ctx->test_mode = 0;
		}
		else if (os_strcmp(argv[2], "conexist") == 0)
		{
			ctx->test_mode = 1;
		}
		else if (os_strcmp(argv[2], "continue_write") == 0)
		{
			ctx->test_mode = 2;
		} else if (os_strcmp(argv[2], "dma") == 0) {
			ctx->test_mode = 3;
			ctx->dma_channel = bk_dma_alloc(DMA_DEV_DTCM);
		} else if (os_strcmp(argv[2], "calibrate") == 0) {
			ctx->test_mode = 4;
		} else if (os_strcmp(argv[2], "new") == 0) {
			ctx->test_mode = 5;
		} else if (os_strcmp(argv[2], "scan") == 0) {
			ctx->test_mode = 6;
		} else {
			CLI_LOGE("unknown test mode: %s\r\n", argv[2]);
			os_free(ctx);
			psram_debug[psram_id] = NULL;
			msg = CLI_CMD_RSP_ERROR;
			os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
			return;
		}


		if(argc >= 7)
		{
			ctx->data_type = os_strtoul(argv[6], NULL, 10);
		}

		if (ctx->data == NULL)
		{
			ctx->length = 1024 * 32;
			ctx->data = (uint32_t *)os_malloc(ctx->length);
			if (ctx->data == NULL)
			{
				CLI_LOGE("malloc error!\r\n");
				os_free(ctx);
				psram_debug[psram_id] = NULL;
				return;
			}
		}

		for (int i = 0; i < ctx->length / 4; i++)
		{
			if(ctx->data_type == 1)
			{
				ctx->data[i] = bk_rand() + i;
			}
			else if (ctx->data_type == 2)
			{
				ctx->data[i] = 0xA55AA55A + i + (i << 8) + (i << 16) + (i << 24);
			}
		}

		if (os_strcmp(argv[3], "1") == 0)
		{
			ctx->cacheable = 1;
		}
		else
		{
			ctx->cacheable = 0;
		}

		ctx->delay_time = 500;

		if (argc >= 5)
		{
			ctx->delay_time = os_strtoul(argv[4], NULL, 10);

			if (ctx->delay_time == 0)
				ctx->delay_time = 500;
		}

		if(argc >= 6)
		{
			ctx->silent_mode = os_strtoul(argv[5], NULL, 10);
		}

		if (psram_task_init(ctx) != kNoErr)
		{
			CLI_LOGE("psram test failed!\r\n");
			ctx->test_running = 0;
			msg = CLI_CMD_RSP_ERROR;
			os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
			return;
		}

		CLI_LOGD("psram test start success! psram_id=%d\r\n", psram_id);
		msg = CLI_CMD_RSP_SUCCEED;
	}
	else if (os_strcmp(argv[1], "stop") == 0)
	{
		bool stop_all = true;
		uint8_t stop_id = 0;

		if (argc >= 3) {
			stop_all = false;
			stop_id = (uint8_t)os_strtoul(argv[2], NULL, 10);
			if (stop_id >= PSRAM_TEST_MAX_INSTANCE) {
				stop_all = true;
			}
		}

		for (uint8_t id = 0; id < PSRAM_TEST_MAX_INSTANCE; id++) {
			if (!stop_all && (id != stop_id)) {
				continue;
			}

			if (psram_thread_hdl[id] && psram_debug[id]) {
				psram_debug[id]->test_running = 0;
			}

			while (psram_thread_hdl[id]) {
				rtos_delay_milliseconds(10);
			}

			bk_psram_deinit_with_id((psram_id_t)id);
		}

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

#if (CONFIG_CACHE_MAINTENANCE)
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


/* ============================================================
 * psram_test_ext m55pwd_retention [psram_id] [size_kb] [retention_ms]
 *
 * Purpose:
 *   Verify PSRAM data retention while the M55 (AP) subsystem is powered
 *   down. PSRAM I/O pads are latched at 3V before the M55 LDO is cut, so
 *   external PSRAM keeps its data while M55 is dark. After re-powering
 *   M55 and unlatching the pads, the PSRAM controller is "recovered"
 *   (clock-gating bypass + soft-reset + mode-register rewrite) and the
 *   stored pattern is read back and compared.
 *
 *   The pad-latch (ana_reg5.gpio_latch) covers both PSRAM0 and PSRAM1
 *   pads, and the PSRAM controllers sit on the CP power domain, so the
 *   same test flow works for either PSRAM instance. The psram_id
 *   argument selects which one is exercised (default 0 = PSRAM0).
 *
 * Bit-level mapping (BK7259 confirmed):
 *   ana_reg5.gpio_latch     bit 7  - 1 = latch GPIO/PSRAM pad at 3V
 *   aon_pmu_r2.m55_iso_en   bit 16 - 1 = isolate M55 subsystem
 *   aon_pmu_r2.m55_rstn     bit 17 - 0 = assert M55 reset, 1 = release
 *   aon_pmu_r2.m55_clk_en   bit 20 - 1 = enable M55 clock
 *   aon_pmu_r74.por_corehs_n       - poll for HS LDO POR ready
 *   ana_reg9.pwd_hsldo      bit 13 - 1 = HS LDO power-down (M55 LDO)
 *   ana_reg16.enhspw               - HS power switch enable
 *   ana_reg16.vcorehssel           - HS LDO voltage select (0xA = 0.95V)
 *   ana_reg10.spi_latch1v   bit 9  - 1 = SPI latch 1V (gate around LDO writes)
 *   sys_ahbp_rege.pwd_m55          - M55 sub-system soft power-down hint
 *
 * psram_recovery() port (literal mapping of user's reference, PSRAM0
 * variant; PSRAM1 routes through the same sys_drv APIs with id=1):
 *   setf_PSRAM_Ckg_Bypass(PSRAMx)      -> psram_hal REG2 bit 1 = 1
 *   set_SYSTEM_Reg0x8_cksel_pram0(0)   -> sys_drv_psram_clk_sel_with_id(id, 0)
 *   set_SYSTEM_Reg0x8_ckdiv_pram0(1)   -> sys_drv_psram_set_clkdiv_with_id(id, 1)
 *   setf_SYSTEM_Reg0xA_pram0_cken      -> sys_drv_psram_disckg_with_id(id, 1)
 *   setf_PSRAM_Soft_Reset(PSRAMx)      -> psram_hal_set_sf_reset_with_id()
 *   addPSRAMx_Reg0x4 = 0xd8054049      -> psram_hal_set_mode_value_with_id()
 *
 * NOTE: the 3 clock-setup writes (cksel / ckdiv / cken) are guarded
 * by PM_PSRAM_M55PWD_RECOVER_RESET_CLOCK and skipped by default,
 * because AHBP_PSRAM stays powered through the M55 power cycle in
 * the current PM policy and the muxer state is therefore preserved.
 *
 * Test region (per PSRAM instance):
 *   PSRAM0 -> PSRAM_TEST_CPU  : 0x60000000 .. 0x60400000 (4MB)
 *   PSRAM1 -> PSRAM1_TEST_CPU : 0x64000000 .. 0x64100000 (1MB)
 *   The PSRAM1 cap is intentionally small to keep clear of
 *   AP_PSRAM_HEAP / AP_PSRAM_DATA_SECTION / AP_PSRAM_CODE_SECTION
 *   which live further inside PSRAM1.
 *   Default size = 32KB (matches the legacy reference test).
 * ============================================================ */

#define PSRAM_M55PWD_REGION0_BASE     (0x60000000U)
#define PSRAM_M55PWD_REGION0_SIZE     (4U * 1024U * 1024U)  /* PSRAM_TEST_CPU = 4MB */
#define PSRAM_M55PWD_REGION1_BASE     (0x64000000U)
#define PSRAM_M55PWD_REGION1_SIZE     (1U * 1024U * 1024U)  /* PSRAM1_TEST_CPU = 1MB */

#define PSRAM_M55PWD_DEFAULT_WORDS    (8U * 1024U)          /* 32KB, matches legacy code */
#define PSRAM_M55PWD_PRINT_ERR_MAX    (10U)
#define PSRAM_M55PWD_FLUSH_BIT        (0x1U << 3)
#define PSRAM_M55PWD_CKG_BYPASS_BIT   (0x1U << 1)
#define PSRAM_M55PWD_DEFAULT_HOLD_MS  (500U)
/* Fallback only - used when no live snapshot was taken. The normal
 * flow reads the actual PSRAM REG4 right before pad-latch and writes
 * it back in psram_m55pwd_recovery(). */
#define PSRAM_M55PWD_MODE_REG_FALLBACK  (PSRAM_MODE9)

/* See PM_PSRAM_RECOVER_RESET_CLOCK in psram_driver.c for the full
 * rationale. Mirror the same switch here so the m55pwd_retention
 * test path validates the same code shape as production retention. */
#ifndef PM_PSRAM_M55PWD_RECOVER_RESET_CLOCK
#define PM_PSRAM_M55PWD_RECOVER_RESET_CLOCK 0
#endif

static uint32_t s_psram_m55pwd_saved_mode[PSRAM_ID_MAX] = {0};
static bool     s_psram_m55pwd_mode_valid[PSRAM_ID_MAX] = {false};

static inline uint32_t psram_m55pwd_region_base(psram_id_t psram_id)
{
	return (psram_id == PSRAM_ID_1) ? PSRAM_M55PWD_REGION1_BASE
									: PSRAM_M55PWD_REGION0_BASE;
}

static inline uint32_t psram_m55pwd_region_max_words(psram_id_t psram_id)
{
	uint32_t bytes = (psram_id == PSRAM_ID_1) ? PSRAM_M55PWD_REGION1_SIZE
											  : PSRAM_M55PWD_REGION0_SIZE;
	return bytes / 4U;
}

static void psram_m55pwd_flush(psram_id_t psram_id)
{
	uint32_t v = psram_hal_get_reg8_value_with_id(psram_id);
	psram_hal_set_reg8_value_with_id(psram_id, v | PSRAM_M55PWD_FLUSH_BIT);
	while (psram_hal_get_reg8_value_with_id(psram_id) & PSRAM_M55PWD_FLUSH_BIT) {
		/* spin until controller clears flush bit */
	}
}

static void psram_m55pwd_save_mode(psram_id_t psram_id)
{
	s_psram_m55pwd_saved_mode[psram_id] = psram_hal_get_mode_value_with_id(psram_id);
	s_psram_m55pwd_mode_valid[psram_id] = true;
	CLI_LOGI("m55pwd_retention: snapshot PSRAM%d mode=0x%08x\r\n",
			 psram_id, s_psram_m55pwd_saved_mode[psram_id]);
}

static uint32_t psram_m55pwd_get_restore_mode(psram_id_t psram_id)
{
	if (s_psram_m55pwd_mode_valid[psram_id]) {
		return s_psram_m55pwd_saved_mode[psram_id];
	}
	CLI_LOGW("m55pwd_retention: no PSRAM%d mode snapshot, use fallback 0x%08x\r\n",
			 psram_id, (uint32_t)PSRAM_M55PWD_MODE_REG_FALLBACK);
	return PSRAM_M55PWD_MODE_REG_FALLBACK;
}

/*
 * Bring the M55 (AP / HS) subsystem down. The PSRAM pad must already be
 * latched at 3V before this is called.
 *
 * Sequence is the reverse of sys_hal_m55_clock_power_init() / power_ctrl()
 * in cp/middleware/soc/bk7259/hal/sys_hal.c.
 */
static void psram_m55pwd_power_down_m55(void)
{
	/* Isolate before cutting the rail. */
	aon_pmu_ll_set_r2_m55_iso_en(1);

	/* Hold M55 in reset and gate its clock to stop bus activity. */
	aon_pmu_ll_set_r2_m55_rstn(0);
	aon_pmu_ll_set_r2_m55_clk_en(0);

	/* Optional sub-system power-down hint (mirrors existing pwr-off path). */
	sys_ahbp_ll_set_rege_pwd_m55(1);

	/* Cut the M55 HS LDO. Writes are gated by spi_latch1v. */
	sys_ll_set_ana_reg10_spi_latch1v(1);
	sys_ll_set_ana_reg16_enhspw(0);
	sys_ll_set_ana_reg9_pwd_hsldo(1);
	sys_ll_set_ana_reg10_spi_latch1v(0);
}

/*
 * Bring the M55 subsystem back up. We do not reload AP firmware; the AP
 * application will not resume, but PSRAM data is the artefact we care
 * about and the M55 needs to be powered to keep the SoC stable.
 */
static void psram_m55pwd_power_up_m55(void)
{
	sys_ll_set_ana_reg10_spi_latch1v(1);
	sys_ll_set_ana_reg9_pwd_hsldo(1);
	bk_delay_us(20);
	sys_ll_set_ana_reg9_pwd_hsldo(0);
	bk_delay_us(200);
	sys_ll_set_ana_reg16_enhspw(1);
	bk_delay_us(200);
	sys_ll_set_ana_reg16_vcorehssel(0xA); /* 0.7 + 0.025 * 0xA = 0.95V */
	bk_delay_us(200);
	sys_ll_set_ana_reg10_spi_latch1v(0);

	/* Wait for HS LDO POR to assert before releasing reset / iso. */
	while (aon_pmu_ll_get_r74_por_corehs_n() == 0) {
		/* spin */
	}

	sys_ahbp_ll_set_rege_pwd_m55(0);
	aon_pmu_ll_set_r2_m55_clk_en(1);
	aon_pmu_ll_set_r2_m55_rstn(1);
	bk_delay_us(20);
	aon_pmu_ll_set_r2_m55_iso_en(0);
}

/*
 * Direct port of the reference psram_recovery():
 *   setf_PSRAM_Ckg_Bypass(BASEADDR_PSRAMx);
 *   set_SYSTEM_Reg0x8_cksel_pram0(0) / Reg0x9_cksel_pram1(0);   // 320M source
 *   set_SYSTEM_Reg0x8_ckdiv_pram0(1) / Reg0x9_ckdiv_pram1(1);   // /2 -> 160MHz
 *   setf_SYSTEM_Reg0xA_pram0_cken    / pram1_cken;
 *   setf_PSRAM_Soft_Reset(BASEADDR_PSRAMx);
 *   addPSRAMx_Reg0x4 = 0xd8054049;
 */
static void psram_m55pwd_recovery(psram_id_t psram_id)
{
	uint32_t v = 0;
	uint32_t mode = psram_m55pwd_get_restore_mode(psram_id);

	/* PSRAM REG2 bit1 = 1 : clock-gating bypass */
	v = psram_hal_get_reg2_value_with_id(psram_id);
	v |= PSRAM_M55PWD_CKG_BYPASS_BIT;
	psram_hal_set_reg2_value_with_id(psram_id, v);

#if PM_PSRAM_M55PWD_RECOVER_RESET_CLOCK
	/* PSRAMx bus clock: 320M source / (1+1) = 160MHz.
	 * sys_drv_psram_disckg_with_id() / clk_sel_with_id() /
	 * set_clkdiv_with_id() route to reg8/reg9/rega internally
	 * based on psram_id and wrap each bit-write inside a
	 * critical-section. Disabled by default since AHBP_PSRAM
	 * stays powered through the M55 power cycle in current PM
	 * policy; see PM_PSRAM_M55PWD_RECOVER_RESET_CLOCK. */
	sys_drv_psram_clk_sel_with_id((uint32_t)psram_id, 0);    /* 320M source */
	sys_drv_psram_set_clkdiv_with_id((uint32_t)psram_id, 1); /* /(1+1) -> 160MHz */
	sys_drv_psram_disckg_with_id((uint32_t)psram_id, 1);     /* bus clk enable */
#endif

	/* PSRAM REG2 bit0 = 1 : soft-reset PSRAM controller */
	psram_hal_set_sf_reset_with_id(psram_id, 1);

	/* Restore the snapshotted PSRAM mode register, so the controller
	 * comes back exactly to the state it had before retention. */
	psram_hal_set_mode_value_with_id(psram_id, mode);
	CLI_LOGI("m55pwd_retention: restore PSRAM%d mode=0x%08x\r\n", psram_id, mode);
}

static int psram_m55pwd_retention_test(psram_id_t psram_id, uint32_t test_words,
									   uint32_t retention_ms)
{
	uint32_t base_addr = psram_m55pwd_region_base(psram_id);
	uint32_t max_words = psram_m55pwd_region_max_words(psram_id);
	uint32_t i = 0;
	uint32_t err_cnt = 0;
	uint32_t printed = 0;

	if (psram_id >= PSRAM_ID_MAX) {
		CLI_LOGE("m55pwd_retention: invalid psram_id %d\r\n", psram_id);
		return -1;
	}

	if (test_words == 0) {
		test_words = PSRAM_M55PWD_DEFAULT_WORDS;
	}
	if (test_words > max_words) {
		CLI_LOGW("m55pwd_retention: clamp words %u -> %u for PSRAM%d\r\n",
				 test_words, max_words, psram_id);
		test_words = max_words;
	}
	if (retention_ms == 0) {
		retention_ms = PSRAM_M55PWD_DEFAULT_HOLD_MS;
	}

	CLI_LOGI("m55pwd_retention start: PSRAM%d region=[0x%08x-0x%08x), "
			 "words=%u (%uKB), hold=%ums\r\n",
			 psram_id, base_addr, base_addr + test_words * 4U,
			 test_words, (test_words * 4U) >> 10, retention_ms);

	/* Step 1: ensure both PSRAMs are initialized (controllers + voltage are
	 * shared, init-once is cheap; we always need PSRAM0 controller alive
	 * because the AP image lives in PSRAM1 too if CONFIG_ALL_CODE_IN_PSRAM=y). */
	if (bk_psram_init_with_id(PSRAM_ID_0) != BK_OK) {
		CLI_LOGE("m55pwd_retention: psram0 init failed\r\n");
		return -1;
	}
	if (psram_id == PSRAM_ID_1 && bk_psram_init_with_id(PSRAM_ID_1) != BK_OK) {
		CLI_LOGE("m55pwd_retention: psram1 init failed\r\n");
		return -1;
	}

	/* Step 2: write deterministic pattern. */
	for (i = 0; i < test_words; i++) {
		write_data(base_addr + i * 4U, 0x11111111U + i);
	}

	/* Step 3a: snapshot the live PSRAM mode register so the recovery
	 * path can restore the exact same controller state, regardless of
	 * which die / mode is in use. */
	psram_m55pwd_save_mode(psram_id);

	/* Step 3b: flush PSRAM controller write buffer for the instance under test. */
	psram_m55pwd_flush(psram_id);
	rtos_delay_milliseconds(5);

	CLI_LOGI("m55pwd_retention: latch psram pad 3v & power down M55\r\n");

	/* Step 4: latch PSRAM pads at 3V (ana_reg5 bit7).
	 * NOTE: gpio_latch covers both PSRAM0 and PSRAM1 pads. */
	sys_drv_set_psram_pad_latch(1);

	/* Step 5: power down M55 subsystem. */
	psram_m55pwd_power_down_m55();

	/* Step 6: hold the M55 in powered-down state. */
	rtos_delay_milliseconds(retention_ms);

	CLI_LOGI("m55pwd_retention: power M55 back up\r\n");

	/* Step 7: power M55 back up. */
	psram_m55pwd_power_up_m55();

	/* Step 8: release PSRAM pad latch. */
	sys_drv_set_psram_pad_latch(0);
	rtos_delay_milliseconds(5);

	/* Step 9: recover PSRAM controller (port of reference psram_recovery). */
	psram_m55pwd_recovery(psram_id);

	/* Step 10: read back and compare. */
	CLI_LOGI("m55pwd_retention: read back from 0x%08x\r\n", base_addr);
	for (i = 0; i < test_words; i++) {
		uint32_t val = get_addr_data(base_addr + i * 4U);
		uint32_t exp = 0x11111111U + i;
		if (val != exp) {
			if (printed < PSRAM_M55PWD_PRINT_ERR_MAX) {
				CLI_LOGE("m55pwd_retention ERR @0x%08x: got 0x%08x expect 0x%08x xor 0x%08x\r\n",
						 base_addr + i * 4U, val, exp, val ^ exp);
				printed++;
			}
			err_cnt++;
		}
	}

	if (err_cnt) {
		CLI_LOGE("m55pwd_retention FAIL: PSRAM%d error_num=%u (total=%u)\r\n",
				 psram_id, err_cnt, test_words);
		return -1;
	}

	CLI_LOGI("m55pwd_retention PASS: PSRAM%d total=%u words verified\r\n",
			 psram_id, test_words);
	return 0;
}

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
				bk_psram_set_clk_with_id(PSRAM_ID_0, PSRAM_80M);
				break;

			case 120:
				bk_psram_set_clk_with_id(PSRAM_ID_0, PSRAM_120M);
				break;

			case 160:
				bk_psram_set_clk_with_id(PSRAM_ID_0, PSRAM_160M);
				break;

			case 240:
				bk_psram_set_clk_with_id(PSRAM_ID_0, PSRAM_240M);
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
#if (CONFIG_CACHE_MAINTENANCE)
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
	else if (os_strcmp(argv[1], "m55pwd_retention") == 0)
	{
		/* psram_test_ext m55pwd_retention [psram_id] [size_kb] [retention_ms]
		 *   psram_id    : 0 = PSRAM0 (default), 1 = PSRAM1
		 *   size_kb     : test region size in KB (default 32; clamped to per-id max)
		 *   retention_ms: M55 hold-off time in ms (default 500)
		 */
		psram_id_t target_id = PSRAM_ID_0;
		uint32_t size_kb = 32;
		uint32_t retention_ms = 500;
		uint32_t test_words = 0;
		int ret = 0;

		if (argc >= 3) {
			uint32_t id_arg = os_strtoul(argv[2], NULL, 10);
			if (id_arg >= PSRAM_ID_MAX) {
				CLI_LOGE("psram_id must be 0 or 1\r\n");
				msg = CLI_CMD_RSP_ERROR;
				goto out;
			}
			target_id = (psram_id_t)id_arg;
		}
		if (argc >= 4) {
			size_kb = os_strtoul(argv[3], NULL, 10);
			if (size_kb == 0) {
				size_kb = 32;
			}
		}
		if (argc >= 5) {
			retention_ms = os_strtoul(argv[4], NULL, 10);
		}

		test_words = (size_kb * 1024) / 4;
		ret = psram_m55pwd_retention_test(target_id, test_words, retention_ms);
		msg = (ret == 0) ? CLI_CMD_RSP_SUCCEED : CLI_CMD_RSP_ERROR;
	}

out:
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
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
	{"psram_test_ext", "init|byte|word|rewirte|deinit|m55pwd_retention", cli_psram_cmd_handle_ext},
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
	return cli_register_commands(s_psram_commands, PSRAM_CNT);
}
// eof

