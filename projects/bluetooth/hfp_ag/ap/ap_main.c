#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>

#include "bt_manager.h"
#include "media_service.h"

#define AUTO_ENABLE_BLUETOOTH_DEMO 1

int main(void)
{
	bk_init();

	media_service_init();

#if AUTO_ENABLE_BLUETOOTH_DEMO
	bt_manager_init();

#if CONFIG_HFP_AG_DEMO
	extern int hfp_ag_demo_init(uint8_t msbc_supported);
	hfp_ag_demo_init(0);

	extern int cli_hfp_ag_demo_init(void);
	cli_hfp_ag_demo_init();
#endif

#endif
	return 0;
}
