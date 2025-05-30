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
#if CONFIG_USB_HOST
#include <components/cherryusb/usbh_core.h>
#endif
#if CONFIG_USB_DEVICE
#include <components/cherryusb/usbd_core.h>
#endif
#if CONFIG_USB_HUB
#include <components/cherryusb/usbh_hub.h>
#endif
#if CONFIG_USBH_UAC
#include <components/cherryusb/usbh_audio.h>
#endif
#if CONFIG_USBH_UVC
#include <components/cherryusb/usbh_video.h>
#endif
#if CONFIG_USB_CDC_ACM_DEMO
#include "usbh_cdc_acm.h"
#include "bk_cherry_usb_cdc_acm_api.h"
#endif
#if CONFIG_USBH_SERIAL_CH340
#include "usbh_ch34x.h"
#endif

static beken_thread_t  s_usb_drv_thread_hdl = NULL;
static beken_queue_t s_usb_drv_msg_que = NULL;
static beken_mutex_t s_usb_drv_task_mutex = NULL;
static bk_usb_driver_comprehensive_ops *s_usb_driver_ops = NULL;

static bool s_usb_driver_init_flag = 0;
static bool s_usb_power_on_flag = 0;
static bool s_usb_open_close_flag = 0;
static bool s_usbh_device_connect_flag = 0;

static bk_err_t usb_driver_sw_deinit();

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
#if CONFIG_USB_HUB
	usbh_hub_class_register();
#endif

#if CONFIG_USBH_UVC
	usbh_uvc_class_register();
#endif
#if CONFIG_USBH_UAC
	usbh_uac_class_register();
#endif

#if CONFIG_USBH_MSC
	extern void usbh_msc_register();
	usbh_msc_register();
#endif

#if CONFIG_USB_CDC
	extern void usbh_cdc_acm_class_register();
	usbh_cdc_acm_class_register();
#endif
#if CONFIG_USBH_SERIAL_CH340
	usbh_class_serial_ch340_register_driver();
#endif
}

bk_err_t bk_usb_power_ops(uint32_t gpio_id, bool ops)
{
	if (ops)
	{
		USB_RETURN_NOT_POWERED_DOWN();
		bk_gpio_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_USB, gpio_id, GPIO_OUTPUT_STATE_HIGH);
		s_usb_power_on_flag = ops;
	}
	else
	{
		USB_RETURN_NOT_POWERED_ON();
		bk_gpio_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_USB, gpio_id, GPIO_OUTPUT_STATE_LOW);
		s_usb_power_on_flag = ops;
	}

	return BK_OK;
}

bk_err_t bk_usb_driver_init(void)
{
	USB_DRIVER_RETURN_NOT_DEINIT();
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);

	bk_usb_init_all_device_driver_sw();

	s_usb_driver_init_flag = 1;

	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

	return BK_OK;
}

bk_err_t bk_usb_driver_deinit(void)
{
	USB_DRIVER_RETURN_NOT_INIT();

	sys_drv_int_disable(USB_INTERRUPT_CTRL_BIT);
	bk_int_isr_unregister(INT_SRC_USB);
	sys_drv_dev_clk_pwr_up(CLK_PWR_ID_USB_1, CLK_PWR_CTRL_PWR_DOWN);
	s_usb_driver_init_flag = 0;
	return BK_OK;
}

extern void delay(INT32 num);
static void bk_usb_host_custom_register_set()
{
	REG_USB_USR_SOFT_RESETEN &= ~(R708_USB_USR_SOFT_RESETN);
	REG_USB_USR_CONFIG &= ~(R710_USB_USR_RESET);
	delay(100);

	uint32_t config_reg = 0;
    config_reg = R710_USB_USR_TML | R710_USB_USR_CFG_RSTN | R710_USB_USR_REFCLK_MODE | 
                 R710_USB_USR_PLL_EN | R710_USB_USR_DATA_BUSL6_8 | R710_USB_USR_OTG_SUSPENDM |
                 R710_USB_USR_ID_DIG_SEL | R710_USB_USR_OTG_AVALID_REG | R710_USB_USR_OTG_AVALID_SEL |
				 R710_USB_USR_OTG_VBUSVALID_REG | R710_USB_USR_OTG_VBUSVALID_SEL | R710_USB_USR_OTG_SESSEND_SEL;
	config_reg &= ~R710_USB_USR_OTG_SESSEND_REG;
	REG_USB_USR_CONFIG = config_reg;

	REG_USB_USR_CONFIG |= R710_USB_USR_RESET;
	REG_USB_USR_SOFT_RESETEN |=	R708_USB_USR_SOFT_RESETN;
}

static void bk_usb_device_custom_register_set()
{
	REG_USB_USR_SOFT_RESETEN &= ~(R708_USB_USR_SOFT_RESETN);
	REG_USB_USR_CONFIG &= ~(R710_USB_USR_RESET);
    //REG_USB_USR_CONFIG |= (0x0<< 0);
	uint32_t config_reg = 0;
    config_reg = R710_USB_USR_REFCLK_MODE | R710_USB_USR_PLL_EN | R710_USB_USR_RESET|
	             R710_USB_USR_DATA_BUSL6_8 | R710_USB_USR_OTG_SUSPENDM | R710_USB_USR_ID_DIG_REG |
                 R710_USB_USR_ID_DIG_SEL | R710_USB_USR_OTG_AVALID_REG | R710_USB_USR_OTG_AVALID_SEL |
				 R710_USB_USR_OTG_VBUSVALID_REG | R710_USB_USR_OTG_VBUSVALID_SEL | R710_USB_USR_OTG_SESSEND_SEL;
	config_reg &= ~R710_USB_USR_OTG_SESSEND_REG;
	REG_USB_USR_CONFIG = config_reg;

    REG_USB_USR_SOFT_RESETEN |=	R708_USB_USR_SOFT_RESETN;
}

static void bk_analog_layer_usb_sys_related_ops(uint32_t usb_mode, bool ops)
{
	if(ops){
		sys_drv_usb_clock_ctrl(true, NULL);
		delay(100);

		if(!sys_hal_psram_ldo_status()) {
			sys_drv_psram_ldo_enable(1);
		}
		sys_drv_usb_analog_phy_en(1, NULL);

		if(usb_mode == USB_HOST_MODE) {
			bk_usb_host_custom_register_set();
		} else {
			bk_usb_device_custom_register_set();
		}
	} else {
		sys_drv_usb_analog_phy_en(0, NULL);
		sys_drv_usb_clock_ctrl(false, NULL);
	}
}

void bk_usb_phy_register_refresh()
{
#if CONFIG_USB_HOST
	sys_drv_int_disable(USB_INTERRUPT_CTRL_BIT);
	bk_gpio_set_output_low(CONFIG_USB_VBAT_CONTROL_GPIO_ID);
	bk_analog_layer_usb_sys_related_ops(USB_HOST_MODE, false);
	bk_analog_layer_usb_sys_related_ops(USB_HOST_MODE, true);
	extern int usb_hc_mhdrc_register_init(void);
	usb_hc_mhdrc_register_init();
	sys_drv_int_enable(USB_INTERRUPT_CTRL_BIT);
	if(s_usb_power_on_flag) {
		bk_gpio_set_output_high(CONFIG_USB_VBAT_CONTROL_GPIO_ID);
	}
#endif
}

bk_err_t bk_usb_drv_send_msg(int op, void *param)
{
	bk_err_t ret;
	bk_usb_drv_msg_t msg;

	msg.op = op;
	msg.param = param;
	if (s_usb_drv_msg_que) {
		ret = rtos_push_to_queue(&s_usb_drv_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			USB_DRIVER_LOGE("usb_driver_send_msg fail ret:%d op:%d\r\n", ret, op);
			return BK_FAIL;
		}

		return ret;
	}
	return BK_OK;
}

bk_err_t bk_usb_drv_send_msg_front(int op, void *param)
{
	bk_err_t ret;
	bk_usb_drv_msg_t msg;

	msg.op = op;
	msg.param = param;
	if (s_usb_drv_msg_que) {
		ret = rtos_push_to_queue_front(&s_usb_drv_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			USB_DRIVER_LOGE("%s fail \r\n", __func__);
			return BK_FAIL;
		}

		return ret;
	}
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

static void usb_drv_task_main(beken_thread_arg_t param_data)
{
	bk_err_t ret = kNoErr;

	while(1) {
		bk_usb_drv_msg_t msg;
		ret = rtos_pop_from_queue(&s_usb_drv_msg_que, &msg, BEKEN_WAIT_FOREVER);
		if (kNoErr == ret) {
			switch (msg.op) {
				case USB_DRV_IDLE:
					break;
				case USB_DRV_EXIT:
					rtos_reset_queue(&s_usb_drv_msg_que);
					s_usb_open_close_flag = 0;
					s_usbh_device_connect_flag = false;
					s_usb_driver_ops = NULL;
					goto usb_driver_exit;
					break;
				case USB_DRV_USB_OPEN:
					USB_DRIVER_LOGI("USB_DRV_USB_OPEN!\r\n");
					if(*((uint32_t *)msg.param) == USB_HOST_MODE) {
#if CONFIG_USB_HOST	
						bk_analog_layer_usb_sys_related_ops(USB_HOST_MODE, true);
						usbh_initialize();
#endif
					} else if(*((uint32_t *)msg.param) == USB_DEVICE_MODE){
#if CONFIG_USB_DEVICE
						bk_analog_layer_usb_sys_related_ops(USB_DEVICE_MODE, true);
						usbd_initialize();
#endif
					} else
						USB_DRIVER_LOGI("PLEASE check USB mode\r\n");

					s_usb_open_close_flag = 1;
					break;
				case USB_DRV_USB_CLOSE:
					USB_DRIVER_LOGI("USB_DRV_USB_CLOSE!\r\n");
					if(!s_usb_open_close_flag) break;
					bk_usb_driver_task_lock_mutex();
					sys_drv_int_disable(USB_INTERRUPT_CTRL_BIT);
#if CONFIG_USB_HOST
					usbh_deinitialize();
					bk_analog_layer_usb_sys_related_ops(USB_HOST_MODE, false);
#endif
#if CONFIG_USB_DEVICE
					usbd_deinitialize();
					bk_analog_layer_usb_sys_related_ops(USB_DEVICE_MODE, false);
#endif
					bk_usb_driver_task_unlock_mutex();

					bk_usb_drv_send_msg_front(USB_DRV_EXIT, NULL);
					break;

				default:
					break;
			}
		}
	}

usb_driver_exit:
	usb_driver_sw_deinit();
}

static bk_err_t usb_driver_sw_init()
{
	uint32_t ret = BK_OK;

	if ((!s_usb_drv_thread_hdl) && (!s_usb_drv_msg_que)) {

		if(!s_usb_drv_task_mutex)
			rtos_init_mutex(&s_usb_drv_task_mutex);

		ret = rtos_init_queue(&s_usb_drv_msg_que,
							  "usb_driver_queue",
							  sizeof(bk_usb_drv_msg_t),
							  USB_DRIVER_QITEM_COUNT);
		if (ret != kNoErr) {
			USB_DRIVER_LOGE("ceate usb driver internal message queue fail \r\n");
			return BK_FAIL;
		}

		//create usb driver task
		ret = rtos_create_thread(&s_usb_drv_thread_hdl,
							 CONFIG_TASK_USB_PRIO,
							 "usb_driver",
							 (beken_thread_function_t)usb_drv_task_main,
							 CONFIG_USB_DRIVER_TASK_SIZE,
							 NULL);
		if (ret != kNoErr) {
			USB_DRIVER_LOGE("create usb driver task fail \r\n");
			rtos_deinit_queue(&s_usb_drv_msg_que);
			s_usb_drv_msg_que = NULL;
			s_usb_drv_thread_hdl = NULL;
		}
		USB_DRIVER_LOGD("create usb driver task complete \r\n");

	} else {
		return BK_FAIL;
	}

	return BK_OK;
}

static bk_err_t usb_driver_sw_deinit()
{
	beken_mutex_t usb_drv_task_mutex = NULL;
	beken_queue_t usb_drv_msg_que = NULL;

	if(s_usb_drv_task_mutex) {
		usb_drv_task_mutex = s_usb_drv_task_mutex;
		s_usb_drv_task_mutex = NULL;
		rtos_deinit_mutex(&usb_drv_task_mutex);
	}

	if(s_usb_drv_msg_que) {
		usb_drv_msg_que = s_usb_drv_msg_que;
		s_usb_drv_msg_que = NULL;
		rtos_deinit_queue(&usb_drv_msg_que);
	}

	if (s_usb_drv_thread_hdl) {
		s_usb_drv_thread_hdl = NULL;
		rtos_delete_thread(NULL);
	}

	return BK_OK;
}

bk_err_t bk_usb_open(uint32_t usb_mode)
{
	USB_DRIVER_LOGD("[+]%s\r\n", __func__);

	USB_DRIVER_RETURN_NOT_INIT();
	USB_RETURN_NOT_CLOSED();

	static uint32_t usb_ip_mode = USB_HOST_MODE;
	usb_ip_mode = usb_mode;

	if(usb_driver_sw_init() == BK_OK) {
		USB_DRIVER_LOGD("usb driver sw init OK\r\n");
	} else {
		USB_DRIVER_LOGE("usb driver sw init ERROR\r\n");
		return BK_FAIL;
	}

	bk_usb_drv_send_msg_front(USB_DRV_USB_OPEN, (void *)&usb_ip_mode);

	USB_DRIVER_LOGD("[-]%s\r\n", __func__);

	return BK_OK;
}

bk_err_t bk_usb_close(void)
{
	USB_DRIVER_RETURN_NOT_INIT();
	USB_RETURN_NOT_OPENED();

	bk_err_t ret = BK_OK;
	ret = bk_usb_drv_send_msg(USB_DRV_USB_CLOSE, NULL);

	uint8_t wait_close_finish_count = 0;
	while(s_usb_open_close_flag) {
		if(wait_close_finish_count > 10)
		{
			USB_DRIVER_LOGE("[=]%s close timeout\r\n", __func__);
			ret = BK_FAIL;
			break;
		}
		rtos_delay_milliseconds(10);
		wait_close_finish_count++;
	}

	return ret;
}


