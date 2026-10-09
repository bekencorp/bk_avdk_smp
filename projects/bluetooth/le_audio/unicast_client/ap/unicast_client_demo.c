#include "unicast_client_demo.h"
#include "../../common/audio.h"

#include <components/bluetooth/bk_assigned_numbers.h>
#include <components/bluetooth/bk_dm_bap.h>
#include <components/bluetooth/bk_dm_bass.h>
#include <components/bluetooth/bk_dm_gap_ble.h>
#include <components/bluetooth/bk_dm_gattc.h>
#include <components/log.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>

#define TAG "lea_ucli"
#define DEMO_MAX_PEERS       8U
#define DEMO_MAX_SOURCES     8U
#define ASST_PA_INFO_REQ     0x01U
#define UCLI_EVT_QUEUE_LEN   24U
#define UCLI_THREAD_PRIO     5U
#define UCLI_THREAD_STACK    4096U
#define UCLI_CMD_TIMEOUT_MS  5000U
#define UCLI_AUTO_CIG_ID     1U
/* CIS IDs are handed out per stream: 1, 2, ... */
#define UCLI_AUTO_CIS_ID(i)  ((uint8_t)((i) + 1U))

typedef struct
{
	uint8_t addr[6];
	uint8_t addr_type;
	uint8_t event_type;
	int8_t rssi;
	char name[32];
} demo_peer_t;

static demo_peer_t s_peers[DEMO_MAX_PEERS];
static uint8_t s_peer_count;
static uint8_t s_scanning;

static bk_bap_source_announce_data_t s_sources[DEMO_MAX_SOURCES];
static uint8_t s_source_count;
static uint8_t s_bcast_scan_on;
static uint16_t s_src_sync_handle = 0xFFFF;
static uint8_t s_deleg_addr[6];
static uint8_t s_deleg_type;
static uint8_t s_have_deleg;
static uint8_t s_past_done;
static uint8_t s_code_done;
static uint8_t s_bcode[16];
static uint8_t s_bcode_set;

static uint8_t s_last_ase;
static uint8_t s_last_role;
static uint16_t s_last_cis_handle;

/*
 * One Sink ASE per audio channel, all in a single CIG. The CIG has to be
 * programmed and its CISes created in one command each, so the ASEs are
 * collected here and the group steps run once every ASE has caught up.
 */
typedef struct
{
	uint8_t  ase_id;
	uint8_t  role;
	uint8_t  stage;   /*!< ucli_stage_t, only ever moves forward */
	uint16_t cis_handle;
} ucli_stream_t;

/*
 * A stream walks these in order, and a group step runs once every stream of the
 * group reached the stage that step needs.
 */
typedef enum
{
	UCLI_STAGE_DISCOVERED = 0,
	UCLI_STAGE_CODEC_CONFIGURED,
	UCLI_STAGE_CIS_ASSIGNED,
	UCLI_STAGE_QOS_CONFIGURED,
	UCLI_STAGE_ENABLING,
	UCLI_STAGE_ISO_READY,
} ucli_stage_t;

static ucli_stream_t s_streams[LE_AUDIO_MAX_STREAMS];
static uint8_t s_stream_count;

typedef enum
{
	UCLI_EVT_CMD_SCAN = 1,
	UCLI_EVT_CMD_LIST,
	UCLI_EVT_CMD_CONNECT_PEER,
	UCLI_EVT_CMD_CONNECT_ADDR,
	UCLI_EVT_CMD_TONE_START,
	UCLI_EVT_CMD_TONE_STOP,
	UCLI_EVT_READY,
	UCLI_EVT_STATE,
	UCLI_EVT_ASE_DISCOVERED,
	UCLI_EVT_CIS_HANDLE,
	UCLI_EVT_CIS_ESTABLISHED,
	UCLI_EVT_ISO_PATH_READY,
	UCLI_EVT_GATT_CONNECTED,
	UCLI_EVT_MTU_UPDATED,
} ucli_evt_type_t;

typedef struct
{
	ucli_evt_type_t type;
	uint8_t sync;
	uint8_t on;
	uint8_t index;
	uint8_t addr[6];
	uint8_t addr_type;
	uint16_t handle;
	union
	{
		bk_bap_unicast_ready_t ready;
		bk_bap_unicast_state_t state;
		bk_bap_unicast_ase_discovered_t ase;
		bk_bap_unicast_cis_info_t cis;
		bk_bap_unicast_iso_path_t iso;
		struct
		{
			uint16_t conn_id;
			uint16_t mtu;
			uint8_t  link_role;
			uint8_t  status;
		} gatt;
	} u;
} ucli_evt_t;

static bk_gatt_if_t s_gattc_if;

/*
 * The profile flow only starts once the ACL is up and the ATT MTU is settled.
 * At the default 23 octet MTU a PAC record read or a Config QoS for two ASEs
 * does not fit in one PDU and the peer answers Invalid Length. The GA bearer
 * reports the link up before the GATT client reports the connection, so both
 * are joined here.
 */
static uint8_t s_acl_up;
static uint8_t s_mtu_ready;

static beken_queue_t s_evt_queue;
static beken_thread_t s_evt_thread;
static beken_semaphore_t s_cmd_sem;
static int s_cmd_result;

static void ucli_set_security(void)
{
	uint8_t iocap = BK_IO_CAP_NONE;
	uint8_t authreq = BK_LE_AUTH_REQ_SC_BOND;

	(void)bk_ble_gap_set_security_param(BK_BLE_SM_IOCAP_MODE, &iocap, sizeof(iocap));
	(void)bk_ble_gap_set_security_param(BK_BLE_SM_AUTHEN_REQ_MODE, &authreq, sizeof(authreq));
	BK_LOGI(TAG, "security: iocap=NONE authreq=SC|BOND\n");
}

static bk_err_t ucli_send(uint16_t handle, uint16_t seq, uint8_t *data, uint16_t len)
{
	return bk_dm_bap_unicast_send(handle, 0, 0, seq, data, len);
}

static int ucli_post_event(const ucli_evt_t *evt, uint32_t timeout_ms)
{
	if (!s_evt_queue || !evt)
	{
		return BK_FAIL;
	}
	return rtos_push_to_queue(&s_evt_queue, (void *)evt, timeout_ms);
}

static int ucli_call_event(ucli_evt_t *evt)
{
	int ret;

	if (!evt)
	{
		return BK_FAIL;
	}
	evt->sync = 1;
	s_cmd_result = BK_FAIL;
	ret = ucli_post_event(evt, BEKEN_WAIT_FOREVER);
	if (ret != BK_OK)
	{
		return ret;
	}
	if (rtos_get_semaphore(&s_cmd_sem, UCLI_CMD_TIMEOUT_MS) != BK_OK)
	{
		return BK_FAIL;
	}
	return s_cmd_result;
}

static void ucli_finish_cmd(const ucli_evt_t *evt, int ret)
{
	if (evt && evt->sync)
	{
		s_cmd_result = ret;
		(void)rtos_set_semaphore(&s_cmd_sem);
	}
}

static void ucli_parse_name(const uint8_t *data, uint8_t len, char *name, uint8_t name_len)
{
	uint8_t pos = 0;

	if (!data || !name || name_len == 0)
	{
		return;
	}

	while (pos + 1 < len)
	{
		uint8_t field_len = data[pos];
		uint8_t type;

		if (field_len == 0 || pos + field_len >= len)
		{
			break;
		}
		type = data[pos + 1];
		if (type == BK_BLE_AD_TYPE_NAME_SHORT || type == BK_BLE_AD_TYPE_NAME_CMPL)
		{
			uint8_t copy_len = field_len - 1;

			if (copy_len >= name_len)
			{
				copy_len = name_len - 1;
			}
			os_memcpy(name, &data[pos + 2], copy_len);
			name[copy_len] = '\0';
			return;
		}
		pos += field_len + 1;
	}
}

static uint8_t ucli_has_uuid16(const uint8_t *data, uint8_t len, uint16_t uuid)
{
	uint8_t pos = 0;

	while (data && pos + 1 < len)
	{
		uint8_t field_len = data[pos];
		uint8_t type;
		uint8_t i;

		if (field_len == 0 || pos + field_len >= len)
		{
			break;
		}

		type = data[pos + 1];
		if (type == BK_BLE_AD_TYPE_16SRV_PART ||
		    type == BK_BLE_AD_TYPE_16SRV_CMPL ||
		    type == BK_BLE_AD_TYPE_SERVICE_DATA)
		{
			for (i = pos + 2; i + 1 <= pos + field_len; i += 2)
			{
				uint16_t u = (uint16_t)data[i] | ((uint16_t)data[i + 1] << 8);

				if (u == uuid)
				{
					return 1;
				}
			}
		}
		pos += field_len + 1;
	}

	return 0;
}

static uint8_t ucli_is_le_audio_adv(const uint8_t *data, uint8_t len)
{
	return ucli_has_uuid16(data, len, BK_BT_UUID_ASCS) ||
	       ucli_has_uuid16(data, len, BK_BT_UUID_PACS) ||
	       ucli_has_uuid16(data, len, BK_BT_UUID_BASS);
}

static int ucli_peer_index(const uint8_t addr[6], uint8_t addr_type)
{
	uint8_t i;

	for (i = 0; i < s_peer_count; i++)
	{
		if (s_peers[i].addr_type == addr_type &&
		    os_memcmp(s_peers[i].addr, addr, sizeof(s_peers[i].addr)) == 0)
		{
			return i;
		}
	}

	return -1;
}

static void ucli_print_peer(uint8_t index, const demo_peer_t *peer)
{
	BK_LOGI(TAG, "[%u] addr=%02x:%02x:%02x:%02x:%02x:%02x type=%u evt=0x%02x rssi=%d name=%s\n",
	        index, peer->addr[5], peer->addr[4], peer->addr[3],
	        peer->addr[2], peer->addr[1], peer->addr[0],
	        peer->addr_type, peer->event_type, peer->rssi,
	        peer->name[0] ? peer->name : "<none>");
}

static void ucli_gap_cb(bk_ble_gap_cb_event_t event, bk_ble_gap_cb_param_t *param)
{
	if (event == BK_BLE_GAP_EXT_ADV_REPORT_EVT)
	{
		bk_ble_gap_ext_adv_reprot_t *r = &param->ext_adv_report.params;
		int index;

		if (!ucli_is_le_audio_adv(r->adv_data, r->adv_data_len))
		{
			return;
		}

		index = ucli_peer_index(r->addr, r->addr_type);
		if (index < 0)
		{
			if (s_peer_count >= DEMO_MAX_PEERS)
			{
				return;
			}
			index = s_peer_count++;
			os_memset(&s_peers[index], 0, sizeof(s_peers[index]));
			os_memcpy(s_peers[index].addr, r->addr, sizeof(s_peers[index].addr));
			s_peers[index].addr_type = r->addr_type;
		}

		s_peers[index].event_type = r->event_type;
		s_peers[index].rssi = r->rssi;
		ucli_parse_name(r->adv_data, r->adv_data_len,
		                s_peers[index].name, sizeof(s_peers[index].name));
		ucli_print_peer((uint8_t)index, &s_peers[index]);
	}
	else if (event == BK_BLE_GAP_EXT_SCAN_START_COMPLETE_EVT)
	{
		BK_LOGI(TAG, "ext scan started\n");
	}
	else if (event == BK_BLE_GAP_EXT_SCAN_STOP_COMPLETE_EVT)
	{
		BK_LOGI(TAG, "ext scan stopped\n");
	}
}

/* GATT client up-calls. GA does not expose the ATT bearer, so the MTU exchange
 * and the connection role come from the BLE GATT interface. */
static int32_t ucli_gattc_cb(bk_gattc_cb_event_t event, bk_gatt_if_t gattc_if,
                             bk_ble_gattc_cb_param_t *param)
{
	ucli_evt_t evt = {0};

	(void)gattc_if;
	if (!param)
	{
		return 0;
	}

	switch (event)
	{
	case BK_GATTC_REG_EVT:
		s_gattc_if = param->reg.gatt_if;
		break;

	case BK_GATTC_CONNECT_EVT:
		evt.type = UCLI_EVT_GATT_CONNECTED;
		evt.u.gatt.conn_id = param->connect.conn_id;
		evt.u.gatt.link_role = param->connect.link_role;
		(void)ucli_post_event(&evt, 0);
		break;

	case BK_GATTC_CFG_MTU_EVT:
		evt.type = UCLI_EVT_MTU_UPDATED;
		evt.u.gatt.conn_id = param->cfg_mtu.conn_id;
		evt.u.gatt.mtu = param->cfg_mtu.mtu;
		evt.u.gatt.status = (uint8_t)param->cfg_mtu.status;
		(void)ucli_post_event(&evt, 0);
		break;

	default:
		break;
	}

	return 0;
}

static void ucli_on_ready(bk_bap_unicast_ready_t *info)
{
	ucli_evt_t evt = {0};

	if (!info)
	{
		return;
	}
	evt.type = UCLI_EVT_READY;
	evt.u.ready = *info;
	(void)ucli_post_event(&evt, 0);
}

static void ucli_on_state(bk_bap_unicast_state_t *info)
{
	ucli_evt_t evt = {0};

	if (!info)
	{
		return;
	}

	evt.type = UCLI_EVT_STATE;
	evt.u.state = *info;
	(void)ucli_post_event(&evt, 0);
}

static void ucli_on_ase_discovered(bk_bap_unicast_ase_discovered_t *info)
{
	ucli_evt_t evt = {0};

	if (!info)
	{
		return;
	}
	evt.type = UCLI_EVT_ASE_DISCOVERED;
	evt.u.ase = *info;
	(void)ucli_post_event(&evt, 0);
}

static void ucli_on_cis_handle(bk_bap_unicast_cis_info_t *info)
{
	ucli_evt_t evt = {0};

	if (!info)
	{
		return;
	}
	evt.type = UCLI_EVT_CIS_HANDLE;
	evt.u.cis = *info;
	(void)ucli_post_event(&evt, 0);
}

static void ucli_on_cis_established(bk_bap_unicast_cis_info_t *info)
{
	ucli_evt_t evt = {0};

	if (!info)
	{
		return;
	}
	evt.type = UCLI_EVT_CIS_ESTABLISHED;
	evt.u.cis = *info;
	(void)ucli_post_event(&evt, 0);
}

static void ucli_on_iso_path_ready(bk_bap_unicast_iso_path_t *info)
{
	ucli_evt_t evt = {0};

	if (!info)
	{
		return;
	}
	evt.type = UCLI_EVT_ISO_PATH_READY;
	evt.u.iso = *info;
	(void)ucli_post_event(&evt, 0);
}

static void ucli_assist_on_announcement(bk_bap_source_announce_data_t *source)
{
	int index;

	if (!source || !s_bcast_scan_on)
	{
		return;
	}

	for (index = 0; index < s_source_count; index++)
	{
		if (s_sources[index].advertising_sid == source->advertising_sid &&
		    s_sources[index].address_type == source->address_type &&
		    os_memcmp(s_sources[index].address, source->address, sizeof(source->address)) == 0)
		{
			s_sources[index] = *source;
			return;
		}
	}

	if (s_source_count >= DEMO_MAX_SOURCES)
	{
		return;
	}

	index = s_source_count++;
	s_sources[index] = *source;
	BK_LOGI(TAG, "source[%d] sid=%u addr=%02x:%02x:%02x:%02x:%02x:%02x type=%u rssi=%d bcast_id=0x%06lx\n",
	        index, source->advertising_sid,
	        source->address[5], source->address[4], source->address[3],
	        source->address[2], source->address[1], source->address[0],
	        source->address_type, source->rssi, (unsigned long)source->broadcast_id);
	BK_LOGI(TAG, "next: ap_cmd le_audio assistant_add <source_index> [bis_mask]\n");
}

/* Transfer our source periodic-adv sync to the delegator (PAST), optionally
 * pushing the broadcast code first. */
static void ucli_assist_try_past(uint16_t acl_handle, uint8_t src_id)
{
	int ret;

	if (!s_have_deleg || s_past_done)
	{
		return;
	}

	if (s_src_sync_handle == 0xFFFF)
	{
		BK_LOGW(TAG, "delegator requests PAST but source is not PA-synced\n");
		return;
	}

	if (s_bcode_set && !s_code_done)
	{
		ret = bk_dm_bass_set_broadcast_code(acl_handle, src_id, s_bcode);
		BK_LOGI(TAG, "Set Broadcast Code src_id=%u ret=%d\n", src_id, ret);
		if (ret == 0)
		{
			s_code_done = 1;
		}
	}

	ret = bk_dm_bass_assistant_send_past(s_deleg_addr, s_deleg_type, s_src_sync_handle);
	BK_LOGI(TAG, "PAST -> delegator sync=0x%04x ret=%d\n", s_src_sync_handle, ret);
	if (ret == 0)
	{
		s_past_done = 1;
	}
}

static int ucli_assist_send_add_source(uint8_t index, uint32_t bis_sync)
{
	bk_bap_source_announce_data_t *source = &s_sources[index];
	uint32_t bcast_id = source->broadcast_id != 0U ? source->broadcast_id : 0x00ADBEEFU;

	BK_LOGI(TAG, "Add Source idx=%u sid=%u bcast_id=0x%06lx bis_sync=0x%08lx\n",
	        index, source->advertising_sid, (unsigned long)bcast_id, (unsigned long)bis_sync);
	return bk_dm_bass_add_source_ex(0, source->address, source->address_type,
	                                source->advertising_sid, bcast_id, bis_sync);
}

static void ucli_assist_on_associate(bk_bap_source_associate_data_t *data)
{
	if (!data)
	{
		return;
	}
	s_src_sync_handle = data->handle;
	BK_LOGI(TAG, "source PA-synced for PAST handle=0x%04x\n", data->handle);
}

static void ucli_assist_on_broadcast_event(bk_bap_sink_cb_evt_t event, uint32_t status)
	{
	BK_LOGI(TAG, "broadcast evt=%d status=0x%02lx\n", event, (unsigned long)status);
}

static void ucli_assist_bass_setup(uint16_t acl_handle, uint8_t status)
{
	BK_LOGI(TAG, "BASS discovered handle=0x%04x status=0x%02x\n", acl_handle, status);
}

static void ucli_assist_bass_rx_state(uint16_t acl_handle, const uint8_t *data, uint16_t len)
{
	if ((data == NULL) || (len == 0))
	{
		BK_LOGI(TAG, "delegator rx_state handle=0x%04x EMPTY\n", acl_handle);
		return;
	}
	if (len < 15)
	{
		BK_LOGI(TAG, "delegator rx_state handle=0x%04x len=%u short\n", acl_handle, len);
		return;
	}

	BK_LOGI(TAG, "delegator rx_state src_id=%u sid=%u pa_sync=%u big_enc=%u num_sg=%u\n",
	        data[0], data[8], data[12], data[13], data[14]);
	if (len >= 19)
	{
		BK_LOGI(TAG, "  bis_sync=0x%02x%02x%02x%02x\n", data[18], data[17], data[16], data[15]);
	}

	if ((data[12] == ASST_PA_INFO_REQ) && (s_past_done == 0) && s_have_deleg)
	{
		ucli_assist_try_past(acl_handle, data[0]);
	}
}

static void ucli_assist_bass_event(uint16_t acl_handle, bk_bass_cb_evt_t evt, uint8_t status)
{
	BK_LOGI(TAG, "BASS client evt=%d handle=0x%04x status=0x%02x\n", evt, acl_handle, status);
}

static int ucli_do_tone_start(const uint16_t *handles, uint8_t count)
{
	le_audio_codec_cfg_t cfg;

	le_audio_codec_cfg_default(&cfg);
	cfg.channels = count;
	return le_audio_audio_tx_start(handles, count, ucli_send, &cfg);
}

static int ucli_do_tone_stop(void)
{
	return le_audio_audio_tx_stop();
}

static void ucli_streams_reset(void);

static int ucli_do_scan(uint8_t on)
{
	if (!on)
	{
		s_scanning = 0;
		return bk_ble_gap_stop_scan();
	}

	bk_ble_ext_scan_params_t param = {0};

	os_memset(s_peers, 0, sizeof(s_peers));
	s_peer_count = 0;
	param.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
	param.filter_policy = BLE_SCAN_FILTER_ALLOW_ALL;
	param.scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE;
	param.cfg_mask = BK_BLE_GAP_EXT_SCAN_CFG_UNCODE_MASK;
	param.uncoded_cfg.scan_interval = 0x64;
	param.uncoded_cfg.scan_window = 0x1e;
	param.uncoded_cfg.scan_type = BLE_SCAN_TYPE_ACTIVE;

	if (bk_ble_gap_set_scan_params(&param) != BK_OK)
	{
		return -1;
	}
	s_scanning = 1;
	return bk_ble_gap_start_scan(0, 0);
}

static void ucli_do_list_peers(void)
{
	if (s_peer_count == 0)
	{
		BK_LOGI(TAG, "no LE Audio peers; run ap_cmd le_audio scan on\n");
		return;
	}

	for (uint8_t i = 0; i < s_peer_count; i++)
	{
		ucli_print_peer(i, &s_peers[i]);
	}
}

static int ucli_do_connect_addr(uint8_t *addr, uint8_t addr_type)
{
	if (!addr)
	{
		return BK_FAIL;
	}
	if (s_scanning)
	{
		bk_ble_gap_stop_scan();
		s_scanning = 0;
	}
	BK_LOGI(TAG, "auto connect %02x:%02x:%02x:%02x:%02x:%02x type=%u\n",
	        addr[5], addr[4], addr[3], addr[2], addr[1], addr[0], addr_type);
	return bk_dm_bap_unicast_connect(addr, addr_type, 1U);
}

static int ucli_do_connect_peer(uint8_t index)
{
	if (index >= s_peer_count)
	{
		BK_LOGW(TAG, "bad peer index=%u count=%u\n", index, s_peer_count);
		return BK_FAIL;
	}
	return ucli_do_connect_addr(s_peers[index].addr, s_peers[index].addr_type);
}

static void ucli_setup_if_ready(void)
{
	int ret;

	if (!s_acl_up || !s_mtu_ready)
	{
		BK_LOGI(TAG, "setup waits for acl=%u mtu=%u\n", s_acl_up, s_mtu_ready);
		return;
	}

	ret = bk_dm_bap_unicast_setup();
	BK_LOGI(TAG, "auto setup ret=%d\n", ret);
}

static void ucli_handle_gatt_connected(uint16_t conn_id, uint8_t link_role)
{
	int ret;

	/* Only the central negotiates the MTU: the stack refuses a request coming
	 * from the peripheral, so on that side there is nothing to wait for. */
	if (link_role != 0U)
	{
		s_mtu_ready = 1U;
	}
	else
	{
		ret = bk_ble_gattc_send_mtu_req(s_gattc_if, conn_id);
		BK_LOGI(TAG, "mtu req conn=%u ret=%d\n", conn_id, ret);
		if (ret != BK_OK)
		{
			s_mtu_ready = 1U;
		}
	}

	ucli_setup_if_ready();
}

static void ucli_handle_ready(const bk_bap_unicast_ready_t *info)
{
	int ret;

	if (!info)
	{
		return;
	}
	if (info->status != 0)
	{
		BK_LOGW(TAG, "ready phase=%u failed status=0x%02x\n", info->phase, info->status);
		return;
	}

	BK_LOGI(TAG, "ready phase=%u acl=0x%04x addr=%02x:%02x:%02x:%02x:%02x:%02x type=%u\n",
	        info->phase, info->acl_handle, info->addr[5], info->addr[4], info->addr[3],
	        info->addr[2], info->addr[1], info->addr[0], info->addr_type);

	if (info->phase == BK_BAP_UNICAST_READY_ACL_CONNECTED)
	{
		s_acl_up = 1U;
		ucli_setup_if_ready();
	}
	else if (info->phase == BK_BAP_UNICAST_READY_GA_SETUP)
	{
		ret = bk_dm_bap_unicast_get_capabilities(BK_GAP_ROLE_SINK);
		BK_LOGI(TAG, "auto caps sink ret=%d\n", ret);
	}
	else if (info->phase == BK_BAP_UNICAST_READY_CAPABILITIES)
	{
		ret = bk_dm_bap_unicast_discover();
		BK_LOGI(TAG, "auto discover ret=%d\n", ret);
	}
	else if (info->phase == BK_BAP_UNICAST_READY_ACL_DISCONNECTED)
	{
		BK_LOGI(TAG, "peer disconnected, stopping tone\n");
		s_acl_up = 0U;
		s_mtu_ready = 0U;
		le_audio_audio_tx_stop();
		ucli_streams_reset();
	}
}

static int ucli_stream_index(uint8_t ase_id)
{
	uint8_t i;

	for (i = 0; i < s_stream_count; i++)
	{
		if (s_streams[i].ase_id == ase_id)
		{
			return (int)i;
		}
	}

	return -1;
}

static void ucli_streams_reset(void)
{
	os_memset(s_streams, 0, sizeof(s_streams));
	s_stream_count = 0;
}

/*
 * Collect the ASE IDs of the group, provided every stream reached the stage the
 * caller needs. Reports the stream still holding the group up otherwise.
 */
static bool ucli_group_at_stage(uint8_t stage, const char *step, uint8_t *ase_ids)
{
	uint8_t i;

	if (s_stream_count == 0U)
	{
		return false;
	}

	for (i = 0; i < s_stream_count; i++)
	{
		if (s_streams[i].stage < stage)
		{
			BK_LOGI(TAG, "%s waits for ase=%u stage=%u\n", step, s_streams[i].ase_id, s_streams[i].stage);
			return false;
		}
		ase_ids[i] = s_streams[i].ase_id;
	}

	return true;
}

/*
 * Configure the ASEs one at a time, each with its own audio location, and wait
 * for the Codec Configured state of one before starting the next. The ASE
 * control point carries a single operation and only one response of a write
 * covering several ASEs surfaces, so this step cannot be batched. The steps
 * after it do work on the whole group.
 */
static void ucli_streams_configure_next(void)
{
	bk_bap_codec_spec_config_t codec_cfg = {0};
	le_audio_codec_cfg_t cfg;
	uint8_t ase_id;
	uint8_t role;
	uint8_t i;
	int ret;

	for (i = 0; i < s_stream_count; i++)
	{
		if (s_streams[i].stage < UCLI_STAGE_CODEC_CONFIGURED)
		{
			break;
		}
	}

	if (i >= s_stream_count)
	{
		/* Every ASE carries its codec configuration */
		return;
	}

	le_audio_codec_cfg_default(&cfg);
	codec_cfg.sampling_frequency = cfg.sf;
	codec_cfg.frame_duration = cfg.fd;
	codec_cfg.octects_per_codec_frame = cfg.frame_bytes;
	codec_cfg.audio_channel_allocation = le_audio_channel_location(i);

	ase_id = s_streams[i].ase_id;
	role = BK_GAP_ROLE_SINK;

	ret = bk_dm_bap_unicast_configure(1U, &ase_id, &role, 0, &codec_cfg);
	BK_LOGI(TAG, "auto config ase=%u location=0x%08lx ret=%d\n",
	        ase_id, (unsigned long)codec_cfg.audio_channel_allocation, ret);
}

/* Program the whole CIG once every ASE reports its codec configuration */
static void ucli_streams_set_cig(void)
{
	uint8_t ase_ids[LE_AUDIO_MAX_STREAMS];
	uint8_t cis_ids[LE_AUDIO_MAX_STREAMS];
	bk_bap_qos_cfg_t qos;
	uint8_t i;
	int ret;

	if (!ucli_group_at_stage(UCLI_STAGE_CODEC_CONFIGURED, "cig", ase_ids))
	{
		return;
	}
	for (i = 0; i < s_stream_count; i++)
	{
		cis_ids[i] = UCLI_AUTO_CIS_ID(i);
	}

	/*
	 * Follows the BAP 48_4_1 preset that goes with the 48 kHz / 10 ms / 120
	 * octet codec configuration - the acceptor only exposes preferences, the
	 * initiator decides. Two deviations are forced by the controller, which
	 * pins the ISO interval to the SDU interval and rejects a CIG whose total
	 * air time exceeds it: the group is interleaved so the CIS share sub event
	 * slots instead of being laid end to end, and the retransmissions stay at 2
	 * because the preset value of 5 needs more sub events than fit in 10 ms.
	 */
	os_memset(&qos, 0, sizeof(qos));
	qos.framing = BK_BAP_QOS_FRAMING_UNFRAMED;
	qos.phy = BK_BAP_QOS_PHY_2M;
	qos.retransmission_number = BK_BAP_QOS_RETRANSMISSION_2;
	qos.max_transport_latency = BK_BAP_QOS_LATENCY_20MS;
	qos.presentation_delay = BK_BAP_QOS_PRESENTATION_DELAY_40MS;
	qos.packing = (s_stream_count > 1U) ? BK_BAP_QOS_PACKING_INTERLEAVED
	                                    : BK_BAP_QOS_PACKING_SEQUENTIAL;

	ret = bk_dm_bap_unicast_set_cig(UCLI_AUTO_CIG_ID, s_stream_count, ase_ids, cis_ids, &qos);
	BK_LOGI(TAG, "auto cig streams=%u packing=%u ret=%d\n", s_stream_count, qos.packing, ret);
}

/* QoS for the group, once the CIG handed out a CIS handle to every ASE */
static void ucli_streams_qos(void)
{
	uint8_t ase_ids[LE_AUDIO_MAX_STREAMS];
	int ret;

	if (!ucli_group_at_stage(UCLI_STAGE_CIS_ASSIGNED, "qos", ase_ids))
	{
		return;
	}

	ret = bk_dm_bap_unicast_qos(s_stream_count, ase_ids);
	BK_LOGI(TAG, "auto qos streams=%u ret=%d\n", s_stream_count, ret);
}

/* Enable the group, once every ASE reports its QoS configuration */
static void ucli_streams_enable(void)
{
	uint8_t ase_ids[LE_AUDIO_MAX_STREAMS];
	int ret;

	if (!ucli_group_at_stage(UCLI_STAGE_QOS_CONFIGURED, "enable", ase_ids))
	{
		return;
	}

	ret = bk_dm_bap_unicast_enable(s_stream_count, ase_ids, BK_GAP_DEFAULT_CONTEXTS);
	BK_LOGI(TAG, "auto enable streams=%u ret=%d\n", s_stream_count, ret);
}

/* Create every CIS of the CIG with one command, once all ASEs are enabling */
static void ucli_streams_create_cis(void)
{
	uint8_t ase_ids[LE_AUDIO_MAX_STREAMS];
	int ret;

	if (!ucli_group_at_stage(UCLI_STAGE_ENABLING, "create cis", ase_ids))
	{
		return;
	}

	ret = bk_dm_bap_unicast_create_cis(s_stream_count, ase_ids);
	BK_LOGI(TAG, "auto cis streams=%u ret=%d\n", s_stream_count, ret);
}

static void ucli_handle_ase_discovered(const bk_bap_unicast_ase_discovered_t *info)
{
	le_audio_codec_cfg_t cfg;

	if (!info)
	{
		return;
	}
	s_last_ase = info->ase_id;
	s_last_role = info->ase_role;
	BK_LOGI(TAG, "ase_discovered ase=0x%02x role=%u state=0x%02x acl=0x%04x last=%u\n",
	        info->ase_id, info->ase_role, info->ase_state, info->acl_handle, info->last);

	le_audio_codec_cfg_default(&cfg);
	if (ucli_stream_index(info->ase_id) >= 0)
	{
		/* Already collected */
	}
	else if (s_stream_count >= cfg.channels)
	{
		BK_LOGI(TAG, "ase=%u ignored, already using %u streams\n", info->ase_id, s_stream_count);
	}
	else
	{
		s_streams[s_stream_count].ase_id = info->ase_id;
		s_streams[s_stream_count].role = info->ase_role;
		s_stream_count++;
	}

	/* Wait for the whole ASE list before touching the control point */
	if (info->last)
	{
		ucli_streams_configure_next();
	}
}

static void ucli_handle_state(const bk_bap_unicast_state_t *info)
{
	int index;

	if (!info)
	{
		return;
	}
	BK_LOGI(TAG, "state=%u ase=0x%02x role=%u acl=0x%04x\n",
	        info->state, info->ase_id, info->ase_role, info->acl_handle);
	index = ucli_stream_index(info->ase_id);
	if (index < 0)
	{
		return;
	}

	if (info->state == BK_BAP_UNICAST_STATE_CODEC_CONFIGURED)
	{
		s_streams[index].stage = UCLI_STAGE_CODEC_CONFIGURED;
		/* Next ASE, then the CIG once all of them carry a configuration */
		ucli_streams_configure_next();
		ucli_streams_set_cig();
	}
	else if (info->state == BK_BAP_UNICAST_STATE_QOS_CONFIGURED)
	{
		s_streams[index].stage = UCLI_STAGE_QOS_CONFIGURED;
		ucli_streams_enable();
	}
	else if (info->state == BK_BAP_UNICAST_STATE_ENABLING)
	{
		s_streams[index].stage = UCLI_STAGE_ENABLING;
		ucli_streams_create_cis();
	}
	else if (info->state == BK_BAP_UNICAST_STATE_IDLE)
	{
		ucli_streams_reset();
	}
}

static void ucli_handle_cis_handle(const bk_bap_unicast_cis_info_t *info)
{
	int index;

	if (!info)
	{
		return;
	}
	s_last_cis_handle = info->local_cis_handle;
	BK_LOGI(TAG, "cis_handle ase=0x%02x cig=0x%02x cis=0x%02x cis_handle=0x%04x\n",
	        info->ase_id, info->cig_id, info->cis_id, info->local_cis_handle);

	index = ucli_stream_index(info->ase_id);
	if (index < 0)
	{
		return;
	}
	s_streams[index].cis_handle = info->local_cis_handle;
	s_streams[index].stage = UCLI_STAGE_CIS_ASSIGNED;
	ucli_streams_qos();
}

static void ucli_handle_iso_path_ready(const bk_bap_unicast_iso_path_t *info)
{
	uint16_t handles[LE_AUDIO_MAX_STREAMS];
	uint8_t ase_ids[LE_AUDIO_MAX_STREAMS];
	int index;
	uint8_t i;
	int ret;

	if (!info)
	{
		return;
	}
	s_last_cis_handle = info->local_cis_handle;
	BK_LOGI(TAG, "iso_path_ready ase=0x%02x cis_handle=0x%04x\n",
	        info->ase_id, info->local_cis_handle);

	index = ucli_stream_index(info->ase_id);
	if (index < 0)
	{
		return;
	}
	s_streams[index].cis_handle = info->local_cis_handle;
	s_streams[index].stage = UCLI_STAGE_ISO_READY;

	/* Send only once every stream can carry data, so channels stay aligned */
	if (!ucli_group_at_stage(UCLI_STAGE_ISO_READY, "tone", ase_ids))
	{
		return;
	}
	for (i = 0; i < s_stream_count; i++)
	{
		handles[i] = s_streams[i].cis_handle;
	}

	ret = ucli_do_tone_start(handles, s_stream_count);
	BK_LOGI(TAG, "auto tone streams=%u ret=%d\n", s_stream_count, ret);
}

static void ucli_evt_thread(void *arg)
{
	(void)arg;

	while (1)
	{
		ucli_evt_t evt = {0};
		int ret = BK_OK;

		if (rtos_pop_from_queue(&s_evt_queue, &evt, BEKEN_WAIT_FOREVER) != BK_OK)
		{
			continue;
		}

		switch (evt.type)
		{
		case UCLI_EVT_CMD_SCAN:
			ret = ucli_do_scan(evt.on);
			break;
		case UCLI_EVT_CMD_LIST:
			ucli_do_list_peers();
			break;
		case UCLI_EVT_CMD_CONNECT_PEER:
			ret = ucli_do_connect_peer(evt.index);
			break;
		case UCLI_EVT_CMD_CONNECT_ADDR:
			ret = ucli_do_connect_addr(evt.addr, evt.addr_type);
			break;
		case UCLI_EVT_CMD_TONE_START:
			ret = ucli_do_tone_start(&evt.handle, 1U);
			break;
		case UCLI_EVT_CMD_TONE_STOP:
			ret = ucli_do_tone_stop();
			break;
		case UCLI_EVT_READY:
			ucli_handle_ready(&evt.u.ready);
			break;
		case UCLI_EVT_STATE:
			ucli_handle_state(&evt.u.state);
			break;
		case UCLI_EVT_ASE_DISCOVERED:
			ucli_handle_ase_discovered(&evt.u.ase);
			break;
		case UCLI_EVT_CIS_HANDLE:
			ucli_handle_cis_handle(&evt.u.cis);
			break;
		case UCLI_EVT_CIS_ESTABLISHED:
			s_last_cis_handle = evt.u.cis.local_cis_handle;
			BK_LOGI(TAG, "cis_established ase=0x%02x cis_handle=0x%04x\n",
			        evt.u.cis.ase_id, evt.u.cis.local_cis_handle);
			break;
		case UCLI_EVT_ISO_PATH_READY:
			ucli_handle_iso_path_ready(&evt.u.iso);
			break;
		case UCLI_EVT_GATT_CONNECTED:
			ucli_handle_gatt_connected(evt.u.gatt.conn_id, evt.u.gatt.link_role);
			break;
		case UCLI_EVT_MTU_UPDATED:
			BK_LOGI(TAG, "mtu=%u status=0x%02x conn=%u\n",
			        evt.u.gatt.mtu, evt.u.gatt.status, evt.u.gatt.conn_id);
			s_mtu_ready = 1U;
			ucli_setup_if_ready();
			break;
		default:
			break;
		}

		ucli_finish_cmd(&evt, ret);
	}
}

static int ucli_fsm_init(void)
{
	int ret;

	if (s_evt_queue)
	{
		return BK_OK;
	}

	ret = rtos_init_queue(&s_evt_queue, "ucli_evt", sizeof(ucli_evt_t), UCLI_EVT_QUEUE_LEN);
	if (ret != BK_OK)
	{
		return ret;
	}

	ret = rtos_init_semaphore(&s_cmd_sem, 1);
	if (ret != BK_OK)
	{
		rtos_deinit_queue(&s_evt_queue);
		s_evt_queue = NULL;
		return ret;
	}

	ret = rtos_create_thread(&s_evt_thread, UCLI_THREAD_PRIO, "ucli_fsm",
	                         (beken_thread_function_t)ucli_evt_thread,
	                         UCLI_THREAD_STACK, NULL);
	if (ret != BK_OK)
	{
		rtos_deinit_semaphore(&s_cmd_sem);
		s_cmd_sem = NULL;
		rtos_deinit_queue(&s_evt_queue);
		s_evt_queue = NULL;
		return ret;
	}

	return BK_OK;
}

int unicast_client_demo_init(void)
{
	static const bk_bap_source_callbacks_t source_cbs =
	{
		.unicast_ase_discovered_cb = ucli_on_ase_discovered,
		.unicast_cis_handle_assigned_cb = ucli_on_cis_handle,
		.unicast_cis_established_cb = ucli_on_cis_established,
		.unicast_iso_path_ready_cb = ucli_on_iso_path_ready,
		.unicast_state_cb = ucli_on_state,
		.unicast_ready_cb = ucli_on_ready,
		.iso_tx_complete_cb = le_audio_audio_tx_iso_complete,
	};
	static const bk_bap_sink_callbacks_t sink_cbs =
	{
		.announcement_cb = ucli_assist_on_announcement,
		.associate_cb = ucli_assist_on_associate,
		.broadcast_event_cb = ucli_assist_on_broadcast_event,
	};
	static const bk_bass_client_callbacks_t bass_cbs =
	{
		.on_setup = ucli_assist_bass_setup,
		.on_rx_state = ucli_assist_bass_rx_state,
		.event_cb = ucli_assist_bass_event,
	};
	int ret;

	(void)s_last_ase;
	(void)s_last_role;
	(void)s_last_cis_handle;
	ucli_set_security();
	ret = bk_ble_gap_register_callback(ucli_gap_cb);
	BK_LOGI(TAG, "bk_ble_gap_register_callback ret=%d\n", ret);
	if (ret != BK_OK)
	{
		return ret;
	}

	ret = bk_ble_gattc_register_callback(ucli_gattc_cb);
	BK_LOGI(TAG, "bk_ble_gattc_register_callback ret=%d\n", ret);
	if (ret != BK_OK)
	{
		return ret;
	}

	/* Hands out the gattc_if reported by BK_GATTC_REG_EVT */
	ret = bk_ble_gattc_app_register(0U);
	BK_LOGI(TAG, "bk_ble_gattc_app_register ret=%d gattc_if=%u\n", ret, s_gattc_if);
	if (ret != BK_OK)
	{
		return ret;
	}

	ret = bk_dm_bap_init();
	BK_LOGI(TAG, "bk_dm_bap_init ret=%d\n", ret);
	if (ret != BK_OK)
	{
		return ret;
	}

	le_audio_audio_set_source(le_audio_source_tone());
	ret = ucli_fsm_init();
	BK_LOGI(TAG, "ucli_fsm_init ret=%d\n", ret);
	if (ret != BK_OK)
	{
		return ret;
	}

	ret = bk_dm_bap_source_register(&source_cbs);
	BK_LOGI(TAG, "bk_dm_bap_source_register ret=%d\n", ret);
	if (ret != BK_OK)
	{
		return ret;
	}

	ret = bk_dm_bap_sink_register(&sink_cbs);
	BK_LOGI(TAG, "bk_dm_bap_sink_register ret=%d\n", ret);
	if (ret != BK_OK)
	{
		return ret;
	}

	ret = bk_dm_bass_init();
	BK_LOGI(TAG, "bk_dm_bass_init ret=%d\n", ret);
	if (ret != BK_OK)
	{
		return ret;
	}

	ret = bk_dm_bass_client_register(&bass_cbs);
	BK_LOGI(TAG, "bk_dm_bass_client_register ret=%d\n", ret);
	if (ret != BK_OK)
	{
		return ret;
	}

	BK_LOGI(TAG, "BASS Broadcast Assistant registered\n");
	return BK_OK;
}

int unicast_client_demo_scan_peers(uint8_t on)
{
	ucli_evt_t evt = {0};

	evt.type = UCLI_EVT_CMD_SCAN;
	evt.on = on ? 1 : 0;
	return ucli_call_event(&evt);
}

void unicast_client_demo_list_peers(void)
{
	ucli_evt_t evt = {0};

	evt.type = UCLI_EVT_CMD_LIST;
	(void)ucli_call_event(&evt);
}

int unicast_client_demo_connect_peer(uint8_t index)
{
	ucli_evt_t evt = {0};

	evt.type = UCLI_EVT_CMD_CONNECT_PEER;
	evt.index = index;
	return ucli_call_event(&evt);
}

int unicast_client_demo_connect_addr(uint8_t *addr, uint8_t addr_type)
{
	ucli_evt_t evt = {0};

	if (!addr)
	{
		return BK_FAIL;
	}
	evt.type = UCLI_EVT_CMD_CONNECT_ADDR;
	os_memcpy(evt.addr, addr, sizeof(evt.addr));
	evt.addr_type = addr_type;
	return ucli_call_event(&evt);
}

int unicast_client_demo_tone_start(uint16_t connection_handle)
{
	ucli_evt_t evt = {0};

	evt.type = UCLI_EVT_CMD_TONE_START;
	evt.handle = connection_handle;
	return ucli_call_event(&evt);
}

int unicast_client_demo_tone_stop(void)
{
	ucli_evt_t evt = {0};

	evt.type = UCLI_EVT_CMD_TONE_STOP;
	return ucli_call_event(&evt);
}

int unicast_client_demo_assistant_scan(uint8_t on)
{
	if (!on)
	{
		if (s_bcast_scan_on)
		{
			bk_dm_bap_broadcast_scan_stop();
			s_bcast_scan_on = 0;
		}
		return 0;
	}

	os_memset(s_sources, 0, sizeof(s_sources));
	s_source_count = 0;
	s_bcast_scan_on = 1;
	return bk_dm_bap_broadcast_scan_start();
}

void unicast_client_demo_assistant_list_sources(void)
{
	if (s_source_count == 0)
	{
		BK_LOGI(TAG, "no broadcast sources; run le_audio assistant_scan on\n");
		return;
	}

	for (uint8_t i = 0; i < s_source_count; i++)
	{
		BK_LOGI(TAG, "source[%u] sid=%u addr=%02x:%02x:%02x:%02x:%02x:%02x type=%u bcast_id=0x%06lx\n",
		        i, s_sources[i].advertising_sid,
		        s_sources[i].address[5], s_sources[i].address[4], s_sources[i].address[3],
		        s_sources[i].address[2], s_sources[i].address[1], s_sources[i].address[0],
		        s_sources[i].address_type, (unsigned long)s_sources[i].broadcast_id);
	}
}

int unicast_client_demo_assistant_connect(uint8_t *deleg_addr, uint8_t addr_type)
{
	if (!deleg_addr)
	{
		return -1;
	}
	if (s_bcast_scan_on)
	{
		bk_dm_bap_broadcast_scan_stop();
		s_bcast_scan_on = 0;
	}
	os_memcpy(s_deleg_addr, deleg_addr, sizeof(s_deleg_addr));
	s_deleg_type = addr_type;
	s_have_deleg = 1;
	s_past_done = 0;
	s_code_done = 0;
	BK_LOGI(TAG, "connect delegator %02x:%02x:%02x:%02x:%02x:%02x type=%u\n",
	        deleg_addr[5], deleg_addr[4], deleg_addr[3],
	        deleg_addr[2], deleg_addr[1], deleg_addr[0], addr_type);
	return bk_dm_bap_unicast_connect(deleg_addr, addr_type, 1U);
}

int unicast_client_demo_assistant_discover(uint8_t *deleg_addr, uint8_t addr_type)
{
	if (!deleg_addr)
	{
		return -1;
	}
	return bk_dm_bass_discover(deleg_addr, addr_type);
}

int unicast_client_demo_assistant_add(uint8_t source_index, uint32_t bis_sync)
{
	int ret;

	if (source_index >= s_source_count)
	{
		BK_LOGW(TAG, "bad source index=%u count=%u\n", source_index, s_source_count);
		return -1;
	}

	if (s_src_sync_handle == 0xFFFF)
	{
		ret = bk_dm_bap_broadcast_associate(&s_sources[source_index]);
		BK_LOGI(TAG, "PA-sync source[%u] ret=%d\n", source_index, ret);
		if (ret != 0)
		{
			return ret;
		}
	}

	return ucli_assist_send_add_source(source_index,
	                                   bis_sync != 0U ? bis_sync : 0x00000001U);
}

int unicast_client_demo_assistant_remove(uint8_t source_id)
{
	return bk_dm_bass_remove_source(0, source_id);
}

void unicast_client_demo_assistant_stop(void)
{
	if (s_bcast_scan_on)
	{
		bk_dm_bap_broadcast_scan_stop();
		s_bcast_scan_on = 0;
	}
	s_source_count = 0;
	s_src_sync_handle = 0xFFFF;
	s_have_deleg = 0;
	s_past_done = 0;
	s_code_done = 0;
}

int unicast_client_demo_set_broadcast_code(const uint8_t *code16)
{
	if (!code16)
	{
		s_bcode_set = 0;
		os_memset(s_bcode, 0, sizeof(s_bcode));
		BK_LOGI(TAG, "broadcast code cleared\n");
		return 0;
	}

	os_memcpy(s_bcode, code16, sizeof(s_bcode));
	s_bcode_set = 1;
	BK_LOGI(TAG, "broadcast code set\n");
	return 0;
}
