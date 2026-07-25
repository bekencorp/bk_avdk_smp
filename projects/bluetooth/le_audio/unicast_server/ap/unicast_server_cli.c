#include "cli.h"
#include "unicast_server_demo.h"
#include "../../common/audio.h"

#include <os/str.h>
#include <stdio.h>
#include <stdlib.h>

#define CLI_RSP_OK     "CMDRSP:OK\r\n"
#define CLI_RSP_ERROR  "CMDRSP:ERROR\r\n"

static void le_audio_cli_write_rsp(char *pcWriteBuffer, int xWriteBufferLen, const char *msg)
{
	if (pcWriteBuffer && xWriteBufferLen > 0)
	{
		snprintf(pcWriteBuffer, xWriteBufferLen, "%s", msg);
	}
}

static int le_audio_preset_cmd(int argc, char **argv)
{
	if (argc < 3)
	{
		return -1;
	}

	if (os_strcmp(argv[2], "list") == 0)
	{
		uint8_t count = 0;
		const le_audio_preset_t *presets = le_audio_preset_table(&count);

		CLI_LOGI("presets (current=%s):\n", le_audio_codec_current_name());
		for (uint8_t i = 0; i < count; i++)
		{
			CLI_LOGI("  %s sr=%lu frame=%luus bytes=%u\n",
			         presets[i].name, presets[i].cfg.sample_rate,
			         presets[i].cfg.frame_us, presets[i].cfg.frame_bytes);
		}
		return 0;
	}

	return le_audio_codec_set_preset(argv[2]);
}

static void le_audio_usage(void)
{
	CLI_LOGI("le_audio unicast server:\n"
	         "  le_audio preset list|<name>\n"
	         "  le_audio adv on|off\n"
	         "  le_audio rx_ready <ase>\n"
	         "  le_audio release <ase>\n");
}

static void cmd_le_audio(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret = -1;

	if (argc <= 1 || os_strcmp(argv[1], "-h") == 0)
	{
		le_audio_usage();
		le_audio_cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_RSP_OK);
		return;
	}

	if (os_strcmp(argv[1], "preset") == 0)
	{
		ret = le_audio_preset_cmd(argc, argv);
	}
	else if (os_strcmp(argv[1], "adv") == 0 && argc >= 3)
	{
		if (os_strcmp(argv[2], "on") == 0)
		{
			ret = unicast_server_demo_adv(1);
		}
		else if (os_strcmp(argv[2], "off") == 0)
		{
			ret = unicast_server_demo_adv(0);
		}
	}
	else if (os_strcmp(argv[1], "rx_ready") == 0 && argc >= 3)
	{
		ret = unicast_server_demo_rx_ready((uint8_t)strtoul(argv[2], NULL, 0));
	}
	else if (os_strcmp(argv[1], "release") == 0 && argc >= 3)
	{
		ret = unicast_server_demo_release((uint8_t)strtoul(argv[2], NULL, 0));
	}

	le_audio_cli_write_rsp(pcWriteBuffer, xWriteBufferLen, ret == 0 ? CLI_RSP_OK : CLI_RSP_ERROR);
}

static const struct cli_command s_le_audio_commands[] =
{
	{"le_audio", "LE Audio role demo, see le_audio -h", cmd_le_audio},
};

int unicast_server_cli_init(void)
{
	return cli_register_commands(s_le_audio_commands,
	                             sizeof(s_le_audio_commands) / sizeof(s_le_audio_commands[0]));
}
