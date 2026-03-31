#include <os/os.h>
#include "bk_private/bk_init.h"
#include <components/system.h>

#include <components/shell_task.h>
#include <components/bk_frame_buffer.h>
#include "cli.h"
#include "uvc_cli.h"

int main(void)
{
    bk_init();
    bk_frame_buffer_init();

    cli_uvc_test_init();

    return 0;
}
