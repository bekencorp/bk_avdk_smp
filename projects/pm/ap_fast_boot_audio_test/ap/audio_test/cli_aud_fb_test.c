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

#include <os/os.h>
#include <os/str.h>
#include <os/mem.h>
#include <components/log.h>
#include "cli.h"
#include <modules/wdrv_common.h>

#include "cli_aud_fb_test.h"
#include "aud_fb_ipc.h"
#if CONFIG_AUD_PM_FAST_COLD
#include <components/bk_voice_service.h>
#include <components/bk_audio_asr_service.h>
#include <components/bk_asr_service.h>
#include <components/bk_player_service.h>
#endif

#define TAG "aud_fb"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define CLI_CMD_RSP_SUCCEED "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR   "CMDRSP:ERROR\r\n"

enum {
	AUD_FB_SVC_VOICE  = (1u << 0),
	AUD_FB_SVC_ASR    = (1u << 1),
	AUD_FB_SVC_PLAYER = (1u << 2),
};

static uint32_t s_aud_fb_running;
static bool s_aud_fb_auto_ap_off = true;
static bool s_aud_fb_player_prompt;
static uint32_t s_aud_fb_player_tone_id;

/* Forward into existing example CLIs (same translation units). */
extern void cli_voice_service_test_cmd(char *pcWriteBuffer, int xWriteBufferLen,
	int argc, char **argv);
extern void cli_asr_service_test_cmd(char *pcWriteBuffer, int xWriteBufferLen,
	int argc, char **argv);
extern void cli_player_service_test_cmd(char *pcWriteBuffer, int xWriteBufferLen,
	int argc, char **argv);

static void aud_fb_print_help(void)
{
	LOGI("aud_fb service voice init [onboard 8000 1 g711a g711a onboard 8000 1]\r\n");
	LOGI("aud_fb service voice stop              (keep cfg, auto resume)\r\n");
	LOGI("aud_fb service voice deinit            (clear cfg, no resume)\r\n");
	LOGI("aud_fb service asr init [startwithmic onboard 16000 | startnomic onboard 16000]\r\n");
	LOGI("aud_fb service asr stop                (keep cfg, auto resume)\r\n");
	LOGI("aud_fb service asr deinit              (clear cfg, no resume)\r\n");
	LOGI("aud_fb service player init [playback array 0 | prompt_tone 0 array 0]\r\n");
	LOGI("aud_fb service player stop             (keep cfg, auto resume)\r\n");
	LOGI("aud_fb service player deinit           (clear cfg, no resume)\r\n");
	LOGI("aud_fb service status\r\n");
	LOGI("aud_fb auto_ap_off on|off   (default on: last stop/deinit -> request CP AP OFF)\r\n");
	LOGI("aud_fb ap off               (force request CP AP OFF, debug only)\r\n");
	LOGI("CP side after AP OFF: ap_fast_boot on\r\n");
}

/* Keepalive-style: AP only requests; CP votes AP OFF. */
static bk_err_t aud_fb_request_ap_off(void)
{
	aud_fb_ipc_ap_off_t payload = {
		.magic = AUD_FB_IPC_AP_OFF_MAGIC,
	};
	bk_err_t ret = bk_wdrv_customer_transfer(AUD_FB_IPC_CMD_AP_OFF,
		(uint8_t *)&payload, sizeof(payload));

	LOGI("request CP AP OFF ret=%d\r\n", ret);
	return ret;
}

static void aud_fb_maybe_power_off_ap(void)
{
	if (!s_aud_fb_auto_ap_off) {
		return;
	}
	if (s_aud_fb_running != 0) {
		return;
	}

	LOGI("no audio service running -> request CP AP OFF\r\n");
	(void)aud_fb_request_ap_off();
}

static void aud_fb_call_voice(char *buf, int len, int argc, char **argv)
{
	cli_voice_service_test_cmd(buf, len, argc, argv);
}

static void aud_fb_call_asr(char *buf, int len, int argc, char **argv)
{
	cli_asr_service_test_cmd(buf, len, argc, argv);
}

static void aud_fb_call_player(char *buf, int len, int argc, char **argv)
{
	cli_player_service_test_cmd(buf, len, argc, argv);
}

static void aud_fb_service_voice(char *buf, int len, int argc, char **argv)
{
	char *local_argv[12];
	int local_argc;
	char tmp[16];

	if (argc < 1) {
		goto usage;
	}

	if (os_strcmp(argv[0], "init") == 0) {
		if (argc == 1) {
			/* Default minimal onboard loopback-friendly preset */
			os_memset(local_argv, 0, sizeof(local_argv));
			local_argv[0] = "voice_service";
			local_argv[1] = "start";
			local_argv[2] = "onboard";
			local_argv[3] = "8000";
			local_argv[4] = "1";
			local_argv[5] = "g711a";
			local_argv[6] = "g711a";
			local_argv[7] = "onboard";
			local_argv[8] = "8000";
			local_argv[9] = "1";
			local_argc = 10;
			aud_fb_call_voice(buf, len, local_argc, local_argv);
		} else {
			os_snprintf(tmp, sizeof(tmp), "voice_service");
			local_argv[0] = tmp;
			local_argv[1] = "start";
			for (int i = 1; i < argc && i + 1 < (int)(sizeof(local_argv) / sizeof(local_argv[0])); i++) {
				local_argv[i + 1] = argv[i];
			}
			local_argc = argc + 1;
			aud_fb_call_voice(buf, len, local_argc, local_argv);
		}
		s_aud_fb_running |= AUD_FB_SVC_VOICE;
		return;
	}

	if ((os_strcmp(argv[0], "stop") == 0) || (os_strcmp(argv[0], "deinit") == 0)) {
		local_argv[0] = "voice_service";
		local_argv[1] = "stop";
		aud_fb_call_voice(buf, len, 2, local_argv);
		s_aud_fb_running &= ~AUD_FB_SVC_VOICE;
#if CONFIG_AUD_PM_FAST_COLD
		if (os_strcmp(argv[0], "deinit") == 0) {
			bk_voice_pm_clear();
		}
#endif
		aud_fb_maybe_power_off_ap();
		return;
	}

usage:
	LOGE("usage: aud_fb service voice init [...]\r\n");
	LOGE("       aud_fb service voice stop\r\n");
	LOGE("       aud_fb service voice deinit\r\n");
	os_memcpy(buf, CLI_CMD_RSP_ERROR, os_strlen(CLI_CMD_RSP_ERROR));
}

static void aud_fb_service_asr(char *buf, int len, int argc, char **argv)
{
	char *local_argv[8];
	int local_argc;
	char tmp[16];

	if (argc < 1) {
		goto usage;
	}

	if (os_strcmp(argv[0], "init") == 0) {
		if (argc == 1) {
			local_argv[0] = "asr_service";
			local_argv[1] = "startwithmic";
			local_argv[2] = "onboard";
			local_argv[3] = "16000";
			local_argc = 4;
			aud_fb_call_asr(buf, len, local_argc, local_argv);
		} else {
			/* Allow: init startwithmic ...  OR  startwithmic ... */
			os_snprintf(tmp, sizeof(tmp), "asr_service");
			local_argv[0] = tmp;
			if ((os_strcmp(argv[1], "startwithmic") == 0) ||
				(os_strcmp(argv[1], "startnomic") == 0)) {
				for (int i = 1; i < argc; i++) {
					local_argv[i] = argv[i];
				}
				local_argc = argc;
			} else {
				local_argv[1] = "startwithmic";
				for (int i = 1; i < argc; i++) {
					local_argv[i + 1] = argv[i];
				}
				local_argc = argc + 1;
			}
			aud_fb_call_asr(buf, len, local_argc, local_argv);
		}
		s_aud_fb_running |= AUD_FB_SVC_ASR;
		return;
	}

	if ((os_strcmp(argv[0], "stop") == 0) || (os_strcmp(argv[0], "deinit") == 0)) {
		local_argv[0] = "asr_service";
		local_argv[1] = "stop";
		aud_fb_call_asr(buf, len, 2, local_argv);
		s_aud_fb_running &= ~AUD_FB_SVC_ASR;
#if CONFIG_AUD_PM_FAST_COLD
		if (os_strcmp(argv[0], "deinit") == 0) {
			bk_asr_pm_clear();
			bk_aud_asr_pm_clear();
		}
#endif
		aud_fb_maybe_power_off_ap();
		return;
	}

usage:
	LOGE("usage: aud_fb service asr init [...]\r\n");
	LOGE("       aud_fb service asr stop\r\n");
	LOGE("       aud_fb service asr deinit\r\n");
	os_memcpy(buf, CLI_CMD_RSP_ERROR, os_strlen(CLI_CMD_RSP_ERROR));
}

static void aud_fb_service_player(char *buf, int len, int argc, char **argv)
{
	char *local_argv[8];
	int local_argc;
	char tmp[16];

	if (argc < 1) {
		goto usage;
	}

	if (os_strcmp(argv[0], "init") == 0) {
		s_aud_fb_player_prompt = false;
		s_aud_fb_player_tone_id = 0;
		if (argc == 1) {
			local_argv[0] = "player_service";
			local_argv[1] = "playback";
			local_argv[2] = "start";
			local_argv[3] = "array";
			local_argv[4] = "0";
			local_argc = 5;
			aud_fb_call_player(buf, len, local_argc, local_argv);
		} else {
			os_snprintf(tmp, sizeof(tmp), "player_service");
			local_argv[0] = tmp;
			if (os_strcmp(argv[1], "playback") == 0 ||
				os_strcmp(argv[1], "prompt_tone") == 0) {
				int dst = 1;
				int src = 1;

				s_aud_fb_player_prompt = (os_strcmp(argv[1], "prompt_tone") == 0);
				local_argv[dst++] = argv[src++];
				if (src >= argc || os_strcmp(argv[src], "start") != 0) {
					local_argv[dst++] = "start";
				} else {
					src++;
					local_argv[dst++] = "start";
				}
				if (s_aud_fb_player_prompt && src < argc) {
					s_aud_fb_player_tone_id = os_strtoul(argv[src], NULL, 10);
				}
				while (src < argc &&
					dst < (int)(sizeof(local_argv) / sizeof(local_argv[0]))) {
					local_argv[dst++] = argv[src++];
				}
				local_argc = dst;
			} else {
				local_argv[1] = "playback";
				local_argv[2] = "start";
				for (int i = 1; i < argc &&
					i + 2 < (int)(sizeof(local_argv) / sizeof(local_argv[0])); i++) {
					local_argv[i + 2] = argv[i];
				}
				local_argc = argc + 2;
			}
			aud_fb_call_player(buf, len, local_argc, local_argv);
		}
		s_aud_fb_running |= AUD_FB_SVC_PLAYER;
		return;
	}

	if ((os_strcmp(argv[0], "stop") == 0) || (os_strcmp(argv[0], "deinit") == 0)) {
		char tone_id_str[12];

		local_argv[0] = "player_service";
		if (s_aud_fb_player_prompt) {
			os_snprintf(tone_id_str, sizeof(tone_id_str), "%u",
				s_aud_fb_player_tone_id);
			local_argv[1] = "prompt_tone";
			local_argv[2] = "stop";
			local_argv[3] = tone_id_str;
			aud_fb_call_player(buf, len, 4, local_argv);
		} else {
			local_argv[1] = "playback";
			local_argv[2] = "stop";
			aud_fb_call_player(buf, len, 3, local_argv);
		}
		s_aud_fb_running &= ~AUD_FB_SVC_PLAYER;
#if CONFIG_AUD_PM_FAST_COLD
		if (os_strcmp(argv[0], "deinit") == 0) {
			bk_player_pm_clear();
		}
#endif
		aud_fb_maybe_power_off_ap();
		return;
	}

usage:
	LOGE("usage: aud_fb service player init [...]\r\n");
	LOGE("       aud_fb service player stop\r\n");
	LOGE("       aud_fb service player deinit\r\n");
	os_memcpy(buf, CLI_CMD_RSP_ERROR, os_strlen(CLI_CMD_RSP_ERROR));
}

static void cli_aud_fb_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = CLI_CMD_RSP_ERROR;

	if (argc < 2) {
		aud_fb_print_help();
		goto out;
	}

	if (os_strcmp(argv[1], "auto_ap_off") == 0) {
		if (argc < 3) {
			goto out;
		}
		s_aud_fb_auto_ap_off = (os_strcmp(argv[2], "on") == 0);
		LOGI("auto_ap_off=%d\r\n", s_aud_fb_auto_ap_off);
		msg = CLI_CMD_RSP_SUCCEED;
		goto out;
	}

	if (os_strcmp(argv[1], "ap") == 0) {
		if (argc < 3) {
			goto out;
		}
		if (os_strcmp(argv[2], "off") == 0) {
			if (aud_fb_request_ap_off() == BK_OK) {
				msg = CLI_CMD_RSP_SUCCEED;
			}
		} else if (os_strcmp(argv[2], "on") == 0) {
			LOGE("AP cannot vote itself ON; use CP: ap_fast_boot on\r\n");
		}
		goto out;
	}

	if (os_strcmp(argv[1], "service") == 0) {
		if (argc < 3) {
			aud_fb_print_help();
			goto out;
		}

		if (os_strcmp(argv[2], "status") == 0) {
			LOGI("running=0x%x voice=%d asr=%d player=%d auto_ap_off=%d\r\n",
				s_aud_fb_running,
				!!(s_aud_fb_running & AUD_FB_SVC_VOICE),
				!!(s_aud_fb_running & AUD_FB_SVC_ASR),
				!!(s_aud_fb_running & AUD_FB_SVC_PLAYER),
				s_aud_fb_auto_ap_off);
			msg = CLI_CMD_RSP_SUCCEED;
			goto out;
		}

		if (argc < 4) {
			aud_fb_print_help();
			goto out;
		}

		/* argv[3]=init|stop|deinit, argv[4...] optional service args */
		if (os_strcmp(argv[2], "voice") == 0) {
			aud_fb_service_voice(pcWriteBuffer, xWriteBufferLen, argc - 3, &argv[3]);
			return;
		}
		if (os_strcmp(argv[2], "asr") == 0) {
			aud_fb_service_asr(pcWriteBuffer, xWriteBufferLen, argc - 3, &argv[3]);
			return;
		}
		if (os_strcmp(argv[2], "player") == 0) {
			aud_fb_service_player(pcWriteBuffer, xWriteBufferLen, argc - 3, &argv[3]);
			return;
		}
	}

	aud_fb_print_help();

out:
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

static const struct cli_command s_aud_fb_commands[] = {
	{
		"aud_fb",
		"aud_fb service|ap|auto_ap_off",
		cli_aud_fb_cmd,
	},
};

int cli_aud_fb_test_init(void)
{
	return cli_register_commands(s_aud_fb_commands,
		sizeof(s_aud_fb_commands) / sizeof(s_aud_fb_commands[0]));
}
