#include "cli.h"
#include <os/os.h>
#include <driver/pwm.h>

#if CONFIG_PWM && CONFIG_PWM_AUTO

#define PWM_AUTO_OUTPUT_CH0       (2)
#define PWM_AUTO_OUTPUT_CH1       (3)
#define PWM_AUTO_CAPTURE_CH0      (4)
#define PWM_AUTO_CAPTURE_CH1      (5)

#define PWM_AUTO_PERIOD_CYCLE     (26000U)
#define PWM_AUTO_DUTY_25_PERCENT  (PWM_AUTO_PERIOD_CYCLE / 4U)
#define PWM_AUTO_DUTY_50_PERCENT  (PWM_AUTO_PERIOD_CYCLE / 2U)
#define PWM_AUTO_DUTY_75_PERCENT  ((PWM_AUTO_PERIOD_CYCLE * 3U) / 4U)
#define PWM_AUTO_GROUP_PERIOD     (8000U)
#define PWM_AUTO_GROUP_DUTY_25    (PWM_AUTO_GROUP_PERIOD / 4U)
#define PWM_AUTO_GROUP_DUTY_50    (PWM_AUTO_GROUP_PERIOD / 2U)
#define PWM_AUTO_CAPTURE_TIMEOUT  (1000U)
#define PWM_AUTO_SETTLE_TIME_MS   (20U)

static uint32_t pwm_auto_abs_diff(uint32_t value1, uint32_t value2)
{
	return (value1 > value2) ? (value1 - value2) : (value2 - value1);
}

static bool pwm_auto_value_is_close(uint32_t actual, uint32_t expected, uint32_t tolerance)
{
	return pwm_auto_abs_diff(actual, expected) <= tolerance;
}

static bk_err_t pwm_auto_capture_cycles(pwm_chan_t chan, pwm_capture_edge_t edge, uint32_t *cycles)
{
	pwm_capture_init_config_t config = {
		.edge = edge,
		.isr = NULL,
	};
	bk_err_t ret;
	bool capture_inited = false;
	bool capture_started = false;

	ret = bk_pwm_capture_init(chan, &config);
	if (ret != BK_OK) {
		CLI_LOGE("PWM_AUTO capture_init FAIL chan=%d edge=%d ret=-0x%x\r\n",
				 chan, edge, -ret);
		goto cleanup;
	}
	capture_inited = true;

	ret = bk_pwm_capture_start(chan);
	if (ret != BK_OK) {
		CLI_LOGE("PWM_AUTO capture_start FAIL chan=%d edge=%d ret=-0x%x\r\n",
				 chan, edge, -ret);
		goto cleanup;
	}
	capture_started = true;

	rtos_delay_milliseconds(PWM_AUTO_SETTLE_TIME_MS);
	*cycles = bk_pwm_capture_get_period_duty_cycle(chan, PWM_AUTO_CAPTURE_TIMEOUT);
	if (*cycles == 0) {
		CLI_LOGE("PWM_AUTO capture_value FAIL chan=%d edge=%d value=0\r\n", chan, edge);
		ret = BK_FAIL;
	}

cleanup:
	if (capture_started) {
		bk_err_t cleanup_ret = bk_pwm_capture_stop(chan);
		if ((ret == BK_OK) && (cleanup_ret != BK_OK))
			ret = cleanup_ret;
	}
	if (capture_inited) {
		bk_err_t cleanup_ret = bk_pwm_capture_deinit(chan);
		if ((ret == BK_OK) && (cleanup_ret != BK_OK))
			ret = cleanup_ret;
	}

	return ret;
}

static bk_err_t pwm_auto_measure_wave(pwm_chan_t capture_chan,
									  uint32_t expected_period,
									  uint32_t expected_duty)
{
	uint32_t actual_period = 0;
	uint32_t actual_duty = 0;
	uint32_t period_tolerance = expected_period / 50U;
	uint32_t duty_tolerance = expected_period / 20U;
	bk_err_t ret;

	ret = pwm_auto_capture_cycles(capture_chan, PWM_CAPTURE_POS, &actual_period);
	if (ret != BK_OK)
		return ret;

	ret = pwm_auto_capture_cycles(capture_chan, PWM_CAPTURE_EDGE, &actual_duty);
	if (ret != BK_OK)
		return ret;

	CLI_LOGI("PWM_AUTO measure chan=%d period=%u duty=%u expected_period=%u expected_duty=%u\r\n",
			 capture_chan, actual_period, actual_duty, expected_period, expected_duty);

	if (!pwm_auto_value_is_close(actual_period, expected_period, period_tolerance)) {
		CLI_LOGE("PWM_AUTO period FAIL chan=%d actual=%u expected=%u tolerance=%u\r\n",
				 capture_chan, actual_period, expected_period, period_tolerance);
		return BK_FAIL;
	}

	if (!pwm_auto_value_is_close(actual_duty, expected_duty, duty_tolerance)) {
		CLI_LOGE("PWM_AUTO duty FAIL chan=%d actual=%u expected=%u tolerance=%u\r\n",
				 capture_chan, actual_duty, expected_duty, duty_tolerance);
		return BK_FAIL;
	}

	return BK_OK;
}

static uint32_t pwm_auto_duty_from_percent(uint32_t duty_percent)
{
	return (PWM_AUTO_PERIOD_CYCLE * duty_percent) / 100U;
}

static bk_err_t pwm_auto_single_test(uint32_t duty_percent)
{
	uint32_t duty_cycle = pwm_auto_duty_from_percent(duty_percent);
	pwm_init_config_t init_config = {
		.period_cycle = PWM_AUTO_PERIOD_CYCLE,
		.duty_cycle = duty_cycle,
		.duty2_cycle = 0,
		.duty3_cycle = 0,
		.psc = 0,
	};
	bk_err_t ret;
	bool output_inited = false;
	bool output_started = false;

	ret = bk_pwm_init(PWM_AUTO_OUTPUT_CH0, &init_config);
	if (ret != BK_OK) {
		CLI_LOGE("PWM_AUTO single init FAIL ret=-0x%x\r\n", -ret);
		goto cleanup;
	}
	output_inited = true;

	ret = bk_pwm_start(PWM_AUTO_OUTPUT_CH0);
	if (ret != BK_OK) {
		CLI_LOGE("PWM_AUTO single start FAIL ret=-0x%x\r\n", -ret);
		goto cleanup;
	}
	output_started = true;

	rtos_delay_milliseconds(PWM_AUTO_SETTLE_TIME_MS);
	ret = pwm_auto_measure_wave(PWM_AUTO_CAPTURE_CH0,
								PWM_AUTO_PERIOD_CYCLE,
								duty_cycle);

cleanup:
	if (output_started) {
		bk_err_t cleanup_ret = bk_pwm_stop(PWM_AUTO_OUTPUT_CH0);
		if ((ret == BK_OK) && (cleanup_ret != BK_OK))
			ret = cleanup_ret;
	}
	if (output_inited) {
		bk_err_t cleanup_ret = bk_pwm_deinit(PWM_AUTO_OUTPUT_CH0);
		if ((ret == BK_OK) && (cleanup_ret != BK_OK))
			ret = cleanup_ret;
	}

	if (ret == BK_OK)
		CLI_LOGI("PWM_AUTO single %u PASS\r\n", duty_percent);

	return ret;
}

static const char *pwm_auto_capture_edge_name(pwm_capture_edge_t edge)
{
	switch (edge) {
	case PWM_CAPTURE_POS:
		return "pos";
	case PWM_CAPTURE_NEG:
		return "neg";
	case PWM_CAPTURE_EDGE:
		return "edge";
	default:
		return "invalid";
	}
}

static bk_err_t pwm_auto_capture_test(pwm_capture_edge_t edge)
{
	pwm_init_config_t init_config = {
		.period_cycle = PWM_AUTO_PERIOD_CYCLE,
		.duty_cycle = PWM_AUTO_DUTY_50_PERCENT,
		.duty2_cycle = 0,
		.duty3_cycle = 0,
		.psc = 0,
	};
	uint32_t captured_cycles = 0;
	bk_err_t ret;
	bool output_inited = false;
	bool output_started = false;

	ret = bk_pwm_init(PWM_AUTO_OUTPUT_CH0, &init_config);
	if (ret != BK_OK)
		goto cleanup;
	output_inited = true;

	ret = bk_pwm_start(PWM_AUTO_OUTPUT_CH0);
	if (ret != BK_OK)
		goto cleanup;
	output_started = true;

	rtos_delay_milliseconds(PWM_AUTO_SETTLE_TIME_MS);
	if (edge == PWM_CAPTURE_EDGE) {
		ret = pwm_auto_measure_wave(PWM_AUTO_CAPTURE_CH0,
									PWM_AUTO_PERIOD_CYCLE,
									PWM_AUTO_DUTY_50_PERCENT);
	} else {
		ret = pwm_auto_capture_cycles(PWM_AUTO_CAPTURE_CH0, edge, &captured_cycles);
		if ((ret == BK_OK) &&
			!pwm_auto_value_is_close(captured_cycles,
									 PWM_AUTO_PERIOD_CYCLE,
									 PWM_AUTO_PERIOD_CYCLE / 50U)) {
			CLI_LOGE("PWM_AUTO capture %s FAIL actual=%u expected=%u\r\n",
					 pwm_auto_capture_edge_name(edge),
					 captured_cycles,
					 PWM_AUTO_PERIOD_CYCLE);
			ret = BK_FAIL;
		}
	}

cleanup:
	if (output_started) {
		bk_err_t cleanup_ret = bk_pwm_stop(PWM_AUTO_OUTPUT_CH0);
		if ((ret == BK_OK) && (cleanup_ret != BK_OK))
			ret = cleanup_ret;
	}
	if (output_inited) {
		bk_err_t cleanup_ret = bk_pwm_deinit(PWM_AUTO_OUTPUT_CH0);
		if ((ret == BK_OK) && (cleanup_ret != BK_OK))
			ret = cleanup_ret;
	}

	if (ret == BK_OK)
		CLI_LOGI("PWM_AUTO capture %s PASS\r\n", pwm_auto_capture_edge_name(edge));

	return ret;
}

static bk_err_t pwm_auto_group_measure(uint32_t expected_duty0, uint32_t expected_duty1)
{
	bk_err_t ret;

	ret = pwm_auto_measure_wave(PWM_AUTO_CAPTURE_CH0,
								PWM_AUTO_GROUP_PERIOD,
								expected_duty0);
	if (ret != BK_OK)
		return ret;

	return pwm_auto_measure_wave(PWM_AUTO_CAPTURE_CH1,
								 PWM_AUTO_GROUP_PERIOD,
								 expected_duty1);
}

static bk_err_t pwm_auto_group_test(bool update)
{
	pwm_group_init_config_t init_config = {
		.chan1 = PWM_AUTO_OUTPUT_CH0,
		.chan2 = PWM_AUTO_OUTPUT_CH1,
		.period_cycle = PWM_AUTO_GROUP_PERIOD,
		.chan1_duty_cycle = PWM_AUTO_GROUP_DUTY_25,
		.chan2_duty_cycle = PWM_AUTO_GROUP_DUTY_50,
		.psc = 0,
	};
	pwm_group_config_t update_config = {
		.period_cycle = PWM_AUTO_GROUP_PERIOD,
		.chan1_duty_cycle = PWM_AUTO_GROUP_DUTY_50,
		.chan2_duty_cycle = PWM_AUTO_GROUP_DUTY_25,
		.psc = 0,
	};
	pwm_group_t group = 0;
	bk_err_t ret;
	bool group_inited = false;
	bool group_started = false;

	ret = bk_pwm_group_init(&init_config, &group);
	if (ret != BK_OK) {
		CLI_LOGE("PWM_AUTO group init FAIL ret=-0x%x\r\n", -ret);
		goto cleanup;
	}
	group_inited = true;

	ret = bk_pwm_group_start(group);
	if (ret != BK_OK) {
		CLI_LOGE("PWM_AUTO group start FAIL ret=-0x%x\r\n", -ret);
		goto cleanup;
	}
	group_started = true;

	if (update) {
		ret = bk_pwm_group_set_config(group, &update_config);
		if (ret != BK_OK) {
			CLI_LOGE("PWM_AUTO group update FAIL ret=-0x%x\r\n", -ret);
			goto cleanup;
		}
	}

	rtos_delay_milliseconds(PWM_AUTO_SETTLE_TIME_MS);
	if (update) {
		ret = pwm_auto_group_measure(PWM_AUTO_GROUP_DUTY_50,
									 PWM_AUTO_GROUP_DUTY_25);
	} else {
		ret = pwm_auto_group_measure(PWM_AUTO_GROUP_DUTY_25,
									 PWM_AUTO_GROUP_DUTY_50);
	}

cleanup:
	if (group_started) {
		bk_err_t cleanup_ret = bk_pwm_group_stop(group);
		if ((ret == BK_OK) && (cleanup_ret != BK_OK))
			ret = cleanup_ret;
	}
	if (group_inited) {
		bk_err_t cleanup_ret = bk_pwm_group_deinit(group);
		if ((ret == BK_OK) && (cleanup_ret != BK_OK))
			ret = cleanup_ret;
	}

	if (ret == BK_OK)
		CLI_LOGI("PWM_AUTO group %s PASS\r\n", update ? "update" : "base");

	return ret;
}

static bk_err_t pwm_auto_startup_driver_test(void)
{
	bk_err_t ret = bk_pwm_driver_init();
	if (ret != BK_OK) {
		CLI_LOGE("PWM_AUTO startup driver init FAIL ret=-0x%x\r\n", -ret);
		return ret;
	}

	ret = bk_pwm_driver_deinit();
	if (ret == BK_OK)
		CLI_LOGI("PWM_AUTO startup driver PASS\r\n");
	else
		CLI_LOGE("PWM_AUTO startup driver deinit FAIL ret=-0x%x\r\n", -ret);

	return ret;
}

static bk_err_t pwm_auto_startup_channel_test(void)
{
	pwm_init_config_t config = {
		.period_cycle = PWM_AUTO_PERIOD_CYCLE,
		.duty_cycle = PWM_AUTO_DUTY_50_PERCENT,
		.duty2_cycle = 0,
		.duty3_cycle = 0,
		.psc = 0,
	};
	bk_err_t ret;
	bool inited = false;
	bool started = false;

	ret = bk_pwm_init(PWM_AUTO_OUTPUT_CH0, &config);
	if (ret != BK_OK)
		goto cleanup;
	inited = true;

	ret = bk_pwm_start(PWM_AUTO_OUTPUT_CH0);
	if (ret != BK_OK)
		goto cleanup;
	started = true;

	ret = bk_pwm_stop(PWM_AUTO_OUTPUT_CH0);
	if (ret != BK_OK)
		goto cleanup;
	started = false;

	ret = bk_pwm_deinit(PWM_AUTO_OUTPUT_CH0);
	if (ret != BK_OK)
		goto cleanup;
	inited = false;

cleanup:
	if (started)
		bk_pwm_stop(PWM_AUTO_OUTPUT_CH0);
	if (inited)
		bk_pwm_deinit(PWM_AUTO_OUTPUT_CH0);

	if (ret == BK_OK)
		CLI_LOGI("PWM_AUTO startup channel PASS\r\n");
	else
		CLI_LOGE("PWM_AUTO startup channel FAIL ret=-0x%x\r\n", -ret);

	return ret;
}

static bk_err_t pwm_auto_startup_capture_test(void)
{
	pwm_capture_init_config_t config = {
		.edge = PWM_CAPTURE_POS,
		.isr = NULL,
	};
	bk_err_t ret;
	bool inited = false;
	bool started = false;

	ret = bk_pwm_capture_init(PWM_AUTO_CAPTURE_CH0, &config);
	if (ret != BK_OK)
		goto cleanup;
	inited = true;

	ret = bk_pwm_capture_start(PWM_AUTO_CAPTURE_CH0);
	if (ret != BK_OK)
		goto cleanup;
	started = true;

	ret = bk_pwm_capture_stop(PWM_AUTO_CAPTURE_CH0);
	if (ret != BK_OK)
		goto cleanup;
	started = false;

	ret = bk_pwm_capture_deinit(PWM_AUTO_CAPTURE_CH0);
	if (ret != BK_OK)
		goto cleanup;
	inited = false;

cleanup:
	if (started)
		bk_pwm_capture_stop(PWM_AUTO_CAPTURE_CH0);
	if (inited)
		bk_pwm_capture_deinit(PWM_AUTO_CAPTURE_CH0);

	if (ret == BK_OK)
		CLI_LOGI("PWM_AUTO startup capture PASS\r\n");
	else
		CLI_LOGE("PWM_AUTO startup capture FAIL ret=-0x%x\r\n", -ret);

	return ret;
}

static bk_err_t pwm_auto_startup_group_test(void)
{
	pwm_group_init_config_t config = {
		.chan1 = PWM_AUTO_OUTPUT_CH0,
		.chan2 = PWM_AUTO_OUTPUT_CH1,
		.period_cycle = PWM_AUTO_GROUP_PERIOD,
		.chan1_duty_cycle = PWM_AUTO_GROUP_DUTY_25,
		.chan2_duty_cycle = PWM_AUTO_GROUP_DUTY_50,
		.psc = 0,
	};
	pwm_group_t group = 0;
	bk_err_t ret;
	bool inited = false;
	bool started = false;

	ret = bk_pwm_group_init(&config, &group);
	if (ret != BK_OK)
		goto cleanup;
	inited = true;

	ret = bk_pwm_group_start(group);
	if (ret != BK_OK)
		goto cleanup;
	started = true;

	ret = bk_pwm_group_stop(group);
	if (ret != BK_OK)
		goto cleanup;
	started = false;

	ret = bk_pwm_group_deinit(group);
	if (ret != BK_OK)
		goto cleanup;
	inited = false;

cleanup:
	if (started)
		bk_pwm_group_stop(group);
	if (inited)
		bk_pwm_group_deinit(group);

	if (ret == BK_OK)
		CLI_LOGI("PWM_AUTO startup group PASS\r\n");
	else
		CLI_LOGE("PWM_AUTO startup group FAIL ret=-0x%x\r\n", -ret);

	return ret;
}

static bk_err_t pwm_auto_expect_error(const char *name, bk_err_t actual, bk_err_t expected)
{
	if (actual != expected) {
		CLI_LOGE("PWM_AUTO negative %s FAIL actual=-0x%x expected=-0x%x\r\n",
				 name, -actual, -expected);
		return BK_FAIL;
	}

	return BK_OK;
}

static bk_err_t pwm_auto_negative_test(const char *test_case)
{
	pwm_init_config_t invalid_duty = {
		.period_cycle = 100,
		.duty_cycle = 101,
	};
	pwm_group_init_config_t same_channel = {
		.chan1 = PWM_AUTO_OUTPUT_CH0,
		.chan2 = PWM_AUTO_OUTPUT_CH0,
		.period_cycle = 100,
		.chan1_duty_cycle = 40,
		.chan2_duty_cycle = 40,
	};
	pwm_group_init_config_t invalid_group_duty = {
		.chan1 = PWM_AUTO_OUTPUT_CH0,
		.chan2 = PWM_AUTO_OUTPUT_CH1,
		.period_cycle = 100,
		.chan1_duty_cycle = 60,
		.chan2_duty_cycle = 50,
	};
	pwm_group_t group = 0;
	bk_err_t ret;

	if (os_strcmp(test_case, "start") == 0) {
		ret = pwm_auto_expect_error("start",
									bk_pwm_start(PWM_AUTO_OUTPUT_CH0),
									BK_ERR_PWM_CHAN_NOT_INIT);
	} else if (os_strcmp(test_case, "duty") == 0) {
		ret = pwm_auto_expect_error("duty",
									bk_pwm_init(PWM_AUTO_OUTPUT_CH0, &invalid_duty),
									BK_ERR_PWM_PERIOD_DUTY);
	} else if (os_strcmp(test_case, "same_chan") == 0) {
		ret = pwm_auto_expect_error("same_chan",
									bk_pwm_group_init(&same_channel, &group),
									BK_ERR_PWM_GROUP_SAME_CHAN);
	} else if (os_strcmp(test_case, "group_duty") == 0) {
		ret = pwm_auto_expect_error("group_duty",
									bk_pwm_group_init(&invalid_group_duty, &group),
									BK_ERR_PWM_GROUP_DUTY);
	} else {
		return BK_FAIL;
	}

	if (ret == BK_OK)
		CLI_LOGI("PWM_AUTO negative %s PASS\r\n", test_case);

	return ret;
}

static bk_err_t pwm_auto_run_all(void)
{
	const uint32_t duty_percent[] = {25, 50, 75};
	const pwm_capture_edge_t capture_edges[] = {
		PWM_CAPTURE_POS,
		PWM_CAPTURE_NEG,
		PWM_CAPTURE_EDGE,
	};
	const char *negative_cases[] = {
		"start",
		"duty",
		"same_chan",
		"group_duty",
	};
	bk_err_t ret = BK_OK;

	ret = pwm_auto_startup_channel_test();
	if (ret != BK_OK)
		return ret;

	ret = pwm_auto_startup_capture_test();
	if (ret != BK_OK)
		return ret;

	ret = pwm_auto_startup_group_test();
	if (ret != BK_OK)
		return ret;

	for (uint32_t i = 0; i < (sizeof(duty_percent) / sizeof(duty_percent[0])); i++) {
		ret = pwm_auto_single_test(duty_percent[i]);
		if (ret != BK_OK)
			return ret;
	}

	for (uint32_t i = 0; i < (sizeof(capture_edges) / sizeof(capture_edges[0])); i++) {
		ret = pwm_auto_capture_test(capture_edges[i]);
		if (ret != BK_OK)
			return ret;
	}

	ret = pwm_auto_group_test(false);
	if (ret != BK_OK)
		return ret;

	ret = pwm_auto_group_test(true);
	if (ret != BK_OK)
		return ret;

	for (uint32_t i = 0; i < (sizeof(negative_cases) / sizeof(negative_cases[0])); i++) {
		ret = pwm_auto_negative_test(negative_cases[i]);
		if (ret != BK_OK)
			return ret;
	}

	CLI_LOGI("PWM_AUTO all PASS\r\n");

	return ret;
}

static void cli_pwm_auto_help(void)
{
	CLI_LOGI("pwm_auto startup {driver|channel|capture|group}\r\n");
	CLI_LOGI("pwm_auto single {25|50|75}\r\n");
	CLI_LOGI("pwm_auto capture {pos|neg|edge}\r\n");
	CLI_LOGI("pwm_auto group {base|update}\r\n");
	CLI_LOGI("pwm_auto negative {start|duty|same_chan|group_duty}\r\n");
	CLI_LOGI("pwm_auto all\r\n");
	CLI_LOGI("loopback: PWM2(GPIO32)->PWM4(GPIO34), PWM3(GPIO33)->PWM5(GPIO35)\r\n");
}

static bk_err_t pwm_auto_run_selected(int argc, char **argv)
{
	if ((argc == 2) && (os_strcmp(argv[1], "all") == 0))
		return pwm_auto_run_all();

	if (argc != 3)
		return BK_FAIL;

	if (os_strcmp(argv[1], "startup") == 0) {
		if (os_strcmp(argv[2], "channel") == 0)
			return pwm_auto_startup_channel_test();
		if (os_strcmp(argv[2], "capture") == 0)
			return pwm_auto_startup_capture_test();
		if (os_strcmp(argv[2], "group") == 0)
			return pwm_auto_startup_group_test();
	} else if (os_strcmp(argv[1], "single") == 0) {
		uint32_t duty_percent = os_strtoul(argv[2], NULL, 10);
		if ((duty_percent != 25) && (duty_percent != 50) && (duty_percent != 75))
			return BK_FAIL;
		return pwm_auto_single_test(duty_percent);
	} else if (os_strcmp(argv[1], "capture") == 0) {
		if (os_strcmp(argv[2], "pos") == 0)
			return pwm_auto_capture_test(PWM_CAPTURE_POS);
		if (os_strcmp(argv[2], "neg") == 0)
			return pwm_auto_capture_test(PWM_CAPTURE_NEG);
		if (os_strcmp(argv[2], "edge") == 0)
			return pwm_auto_capture_test(PWM_CAPTURE_EDGE);
	} else if (os_strcmp(argv[1], "group") == 0) {
		if (os_strcmp(argv[2], "base") == 0)
			return pwm_auto_group_test(false);
		if (os_strcmp(argv[2], "update") == 0)
			return pwm_auto_group_test(true);
	} else if (os_strcmp(argv[1], "negative") == 0) {
		return pwm_auto_negative_test(argv[2]);
	}

	return BK_FAIL;
}

static void cli_pwm_auto_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret;

	(void)pcWriteBuffer;
	(void)xWriteBufferLen;

	if ((argc < 2) || (argc > 3)) {
		cli_pwm_auto_help();
		return;
	}

	if ((argc == 3) &&
		(os_strcmp(argv[1], "startup") == 0) &&
		(os_strcmp(argv[2], "driver") == 0)) {
		ret = pwm_auto_startup_driver_test();
		if (ret != BK_OK)
			CLI_LOGE("PWM_AUTO command FAIL ret=-0x%x\r\n", -ret);
		return;
	}

	if ((argc == 2) && (os_strcmp(argv[1], "all") == 0)) {
		ret = pwm_auto_startup_driver_test();
		if (ret != BK_OK) {
			CLI_LOGE("PWM_AUTO command FAIL ret=-0x%x\r\n", -ret);
			return;
		}
	}

	ret = bk_pwm_driver_init();
	if (ret == BK_OK)
		ret = pwm_auto_run_selected(argc, argv);

	bk_err_t cleanup_ret = bk_pwm_driver_deinit();
	if ((ret == BK_OK) && (cleanup_ret != BK_OK))
		ret = cleanup_ret;

	if (ret != BK_OK) {
		CLI_LOGE("PWM_AUTO command FAIL ret=-0x%x\r\n", -ret);
		cli_pwm_auto_help();
	}
}

DRV_CLI_CMD_EXPORT static const struct cli_command s_pwm_auto_commands[] = {
	{"pwm_auto", "pwm_auto {startup|single|capture|group|negative|all} [case]", cli_pwm_auto_cmd},
};

#endif
