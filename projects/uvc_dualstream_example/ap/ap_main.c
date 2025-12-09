#include <os/os.h>
#include <os/mem.h>
#include "bk_private/bk_init.h"
#include <components/system.h>
#include <components/shell_task.h>
#include <media_service.h>
#include "cli.h"
#include "uvc_cli.h"

int main(void)
{
    bk_init();
    media_service_init();
#ifdef CONFIG_USB_CAMERA
    cli_uvc_test_init();
#endif
    return 0;
}