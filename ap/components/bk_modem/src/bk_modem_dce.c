/**
 ****************************************************************************************
 *
 * @file bk_modem_dce.c
 *
 * @brief 4G Modem Related Processes.
 *
 ****************************************************************************************
 */
#include <common/bk_include.h>
#include "bk_modem_dce.h"
#include "bk_modem_at_cmd.h"
#include "bk_modem_main.h"
#include "bk_modem_uart.h"
#include "os/os.h"

bool bk_modem_dce_send_at(void)
{
    /* USB interface: use original logic */
    if (bk_modem_env.comm_if != UART_IF)
    {
        return (BK_OK == bk_modem_at_ready());
    }

    /* UART: try AT at current baud rate, switch 2M<->5.2M on 3 failures, loop until success */
    while (1)
    {
        for (int i = 0; i < 3; i++)
        {
            if (BK_OK == bk_modem_at_ready())
            {
                return true;
            }
        }

        /* AT failed 2 times at current rate, switch to the other rate */
        uint32_t current_baud = bk_modem_uart_get_baud_rate();
        uint32_t new_baud = (current_baud == BK_MODEM_UART_5M2_BAUD) ? BK_MODEM_UART_2M_BAUD : BK_MODEM_UART_5M2_BAUD;
        BK_MODEM_LOGI("AT fail at %d, switch to %d\r\n", (int)current_baud, (int)new_baud);
        if (bk_modem_uart_set_baud_rate(new_baud) != BK_OK)
        {
            BK_MODEM_LOGW("set baud rate to %d fail\r\n", (int)new_baud);
            return false;
        }
        rtos_delay_milliseconds(100);
    }
}

bool bk_modem_dce_check_sim(void)
{
    return (BK_OK == bk_modem_at_cpin());
}

bool bk_modem_dce_check_signal(void)
{
    return (BK_OK == bk_modem_at_csq());
}

bool bk_modem_dce_check_register(void)
{
    return (BK_OK == bk_modem_at_get_operator_name());
}

bool bk_modem_dce_set_apn(void)
{
    return (BK_OK == bk_modem_at_cgdcont(1,"ipv4v6",""));
}

bool bk_modem_dce_check_attach(void)
{
    return (BK_OK == bk_modem_at_get_ps_reg());
}

bool bk_modem_dce_start_ppp(void)
{
    return (BK_OK == bk_modem_at_ppp_connect());
}

bool bk_modem_dce_enter_cmd_mode(void)
{
    return (BK_OK == bk_modem_at_enter_cmd_mode());
}

bool bk_modem_dce_stop_ppp(void)
{
    return (BK_OK == bk_modem_at_disconnect());
}

bool bk_modem_dce_enter_flight_mode(void)
{
    return (BK_OK == bk_modem_at_cfun(0));
}

bool bk_modem_dce_exit_flight_mode(void)
{
    return (BK_OK == bk_modem_at_cfun(1));
}

/// ec own at cmd
bool bk_modem_dce_ec_check_nat(void)
{
    return (BK_OK == bk_modem_ec_at_check_nat());
}

bool bk_modem_dce_ec_close_rndis(void)
{
    return (BK_OK == bk_modem_ec_at_close_rndis());
}

bool bk_modem_dce_ec_open_datapath(void)
{
    return (BK_OK == bk_modem_ec_at_open_datapath());
}

bool bk_modem_dce_ec_set_nat(void)
{
    return (BK_OK == bk_modem_ec_at_set_nat());
}

bool bk_modem_dce_ec_rst(void)
{
    return (BK_OK == bk_modem_ec_at_rst());
}