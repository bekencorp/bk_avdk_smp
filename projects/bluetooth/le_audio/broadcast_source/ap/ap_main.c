#include <stdio.h>

#include "bk_private/bk_init.h"
#include "components/system.h"
#include "components/log.h"
#include <os/os.h>

#include "broadcast_source_demo.h"

#define TAG "bsrc_main"

int main(void)
{
	int ret;

	bk_init();
	rtos_delay_milliseconds(500);

	ret = broadcast_source_demo_init();
	if (ret != BK_OK)
	{
		BK_LOGE(TAG, "broadcast_source_demo_init failed ret=%d\n", ret);
		return ret;
	}

	ret = broadcast_source_cli_init();
	if (ret != BK_OK)
	{
		BK_LOGE(TAG, "broadcast_source_cli_init failed ret=%d\n", ret);
		return ret;
	}

	return 0;
}
