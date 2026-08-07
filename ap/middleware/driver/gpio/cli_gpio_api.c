// Copyright 2020-2024 Beken
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
#include "argtable3.h"
#include "cli_common.h"
#include <stdbool.h>
#include <driver/gpio.h>
#include "gpio_driver.h"

extern void print_help(const char *progname, void **argtable);
extern void common_cmd_handler(int argc, char **argv, void **argtable, int argtable_size, void (*handler)(void **argtable));

static void gpio_api_test_isr(gpio_id_t id)
{
	CLI_LOGI("gpio api isr index:%d\r\n", id);
}

static void cli_gpio_api_cmd_handler(void **argtable)
{
	struct arg_lit *help      = (struct arg_lit *)argtable[0];
	struct arg_str *driver    = (struct arg_str *)argtable[1];
	struct arg_int *id_arg    = (struct arg_int *)argtable[2];
	struct arg_str *action    = (struct arg_str *)argtable[3];
	struct arg_str *pull      = (struct arg_str *)argtable[4];
	struct arg_str *set       = (struct arg_str *)argtable[5];
	struct arg_int *value     = (struct arg_int *)argtable[6];
	struct arg_int *mode      = (struct arg_int *)argtable[7];
	struct arg_int *output    = (struct arg_int *)argtable[8];
	struct arg_int *capacity  = (struct arg_int *)argtable[9];
	struct arg_int *type      = (struct arg_int *)argtable[10];
	struct arg_str *get       = (struct arg_str *)argtable[11];
	struct arg_str *intr      = (struct arg_str *)argtable[12];
	struct arg_str *wake      = (struct arg_str *)argtable[13];
	struct arg_int *func      = (struct arg_int *)argtable[14];
	struct arg_str *keep      = (struct arg_str *)argtable[15];

	gpio_id_t id = (id_arg->count > 0) ? (gpio_id_t)id_arg->ival[0] : 0;

	if (help->count > 0) {
		print_help(argtable[0], argtable);
		return;
	}

	else if (driver->count > 0) {
		if (strcmp(driver->sval[0], "init") == 0) {
			BK_LOG_ON_ERR(bk_gpio_driver_init());
			CLI_LOGI("GPIO driver initialized successfully\r\n");
		} else if (strcmp(driver->sval[0], "deinit") == 0) {
			BK_LOG_ON_ERR(bk_gpio_driver_deinit());
			CLI_LOGI("GPIO driver deinitialized successfully\r\n");
		} else {
			CLI_LOGE("Invalid parameter for driver: %s\r\n", driver->sval[0]);
		}
	}

	else if (action->count > 0) {
		if (strcmp(action->sval[0], "en_output") == 0) {
			BK_LOG_ON_ERR(bk_gpio_enable_output(id));
			CLI_LOGI("Enable GPIO %d output mode successfully\r\n", id);
		} else if (strcmp(action->sval[0], "dis_output") == 0) {
			BK_LOG_ON_ERR(bk_gpio_disable_output(id));
			CLI_LOGI("Disable GPIO %d output mode successfully\r\n", id);
		} else if (strcmp(action->sval[0], "en_input") == 0) {
			BK_LOG_ON_ERR(bk_gpio_enable_input(id));
			CLI_LOGI("Enable GPIO %d input mode successfully\r\n", id);
		} else if (strcmp(action->sval[0], "dis_input") == 0) {
			BK_LOG_ON_ERR(bk_gpio_disable_input(id));
			CLI_LOGI("Disable GPIO %d input mode successfully\r\n", id);
		} else if (strcmp(action->sval[0], "en_pull") == 0) {
			BK_LOG_ON_ERR(bk_gpio_enable_pull(id));
			CLI_LOGI("Enable GPIO %d pull mode successfully\r\n", id);
		} else if (strcmp(action->sval[0], "dis_pull") == 0) {
			BK_LOG_ON_ERR(bk_gpio_disable_pull(id));
			CLI_LOGI("Disable GPIO %d pull mode successfully\r\n", id);
		} else {
			CLI_LOGE("Invalid parameter for action: %s\r\n", action->sval[0]);
		}
	}

	else if (pull->count > 0) {
		BK_LOG_ON_ERR(bk_gpio_enable_pull(id));
		if (strcmp(pull->sval[0], "up") == 0) {
			BK_LOG_ON_ERR(bk_gpio_pull_up(id));
			CLI_LOGI("Set GPIO %d as pull up mode successfully\r\n", id);
		} else if (strcmp(pull->sval[0], "down") == 0) {
			BK_LOG_ON_ERR(bk_gpio_pull_down(id));
			CLI_LOGI("Set GPIO %d as pull down mode successfully\r\n", id);
		} else {
			CLI_LOGE("Invalid parameter for pull: %s\r\n", pull->sval[0]);
		}
	}

	else if (set->count > 0) {
		if (strcmp(set->sval[0], "value") == 0) {
			uint32_t v = (value->count > 0) ? (uint32_t)value->ival[0] : 0;
			bk_gpio_set_value(id, v);
			CLI_LOGI("Direct set GPIO %d config value %d. successfully\r\n", id, v);
		} else if (strcmp(set->sval[0], "config") == 0) {
			gpio_config_t config = {0};
			config.io_mode   = (mode->count > 0)   ? (gpio_io_mode_t)mode->ival[0]     : GPIO_IO_DISABLE;
			config.pull_mode = (output->count > 0) ? (gpio_pull_mode_t)output->ival[0] : GPIO_PULL_DISABLE;
			config.func_mode = GPIO_SECOND_FUNC_DISABLE;
			BK_LOG_ON_ERR(bk_gpio_set_config(id, &config));
			CLI_LOGI("gpio io(output/disable/input): %x , pull(disable/down/up) : %x\r\n",
				config.io_mode, config.pull_mode);
		} else if (strcmp(set->sval[0], "output_high") == 0) {
			BK_LOG_ON_ERR(bk_gpio_set_output_high(id));
			CLI_LOGI("Set GPIO %d output high successfully\r\n", id);
		} else if (strcmp(set->sval[0], "output_low") == 0) {
			BK_LOG_ON_ERR(bk_gpio_set_output_low(id));
			CLI_LOGI("Set GPIO %d output low successfully\r\n", id);
		} else if (strcmp(set->sval[0], "capacity") == 0) {
			uint32_t cap = (capacity->count > 0) ? (uint32_t)capacity->ival[0] : 0;
			bk_gpio_set_capacity(id, cap);
			CLI_LOGI("Direct set GPIO %d config capacity %d. successfully\r\n", id, cap);
		} else if (strcmp(set->sval[0], "type") == 0) {
			uint32_t t = (type->count > 0) ? (uint32_t)type->ival[0] : 0;
			BK_LOG_ON_ERR(bk_gpio_set_interrupt_type(id, (gpio_int_type_t)t));
			CLI_LOGI("Direct set GPIO %d intterrupt type mode %d successfully\r\n", id, t);
		} else {
			CLI_LOGE("Invalid parameter for set: %s\r\n", set->sval[0]);
		}
	}

	else if (get->count > 0) {
		if (strcmp(get->sval[0], "input") == 0) {
			uint8_t level = bk_gpio_get_input(id);
			CLI_LOGI("Get GPIO %d input level successfully, level:%d\r\n", id, level);
		} else if (strcmp(get->sval[0], "value") == 0) {
			uint32_t v = bk_gpio_get_value(id);
			CLI_LOGI("Get GPIO %d config value successfully, value:0x%x\r\n", id, v);
		} else {
			CLI_LOGE("Invalid parameter for get: %s\r\n", get->sval[0]);
		}
	}

	else if (intr->count > 0) {
		if (strcmp(intr->sval[0], "enable") == 0) {
			BK_LOG_ON_ERR(bk_gpio_enable_interrupt(id));
			CLI_LOGI("Enable GPIO %d intterrupt successfully\r\n", id);
		} else if (strcmp(intr->sval[0], "disable") == 0) {
			BK_LOG_ON_ERR(bk_gpio_disable_interrupt(id));
			CLI_LOGI("Disable GPIO %d intterrupt successfully\r\n", id);
		} else if (strcmp(intr->sval[0], "clear") == 0) {
			BK_LOG_ON_ERR(bk_gpio_register_isr(id, gpio_api_test_isr));
			CLI_LOGI("Register the interrupt service routine for GPIO %d successfully\r\n", id);
		} else {
			CLI_LOGE("Invalid parameter for interrupt: %s\r\n", intr->sval[0]);
		}
	}

#if CONFIG_GPIO_DYNAMIC_WAKEUP_SUPPORT
	else if (wake->count > 0) {
		if (strcmp(wake->sval[0], "register") == 0) {
			gpio_int_type_t int_type = (func->count > 0) ? (gpio_int_type_t)func->ival[0] : GPIO_INT_TYPE_LOW_LEVEL;
			BK_LOG_ON_ERR(bk_gpio_register_wakeup_source(id, int_type));
			CLI_LOGI("GPIO %d register wake up successfully\r\n", id);
		} else if (strcmp(wake->sval[0], "unregister") == 0) {
			BK_LOG_ON_ERR(bk_gpio_unregister_wakeup_source(id));
			CLI_LOGI("GPIO %d driver unregister wakeup successfully\r\n", id);
		} else if (strcmp(wake->sval[0], "get_id") == 0) {
			CLI_LOGI("GET wakeup gpio id: %d\r\n", bk_gpio_get_wakeup_gpio_id());
		} else {
			CLI_LOGE("Invalid parameter for wakeup: %s\r\n", wake->sval[0]);
		}
	}
#endif

	else if (keep->count > 0) {
		if (strcmp(keep->sval[0], "external_ldo") == 0) {
			uint32_t module = (mode->count > 0) ? (uint32_t)mode->ival[0] : 0;
			gpio_output_state_e out = (output->count > 0 && output->ival[0]) ? GPIO_OUTPUT_STATE_HIGH : GPIO_OUTPUT_STATE_LOW;
			BK_LOG_ON_ERR(bk_gpio_ctrl_external_ldo(module, id, out));
			CLI_LOGI("Gpio %d module(%d) external ldo ctrl successfully\r\n", id, module);
		}
#if CONFIG_GPIO_DYNAMIC_KPSTAT_SUPPORT
		else if (strcmp(keep->sval[0], "register") == 0) {
			gpio_config_t config = {0};
			config.io_mode   = (output->count > 0) ? (gpio_io_mode_t)output->ival[0]   : GPIO_IO_DISABLE;
			config.pull_mode = (mode->count > 0)   ? (gpio_pull_mode_t)mode->ival[0]    : GPIO_PULL_DISABLE;
			config.func_mode = (func->count > 0)   ? (gpio_func_mode_t)func->ival[0]    : GPIO_SECOND_FUNC_DISABLE;
			BK_LOG_ON_ERR(bk_gpio_register_lowpower_keep_status(id, &config));
			CLI_LOGI("GPIO %d register keep status successfully\r\n", id);
		} else if (strcmp(keep->sval[0], "unregister") == 0) {
			BK_LOG_ON_ERR(bk_gpio_unregister_lowpower_keep_status(id));
			CLI_LOGI("GPIO %d unregister keep status successfully\r\n", id);
		}
#endif
		else {
			CLI_LOGE("Invalid parameter for keep/ldo: %s\r\n", keep->sval[0]);
		}
	}

	else {
		print_help(argtable[0], argtable);
	}
}

static void cli_arggpio_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	struct arg_lit *help     = arg_lit0("h", "help", "Display this help message");
	struct arg_str *driver   = arg_str0("d", "driver", "<init/deinit>", "Init or deinit GPIO driver");
	struct arg_int *id_arg   = arg_int0("i", "id", "<id>", "GPIO id");
	struct arg_str *action   = arg_str0("a", "action", "<en_output/dis_output/en_input/dis_input/en_pull/dis_pull>", "GPIO mode action");
	struct arg_str *pull     = arg_str0("p", "pull", "<up/down>", "GPIO pull mode");
	struct arg_str *set      = arg_str0("s", "set", "<value/config/output_high/output_low/capacity/type>", "GPIO set config");
	struct arg_int *value    = arg_int0("v", "value", "<value>", "GPIO config value");
	struct arg_int *mode     = arg_int0("m", "mode", "<mode>", "GPIO io_mode / ldo module");
	struct arg_int *output   = arg_int0("o", "output", "<output>", "GPIO pull_mode / io_mode / ldo output");
	struct arg_int *capacity = arg_int0("c", "capacity", "<capacity>", "GPIO driver capacity 0~3");
	struct arg_int *type     = arg_int0("t", "type", "<type>", "GPIO interrupt type 0~3");
	struct arg_str *get      = arg_str0("g", "get", "<input/value>", "GPIO get config");
	struct arg_str *intr     = arg_str0("n", "interrupt", "<enable/disable/clear>", "GPIO interrupt control");
	struct arg_str *wake     = arg_str0("w", "wakeup", "<register/unregister/get_id>", "GPIO wakeup source");
	struct arg_int *func     = arg_int0("f", "func", "<func>", "GPIO int_type / func_mode");
	struct arg_str *keep     = arg_str0("u", "keep", "<register/unregister/external_ldo>", "GPIO keep status / external ldo");
	struct arg_end *end      = arg_end(24);

	void *argtable[] = { help, driver, id_arg, action, pull, set, value, mode, output,
			     capacity, type, get, intr, wake, func, keep, end };
	int argtable_size = sizeof(argtable) / sizeof(argtable[0]);

	common_cmd_handler(argc, argv, argtable, argtable_size, cli_gpio_api_cmd_handler);
}

#define GPIO_CMD_CNT (sizeof(s_gpio_commands) / sizeof(struct cli_command))
DRV_CLI_CMD_EXPORT static const struct cli_command s_gpio_commands[] = {
	{"arggpio", "arggpio { -d driver | -i id [-a action | -p pull | -s set | -g get | -n interrupt | -w wakeup | -u keep/ldo] }", cli_arggpio_cmd},
};

int bk_gpio_api_register_cli_test_feature(void)
{
	return cli_register_module_test_feature(s_gpio_commands, GPIO_CMD_CNT);
}
// eof
