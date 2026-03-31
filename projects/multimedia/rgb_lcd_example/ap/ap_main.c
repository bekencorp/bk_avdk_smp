#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <driver/psram.h>
#include <avdk_check.h>
#include <avdk_error.h>
#include <components/bk_frame_buffer.h>
#include "sys_driver.h"
#include "media_service.h"
#include <common/avdk_pixel_types.h>
#include "lcd_example.h"

#define DEFAULT_RGB_PANEL_NAME   "st7701sn_rgb_480x854"
#define DEFAULT_RGB_FORMAT      BK_PIXEL_FORMAT_RGB565

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
    media_service_init();

    #if (BK_IPC_UT_TEST)
    bk_ipc_test_init();
    #endif
    bk_printf("%s, %d, m55 running...\r\n", __func__, __LINE__);


    bk_printf("lodoen enable start...\r\n");

    bk_lodoen_enable();

    bk_printf("lodoen enable...\r\n");
    #ifdef CONFIG_FRAME_BUFFER
    bk_frame_buffer_init();
    #endif

    /* Power-on display: open RGB panel with panel name and format, then start flush thread. */
    static display_ctx_t s_rgb_disp_ctx;
    os_memset(&s_rgb_disp_ctx, 0, sizeof(s_rgb_disp_ctx));
    if (lcd_example_rgb_open(&s_rgb_disp_ctx, DEFAULT_RGB_PANEL_NAME, DEFAULT_RGB_FORMAT) == AVDK_ERR_OK)
        lcd_example_flush_thread_start(&s_rgb_disp_ctx);

    return 0;
}
