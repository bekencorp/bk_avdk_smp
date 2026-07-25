#include <stdio.h>

#include "bk_private/bk_init.h"
#include "components/system.h"
#include "components/log.h"
#include "media_service.h"
#include <os/os.h>

#include "broadcast_sink_demo.h"

#define TAG "bsink_main"

int main(void)
{
	int ret;

	bk_init();
	rtos_delay_milliseconds(500);
	media_service_init();

	ret = broadcast_sink_demo_init();
	if (ret != BK_OK)
	{
		BK_LOGE(TAG, "broadcast_sink_demo_init failed ret=%d\n", ret);
		return ret;
	}

	ret = broadcast_sink_cli_init();
	if (ret != BK_OK)
	{
		BK_LOGE(TAG, "broadcast_sink_cli_init failed ret=%d\n", ret);
		return ret;
	}

	return 0;
}
