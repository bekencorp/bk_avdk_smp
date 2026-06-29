#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/bk_frame_buffer.h>
#include "media_service.h"
#include "draw_osd_test.h"

#define TAG "draw_osd"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)

int main(void)
{
    bk_init();
    media_service_init();

#ifdef CONFIG_FRAME_BUFFER
    bk_frame_buffer_init();
#endif

    LOGI("draw_osd_example m55 running...\r\n");

    cli_draw_osd_test_init();

    return 0;
}
