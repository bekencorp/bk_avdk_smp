#include <common/bk_include.h>
#include <os/mem.h>
#include <driver/uart.h>
#include <modules/bk_riscv.h>
#include "sys_driver.h"

#include "bk_uart.h"
#include "riscv_usb_bridge.h"
#include "sys_sw_regs.h"

#define USB_HS_BASE            SOC_USB_HS_BASE
#define REG_USB_USR_105        (*(volatile uint32_t *)(USB_HS_BASE + 0x714U))
#define REG_USB_USR_106        (*(volatile uint32_t *)(USB_HS_BASE + 0x718U))
#define RISCV_TCM_ADDR         SOC_USB_TCM_BASE
#define RISCV_RESET_VEC_TCM    0x80U
#define RISCV_BOOT_PARAM_MAGIC 0x52565041UL
#define RISCV_BOOT_PARAM_VERSION 0x00000001UL
#define RISCV_BOOT_PARAM_OFFSET 0x0000FFC0U
#define RISCV_SYS_SW_REGS_BASE_ADDR CONFIG_SWAP_ADDR
#define RISCV_USB_LOG_UART_ID  UART_ID_5
#define RISCV_USB_LOG_BAUDRATE 115200

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t size;
    uint32_t sys_sw_regs_base;
    uint32_t flags;
    uint32_t reserved[3];
} riscv_boot_param_t;

static uint32_t s_host_started = 0U;

static volatile riscv_usb_probe_t s_riscv_probe = {0};

volatile riscv_usb_probe_t *get_riscv_usb_probe(void)
{
    return &s_riscv_probe;
}

void riscv_usb_probe_init(void)
{
    bk_sys_sw_regs_ptr()->riscv_swap = (void *)&s_riscv_probe;
}

static void usb_hc_riscv_power_on(void)
{
    REG_USB_USR_105 |= (1U << 14);
}

static void usb_hc_route_irq_to_riscv(void)
{
    uint32_t ints_config = sys_drv_get_ints_config_riscv_0_31();
    ints_config |= (1U << 8);
    sys_drv_set_ints_config_riscv_0_31(ints_config);
}

void usb_hc_riscv_start_core(uint32_t reset_vec)
{
    REG_USB_USR_106 = reset_vec;
    REG_USB_USR_105 |= (1U << 15);
}

int usb_hc_riscv_start_firmware(const unsigned char *fw, unsigned int fw_len, uint32_t reset_vec)
{
    volatile riscv_boot_param_t *boot_param = (volatile riscv_boot_param_t *)(RISCV_TCM_ADDR + RISCV_BOOT_PARAM_OFFSET);

    riscv_usb_probe_init();

    usb_hc_riscv_power_on();
    usb_hc_route_irq_to_riscv();
    os_memcpy((void *)RISCV_TCM_ADDR, fw, fw_len);
    boot_param->magic = RISCV_BOOT_PARAM_MAGIC;
    boot_param->version = RISCV_BOOT_PARAM_VERSION;
    boot_param->size = sizeof(riscv_boot_param_t);
    boot_param->sys_sw_regs_base = RISCV_SYS_SW_REGS_BASE_ADDR;
    boot_param->flags = 0U;
    boot_param->reserved[0] = 0U;
    boot_param->reserved[1] = 0U;
    boot_param->reserved[2] = 0U;
    __sync_synchronize();
    usb_hc_riscv_start_core(reset_vec);
    return 0;
}

void usb_hc_riscv_stop_firmware(void)
{
    REG_USB_USR_105 &= ~(1U << 15);
    REG_USB_USR_105 &= ~(1U << 14);
#if CONFIG_USB_RISCV_LOG_UART && (CONFIG_UART_PRINT_PORT != RISCV_USB_LOG_UART_ID)
    bk_uart_deinit(RISCV_USB_LOG_UART_ID);
#endif
    s_host_started = 0U;
}

/* MILESTONE A: device-side bridge entry stub.
 *
 * Intentionally returns -1 so usb_dc_low_level_init() in
 * usb_dc_beken_musb_mhdrc.c keeps registering USBD_IRQHandler on the M55
 * and the device path runs unchanged. The full device path (load a
 * dual-role RISC-V firmware, route INT_SRC_USB_HS to RISC-V, publish the
 * shared-memory device region with riscv_usbd_probe_t fields, ack-back
 * via IPI) is staged in the follow-up milestone -- see
 * docs/USB重构/07-M55_RISCV_USB桥设计.md §5.
 *
 * Implementation note for the follow-up: most of the heavy lifting is
 * already provided by usb_hc_riscv_start_firmware() above (power-on,
 * IRQ route to RISC-V, fw memcpy, boot_param hand-off, core release).
 * The device variant only needs (a) a dual-role firmware blob from
 * bk_riscv_usb_dual_fw_addr/_len(), (b) to set
 * sys_sw_regs_ptr()->riscv_usb_probe.role = RISCV_USB_ROLE_DEVICE before
 * starting the core, and (c) a refcount-aware start/stop policy that
 * matches bk_usb_open/bk_usb_close.
 */
int usb_dc_riscv_device_prepare(void)
{
    return -1;
}

int usb_hc_riscv_host_prepare(void)
{
    const unsigned char *fw = bk_riscv_usb_host_fw_addr();
    const unsigned int fw_len = bk_riscv_usb_host_fw_len();
#if CONFIG_USB_RISCV_LOG_UART && (CONFIG_UART_PRINT_PORT != RISCV_USB_LOG_UART_ID)
    const uart_config_t config =
    {
        .baud_rate = RISCV_USB_LOG_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_NONE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_FLOWCTRL_DISABLE,
        .src_clk = UART_SCLK_APLL
    };

    bk_uart_init(RISCV_USB_LOG_UART_ID, &config);
#endif

    if (s_host_started != 0U) {
        return 0;
    }

    if ((fw == NULL) || (fw_len == 0U)) {
        return -1;
    }

    if (usb_hc_riscv_start_firmware(fw, fw_len, RISCV_RESET_VEC_TCM) != 0) {
        return -1;
    }

    s_host_started = 1U;
    return 0;
}
