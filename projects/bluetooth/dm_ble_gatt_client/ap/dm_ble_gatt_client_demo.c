#include <stdio.h>
#include <string.h>

#include <components/log.h>
#include <os/os.h>

#include "cli.h"
#include "components/bluetooth/bk_dm_bluetooth_types.h"
#include "components/bluetooth/bk_dm_gap_ble_types.h"
#include "components/bluetooth/bk_dm_gap_ble.h"
#include "dm_gatt.h"
#include "dm_gattc.h"
#include "dm_ble_gatt_client_demo.h"

#define TAG "dm_ble_gatt_client"

#define DM_GATTC_DEMO_MAX_SERVICE_COUNT 16
#define DM_GATTC_DEMO_MAX_CHAR_COUNT    64
#define DM_GATTC_DEMO_MAX_DESC_COUNT    64
#define DM_GATTC_DEMO_CLI_CMD           "dm_ble_gatt_client"

typedef struct
{
	uint16_t start_handle;
	uint16_t end_handle;
	uint16_t uuid16;
	uint8_t uuid_len;
	uint8_t is_primary;
} dm_ble_gatt_client_demo_service_t;

typedef struct
{
	uint16_t start_handle;
	uint16_t end_handle;
	uint16_t value_handle;
	uint16_t uuid16;
	uint16_t prop;
	uint8_t uuid_len;
} dm_ble_gatt_client_demo_char_t;

typedef struct
{
	uint16_t char_handle;
	uint16_t desc_handle;
	uint16_t uuid16;
	uint8_t uuid_len;
} dm_ble_gatt_client_demo_desc_t;

typedef struct
{
	uint8_t connected;
	uint16_t conn_id;
	uint8_t peer_addr[6];
	uint32_t service_count;
	uint32_t char_count;
	uint32_t desc_count;
	dm_ble_gatt_client_demo_service_t services[DM_GATTC_DEMO_MAX_SERVICE_COUNT];
	dm_ble_gatt_client_demo_char_t chars[DM_GATTC_DEMO_MAX_CHAR_COUNT];
	dm_ble_gatt_client_demo_desc_t descs[DM_GATTC_DEMO_MAX_DESC_COUNT];
} dm_ble_gatt_client_demo_env_t;

static dm_ble_gatt_client_demo_env_t s_demo_env;

static void dm_ble_gatt_client_demo_reset_db(void)
{
	s_demo_env.service_count = 0;
	s_demo_env.char_count = 0;
	s_demo_env.desc_count = 0;
	memset(s_demo_env.services, 0, sizeof(s_demo_env.services));
	memset(s_demo_env.chars, 0, sizeof(s_demo_env.chars));
	memset(s_demo_env.descs, 0, sizeof(s_demo_env.descs));
}

static uint16_t dm_ble_gatt_client_demo_uuid16(const bk_bt_uuid_t *uuid)
{
	uint16_t uuid16 = 0;

	if (!uuid)
	{
		return 0;
	}

	if (uuid->len == BK_UUID_LEN_16)
	{
		uuid16 = uuid->uuid.uuid16;
	}
	else if (uuid->len == BK_UUID_LEN_128)
	{
		uuid16 = (uuid->uuid.uuid128[13] << 8) | uuid->uuid.uuid128[12];
	}

	return uuid16;
}

static void dm_ble_gatt_client_demo_dump_db(void)
{
	BK_LOGI(TAG, "discovery db summary: services=%u chars=%u descs=%u\n",
			s_demo_env.service_count, s_demo_env.char_count, s_demo_env.desc_count);

	for (uint32_t i = 0; i < s_demo_env.service_count; i++)
	{
		BK_LOGI(TAG, "db service[%u] uuid16=0x%04x uuid_len=%u primary=%u handle=%u~%u\n",
				i,
				s_demo_env.services[i].uuid16,
				s_demo_env.services[i].uuid_len,
				s_demo_env.services[i].is_primary,
				s_demo_env.services[i].start_handle,
				s_demo_env.services[i].end_handle);
	}

	for (uint32_t i = 0; i < s_demo_env.char_count; i++)
	{
		BK_LOGI(TAG, "db char[%u] uuid16=0x%04x uuid_len=%u decl=%u~%u value_handle=%u prop=0x%x\n",
				i,
				s_demo_env.chars[i].uuid16,
				s_demo_env.chars[i].uuid_len,
				s_demo_env.chars[i].start_handle,
				s_demo_env.chars[i].end_handle,
				s_demo_env.chars[i].value_handle,
				s_demo_env.chars[i].prop);
	}

	for (uint32_t i = 0; i < s_demo_env.desc_count; i++)
	{
		BK_LOGI(TAG, "db desc[%u] uuid16=0x%04x uuid_len=%u char_handle=%u desc_handle=%u%s\n",
				i,
				s_demo_env.descs[i].uuid16,
				s_demo_env.descs[i].uuid_len,
				s_demo_env.descs[i].char_handle,
				s_demo_env.descs[i].desc_handle,
				s_demo_env.descs[i].uuid16 == BK_GATT_UUID_CHAR_CLIENT_CONFIG ? " cccd" : "");
	}
}

static void dm_ble_gatt_client_demo_dump_value(const char *prefix, const uint8_t *data, uint16_t len)
{
	char hex[64] = {0};
	char text[64] = {0};
	uint16_t dump_len = len > 16 ? 16 : len;
	uint16_t text_len = len > (sizeof(text) - 1) ? (sizeof(text) - 1) : len;
	uint8_t is_text = (text_len > 0);

	for (uint16_t i = 0; i < dump_len; i++)
	{
		snprintf(hex + i * 3, sizeof(hex) - i * 3, "%02x ", data[i]);
	}

	for (uint16_t i = 0; i < text_len; i++)
	{
		if (data[i] < 0x20 || data[i] > 0x7e)
		{
			is_text = 0;
		}
		text[i] = (char)data[i];
	}

	if (is_text)
	{
		BK_LOGI(TAG, "%s len=%u text:%s%s\n", prefix, len, text, len > text_len ? "..." : "");
	}

	BK_LOGI(TAG, "%s len=%u hex:%s%s\n", prefix, len, hex, len > dump_len ? "..." : "");
}

static void dm_ble_gatt_client_demo_parse_name(const uint8_t *data, uint8_t len, char *name, uint8_t name_len)
{
	uint8_t pos = 0;

	if (!data || !name || !name_len)
	{
		return;
	}

	while (pos < len)
	{
		uint8_t field_len = data[pos];
		uint8_t type = 0;

		if (!field_len || pos + field_len >= len)
		{
			break;
		}

		type = data[pos + 1];
		if (type == 0x08 || type == 0x09)
		{
			uint8_t copy_len = field_len - 1;
			if (copy_len >= name_len)
			{
				copy_len = name_len - 1;
			}
			memcpy(name, &data[pos + 2], copy_len);
			name[copy_len] = '\0';
			return;
		}

		pos += field_len + 1;
	}
}

static int32_t dm_ble_gatt_client_demo_gap_cb(bk_ble_gap_cb_event_t event, bk_ble_gap_cb_param_t *param)
{
	switch (event)
	{
	case BK_BLE_GAP_EXT_ADV_REPORT_EVT:
	{
		struct ble_ext_adv_report_param *pm = (typeof(pm))param;
		char name[32] = {0};

		dm_ble_gatt_client_demo_parse_name(pm->params.adv_data,
										  pm->params.adv_data_len,
										  name,
										  sizeof(name));
		BK_LOGI(TAG, "scan report addr=%02x:%02x:%02x:%02x:%02x:%02x type=%u rssi=%d name=%s\n",
				pm->params.addr[5], pm->params.addr[4], pm->params.addr[3],
				pm->params.addr[2], pm->params.addr[1], pm->params.addr[0],
				pm->params.addr_type,
				pm->params.rssi,
				name[0] ? name : "<none>");
		break;
	}

	case BK_BLE_GAP_EXT_SCAN_START_COMPLETE_EVT:
		BK_LOGI(TAG, "scan start complete\n");
		break;

	case BK_BLE_GAP_EXT_SCAN_STOP_COMPLETE_EVT:
		BK_LOGI(TAG, "scan stop complete\n");
		break;

	case BK_BLE_GAP_SEC_REQ_EVT:
		BK_LOGI(TAG, "security request from %02x:%02x:%02x:%02x:%02x:%02x\n",
				param->ble_security.ble_req.bd_addr[5], param->ble_security.ble_req.bd_addr[4],
				param->ble_security.ble_req.bd_addr[3], param->ble_security.ble_req.bd_addr[2],
				param->ble_security.ble_req.bd_addr[1], param->ble_security.ble_req.bd_addr[0]);
		break;

	case BK_BLE_GAP_PASSKEY_NOTIF_EVT:
		BK_LOGI(TAG, "passkey notify %06u, enter it on the peer device\n",
				param->ble_security.key_notif.passkey);
		break;

	case BK_BLE_GAP_PASSKEY_REQ_EVT:
		BK_LOGI(TAG, "passkey request, reply with: ap_cmd ble_gatt_demo gap passkey_reply\n");
		break;

	case BK_BLE_GAP_NC_REQ_EVT:
		BK_LOGI(TAG, "numeric compare %06u, confirm if it matches the peer\n",
				param->ble_security.key_notif.passkey);
		break;

	case BK_BLE_GAP_AUTH_CMPL_EVT:
		BK_LOGI(TAG, "pairing %s peer=%02x:%02x:%02x:%02x:%02x:%02x reason=0x%x\n",
				param->ble_security.auth_cmpl.success ? "success" : "fail",
				param->ble_security.auth_cmpl.bd_addr[5], param->ble_security.auth_cmpl.bd_addr[4],
				param->ble_security.auth_cmpl.bd_addr[3], param->ble_security.auth_cmpl.bd_addr[2],
				param->ble_security.auth_cmpl.bd_addr[1], param->ble_security.auth_cmpl.bd_addr[0],
				param->ble_security.auth_cmpl.fail_reason);
		break;

	case BK_BLE_GAP_UPDATE_CONN_PARAMS_EVT:
		BK_LOGI(TAG, "conn params updated status=%d interval=0x%x latency=%u timeout=0x%x\n",
				param->update_conn_params.status, param->update_conn_params.conn_int,
				param->update_conn_params.latency, param->update_conn_params.timeout);
		break;

	default:
		break;
	}

	return DM_BLE_GAP_APP_CB_RET_NO_INTERESTING;
}

static int32_t dm_ble_gatt_client_demo_gattc_cb(bk_gattc_cb_event_t event, bk_gatt_if_t gattc_if,
										   bk_ble_gattc_cb_param_t *param)
{
	(void)gattc_if;

	switch (event)
	{
	case BK_GATTC_CONNECT_EVT:
		memset(&s_demo_env, 0, sizeof(s_demo_env));
		s_demo_env.connected = 1;
		s_demo_env.conn_id = param->connect.conn_id;
		memcpy(s_demo_env.peer_addr, param->connect.remote_bda, sizeof(s_demo_env.peer_addr));
		dm_ble_gatt_client_demo_reset_db();

		BK_LOGI(TAG, "connected conn_id=%u peer=%02x:%02x:%02x:%02x:%02x:%02x\n",
				param->connect.conn_id,
				param->connect.remote_bda[5], param->connect.remote_bda[4],
				param->connect.remote_bda[3], param->connect.remote_bda[2],
				param->connect.remote_bda[1], param->connect.remote_bda[0]);
		bk_dm_prf_gattc_discover(param->connect.conn_id);
		break;

	case BK_GATTC_DISCONNECT_EVT:
		BK_LOGI(TAG, "disconnected conn_id=%u reason=0x%x\n",
				param->disconnect.conn_id, param->disconnect.reason);
		memset(&s_demo_env, 0, sizeof(s_demo_env));
		break;

	case BK_GATTC_DIS_RES_SERVICE_EVT:
		BK_LOGI(TAG, "service discovery conn_id=%u count=%u\n",
				param->dis_res_service.conn_id, param->dis_res_service.count);
		for (uint32_t i = 0; i < param->dis_res_service.count; i++)
		{
			uint16_t uuid16 = dm_ble_gatt_client_demo_uuid16(&param->dis_res_service.array[i].srvc_id.uuid);
			uint32_t index = s_demo_env.service_count;

			BK_LOGI(TAG, "service[%u] uuid=0x%04x primary=%u handle=%u~%u\n",
					i,
					uuid16,
					param->dis_res_service.array[i].is_primary,
					param->dis_res_service.array[i].start_handle,
					param->dis_res_service.array[i].end_handle);

			if (index < DM_GATTC_DEMO_MAX_SERVICE_COUNT)
			{
				s_demo_env.services[index].start_handle = param->dis_res_service.array[i].start_handle;
				s_demo_env.services[index].end_handle = param->dis_res_service.array[i].end_handle;
				s_demo_env.services[index].uuid16 = uuid16;
				s_demo_env.services[index].uuid_len = param->dis_res_service.array[i].srvc_id.uuid.len;
				s_demo_env.services[index].is_primary = param->dis_res_service.array[i].is_primary;
				s_demo_env.service_count++;
			}
			else
			{
				BK_LOGW(TAG, "service db full, drop service uuid=0x%04x\n", uuid16);
			}
		}
		break;

	case BK_GATTC_DIS_RES_CHAR_EVT:
		BK_LOGI(TAG, "char discovery conn_id=%u count=%u\n",
				param->dis_res_char.conn_id, param->dis_res_char.count);
		for (uint32_t i = 0; i < param->dis_res_char.count; i++)
		{
			uint16_t uuid16 = dm_ble_gatt_client_demo_uuid16(&param->dis_res_char.array[i].uuid.uuid);
			uint32_t index = s_demo_env.char_count;

			BK_LOGI(TAG, "char[%u] uuid=0x%04x decl=%u~%u value_handle=%u prop=0x%x\n",
					i,
					uuid16,
					param->dis_res_char.array[i].start_handle,
					param->dis_res_char.array[i].end_handle,
					param->dis_res_char.array[i].char_value_handle,
					param->dis_res_char.array[i].prop);

			if (index < DM_GATTC_DEMO_MAX_CHAR_COUNT)
			{
				s_demo_env.chars[index].start_handle = param->dis_res_char.array[i].start_handle;
				s_demo_env.chars[index].end_handle = param->dis_res_char.array[i].end_handle;
				s_demo_env.chars[index].value_handle = param->dis_res_char.array[i].char_value_handle;
				s_demo_env.chars[index].uuid16 = uuid16;
				s_demo_env.chars[index].uuid_len = param->dis_res_char.array[i].uuid.uuid.len;
				s_demo_env.chars[index].prop = param->dis_res_char.array[i].prop;
				s_demo_env.char_count++;
			}
			else
			{
				BK_LOGW(TAG, "char db full, drop char uuid=0x%04x\n", uuid16);
			}
		}
		break;

	case BK_GATTC_DIS_RES_CHAR_DESC_EVT:
		BK_LOGI(TAG, "desc discovery conn_id=%u count=%u\n",
				param->dis_res_char_desc.conn_id, param->dis_res_char_desc.count);
		for (uint32_t i = 0; i < param->dis_res_char_desc.count; i++)
		{
			uint16_t uuid16 = dm_ble_gatt_client_demo_uuid16(&param->dis_res_char_desc.array[i].uuid.uuid);
			uint32_t index = s_demo_env.desc_count;

			BK_LOGI(TAG, "desc[%u] uuid=0x%04x char_handle=%u desc_handle=%u\n",
					i,
					uuid16,
					param->dis_res_char_desc.array[i].char_handle,
					param->dis_res_char_desc.array[i].desc_handle);

			if (index < DM_GATTC_DEMO_MAX_DESC_COUNT)
			{
				s_demo_env.descs[index].char_handle = param->dis_res_char_desc.array[i].char_handle;
				s_demo_env.descs[index].desc_handle = param->dis_res_char_desc.array[i].desc_handle;
				s_demo_env.descs[index].uuid16 = uuid16;
				s_demo_env.descs[index].uuid_len = param->dis_res_char_desc.array[i].uuid.uuid.len;
				s_demo_env.desc_count++;
			}
			else
			{
				BK_LOGW(TAG, "desc db full, drop desc uuid=0x%04x\n", uuid16);
			}
		}
		break;

	case BK_GATTC_DIS_SRVC_CMPL_EVT:
		BK_LOGI(TAG, "discovery complete conn_id=%u status=0x%x\n",
				param->dis_srvc_cmpl.conn_id, param->dis_srvc_cmpl.status);
		dm_ble_gatt_client_demo_dump_db();
		break;

	case BK_GATTC_READ_CHAR_EVT:
		BK_LOGI(TAG, "read complete conn_id=%u handle=%u status=0x%x len=%u\n",
				param->read.conn_id, param->read.handle, param->read.status, param->read.value_len);
		if (!param->read.status && param->read.value && param->read.value_len)
		{
			dm_ble_gatt_client_demo_dump_value("read value", param->read.value, param->read.value_len);
		}
		break;

	case BK_GATTC_WRITE_CHAR_EVT:
		BK_LOGI(TAG, "write complete conn_id=%u handle=%u status=0x%x\n",
				param->write.conn_id, param->write.handle, param->write.status);
		break;

	case BK_GATTC_CFG_MTU_EVT:
		BK_LOGI(TAG, "mtu exchanged conn_id=%u mtu=%u status=0x%x\n",
				param->cfg_mtu.conn_id, param->cfg_mtu.mtu, param->cfg_mtu.status);
		break;

	case BK_GATTC_NOTIFY_EVT:
		BK_LOGI(TAG, "%s conn_id=%u handle=%u len=%u\n",
				param->notify.is_notify ? "notify" : "indicate",
				param->notify.conn_id,
				param->notify.handle,
				param->notify.value_len);
		if (param->notify.value && param->notify.value_len)
		{
			dm_ble_gatt_client_demo_dump_value("notify value", param->notify.value, param->notify.value_len);
		}
		break;

	default:
		break;
	}

	return 0;
}

int dm_ble_gatt_client_demo_read(uint16_t conn_id, uint16_t attr_handle, uint8_t *data, uint16_t len)
{
	return bk_dm_prf_gattc_read(conn_id, attr_handle, data, len);
}

int dm_ble_gatt_client_demo_write_ccc(uint16_t conn_id, uint16_t ccc_handle, uint8_t enable)
{
	uint16_t ccc_value = enable;

	if (!ccc_handle)
	{
		BK_LOGE(TAG, "invalid ccc handle\n");
		return -1;
	}

	if (enable > 2)
	{
		BK_LOGE(TAG, "invalid ccc value %u\n", enable);
		return -1;
	}

	return bk_dm_prf_gattc_write(conn_id,
								 ccc_handle,
								 (uint8_t *)&ccc_value,
								 sizeof(ccc_value));
}

static void dm_ble_gatt_client_demo_cli_usage(void)
{
	BK_LOGI(TAG, "Usage:\n");
	BK_LOGI(TAG, "  %s gattc scan_start [duration] [period]\n", DM_GATTC_DEMO_CLI_CMD);
	BK_LOGI(TAG, "  %s gattc scan_stop\n", DM_GATTC_DEMO_CLI_CMD);
	BK_LOGI(TAG, "  %s gattc read <conn_id> <attr_handle> [len]\n", DM_GATTC_DEMO_CLI_CMD);
	BK_LOGI(TAG, "  %s gattc write_ccc <conn_id> <ccc_handle> <0|1|2>\n", DM_GATTC_DEMO_CLI_CMD);
	BK_LOGI(TAG, "Common connect/disconnect/discover/write/security commands are provided by ble_gatt_demo.\n");
}

static void dm_ble_gatt_client_demo_cli(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = CLI_CMD_RSP_SUCCEED;
	int ret = 0;

	(void)xWriteBufferLen;

	if (argc == 1 || !os_strcmp(argv[1], "-h") || !os_strcmp(argv[1], "--help"))
	{
		dm_ble_gatt_client_demo_cli_usage();
		os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
		return;
	}

	if (argc < 3)
	{
		goto error;
	}

	if (!os_strcmp(argv[1], "gattc") && !os_strcmp(argv[2], "scan_start"))
	{
		bk_ble_ext_scan_params_t param = {0};
		uint32_t duration = 0;
		uint32_t period = 0;

		if (argc >= 4 && sscanf(argv[3], "%u", &duration) != 1)
		{
			goto error;
		}

		if (argc >= 5 && sscanf(argv[4], "%u", &period) != 1)
		{
			goto error;
		}

		param.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
		param.filter_policy = BLE_SCAN_FILTER_ALLOW_ALL;
		param.scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE;
		param.cfg_mask = BK_BLE_GAP_EXT_SCAN_CFG_UNCODE_MASK;
		param.uncoded_cfg.scan_interval = 0x64;
		param.uncoded_cfg.scan_window = 0x1e;
		param.uncoded_cfg.scan_type = BLE_SCAN_TYPE_ACTIVE;

		ret = bk_ble_gap_set_scan_params(&param);
		if (!ret)
		{
			rtos_delay_milliseconds(100);
			ret = bk_ble_gap_start_scan(duration, (uint16_t)period);
		}
	}
	else if (!os_strcmp(argv[1], "gattc") && !os_strcmp(argv[2], "scan_stop"))
	{
		ret = bk_ble_gap_stop_scan();
	}
	else if (!os_strcmp(argv[1], "gattc") && !os_strcmp(argv[2], "read") && argc >= 5)
	{
		uint16_t conn_id = 0;
		uint16_t attr_handle = 0;
		uint32_t len = 128;
		uint8_t data[128] = {0};

		if (sscanf(argv[3], "%hu", &conn_id) != 1 ||
			sscanf(argv[4], "%hu", &attr_handle) != 1)
		{
			goto error;
		}

		if (argc >= 6 && (sscanf(argv[5], "%u", &len) != 1 || len > sizeof(data)))
		{
			goto error;
		}

		ret = dm_ble_gatt_client_demo_read(conn_id, attr_handle, data, (uint16_t)len);
	}
	else if (!os_strcmp(argv[1], "gattc") && !os_strcmp(argv[2], "write_ccc") && argc >= 6)
	{
		uint16_t conn_id = 0;
		uint16_t ccc_handle = 0;
		uint32_t ccc_value = 0;

		if (sscanf(argv[3], "%hu", &conn_id) != 1 ||
			sscanf(argv[4], "%hu", &ccc_handle) != 1 ||
			sscanf(argv[5], "%u", &ccc_value) != 1 ||
			ccc_value > 2)
		{
			goto error;
		}

		ret = dm_ble_gatt_client_demo_write_ccc(conn_id, ccc_handle, (uint8_t)ccc_value);
	}
	else
	{
		goto error;
	}

	if (ret)
	{
		goto error;
	}

	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return;

error:
	msg = CLI_CMD_RSP_ERROR;
	dm_ble_gatt_client_demo_cli_usage();
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

static const struct cli_command s_dm_ble_gatt_client_demo_cmds[] =
{
	{DM_GATTC_DEMO_CLI_CMD, "dm ble gatt client demo", dm_ble_gatt_client_demo_cli},
};

int dm_ble_gatt_client_demo_init(void)
{
	uint8_t rpa = 0;
	uint8_t pa = 1;
	cli_gatt_param_t param = {0};

	param.p_rpa = &rpa;
	param.p_pa = &pa;

	bk_dm_prf_gap_main(&param);
	bk_dm_prf_gattc_main(&param);
	bk_dm_prf_gattc_add_gattc_callback(dm_ble_gatt_client_demo_gattc_cb);
	bk_dm_prf_gap_add_gap_callback(dm_ble_gatt_client_demo_gap_cb);
	cli_register_commands(s_dm_ble_gatt_client_demo_cmds,
						  sizeof(s_dm_ble_gatt_client_demo_cmds) / sizeof(s_dm_ble_gatt_client_demo_cmds[0]));

	BK_LOGI(TAG, "dm_ble_gatt_client demo init success\n");
	return 0;
}
