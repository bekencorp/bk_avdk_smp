#include "bk_private/bk_init.h"
#include <os/os.h>
#include "sys_driver.h"

int main(void)
{
    GPIO_UP(48);
    GPIO_DOWN(48);

    sys_drv_flash_cksel(0);
    sys_drv_flash_set_clk_div(0);

    bk_init();

    BK_LOGI(NULL, "AP main running...\r\n");

    return 0;
}
