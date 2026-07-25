#include "cli.h"
#include "unicast_client_demo.h"
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

static int le_audio_parse_addr(const char *text, uint8_t addr[6])
{
	unsigned int b[6];
	int i;

	if (!text || !addr)
	{
		return -1;
	}
	if (sscanf(text, "%02x:%02x:%02x:%02x:%02x:%02x",
	           &b[5], &b[4], &b[3], &b[2], &b[1], &b[0]) != 6)
	{
		return -1;
	}
	for (i = 0; i < 6; i++)
	{
		if (b[i] > 0xFF)
		{
			return -1;
		}
		addr[i] = (uint8_t)b[i];
	}
	return 0;
}

static int le_audio_is_index(const char *text)
{
	if (!text || !text[0])
	{
		return 0;
	}
	while (*text)
	{
		if (*text < '0' || *text > '9')
		{
			return 0;
		}
		text++;
	}
	return 1;
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
	CLI_LOGI("le_audio unicast client:\n"
	         "  le_audio preset list|<name>\n"
	         "  le_audio scan on|off | peers\n"
	         "  le_audio connect <peer_index>|<addr> [type]\n"
	         "  le_audio tone stop\n"
	         "  le_audio assistant_scan on|off | assistant_sources\n"
	         "  le_audio assistant_connect <deleg_addr> [type]\n"
	         "  le_audio assistant_discover <deleg_addr> [type]\n"
	         "  le_audio assistant_add <source_index> [bis_mask]\n"
	         "  le_audio assistant_remove <source_id> | assistant_stop\n"
	         "  le_audio broadcast_code <hex32>|clear\n");
}

static int le_audio_client_cmd(int argc, char **argv)
{
	const char *op = argv[1];

	if (os_strcmp(op, "preset") == 0)
	{
		return le_audio_preset_cmd(argc, argv);
	}

	if (os_strcmp(op, "scan") == 0 && argc >= 3)
	{
		return unicast_client_demo_scan_peers(os_strcmp(argv[2], "on") == 0 ? 1 : 0);
	}
	if (os_strcmp(op, "peers") == 0)
	{
		unicast_client_demo_list_peers();
		return 0;
	}
	if (os_strcmp(op, "connect") == 0 && argc >= 3)
	{
		if (le_audio_is_index(argv[2]))
		{
			return unicast_client_demo_connect_peer((uint8_t)strtoul(argv[2], NULL, 0));
		}
		else
		{
			uint8_t addr[6];
			uint8_t type = (argc >= 4) ? (uint8_t)strtoul(argv[3], NULL, 0) : 0;

			if (le_audio_parse_addr(argv[2], addr) == 0)
			{
				return unicast_client_demo_connect_addr(addr, type);
			}
		}
	}
	if (os_strcmp(op, "tone") == 0 && argc >= 3)
	{
		if (os_strcmp(argv[2], "stop") == 0)
		{
			return unicast_client_demo_tone_stop();
		}
	}

	return -1;
}

static int le_audio_assistant_cmd(int argc, char **argv)
{
	const char *op = argv[1];

	if (os_strcmp(op, "assistant_scan") == 0 && argc >= 3)
	{
		return unicast_client_demo_assistant_scan(os_strcmp(argv[2], "on") == 0 ? 1 : 0);
	}
	if (os_strcmp(op, "assistant_sources") == 0)
	{
		unicast_client_demo_assistant_list_sources();
		return 0;
	}
	if ((os_strcmp(op, "assistant_connect") == 0 ||
	     os_strcmp(op, "assistant_discover") == 0) && argc >= 3)
	{
		uint8_t addr[6];
		uint8_t type = (argc >= 4) ? (uint8_t)strtoul(argv[3], NULL, 0) : 0;

		if (le_audio_parse_addr(argv[2], addr) != 0)
		{
			return -1;
		}
		return (os_strcmp(op, "assistant_connect") == 0) ?
		       unicast_client_demo_assistant_connect(addr, type) :
		       unicast_client_demo_assistant_discover(addr, type);
	}
	if (os_strcmp(op, "assistant_add") == 0 && argc >= 3)
	{
		uint8_t source_index = (uint8_t)strtoul(argv[2], NULL, 0);
		uint32_t bis_sync = (argc >= 4) ? (uint32_t)strtoul(argv[3], NULL, 0) : 1U;

		return unicast_client_demo_assistant_add(source_index, bis_sync);
	}
	if (os_strcmp(op, "assistant_remove") == 0 && argc >= 3)
	{
		return unicast_client_demo_assistant_remove((uint8_t)strtoul(argv[2], NULL, 0));
	}
	if (os_strcmp(op, "assistant_stop") == 0)
	{
		unicast_client_demo_assistant_stop();
		return 0;
	}
	if (os_strcmp(op, "broadcast_code") == 0 && argc >= 3)
	{
		if (os_strcmp(argv[2], "clear") == 0)
		{
			return unicast_client_demo_set_broadcast_code(NULL);
		}
		else
		{
			uint8_t code[16];

			if (le_audio_parse_broadcast_code(argv[2], code) == 0)
			{
				return unicast_client_demo_set_broadcast_code(code);
			}
		}
	}

	return -1;
}

static void cmd_le_audio(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret;

	if (argc <= 1 || os_strcmp(argv[1], "-h") == 0)
	{
		le_audio_usage();
		le_audio_cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_RSP_OK);
		return;
	}

	ret = le_audio_client_cmd(argc, argv);
	if (ret != 0)
	{
		ret = le_audio_assistant_cmd(argc, argv);
	}

	le_audio_cli_write_rsp(pcWriteBuffer, xWriteBufferLen, ret == 0 ? CLI_RSP_OK : CLI_RSP_ERROR);
}

static const struct cli_command s_le_audio_commands[] =
{
	{"le_audio", "LE Audio unicast client demo, see le_audio -h", cmd_le_audio},
};

int unicast_client_cli_init(void)
{
	return cli_register_commands(s_le_audio_commands,
	                             sizeof(s_le_audio_commands) / sizeof(s_le_audio_commands[0]));
}
