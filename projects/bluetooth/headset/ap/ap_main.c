#include <stdio.h>
#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include "components/bluetooth/bk_dm_bluetooth.h"

#include "bt_manager.h"
#include "headset_user_config.h"
#include "media_service.h"
#include "a2dp_sink/a2dp_sink_demo.h"
#include "dm_gatt.h"
#include "dm_gatts.h"
#include "dm_gattc.h"

#define AUTO_ENABLE_BLUETOOTH_DEMO 1

int main(void)
{
	bk_init();

	media_service_init();

#if AUTO_ENABLE_BLUETOOTH_DEMO
	uint8_t bt_mac[6] = {0};
	char local_name[30] = {0};
	bk_err_t err = bk_bluetooth_get_address(bt_mac);
	if (err == BK_OK)
	{
		snprintf(local_name,
				 sizeof(local_name),
				 "%s_%02x%02x%02x",
				 LOCAL_NAME,
				 bt_mac[2],
				 bt_mac[1],
				 bt_mac[0]);
	}
	else
	{
		snprintf(local_name, sizeof(local_name), "%s", LOCAL_NAME);
	}

	bt_manager_cfg_t bt_manager_cfg =
	{
		.local_name = local_name,
		.device_class = COD_SOUNDBAR,
		.page_scan_interval = PAGE_SCAN_INTV,
		.page_scan_window = PAGE_SCAN_WIN,
		.page_timeout = CONFIG_PAGE_TIMEOUT,
		.reconnect_interval_ms = CONFIG_RECONN_INTERVAL,
		.max_reconnect_count = CONFIG_MAX_RECONN_COUNT,
		.io_capability = BK_BT_IO_CAP_NONE,
	};
	bt_manager_init(&bt_manager_cfg);

#if CONFIG_A2DP_SINK_DEMO
	a2dp_sink_demo_init(0, 1);
#endif

#if CONFIG_HFP_HF_DEMO
	extern int hfp_hf_demo_init(uint8_t msbc_supported);
	hfp_hf_demo_init(0);
#endif

#if CONFIG_BT
	extern int cli_headset_demo_init(void);
	cli_headset_demo_init();
#endif

#if CONFIG_AUDIO_PLAY
	extern int audio_play_cli_init(void);
	audio_play_cli_init();
#endif

#if CONFIG_BLE
    //cli_gatt_param_t param = {.rpa = 0, .p_rpa = &param.rpa, .pa = 0, .p_pa = &param.pa};

    //dm_gatt_main(&param);
    //dm_gatts_main(&param);

    extern int cli_ble_gatt_demo_init(void);
    cli_ble_gatt_demo_init();
#endif

#endif
	return 0;
}
