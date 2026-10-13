#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <modules/pm.h>
#include <driver/pwr_clk.h>
#include <components/ate.h>
#include <components/log.h>
#if CONFIG_BLUETOOTH_A2DP_OFFLOAD_SINK
#include "a2dp_sink_offload.h"
#include "a2dp_sink_offload_player.h"
#endif
#if CONFIG_BLUETOOTH_HFP_SCO_OFFLOAD
#include "hfp_sco_offload.h"
#include "hfp_sco_offload_player.h"
#endif

extern void rtos_set_user_app_entry(beken_thread_function_t entry);

void user_app_main(void) {
    if (!ate_is_enabled())
    {
        //bk_start_ap_system();
        bk_pm_module_vote_boot_ap_ctrl(PM_BOOT_AP_MODULE_NAME_APP, PM_POWER_MODULE_STATE_ON);
    }
#if CONFIG_ADK
	extern int bk_audio_osi_funcs_init(void);
	bk_audio_osi_funcs_init();
#endif

#if CONFIG_BLUETOOTH_A2DP_OFFLOAD_SINK
    /* A2DP Sink Offload: shim first (it clears the callback slots), then the
     * playback driver that registers into them. Failure does not block boot. */
    if (a2dp_sink_offload_init() != BK_OK)
    {
        BK_LOGW(NULL, "a2dp sink offload init failed\r\n");
    }
    if (a2dp_sink_offload_player_init() != BK_OK)
    {
        BK_LOGW(NULL, "a2dp sink offload player init failed\r\n");
    }
#endif

#if CONFIG_BLUETOOTH_HFP_SCO_OFFLOAD
    if (hfp_sco_offload_init() != BK_OK)
    {
        BK_LOGW(NULL, "hfp sco offload init failed\r\n");
    }
    if (hfp_sco_offload_player_init() != BK_OK)
    {
        BK_LOGW(NULL, "hfp sco offload player init failed\r\n");
    }
#endif
}



int main(void)
{
    rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
    bk_init();

    return 0;
}