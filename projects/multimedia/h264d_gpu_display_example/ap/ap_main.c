#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/bk_frame_buffer.h>

#include "cli.h"
#include "media_service.h"
#include "avdk_monitor.h"
#include "h264d_gpu_display_boot.h"
#include "h264d_gpu_display_demo.h"

int main(void)
{
	bk_init();
	media_service_init();

#ifdef CONFIG_FRAME_BUFFER
	bk_frame_buffer_init();
#endif

	avdk_monitor_init();
	avdk_monitor_start();

	cli_h264d_gpu_display_init();
#ifdef CONFIG_BK_DECODER
	h264d_gpu_display_run_boot_case();
#endif
	return 0;
}
