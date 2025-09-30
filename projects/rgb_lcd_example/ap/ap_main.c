#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <media_service.h>
#include <driver/pwr_clk.h>
#include "cli.h"
#include "components/bk_display.h"
#include "gpio_driver.h"
#include "frame_buffer.h"
#include "lcd_panel_devices.h"
#include <components/avdk_utils/avdk_error.h>
#include "rgb_lcd_test.h"


int main(void)
{
    bk_init();
    media_service_init();
    bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_VIDP_LCD, PM_POWER_MODULE_STATE_ON);
    cli_lcd_display_init();
    return 0;
}
