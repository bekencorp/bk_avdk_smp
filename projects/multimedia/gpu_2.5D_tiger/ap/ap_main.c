#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <stdint.h>

#include "avdk_monitor.h"
#include "media_service.h"
#include "draw_tiger.h"

#if CONFIG_GCOV
#include <driver/uart.h>
#include <driver/hal/hal_uart_types.h>
#include "gcov_public.h"
#include "cli.h"
#endif


#if CONFIG_GCOV
static void cli_gcov_dump_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    __gcov_exit();
    bk_printf("[RESULT][PASS] gcov_dump success\r\n");
}

static const struct cli_command s_gcov_test_cmds[] = {
    { "gcov_dump_test", "dump gcov data then print test result", cli_gcov_dump_test_cmd },
};
#endif

int main(void)
{
    bk_init();

#if CONFIG_GCOV
    {
        uart_config_t gcov_uart_cfg = {0};
        gcov_uart_cfg.baud_rate = 115200;
        gcov_uart_cfg.data_bits = UART_DATA_8_BITS;
        gcov_uart_cfg.parity = UART_PARITY_NONE;
        gcov_uart_cfg.stop_bits = UART_STOP_BITS_1;
        gcov_uart_cfg.flow_ctrl = UART_FLOWCTRL_DISABLE;
        gcov_uart_cfg.src_clk = UART_SCLK_APLL;
        gcov_init_uart(UART_ID_5, &gcov_uart_cfg);
        cli_register_commands(s_gcov_test_cmds, sizeof(s_gcov_test_cmds) / sizeof(s_gcov_test_cmds[0]));
    }
#endif

    media_service_init();

    bk_printf("lodoen enable...\r\n");

    avdk_monitor_init();
    avdk_monitor_start();
    draw_tiger();

    return 0;
}
