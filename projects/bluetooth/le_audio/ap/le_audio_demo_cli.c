#include "cli.h"
#include "le_audio_demo/le_audio_protocol_demo.h"
#include "bluetooth_app.h"
#include "bta_auracast.h"
#include "bta_manager.h"

#include <components/bluetooth/bk_dm_bap.h>
#include <components/bluetooth/bk_dm_bap_types.h>
#include <os/str.h>
#include <stdio.h>
#include <stdlib.h>

#define CLI_RSP_OK     "CMDRSP:OK\r\n"
#define CLI_RSP_ERROR  "CMDRSP:ERROR\r\n"

static bk_bap_source_announce_data_t s_cli_last_source;
static uint8_t s_cli_has_source;

static void le_audio_cli_write_rsp(char *pcWriteBuffer, int xWriteBufferLen, const char *msg)
{
	if (pcWriteBuffer && xWriteBufferLen > 0)
	{
		snprintf(pcWriteBuffer, xWriteBufferLen, "%s", msg);
	}
}

static void bta_cli_sink_scan_result_callback(bk_bap_source_announce_data_t *source)
{
	if (!source)
	{
		return;
	}

	os_memcpy(&s_cli_last_source, source, sizeof(s_cli_last_source));
	s_cli_has_source = 1;

	CLI_LOGI("auracast source %02X:%02X:%02X:%02X:%02X:%02X sid=%u rssi=%d\n",
	         source->address[5], source->address[4], source->address[3],
	         source->address[2], source->address[1], source->address[0],
	         source->advertising_sid, source->rssi);
}

static void auracast_usage(void)
{
	CLI_LOGI("auracast commands:\n"
	         "  auracast client scan_start|scan_stop|associate|dissociate|enable|disable\n"
	         "  auracast server announcement|start|dummy\n"
	         "  auracast pairing|broadcast|play|on|off|+|-\n"
	         "  auracast volume <0-100>\n"
	         "  auracast address|name <name>|white_list XX:XX:XX:XX:XX:XX\n");
}

static void cmd_auracast(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2 || os_strcmp(argv[1], "-h") == 0)
	{
		auracast_usage();
		le_audio_cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_RSP_OK);
		return;
	}

	if (os_strcmp(argv[1], "client") == 0 && argc >= 3)
	{
		if (os_strcmp(argv[2], "scan_start") == 0)
		{
			bta_auracast_scan_cfg_t cfg = {
				.result_cb = bta_cli_sink_scan_result_callback,
				.stop_cb = NULL,
			};
			bta_auracast_boradcast_scan_start(&cfg);
		}
		else if (os_strcmp(argv[2], "scan_stop") == 0)
		{
			bta_auracast_boradcast_scan_stop();
		}
		else if (os_strcmp(argv[2], "associate") == 0)
		{
			if (!s_cli_has_source)
			{
				le_audio_cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_RSP_ERROR);
				return;
			}
			bta_auracast_boradcast_associate(&s_cli_last_source);
		}
		else if (os_strcmp(argv[2], "dissociate") == 0)
		{
			bta_auracast_boradcast_dissociate();
		}
		else if (os_strcmp(argv[2], "enable") == 0)
		{
			bta_auracast_boradcast_enable();
		}
		else if (os_strcmp(argv[2], "disable") == 0)
		{
			bta_auracast_boradcast_disable();
		}
	}
	else if (os_strcmp(argv[1], "server") == 0 && argc >= 3)
	{
		if (os_strcmp(argv[2], "announcement") == 0)
		{
			bta_auracast_boradcast_setup_announcement();
		}
		else if (os_strcmp(argv[2], "start") == 0)
		{
			bta_auracast_boradcast_start();
		}
		else if (os_strcmp(argv[2], "dummy") == 0)
		{
			bta_auracast_lc3_dummy();
		}
	}
	else if (os_strcmp(argv[1], "pairing") == 0)
	{
		bluetooth_app_pairing();
	}
	else if (os_strcmp(argv[1], "broadcast") == 0)
	{
		bluetooth_app_broadcast();
	}
	else if (os_strcmp(argv[1], "play") == 0)
	{
		bluetooth_app_play();
	}
	else if (os_strcmp(argv[1], "on") == 0)
	{
		bluetooth_app_on();
	}
	else if (os_strcmp(argv[1], "off") == 0)
	{
		bluetooth_app_off();
	}
	else if (os_strcmp(argv[1], "+") == 0)
	{
		bluetooth_app_volume_up();
	}
	else if (os_strcmp(argv[1], "-") == 0)
	{
		bluetooth_app_volume_down();
	}
	else if (os_strcmp(argv[1], "volume") == 0 && argc >= 3)
	{
		uint32_t volume = strtoul(argv[2], NULL, 10);
		bluetooth_app_abs_volume(volume & 0xFF);
	}
	else if (os_strcmp(argv[1], "address") == 0)
	{
		uint8_t address[6] = {0};
		bluetooth_gat_address(address);
		CLI_LOGI("local address: %02X:%02X:%02X:%02X:%02X:%02X\n",
		         address[0], address[1], address[2], address[3], address[4], address[5]);
	}
	else if (os_strcmp(argv[1], "name") == 0 && argc >= 3)
	{
		bluetooth_set_name(argv[2]);
	}
	else
	{
		auracast_usage();
		le_audio_cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_RSP_ERROR);
		return;
	}

	le_audio_cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_RSP_OK);
}

#if CONFIG_LE_AUDIO_PROTOCOL_DEMO
static void le_audio_usage(void)
{
	CLI_LOGI("le_audio protocol demo:\n"
	         "  le_audio role source|sink\n"
	         "  le_audio broadcast source start|stop\n"
	         "  le_audio broadcast sink scan|sync <id>|stop\n"
	         "  le_audio scan|sync <id>|broadcast start|broadcast stop (compat)\n"
	         "  le_audio adv on|off\n"
	         "  le_audio peer <addr> [type]\n"
	         "  le_audio connect <addr> [type] [legacy|ext]\n"
	         "  le_audio unicast source setup|discover|caps sink|source\n"
	         "  le_audio unicast source config <ase_id> sink|source [cap_index]\n"
	         "  le_audio unicast source cig <ase_id> <cig_id> <cis_id>\n"
	         "  le_audio unicast source qos|enable|cis|release <ase_id>\n"
	         "  le_audio unicast sink rx_ready|release <ase_id>\n"
	         "  le_audio unicast remove_iso <handle> <dir>\n"
	         "  le_audio unicast source send <handle> <len>\n"
	         "  le_audio unicast source tone start <handle>|stop\n");
}

static int le_audio_parse_role(const char *role, uint8_t *out)
{
	if (!role || !out)
	{
		return -1;
	}

	if (os_strcmp(role, "sink") == 0)
	{
		*out = BK_GAP_ROLE_SINK;
		return 0;
	}

	if (os_strcmp(role, "source") == 0)
	{
		*out = BK_GAP_ROLE_SOURCE;
		return 0;
	}

	return -1;
}

static int le_audio_parse_addr(const char *text, uint8_t addr[6])
{
	unsigned int b[6];

	if (!text || !addr)
	{
		return -1;
	}

	if (sscanf(text,
	           "%02x:%02x:%02x:%02x:%02x:%02x",
	           &b[5], &b[4], &b[3], &b[2], &b[1], &b[0]) != 6)
	{
		return -1;
	}

	for (int i = 0; i < 6; i++)
	{
		if (b[i] > 0xFF)
		{
			return -1;
		}
		addr[i] = (uint8_t)b[i];
	}

	return 0;
}

static int le_audio_unicast_cmd(int argc, char **argv)
{
	int ret = -1;
	int base = 2;
	uint8_t is_source_cmd = 0;
	uint8_t is_sink_cmd = 0;

	if (argc < 3)
	{
		return -1;
	}

	if ((os_strcmp(argv[2], "source") == 0) || (os_strcmp(argv[2], "sink") == 0))
	{
		is_source_cmd = (os_strcmp(argv[2], "source") == 0);
		is_sink_cmd = (os_strcmp(argv[2], "sink") == 0);
		base = 3;
		if (argc <= base)
		{
			return -1;
		}
	}

	if (is_sink_cmd &&
	    ((os_strcmp(argv[base], "setup") == 0) ||
	     (os_strcmp(argv[base], "discover") == 0) ||
	     (os_strcmp(argv[base], "caps") == 0) ||
	     (os_strcmp(argv[base], "config") == 0) ||
	     (os_strcmp(argv[base], "cig") == 0) ||
	     (os_strcmp(argv[base], "qos") == 0) ||
	     (os_strcmp(argv[base], "enable") == 0) ||
	     (os_strcmp(argv[base], "cis") == 0) ||
	     (os_strcmp(argv[base], "send") == 0) ||
	     (os_strcmp(argv[base], "tone") == 0)))
	{
		return -1;
	}

	if (is_source_cmd && os_strcmp(argv[base], "rx_ready") == 0)
	{
		return -1;
	}

	if (os_strcmp(argv[base], "setup") == 0)
	{
		ret = bk_dm_bap_unicast_setup();
	}
	else if (os_strcmp(argv[base], "discover") == 0)
	{
		ret = bk_dm_bap_unicast_discover();
	}
	else if (os_strcmp(argv[base], "caps") == 0 && argc > (base + 1))
	{
		uint8_t role;
		if (le_audio_parse_role(argv[base + 1], &role) == 0)
		{
			ret = bk_dm_bap_unicast_get_capabilities(role);
		}
	}
	else if (os_strcmp(argv[base], "config") == 0 && argc > (base + 2))
	{
		uint8_t role;
		uint8_t ase_id = (uint8_t)strtoul(argv[base + 1], NULL, 0);
		uint8_t cap_index = (argc > (base + 3)) ? (uint8_t)strtoul(argv[base + 3], NULL, 0) : 0;

		if (le_audio_parse_role(argv[base + 2], &role) == 0)
		{
			CLI_LOGI("unicast config ase=%u role=%s cap=%u\n", ase_id, argv[base + 2], cap_index);
			ret = bk_dm_bap_unicast_configure(ase_id, role, cap_index);
		}
	}
	else if (os_strcmp(argv[base], "cig") == 0 && argc > (base + 3))
	{
		uint8_t ase_id = (uint8_t)strtoul(argv[base + 1], NULL, 0);
		uint8_t cig_id = (uint8_t)strtoul(argv[base + 2], NULL, 0);
		uint8_t cis_id = (uint8_t)strtoul(argv[base + 3], NULL, 0);
		CLI_LOGI("unicast cig ase=%u cig=%u cis=%u\n", ase_id, cig_id, cis_id);
		ret = bk_dm_bap_unicast_set_cig(ase_id, cig_id, cis_id);
	}
	else if (os_strcmp(argv[base], "qos") == 0 && argc > (base + 1))
	{
		uint8_t ase_id = (uint8_t)strtoul(argv[base + 1], NULL, 0);
		CLI_LOGI("unicast qos ase=%u\n", ase_id);
		ret = bk_dm_bap_unicast_qos(ase_id);
	}
	else if (os_strcmp(argv[base], "enable") == 0 && argc > (base + 1))
	{
		uint8_t ase_id = (uint8_t)strtoul(argv[base + 1], NULL, 0);
		uint16_t contexts = (argc > (base + 2)) ? (uint16_t)strtoul(argv[base + 2], NULL, 0) : BK_GAP_DEFAULT_CONTEXTS;
		CLI_LOGI("unicast enable ase=%u contexts=0x%04x\n", ase_id, contexts);
		ret = bk_dm_bap_unicast_enable(ase_id, contexts);
	}
	else if (os_strcmp(argv[base], "cis") == 0 && argc > (base + 1))
	{
		uint8_t ase_id = (uint8_t)strtoul(argv[base + 1], NULL, 0);
		CLI_LOGI("unicast cis ase=%u\n", ase_id);
		ret = bk_dm_bap_unicast_create_cis(ase_id);
	}
	else if (os_strcmp(argv[base], "rx_ready") == 0 && argc > (base + 1))
	{
		uint8_t ase_id = (uint8_t)strtoul(argv[base + 1], NULL, 0);
		CLI_LOGI("unicast rx_ready ase=%u\n", ase_id);
		ret = le_audio_unicast_sink_receiver_start_ready(ase_id);
	}
	else if (os_strcmp(argv[base], "release") == 0 && argc > (base + 1))
	{
		uint8_t ase_id = (uint8_t)strtoul(argv[base + 1], NULL, 0);
		CLI_LOGI("unicast release ase=%u\n", ase_id);
		le_audio_unicast_tone_stop();
		if (is_sink_cmd)
		{
			ret = le_audio_unicast_sink_release(ase_id);
		}
		else
		{
			ret = bk_dm_bap_unicast_release(ase_id);
		}
	}
	else if (os_strcmp(argv[base], "remove_iso") == 0 && argc > (base + 2))
	{
		uint16_t handle = (uint16_t)strtoul(argv[base + 1], NULL, 0);
		uint8_t direction = (uint8_t)strtoul(argv[base + 2], NULL, 0);
		le_audio_unicast_tone_stop();
		ret = bk_dm_bap_unicast_remove_iso(handle, direction);
	}
	else if (os_strcmp(argv[base], "send") == 0 && argc > (base + 2))
	{
		static uint8_t s_seq;
		uint8_t data[120] = {0};
		uint16_t handle = (uint16_t)strtoul(argv[base + 1], NULL, 0);
		uint16_t len = (uint16_t)strtoul(argv[base + 2], NULL, 0);

		if (len > sizeof(data))
		{
			len = sizeof(data);
		}
		data[0] = s_seq++;
		CLI_LOGI("unicast send handle=0x%04x len=%u seq=%u\n", handle, len, s_seq);
		ret = bk_dm_bap_unicast_send(handle, 0, 0, s_seq, data, len);
	}
	else if (os_strcmp(argv[base], "tone") == 0 && argc > (base + 1))
	{
		if (os_strcmp(argv[base + 1], "start") == 0 && argc > (base + 2))
		{
			uint16_t handle = (uint16_t)strtoul(argv[base + 2], NULL, 0);
			CLI_LOGI("unicast tone start handle=0x%04x\n", handle);
			ret = le_audio_unicast_tone_start(handle);
		}
		else if (os_strcmp(argv[base + 1], "stop") == 0)
		{
			ret = le_audio_unicast_tone_stop();
		}
	}

	return ret;
}

static void cmd_le_audio_demo(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret = -1;

	if (argc <= 1 || os_strcmp(argv[1], "-h") == 0)
	{
		le_audio_usage();
		le_audio_cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_RSP_OK);
		return;
	}

	if (os_strcmp(argv[1], "role") == 0 && argc >= 3)
	{
		if (os_strcmp(argv[2], "source") == 0)
		{
			ret = le_audio_demo_set_role(LE_AUDIO_DEMO_ROLE_SOURCE);
		}
		else if (os_strcmp(argv[2], "sink") == 0)
		{
			ret = le_audio_demo_set_role(LE_AUDIO_DEMO_ROLE_SINK);
		}
	}
	else if (os_strcmp(argv[1], "broadcast") == 0 && argc >= 3)
	{
		if (((argc >= 4) && (os_strcmp(argv[2], "source") == 0) && (os_strcmp(argv[3], "start") == 0)) ||
		    (os_strcmp(argv[2], "start") == 0))
		{
			ret = le_audio_demo_broadcast_start();
		}
		else if (((argc >= 4) && (os_strcmp(argv[2], "source") == 0) && (os_strcmp(argv[3], "stop") == 0)) ||
		         (os_strcmp(argv[2], "stop") == 0))
		{
			ret = le_audio_demo_broadcast_stop();
		}
		else if ((argc >= 4) && (os_strcmp(argv[2], "sink") == 0) && (os_strcmp(argv[3], "scan") == 0))
		{
			ret = le_audio_broadcast_sink_scan();
		}
		else if ((argc >= 5) && (os_strcmp(argv[2], "sink") == 0) && (os_strcmp(argv[3], "sync") == 0))
		{
			ret = le_audio_broadcast_sink_sync((uint32_t)strtoul(argv[4], NULL, 0));
		}
		else if ((argc >= 4) && (os_strcmp(argv[2], "sink") == 0) && (os_strcmp(argv[3], "stop") == 0))
		{
			ret = le_audio_demo_stop();
		}
	}
	else if (os_strcmp(argv[1], "scan") == 0)
	{
		ret = le_audio_broadcast_sink_scan();
	}
	else if (os_strcmp(argv[1], "sync") == 0 && argc >= 3)
	{
		ret = le_audio_broadcast_sink_sync((uint32_t)strtoul(argv[2], NULL, 0));
	}
	else if (os_strcmp(argv[1], "stop") == 0)
	{
		ret = le_audio_demo_stop();
	}
	else if (os_strcmp(argv[1], "adv") == 0 && argc >= 3)
	{
		if (os_strcmp(argv[2], "on") == 0)
		{
			ret = bk_dm_bap_unicast_adv(1);
		}
		else if (os_strcmp(argv[2], "off") == 0)
		{
			ret = bk_dm_bap_unicast_adv(0);
		}
	}
	else if ((os_strcmp(argv[1], "peer") == 0 || os_strcmp(argv[1], "connect") == 0) && argc >= 3)
	{
		uint8_t addr[6];
		uint8_t addr_type = (argc >= 4) ? (uint8_t)strtoul(argv[3], NULL, 0) : 0;

		if (le_audio_parse_addr(argv[2], addr) == 0)
		{
			if (os_strcmp(argv[1], "peer") == 0)
			{
				ret = bk_dm_bap_unicast_set_peer(addr, addr_type);
			}
			else
			{
				uint8_t extended = (argc >= 5 && os_strcmp(argv[4], "ext") == 0) ? 1 : 0;
				ret = bk_dm_bap_unicast_connect(addr, addr_type, extended);
			}
		}
	}
	else if (os_strcmp(argv[1], "unicast") == 0)
	{
		ret = le_audio_unicast_cmd(argc, argv);
	}

	le_audio_cli_write_rsp(pcWriteBuffer, xWriteBufferLen, ret == 0 ? CLI_RSP_OK : CLI_RSP_ERROR);
}
#endif

static const struct cli_command s_le_audio_commands[] =
{
	{"auracast", "BTA auracast test, see auracast -h", cmd_auracast},
#if CONFIG_LE_AUDIO_PROTOCOL_DEMO
	{"le_audio", "le_audio protocol demo", cmd_le_audio_demo},
#endif
};

int cli_le_audio_demo_init(void)
{
	return cli_register_commands(s_le_audio_commands,
	                             sizeof(s_le_audio_commands) / sizeof(s_le_audio_commands[0]));
}
