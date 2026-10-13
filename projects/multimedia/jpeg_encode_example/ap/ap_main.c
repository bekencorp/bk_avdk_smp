
#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include "cli.h"
#include <components/bk_frame_buffer.h>
#include "jpeg_encode_test.h"
#include "media_service.h"

#define TAG "ap_main"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

int main(void)
{
	bk_init();
	media_service_init();

#if (BK_IPC_UT_TEST)
	bk_ipc_test_init();
#endif

	bk_printf("%s, %d, m55 running...\r\n", __func__, __LINE__);

#ifdef CONFIG_FRAME_BUFFER
	bk_frame_buffer_init();
#endif

	cli_jpeg_encode_init();
#ifdef CONFIG_BK_ENCODER
	vcenc_jpeg_run_boot_demo();
#endif
	return 0;
}
