#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>

/*
 * WiFi bridge demo — control via CLI on AP core:
 *   bridge open <sta_ssid> [key] [bridge_ssid]
 *   bridge close
 *   state
 * Bring-up continues in wdrv_cntrl after STA GOT_IP.
 */

extern int bk_ipc_init(void);

int main(void)
{
	bk_init();
	return 0;
}