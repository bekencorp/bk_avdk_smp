#include "cli.h"
#include "broadcast_sink_demo.h"
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

static int le_audio_parse_broadcast_code(const char *text, uint8_t code[16])
{
	int i;

	if (!text || !code)
	{
		return -1;
	}
	if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X'))
	{
		text += 2;
	}
	if (os_strlen(text) != 32)
	{
		return -1;
	}
	for (i = 0; i < 16; i++)
	{
		unsigned int byte;

		if (sscanf(text + (i * 2), "%02x", &byte) != 1)
		{
			return -1;
		}
		code[i] = (uint8_t)byte;
	}
	return 0;
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
	CLI_LOGI("le_audio broadcast sink:\n"
	         "  ap_cmd le_audio preset list|<name>\n"
	         "  ap_cmd le_audio scan on|off       # report broadcast source index\n"
	         "  ap_cmd le_audio list              # print scanned sources\n"
	         "  ap_cmd le_audio sync <src> 1      # BIS index is 1-based\n"
	         "  ap_cmd le_audio delegator on|off  # expose BASS for Broadcast Assistant\n"
	         "  ap_cmd le_audio stop\n"
	         "  ap_cmd le_audio broadcast_code <hex32>|clear\n");
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
	else if (os_strcmp(argv[1], "scan") == 0 && argc >= 3)
	{
		ret = broadcast_sink_demo_scan(os_strcmp(argv[2], "on") == 0 ? 1 : 0);
	}
	else if (os_strcmp(argv[1], "list") == 0)
	{
		broadcast_sink_demo_list_sources();
		ret = 0;
	}
	else if (os_strcmp(argv[1], "sync") == 0 && argc >= 4)
	{
		ret = broadcast_sink_demo_sync((uint8_t)strtoul(argv[2], NULL, 0),
		                (uint8_t)strtoul(argv[3], NULL, 0));
	}
	else if (os_strcmp(argv[1], "delegator") == 0 && argc >= 3)
	{
		ret = broadcast_sink_demo_delegator_adv(os_strcmp(argv[2], "on") == 0 ? 1 : 0);
	}
	else if (os_strcmp(argv[1], "stop") == 0)
	{
		broadcast_sink_demo_stop();
		ret = 0;
	}
	else if (os_strcmp(argv[1], "broadcast_code") == 0 && argc >= 3)
	{
		if (os_strcmp(argv[2], "clear") == 0)
		{
			ret = broadcast_sink_demo_set_broadcast_code(NULL);
		}
		else
		{
			uint8_t code[16];

			if (le_audio_parse_broadcast_code(argv[2], code) == 0)
			{
				ret = broadcast_sink_demo_set_broadcast_code(code);
			}
		}
	}

	le_audio_cli_write_rsp(pcWriteBuffer, xWriteBufferLen, ret == 0 ? CLI_RSP_OK : CLI_RSP_ERROR);
}

static const struct cli_command s_le_audio_commands[] =
{
	{"le_audio", "LE Audio broadcast sink demo, see le_audio -h", cmd_le_audio},
};

int broadcast_sink_cli_init(void)
{
	return cli_register_commands(s_le_audio_commands,
	                             sizeof(s_le_audio_commands) / sizeof(s_le_audio_commands[0]));
}
