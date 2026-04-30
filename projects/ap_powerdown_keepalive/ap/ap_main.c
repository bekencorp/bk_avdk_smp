#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include "lp_ipc_msg/lp_ipc_msg.h"
#include "keepalive/keepalive.h"

int main(void)
{
	bk_init();

    lp_ipc_cli_init();
    lp_ipc_wakeup_env_init();
    keepalive_handle_wakeup_reason();

	return 0;
}
