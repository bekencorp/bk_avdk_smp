#include <os/os.h>
#include <os/mem.h>
#include <common/bk_err.h>
#include <driver/int_types.h>
#include <driver/int.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"
#include "sys_driver.h"
#include "sys_types.h"
#include "sys_rtos.h"
#include "usb_driver.h"
#include "usb_regs_address.h"
#include "sys_sw_regs.h"
#include "bk_cherryusb_adapter.h"
#include "port/beken_musb/riscv_bridge/riscv_usb_probe_defs.h"
#if CONFIG_USB_CDC_ACM_DEMO
#include "usbh_cdc_acm.h"
#include "bk_cherry_usb_cdc_acm_api.h"
#endif
#if CONFIG_USB_RISCV_BRIDGE
#include "riscv_usb_bridge.h"
#endif

static beken_mutex_t s_usb_drv_task_mutex = NULL;
static bool s_usb_driver_init_flag = 0;
static bool s_usb_power_on_flag = 0;
static uint32_t s_usb_power_users = 0;
/* Non-static: also referenced by bk_usbd_mtp.c to test whether any host
 * remains before releasing the shared analog island/clock/PM vote. */
bool s_usb_open_close_flag = 0;
bool s_usb_fs_open_close_flag = 0; /* stays 0 when CONFIG_USB_FS_HOST=n */
static bool s_usb_mtp_active = 0;
static uint32_t s_usb_current_mode = 0xFFFFFFFFU;
#if CONFIG_USB_FS_HOST
/* Dual-IP: HS IP (bus 0) keeps s_usb_open_close_flag/s_usb_current_mode;
 * the FS IP (bus 1) has its own open guard. Shared resources (VBUS LDO,
 * USB analog island, USB_1 clock, PM_SLEEP_MODULE_NAME_USB_1 vote) are
 * released only when BOTH ports are closed. */
static bool s_usb_fs_ldo_auto_on = 0; /* we turned the shared LDO on ourselves */
#include "usb_hc_beken_fs.h"
#endif

static bk_err_t usb_driver_sw_deinit();
bk_err_t bk_usb_fs_close(void); /* external linkage: declared in usb.h */

extern void usb_clk_config(uint8_t);
extern void sys_ana_usb_phy_op(uint8_t en);
extern void spitrig_toggle(void);

/* MTP tracks its own activity here instead of setting s_usb_open_close_flag:
 * MTP is a device gadget on busid 0, orthogonal to the host-mode flag, but it
 * owns the same shared analog/clock/PM resources. */
void bk_usb_mtp_set_active(bool active)
{
    s_usb_mtp_active = active;
}

bool bk_usb_mtp_is_active(void)
{
    return s_usb_mtp_active;
}

static bool usb_hs_irq_should_be_managed_by_ap(void)
{
#if CONFIG_USB_RISCV_BRIDGE
	volatile riscv_usb_probe_t *probe = get_riscv_usb_probe();

	if ((probe->magic == RISCV_USB_PROBE_MAGIC) &&
	    (probe->owner == RISCV_USB_PROBE_OWNER_RISCV)) {
		return false;
	}
#endif
	return true;
}

static void usb_hs_irq_route_to_ap(bool route_to_ap)
{
	uint32_t ints_config = sys_drv_get_ints_config_riscv_0_31();

	if (route_to_ap) {
		ints_config &= ~(1U << 8);
	} else {
		ints_config |= (1U << 8);
	}

	sys_drv_set_ints_config_riscv_0_31(ints_config);
}

static void usb_hs_irq_apply(bool route_to_ap, bool enable_ap_irq)
{
	usb_hs_irq_route_to_ap(route_to_ap);
#if CONFIG_SOC_SMP
	sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_USB_HS, enable_ap_irq ? 1 : 0);
#else
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_USB_HS, enable_ap_irq ? 1 : 0);
#endif
}

#define USB_DRIVER_RETURN_NOT_INIT() do {\
	if(!s_usb_driver_init_flag) {\
			return BK_FAIL;\
		}\
	} while(0)


#define USB_DRIVER_RETURN_NOT_DEINIT() do {\
	if(s_usb_driver_init_flag) {\
			return BK_FAIL;\
		}\
	} while(0)

#define USB_RETURN_NOT_POWERED_ON() do {\
		if(!s_usb_power_on_flag) {\
			return BK_ERR_USB_NOT_POWER;\
		}\
	} while(0)


#define USB_RETURN_NOT_POWERED_DOWN() do {\
		if(s_usb_power_on_flag) {\
			return BK_ERR_USB_NOT_POWER;\
		}\
	} while(0)


#define USB_RETURN_NOT_OPENED() do {\
		if(!s_usb_open_close_flag) {\
			return BK_ERR_USB_NOT_OPEN;\
		}\
	} while(0)

#define USB_RETURN_NOT_CLOSED() do {\
		if(s_usb_open_close_flag) {\
			return BK_ERR_USB_NOT_CLOSE;\
		}\
	} while(0)

static void bk_usb_init_all_device_driver_sw(void)
{
	bk_cherryusb_register_host_classes();
	bk_cherryusb_register_device_classes();
}

bk_err_t bk_usb_power_ops(uint32_t gpio_id, bool ops)
{
	if (gpio_id == 0xFF)
	{
		s_usb_power_on_flag = ops;
		return BK_OK;
	}

	if (ops)
	{
		/* Refcounted VBUS LDO: every open path (legacy CLI, simulate-uart,
		 * FS host auto-on, application) registers itself here; the LDO is
		 * only physically released when the last user closes. */
		if (s_usb_power_users == 0)
		{
			bk_gpio_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_USB, gpio_id, GPIO_OUTPUT_STATE_HIGH);
			s_usb_power_on_flag = true;
		}
		s_usb_power_users++;
	}
	else
	{
		if (s_usb_power_users == 0) {
			USB_DRIVER_LOGW("bk_usb_power_ops: unbalanced off\r\n");
			return BK_ERR_USB_NOT_POWER;
		}
		s_usb_power_users--;
		if (s_usb_power_users == 0)
		{
			bk_gpio_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_USB, gpio_id, GPIO_OUTPUT_STATE_LOW);
			s_usb_power_on_flag = false;
		}
	}

	return BK_OK;
}

bk_err_t bk_usb_driver_init(void)
{
	USB_DRIVER_RETURN_NOT_DEINIT();
	USB_DRIVER_LOGV("[+]%s\r\n",__func__);

	bk_usb_init_all_device_driver_sw();

	if(!s_usb_drv_task_mutex){
		rtos_init_mutex(&s_usb_drv_task_mutex);
	}

	s_usb_driver_init_flag = 1;

	USB_DRIVER_LOGV("[-]%s\r\n",__func__);

	return BK_OK;
}

bk_err_t bk_usb_driver_deinit(void)
{
	USB_DRIVER_RETURN_NOT_INIT();

#if CONFIG_USB_FS_HOST
	/* Implicitly close the FS bus first: this unregisters INT_SRC_USB_FS
	 * and powers the FS sub-domain down via the port deinit. */
	if (s_usb_fs_open_close_flag) {
		bk_usb_fs_close();
	}
	/* Belt-and-braces: make sure the FS line is quiet even if an open
	 * failed half-way (driver_deinit must not leave a live IRQ behind). */
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_USB_FS, 0);
	bk_int_isr_unregister(INT_SRC_USB_FS);
	/* The whole USB driver is going away: release the shared VBUS LDO if
	 * the FS path still owns it (bk_usb_fs_close() above deferred to an
	 * open HS port, which is being torn down here as well). */
	if (s_usb_fs_ldo_auto_on) {
		bk_usb_power_ops(CONFIG_USB_VBAT_CONTROL_GPIO_ID, false);
		s_usb_fs_ldo_auto_on = 0;
	}
#endif

#if CONFIG_SOC_SMP
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_USB_HS, 0);
#else
    sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_USB_HS, 0);
#endif
    bk_int_isr_unregister(INT_SRC_USB_HS);

	sys_drv_dev_clk_pwr_up(CLK_PWR_ID_USB_1, CLK_PWR_CTRL_PWR_DOWN);

	if(s_usb_drv_task_mutex) {
		rtos_deinit_mutex(&s_usb_drv_task_mutex);
	}

	s_usb_driver_init_flag = 0;
	return BK_OK;
}

void bk_usb_driver_task_lock_mutex()
{
	if(s_usb_drv_task_mutex)
		rtos_lock_mutex(&s_usb_drv_task_mutex);
}

void bk_usb_driver_task_unlock_mutex()
{
	if(s_usb_drv_task_mutex)
		rtos_unlock_mutex(&s_usb_drv_task_mutex);
}

extern void delay(INT32 num);
// static void bk_usb_host_custom_register_set()
// {
// 	REG_USB_USR_SOFT_RESETEN &= ~(R708_USB_USR_SOFT_RESETN);
// 	REG_USB_USR_CONFIG &= ~(R710_USB_USR_RESET);
// 	delay(100);

// 	uint32_t config_reg = 0;
//     config_reg = R710_USB_USR_TML | R710_USB_USR_CFG_RSTN | R710_USB_USR_REFCLK_MODE | 
//                  R710_USB_USR_PLL_EN | R710_USB_USR_DATA_BUSL6_8 | R710_USB_USR_OTG_SUSPENDM |
//                  R710_USB_USR_ID_DIG_SEL | R710_USB_USR_OTG_AVALID_REG | R710_USB_USR_OTG_AVALID_SEL |
// 				 R710_USB_USR_OTG_VBUSVALID_REG | R710_USB_USR_OTG_VBUSVALID_SEL | R710_USB_USR_OTG_SESSEND_SEL;
// 	config_reg &= ~R710_USB_USR_OTG_SESSEND_REG;
// 	REG_USB_USR_CONFIG = config_reg;

// 	REG_USB_USR_CONFIG |= R710_USB_USR_RESET;
// 	REG_USB_USR_SOFT_RESETEN |=	R708_USB_USR_SOFT_RESETN;
// }

// static void bk_usb_device_custom_register_set()
// {
// 	REG_USB_USR_SOFT_RESETEN &= ~(R708_USB_USR_SOFT_RESETN);
// 	REG_USB_USR_CONFIG &= ~(R710_USB_USR_RESET);
//     //REG_USB_USR_CONFIG |= (0x0<< 0);
// 	uint32_t config_reg = 0;
//     config_reg = R710_USB_USR_REFCLK_MODE | R710_USB_USR_PLL_EN | R710_USB_USR_RESET|
// 	             R710_USB_USR_DATA_BUSL6_8 | R710_USB_USR_OTG_SUSPENDM | R710_USB_USR_ID_DIG_REG |
//                  R710_USB_USR_ID_DIG_SEL | R710_USB_USR_OTG_AVALID_REG | R710_USB_USR_OTG_AVALID_SEL |
// 				 R710_USB_USR_OTG_VBUSVALID_REG | R710_USB_USR_OTG_VBUSVALID_SEL | R710_USB_USR_OTG_SESSEND_SEL;
// 	config_reg &= ~R710_USB_USR_OTG_SESSEND_REG;
// 	REG_USB_USR_CONFIG = config_reg;

//     REG_USB_USR_02 |=R02_USB_USR_SOFT_RESETN;
// }

void bk_analog_layer_usb_sys_related_ops(uint32_t usb_mode, bool ops)
{
	extern void delay(INT32 num);

	if(ops){
		spitrig_toggle();
		delay(100);
		usb_clk_config(1); // sys_drv_usb_clock_ctrl(true, NULL);
		delay(100);
		sys_ana_usb_phy_op(1);// sys_drv_usb_analog_phy_en(1, NULL);

		if(usb_mode == USB_HOST_MODE) {
			REG_USB_USR_708 = 0x0;
			REG_USB_USR_710 &= ~(0x1<< 7);
			delay(100);

			REG_USB_USR_710 |= (0x1<<15);
			REG_USB_USR_710 &=~(0x1<<14);   /* id_dig_reg=0 -> ID low -> A-host (was left at 1) */
			REG_USB_USR_710 |= (0x1<<16);
			REG_USB_USR_710 |= (0x1<<17);
			REG_USB_USR_710 |= (0x1<<18);
			REG_USB_USR_710 |= (0x1<<19);
			REG_USB_USR_710 &=~(0x1<<20);
			REG_USB_USR_710 |= (0x1<<21);
			REG_USB_USR_710 |= (0x0<< 1);
			REG_USB_USR_710 |= (0x1<< 5);
			REG_USB_USR_710 |= (0x1<< 6);
			REG_USB_USR_710 |= (0x1<< 9);
			REG_USB_USR_710 |= (0x1<<10);
			REG_USB_USR_710 |= (0x1<< 1);

			REG_USB_USR_710 |= (0x1<< 7);
			REG_USB_USR_708  =	0x1;

		} else {
			/* #9196: assert the wrapper reset (M55sub USB_HS: 0x2[0] soft_resetn,
			 * 0x4[7] reset); released by the writes at the end of this branch.
			 * Same 0->1 pulse the host branch does - the device branch used to
			 * skip it, so host->device left stale core state. */
			REG_USB_USR_708 = 0x0;
			REG_USB_USR_710 &= ~(0x1<< 7);
			delay(100);

			REG_USB_USR_710 |= (0x1<<15);
			REG_USB_USR_710 |= (0x1<<14);   /* id_dig_reg=1 -> ID high -> B-device */
			REG_USB_USR_710 |= (0x1<<16);
			REG_USB_USR_710 |= (0x1<<17);
			REG_USB_USR_710 |= (0x1<<18);
			REG_USB_USR_710 |= (0x1<<19);
			REG_USB_USR_710 |= (0x1<<20);
			REG_USB_USR_710 &=~(0x1<<21);
			REG_USB_USR_710 |= (0x0<< 0);
			REG_USB_USR_710 |= (0x1<< 5);
			REG_USB_USR_710 |= (0x1<< 6);
			REG_USB_USR_710 |= (0x1<< 9);
			REG_USB_USR_710 |= (0x1<<10);
			REG_USB_USR_710 |= (0x1<< 7);
			REG_USB_USR_710 |= (0x1<< 1);

			REG_USB_USR_708  =	0x1;

		}
	} else {
		sys_ana_usb_phy_op(0); // sys_drv_usb_analog_phy_en(0, NULL);
		usb_clk_config(0); // sys_drv_usb_clock_ctrl(false, NULL);
	}
}

void bk_usb_phy_register_refresh()
{
#if CONFIG_USB_HOST
	bool ap_irq_control = usb_hs_irq_should_be_managed_by_ap();
	/* Force route to AP first, then disable AP IRQ to guarantee both sides are quiet during refresh. */
	usb_hs_irq_apply(true, false);
	bk_gpio_set_output_low(CONFIG_USB_VBAT_CONTROL_GPIO_ID);
	bk_analog_layer_usb_sys_related_ops(USB_HOST_MODE, false);
	bk_analog_layer_usb_sys_related_ops(USB_HOST_MODE, true);
	extern int usb_hc_mhdrc_register_init(void);
	usb_hc_mhdrc_register_init();
	usb_hs_irq_apply(ap_irq_control, ap_irq_control);
	if(s_usb_power_on_flag) {
		bk_gpio_set_output_high(CONFIG_USB_VBAT_CONTROL_GPIO_ID);
	}
#endif
}

bk_err_t bk_usb_open(uint32_t usb_mode)
{
	USB_DRIVER_LOGV("[+]%s\r\n", __func__);

	USB_DRIVER_RETURN_NOT_INIT();
	USB_RETURN_NOT_CLOSED();
	bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_USB_1, 0, 0);
	USB_DRIVER_LOGI("USB_DRV_USB_OPEN!\r\n");
	if(usb_mode == USB_HOST_MODE) {
#if CONFIG_USB_HOST
		bk_analog_layer_usb_sys_related_ops(USB_HOST_MODE, true);
		bk_cherryusb_host_open();
#endif
	} else if(usb_mode == USB_DEVICE_MODE){
#if CONFIG_USB_DEVICE
		bk_analog_layer_usb_sys_related_ops(USB_DEVICE_MODE, true);
		bk_cherryusb_device_open();
#endif
	} else {
		USB_DRIVER_LOGI("PLEASE check USB mode\r\n");
	}

	s_usb_open_close_flag = 1;
	s_usb_current_mode = usb_mode;

	USB_DRIVER_LOGV("[-]%s\r\n", __func__);

	return BK_OK;
}

bk_err_t bk_usb_close(void)
{
	USB_DRIVER_RETURN_NOT_INIT();
	USB_RETURN_NOT_OPENED();

	bk_err_t ret = BK_OK;
	USB_DRIVER_LOGI("USB_DRV_USB_CLOSE!\r\n");
	usb_hs_irq_apply(true, false);
#if CONFIG_USB_RISCV_BRIDGE
	usb_hc_riscv_stop_firmware();
#endif
	if(s_usb_current_mode == USB_HOST_MODE) {
#if CONFIG_USB_HOST
		ret = bk_cherryusb_host_close();
		/* Shared analog island + USB_1 clock are released only when no other
		 * USB user remains (FS host or MTP; HS is closing right here). */
		if (!s_usb_fs_open_close_flag && !s_usb_mtp_active) {
			bk_analog_layer_usb_sys_related_ops(USB_HOST_MODE, false);
		}
#endif
	} else if(s_usb_current_mode == USB_DEVICE_MODE) {
#if CONFIG_USB_DEVICE
		ret = bk_cherryusb_device_close();
		if (!s_usb_fs_open_close_flag && !s_usb_mtp_active) {
			bk_analog_layer_usb_sys_related_ops(USB_DEVICE_MODE, false);
		}
#endif
	}
	s_usb_open_close_flag = 0;
	s_usb_current_mode = 0xFFFFFFFFU;
	if (!s_usb_fs_open_close_flag && !s_usb_mtp_active) {
		bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_USB_1, 1, 0);
	}
#if CONFIG_USB_FS_HOST
	/* Shared VBUS LDO hand-off: if the FS port auto-enabled the LDO and
	 * the FS bus is already closed, this last HS close releases it (the
	 * HS path itself never manages the LDO on its own). */
	if (s_usb_fs_ldo_auto_on && !s_usb_fs_open_close_flag) {
		bk_usb_power_ops(CONFIG_USB_VBAT_CONTROL_GPIO_ID, false);
		s_usb_fs_ldo_auto_on = 0;
	}
#endif
	return ret;
}

#if CONFIG_USB_FS_HOST
bk_err_t bk_usb_fs_open(uint32_t usb_mode)
{
	bk_err_t ret;

	USB_DRIVER_RETURN_NOT_INIT();
	USB_DRIVER_LOGV("[+]%s\r\n", __func__);

	if (s_usb_fs_open_close_flag) {
		USB_DRIVER_LOGI("USB_FS: already open\r\n");
		return BK_ERR_USB_NOT_CLOSE;
	}
	if (usb_mode != USB_HOST_MODE) {
		/* FS device port is not implemented; host-only for now. */
		USB_DRIVER_LOGI("USB_FS: only USB_HOST_MODE is supported\r\n");
		return BK_FAIL;
	}

	/* keep the AP awake while the FS bus is live (vote shared with HS) */
	bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_USB_1, 0, 0);

	/* Shared VBUS LDO: register the FS port as a user unconditionally.
	 * The refcounted bk_usb_power_ops() only physically powers the LDO on
	 * the first user and releases it after the last one closes; legacy
	 * callers (simulate-uart, CLI, CDC demo) are automatically included
	 * in the same count. */
	ret = bk_usb_power_ops(CONFIG_USB_VBAT_CONTROL_GPIO_ID, true);
	if (ret != BK_OK) {
		/* rollback must not re-allow sleep while the HS port is
		 * keeping the AP awake with its own vote */
		if (!s_usb_open_close_flag)
			bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_USB_1, 1, 0);
		return ret;
	}
	s_usb_fs_ldo_auto_on = 1;

	/* FS power sub-domain, usb_fs clock gate, USB analog island and the
	 * shared USB_1 clock are brought up inside the FS port's
	 * usb_hc_beken_fs_init() (hardware-validated sequence). */
	ret = bk_cherryusb_fs_host_open();
	if (ret != BK_OK) {
		if (s_usb_fs_ldo_auto_on) {
			bk_usb_power_ops(CONFIG_USB_VBAT_CONTROL_GPIO_ID, false);
			s_usb_fs_ldo_auto_on = 0;
		}
		if (!s_usb_open_close_flag)
			bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_USB_1, 1, 0);
		return ret;
	}

	s_usb_fs_open_close_flag = 1;
	USB_DRIVER_LOGI("USB_DRV_USB_FS_OPEN!\r\n");

	USB_DRIVER_LOGV("[-]%s\r\n", __func__);
	return BK_OK;
}

bk_err_t bk_usb_fs_close(void)
{
	bk_err_t ret;

	USB_DRIVER_RETURN_NOT_INIT();
	USB_DRIVER_LOGV("[+]%s\r\n", __func__);

	if (!s_usb_fs_open_close_flag) {
		return BK_ERR_USB_NOT_OPEN;
	}

	ret = bk_cherryusb_fs_host_close();

	s_usb_fs_open_close_flag = 0;

	/* Shared VBUS LDO: drop the FS port's user count. The LDO is only
	 * physically released when the last user (HS or FS or legacy) closes. */
	if (s_usb_fs_ldo_auto_on) {
		bk_usb_power_ops(CONFIG_USB_VBAT_CONTROL_GPIO_ID, false);
		s_usb_fs_ldo_auto_on = 0;
	}

	/* Shared USB analog island: when the FS bus is the last USB user,
	 * mirror the HS close path and clear the analog enables + USB_1
	 * clock gate (safe: no other USB user remains). */
	if (!s_usb_open_close_flag && !s_usb_mtp_active) {
		bk_analog_layer_usb_sys_related_ops(USB_HOST_MODE, false);
	}

	/* PM sleep vote is shared: only the last USB user may re-allow sleep. */
	if (!s_usb_open_close_flag && !s_usb_mtp_active) {
		bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_USB_1, 1, 0);
	}

	USB_DRIVER_LOGV("[-]%s\r\n", __func__);
	return ret;
}

bool bk_usb_fs_get_device_connect_status(void)
{
	if (!s_usb_fs_open_close_flag) {
		return false;
	}
	return usb_hc_beken_fs_port_connect_status();
}
#endif /* CONFIG_USB_FS_HOST */


