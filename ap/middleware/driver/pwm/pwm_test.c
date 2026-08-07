#include "cli.h"
#include <os/os.h>
#include <driver/pwm.h>
#include <components/bk_platform.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#if CONFIG_PWM

#ifndef PWM_CLOCK_SRC_XTAL
#define PWM_CLOCK_SRC_XTAL 320000000
#endif
#define _PERIOD_2_FREQ(period)    ((period == 0) ? (0) : (PWM_CLOCK_SRC_XTAL / (period)))
#define CLI_PWM_RETURN_ON_ERR(expr) do {\
	bk_err_t ret = (expr);\
	if (ret != BK_OK) {\
		CLI_LOGE("%s: ret=-0x%x\r\n", __func__, -ret);\
		return;\
	}\
} while (0)

static void cli_pwm_help(void)
{
	CLI_LOGD("pwm_driver init\n");
	CLI_LOGD("pwm_driver deinit\n");
	CLI_LOGD("pwm {chan} init {period_v} {duty_v} [duty2_v] [duty3_v] [psc]\n");
	CLI_LOGD("pwm {chan} {start|stop|deinit}\n");
	CLI_LOGD("pwm {chan} duty {period_v} {duty1_v} [duty2_v] [duty3_v] [psc]\n");
	CLI_LOGD("pwm {chan|all} duty_ramp {freq} {psc}\n");
	CLI_LOGD("pwm multi_chan_test {freq} {psc}\n");
	CLI_LOGD("pwm {chan} signal {low|high}\n");
	CLI_LOGD("pwm_group init {chan1} {chan2} {period} {chan1_duty} {chan2_duty} [psc]\n");
	CLI_LOGD("pwm_group {start|stop|deinit} [group]\n");
	CLI_LOGD("pwm_group config {group} {period} {chan1_duty} {chan2_duty}\n");
	CLI_LOGD("pwm_int {chan} {reg|enable|disable}\n");
	CLI_LOGD("pwm_capture {chan} init [pos|neg|edge]\n");
	CLI_LOGD("pwm_capture {chan} {start|stop|deinit}\n");
	CLI_LOGD("pwm_idle_test {idle_start|idle_stop}\n");
}

static void cli_pwm_timer_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
}

static void cli_pwm_counter_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
}

static void cli_pwm_carrier_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
}

static void cli_pwm_driver_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_pwm_help();
		return;
	}

	if (os_strcmp(argv[1], "init") == 0) {
		CLI_PWM_RETURN_ON_ERR(bk_pwm_driver_init());
		CLI_LOGD("pwm init\n");
	} else if (os_strcmp(argv[1], "deinit") == 0) {
		CLI_PWM_RETURN_ON_ERR(bk_pwm_driver_deinit());
		CLI_LOGD("pwm deinit\n");
	} else {
		cli_pwm_help();
		return;
	}
}

static void cli_pwm_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t chan;

	if (argc < 3) {
		cli_pwm_help();
		return;
	}

	// Handle multi_chan_test command (doesn't need channel number)
	if (os_strcmp(argv[1], "multi_chan_test") == 0) {
		// pwm multi_chan_test {freq} {psc}
		// Example: pwm multi_chan_test 5000 319
		if (argc < 4) {
			CLI_LOGD("Usage: pwm multi_chan_test {freq} {psc}\n");
			CLI_LOGD("Example: pwm multi_chan_test 5000 319\n");
			return;
		}

		uint32_t freq = os_strtoul(argv[2], NULL, 10);
		uint32_t psc = os_strtoul(argv[3], NULL, 10);
		uint32_t period_cycle = PWM_CLOCK_SRC_XTAL / freq / (psc + 1);
		uint32_t total_channels = SOC_PWM_CHAN_NUM_PER_UNIT * SOC_PWM_UNIT_NUM;
		pwm_period_duty_config_t config = {0};

		// Initialize PWM driver
		BK_LOG_ON_ERR(bk_pwm_driver_init());

		// Initialize all channels with different initial duty cycles
		pwm_init_config_t init_config = {0};
		CLI_LOGD("Initializing %d PWM channels...\n", total_channels);
		for (pwm_chan_t chan = 0; chan < total_channels; chan++) {
			// Each channel starts with a different duty cycle (5% * channel_number)
			uint32_t init_duty_percent = (chan + 1) * 5; // 5%, 10%, 15%, ..., 60%
			if (init_duty_percent > 100) {
				init_duty_percent = 100;
			}

			init_config.period_cycle = period_cycle;
			init_config.duty_cycle = (period_cycle * init_duty_percent) / 100;
			init_config.duty2_cycle = 0;
			init_config.duty3_cycle = 0;
			init_config.psc = psc;

			BK_LOG_ON_ERR(bk_pwm_init(chan, &init_config));
			BK_LOG_ON_ERR(bk_pwm_start(chan));
			CLI_LOGD("Channel %d initialized with %d%% duty cycle\n", chan, init_duty_percent);
		}

		CLI_LOGD("All channels started. Testing independent duty cycle adjustment...\n");
		CLI_LOGD("Each channel will cycle through different duty cycles independently.\n");

		// Test independent duty cycle adjustment for each channel
		// Each channel will have its own duty cycle pattern to verify independence
		uint32_t cycle_count = 0;
		while (1) {
			cycle_count++;
			CLI_LOGD("\n=== Cycle %d ===\n", cycle_count);

			// Adjust each channel independently with different patterns
			// Each channel follows a unique pattern to verify they work independently
			for (pwm_chan_t chan = 0; chan < total_channels; chan++) {
				// Each channel has a different pattern:
				// Channel 0: 0%, 10%, 20%, ..., 100%, then back to 0%
				// Channel 1: 5%, 15%, 25%, ..., 95%, then back to 5%
				// Channel 2: 0%, 8%, 16%, ..., 96%, then back to 0%
				// Channel 3: 3%, 13%, 23%, ..., 93%, then back to 3%
				// etc. - each channel has different step and offset
				uint32_t step = 8 + (chan % 3) * 2; // Different step for each channel (8, 10, 12)
				uint32_t offset = (chan % 4) * 3; // Different offset (0, 3, 6, 9)
				uint32_t duty_percent = ((cycle_count * step) + offset) % 101; // Cycle 0-100%

				config.period_cycle = period_cycle;
				config.duty_cycle = (period_cycle * duty_percent) / 100;
				config.duty2_cycle = 0;
				config.duty3_cycle = 0;
				config.psc = psc;

				BK_LOG_ON_ERR(bk_pwm_set_period_duty(chan, &config));
				CLI_LOGD("Channel %2d: duty=%3d%%, duty_cycle=%6d\n", chan, duty_percent, config.duty_cycle);
			}

			rtos_delay_milliseconds(2000); // Wait 2 seconds between cycles
		}

		return;
	}

	chan = os_strtoul(argv[1], NULL, 10);

	if (os_strcmp(argv[2], "init") == 0) {
		CLI_RET_ON_INVALID_ARGC(argc, 5);
		if (argc > 8) {
			cli_pwm_help();
			return;
		}
		pwm_init_config_t config = {0};

		config.period_cycle = os_strtoul(argv[3], NULL, 10);
		config.duty_cycle = os_strtoul(argv[4], NULL, 10);
		if (argc > 5)
			config.duty2_cycle = os_strtoul(argv[5], NULL, 10);
		if (argc > 6)
			config.duty3_cycle = os_strtoul(argv[6], NULL, 10);
		if (argc > 7)
			config.psc = os_strtoul(argv[7], NULL, 10);

		CLI_PWM_RETURN_ON_ERR(bk_pwm_init(chan, &config));
		CLI_LOGD("pwm init, chan=%d period=%x duty=%x\n", chan, config.period_cycle, config.duty_cycle);
	} else if (os_strcmp(argv[2], "start") == 0) {
		CLI_PWM_RETURN_ON_ERR(bk_pwm_start(chan));
		CLI_LOGD("pwm start, chan=%d\n", chan);
	} else if (os_strcmp(argv[2], "stop") == 0) {
		CLI_PWM_RETURN_ON_ERR(bk_pwm_stop(chan));
		CLI_LOGD("pwm stop, chan=%d\n", chan);
	} else if (os_strcmp(argv[2], "deinit") == 0) {
		CLI_PWM_RETURN_ON_ERR(bk_pwm_deinit(chan));
		CLI_LOGD("pwm deinit, chan=%d\n", chan);
	} else if (os_strcmp(argv[2], "signal") == 0) {
		if (argc != 4) {
			cli_pwm_help();
			return;
		}

		if (os_strcmp(argv[3], "low") == 0)
			CLI_PWM_RETURN_ON_ERR(bk_pwm_set_init_signal_low(chan));
		else
			CLI_PWM_RETURN_ON_ERR(bk_pwm_set_init_signal_high(chan));
		CLI_LOGD("pwm set signal, chan=%d\n", chan);
	} else if (os_strcmp(argv[2], "duty") == 0) {
		pwm_period_duty_config_t config = {0};
		if (argc < 5) {
			cli_pwm_help();
			return;
		}
		if (argc > 8) {
			cli_pwm_help();
			return;
		}

		config.period_cycle = os_strtoul(argv[3], NULL, 10);
		config.duty_cycle = os_strtoul(argv[4], NULL, 10);
		if (argc > 5)
			config.duty2_cycle = os_strtoul(argv[5], NULL, 10);
		if (argc > 6)
			config.duty3_cycle = os_strtoul(argv[6], NULL, 10);
		if (argc > 7)
			config.psc = os_strtoul(argv[7], NULL, 10);
		CLI_PWM_RETURN_ON_ERR(bk_pwm_set_period_duty(chan, &config));
		CLI_LOGD("pwm duty, chan=%d period=%d t1=%d t2=%d t3=%d\n", chan, config.period_cycle,
				 config.duty_cycle, config.duty2_cycle, config.duty3_cycle);
	} else if (os_strcmp(argv[2], "duty_ramp") == 0) {
		// pwm {chan|all} duty_ramp {freq} {psc}
		// Example: pwm 0 duty_ramp 5000 31 (single channel)
		// Example: pwm all duty_ramp 5000 31 (all channels)
		if (argc < 5) {
			CLI_LOGD("Usage: pwm {chan|all} duty_ramp {freq} {psc}\n");
			CLI_LOGD("Example: pwm 0 duty_ramp 5000 31 (single channel)\n");
			CLI_LOGD("Example: pwm all duty_ramp 5000 31 (all channels)\n");
			return;
		}

		uint32_t freq = os_strtoul(argv[3], NULL, 10);
		uint32_t psc = os_strtoul(argv[4], NULL, 10);
		uint32_t period_cycle = PWM_CLOCK_SRC_XTAL / freq / (psc + 1);
		uint32_t total_channels = SOC_PWM_CHAN_NUM_PER_UNIT * SOC_PWM_UNIT_NUM;
		pwm_period_duty_config_t config = {0};

		// Determine if testing all channels or single channel
		bool test_all_channels = (os_strcmp(argv[1], "all") == 0);
		uint32_t single_chan = 0;
		if (!test_all_channels) {
			single_chan = os_strtoul(argv[1], NULL, 10);
		}
		uint32_t start_chan = test_all_channels ? 0 : single_chan;
		uint32_t end_chan = test_all_channels ? total_channels : (single_chan + 1);

		// Initialize PWM driver if testing all channels
		if (test_all_channels) {
			BK_LOG_ON_ERR(bk_pwm_driver_init());
		}

		// Initialize PWM channels
		pwm_init_config_t init_config = {0};
		init_config.period_cycle = period_cycle;
		init_config.duty_cycle = 0;
		init_config.duty2_cycle = 0;
		init_config.duty3_cycle = 0;
		init_config.psc = psc;

		CLI_LOGD("PWM duty ramp test start, channels=%s, freq=%dHz, period_cycle=%d, psc=%d\n",
				 test_all_channels ? "all" : argv[1], freq, period_cycle, psc);

		for (pwm_chan_t ch = start_chan; ch < end_chan; ch++) {
			BK_LOG_ON_ERR(bk_pwm_init(ch, &init_config));
			BK_LOG_ON_ERR(bk_pwm_start(ch));
		}

		CLI_LOGD("Duty will cycle from 0%% to 100%% (step 2%%), then repeat from 0%%\n");

		// Ramp duty from 0% to 100%, step 2%, interval 1 second, then repeat
		while (1) {
			for (uint32_t duty_percent = 0; duty_percent <= 100; duty_percent += 2) {
				// Update all selected channels with the same duty cycle
				for (pwm_chan_t ch = start_chan; ch < end_chan; ch++) {
					config.period_cycle = period_cycle;
					config.duty_cycle = (period_cycle * duty_percent) / 100;
					config.duty2_cycle = 0;
					config.duty3_cycle = 0;
					config.psc = psc;

					BK_LOG_ON_ERR(bk_pwm_set_period_duty(ch, &config));
				}

				if (test_all_channels) {
					CLI_LOGD("All channels: duty=%d%%\n", duty_percent);
				} else {
					CLI_LOGD("Channel %d: duty=%d%%, duty_cycle=%d\n", single_chan, duty_percent, config.duty_cycle);
				}

				rtos_delay_milliseconds(1000); // Wait 1 second
			}
			// After reaching 100%, loop back to 0% in the next iteration
			CLI_LOGD("Cycle completed, restarting from 0%%\n");
		}
	} else {
		cli_pwm_help();
		return;
	}
}

static void cli_pwm_group_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t group = 0;

	if (argc < 2) {
		cli_pwm_help();
		return;
	}

	if (os_strcmp(argv[1], "init") == 0) {
		if (argc < 7 || argc > 8) {
			cli_pwm_help();
			return;
		}
		pwm_group_init_config_t config = {0};
		pwm_group_t group = 0;

		config.chan1 = os_strtoul(argv[2], NULL, 10);
		config.chan2 = os_strtoul(argv[3], NULL, 10);
		config.period_cycle = os_strtoul(argv[4], NULL, 10);
		config.chan1_duty_cycle = os_strtoul(argv[5], NULL, 10);
		config.chan2_duty_cycle = os_strtoul(argv[6], NULL, 10);
		if (argc > 7)
			config.psc = os_strtoul(argv[7], NULL, 10);
		BK_LOG_ON_ERR(bk_pwm_group_init(&config, &group));
		CLI_LOGD("pwm init, group=%d chan1=%d chan2=%d period=%x d1=%x d2=%x\n",
				 group, config.chan1, config.chan2, config.period_cycle,
				 config.chan1_duty_cycle, config.chan2_duty_cycle);
	} else if (os_strcmp(argv[1], "start") == 0) {
		if (argc > 2)
			group = os_strtoul(argv[2], NULL, 10);
		BK_LOG_ON_ERR(bk_pwm_group_start(group));
		CLI_LOGD("pwm start, group=%d\n", group);
	} else if (os_strcmp(argv[1], "stop") == 0) {
		if (argc > 2)
			group = os_strtoul(argv[2], NULL, 10);
		BK_LOG_ON_ERR(bk_pwm_group_stop(group));
		CLI_LOGD("pwm stop, group=%d\n", group);
	} else if (os_strcmp(argv[1], "deinit") == 0) {
		if (argc > 2)
			group = os_strtoul(argv[2], NULL, 10);
		BK_LOG_ON_ERR(bk_pwm_group_deinit(group));
		CLI_LOGD("pwm deinit, group=%d\n", group);
	} else if (os_strcmp(argv[1], "config") == 0) {
		pwm_group_config_t config = {0};
		if (argc != 6) {
			cli_pwm_help();
			return;
		}

		group = os_strtoul(argv[2], NULL, 10);
		config.period_cycle = os_strtoul(argv[3], NULL, 10);
		config.chan1_duty_cycle = os_strtoul(argv[4], NULL, 10);
		config.chan2_duty_cycle = os_strtoul(argv[5], NULL, 10);
		BK_LOG_ON_ERR(bk_pwm_group_set_config(group, &config));
		CLI_LOGD("pwm config, group=%x period=%x chan1_duty=%x chan2_duty=%x\n",
				 group, config.period_cycle, config.chan1_duty_cycle, config.chan2_duty_cycle);
	} else {
		cli_pwm_help();
		return;
	}
}


static void cli_pwm_isr(pwm_chan_t chan)
{
	CLI_LOGD("isr(%d)\n", chan);
}

static void cli_pwm_capture_isr(pwm_chan_t chan)
{
	CLI_LOGD("capture(%d), value=%x\n", chan, bk_pwm_capture_get_value(chan));
}

static void cli_pwm_int_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t chan;

	if (argc != 3) {
		cli_pwm_help();
		return;
	}

	chan = os_strtoul(argv[1], NULL, 10);

	if (os_strcmp(argv[2], "reg") == 0) {
		BK_LOG_ON_ERR(bk_pwm_register_isr(chan, cli_pwm_isr));
		CLI_LOGD("pwm chan%d register interrupt isr\n", chan);
	} else if (os_strcmp(argv[2], "enable") == 0) {
		BK_LOG_ON_ERR(bk_pwm_enable_interrupt(chan));
		CLI_LOGD("pwm chan%d enable interrupt\n", chan);
	} else if (os_strcmp(argv[2], "disable") == 0) {
		BK_LOG_ON_ERR(bk_pwm_disable_interrupt(chan));
		CLI_LOGD("pwm chan%d disable interrupt\n", chan);
	} else {
		cli_pwm_help();
		return;
	}
}

static void cli_pwm_capture_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t chan;

	if (argc < 2) {
		cli_pwm_help();
		return;
	}

	chan = os_strtoul(argv[1], NULL, 10);

	if (os_strcmp(argv[2], "init") == 0) {
		pwm_capture_init_config_t config = {0};
		if (argc > 4) {
			cli_pwm_help();
			return;
		}

		if (argc == 3 || os_strcmp(argv[3], "pos") == 0)
			config.edge = PWM_CAPTURE_POS;
		else if (os_strcmp(argv[3], "neg") == 0)
			config.edge = PWM_CAPTURE_NEG;
		else if (os_strcmp(argv[3], "edge") == 0)
			config.edge = PWM_CAPTURE_EDGE;
		else {
			cli_pwm_help();
			return;
		}

		config.isr = cli_pwm_capture_isr;
		BK_LOG_ON_ERR(bk_pwm_capture_init(chan, &config));
		CLI_LOGD("pwm_capture init, chan=%d\n", chan);
	} else if (os_strcmp(argv[2], "start") == 0) {
		BK_LOG_ON_ERR(bk_pwm_capture_start(chan));
		CLI_LOGD("pwm_capture start, chan=%d\n", chan);
	} else if (os_strcmp(argv[2], "stop") == 0) {
		BK_LOG_ON_ERR(bk_pwm_capture_stop(chan));
		CLI_LOGD("pwm_capture stop, chan=%d\n", chan);
	} else if (os_strcmp(argv[2], "deinit") == 0) {
		BK_LOG_ON_ERR(bk_pwm_capture_deinit(chan));
		CLI_LOGD("pwm_capture deinit, chan=%d\n", chan);
	} else if (os_strcmp(argv[2], "capture_example") == 0) {
		pwm_capture_init_config_t config = {0};
		float duty_ratio = 0;

		config.edge = PWM_CAPTURE_POS;
		config.isr = NULL;
		BK_LOG_ON_ERR(bk_pwm_capture_deinit(chan));
		BK_LOG_ON_ERR(bk_pwm_capture_init(chan, &config));
		BK_LOG_ON_ERR(bk_pwm_capture_start(chan));

		uint32_t period_cycle = bk_pwm_capture_get_period_duty_cycle(chan, 1000);
		CLI_LOGD("pwm_capture period_cycle:%d, freq:%dHz\n", period_cycle, _PERIOD_2_FREQ(period_cycle));
		BK_LOG_ON_ERR(bk_pwm_capture_stop(chan));
		BK_LOG_ON_ERR(bk_pwm_capture_deinit(chan));

		config.edge = PWM_CAPTURE_EDGE;
		config.isr = NULL;
		BK_LOG_ON_ERR(bk_pwm_capture_init(chan, &config));
		BK_LOG_ON_ERR(bk_pwm_capture_start(chan));
		uint32_t duty_cycle = bk_pwm_capture_get_period_duty_cycle(chan, 1000);
		if (period_cycle == 0) {
			duty_ratio = 0;
		} else {
			duty_ratio = (float)duty_cycle / (float)period_cycle;
		}
		CLI_LOGD("pwm_capture duty_cycle:%d, duty_ratio:%f\r\n", duty_cycle, duty_ratio);
		BK_LOG_ON_ERR(bk_pwm_capture_stop(chan));
		BK_LOG_ON_ERR(bk_pwm_capture_deinit(chan));
	} else {
		cli_pwm_help();
		return;
	}
}

#define PWM_FREQ           (16000)
static uint32_t s_period_cycle = PWM_CLOCK_SRC_XTAL / PWM_FREQ;

static void pwm_init_and_start_all_chan_test(uint32_t period_cycle, uint32_t duty_cycle)
{
	BK_LOG_ON_ERR(bk_pwm_driver_init());

	pwm_init_config_t init_config = {0};

	for(pwm_chan_t chan = 0; chan < SOC_PWM_CHAN_NUM_PER_UNIT * SOC_PWM_UNIT_NUM; chan++) {
		init_config.period_cycle = period_cycle;
		init_config.duty_cycle = duty_cycle;
		BK_LOG_ON_ERR(bk_pwm_init(chan, &init_config));
		BK_LOG_ON_ERR(bk_pwm_start(chan));
	}
	s_period_cycle = period_cycle;
	rtos_delay_milliseconds(1000);

	CLI_LOGD("pwm_init_and_start period=%d duty=%d\r\n", init_config.period_cycle, init_config.duty_cycle);
}

static void pwm_update_all_chan_test(void)
{
	pwm_period_duty_config_t pwm_config = {0};
	uint32_t pwm_step = s_period_cycle / 10;
	uint32_t i = 0;
	uint32_t j = 0;
	pwm_chan_t chan = 0;

	for (j = 0; j < 5; j++) {
		CLI_LOGD("turn on\r\n");
		for (i = 0; i < 11; i++) {
			for(chan = 0; chan < SOC_PWM_CHAN_NUM_PER_UNIT * SOC_PWM_UNIT_NUM; chan++) {
				pwm_config.period_cycle = s_period_cycle;
				pwm_config.duty_cycle = pwm_step * i;
				pwm_config.duty2_cycle = 0;
				pwm_config.duty3_cycle = 0;
				pwm_config.psc = 0;
				bk_pwm_set_period_duty(chan, &pwm_config);
			}
			rtos_delay_milliseconds(100);
			chan = 0;
		}

		CLI_LOGD("turn off\r\n");
		for (i = 0; i < 11; i++) {
			for(chan = 0; chan < SOC_PWM_CHAN_NUM_PER_UNIT * SOC_PWM_UNIT_NUM; chan++) {
				pwm_config.period_cycle = s_period_cycle;
				pwm_config.duty_cycle = pwm_step * (10 - i);
				pwm_config.duty2_cycle = 0;
				pwm_config.duty3_cycle = 0;
				pwm_config.psc = 0;
				bk_pwm_set_period_duty(chan, &pwm_config);
			}
			rtos_delay_milliseconds(100);
		}
	}
}

#if CONFIG_PWM_PHASE_SHIFT
#define PHASE_SHIFT_CONFIG { \
	.psc = 0, \
	.chan_num = 6,\
	.period_cycle = s_period_cycle, \
	.duty_config[0] = { \
		.chan = 0, \
		.duty_cycle = 0, \
	}, \
	.duty_config[1] = { \
		.chan = 1, \
 		.duty_cycle = 0, \
	}, \
	.duty_config[2] = { \
		.chan = 2, \
		.duty_cycle = 0, \
	}, \
	.duty_config[3] = { \
		.chan = 3, \
		.duty_cycle = 0, \
	}, \
	.duty_config[4] = { \
		.chan = 4, \
		.duty_cycle = 0, \
	}, \
	.duty_config[5] = { \
		.chan = 5, \
		.duty_cycle = 0, \
	}, \
} \

static int pwm_phase_shift_test(void)
{
	pwm_phase_shift_config_t config = PHASE_SHIFT_CONFIG;

	BK_LOG_ON_ERR(bk_pwm_driver_init());
	BK_LOG_ON_ERR(bk_pwm_phase_shift_init(&config));
	BK_LOG_ON_ERR(bk_pwm_phase_shift_start());

	uint32_t rand_num = 0;
	time_t time_num;

	srand((unsigned)time(&time_num));

	for (int j = 0; j < 10; j++) {
		CLI_LOGD("phase_shift test_num:%d\r\n", j);
		for (int i = 0; i < config.chan_num; i++) {
			rand_num = rand() % 11;
			CLI_LOGD("phase_shift rand_num:%d\r\n", rand_num);
			BK_LOG_ON_ERR(bk_pwm_phase_shift_set_duty(config.duty_config[i].chan, s_period_cycle * 0.1f * rand_num));
		}
		BK_LOG_ON_ERR(bk_pwm_phase_shift_update_duty());
		rtos_delay_milliseconds(10);
		BK_LOGD(NULL, "\r\n");
	}

	BK_LOG_ON_ERR(bk_pwm_phase_shift_set_duty(0, 0));
	BK_LOG_ON_ERR(bk_pwm_phase_shift_set_duty(1, 0));
	BK_LOG_ON_ERR(bk_pwm_phase_shift_set_duty(2, 0));
	BK_LOG_ON_ERR(bk_pwm_phase_shift_set_duty(3, 0));
	BK_LOG_ON_ERR(bk_pwm_phase_shift_set_duty(4, 0));
	BK_LOG_ON_ERR(bk_pwm_phase_shift_set_duty(5, 0));
	BK_LOG_ON_ERR(bk_pwm_phase_shift_update_duty());

	CLI_LOGD("phase_shift test over\r\n");

	return 0;
}
#endif

#if CONFIG_PWM_FADE
static void cli_pwm_fade_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 6) {
		cli_pwm_help();
		return;
	}

	pwm_fade_mode_t fade_mode = 0;

	if (os_strcmp(argv[1], "inc") == 0) {
		fade_mode = PWM_DUTY_DIR_INCREASE;
	} else {
		fade_mode = PWM_DUTY_DIR_DECREASE;
	}

	uint32_t chan = os_strtoul(argv[2], NULL, 10);
	uint32_t fade_scale = os_strtoul(argv[3], NULL, 10);
	uint32_t fade_intv_cycle = os_strtoul(argv[4], NULL, 10);
	uint32_t fade_num = os_strtoul(argv[5], NULL, 10);

	// bk_pwm_fade_init(0, 100, 1000, 10);
	bk_pwm_fade_init(chan, fade_scale, fade_intv_cycle, fade_num);
	bk_pwm_fade_start(chan, fade_mode);

	CLI_LOGD("pwm fade test done\r\n");
}
#endif

static void cli_pwm_idle_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_pwm_help();
		return;
	}

	pwm_group_t group = 0;

	if (os_strcmp(argv[1], "idle_init") == 0) {
		if (argc < 4) {
			cli_pwm_help();
			return;
		}
		uint32_t freq = os_strtoul(argv[2], NULL, 10);
		uint32_t period_cycle = PWM_CLOCK_SRC_XTAL / freq;
		uint32_t duty_cycle = period_cycle * os_strtoul(argv[3], NULL, 10) / 100;

		// duty_cycle should test: 0, period_cycle, 1, period_cycle - 1
		CLI_LOGD("period_cycle=%d duty_cycle=%d\r\n", period_cycle, duty_cycle);
		pwm_init_and_start_all_chan_test(period_cycle, duty_cycle);
		CLI_LOGD("pwm idle_init done\r\n");
	} else if (os_strcmp(argv[1], "idle_start") == 0) {
		for(int chan = 0; chan < SOC_PWM_CHAN_NUM_PER_UNIT * SOC_PWM_UNIT_NUM; chan++) {
				BK_LOG_ON_ERR(bk_pwm_start(chan));
		}
	} else if (os_strcmp(argv[1], "idle_stop") == 0) {
		for(int chan = 0; chan < SOC_PWM_CHAN_NUM_PER_UNIT * SOC_PWM_UNIT_NUM; chan++) {
			BK_LOG_ON_ERR(bk_pwm_stop(chan));
		}
	} else if (os_strcmp(argv[1], "idle_update") == 0) {
		pwm_update_all_chan_test();
	} else if (os_strcmp(argv[1], "group_init") == 0) {
		if (argc < 5) {
			cli_pwm_help();
			return;
		}
		BK_LOG_ON_ERR(bk_pwm_driver_init());
		pwm_group_init_config_t config = {0};

		config.chan1 = os_strtoul(argv[2], NULL, 10);
		config.chan2 = os_strtoul(argv[3], NULL, 10);
		uint32_t freq = os_strtoul(argv[4], NULL, 10);
		uint32_t period_cycle = PWM_CLOCK_SRC_XTAL / freq;

		config.period_cycle = period_cycle;
		config.chan1_duty_cycle = period_cycle * os_strtoul(argv[5], NULL, 10) / 100;
		config.chan2_duty_cycle = period_cycle * os_strtoul(argv[6], NULL, 10) / 100;
		BK_LOG_ON_ERR(bk_pwm_group_init(&config, &group));

		CLI_LOGD("pwm group=%d chan1=%d chan2=%d period=%d d1=%d d2=%d\n",
				 group, config.chan1, config.chan2, config.period_cycle,
				 config.chan1_duty_cycle, config.chan2_duty_cycle);
	} else if (os_strcmp(argv[1], "group_start") == 0) {
		uint32_t start_group = os_strtoul(argv[2], NULL, 10);
		BK_LOG_ON_ERR(bk_pwm_group_start(start_group));
	} else if (os_strcmp(argv[1], "group_stop") == 0) {
		BK_LOG_ON_ERR(bk_pwm_group_stop(group));
	} else if (os_strcmp(argv[1], "group_update") == 0) {
		uint32_t start_group = os_strtoul(argv[2], NULL, 10);
		uint32_t freq = os_strtoul(argv[3], NULL, 10);
		uint32_t period_cycle = PWM_CLOCK_SRC_XTAL / freq;

		pwm_group_config_t config = {0};

		config.period_cycle = period_cycle;
		config.chan1_duty_cycle = period_cycle;
		config.chan2_duty_cycle = 0;
		BK_LOG_ON_ERR(bk_pwm_group_set_config(start_group, &config));
		rtos_delay_milliseconds(1000);
		config.period_cycle = period_cycle;
		config.chan1_duty_cycle = period_cycle * 0.3f;
		config.chan2_duty_cycle = period_cycle * 0.3f;
		BK_LOG_ON_ERR(bk_pwm_group_set_config(start_group, &config));
		rtos_delay_milliseconds(1000);

		config.period_cycle = period_cycle;
		config.chan1_duty_cycle = 0;
		config.chan2_duty_cycle = period_cycle;
		BK_LOG_ON_ERR(bk_pwm_group_set_config(start_group, &config));
		rtos_delay_milliseconds(1000);
		config.period_cycle = period_cycle;
		config.chan1_duty_cycle = period_cycle * 0.2f;
		config.chan2_duty_cycle = period_cycle * 0.4f;
		BK_LOG_ON_ERR(bk_pwm_group_set_config(start_group, &config));
		rtos_delay_milliseconds(1000);

		config.period_cycle = period_cycle;
		config.chan1_duty_cycle = period_cycle;
		config.chan2_duty_cycle = 0;
		BK_LOG_ON_ERR(bk_pwm_group_set_config(start_group, &config));
		rtos_delay_milliseconds(1000);
		config.period_cycle = period_cycle;
		config.chan1_duty_cycle = 0;
		config.chan2_duty_cycle = period_cycle;
		BK_LOG_ON_ERR(bk_pwm_group_set_config(start_group, &config));
		rtos_delay_milliseconds(1000);
		config.period_cycle = period_cycle;
		config.chan1_duty_cycle = period_cycle * 0.2f;
		config.chan2_duty_cycle = period_cycle * 0.4f;
		BK_LOG_ON_ERR(bk_pwm_group_set_config(start_group, &config));
		rtos_delay_milliseconds(1000);

		config.period_cycle = period_cycle;
		config.chan1_duty_cycle = 0;
		config.chan2_duty_cycle = period_cycle;
		BK_LOG_ON_ERR(bk_pwm_group_set_config(start_group, &config));
		rtos_delay_milliseconds(1000);
		config.period_cycle = period_cycle;
		config.chan1_duty_cycle = period_cycle;
		config.chan2_duty_cycle = 0;
		BK_LOG_ON_ERR(bk_pwm_group_set_config(start_group, &config));
		rtos_delay_milliseconds(1000);
		config.period_cycle = period_cycle;
		config.chan1_duty_cycle = period_cycle * 0.2f;
		config.chan2_duty_cycle = period_cycle * 0.4f;
		BK_LOG_ON_ERR(bk_pwm_group_set_config(start_group, &config));
	}
#if CONFIG_PWM_PHASE_SHIFT
	else if(os_strcmp(argv[1], "phase_shift") == 0) {
		pwm_phase_shift_test();
	}
#endif
	else {
		cli_pwm_help();
		return;
	}
}

/*
 * Servo motor control via PWM
 *
 * Standard servo: 50Hz (20ms period), pulse width 0.5ms~2.5ms -> 0~180 degrees
 * Clock source: PWM_CLOCK_SRC_XTAL (320MHz)
 * With psc=249: effective clock = 320MHz / (249+1) = 1.28MHz
 * period_cycle for 50Hz = 1.28MHz / 50 = 25600
 * 0.5ms duty = 1.28MHz * 0.0005 = 640
 * 2.5ms duty = 1.28MHz * 0.0025 = 3200
 */
#define SERVO_PSC             249
#define SERVO_EFFECTIVE_CLK   (PWM_CLOCK_SRC_XTAL / (SERVO_PSC + 1))
#define SERVO_FREQ            50
#define SERVO_PERIOD_CYCLE    (SERVO_EFFECTIVE_CLK / SERVO_FREQ)
#define SERVO_PULSE_MIN       (SERVO_EFFECTIVE_CLK * 5 / 10000)
#define SERVO_PULSE_MAX       (SERVO_EFFECTIVE_CLK * 25 / 10000)
#define SERVO_ANGLE_MIN       0
#define SERVO_ANGLE_MAX       180

static bool s_servo_inited[SOC_PWM_CHAN_NUM_MAX] = {false};

static uint32_t servo_angle_to_duty(uint32_t angle)
{
	if (angle > SERVO_ANGLE_MAX)
		angle = SERVO_ANGLE_MAX;
	return SERVO_PULSE_MIN + (uint32_t)angle * (SERVO_PULSE_MAX - SERVO_PULSE_MIN) / SERVO_ANGLE_MAX;
}

static void cli_servo_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		CLI_LOGD("servo init {chan}\n");
		CLI_LOGD("servo set {chan} {angle 0~180}\n");
		CLI_LOGD("servo sweep {chan} [step] [delay_ms]\n");
		CLI_LOGD("servo stop {chan}\n");
		CLI_LOGD("servo deinit {chan}\n");
		CLI_LOGD("\nServo params: freq=%dHz, period=%d, psc=%d\n",
				 SERVO_FREQ, SERVO_PERIOD_CYCLE, SERVO_PSC);
		CLI_LOGD("pulse range: %d~%d (0.5ms~2.5ms)\n", SERVO_PULSE_MIN, SERVO_PULSE_MAX);
		return;
	}

	if (os_strcmp(argv[1], "init") == 0) {
		if (argc < 3) {
			CLI_LOGD("Usage: servo init {chan}\n");
			return;
		}
		uint32_t chan = os_strtoul(argv[2], NULL, 10);
		pwm_init_config_t config = {0};

		config.period_cycle = SERVO_PERIOD_CYCLE;
		config.duty_cycle = servo_angle_to_duty(90);
		config.duty2_cycle = 0;
		config.duty3_cycle = 0;
		config.psc = SERVO_PSC;

		BK_LOG_ON_ERR(bk_pwm_driver_init());
		BK_LOG_ON_ERR(bk_pwm_init(chan, &config));
		BK_LOG_ON_ERR(bk_pwm_start(chan));
		s_servo_inited[chan] = true;
		CLI_LOGD("servo init chan=%d, angle=90, duty=%d\n", chan, config.duty_cycle);
	} else if (os_strcmp(argv[1], "set") == 0) {
		if (argc < 4) {
			CLI_LOGD("Usage: servo set {chan} {angle 0~180}\n");
			return;
		}
		uint32_t chan = os_strtoul(argv[2], NULL, 10);
		uint32_t angle = os_strtoul(argv[3], NULL, 10);

		if (!s_servo_inited[chan]) {
			CLI_LOGD("servo chan=%d not inited, run 'servo init %d' first\n", chan, chan);
			return;
		}

		uint32_t duty = servo_angle_to_duty(angle);
		pwm_period_duty_config_t config = {0};
		config.period_cycle = SERVO_PERIOD_CYCLE;
		config.duty_cycle = duty;
		config.psc = SERVO_PSC;

		BK_LOG_ON_ERR(bk_pwm_set_period_duty(chan, &config));
		CLI_LOGD("servo set chan=%d angle=%d duty=%d\n", chan, angle, duty);
	} else if (os_strcmp(argv[1], "sweep") == 0) {
		if (argc < 3) {
			CLI_LOGD("Usage: servo sweep {chan} [step] [delay_ms]\n");
			return;
		}
		uint32_t chan = os_strtoul(argv[2], NULL, 10);
		uint32_t step = (argc > 3) ? os_strtoul(argv[3], NULL, 10) : 10;
		uint32_t delay_ms = (argc > 4) ? os_strtoul(argv[4], NULL, 10) : 500;

		if (!s_servo_inited[chan]) {
			CLI_LOGD("servo chan=%d not inited, run 'servo init %d' first\n", chan, chan);
			return;
		}

		if (step == 0) step = 10;
		if (delay_ms == 0) delay_ms = 500;

		CLI_LOGD("servo sweep chan=%d step=%d delay=%dms\n", chan, step, delay_ms);

		pwm_period_duty_config_t config = {0};
		config.period_cycle = SERVO_PERIOD_CYCLE;
		config.psc = SERVO_PSC;

		for (uint32_t angle = SERVO_ANGLE_MIN; angle <= SERVO_ANGLE_MAX; angle += step) {
			config.duty_cycle = servo_angle_to_duty(angle);
			BK_LOG_ON_ERR(bk_pwm_set_period_duty(chan, &config));
			CLI_LOGD("  -> angle=%d duty=%d\n", angle, config.duty_cycle);
			rtos_delay_milliseconds(delay_ms);
		}
		for (int angle = SERVO_ANGLE_MAX; angle >= (int)SERVO_ANGLE_MIN; angle -= step) {
			config.duty_cycle = servo_angle_to_duty((uint32_t)angle);
			BK_LOG_ON_ERR(bk_pwm_set_period_duty(chan, &config));
			CLI_LOGD("  -> angle=%d duty=%d\n", angle, config.duty_cycle);
			rtos_delay_milliseconds(delay_ms);
		}
		CLI_LOGD("servo sweep done\n");
	} else if (os_strcmp(argv[1], "stop") == 0) {
		if (argc < 3) {
			CLI_LOGD("Usage: servo stop {chan}\n");
			return;
		}
		uint32_t chan = os_strtoul(argv[2], NULL, 10);
		BK_LOG_ON_ERR(bk_pwm_stop(chan));
		CLI_LOGD("servo stop chan=%d\n", chan);
	} else if (os_strcmp(argv[1], "deinit") == 0) {
		if (argc < 3) {
			CLI_LOGD("Usage: servo deinit {chan}\n");
			return;
		}
		uint32_t chan = os_strtoul(argv[2], NULL, 10);
		BK_LOG_ON_ERR(bk_pwm_stop(chan));
		BK_LOG_ON_ERR(bk_pwm_deinit(chan));
		s_servo_inited[chan] = false;
		CLI_LOGD("servo deinit chan=%d\n", chan);
	} else {
		CLI_LOGD("Unknown servo command: %s\n", argv[1]);
	}
}

#define PWM_CMD_CNT (sizeof(s_pwm_commands) / sizeof(struct cli_command))
DRV_CLI_CMD_EXPORT static const struct cli_command s_pwm_commands[] = {
	{"pwm_driver", "{init|deinit}}", cli_pwm_driver_cmd},
	{"pwm", "pwm {chan} {config|start|stop|init|deinit|signal} [...]", cli_pwm_cmd},
	{"pwm_int", "pwm_int {chan} {reg|enable|disable}", cli_pwm_int_cmd},
	//{"pwm_duty", "pwm_duty {chan} {period} {d1} [d2] [d3]", cli_pwm_cmd},
	{"pwm_capture", "pwm_capture {chan} {config|start|stop|init|deinit}", cli_pwm_capture_cmd},
	{"pwm_group", "pwm_group {init|deinit|config|start|stop} [...]", cli_pwm_group_cmd},
	{"pwm_timer", "pwm_timer ", cli_pwm_timer_cmd},
	{"pwm_counter", "pwm_counter", cli_pwm_counter_cmd},
	{"pwm_carrier", "pwm_carrier", cli_pwm_carrier_cmd},
	{"pwm_idle_test", "{idle_init|idle_start|idle_stop|phase_shift}", cli_pwm_idle_test_cmd},
#if CONFIG_PWM_FADE
	{"pwm_fade", "pwm_fade", cli_pwm_fade_cmd},
#endif
	{"servo", "servo {init|set|sweep|stop|deinit} {chan} [angle] [step] [delay_ms]", cli_servo_cmd},
};

int bk_pwm_register_cli_test_feature(void)
{
	BK_LOG_ON_ERR(bk_pwm_driver_init());
	return cli_register_module_test_feature(s_pwm_commands, PWM_CMD_CNT);
}

#endif
