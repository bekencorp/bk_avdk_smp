#include <stdio.h>
#include <string.h>

#include <components/log.h>
#include <os/os.h>
#include <os/mem.h>

#include "cli.h"
#include "components/bluetooth/bk_dm_bluetooth_types.h"
#include "components/bluetooth/bk_dm_gap_ble.h"
#include "dm_gatt.h"
#include "dm_gatts.h"
#include "dm_ble_gatt_server_demo.h"

#define TAG "dm_ble_gatt_server"
#define ADV_HANDLE 0
#define BEKEN_COMPANY_ID 0x05F0
#define DM_BLE_GATT_SERVER_ADV_OP_TIMEOUT_MS 2000
#define DM_GATTS_DEMO_CLI_CMD "dm_ble_gatt_server"

enum
{
	DM_GATTS_IDX_SVC,
	DM_GATTS_IDX_NOTIFY_CHAR,
	DM_GATTS_IDX_NOTIFY_CHAR_CCC,
	DM_GATTS_IDX_WRITE_CHAR,
	DM_GATTS_IDX_RW_N2_CHAR,
	DM_GATTS_IDX_RW_N3_CHAR,
	DM_GATTS_IDX_RW_N4_CHAR,
	DM_GATTS_IDX_NB,
};

static uint16_t s_attr_handles[DM_GATTS_IDX_NB];
static uint16_t s_ccc_value;
static uint8_t s_notify_value[2] = {0x12, 0x34};
static uint8_t s_write_value[128];
static uint8_t s_rw_n2_value[128];
static uint8_t s_rw_n3_value[128];
static uint8_t s_rw_n4_value[128];
static uint16_t s_conn_id = 0xFFFF;
static beken_semaphore_t s_adv_op_sema;

static bk_gatts_attr_db_t s_gatts_db[DM_GATTS_IDX_NB] =
{
	[DM_GATTS_IDX_SVC] =
	{
		BK_GATT_PRIMARY_SERVICE_DECL(DM_BLE_GATT_SERVER_DEMO_SERVICE_UUID),
	},
	[DM_GATTS_IDX_NOTIFY_CHAR] =
	{
		BK_GATT_CHAR_DECL(DM_BLE_GATT_SERVER_DEMO_NOTIFY_CHAR_UUID,
						  sizeof(s_notify_value),
						  s_notify_value,
						  BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_NOTIFY,
						  BK_GATT_PERM_READ,
						  BK_GATT_AUTO_RSP),
	},
	[DM_GATTS_IDX_NOTIFY_CHAR_CCC] =
	{
		BK_GATT_CHAR_DESC_DECL(BK_GATT_UUID_CHAR_CLIENT_CONFIG,
							   sizeof(s_ccc_value),
							   (uint8_t *)&s_ccc_value,
							   BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
							   BK_GATT_AUTO_RSP),
	},
	[DM_GATTS_IDX_WRITE_CHAR] =
	{
		BK_GATT_CHAR_DECL(DM_BLE_GATT_SERVER_DEMO_WRITE_CHAR_UUID,
						  sizeof(s_write_value),
						  s_write_value,
						  BK_GATT_CHAR_PROP_BIT_WRITE | BK_GATT_CHAR_PROP_BIT_WRITE_NR,
						  BK_GATT_PERM_WRITE,
						  BK_GATT_AUTO_RSP),
	},
	[DM_GATTS_IDX_RW_N2_CHAR] =
	{
		BK_GATT_CHAR_DECL(DM_BLE_GATT_SERVER_DEMO_RW_N2_CHAR_UUID,
						  sizeof(s_rw_n2_value),
						  s_rw_n2_value,
						  BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_WRITE,
						  BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
						  BK_GATT_AUTO_RSP),
	},
	[DM_GATTS_IDX_RW_N3_CHAR] =
	{
		BK_GATT_CHAR_DECL(DM_BLE_GATT_SERVER_DEMO_RW_N3_CHAR_UUID,
						  sizeof(s_rw_n3_value),
						  s_rw_n3_value,
						  BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_WRITE,
						  BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
						  BK_GATT_AUTO_RSP),
	},
	[DM_GATTS_IDX_RW_N4_CHAR] =
	{
		BK_GATT_CHAR_DECL(DM_BLE_GATT_SERVER_DEMO_RW_N4_CHAR_UUID,
						  sizeof(s_rw_n4_value),
						  s_rw_n4_value,
						  BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_WRITE,
						  BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
						  BK_GATT_AUTO_RSP),
	},
};

static void dm_ble_gatt_server_demo_dump_value(const char *prefix, const uint8_t *data, uint16_t len)
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

static void dm_ble_gatt_server_demo_store_rw_value(uint8_t *dst, uint16_t dst_len,
												   uint16_t attr_idx,
												   const uint8_t *value,
												   uint16_t value_len,
												   const char *label)
{
	uint16_t len = value_len > dst_len ? dst_len : value_len;

	os_memset(dst, 0, dst_len);
	os_memcpy(dst, value, len);
	s_gatts_db[attr_idx].att_desc.value.attr_len = len;
	dm_ble_gatt_server_demo_dump_value(label, dst, len);
}

static int32_t dm_ble_gatt_server_demo_db_cb(bk_gatts_cb_event_t event, bk_gatt_if_t gatts_if,
									  bk_ble_gatts_cb_param_t *param)
{
	(void)gatts_if;

	switch (event)
	{
	case BK_GATTS_READ_EVT:
		BK_LOGI(TAG, "read conn_id=%u handle=%u offset=%u need_rsp=%u\n",
				param->read.conn_id, param->read.handle, param->read.offset, param->read.need_rsp);
		break;

	case BK_GATTS_WRITE_EVT:
		BK_LOGI(TAG, "write conn_id=%u handle=%u len=%u need_rsp=%u\n",
				param->write.conn_id, param->write.handle, param->write.len, param->write.need_rsp);

		if (param->write.handle == s_attr_handles[DM_GATTS_IDX_WRITE_CHAR])
		{
			uint16_t len = param->write.len > sizeof(s_write_value) ? sizeof(s_write_value) : param->write.len;
			os_memcpy(s_write_value, param->write.value, len);
			s_gatts_db[DM_GATTS_IDX_WRITE_CHAR].att_desc.value.attr_len = len;
			dm_ble_gatt_server_demo_dump_value("write-only value", s_write_value, len);
		}
		else if (param->write.handle == s_attr_handles[DM_GATTS_IDX_RW_N2_CHAR])
		{
			dm_ble_gatt_server_demo_store_rw_value(s_rw_n2_value, sizeof(s_rw_n2_value),
												   DM_GATTS_IDX_RW_N2_CHAR,
												   param->write.value,
												   param->write.len,
												   "N2 read/write value");
		}
		else if (param->write.handle == s_attr_handles[DM_GATTS_IDX_RW_N3_CHAR])
		{
			dm_ble_gatt_server_demo_store_rw_value(s_rw_n3_value, sizeof(s_rw_n3_value),
												   DM_GATTS_IDX_RW_N3_CHAR,
												   param->write.value,
												   param->write.len,
												   "N3 read/write value");
		}
		else if (param->write.handle == s_attr_handles[DM_GATTS_IDX_RW_N4_CHAR])
		{
			dm_ble_gatt_server_demo_store_rw_value(s_rw_n4_value, sizeof(s_rw_n4_value),
												   DM_GATTS_IDX_RW_N4_CHAR,
												   param->write.value,
												   param->write.len,
												   "N4 read/write value");
		}
		break;

	default:
		break;
	}

	return 0;
}

static int32_t dm_ble_gatt_server_demo_gatts_cb(bk_gatts_cb_event_t event, bk_gatt_if_t gatts_if,
										 bk_ble_gatts_cb_param_t *param)
{
	(void)gatts_if;

	switch (event)
	{
	case BK_GATTS_CONNECT_EVT:
		s_conn_id = param->connect.conn_id;
		BK_LOGI(TAG, "connected conn_id=%u peer=%02x:%02x:%02x:%02x:%02x:%02x\n",
				s_conn_id,
				param->connect.remote_bda[5], param->connect.remote_bda[4],
				param->connect.remote_bda[3], param->connect.remote_bda[2],
				param->connect.remote_bda[1], param->connect.remote_bda[0]);
		break;

	case BK_GATTS_DISCONNECT_EVT:
		BK_LOGI(TAG, "disconnected conn_id=%u reason=0x%x\n",
				param->disconnect.conn_id, param->disconnect.reason);
		s_conn_id = 0xFFFF;
		break;

	case BK_GATTS_CONF_EVT:
		BK_LOGI(TAG, "notify/indicate confirm status=%d\n", param->conf.status);
		break;

	case BK_GATTS_MTU_EVT:
		BK_LOGI(TAG, "mtu exchanged conn_id=%u mtu=%u\n", param->mtu.conn_id, param->mtu.mtu);
		break;

	case BK_GATTS_EXEC_WRITE_EVT:
		BK_LOGI(TAG, "exec write conn_id=%u flag=0x%x\n",
				param->exec_write.conn_id, param->exec_write.exec_write_flag);
		break;

	default:
		break;
	}

	return 0;
}

/* Additive GAP callback: serialize adv ops and log pairing/bonding results. */
static int32_t dm_ble_gatt_server_demo_gap_cb(bk_ble_gap_cb_event_t event, bk_ble_gap_cb_param_t *param)
{
	switch (event)
	{
	case BK_BLE_GAP_EXT_ADV_SET_RAND_ADDR_COMPLETE_EVT:
	case BK_BLE_GAP_EXT_ADV_PARAMS_SET_COMPLETE_EVT:
	case BK_BLE_GAP_EXT_ADV_DATA_SET_COMPLETE_EVT:
	case BK_BLE_GAP_EXT_SCAN_RSP_DATA_SET_COMPLETE_EVT:
	case BK_BLE_GAP_EXT_ADV_START_COMPLETE_EVT:
		if (s_adv_op_sema)
		{
			rtos_set_semaphore(&s_adv_op_sema);
		}
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

static int dm_ble_gatt_server_demo_wait_adv_op(const char *step)
{
	int ret = rtos_get_semaphore(&s_adv_op_sema, DM_BLE_GATT_SERVER_ADV_OP_TIMEOUT_MS);

	if (ret != BK_OK)
	{
		BK_LOGE(TAG, "wait %s complete failed ret=%d\n", step, ret);
	}

	return ret;
}

/* Device name is set by the component; here we only set adv data and start adv. */
static int dm_ble_gatt_server_demo_start_adv(void)
{
	int ret;
	uint8_t identity_addr[6] = {0};
	bk_ble_gap_ext_adv_params_t adv_param = {0};
	/* 16-bit service UUID embedded in the BLE base UUID; the API folds it back to 16-bit. */
	uint8_t service_uuid[16] =
	{
		0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
		0x00, 0x10, 0x00, 0x00,
		DM_BLE_GATT_SERVER_DEMO_SERVICE_UUID & 0xFF,
		(DM_BLE_GATT_SERVER_DEMO_SERVICE_UUID >> 8) & 0xFF,
		0x00, 0x00,
	};
	uint8_t company_id[2] = {BEKEN_COMPANY_ID & 0xFF, BEKEN_COMPANY_ID >> 8};
	bk_ble_adv_data_t adv_data = {0};
	const bk_ble_gap_ext_adv_t ext_adv =
	{
		.instance = ADV_HANDLE,
		.duration = 0,
		.max_events = 0,
	};

	adv_param.type = BK_BLE_GAP_SET_EXT_ADV_PROP_LEGACY_IND;
	adv_param.interval_min = 0x120;
	adv_param.interval_max = 0x160;
	adv_param.channel_map = BK_ADV_CHNL_ALL;
	adv_param.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
	adv_param.filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;
	adv_param.tx_power = EXT_ADV_TX_PWR_NO_PREFERENCE;
	adv_param.primary_phy = BK_BLE_GAP_PRI_PHY_1M;
	adv_param.secondary_phy = BK_BLE_GAP_PHY_1M;
	ret = bk_ble_gap_set_adv_params(ADV_HANDLE, &adv_param);
	if (ret)
	{
		BK_LOGE(TAG, "set adv params failed ret=%d\n", ret);
		return ret;
	}
	if (dm_ble_gatt_server_demo_wait_adv_op("adv params") != BK_OK)
	{
		return BK_FAIL;
	}

	adv_data.set_scan_rsp = 0;
	adv_data.include_name = 0;
	adv_data.manufacturer_len = sizeof(company_id);
	adv_data.p_manufacturer_data = company_id;
	adv_data.service_uuid_len = sizeof(service_uuid);
	adv_data.p_service_uuid = service_uuid;
	adv_data.flag = 0x06;
	ret = bk_ble_gap_set_adv_data(&adv_data);
	if (ret)
	{
		BK_LOGE(TAG, "set adv data failed ret=%d\n", ret);
		return ret;
	}
	if (dm_ble_gatt_server_demo_wait_adv_op("adv data") != BK_OK)
	{
		return BK_FAIL;
	}

	memset(&adv_data, 0, sizeof(adv_data));
	adv_data.set_scan_rsp = 1;
	adv_data.include_name = 1;
	ret = bk_ble_gap_set_adv_data(&adv_data);
	if (ret)
	{
		BK_LOGE(TAG, "set scan response failed ret=%d\n", ret);
		return ret;
	}
	if (dm_ble_gatt_server_demo_wait_adv_op("scan response") != BK_OK)
	{
		return BK_FAIL;
	}

	ret = bk_ble_gap_adv_start(1, &ext_adv);
	if (ret)
	{
		BK_LOGE(TAG, "start advertising failed ret=%d\n", ret);
		return ret;
	}
	if (dm_ble_gatt_server_demo_wait_adv_op("adv start") != BK_OK)
	{
		return BK_FAIL;
	}

	/* Print the device name/service so testers know what to connect to. */
	bk_dm_prf_gap_get_identity_addr(identity_addr);
	BK_LOGI(TAG, "adv start ok, connect to device name BKDMBLE-%02X%02X%02X (service 0x%04X)\n",
			identity_addr[2], identity_addr[1], identity_addr[0],
			DM_BLE_GATT_SERVER_DEMO_SERVICE_UUID);

	return BK_OK;
}

int dm_ble_gatt_server_demo_send_notify(const uint8_t *data, uint16_t len)
{
	if (s_conn_id == 0xFFFF || !data || !len)
	{
		return -1;
	}

	dm_ble_gatt_server_demo_dump_value("notify value", data, len);

	return bk_dm_prf_gatts_send_notify(s_conn_id,
									   s_attr_handles[DM_GATTS_IDX_NOTIFY_CHAR],
									   (uint8_t *)data,
									   len,
									   1);
}

static void dm_ble_gatt_server_demo_cli_usage(void)
{
	BK_LOGI(TAG, "Usage:\n");
	BK_LOGI(TAG, "  %s gatts notify <data>\n", DM_GATTS_DEMO_CLI_CMD);
	BK_LOGI(TAG, "Common disconnect/adv/security commands are provided by ble_gatt_demo.\n");
}

static void dm_ble_gatt_server_demo_cli(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = CLI_CMD_RSP_SUCCEED;
	int ret = 0;

	(void)xWriteBufferLen;

	if (argc == 1 || !os_strcmp(argv[1], "-h") || !os_strcmp(argv[1], "--help"))
	{
		dm_ble_gatt_server_demo_cli_usage();
		os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
		return;
	}

	if (argc < 3)
	{
		goto error;
	}

	if (argc >= 4 && !os_strcmp(argv[1], "gatts") && !os_strcmp(argv[2], "notify"))
	{
		ret = dm_ble_gatt_server_demo_send_notify((const uint8_t *)argv[3], (uint16_t)strlen(argv[3]));
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
	dm_ble_gatt_server_demo_cli_usage();
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

static const struct cli_command s_dm_ble_gatt_server_demo_cmds[] =
{
	{DM_GATTS_DEMO_CLI_CMD, "dm ble gatt server demo", dm_ble_gatt_server_demo_cli},
};

int dm_ble_gatt_server_demo_init(void)
{
	uint8_t rpa = 0;
	uint8_t pa = 1;
	cli_gatt_param_t param = {0};

	param.p_rpa = &rpa;
	param.p_pa = &pa;

	if (!s_adv_op_sema && rtos_init_semaphore(&s_adv_op_sema, 1) != BK_OK)
	{
		BK_LOGE(TAG, "init adv op semaphore failed\n");
		return -1;
	}

	/* Bring up the host, then add this demo's GATTS DB / GAP callback and start adv. */
	bk_dm_prf_gap_main(&param);
	bk_dm_prf_gatts_main(&param);
	bk_dm_prf_gatts_add_gatts_callback(dm_ble_gatt_server_demo_gatts_cb);
	bk_dm_prf_gatts_reg_db(s_gatts_db, DM_GATTS_IDX_NB, s_attr_handles, dm_ble_gatt_server_demo_db_cb, 1);
	bk_dm_prf_gap_add_gap_callback(dm_ble_gatt_server_demo_gap_cb);
	cli_register_commands(s_dm_ble_gatt_server_demo_cmds,
						  sizeof(s_dm_ble_gatt_server_demo_cmds) / sizeof(s_dm_ble_gatt_server_demo_cmds[0]));

	if (dm_ble_gatt_server_demo_start_adv() != BK_OK)
	{
		BK_LOGE(TAG, "dm_ble_gatt_server demo start adv failed\n");
		return -1;
	}

	BK_LOGI(TAG, "dm_ble_gatt_server demo init success\n");
	return 0;
}
