#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <common/bk_include.h>
#include <media_service.h>
#include <driver/pwr_clk.h>
#include "cli.h"
#include "dma2d_test.h"


int main(void)
{
    bk_init();
    media_service_init();
    bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_VIDP_DMA2D, PM_POWER_MODULE_STATE_ON);
    cli_dma2d_init();
    return 0;
}
