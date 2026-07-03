#include <components/system.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "components/bluetooth/bk_dm_bluetooth_types.h"
#include "components/bluetooth/bk_dm_bluetooth.h"

#include "bta_manager.h"
#include "bluetooth_config.h"
#include "bta_event.h"
#include "bta_auracast.h"
#include "bta_audio.h"

#define TAG "bta_mana"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define SCAN_RESULT_MAX_COUNT       (8)
#define SCAN_RESULT_TIMEOUT         (5000)

static uint8_t s_bta_power_state = BTM_POWER_STATUS_OFF;

static bk_bap_source_announce_data_t *scan_result;
static beken2_timer_t scan_timer;
uint8_t remote_address[6] = {0};
uint8_t auracast_sink_white_list[6] = {0};

void bta_manager_init(void)
{
	s_bta_power_state = BTM_POWER_STATUS_ON;
}

static void bta_enter_pairing_mode_handle(void)
{
	if (AURACAST_STATE_TURNING_OFF == bta_auracast_get_state()
	    && AURACAST_STATE_TURNING_ON == bta_auracast_get_state()
	    && AURACAST_SCANNING == bta_auracast_get_scan_state())
	{
		LOGI("%s auracast busy state: %d %d\n",
		     __func__,
		     bta_auracast_get_state(),
		     bta_auracast_get_scan_state());
		return;
	}

	if (AURACAST_STATE_TURN_ON == bta_auracast_get_state())
	{
		if (AURACAST_ROLE_SOURCE == bta_auracast_get_role())
		{
			LOGI("%s server stop\n", __func__);
			bta_auracast_server_stop();
		}
		else if (AURACAST_ROLE_SINK == bta_auracast_get_role())
		{
			LOGI("%s client stop\n", __func__);
			os_memset(remote_address, 0, 6);
			bta_auracast_client_stop();
		}
	}

}

static void bta_manager_auracast_scan_timeout(void *param, unsigned int ulparam)
{
	(void)param;
	(void)ulparam;
	bta_event_send(BTA_EVT_MAN_SINK_BROADCAST_SCAN_COMPLETE_IND, 0, 0);
}

static void bta_manager_sink_scan_complete_handle(void)
{
	bk_bap_source_announce_data_t *result = NULL;
	int i;

	LOGI("%s\n", __func__);

	for (i = 0; i < SCAN_RESULT_MAX_COUNT; i++)
	{
		if (!bta_auracast_is_zero_address(scan_result[i].address))
		{
			if (result == NULL)
			{
				result = &scan_result[i];
				continue;
			}

			if (result->rssi < scan_result[i].rssi)
			{
				result = &scan_result[i];
			}
		}
	}

	if (result == NULL)
	{
		LOGE("%s not found device, continue scan\n", __func__);
		rtos_start_oneshot_timer(&scan_timer);
		return;
	}

	if (rtos_is_oneshot_timer_init(&scan_timer))
	{
		if (rtos_is_oneshot_timer_running(&scan_timer))
		{
			rtos_stop_oneshot_timer(&scan_timer);
		}

		rtos_deinit_oneshot_timer(&scan_timer);
	}

	LOGI("play source: %02X:%02X:%02X:%02X:%02X:%02X Type: 0x%02X, %d\n",
	     result->address[5],
	     result->address[4],
	     result->address[3],
	     result->address[2],
	     result->address[1],
	     result->address[0],
	     result->address_type,
	     result->rssi);

	os_memcpy(remote_address, result->address, 6);
	bta_auracast_boradcast_associate(result);
}

static void bta_manager_auracast_scan_result_callback(bk_bap_source_announce_data_t *bk_bap_source_announce_data)
{
	uint8_t replace = true;
	int i;

	if (!bta_auracast_is_zero_address(auracast_sink_white_list))
	{
		if (!os_memcmp(auracast_sink_white_list, bk_bap_source_announce_data->address, sizeof(auracast_sink_white_list)))
		{
			os_memcpy(remote_address, auracast_sink_white_list, sizeof(auracast_sink_white_list));
			bta_auracast_boradcast_associate(bk_bap_source_announce_data);
		}
		return;
	}

	if (!bta_auracast_is_zero_address(remote_address)
	    && !os_memcmp(remote_address, bk_bap_source_announce_data->address, sizeof(remote_address)))
	{
		bta_auracast_boradcast_associate(bk_bap_source_announce_data);
		return;
	}

	if (scan_result == NULL)
	{
		LOGE("scan_result is NULL\n");
		return;
	}

	for (i = 0; i < SCAN_RESULT_MAX_COUNT; i++)
	{
		if (!os_memcmp(scan_result[i].address, bk_bap_source_announce_data->address, 6))
		{
			if (bk_bap_source_announce_data->rssi < scan_result[i].rssi)
			{
				os_memcpy(&scan_result[i], bk_bap_source_announce_data, sizeof(bk_bap_source_announce_data_t));
				replace = false;
				break;
			}
		}

		if (bta_auracast_is_zero_address(scan_result[i].address))
		{
			os_memcpy(&scan_result[i], bk_bap_source_announce_data, sizeof(bk_bap_source_announce_data_t));
			replace = false;
			break;
		}
	}

	if (replace)
	{
		for (i = 0; i < SCAN_RESULT_MAX_COUNT; i++)
		{
			if (scan_result[i].rssi < bk_bap_source_announce_data->rssi)
			{
				os_memcpy(&scan_result[i], bk_bap_source_announce_data, sizeof(bk_bap_source_announce_data_t));
				break;
			}
		}
	}

	if (!rtos_is_oneshot_timer_init(&scan_timer))
	{
		rtos_init_oneshot_timer(&scan_timer,
		                        SCAN_RESULT_TIMEOUT,
		                        (timer_2handler_t)bta_manager_auracast_scan_timeout,
		                        NULL,
		                        0);
		rtos_start_oneshot_timer(&scan_timer);
	}
}

void bta_manager_auracast_scan_stop_callback(void *param)
{
	(void)param;

	if (scan_result != NULL)
	{
		os_free(scan_result);
		scan_result = NULL;
	}

	if (rtos_is_oneshot_timer_init(&scan_timer))
	{
		if (rtos_is_oneshot_timer_running(&scan_timer))
		{
			rtos_stop_oneshot_timer(&scan_timer);
		}

		rtos_deinit_oneshot_timer(&scan_timer);
	}
}

static void bta_manager_auracast_client_start(void)
{
	bta_auracast_scan_cfg_t cfg;

	LOGI("%s client start\n", __func__);

	if (scan_result != NULL)
	{
		LOGE("%s memory leak?\n", __func__);
	}
	else
	{
		scan_result = os_malloc(sizeof(bk_bap_source_announce_data_t) * SCAN_RESULT_MAX_COUNT);
		if (scan_result == NULL)
		{
			LOGE("%s maloc scan result buffer failed\n", __func__);
			return;
		}
	}

	os_memset(scan_result, 0, sizeof(bk_bap_source_announce_data_t) * SCAN_RESULT_MAX_COUNT);

	cfg.result_cb = bta_manager_auracast_scan_result_callback;
	cfg.stop_cb = bta_manager_auracast_scan_stop_callback;
	bta_auracast_boradcast_scan_start(&cfg);
}

static void bta_manager_broadcast_handle(void)
{
	LOGI("%s status %d scan %d role %d\n", __func__,
	     bta_auracast_get_state(), bta_auracast_get_scan_state(),
	     bta_auracast_get_role());

	if (AURACAST_STATE_TURNING_OFF == bta_auracast_get_state()
	    && AURACAST_STATE_TURNING_ON == bta_auracast_get_state()
	    && AURACAST_SCANNING == bta_auracast_get_scan_state())
	{
		return;
	}

	if (AURACAST_STATE_TURN_OFF == bta_auracast_get_state())
	{
		LOGI("%s auracast client start\n", __func__);
		bta_manager_auracast_client_start();
	}
	else if (AURACAST_STATE_TURN_ON == bta_auracast_get_state()
	         || AURACAST_STATE_TURNING_ON == bta_auracast_get_state())
	{
		if (AURACAST_ROLE_SOURCE == bta_auracast_get_role())
		{
			bta_auracast_server_stop();
		}
		else if (AURACAST_ROLE_SINK == bta_auracast_get_role())
		{
			os_memset(remote_address, 0, 6);
			bta_auracast_client_stop();
		}
	}
}

static void bta_manager_play_handle(void)
{
	if (!bta_auracast_is_zero_address(remote_address))
	{
		if (AURACAST_STATE_TURN_ON == bta_auracast_get_state())
		{
			bta_auracast_client_stop();
		}
		else if (AURACAST_STATE_TURN_OFF == bta_auracast_get_state())
		{
			bta_manager_auracast_client_start();
		}
	}
}

static void bta_manager_turn_on_handle(void)
{
	s_bta_power_state = BTM_POWER_STATUS_ON;
}

static void bta_manager_turn_off_handle(void)
{
	if (AURACAST_STATE_TURN_ON == bta_auracast_get_state())
	{
		if (AURACAST_ROLE_SOURCE == bta_auracast_get_role())
		{
			bta_auracast_server_stop();
		}
		else if (AURACAST_ROLE_SINK == bta_auracast_get_role())
		{
			os_memset(remote_address, 0, 6);
			bta_auracast_client_stop();
		}
	}

	s_bta_power_state = BTM_POWER_STATUS_OFF;

	os_memset(auracast_sink_white_list, 0, 6);
}

static void bta_manager_volume_up_handle(void)
{
	bta_audio_volume_up();
}

static void bta_manager_volume_down_handle(void)
{
	bta_audio_volume_down();
}

void bta_manager_event_dispather(uint32_t event, uint32_t param, uint32_t extra)
{
	(void)param;
	(void)extra;

	switch (event)
	{
	case BTA_EVT_MAN_PAIRING:
		bta_enter_pairing_mode_handle();
		break;

	case BTA_EVT_MAN_BROADCAST:
		bta_manager_broadcast_handle();
		break;

	case BTA_EVT_MAN_PLAY:
		bta_manager_play_handle();
		break;

	case BTA_EVT_MAN_TURN_ON:
		bta_manager_turn_on_handle();
		break;

	case BTA_EVT_MAN_TURN_OFF:
		bta_manager_turn_off_handle();
		break;

	case BTA_EVT_MAN_VOLUME_UP:
		bta_manager_volume_up_handle();
		break;

	case BTA_EVT_MAN_VOLUME_DOWN:
		bta_manager_volume_down_handle();
		break;

	case BTA_EVT_MAN_SINK_BROADCAST_SCAN_COMPLETE_IND:
		bta_manager_sink_scan_complete_handle();
		break;

	default:
		break;
	}
}

void bta_manager_pairing_mode(void)
{
	if (s_bta_power_state == BTM_POWER_STATUS_OFF)
	{
		LOGI("%s bt not open\n", __func__);
		return;
	}

	bta_event_send(BTA_EVT_MAN_PAIRING, 0, 0);
}

void bta_manager_broadcast(void)
{
	if (s_bta_power_state == BTM_POWER_STATUS_OFF)
	{
		return;
	}

	bta_event_send(BTA_EVT_MAN_BROADCAST, 0, 0);
}

void bta_manager_play(void)
{
	if (s_bta_power_state == BTM_POWER_STATUS_OFF)
	{
		return;
	}

	bta_event_send(BTA_EVT_MAN_PLAY, 0, 0);
}

void bta_manager_on(void)
{
	if (s_bta_power_state == BTM_POWER_STATUS_ON)
	{
		return;
	}

	bta_event_send(BTA_EVT_MAN_TURN_ON, 0, 0);
}

void bta_manager_off(void)
{
	if (s_bta_power_state != BTM_POWER_STATUS_ON)
	{
		return;
	}

	bta_event_send(BTA_EVT_MAN_TURN_OFF, 0, 0);
}

void bta_manager_volume_up(void)
{
	if (s_bta_power_state == BTM_POWER_STATUS_OFF)
	{
		return;
	}

	bta_event_send(BTA_EVT_MAN_VOLUME_UP, 0, 0);
}

void bta_manager_volume_down(void)
{
	if (s_bta_power_state == BTM_POWER_STATUS_OFF)
	{
		return;
	}

	bta_event_send(BTA_EVT_MAN_VOLUME_DOWN, 0, 0);
}

uint8_t bta_manager_is_turn_on(void)
{
	return s_bta_power_state == BTM_POWER_STATUS_ON;
}

void bta_manager_get_address(uint8_t *address)
{
	bk_bluetooth_get_address(address);
}

void bta_manager_set_name(char *name)
{
	LOGI("%s ignored after removing classic BT GAP name setup: %s\n", __func__, name ? name : "");
}

void bta_manager_set_white_list(uint8_t *address)
{
	os_memcpy(auracast_sink_white_list, address, sizeof(auracast_sink_white_list));
}

