#include <os/os.h>
#include <os/mem.h>
#include "bk_private/bk_init.h"
#include <components/system.h>
#include <driver/pwr_clk.h>
#include "draw_osd_test.h"
#include <media_service.h>

int main(void)
{
    bk_init();
    media_service_init();
    bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_VIDP_LCD,PM_POWER_MODULE_STATE_ON);
    cli_draw_osd_test_init();
    return 0;
}

