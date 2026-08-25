// Copyright 2025-2026 Beken
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

#include "core_mqtt_at.h"
#include "core_mqtt_test.h"

#include "atsvr_unite.h"
#include "at_server.h"
#include <components/log.h>

#include <os/str.h>

static int at_coremqtt_connect_cmd(int sync, int argc, char **argv)
{
	(void)sync;

	if (argc != 3) {
		atsvr_cmd_rsp_error();
		return -1;
	}

	if (core_mqtt_connect(argv[0], argv[1], argv[2]) != 0) {
		atsvr_cmd_rsp_error();
		return -1;
	}

	atsvr_cmd_rsp_ok();
	return 0;
}

static int at_coremqtt_subscribe_cmd(int sync, int argc, char **argv)
{
	(void)sync;

	if (argc != 1) {
		atsvr_cmd_rsp_error();
		return -1;
	}

	if (core_mqtt_subscribe(argv[0]) != 0) {
		atsvr_cmd_rsp_error();
		return -1;
	}

	atsvr_cmd_rsp_ok();
	return 0;
}

static int at_coremqtt_publish_cmd(int sync, int argc, char **argv)
{
	(void)sync;

	if (argc < 2) {
		atsvr_cmd_rsp_error();
		return -1;
	}

	if (core_mqtt_publish(argv[0], argv[1]) != 0) {
		atsvr_cmd_rsp_error();
		return -1;
	}

	atsvr_cmd_rsp_ok();
	return 0;
}

static int at_coremqtt_destroy_cmd(int sync, int argc, char **argv)
{
	(void)sync;
	(void)argc;
	(void)argv;

	if (core_mqtt_destroy() != 0) {
		atsvr_cmd_rsp_error();
		return -1;
	}

	atsvr_cmd_rsp_ok();
	return 0;
}

static const struct _atsvr_command core_mqtt_at_cmds[] = {
	ATSVR_CMD_HADLER("AT+COREMQTTCONNECT",
			 "AT+COREMQTTCONNECT=<host>,<user>,<password>",
			 NULL, at_coremqtt_connect_cmd, false, 15000, 0, NULL, false),
	ATSVR_CMD_HADLER("AT+COREMQTTSUB",
			 "AT+COREMQTTSUB=<topic>",
			 NULL, at_coremqtt_subscribe_cmd, false, 0, 0, NULL, false),
	ATSVR_CMD_HADLER("AT+COREMQTTPUB",
			 "AT+COREMQTTPUB=<topic>,<msg>",
			 NULL, at_coremqtt_publish_cmd, false, 0, 0, NULL, false),
	ATSVR_CMD_HADLER("AT+COREMQTTDESTROY",
			 "AT+COREMQTTDESTROY",
			 NULL, at_coremqtt_destroy_cmd, false, 0, 0, NULL, false),
};

void core_mqtt_at_cmd_init(void)
{
	int ret;

	ret = atsvr_register_commands(core_mqtt_at_cmds,
				      sizeof(core_mqtt_at_cmds) / sizeof(core_mqtt_at_cmds[0]),
				      "coremqtt", NULL);
	if (ret == 0)
		BK_LOGI("core_mqtt_at", "coreMQTT AT cmds init ok\r\n");
}
