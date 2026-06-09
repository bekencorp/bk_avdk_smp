#include <common/bk_include.h>
#include <driver/int.h>
#include "sys_driver.h"
#include "sys_rtos.h"
#include "usb_musb_reg.h"
#include "usbd_core.h"
#include "usbh_core.h"

#if CONFIG_SOC_SMP
#include <driver/int_types.h>
#endif

extern void USBH_IRQHandler(uint8_t busid);
extern void USBD_IRQHandler(uint8_t busid);

#ifndef USB_PHY_BASE
#define USB_PHY_BASE (SOC_USB_HS_BASE + 0x400)
#endif

#define NANENG_PHY_FC_REG0B (0x0B * 4)

#define HWREGB(x) (*((volatile uint8_t *)(x)))

static void bk_cherryusb_v16_usbh_irq(void)
{
	USBH_IRQHandler(0);
}

static void bk_cherryusb_v16_usbd_irq(void)
{
	USBD_IRQHandler(0);
}

static void bk_cherryusb_v16_route_irq_to_ap(void)
{
	uint32_t ints_config = sys_drv_get_ints_config_riscv_0_31();

	ints_config &= ~(1U << 8);
	sys_drv_set_ints_config_riscv_0_31(ints_config);
}

static struct musb_fifo_cfg s_musb_device_fifo[] = {
	{ .ep_num = 0, .style = FIFO_TXRX, .maxpacket = 64 },
	{ .ep_num = 1, .style = FIFO_TX,   .maxpacket = 1024 },
	{ .ep_num = 1, .style = FIFO_RX,   .maxpacket = 1024 },
	{ .ep_num = 2, .style = FIFO_TX,   .maxpacket = 512 },
	{ .ep_num = 2, .style = FIFO_RX,   .maxpacket = 512 },
	{ .ep_num = 3, .style = FIFO_TX,   .maxpacket = 512 },
	{ .ep_num = 3, .style = FIFO_RX,   .maxpacket = 512 },
	{ .ep_num = 4, .style = FIFO_TX,   .maxpacket = 512 },
	{ .ep_num = 4, .style = FIFO_RX,   .maxpacket = 512 },
	{ .ep_num = 5, .style = FIFO_TX,   .maxpacket = 512 },
	{ .ep_num = 5, .style = FIFO_RX,   .maxpacket = 512 },
	{ .ep_num = 6, .style = FIFO_TXRX, .maxpacket = 512 },
	{ .ep_num = 7, .style = FIFO_TXRX, .maxpacket = 512 },
};

static struct musb_fifo_cfg s_musb_host_fifo[] = {
	{ .ep_num = 0, .style = FIFO_TXRX, .maxpacket = 64 },
	{ .ep_num = 1, .style = FIFO_TX,   .maxpacket = 1024 },
	{ .ep_num = 1, .style = FIFO_RX,   .maxpacket = 1024 },
	{ .ep_num = 2, .style = FIFO_TX,   .maxpacket = 512 },
	{ .ep_num = 2, .style = FIFO_RX,   .maxpacket = 512 },
	{ .ep_num = 3, .style = FIFO_TX,   .maxpacket = 512 },
	{ .ep_num = 3, .style = FIFO_RX,   .maxpacket = 512 },
	{ .ep_num = 4, .style = FIFO_TX,   .maxpacket = 512 },
	{ .ep_num = 4, .style = FIFO_RX,   .maxpacket = 512 },
	{ .ep_num = 5, .style = FIFO_TX,   .maxpacket = 512 },
	{ .ep_num = 5, .style = FIFO_RX,   .maxpacket = 512 },
	{ .ep_num = 6, .style = FIFO_TXRX, .maxpacket = 512 },
	{ .ep_num = 7, .style = FIFO_TXRX, .maxpacket = 512 },
};

uint8_t usbd_get_musb_fifo_cfg(struct musb_fifo_cfg **cfg)
{
	*cfg = s_musb_device_fifo;
	return sizeof(s_musb_device_fifo) / sizeof(s_musb_device_fifo[0]);
}

uint8_t usbh_get_musb_fifo_cfg(struct musb_fifo_cfg **cfg)
{
	*cfg = s_musb_host_fifo;
	return sizeof(s_musb_host_fifo) / sizeof(s_musb_host_fifo[0]);
}

uint32_t usb_get_musb_ram_size(void)
{
	return 8192;
}

void usbd_musb_delay_ms(uint8_t ms)
{
	rtos_delay_milliseconds(ms);
}

#define M55_CLK_EN_REG  (0x48000000 + 0x0A * 4)

void usb_clk_config(uint8_t en)
{
	uint32_t int_level = sys_drv_enter_critical();
	uint32_t reg = REG_READ(M55_CLK_EN_REG);

	if (en) {
		reg |= (1 << 2);
	} else {
		reg &= ~(1 << 2);
	}
	REG_WRITE(M55_CLK_EN_REG, reg);
	sys_drv_exit_critical(int_level);
}

void sys_ana_usb_phy_op(uint8_t en)
{
#define SYS_ANA_LATCH_REG (0x44010000 + 0x4a * 4)
#define SYS_OP_STATUS     (0x44010000 + 0x3a * 4)
#define SYS_ANA4E_REG     (0x44010000 + 0x4e * 4)
	uint32_t lvl = rtos_enter_critical();
	uint32_t reg_latch = REG_READ(SYS_ANA_LATCH_REG);
	uint32_t reg_val = REG_READ(SYS_ANA4E_REG);

	REG_WRITE(SYS_ANA_LATCH_REG, reg_latch | (0x01 << 9));
	if (en & 0x01) {
		REG_WRITE(SYS_ANA4E_REG, reg_val | (0x03 << 10));
	} else {
		REG_WRITE(SYS_ANA4E_REG, reg_val & ~(0x03 << 10));
	}
	while (REG_READ(SYS_OP_STATUS) >> 0x0E) {
	}
	REG_WRITE(SYS_ANA_LATCH_REG, reg_latch);
	rtos_exit_critical(lvl);
}

void spitrig_toggle(void)
{
#define SYS_ANA_SPI_TRIG_REG (0x44010000 + 0x40 * 4)
	uint32_t value = REG_READ(SYS_ANA_SPI_TRIG_REG);

	REG_WRITE(SYS_ANA_SPI_TRIG_REG, value | (0x01 << 19));
	REG_WRITE(SYS_ANA_SPI_TRIG_REG, value & ~(0x01 << 19));
}

#if CONFIG_USB_RISCV_BRIDGE
extern bool bk_v16_bridge_bringup(struct usbh_bus *bus);
#endif

void usb_hc_low_level_init(struct usbh_bus *bus)
{
	(void)bus;
	HWREGB(USB_PHY_BASE + NANENG_PHY_FC_REG0B) = 0x44;

#if CONFIG_USB_RISCV_BRIDGE
	/* Try to hand the low-level MUSB host engine to the RISC-V CP. When it
	 * takes over, the CP owns the USB HS IRQ and the AP must not register
	 * its own USBH_IRQHandler. */
	if (bk_v16_bridge_bringup(bus)) {
		return;
	}
#endif

	bk_cherryusb_v16_route_irq_to_ap();
	bk_int_isr_register(INT_SRC_USB_HS, bk_cherryusb_v16_usbh_irq, NULL);
	bk_int_set_priority(INT_SRC_USB_HS, 2);
#if CONFIG_SOC_SMP
	sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_USB_HS, 1);
#else
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_USB_HS, 1);
#endif
}

void usb_hc_low_level_deinit(struct usbh_bus *bus)
{
	(void)bus;
#if CONFIG_SOC_SMP
	sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_USB_HS, 0);
#else
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_USB_HS, 0);
#endif
	bk_int_isr_unregister(INT_SRC_USB_HS);
}

#if CONFIG_USB_RISCV_BRIDGE
extern bool bk_v16_device_bridge_bringup(void);
#endif

void usb_dc_low_level_init(void)
{
#if CONFIG_USB_RISCV_BRIDGE
	/* Try to hand the low-level MUSB device engine to the RISC-V CP. When it
	 * takes over, the CP owns the USB HS IRQ and the AP must not register its
	 * own USBD_IRQHandler. Falls back to M55-resident USBD on failure. */
	if (bk_v16_device_bridge_bringup()) {
		return;
	}
#endif

	bk_int_isr_register(INT_SRC_USB_HS, bk_cherryusb_v16_usbd_irq, NULL);
	bk_int_set_priority(INT_SRC_USB_HS, 2);
#if CONFIG_SOC_SMP
	sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_USB_HS, 1);
#else
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_USB_HS, 1);
#endif
}

void usb_dc_low_level_deinit(void)
{
#if CONFIG_SOC_SMP
	sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_USB_HS, 0);
#else
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_USB_HS, 0);
#endif
	bk_int_isr_unregister(INT_SRC_USB_HS);
}
