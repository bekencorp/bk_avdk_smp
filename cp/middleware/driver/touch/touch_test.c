// Copyright 2020-2021 Beken
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


#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <driver/touch.h>
#include <driver/touch_types.h>
#include "sys_driver.h"
#include "touch_driver.h"
#include "aon_pmu_driver.h"
#include <driver/timer.h>
#include "bk_saradc.h"
#include <driver/adc.h>
#include "cli.h"


float iir_x[11][3] = {0};
float iir_y[11][2] = {0};
float iir_y_x[11][3] = {0};
float iir_y_y[11][2] = {0};
uint32_t g_gain_s = 0;
UINT8 g_num = 0;
UINT8 g_touch_capa_cali_flag = 0;
beken_timer_t touch_capa_cali_tmr = {0};
beken_timer_t touch_tmr = {0};

static beken_thread_t touch_digital_tube_disp_thread_hdl = NULL;
static beken_timer_t touch_calib_print_tmr = {0};
static uint32_t g_calib_touch_id = 0;
extern void delay(int num);
extern uint32_t s_touch_channel;

static void cli_touch_help(void)
{
	TOUCH_LOGD("touch_single_channel_calib_mode_test {0|1|2|...|15} [gain_s] [vrefs] [crg] [cal_vth]\r\n");
	TOUCH_LOGD("touch_single_channel_calib_mode_stop\r\n");
	TOUCH_LOGD("touch_single_channel_manul_mode_test {0|1|...|15} {calibration_value}\r\n");
	TOUCH_LOGD("touch_multi_channel_scan_mode_test {start|stop} {0|1|2|3}\r\n");
	TOUCH_LOGD("touch_single_channel_multi_calib_test {0|1|...|15} {0|1|2|3}");
}

static void cli_touch_isr(void *param)
{
	uint32_t int_status = 0;
	int_status = bk_touch_get_int_status();
	TOUCH_LOGD("interrupt status = %x\r\n", int_status);
}

static void touch_cyclic_calib_timer_isr(timer_id_t chan)
{
	uint32_t cap_out = 0;
	uint32_t touch_id = 0;
	uint32_t touch_crg[16] = {0};
	uint32_t touch_crg_max = 0;
	uint32_t multi_chann_value = 0xffff;
	touch_config_t touch_config;

	TOUCH_LOGD("multi_channel_cyclic_calib_test start!\r\n");

	bk_touch_clear_int(multi_chann_value);
	bk_touch_int_enable(multi_chann_value, 0);
	bk_touch_scan_mode_enable(0);
	bk_touch_scan_mode_multi_channl_set(0);
	bk_touch_enable(0);

	for(touch_id = 0; touch_id < 16; touch_id++)
	{
		bk_touch_enable(1 << touch_id);

		touch_config.sensitivity_level = g_gain_s;
		touch_config.detect_threshold = TOUCH_DETECT_THRESHOLD_6;
		touch_config.detect_range = TOUCH_DETECT_RANGE_8PF;
		bk_touch_config(&touch_config);

		bk_touch_calibration_start();
		cap_out = bk_touch_get_calib_value();
		if (cap_out >= 0x1F0) {
			touch_config.detect_range = TOUCH_DETECT_RANGE_12PF;
			bk_touch_config(&touch_config);
			bk_touch_calibration_start();
			cap_out = bk_touch_get_calib_value();
			if (cap_out >= 0x1F0) {
				touch_config.detect_range = TOUCH_DETECT_RANGE_19PF;
				bk_touch_config(&touch_config);
				bk_touch_calibration_start();
				cap_out = bk_touch_get_calib_value();
				if (cap_out >= 0x1F0) {
					touch_config.detect_range = TOUCH_DETECT_RANGE_27PF;
					bk_touch_config(&touch_config);
					bk_touch_calibration_start();
					cap_out = bk_touch_get_calib_value();
					if (cap_out >= 0x1F0) {
						TOUCH_LOGE("Calibration value is out of the detect range, the channel cannot be used, please select the other channel!\r\n");
						return;
					}
				}
			}
		}
		touch_crg[touch_id] = touch_config.detect_range;
		TOUCH_LOGD("touch[%d] crg = %d, calibration value = %x !\r\n", touch_id, touch_crg[touch_id], cap_out);
		delay(1000);
	}

	for (touch_id = 0; touch_id < 16; touch_id++)
	{
		if (touch_crg_max < touch_crg[touch_id]) {
			touch_crg_max = touch_crg[touch_id];
		}
	}
	TOUCH_LOGD("touch_crg_max = %d\r\n", touch_crg_max);

	for (touch_id = 0; touch_id < 16; touch_id++)
	{
		if (touch_crg_max != touch_crg[touch_id]) {
			bk_touch_enable(1 << touch_id);
			touch_config.detect_range = touch_crg_max;
			bk_touch_config(&touch_config);
			bk_touch_calibration_start();
		}
	}
	bk_touch_scan_mode_multi_channl_set(multi_chann_value);
	bk_touch_scan_mode_enable(1);
	bk_touch_int_enable(multi_chann_value, 1);

}

static void touch_digital_tube_disp_main(void)
{
	bk_touch_digital_tube_init();

	while(1) {
		bk_touch_digital_tube_display(s_touch_channel);

	}
}

bk_err_t bk_touch_digital_tube_display_init(void)
{
	bk_err_t ret = BK_OK;

	ret = rtos_create_thread(&touch_digital_tube_disp_thread_hdl,
								BEKEN_DEFAULT_WORKER_PRIORITY,
								"touch_digital_tube_disp",
								(beken_thread_function_t)touch_digital_tube_disp_main,
								4096,
								NULL);
	if (ret != kNoErr) {
		BK_LOGD(NULL,"create touch digital tube disp task failed!\r\n");
		touch_digital_tube_disp_thread_hdl = NULL;
	}

	return ret;
}

/*
 * Usage: touch_single_channel_calib_mode_test <touch_id> [gain_s=8] [vrefs=8] [crg=2] [cal_vth=4]
 * All parameters except touch_id are optional; defaults are applied when omitted.
 * Calibration iterates detect_range from crg up to TOUCH_DETECT_RANGE_27PF if cap_out >= 0x1F0.
 */
 void cli_touch_single_channel_calib_mode_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
 {
	 uint32_t touch_id  = 0;
	 uint32_t cap_out   = 0;
	 uint32_t gain_s    = 8;   /* default: gain_s = 8 */
	 uint32_t vrefs     = 8;   /* default: vrefs  = 8 */
	 uint32_t crg       = 2;   /* default: crg    = 2 (TOUCH_DETECT_RANGE_19PF) */
	 uint32_t cal_vth   = 4;   /* default: cal_vth = 4 */

	 if (argc < 2 || argc > 6) {
		 cli_touch_help();
		 return;
	 }

	 touch_id = os_strtoul(argv[1], NULL, 10) & 0xFF;
	 if (argc >= 3) gain_s  = os_strtoul(argv[2], NULL, 10) & 0xFF;
	 if (argc >= 4) vrefs   = os_strtoul(argv[3], NULL, 10) & 0xFF;
	 if (argc >= 5) crg     = os_strtoul(argv[4], NULL, 10) & 0x03;
	 if (argc >= 6) cal_vth = os_strtoul(argv[5], NULL, 10) & 0x07;

	 if (touch_id >= 16) {
		 TOUCH_LOGE("unsupported touch channel selection command!\r\n");
		 return;
	 }

	 TOUCH_LOGD("touch calib mode test ch%d: gain_s=%d vrefs=%d crg=%d cal_vth=%d\r\n",
				touch_id, gain_s, vrefs, crg, cal_vth);

	 bk_touch_gpio_init(1 << touch_id);
	 bk_touch_enable(1 << touch_id);
	 bk_touch_register_touch_isr(1 << touch_id, cli_touch_isr, NULL);

	 /* --- Pre-calibration register setup (per hardware spec) --- */
	 /* reg60 bit31: pwd_td = 0 (power on touch analog) */
	 bk_touch_power_down(0);

	 /* reg60 bit8: rstb_dig - read and force to 0 if set */
	 if (bk_touch_rstb_dig_get()) {
		 TOUCH_LOGD("rstb_dig != 0, clearing\r\n");
		 bk_touch_rstb_dig_set(0);
	 }

	 /* reg60 bit5: ldoen - read and force to 0 if set */
	 if (bk_touch_ldoen_get()) {
		 TOUCH_LOGD("ldoen != 0, clearing\r\n");
		 bk_touch_ldoen_set(0);
	 }

	 /* reg60 bit[29:26] gain_s, bit[25:22] vrefs, bit[19:17] cal_vth */
	 bk_touch_gain_s_set(gain_s);
	 bk_touch_vrefs_set(vrefs);
	 bk_touch_cal_vth_set(cal_vth);

	 /* reg61 bit30: en_cal_auto = 0; bit28: cal_done_clr = 1 */
	 bk_touch_cal_auto_set(0);
	 bk_touch_cal_done_clr(1);

	 /* reg61 bit[7:4]: test_period = 4; bit[3:0]: test_number = 4 */
	 bk_touch_set_test_mode(4, 4);

	 /* reg62 bit[31:23]: cl_period = 0; bit[22:19]: cal_number = 2; bit18: modsel_spi = 0 */
	 bk_touch_set_calib_mode(0, 2);
	 bk_touch_mode_select_set(0);

	 bk_touch_scan_mode_enable(0);

	 /* --- Multi-range calibration: iterate crg from initial value to 3 --- */
	 do {
		 bk_touch_detect_range_set(crg);
		 bk_touch_calibration_start();
		 /* Read cap_cal_mode1 from AON PMU reg73 bit[25:17] */
		 cap_out = aon_pmu_drv_get_cap_cal();
		 TOUCH_LOGD("crg=%d, cap_out = 0x%x\r\n", crg, cap_out);

		 if (cap_out < 0x1F0) {
			 TOUCH_LOGD("touch[%d] calibration done: crg=%d, cap_out=0x%x\r\n",
						touch_id, crg, cap_out);
			 break;
		 }

		 if (crg == TOUCH_DETECT_RANGE_27PF) {
			 TOUCH_LOGE("Calibration value is out of the detect range, the channel cannot be used, please select the other channel!\r\n");
			 return;
		 }

		 crg++;
	 } while (crg <= TOUCH_DETECT_RANGE_27PF);

	 bk_touch_int_enable(1 << touch_id, 1);
 }

void cli_touch_single_channel_calib_mode_stop_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (touch_calib_print_tmr.handle != NULL) {
		rtos_deinit_timer(&touch_calib_print_tmr);
		touch_calib_print_tmr.handle = NULL;
	}

	bk_touch_int_enable(1 << g_calib_touch_id, 0);
	bk_touch_disable();
	TOUCH_LOGD("touch[%d] single channel calib mode test stopped.\r\n", g_calib_touch_id);
}

void cli_touch_single_channel_manul_mode_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t calib_value = 0;
	uint32_t cap_out = 0;
	uint32_t touch_id = 0;
	touch_config_t touch_config;

	if (argc != 3) {
		cli_touch_help();
		return;
	}

	touch_id = os_strtoul(argv[1], NULL, 16) & 0xFF;
	if(touch_id >= 0 && touch_id < 16) {
		TOUCH_LOGD("touch single channel manul mode test %d start!\r\n", touch_id);
		bk_touch_gpio_init(1 << touch_id);
		bk_touch_enable(1 <<touch_id);
		bk_touch_register_touch_isr(1 << touch_id, cli_touch_isr, NULL);

		touch_config.sensitivity_level = TOUCH_SENSITIVITY_LEVLE_3;
		touch_config.detect_threshold = TOUCH_DETECT_THRESHOLD_6;
		touch_config.detect_range = TOUCH_DETECT_RANGE_27PF;
		bk_touch_config(&touch_config);
		bk_touch_scan_mode_enable(0);

		calib_value = os_strtoul(argv[2], NULL, 16) & 0xFFF;
		TOUCH_LOGD("calib_value = %x\r\n", calib_value);
		bk_touch_manul_mode_enable(calib_value);
		bk_touch_int_enable(1 << touch_id, 1);
		cap_out = sys_drv_touch_calib_value_get();
		TOUCH_LOGD("cap_out = %x\r\n", cap_out);
		if(calib_value == cap_out) {
			TOUCH_LOGD("single channel manul mode test is successful!\r\n");
		} else {
			TOUCH_LOGE("single channel manul mode test is failed!\r\n");
			TOUCH_LOGE("please input larger calibration value!\r\n");
			bk_touch_manul_mode_disable();
			bk_touch_disable();
		}
	} else {
		TOUCH_LOGE("unsupported touch channel selection command!\r\n");
	}
}

void cli_touch_multi_channel_scan_mode_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;

	uint32_t multi_chann_value = 0xCF3F;
	uint32_t touch_crg[16] = {0};
	uint32_t touch_crg_max = 0;
	uint32_t touch_id = 0;
	uint32_t cap_out = 0;
	uint32_t gain_s = 0;
	touch_config_t touch_config;

	if (argc != 3) {
		cli_touch_help();
		return;
	}

	ret = bk_touch_digital_tube_display_init();
	if (ret != BK_OK) {
		BK_LOGD(NULL,"init touch digital tube display task failed!\r\n");
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		TOUCH_LOGD("multi_channel_scan_mode_test start!\r\n");
		gain_s = os_strtoul(argv[2], NULL, 10) & 0xFF;
		for(touch_id = 0; touch_id < 16; touch_id++)
		{
			if (touch_id == 6 || touch_id == 7 || touch_id == 13 || touch_id == 12) {
				continue;
			}
			bk_touch_gpio_init(1 << touch_id);
			bk_touch_enable(1 << touch_id);
			bk_touch_register_touch_isr(1 << touch_id, cli_touch_isr, NULL);

			touch_config.sensitivity_level = gain_s;
			touch_config.detect_threshold = TOUCH_DETECT_THRESHOLD_6;
			touch_config.detect_range = TOUCH_DETECT_RANGE_8PF;
			bk_touch_config(&touch_config);

			bk_touch_scan_mode_enable(0);
			bk_touch_calibration_start();
			cap_out = bk_touch_get_calib_value();
			if (cap_out >= 0x1F0) {
				touch_config.detect_range = TOUCH_DETECT_RANGE_12PF;
				bk_touch_config(&touch_config);
				bk_touch_calibration_start();
				cap_out = bk_touch_get_calib_value();
				if (cap_out >= 0x1F0) {
					touch_config.detect_range = TOUCH_DETECT_RANGE_19PF;
					bk_touch_config(&touch_config);
					bk_touch_calibration_start();
					cap_out = bk_touch_get_calib_value();
					if (cap_out >= 0x1F0) {
						touch_config.detect_range = TOUCH_DETECT_RANGE_27PF;
						bk_touch_config(&touch_config);
						bk_touch_calibration_start();
						cap_out = bk_touch_get_calib_value();
						if (cap_out >= 0x1F0) {
							TOUCH_LOGE("Calibration value is out of the detect range, the channel cannot be used, please select the other channel!\r\n");
							return;
						}
					}
				}
			}
			touch_crg[touch_id] = touch_config.detect_range;
			TOUCH_LOGD("touch[%d] crg = %d, calibration value = %x !\r\n", touch_id, touch_crg[touch_id], cap_out);
			delay(1000);
		}

		for (touch_id = 0; touch_id < 16; touch_id++)
		{
			if (touch_id == 6 || touch_id == 7 || touch_id == 13 || touch_id == 12) {
				continue;
			}

			if (touch_crg_max < touch_crg[touch_id]) {
				touch_crg_max = touch_crg[touch_id];
			}
		}
		TOUCH_LOGD("touch_crg_max = %d\r\n", touch_crg_max);

		for (touch_id = 0; touch_id < 16; touch_id++)
		{
			if (touch_id == 6 || touch_id == 7 || touch_id == 13 || touch_id == 12) {
				continue;
			}

			if (touch_crg_max != touch_crg[touch_id]) {
				bk_touch_enable(1 << touch_id);
				touch_config.detect_range = touch_crg_max;
				bk_touch_config(&touch_config);
				bk_touch_calibration_start();
			}
		}

		bk_touch_scan_mode_multi_channl_set(multi_chann_value);
		bk_touch_scan_mode_enable(1);
		bk_touch_int_enable(multi_chann_value, 1);
	} else if (os_strcmp(argv[1], "stop") == 0) {
		TOUCH_LOGD("multi_channel_scan_mode_test stop!\r\n");
		bk_touch_scan_mode_enable(0);
		bk_touch_disable();
	}
}

void cli_touch_single_channel_multi_calib_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t touch_id = 0;
	uint32_t cap_out = 0;
	uint32_t gain_s = 0;
	uint32_t count = 0;
	touch_config_t touch_config;

	if (argc != 3) {
		cli_touch_help();
		return;
	}

	touch_id = os_strtoul(argv[1], NULL, 10) & 0xFF;
	gain_s = os_strtoul(argv[2], NULL, 10) & 0xFF;
	if(touch_id >= 0 && touch_id < 16) {
		TOUCH_LOGD("touch single channel calib mode test %d start!\r\n", touch_id);
		bk_touch_gpio_init(1 << touch_id);
		bk_touch_enable(1 << touch_id);
		bk_touch_register_touch_isr(1 << touch_id, cli_touch_isr, NULL);

		touch_config.sensitivity_level = gain_s;
		touch_config.detect_threshold = TOUCH_DETECT_THRESHOLD_6;
		touch_config.detect_range = TOUCH_DETECT_RANGE_8PF;
		bk_touch_config(&touch_config);

		bk_touch_scan_mode_enable(0);
		bk_touch_calibration_start();
		cap_out = bk_touch_get_calib_value();
		TOUCH_LOGD("cap_out0 = %x\r\n", cap_out);
		if (cap_out >= 0x1F0) {
			touch_config.detect_range = TOUCH_DETECT_RANGE_12PF;
			bk_touch_config(&touch_config);
			bk_touch_calibration_start();
			cap_out = bk_touch_get_calib_value();
			TOUCH_LOGD("cap_out1 = %x\r\n", cap_out);
			if (cap_out >= 0x1F0) {
				touch_config.detect_range = TOUCH_DETECT_RANGE_19PF;
				bk_touch_config(&touch_config);
				bk_touch_calibration_start();
				cap_out = bk_touch_get_calib_value();
				TOUCH_LOGD("cap_out2 = %x\r\n", cap_out);
				if (cap_out >= 0x1F0) {
					touch_config.detect_range = TOUCH_DETECT_RANGE_27PF;
					bk_touch_config(&touch_config);
					bk_touch_calibration_start();
					cap_out = bk_touch_get_calib_value();
					TOUCH_LOGD("cap_out3 = %x\r\n", cap_out);
					if (cap_out >= 0x1F0) {
						TOUCH_LOGE("Calibration value is out of the detect range, the channel cannot be used, please select the other channel!\r\n");
						return;
					}
				}
			}
		}

		for (count = 0; count < 5000; count++)
		{
			bk_touch_calibration_start();
			cap_out = bk_touch_get_calib_value();
			TOUCH_LOGD("cap_out = %x\r\n", cap_out);
			delay(10000);
		}

		bk_touch_int_enable(1 << touch_id, 1);
	} else {
		TOUCH_LOGE("unsupported touch channel selection command!\r\n");
	}
}

void cli_touch_multi_channel_cyclic_calib_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret = 0;
	uint32_t multi_chann_value = 0xffff;
	uint32_t touch_crg[16] = {0};
	uint32_t touch_crg_max = 0;
	uint32_t touch_id = 0;
	uint32_t cap_out = 0;
	touch_config_t touch_config;

	if (argc != 3) {
		cli_touch_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		g_gain_s = os_strtoul(argv[2], NULL, 10) & 0xFF;
		for(touch_id = 0; touch_id < 16; touch_id++)
		{
			bk_touch_gpio_init(1 << touch_id);
			bk_touch_enable(1 << touch_id);
			bk_touch_register_touch_isr(1 << touch_id, cli_touch_isr, NULL);

			touch_config.sensitivity_level = g_gain_s;
			touch_config.detect_threshold = TOUCH_DETECT_THRESHOLD_6;
			touch_config.detect_range = TOUCH_DETECT_RANGE_8PF;
			bk_touch_config(&touch_config);

			bk_touch_scan_mode_enable(0);
			bk_touch_calibration_start();
			cap_out = bk_touch_get_calib_value();
			if (cap_out >= 0x1F0) {
				touch_config.detect_range = TOUCH_DETECT_RANGE_12PF;
				bk_touch_config(&touch_config);
				bk_touch_calibration_start();
				cap_out = bk_touch_get_calib_value();
				if (cap_out >= 0x1F0) {
					touch_config.detect_range = TOUCH_DETECT_RANGE_19PF;
					bk_touch_config(&touch_config);
					bk_touch_calibration_start();
					cap_out = bk_touch_get_calib_value();
					if (cap_out >= 0x1F0) {
						touch_config.detect_range = TOUCH_DETECT_RANGE_27PF;
						bk_touch_config(&touch_config);
						bk_touch_calibration_start();
						cap_out = bk_touch_get_calib_value();
						if (cap_out >= 0x1F0) {
							TOUCH_LOGE("Calibration value is out of the detect range, the channel cannot be used, please select the other channel!\r\n");
							return;
						}
					}
				}
			}
			touch_crg[touch_id] = touch_config.detect_range;
			TOUCH_LOGD("touch[%d] crg = %d, calibration value = %x !\r\n", touch_id, touch_crg[touch_id], cap_out);
			delay(1000);
		}

		for (touch_id = 0; touch_id < 16; touch_id++)
		{
			if (touch_crg_max < touch_crg[touch_id]) {
				touch_crg_max = touch_crg[touch_id];
			}
		}
		TOUCH_LOGD("touch_crg_max = %d\r\n", touch_crg_max);

		for (touch_id = 0; touch_id < 16; touch_id++)
		{
			if (touch_crg_max != touch_crg[touch_id]) {
				bk_touch_enable(1 << touch_id);
				touch_config.detect_range = touch_crg_max;
				bk_touch_config(&touch_config);
				bk_touch_calibration_start();
			}
		}

		bk_touch_scan_mode_multi_channl_set(multi_chann_value);
		bk_touch_scan_mode_enable(1);
		bk_touch_int_enable(multi_chann_value, 1);


		ret = bk_timer_start(TIMER_ID1, 10000, touch_cyclic_calib_timer_isr);
		if (ret != BK_OK) {
			BK_LOGD(NULL,"Timer start failed\r\n");
		}
	}
}

#if CONFIG_SOC_BK7256XX
void touch_push(float *data_buff, float data, UINT8 num)
{
    UINT8 i;

    for(i=0;i<num;i++)
    {
        data_buff[i] = data_buff[i+1];
    }
    data_buff[num] = data;
}

void touch_saradc_iir_iir_fillter(UINT8 chan_idx)
{
    UINT16 value = 0;
    UINT32 sum = 0;
    float touch_delt[11] = {-300,-300,-300,-300,-300,-300,-300,-300,-300,-300,-300};
    uint32_t chan[11] = {2,3,4,5,8,9,10,11,12,14,15};
    float x=0.0,y1=0.0,y2=0.0,b1=1.0,b2=0.0,b3=-1.0,a1=1.0,a2=-1.8945,a3=0.9037,a4=-1.5515,a5=0.6755,s1=0.1588,s2=0.1588;

    aon_pmu_drv_touch_select(chan[chan_idx]);
    for(UINT8 i = 0; i<= 1; i++)
    {
        BK_LOG_ON_ERR(bk_adc_read(&value, ADC_READ_SEMAPHORE_WAIT_TIME));
        value = value << 1;
        sum += value;
    }

    x = ((float)sum)/2;
    //IIR1
    touch_push(iir_x[chan_idx], x, 2);
    y1 = (((b1*iir_x[chan_idx][2] + b2*iir_x[chan_idx][1] + b3*iir_x[chan_idx][0]) - a2 * iir_y[chan_idx][1] - a3 * iir_y[chan_idx][0])/a1);

    touch_push(iir_y[chan_idx], y1, 1);

    //IIR2
    y1 = y1*s1;
    touch_push(iir_y_x[chan_idx], y1, 2);
    y2 = (((b1*iir_y_x[chan_idx][2] + b2*iir_y_x[chan_idx][1] + b3*iir_y_x[chan_idx][0]) - a4 * iir_y_y[chan_idx][1] - a5 * iir_y_y[chan_idx][0])/a1);

    touch_push(iir_y_y[chan_idx], y2, 1);
    y2 = y2*s2;
    //TOUCH_LOGD("y2=%f,touch_chan=%d,num=%d\r\n",y2,chan_idx,num);

    //initial oscilation
    if(g_num >= 100)
    {
        //The y2 is negative
        if ((y2 < touch_delt[chan_idx]))
        {
            s_touch_channel = chan[chan_idx];
        }
    }
    rtos_delay_milliseconds(2);
}

void touch_capa_cali(void *param)
{
    uint32_t touch_chan = 2;
    UINT32 cap_out      = 0;
    int ret             = 0;
    uint32_t chan[11]   = {2,3,4,5,8,9,10,11,12,14,15};
    UINT8 j             = 0;
    touch_config_t touch_config;

    if(g_touch_capa_cali_flag == 0)
    {
        g_touch_capa_cali_flag = 1;
        for(j = 0; j < 11; j++)
        {
            touch_chan = chan[j];
            bk_touch_gpio_init(1 << touch_chan);
            bk_touch_enable(1 << touch_chan);
            bk_touch_scan_mode_enable(0);

            touch_config.sensitivity_level = TOUCH_SENSITIVITY_LEVLE_0;
            touch_config.detect_threshold = TOUCH_DETECT_THRESHOLD_6;
            touch_config.detect_range = TOUCH_DETECT_RANGE_8PF;
            bk_touch_config(&touch_config);

            bk_touch_calibration_start();
            cap_out = bk_touch_get_calib_value();
            //TOUCH_LOGD("cap_out=%d,touch_chan=%d\r\n",cap_out,chan[j]);
            bk_touch_manul_mode_enable(cap_out);
            bk_touch_manul_mode_disable();
        }
        if (touch_capa_cali_tmr.handle != NULL)
        {
            ret = rtos_reload_timer(&touch_capa_cali_tmr);
            BK_ASSERT(kNoErr == ret);
        }
        g_touch_capa_cali_flag = 0;
    }
    else
    {
        TOUCH_LOGD("touch saradc task executing!\r\n");
    }
}

void touch_adc_get(void *param)
{
    UINT8 j;
    int err;

    g_num += 1;
    if(g_touch_capa_cali_flag == 0)
    {
        g_touch_capa_cali_flag = 2;
        BK_LOG_ON_ERR(bk_adc_acquire());
        sys_drv_set_ana_pwd_gadc_buf(0);
        BK_LOG_ON_ERR(bk_adc_init(ADC_9));
        adc_config_t config = {0};

        config.chan = ADC_9;
        config.adc_mode = 3;
        config.src_clk = 1;
        config.clk = 0x31975;
        config.saturate_mode = 4;
        config.steady_ctrl= 7;
        config.adc_filter = 0;
        if(config.adc_mode == ADC_CONTINUOUS_MODE)
        {
            config.sample_rate = 0;
        }

        BK_LOG_ON_ERR(bk_adc_set_config(&config));
        BK_LOG_ON_ERR(bk_adc_enable_bypass_clalibration());
        BK_LOG_ON_ERR(bk_adc_start());

        for(j = 0; j < 11; j++)
        {
            touch_saradc_iir_iir_fillter(j);
        }
        if(g_num >= 100)
            g_num -= 1;
        bk_adc_stop();
        bk_adc_deinit(ADC_9);
        bk_adc_release();
        if (touch_tmr.handle != NULL)
        {
            err = rtos_reload_timer(&touch_tmr);
            BK_ASSERT(kNoErr == err);
        }
        g_touch_capa_cali_flag = 0;
    }
}

void cli_touch_adc_mode_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (os_strcmp(argv[1], "cali") == 0)
    {
        UINT32 t_ms = 5000;
        int err;

        if (touch_capa_cali_tmr.handle != NULL)
        {
            err = rtos_deinit_timer(&touch_capa_cali_tmr);
            BK_ASSERT(kNoErr == err);
            touch_capa_cali_tmr.handle = NULL;
        }

        err = rtos_init_timer(&touch_capa_cali_tmr,
                              t_ms,
                              touch_capa_cali,
                              (void *)0);
        BK_ASSERT(kNoErr == err);
        err = rtos_start_timer(&touch_capa_cali_tmr);
        BK_ASSERT(kNoErr == err);

        //enable LED
        err = bk_touch_digital_tube_display_init();
        if (err != BK_OK)
        {
            TOUCH_LOGD("init touch digital tube display task failed!\r\n");
            return;
        }
    }
    else if(os_strcmp(argv[1], "test") == 0)
    {
        UINT32 t_ms = 100;
        int err;

        if (touch_tmr.handle != NULL)
        {
            err = rtos_deinit_timer(&touch_tmr);
            BK_ASSERT(kNoErr == err);
            touch_tmr.handle = NULL;
        }

        err = rtos_init_timer(&touch_tmr,
                              t_ms,
                              touch_adc_get,
                              (void *)0);
        BK_ASSERT(kNoErr == err);
        err = rtos_start_timer(&touch_tmr);
        BK_ASSERT(kNoErr == err);
    }
}
#endif

#define TOUCH_CMD_CNT	(sizeof(s_touch_commands) / sizeof(struct cli_command))

DRV_CLI_CMD_EXPORT static const struct cli_command s_touch_commands[] = {
    {"touch_single_channel_calib_mode_test", "touch_single_channel_calib_mode_test {0|1|...|15} [gain_s] [vrefs] [crg] [cal_vth]", cli_touch_single_channel_calib_mode_test_cmd},
    {"touch_single_channel_calib_mode_stop", "stop single channel calib mode test", cli_touch_single_channel_calib_mode_stop_cmd},
    {"touch_single_channel_manul_mode_test", "touch_single_channel_manul_mode_test {0|1|...|15} {calibration_value}", cli_touch_single_channel_manul_mode_test_cmd},
    {"touch_multi_channel_scan_mode_test", "touch_multi_channel_scan_mode_test {start|stop} {0|1|2|3}", cli_touch_multi_channel_scan_mode_test_cmd},
    {"touch_single_channel_multi_calib_test", "touch_single_channel_multi_calib_test {0|1|...|15} {0|1|2|3}", cli_touch_single_channel_multi_calib_test_cmd},
    {"touch_multi_channel_cyclic_calib_test", "touch_multi_channel_cyclic_calib_test {start|stop} {0|1|2|3}", cli_touch_multi_channel_cyclic_calib_test_cmd},
};

int bk_touch_register_cli_test_feature(void)
{
	return cli_register_module_test_feature(s_touch_commands, TOUCH_CMD_CNT);
}
//eof
