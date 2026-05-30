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
#endif

#define SYS_ANA_REG_BASE    (0x44010000)
#define LDO_ANA_REG         (0x69)

#define SYS_M55_BASE_ADDR    (0x48000000)
#define SYS_GPIO_BASE_ADDR    (0x44000400)


static void bk_lodoen_enable(void)
{
    uint32_t reg = REG_READ(SYS_ANA_REG_BASE + LDO_ANA_REG * 4);
    reg |= (0xF << 28) | (0x2 << 23) | (0x7 << 19) | (0x7 << 15);
    reg &= ~(0xF << 11);
    reg |= (0x8 << 11);
    REG_WRITE(SYS_ANA_REG_BASE + LDO_ANA_REG * 4, reg);

    // close multimedia clock
    reg = REG_READ(SYS_M55_BASE_ADDR + 0xA * 4);
    reg &= ~(1 << 2); // usb hs clock
    reg &= ~(1 << 5); // qspi0 clock
    reg &= ~(1 << 6); // qspi1 clock
    reg &= ~(1 << 7); // sdio0 clock
    reg &= ~(1 << 8); // sdio1 clock
    reg &= ~(1 << 9); // isp clock
    reg &= ~(1 << 10); // gpu clock
    reg &= ~(1 << 11); // h264e clock
    reg &= ~(1 << 12); // csi clock
    reg &= ~(1 << 13); // dsi clock
    reg &= ~(1 << 14); // dpu clock
    reg &= ~(1 << 15); // usb fs clock
    reg &= ~(1 << 22); // npu clock

    reg &= ~(0x3F << 26); // not used clock
    REG_WRITE(SYS_M55_BASE_ADDR + 0xA * 4, reg);

#if 0
    reg = REG_READ(SYS_ANA_REG_BASE + 0x39 * 4);
    reg |= 0x6;
    REG_WRITE(SYS_ANA_REG_BASE + 0x39 * 4, reg);

    reg = REG_READ(SYS_M55_BASE_ADDR + 0x23 * 4);
    reg |= 0x1;
    REG_WRITE(SYS_M55_BASE_ADDR + 0x23 * 4, reg);


    for (int i = 0x1D; i < 0x28; i++) {
        reg = REG_READ(SYS_GPIO_BASE_ADDR + i * 4);
        reg |= (127 << 24);
        REG_WRITE(SYS_GPIO_BASE_ADDR + i * 4, reg);
    }
#endif
}


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
    }
#endif

    media_service_init();

    bk_lodoen_enable();

    bk_printf("lodoen enable...\r\n");

    bk_printf("M55 main running...\r\n");

    avdk_monitor_init();
    avdk_monitor_start();
    draw_tiger();

    return 0;
}
