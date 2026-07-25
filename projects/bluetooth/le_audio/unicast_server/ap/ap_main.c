#include <stdio.h>

#include "bk_private/bk_init.h"
#include "components/system.h"
#include "components/log.h"
#include "media_service.h"
#include <os/os.h>

#include "unicast_server_demo.h"

#define TAG "usrv_main"

int main(void)
{
	int ret;

	bk_init();
	rtos_delay_milliseconds(500);
	media_service_init();

	ret = unicast_server_demo_init();
	if (ret != BK_OK)
	{
		BK_LOGE(TAG, "unicast_server_demo_init failed ret=%d\n", ret);
		return ret;
	}

	ret = unicast_server_cli_init();
	if (ret != BK_OK)
	{
		BK_LOGE(TAG, "unicast_server_cli_init failed ret=%d\n", ret);
		return ret;
	}

	return 0;
}
