#ifndef _PM_H_
#define _PM_H_


#ifdef __cplusplus
extern "C" {
#endif

#include <driver/int.h>
#include "common/bk_err.h"
/* SOC HAL: power domain enums and PM_POWER_SUB_DOMAIN_* macros (one include for PM + BLE/WIFI/etc.) */
#include "sys_types.h"
#include <driver/aon_rtc_types.h>

/**
 * Sleep callback function type
 *
 * @param arg  User-defined argument passed during registration
 *
 * Note: Callback functions are called at specific points in sleep cycle:
 *   - Pre-sleep callback:  Called before entering sleep mode
 *   - Post-sleep callback: Called after waking up from sleep
 */
typedef void (*sleep_callback_t)(void *arg);
typedef void (*ap_ctrl_callback_t)(void *arg);

typedef enum {
	PM_AP_CTRL_CB_TYPE_POWER_ON = 0,  /**< Execute after AP power-on */
	PM_AP_CTRL_CB_TYPE_POWER_OFF = 1, /**< Execute after AP power-off */
} pm_ap_ctrl_cb_type_t;


#define PM_WIFI_RTC_ALARM_NAME    "bk_wifi"
#define PM_BT_RTC_ALARM_NAME      "bk_bt"
#define PM_APP_RTC_ALARM_NAME     "bk_app"
#define PM_MM_RTC_ALARM_NAME      "mm_rtc"

/* Standard priority definitions for callback execution order
 * Lower value = Higher priority = Executes first
 * Range: 0 (highest) to 255 (lowest)
 */
#define PM_CALLBACK_PRIORITY_CRITICAL    0    /**< Critical hardware (clocks, PMU, power domains) */
#define PM_CALLBACK_PRIORITY_HIGH        50   /**< Important peripherals (UART, SPI, I2C) */
#define PM_CALLBACK_PRIORITY_NORMAL      100  /**< Standard drivers (GPIO, ADC, PWM) */
#define PM_CALLBACK_PRIORITY_LOW         150  /**< Non-critical modules (sensors, LEDs) */
#define PM_CALLBACK_PRIORITY_LOWEST      200  /**< Optional features (debug, logging, statistics) */

#define PARAM_DATA_VALID  (0xFFFF)
#define PM_APP_AUTO_VOTE_ENABLE          (0x1)
#define PM_APP_AUTO_VOTE_DISENABLE       (0x0)

/*--------------------------WAKEUP SOURCE DEFINE  START--------------------------------*/
#define pm_wakeup_source_e                wakeup_source_t

#define PM_WAKEUP_SOURCE_INT_GPIO        (WAKEUP_SOURCE_INT_GPIO)
#define PM_WAKEUP_SOURCE_INT_RTC         (WAKEUP_SOURCE_INT_RTC)
#define PM_WAKEUP_SOURCE_INT_TOUCHED     (WAKEUP_SOURCE_INT_TOUCHED)
#define PM_WAKEUP_SOURCE_INT_SYSTEM_WAKE (WAKEUP_SOURCE_INT_WIFI)//WAKEUP_SOURCE_INT_BT AND WAKEUP_SOURCE_INT_WIFI ARE THE SAME BIT
#define PM_WAKEUP_SOURCE_INT_WIFI        (WAKEUP_SOURCE_INT_WIFI)
#define PM_WAKEUP_SOURCE_INT_BT          (WAKEUP_SOURCE_INT_BT)
#define PM_WAKEUP_SOURCE_INT_USBPLUG     (WAKEUP_SOURCE_INT_USBPLUG)
#define PM_WAKEUP_SOURCE_INT_VAD         (WAKEUP_SOURCE_INT_VAD)
#define PM_WAKEUP_SOURCE_INT_NONE        (WAKEUP_SOURCE_INT_NONE)
/*--------------------------WAKEUP SOURCE DEFINE  END--------------------------------*/

#define pm_dev_clk_e                                   dev_clk_pwr_id_t
/* REG_0xC: 0~31 */
#define PM_CLK_ID_TIMER0                               (CLK_PWR_ID_TIMER0)
#define PM_CLK_ID_TIMER1                               (CLK_PWR_ID_TIMER1)
#define PM_CLK_ID_TIMER2                               (CLK_PWR_ID_TIMER2)
#define PM_CLK_ID_TIMER3                               (CLK_PWR_ID_TIMER3)
#define PM_CLK_ID_UART0                                (CLK_PWR_ID_UART0)
#define PM_CLK_ID_UART1                                (CLK_PWR_ID_UART1)
#define PM_CLK_ID_UART2                                (CLK_PWR_ID_UART2)
#define PM_CLK_ID_UART3                                (CLK_PWR_ID_UART3)
#define PM_CLK_ID_UART4                                (CLK_PWR_ID_UART4)
#define PM_CLK_ID_SPI0                                 (CLK_PWR_ID_SPI0)
#define PM_CLK_ID_SPI1                                 (CLK_PWR_ID_SPI1)
#define PM_CLK_ID_SPI2                                 (CLK_PWR_ID_SPI2)
#define PM_CLK_ID_SPI3                                 (CLK_PWR_ID_SPI3)
#define PM_CLK_ID_SARADC                               (CLK_PWR_ID_SARADC)
#define PM_CLK_ID_PWM0                                 (CLK_PWR_ID_PWM0)
#define PM_CLK_ID_OTP                                  (CLK_PWR_ID_OTP)
#define PM_CLK_ID_I3C                                  (CLK_PWR_ID_I3C)
#define PM_CLK_ID_I2S0                                 (CLK_PWR_ID_I2S0)
#define PM_CLK_ID_I2S1                                 (CLK_PWR_ID_I2S1)
#define PM_CLK_ID_I2S2                                 (CLK_PWR_ID_I2S2)
#define PM_CLK_ID_I2S3                                 (CLK_PWR_ID_I2S3)
#define PM_CLK_ID_I2S4                                 (CLK_PWR_ID_I2S4)
#define PM_CLK_ID_I2C0                                 (CLK_PWR_ID_I2C0)
#define PM_CLK_ID_I2C3                                 (CLK_PWR_ID_I2C3)
#define PM_CLK_ID_IRDA0                                (CLK_PWR_ID_IRDA0)
#define PM_CLK_ID_IRDA1                                (CLK_PWR_ID_IRDA1)
#define PM_CLK_ID_IRDA2                                (CLK_PWR_ID_IRDA2)
#define PM_CLK_ID_IRDA3                                (CLK_PWR_ID_IRDA3)
#define PM_CLK_ID_CAN0                                 (CLK_PWR_ID_CAN0)
#define PM_CLK_ID_CAN1                                 (CLK_PWR_ID_CAN1)
#define PM_CLK_ID_LIN0                                 (CLK_PWR_ID_LIN0)
#define PM_CLK_ID_SCR0                                 (CLK_PWR_ID_SCR0)
/* REG_0xD: 32~62 */
#define PM_CLK_ID_AUDIO                                (CLK_PWR_ID_AUDIO)
#define PM_CLK_ID_AUDIF0                               (CLK_PWR_ID_AUDIF0)
#define PM_CLK_ID_AUDIF1                               (CLK_PWR_ID_AUDIF1)
#define PM_CLK_ID_I2SO                                 (CLK_PWR_ID_I2SO)
#define PM_CLK_ID_CEC                                  (CLK_PWR_ID_CEC)
#define PM_CLK_ID_XDAC0                                (CLK_PWR_ID_XDAC0)
#define PM_CLK_ID_XDAC1                                (CLK_PWR_ID_XDAC1)
#define PM_CLK_ID_AUXS                                 (CLK_PWR_ID_AUXS)
#define PM_CLK_ID_AUXS_ENET                            (CLK_PWR_ID_AUXS_ENET)
#define PM_CLK_ID_26MS                                 (CLK_PWR_ID_26MS)
#define PM_CLK_ID_32KS                                 (CLK_PWR_ID_32KS)
#define PM_CLK_ID_26MO                                 (CLK_PWR_ID_26MO)
#define PM_CLK_ID_240M                                 (CLK_PWR_ID_240M)
#define PM_CLK_ID_320M                                 (CLK_PWR_ID_320M)
#define PM_CLK_ID_480M                                 (CLK_PWR_ID_480M)
#define PM_CLK_ID_160M                                 (CLK_PWR_ID_160M)
#define PM_CLK_ID_120M                                 (CLK_PWR_ID_120M)
#define PM_CLK_ID_TRACE                                (CLK_PWR_ID_TRACE)
#define PM_CLK_ID_WLSS                                 (CLK_PWR_ID_WLSS)
#define PM_CLK_ID_BTDM                                 (CLK_PWR_ID_BTDM)
#define PM_CLK_ID_XVR                                  (CLK_PWR_ID_XVR)
#define PM_CLK_ID_MAC                                  (CLK_PWR_ID_MAC)
#define PM_CLK_ID_PHY                                  (CLK_PWR_ID_PHY)
#define PM_CLK_ID_THREAD                               (CLK_PWR_ID_THREAD)
#define PM_CLK_ID_BK24                                 (CLK_PWR_ID_BK24)
#define PM_CLK_ID_RF                                   (CLK_PWR_ID_RF)
#define PM_CLK_ID_OFDM                                 (CLK_PWR_ID_OFDM)
/* REG_0xA: 64~89 */
#define PM_CLK_ID_CPUA                                 (CLK_PWR_ID_CPUA)
#define PM_CLK_ID_UART5                                (CLK_PWR_ID_UART5)
#define PM_CLK_ID_USB_HS                               (CLK_PWR_ID_USB_HS)
#define PM_CLK_ID_PSRAM0                               (CLK_PWR_ID_PSRAM0)
#define PM_CLK_ID_PSRAM1                               (CLK_PWR_ID_PSRAM1)
#define PM_CLK_ID_QSPI0                                (CLK_PWR_ID_QSPI0)
#define PM_CLK_ID_QSPI1                                (CLK_PWR_ID_QSPI1)
#define PM_CLK_ID_SDIO0                                (CLK_PWR_ID_SDIO0)
#define PM_CLK_ID_SDIO1                                (CLK_PWR_ID_SDIO1)
#define PM_CLK_ID_CISP                                 (CLK_PWR_ID_CISP)
#define PM_CLK_ID_GPU                                  (CLK_PWR_ID_GPU)
#define PM_CLK_ID_H26E                                 (CLK_PWR_ID_H26E)
#define PM_CLK_ID_CSI                                  (CLK_PWR_ID_CSI)
#define PM_CLK_ID_DSI                                  (CLK_PWR_ID_DSI)
#define PM_CLK_ID_DPU                                  (CLK_PWR_ID_DPU)
#define PM_CLK_ID_USB_FS                               (CLK_PWR_ID_USB_FS)
#define PM_CLK_ID_CONF0                                (CLK_PWR_ID_CONF0)
#define PM_CLK_ID_CONF1                                (CLK_PWR_ID_CONF1)
#define PM_CLK_ID_CONF2                                (CLK_PWR_ID_CONF2)
#define PM_CLK_ID_CONF3                                (CLK_PWR_ID_CONF3)
#define PM_CLK_ID_CPU0                                 (CLK_PWR_ID_CPU0)
#define PM_CLK_ID_CPU1                                 (CLK_PWR_ID_CPU1)
#define PM_CLK_ID_NPU                                  (CLK_PWR_ID_NPU)
#define PM_CLK_ID_TIMER4                               (CLK_PWR_ID_TIMER4)
#define PM_CLK_ID_TIMER5                               (CLK_PWR_ID_TIMER5)
#define PM_CLK_ID_TRACE_AP                             (CLK_PWR_ID_TRACE_AP)
#define PM_CLK_ID_EFUSE                                (CLK_PWR_ID_EFUSE)
#define PM_CLK_ID_PSRAM0                               (CLK_PWR_ID_PSRAM0)
#define PM_CLK_ID_PSRAM1                               (CLK_PWR_ID_PSRAM1)
#define PM_CLK_ID_NONE                                 (CLK_PWR_ID_NONE)

/*---------------------------POWER DOMAIN MODUEL DEFINE  END-------------------------------------*/
typedef struct
{
    uint32_t event;
    uint32_t param1;
	uint32_t param2;
    uint32_t param3;
} pm_ap_core_msg_t;

typedef enum
{
   PM_ENTER_LOW_VOLTAGE_MSG = 0,
   PM_ENTER_DEEP_SLEEP_MSG,
   PM_CALLBACK_HANDLE_MSG
}pm_sleep_mode_msg_e;

typedef enum
{
	PM_CP1_AUTO_CTRL_DISABLE = 0,
	PM_CP1_AUTO_CTRL_ENABLE    = 1,
}pm_cp1_auto_ctrl_e;

typedef enum
{
	GPIO_TRIGGER_INTERRUPE_LEVEL_LOW_ACTIVE = 0,
	GPIO_TRIGGER_INTERRUPE_LEVEL_HIGH_ACTIVE,
	GPIO_TRIGGER_INTERRUPE_EDGE_RISING,
	GPIO_TRIGGER_INTERRUPE_EDGE_FALLING,
}gpio_trigger_interrupt_type_e;

typedef struct
{
	uint32_t gpio_id;
	gpio_trigger_interrupt_type_e gpio_trigger_interrupt_type;
	int_group_isr_t isr_callback;
	uint32_t gpio_valid;  //PARAM_DATA_VALID
}gpio_wakeup_param_t;
typedef struct
{
	uint32_t  period;
	int_group_isr_t isr_callback;
	uint32_t rtc_valid;   //PARAM_DATA_VALID
}rtc_wakeup_param_t;
typedef enum
{
	WIFI_WAKEUP = 1,
	BT_WAKEUP,
}wifi_bt_wakeup_type_e;
typedef struct
{
	wifi_bt_wakeup_type_e  wifi_bt_wakeup;
	uint32_t  sleep_time;
	int_group_isr_t isr_callback;
}system_wakeup_param_t;
typedef struct
{
    int_group_isr_t isr_callback;
}usbplug_wakeup_param_t;
typedef struct
{
	uint32_t  touch_channel;
    int_group_isr_t isr_callback;
}touch_wakeup_param_t;
typedef enum
{
	PM_MODE_NORMAL_SLEEP = 0,
	PM_MODE_LOW_VOLTAGE ,
	PM_MODE_DEEP_SLEEP ,
	PM_MODE_SUPER_DEEP_SLEEP ,
	PM_MODE_FORCE_DEEP_SLEEP ,
	PM_MODE_DEFAULT
}pm_sleep_mode_e;

typedef struct
{
	uint32_t rtc_period;  //unit:ms
	uint32_t rtc_cnt;     //total period count == 0Xffffffff means forever period;else period how many times
}pm_rtc_wakeup_config_t;
typedef struct
{
	uint16_t gpio_id;
	uint16_t int_type;
}pm_gpio_wakeup_config_t;
typedef enum
{
	PM_NORMAL_SLEEP_ULTRA_EXIT = 0,  // 0:exit ultra normal sleep
	PM_NORMAL_SLEEP_ULTRA_ENTER,     // 1:enter ultra normal sleep
	PM_NORMAL_SLEEP_MODE_1,          // 2:normal sleep mode 1
	PM_NORMAL_SLEEP_WORKING,         // 3:normal sleep working
}pm_normal_sleep_mode_e;

typedef enum
{
	PM_SLEEP_MODULE_NAME_I2C1 = 0, // 0
	PM_SLEEP_MODULE_NAME_SPI_1,    // 1
	PM_SLEEP_MODULE_NAME_UART1,    // 2
	PM_SLEEP_MODULE_NAME_PWM_1,    // 3
	PM_SLEEP_MODULE_NAME_TIMER_1,  // 4
	PM_SLEEP_MODULE_NAME_SARADC,   // 5
	PM_SLEEP_MODULE_NAME_AUDP,     // 6
	PM_SLEEP_MODULE_NAME_VIDP,     // 7
	PM_SLEEP_MODULE_NAME_BTSP,     // 8
	PM_SLEEP_MODULE_NAME_WIFIP_MAC,// 9
	PM_SLEEP_MODULE_NAME_WIFI_PHY, // 10
	PM_SLEEP_MODULE_NAME_TIMER_2,  // 11
	PM_SLEEP_MODULE_NAME_APP,      // 12  //attention: the APP vote is for customer application using,other applications use the other votes
	PM_SLEEP_MODULE_NAME_OTP,      // 13
	PM_SLEEP_MODULE_NAME_I2S_1,    // 14
	PM_SLEEP_MODULE_NAME_USB_1,    // 15
	PM_SLEEP_MODULE_NAME_CAN,      // 16
	PM_SLEEP_MODULE_NAME_PSRAM,    // 17
	PM_SLEEP_MODULE_NAME_QSPI_1,   // 18
	PM_SLEEP_MODULE_NAME_QSPI_2,   // 19
	PM_SLEEP_MODULE_NAME_SDIO,     // 20
	PM_SLEEP_MODULE_NAME_AUXS,     // 21
	PM_SLEEP_MODULE_NAME_LOG,      // 22
	PM_SLEEP_MODULE_NAME_AT,       // 23
	PM_SLEEP_MODULE_NAME_I2C2,     // 24
	PM_SLEEP_MODULE_NAME_UART2,    // 25
	PM_SLEEP_MODULE_NAME_UART3,    // 26
	PM_SLEEP_MODULE_NAME_WDG,      // 27
	PM_SLEEP_MODULE_NAME_AUDIO_ASR,// 28
	PM_SLEEP_MODULE_NAME_APP1,     // 29
	PM_SLEEP_MODULE_NAME_CPU1,     // 30
	PM_SLEEP_MODULE_NAME_ROSC_PROG,// 31
	PM_SLEEP_MODULE_NAME_ROSC,     // 32
	PM_SLEEP_MODULE_NAME_FLASH_OP, // 33
	PM_SLEEP_MODULE_NAME_LV_WAKEUP,// 34
	PM_SLEEP_MODULE_NAME_BK_MODEM, // 35
	PM_SLEEP_MODULE_NAME_ENCP,     // 36
	/*Attention: if you add vote ,the following[ #define PM_ENTER_LOW_VOL_MODULES_CONFIG] also need add the vote*/
	PM_SLEEP_MODULE_NAME_MAX      //MAX value is 63
}pm_sleep_module_name_e;
typedef enum
{
	PM_POWER_MODULE_STATE_ON = 0,
    PM_POWER_MODULE_STATE_OFF,
	PM_POWER_MODULE_STATE_NONE
}pm_power_module_state_e;
typedef enum
{
	PM_XTAL_RX_TX_ANABUF_ENTER_SLEEP = 0,
    PM_XTAL_RX_TX_ANABUF_EXIT_SLEEP,
	PM_XTAL_RX_TX_ANABUF_NONE
}pm_xtal_rx_tx_anabuf_state_e;
typedef enum
{
	PM_XTAL_RX_TX_ANABUF_MODULE_NAME_DSSS_ONLY = 0,
	PM_XTAL_RX_TX_ANABUF_MODULE_NAME_AP        = 1,
	PM_XTAL_RX_TX_ANABUF_MODULE_NAME_USER_BASE = 20, // application can define private module IDs from this base.
	PM_XTAL_RX_TX_ANABUF_MODULE_NAME_MAX       = 31, // max valid module ID.
}pm_xtal_rx_tx_anabuf_module_name_e;
typedef enum
{
	PM_MODULE_NAME_WIFI = 0,
	PM_MODULE_NAME_BT,
	PM_MODULE_NAME_NONE
}pm_module_name_e;
typedef enum
{
	PM_LPO_SRC_DIVD = 0,//32K from 26m
	PM_LPO_SRC_X32K,    //extern 32k
	PM_LPO_SRC_ROSC,    //32K from ROSC
	PM_LPO_SRC_DEFAULT  //32K from ROSC
}pm_lpo_src_e;

typedef enum
{
	PM_CLK_CTRL_PWR_DOWN = 0,
	PM_CLK_CTRL_PWR_UP,
}pm_dev_clk_pwr_e;
typedef enum
{
	PM_DEV_ID_TIMER_0,  // 0
	PM_DEV_ID_I2C1,     // 1
	PM_DEV_ID_SPI_1,    // 2
	PM_DEV_ID_UART1,    // 3
	PM_DEV_ID_AIRPLAY,  // 4
	PM_DEV_ID_TIMER_1,  // 5
	PM_DEV_ID_SARADC,   // 6
	PM_DEV_ID_IRDA,     // 7
	PM_DEV_ID_EFUSE,    // 8
	PM_DEV_ID_I2C2,     // 9
	PM_DEV_ID_SPI_2,    //10
	PM_DEV_ID_UART2,    // 11
	PM_DEV_ID_UART3,    // 12
	PM_DEV_ID_PWM_2,    // 13
	PM_DEV_ID_TIMER_2,  // 14
	PM_DEV_ID_TIMER_3,  // 15
	PM_DEV_ID_TOUCH,    // 16
	PM_DEV_ID_I2S_1,    // 17
	PM_DEV_ID_USB_1,    // 18
	PM_DEV_ID_CAN,      // 19
	PM_DEV_ID_PSRAM,    // 20
	PM_DEV_ID_QSPI_1,   // 21
	PM_DEV_ID_QSPI_2,   // 22
	PM_DEV_ID_SDIO,     // 23
	PM_DEV_ID_AUXS,     // 24
	PM_DEV_ID_BTDM,     // 25
	PM_DEV_ID_WPAS,     // 26
	PM_DEV_ID_MAC,      // 27
	PM_DEV_ID_PHY,      // 28
	PM_DEV_ID_JPEG,     // 29
	PM_DEV_ID_DISP,     // 30
	PM_DEV_ID_AUDIO,    // 31
	PM_DEV_ID_RTC,      // 32
	PM_DEV_ID_GPIO,     // 33
	PM_DEV_ID_DECODER,  // 34
	PM_DEV_ID_LIN,      // 35
	PM_DEV_ID_PWM_1,    // 36
	PM_DEV_ID_SECURE_WORLD,// 37
	PM_DEV_ID_UART4,       // 38
	PM_DEV_ID_TRNG,        // 39
	PM_DEV_ID_CPU1,        // 40
	PM_DEV_ID_PHY_DPD_CALI, //41
	PM_DEV_ID_KEY,          //42
	PM_DEV_ID_CIF,          //43
	PM_DEV_ID_MAILBOX,      //44
	/*
	 * S2 (HPDMA review):
	 *   Keep CP-side PM device id table in lock-step with the AP side.
	 *   PM_DEV_ID_HPDMA is consumed only by the AP HPDMA driver (CP has
	 *   no HPDMA presence today), but the enum is part of the cross-
	 *   core PM ABI: shifting PM_DEV_ID_DEFAULT to a different ordinal
	 *   on one core would silently misalign any cross-core PM tables.
	 */
	PM_DEV_ID_HPDMA,        //45

	PM_DEV_ID_DEFAULT,      //46  it is used by pm module set default cpu frequency

	PM_DEV_ID_MAX
}pm_dev_id_e;
typedef enum
{
	PM_CPU_FRQ_XTAL = 0, // 0:CPU:XTAL,BUS:XTAL(eg:XTAL:40M/26M)
	PM_CPU_FRQ_60M,      // 1:CPU0:60M,BUS:60M
	PM_CPU_FRQ_80M,      // 2:CPU0:80M,BUS:80M
	PM_CPU_FRQ_120M,     // 3:CPU0:120M,BUS:120M
	PM_CPU_FRQ_160M,     // 4:CPU0:160M,BUS:160M
	PM_CPU_FRQ_240M,     // 5:CPU0:240M,BUS:240M

	PM_CPU_FRQ_HIGHEST,  // 6:(Appli do not need to be concerned with the specific CPU frequency)
	PM_CPU_FRQ_DEFAULT   // default cpu frequency which control by pm module
}pm_cpu_freq_e;
typedef enum
{
	PM_ROSC_CALI_AUTO = 0,  // 0:auto calibration
	PM_ROSC_CALI_MANUAL,    // 1:manual calibration
	PM_ROSC_CALI_STOP,      // 2:stop calibration
}pm_rosc_cali_mode_e;
typedef enum
{
	PM_ENTER_LV_TIME_OUT_MODULE_APP = 0,
	PM_ENTER_LV_TIME_OUT_MODULE_MAX,
}pm_enter_lv_timeout_module_e;
typedef bk_err_t (*pm_enter_lv_timeout_cb)(void *args);
typedef struct {
    pm_enter_lv_timeout_cb cb;
    void *args;
} pm_lv_timeout_cb_conf_t;
typedef struct {
    pm_enter_lv_timeout_module_e module;
    pm_lv_timeout_cb_conf_t cfg;
} pm_enter_lv_timeout_cb_t;

typedef int (*pm_cb)(uint64_t sleep_time, void *args);
typedef struct {
    pm_cb cb;
    void *args;
} pm_cb_conf_t;

typedef struct {
    pm_dev_id_e id;
    pm_cb_conf_t cfg;
} pm_sleep_cb_t;

typedef enum
{
	PM_CB_PRIORITY_0 = 0,
	PM_CB_PRIORITY_1,
	PM_CB_PRIORITY_2,
	PM_CB_PRIORITY_3,
	PM_CB_PRIORITY_MAX,
} pm_cb_priority_e;

typedef enum
{
	PM_32K_MODULE_BT = 0,
	PM_32K_MODULE_WIFI = 1,
	PM_32K_MODULE_MAX,
}pm_cb_module_e;

typedef enum
{
	PM_32K_STEP_BEGIN = 0,
	PM_32K_STEP_FINISH = 1,
}pm_sw_step_e;     // Indicate switch status

typedef int (*pm_cb_notify)(void);
typedef int (*pm_cb_extern32k)(pm_sw_step_e sw_step, pm_lpo_src_e lpo_src, pm_cb_notify cb);
typedef struct {
	pm_cb_module_e cb_module;
	pm_cb_extern32k cb_func;
} pm_cb_extern32k_cfg_t;

typedef enum
{
	PM_LOW_VOL_VOLTAGE_0_6 = 0, // 0.6V
	PM_LOW_VOL_VOLTAGE_0_7,     // 0.7V
	PM_LOW_VOL_VOLTAGE_0_8,     // 0.8V
	PM_LOW_VOL_VOLTAGE_0_9,     // 0.9V
	PM_LOW_VOL_VOLTAGE_1_0,     // 1.0V
	PM_LOW_VOL_VOLTAGE_1_1,     // 1.1V
	PM_LOW_VOL_VOLTAGE_1_2,     // 1.2V
	PM_LOW_VOL_VOLTAGE_1_3      // 1.3V
}pm_low_vol_voltage_e;


/*=====================CONFIG  SECTION  START====================*/
#define PM_ENTER_NORMAL_SLEEP_MODULES_CONFIG \
{\
	PM_SLEEP_MODULE_NAME_I2C1,\
	PM_SLEEP_MODULE_NAME_SPI_1,\
	PM_SLEEP_MODULE_NAME_UART1,\
	PM_SLEEP_MODULE_NAME_PWM_1,\
	PM_SLEEP_MODULE_NAME_TIMER_1,\
	PM_SLEEP_MODULE_NAME_SARADC,\
	PM_SLEEP_MODULE_NAME_AUDP,\
	PM_SLEEP_MODULE_NAME_VIDP,\
	PM_SLEEP_MODULE_NAME_BTSP,\
	PM_SLEEP_MODULE_NAME_WIFIP_MAC,\
	PM_SLEEP_MODULE_NAME_WIFI_PHY,\
	PM_SLEEP_MODULE_NAME_TIMER_2,\
	PM_SLEEP_MODULE_NAME_APP,\
	PM_SLEEP_MODULE_NAME_OTP,\
	PM_SLEEP_MODULE_NAME_I2S_1,\
	PM_SLEEP_MODULE_NAME_USB_1,\
	PM_SLEEP_MODULE_NAME_CAN,\
	PM_SLEEP_MODULE_NAME_PSRAM,\
	PM_SLEEP_MODULE_NAME_QSPI_1,\
	PM_SLEEP_MODULE_NAME_QSPI_2,\
	PM_SLEEP_MODULE_NAME_SDIO,\
	PM_SLEEP_MODULE_NAME_AUXS,\
	PM_SLEEP_MODULE_NAME_LOG,\
	PM_SLEEP_MODULE_NAME_AT,\
	PM_SLEEP_MODULE_NAME_I2C2,\
	PM_SLEEP_MODULE_NAME_UART2,\
	PM_SLEEP_MODULE_NAME_UART3,\
	PM_SLEEP_MODULE_NAME_WDG,\
	PM_SLEEP_MODULE_NAME_AUDIO_ASR,\
	PM_SLEEP_MODULE_NAME_APP1,\
	PM_SLEEP_MODULE_NAME_CPU1,\
	PM_SLEEP_MODULE_NAME_ROSC_PROG,\
	PM_SLEEP_MODULE_NAME_ROSC,\
	PM_SLEEP_MODULE_NAME_FLASH_OP, \
	PM_SLEEP_MODULE_NAME_LV_WAKEUP,\
	PM_SLEEP_MODULE_NAME_BK_MODEM,\
	PM_SLEEP_MODULE_NAME_ENCP,\
}

/*enter low voltage ,require sleep module config*/
#define PM_ENTER_LOW_VOL_MODULES_CONFIG \
{\
	PM_SLEEP_MODULE_NAME_I2C1,\
	PM_SLEEP_MODULE_NAME_SPI_1,\
	PM_SLEEP_MODULE_NAME_UART1,\
	PM_SLEEP_MODULE_NAME_PWM_1,\
	PM_SLEEP_MODULE_NAME_TIMER_1,\
	PM_SLEEP_MODULE_NAME_SARADC,\
	PM_SLEEP_MODULE_NAME_AUDP,\
	PM_SLEEP_MODULE_NAME_VIDP,\
	PM_SLEEP_MODULE_NAME_BTSP,\
	PM_SLEEP_MODULE_NAME_WIFIP_MAC,\
	PM_SLEEP_MODULE_NAME_WIFI_PHY,\
	PM_SLEEP_MODULE_NAME_TIMER_2,\
	PM_SLEEP_MODULE_NAME_APP,\
	PM_SLEEP_MODULE_NAME_OTP,\
	PM_SLEEP_MODULE_NAME_I2S_1,\
	PM_SLEEP_MODULE_NAME_USB_1,\
	PM_SLEEP_MODULE_NAME_CAN,\
	PM_SLEEP_MODULE_NAME_PSRAM,\
	PM_SLEEP_MODULE_NAME_QSPI_1,\
	PM_SLEEP_MODULE_NAME_QSPI_2,\
	PM_SLEEP_MODULE_NAME_SDIO,\
	PM_SLEEP_MODULE_NAME_AUXS,\
	PM_SLEEP_MODULE_NAME_LOG,\
	PM_SLEEP_MODULE_NAME_AT,\
	PM_SLEEP_MODULE_NAME_I2C2,\
	PM_SLEEP_MODULE_NAME_UART2,\
	PM_SLEEP_MODULE_NAME_UART3,\
	PM_SLEEP_MODULE_NAME_WDG,\
	PM_SLEEP_MODULE_NAME_AUDIO_ASR,\
	PM_SLEEP_MODULE_NAME_APP1,\
	PM_SLEEP_MODULE_NAME_CPU1,\
	PM_SLEEP_MODULE_NAME_ROSC_PROG,\
	PM_SLEEP_MODULE_NAME_ROSC,\
	PM_SLEEP_MODULE_NAME_FLASH_OP,\
	PM_SLEEP_MODULE_NAME_LV_WAKEUP,\
	PM_SLEEP_MODULE_NAME_BK_MODEM,\
	PM_SLEEP_MODULE_NAME_ENCP,\
}

/*enter deep sleep ,require sleep module config*/
#define PM_ENTER_DEEP_SLEEP_MODULES_CONFIG \
{\
\
}
typedef enum {
	BK_PM_WAKEUP_UNKNOWN,       /* Unknown Wakeup Source*/
	BK_PM_WAKEUP_BLE,           /* 1:BLE Interrupts */
	BK_PM_WAKEUP_WIFI,          /* 2:Wifi Interrupts */
	BK_PM_WAKEUP_UART_CONSOLE,  /* 3:UART Console Interrupts */
	BK_PM_WAKEUP_UART_TTYS2,    /* 4:UART TTYS2 Interrupts */
	BK_PM_WAKEUP_GPIO,          /* 5:GPIO Interrupts */
	BK_PM_WAKEUP_HW_TIMER,      /* 6:Timer Expiration */
	BK_PM_WAKEUP_SRC_COUNT,     /* 7:Source Count */
} bk_pm_wakeup_reason_e;

typedef enum {
	PM_AP_WORK_STATE_FIRST_BOOT   = (1U << 0), /**< first AP boot */
	PM_AP_WORK_STATE_BOOT_SUCCESS = (1U << 1), /**< AP boot success */
	PM_AP_WORK_STATE_FAST_RESUME  = (1U << 2), /**< AP FreeRTOS context is ready for restore */
	PM_AP_WORK_STATE_RESERVED3    = (1U << 3), /**< reserved for extension */
} pm_ap_work_state_e;

typedef struct {
	volatile uint8_t pm_ap0_sleep_state;
	volatile uint8_t pm_ap1_sleep_state;
	volatile uint8_t pm_cp0_sleep_state;
	volatile uint8_t pm_cp1_sleep_state;
	volatile uint8_t pm_ap_work_state;
	volatile pm_wakeup_source_e wakeup_source;
	volatile uint8_t wakeup_alarm_name[ALARM_NAME_MAX_LEN+1];
	volatile uint8_t gpio_id;
	volatile uint32_t param0;
	volatile uint32_t param1;
	volatile uint32_t param2;
} pm_shared_info_t;

typedef enum {
	BK_SYS_SW_REGS_PM_SHARED_INFO_FIELD_AP0_SLEEP_STATE = (1U << 0),
	BK_SYS_SW_REGS_PM_SHARED_INFO_FIELD_AP1_SLEEP_STATE = (1U << 1),
	BK_SYS_SW_REGS_PM_SHARED_INFO_FIELD_CP0_SLEEP_STATE = (1U << 2),
	BK_SYS_SW_REGS_PM_SHARED_INFO_FIELD_CP1_SLEEP_STATE = (1U << 3),
	BK_SYS_SW_REGS_PM_SHARED_INFO_FIELD_AP_WORK_STATE   = (1U << 4),
	BK_SYS_SW_REGS_PM_SHARED_INFO_FIELD_WAKEUP_SOURCE   = (1U << 5),
	BK_SYS_SW_REGS_PM_SHARED_INFO_FIELD_WAKEUP_ALARM_NAME = (1U << 6),
	BK_SYS_SW_REGS_PM_SHARED_INFO_FIELD_GPIO_ID         = (1U << 7),
	BK_SYS_SW_REGS_PM_SHARED_INFO_FIELD_PARAM0          = (1U << 8),
	BK_SYS_SW_REGS_PM_SHARED_INFO_FIELD_PARAM1          = (1U << 9),
	BK_SYS_SW_REGS_PM_SHARED_INFO_FIELD_PARAM2          = (1U << 10),
	BK_SYS_SW_REGS_PM_SHARED_INFO_FIELD_ALL             = (1U << 11) - 1U,
} bk_sys_sw_regs_pm_shared_info_field_t;

typedef enum
{
	/* Use a dedicated range to avoid clashing with PM thread local events (0~2). */
	PM_CP_CORE_STATE_ENTER_DEEPSLEEP = 0x100,
	PM_CP_CORE_CTRL_AP_STATE,
	PM_CP_CORE_AP_RECOVERY,
	PM_CP_CORE_RTC_DEEPSLEEP,
	PM_CP_CORE_GET_CP_DATA,
	PM_CP_CORE_POWER_CTRL,
	PM_CP_CORE_CLK_CTRL,
	PM_CP_CORE_SLEEP_CTRL,
	PM_CP_CORE_FREQ_CTRL,
	PM_CP_CORE_EXTERNAL_LDO,
	PM_CP_CORE_PSRAM_POWER,
	PM_CP_CORE_WAKEUP_SRC_CFG,
	PM_CP_CORE_RTC_WAKEUPED,
	PM_CP_CORE_GPIO_WAKEUPED,
	PM_CP_CORE_STATE_MAX
}pm_cp_core_state_e;
/*config the voltage at low vol*/
#define PM_VOLTAGE_OF_LOW_VOL            (PM_LOW_VOL_VOLTAGE_0_6)

/*config the lpo source*/
#define PM_LPO_SOURCE                    (PM_LPO_SRC_DIVD)

/*config whether auto vote*/
#define PM_APP_AUTO_VOTE_CTRL            (PM_APP_AUTO_VOTE_ENABLE)

/*=====================CONFIG  SECTION  END=======================*/

/**
 * @brief send message to pm thread for enter deepsleep or low voltage
 *
 * @attention
 * - This API is to send message to pm demo thread for enter deepsleep or low voltage.
 *
 * @param
 * -msg:message info
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
bk_err_t bk_pm_send_msg(pm_ap_core_msg_t *msg);
/**
 * @brief start pm thread
 *
 * @attention
 * - This API is start pm thread.
 *
 * @param
 * -void
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
bk_err_t pm_thread_main(void);
/**
 * @brief Register a pre-sleep callback with execution priority
 *
 * This callback will be called BEFORE the system enters sleep mode.
 * Callbacks are executed in priority order (0=first, 255=last).
 *
 * Typical use cases:
 *   - Save peripheral states
 *   - Power down devices
 *   - Flush buffers
 *
 * @param callback  Function to call before sleep (must not be NULL)
 * @param arg       User argument passed to callback (can be NULL)
 * @param priority  Execution priority: 0 (highest) to 255 (lowest)
 *                  Use predefined constants: PM_CALLBACK_PRIORITY_*
 *
 * @return  0  Success
 *         -1  Invalid callback (NULL pointer)
 *         -2  Out of memory
 *
 * Memory cost: 16 bytes per registration (32-bit ARM, aligned)
 * Time complexity: O(n) insertion, O(n) execution
 *
 * Example:
 *   void uart_pre_sleep(void *arg) {
 *       // Save UART state and power down
 *   }
 *   bk_pm_pre_sleep_callback_register(uart_pre_sleep, NULL, PM_CALLBACK_PRIORITY_HIGH);
 *
 * Priority table example:
 *   Priority 0   (CRITICAL): Clock manager, PMU
 *   Priority 50  (HIGH):     UART, SPI
 *   Priority 100 (NORMAL):   GPIO, ADC
 *   Priority 150 (LOW):      Sensors
 *   Priority 200 (LOWEST):   Debug logging
 */
bk_err_t bk_pm_pre_sleep_callback_register(sleep_callback_t callback, void *arg, uint8_t priority);

/**
 * @brief Unregister a pre-sleep callback
 *
 * @param callback  Function to remove
 *
 * @return  0  Success
 *         -1  Invalid callback (NULL pointer)
 *         -2  Callback not found
 *
 * Memory: Frees 12 bytes
 * Time complexity: O(n) where n is number of registered callbacks
 *
 * Example:
 *   bk_pm_pre_sleep_callback_unregister(uart_pre_sleep);
 */
bk_err_t bk_pm_pre_sleep_callback_unregister(sleep_callback_t callback);

/**
 * @brief Execute all pre-sleep callbacks
 *
 * This function is called automatically by the PM system before entering sleep.
 * Application modules should NOT call this directly.
 *
 * Execution order: LIFO (Last-In-First-Out)
 * - The most recently registered callback executes first
 *
 * Time complexity: O(n)
 */
bk_err_t bk_pm_pre_sleep_callback_execute(void);

/**
 * @brief Register a post-sleep (wakeup) callback with execution priority
 *
 * This callback will be called AFTER the system wakes up from sleep.
 * Callbacks are executed in priority order (0=first, 255=last).
 *
 * Typical use cases:
 *   - Restore peripheral states
 *   - Power up devices
 *   - Reconfigure clocks
 *
 * @param callback  Function to call after wakeup (must not be NULL)
 * @param arg       User argument passed to callback (can be NULL)
 * @param priority  Execution priority: 0 (highest) to 255 (lowest)
 *                  Use predefined constants: PM_CALLBACK_PRIORITY_*
 *
 * @return  0  Success
 *         -1  Invalid callback (NULL pointer)
 *         -2  Out of memory
 *
 * Memory cost: 16 bytes per registration (32-bit ARM, aligned)
 * Time complexity: O(n) insertion, O(n) execution
 *
 * Example:
 *   void uart_post_sleep(void *arg) {
 *       // Restore UART state and power up
 *   }
 *   bk_pm_post_sleep_callback_register(uart_post_sleep, NULL, PM_CALLBACK_PRIORITY_HIGH);
 */
bk_err_t bk_pm_post_sleep_callback_register(sleep_callback_t callback, void *arg, uint8_t priority);

/**
 * @brief Unregister a post-sleep callback
 *
 * @param callback  Function to remove
 *
 * @return  0  Success
 *         -1  Invalid callback (NULL pointer)
 *         -2  Callback not found
 *
 * Memory: Frees 12 bytes
 * Time complexity: O(n)
 *
 * Example:
 *   bk_pm_post_sleep_callback_unregister(uart_post_sleep);
 */
bk_err_t bk_pm_post_sleep_callback_unregister(sleep_callback_t callback);

/**
 * @brief Execute all post-sleep callbacks
 *
 * This function is called automatically by the PM system after waking up.
 * Application modules should NOT call this directly.
 *
 * Execution order: LIFO (Last-In-First-Out)
 *
 * Time complexity: O(n)
 */
bk_err_t bk_pm_post_sleep_callback_execute(void);

/**
 * @brief Register AP power control callback
 *
 * Register callback for AP power-on or power-off notification.
 * Call this API twice if you need both power-on and power-off callbacks.
 *
 * @param callback  Callback function to register
 * @param arg       User data passed to callback
 * @param type      Callback trigger type:
 *                  - PM_AP_CTRL_CB_TYPE_POWER_ON
 *                  - PM_AP_CTRL_CB_TYPE_POWER_OFF
 *
 * @return  0  Success
 *         -1  Invalid callback
 *         -2  Out of memory
 */
bk_err_t bk_pm_ap_ctrl_callback_register(ap_ctrl_callback_t callback, void *arg, pm_ap_ctrl_cb_type_t type);

/**
 * @brief Unregister AP power control callback
 *
 * @param callback  Callback function to unregister
 * @param type      Callback type used at registration
 *
 * @return  0  Success
 *         -1  Invalid callback
 *         -2  Callback not found
 */
bk_err_t bk_pm_ap_ctrl_callback_unregister(ap_ctrl_callback_t callback, pm_ap_ctrl_cb_type_t type);

/**
 * @brief Execute AP power control callbacks by type
 *
 * This function is called internally by PM flow.
 *
 * @param type  Callback type to execute
 *
 * @return BK_OK on success
 */
bk_err_t bk_pm_ap_ctrl_callback_execute(pm_ap_ctrl_cb_type_t type);

/**
 * @brief set whether AP boot completed successfully
 *
 * @param boot_success true to set PM_AP_WORK_STATE_BOOT_SUCCESS, false to clear
 * @return BK_OK on success
 */
bk_err_t bk_pm_ap_boot_success_set(bool boot_success);

/**
 * @brief get whether AP boot completed successfully
 *
 * @return true if AP boot success (PM_AP_WORK_STATE_BOOT_SUCCESS), false otherwise
 */
bool bk_pm_ap_boot_success_get(void);

/**
 * @brief set whether this is the first AP boot
 *
 * @param is_first_boot true for first boot, false otherwise
 * @return BK_OK on success
 */
bk_err_t bk_pm_ap_first_boot_set(bool is_first_boot);

/**
 * @brief get whether this is the first AP boot since cold power-on
 *
 * Mirrors the AP-side getter so CP services can branch on it. The
 * underlying flag (PM_AP_WORK_STATE_FIRST_BOOT) is asserted by
 * reset_reason.c on RESET_SOURCE_POWERON and cleared by
 * pm_module_shutdown_cpu1() after the first successful AP power-off.
 *
 * @return true if PM_AP_WORK_STATE_FIRST_BOOT is set, false otherwise
 */
bool bk_pm_ap_first_boot_get(void);

/**
 * @brief set and save wakeup source of exiting  sleep
 *
 * set and save wakeup source of exiting  sleep
 *
 * @attention
 * - This API is used to set and save wakeup source of exiting  sleep
 *
 * @param
 * -wakeup_irq:wakeup irq
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 *
 */
bk_err_t bk_pm_sleep_wakeup_reason_set(uint64_t wakeup_irq);
/**
 * @brief parse RTC wakeup reason from alarm name
 *
 * When normal sleep is woken by RTC interrupt, multiple modules (WiFi, BLE,
 * application timer, etc.) may share the same RTC interrupt source. This API
 * inspects the first triggered RTC alarm name and maps it to a specific
 * bk_pm_wakeup_reason_e value (e.g. BK_PM_WAKEUP_WIFI, BK_PM_WAKEUP_BLE,
 * BK_PM_WAKEUP_HW_TIMER).
 *
 * @attention
 * - Call this API after bk_pm_sleep_wakeup_reason_set() when the wakeup
 *   interrupt source is INT_SRC_RTC or INT_SRC_ANA_RTC.
 * - The parsed result is saved internally for subsequent wakeup reason query.
 * - MM RTC alarm is ignored because it is not a valid wakeup source.
 *
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
bk_err_t bk_pm_rtc_wakeup_reason_parse();
/**
 * @brief clear wakeup source of exiting  sleep process
 *
 * clear wakeup source of exit  sleep
 *
 * @attention
 * - This API is used to clear wakeup source of exiting  sleep process
 *
 * @param
 * -void
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 *
 */
bk_err_t bk_pm_sleep_wakeup_reason_clear();
/**
 * @brief get wakeup source of exiting  sleep
 *
 * get wakeup source of exiting  sleep
 *
 * @attention
 * - This API is used to get wakeup source of exiting  sleep
 *
 * @param
 * -void
 * @return
 * - wakeup source(refer to icu_int_src_t in int_types.h)
 */
bk_pm_wakeup_reason_e bk_pm_sleep_wakeup_reason_get();
/**
 * @brief clear deep sleep module config
 *
 * clear deep sleep module config
 *
 * @attention
 * - This API is used to clear deep sleep module config
 * - If entering DEEPSLEEP fails, do not call this interface either.
 * - Instead, proactively investigate the reasons why entering DEEPSLEEP was unsuccessful.
 * - Calling this interface indicates that you are not concerned with whether the relevant modules were closed before attempting to enter DEEPSLEEP.
 * - If the relevant modules were prematurely closed before using this method,
 * - it is highly likely that DEEPSLEEP will fail to wake up properly.
 * - Therefore, this interface is a special case for forcibly entering DEEPSLEEP,
 * - but it cannot guarantee that DEEPSLEEP will wake up normally
 * @param
 * -module:module name
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
bk_err_t bk_pm_clear_deep_sleep_modules_config(pm_power_module_name_e module_name);
/**
 * @brief Set subcores wfi state
 *
 * Set subcores wfi state
 *
 * @attention
 * - This API is used to set subcores wfi state
 *
 * @param
 * -subcores_wfi: all subcores enter wfi
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
bk_err_t bk_pm_subcores_wfi_set(bool subcores_wfi);
/**
 * @brief get the video vote power state
 *
 * get the video vote power state
 *
 * @attention
 * - This API is used to get the video vote power state
 *
 * @param
 * -void
 * @return
 * - the video vote power state(refer to the enum:power_sub_module_name_vidp_e)
 *
 */
uint32_t bk_pm_get_video_vote_pwr_state();
/**
 * @brief get the audio vote power state
 *
 * get the audio vote power state
 *
 * @attention
 * - This API is used to get the audio vote power state
 *
 * @param
 * -void
 * @return
 * - the audio vote power state(refer to the enum:power_sub_module_name_audp_e)
 *
 */
uint32_t bk_pm_get_audio_vote_pwr_state();
/**
 * @brief check whether enter low voltage sleep mode time out
 *
 * check whether enter low voltage sleep mode time out
 *
 * @attention
 * - This API is used to check whether enter low voltage sleep mode time out
 *
 * @param
 * -void
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
bk_err_t bk_pm_check_enter_lv_time_out(void);
/**
 * @brief register enter low voltage sleep mode time out callback
 *
 * register enter low voltage sleep mode time out callback
 *
 * @attention
 * - This API is used to register enter low voltage sleep mode time out callback
 *
 * @param
 * -lv_timeout_cb:enter low voltage sleep mode time out callback
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
bk_err_t bk_pm_enter_lv_time_out_register_callback(pm_enter_lv_timeout_cb_t* lv_timeout_cb);
/**
 * @brief clear the lv sleep state
 *
 * clear the lv sleep state
 *
 * @attention
 * - This API is used to clear the lv sleep state
 *
 * @param
 * -module:module name
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
bk_err_t bk_pm_module_lv_sleep_state_clear(pm_dev_id_e module);
/**
 * @brief set the lv sleep state
 *
 * set the lv sleep state
 *
 * @attention
 * - This API is used to set the lv sleep state
 *
 * @param
 * -void
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
__attribute__((section(".itcm_sec_code"))) bk_err_t bk_pm_module_lv_sleep_state_set(void);
/**
 * @brief get the lv sleep state
 *
 * get the lv sleep state
 *
 * @attention
 * - This API is used to get the lv sleep state
 *
 * @param
 * -module:module name
 * @return
 * - module_lv_sleep_state
 */
uint64_t bk_pm_module_lv_sleep_state_get(pm_dev_id_e module);
/**
 * @brief get wakeup source from deepsleep
 *
 * get wakeup source from deepsleep
 *
 * @attention
 * - This API is used to get wakeup source from deepsleep
 *
 * @param
 * -void
 * @return
 * - wakeup source(0:GPIO;1:RTC;2:WIFI;3:BT;4:USBPLUG;5:TOUCHED;6:VAD;7:NONE)
 */
pm_wakeup_source_e bk_pm_deep_sleep_wakeup_source_get(void);
/**
 * @brief clear wakeup source of exiting low voltage process
 *
 * clear wakeup source of exit low voltage
 *
 * @attention
 * - This API is used to clear wakeup source of exiting low voltage process
 *
 * @param
 * -void
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 *
 */
bk_err_t bk_pm_exit_low_vol_wakeup_source_clear(void);
/**
 * @brief set and save wakeup source of exiting low voltage
 *
 * set and save wakeup source of exiting low voltage
 *
 * @attention
 * - This API is used to set and save wakeup source of exiting low voltage
 *
 * @param
 * void
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 *
 */
bk_err_t bk_pm_exit_low_vol_wakeup_source_set(void);
/**
 * @brief get wakeup source of exiting low voltage
 *
 * get wakeup source of exiting low voltage
 *
 * @attention
 * - This API is used to get wakeup source of exiting low voltage
 *
 * @param
 * -void
 * @return
 * - wakeup source(0:GPIO;1:RTC;2:WIFI;3:BT;4:USBPLUG;5:TOUCHED;6:VAD;7:NONE)
 */
pm_wakeup_source_e bk_pm_exit_low_vol_wakeup_source_get(void);
/**
 * @brief rosc calibration
 *
 * rosc calibration
 *
 * @attention
 * - This API is used to calibrate rosc
 *
 * @param
 * -rosc_cali_mode:0x1:rosc calibrate manual; 0x0:rosc calibrate auto
 * -cali_interval: 0x0:0.25s;0x1:0.5s;0x2:0.75s;0x3:1.0s;0x4:1.25s;0x5:1.5s;0x6:1.75s;0x7:2s;
 * @return
 *  - BK_OK: succeed
 *  - others: other errors.
 */
bk_err_t bk_pm_rosc_calibration(pm_rosc_cali_mode_e rosc_cali_mode, uint32_t cali_interval);

/**
 * @brief Get the vote power module
 *
 * Get the vote power module
 *
 * @attention
 * - This API is used to get the vote power module
 * @param void
 * @return vote power module
 */
uint32_t bk_pm_vote_power_module_get();
/**
 * @brief Set the vote power module
 *
 * Set the vote power module
 *
 * @attention
 * - This API is used to set the vote power module
 * @param vote_power_module vote power module
 * @return BK_OK: success
 */
bk_err_t bk_pm_vote_power_module_set(uint32_t vote_power_module);
/**
 * @brief Set PHY re-initialization flag
 *
 * Set the RF/PHY re-initialization flag to indicate whether PHY has been re-initialized
 * after scenarios such as low-power wakeup. MAC/WiFi layer can query this flag via
 * bk_pm_phy_reinit_flag_get() to decide whether to reload RF parameters or perform
 * other follow-up operations.
 *
 * @param reinit_flag true: PHY has been re-initialized; false: not yet or not required
 * @return BK_OK: success
 */
bk_err_t bk_pm_phy_reinit_flag_set(bool reinit_flag);
/**
 * @brief set the state of phy calibration
 *
 * set the state of phy calibration
 *
 * @attention
 * - This API is used to set the state of phy calibration
 */
bk_err_t bk_pm_phy_cali_state_set(bool cali_state);
/**
 * @brief get the state of phy calibration
 *
 * get the state of phy calibration
 *
 * @attention
 * - This API is used to get the state of phy calibration
 *
 * @param
 * -void
 * @return
 *  - the state of phy calibration(True:have calibration;False:not calibration)
 */
bool bk_pm_phy_cali_state_get(void);
/**
 * @brief get the flag of phy reinit part1
 *
 * get the flag of phy reinit part1
 *
 * @attention
 * - This API is used to get the flag of phy reinit part1
 *
 * @param
 * -void
 * @return
 *  - the flag of phy reinit part1 (True:have part1;False:not do it)
 */
bool bk_pm_phy_reinit_flag_get(void);
/**
 * @brief clear the flag of phy reinit part1
 *
 * clear the flag of phy reinit part1
 *
 * @attention
 * - This API is used to clear the flag of phy reinit part1
 *
 * @param
 * -void
 * @return
 * -void
 */
void bk_pm_phy_reinit_flag_clear(void);

/**
 * @brief get the phy power feature state
 *
 * get the phy power feature state
 *
 * @attention
 * - This API is used to get the phy power manage feature state(open or close)
 *
 * @param
 * -void
 * @return
 *  - the state of the phy power manage  (0x0:close the power manage of the corresponding module;  0x1:open the power manage of the corresponding module)
 *  - ( bit0: represents BT's   pm state;
 *  -   bit1: represents WIFI's pm state;
 *  -   bit2: represents RF's   pm state)
 */
uint32_t bk_pm_phy_pm_state_get(void);

/**
 * @brief get the consume time from lowvol wakeup
 *
 * the consume time from lowvol wakeup
 *
 * @attention
 * - This API is used to get the consume time from lowvol wakeup
 *
 * @param
 * -void
 * @return
 *  - the consume time of wakeup from lowvol
 */
uint32_t bk_pm_wakeup_from_lowvol_consume_time_get(void);

#if CONFIG_PM_CP_DEEP_LV_SRAM_CHECK
void bk_pm_cp_deep_lv_sram_check_set_idle_stack(void *start, void *end);
void bk_pm_cp_deep_lv_sram_check_get_idle_stack(void **start, void **end);
#else
static inline void bk_pm_cp_deep_lv_sram_check_set_idle_stack(void *start, void *end)
{
	(void)start;
	(void)end;
}
static inline void bk_pm_cp_deep_lv_sram_check_get_idle_stack(void **start, void **end)
{
	if (start) {
		*start = (void *)0;
	}
	if (end) {
		*end = (void *)0;
	}
}
#endif

/**
 * @brief register sleep mode(low voltage and deepsleep) config
 *
 * register sleep config(include callback function and parameter)
 *
 * @attention
 * - This API is used to register low voltage and deepsleep config
 *
 * @param
 * -sleep_mode:low voltage or deepsleep mode
 * -dev_id:dev id
 * -enter_config:enter sleep config
 * -exit_config:exit sleep config
 * @return
 *  - BK_OK: succeed
 *  - others: other errors.
 */
bk_err_t bk_pm_sleep_register_cb(pm_sleep_mode_e sleep_mode,pm_dev_id_e dev_id,pm_cb_conf_t *enter_config, pm_cb_conf_t *exit_config);
/**
 * @brief unregister sleep mode(low voltage and deepsleep) config
 *
 * unregister sleep config(include callback function and parameter)
 *
 * @attention
 * - This API is used to unregister low voltage and deepsleep config
 *
 * @param
 * -sleep_mode:low voltage or deepsleep mode
 * -dev_id:dev id
 * -enter_cb:whether unregister the enter call back
 * -exit_cb:whether unregister the exit call back
 * @return
 *  - BK_OK: succeed
 *  - others: other errors.
 */
bk_err_t bk_pm_sleep_unregister_cb(pm_sleep_mode_e sleep_mode,pm_dev_id_e dev_id,bool enter_cb, bool exit_cb);
/**
 * @brief register light sleep config
 *
 * register light sleep config(include callback function and parameter)
 *
 * @attention
 * - This API is used to register light sleep config
 *
 * @param
 * -enter_config:enter light sleep config
 * -exit_config:exit light sleep config
 * @return
 *  - BK_OK: succeed
 *  - others: other errors.
 */
bk_err_t bk_pm_light_sleep_register_cb(pm_cb_conf_t *enter_config, pm_cb_conf_t *exit_config);
/**
 * @brief unregister light sleep callback
 *
 * unregister light sleep callback(enter light sleep callback and exit light sleep callback)
 *
 * @attention
 * - This API is used to unregister light sleep callback
 *
 * @param
 * -enter_cb:whether unregister the enter call back
 * -exit_cb:whether unregister the exit call back
 * @return
 *  - BK_OK: succeed
 *  - others: other errors.
 */
bk_err_t bk_pm_light_sleep_unregister_cb(bool enter_cb, bool exit_cb);
/**
 * @brief get sleep state of module
 *
 * get current sleep state of pm sleep module.
 *
 * @attention
 * - This API is used to get the sleep state
 *
 * @param
 * - module: module name
 *
 * @return
 * - sleep state: 0x0:active;0x1:sleeped;
 */
int32_t bk_pm_module_sleep_state_get(pm_sleep_module_name_e module);
/**
 * @brief get power domain of module state
 *
 * get the power domain state
 *
 * @attention
 * - This API is used to get the power domain state
 *
 * @param
 * -module:module name
 * @return
 *  -the state of power domain state
 *
 */
int32_t bk_pm_module_power_state_get(pm_power_module_name_e module);
/**
 * @brief get the cpu frequency of the module vote
 *
 * get the module voting cpu frequency
 *
 * @attention
 * - This API is used to get the module voting cpu frequency
 *
 * @param
 * -module:module id
 * @return
 *  - the cpu frequency of the module vote
 *
 */
pm_cpu_freq_e bk_pm_module_current_cpu_freq_get(pm_dev_id_e module);
/**
 * @brief get the current max and used cpu frequency
 *
 * select the cpu frequency
 *
 * @attention
 * - This API is used to get the current max and used cpu frequency
 *
 * @param
 * -void
 * @return
 *  - get the current max and used cpu frequency
 *
 */
pm_cpu_freq_e bk_pm_current_max_cpu_freq_get(void);
/**
 * @brief vote cpu frequency
 *
 * select the cpu frequency
 *
 * @attention
 * - This API is used to vote ,then select the cpu frequency
 *
 * @param
 * -module:module id;cpu_freq:cpu frequency(240M,160M,120M,26M)
 * @return
 *  - BK_OK: succeed
 *  - others: other errors.
 */
bk_err_t bk_pm_module_vote_cpu_freq(pm_dev_id_e module,pm_cpu_freq_e cpu_freq);
/**
 * @brief clock ctrl
 *
 * enable or disable dev clock
 *
 * @attention
 * - This API is used to enable or disable dev clock
 *
 * @param
 * -module:device id;clock_state:PM_CLK_CTRL_PWR_DOWN or  PM_CLK_CTRL_PWR_DOWN
 * @return
 *  - BK_OK: succeed
 *  - others: other errors.
 */
bk_err_t bk_pm_clock_ctrl(pm_dev_clk_e module,pm_dev_clk_pwr_e clock_state);
/**
 * @brief lp voltage set
 *
 * set the lp voltage
 *
 * @attention
 * - This API is used to set lp voltage when enter low voltage
 *
 * @param
 * -uint32_t:0x0:0.6v;0x1:0.7v;0x2:0.8v;0x3:0.9v;0x4:1.0v;0x5:1.1v;0x6:1.2v;0x7:1.3v;
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 *
 */
bk_err_t bk_pm_lp_vol_set(uint32_t lp_vol);

/**
 * @brief lp voltage get
 *
 * get the lpo voltage
 *
 * @attention
 * - This API is used to get  lpo voltage value
 *
 * @param
 * -void
 * @return
 * - the lp voltage value
 *
 */
uint32_t bk_pm_lp_vol_get(void);

/**
 * @brief rf tx voltage set
 *
 * set the tx voltage of RF
 *
 * @attention
 * - This API is used to set tx voltage of RF
 *
 * @param
 * -uint32_t:0x0:1.25v;0x1:1.3v;0x2:1.35v;0x3:1.4v;0x4:1.45v;0x5:1.5v;0x6:1.55v;0x7:1.6v;
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 *
 */
bk_err_t bk_pm_rf_tx_vol_set(uint32_t tx_vol);

/**
 * @brief rf tx voltage get
 *
 * get the tx voltage of RF
 *
 * @attention
 * - This API is used to get tx voltage value of RF
 *
 * @param
 * -void
 * @return
 * - the tx voltage value
 *
 */
uint32_t bk_pm_rf_tx_vol_get(void);

/**
 * @brief rf rx voltage set
 *
 * set the rx voltage of RF
 *
 * @attention
 * - This API is used to set rx voltage of RF
 *
 * @param
 * -uint32_t:0x0:1.25v;0x1:1.3v;0x2:1.35v;0x3:1.4v;0x4:1.45v;0x5:1.5v;0x6:1.55v;0x7:1.6v;
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 *
 */
bk_err_t bk_pm_rf_rx_vol_set(uint32_t rx_vol);

/**
 * @brief rf rx voltage get
 *
 * get the rx voltage of RF
 *
 * @attention
 * - This API is used to get rx voltage value of RF
 *
 * @param
 * -void
 * @return
 * - the rx voltage value
 *
 */
uint32_t bk_pm_rf_rx_vol_get(void);

/**
 * @brief lpo source set
 *
 * set the lpo source
 *
 * @attention
 * - This API is used to select 32k source when enter low voltage and deepsleep
 *
 * @param
 * -lpo_src:0x0:32K from 26m;0x1:32K from 26m;0x2:32K from ROSC;0x3:32K from ROSC
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
bk_err_t bk_pm_lpo_src_set(pm_lpo_src_e lpo_src);

/**
 * @brief lpo source get
 *
 * get the lpo source
 *
 * @attention
 * - This API is used to get 32k lpo source
 *
 * @param
 * void
 * @return
 * -lpo_src:0x0:32K from 26m;0x1:32K from 26m;0x2:32K from ROSC;0x3:32K from ROSC
 *
 */
pm_lpo_src_e bk_pm_lpo_src_get(void);
/**
 * @brief mcu pm ctrl
 *
 * enabel and disable the mcu power manage
 *
 * @attention
 * - This API is used to enabel and disable the mcu power manage
 *
 * @param
 * -power_state:0x0:enable the mcu power manage;0x1:disable the mcu power manage
 * @return
 *  - BK_OK: succeed
 *  - others: other errors.
 */
bk_err_t bk_pm_mcu_pm_ctrl(uint32_t power_state);

/**
 * @brief get the mcu power feature state
 *
 * get the mcu power feature state
 *
 * @attention
 *-This API is used to get the mcu power manage feature state(open or close)
 *
 * @param
 * none
 * @return
 *-mcu power manage state(0x0:enable the mcu power manage;0x1:disable the mcu power manage)
 *
 */
uint32_t bk_pm_mcu_pm_state_get(void);

/**
 * @brief set sleep mode
 *
 * set sleep mode.
 *
 * @attention
 * - This API set sleep mode
 *
 * @param sleep mode:0x0:NORMAL_SLEEP;0x1:LOW_VOLTAGE;0x2:DEEP_SLEEP;0x3:DEFAULT(if it meet low voltage,enter low voltage,otherwise enter normal sleep);
 *
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
bk_err_t bk_pm_sleep_mode_set(pm_sleep_mode_e sleep_mode);

/**
 * @brief set wakeup source
 *
 * set wakeup source(eg.rtc,gpio)
 *
 * @attention
 * - This API set wakeup source,wifi and bt themselves not need set wakup source when in volatage
 *
 * @param
 * -wakeup_source:wake up source
 * -source_param: the wakeup source parameter
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
bk_err_t bk_pm_wakeup_source_set(pm_wakeup_source_e wakeup_source, void* source_param);

/**
 * @brief module vote sleep ctrl
 *
 * other module tell pm module, they have entered sleep.(eg.wifi or bt enter sleep,then call the function ,tell pm it have entered sleep)
 *
 * @attention
 * - This API is used by wifi or bt... let pm module know them(wifi or bt...) enter sleep or exit sleep
 * - if all the specific module enter sleep, the mcu will enter low voltage
 *
 * @param
 * -module:module name
 * -sleep_state:0x1:enter sleep;0x0:exit sleep
 * -sleep_time: sleep time
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
bk_err_t bk_pm_module_vote_sleep_ctrl(pm_sleep_module_name_e module,uint32_t sleep_state,uint32_t sleep_time);

/**
 * @brief pm module vote power ctrl
 *
 * ther module tell pm module, they can enter power on or power off
 *
 * @attention
 *  - This API is used for the module power on and power off.
 *  - if all the specific module power off, the mcu will enter deep sleep
 * @param
 * -module:module name
 * -power_state:0x1:power off;0x0:power on
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
bk_err_t bk_pm_module_vote_power_ctrl(pm_power_module_name_e module,pm_power_module_state_e power_state);

/**
 * @brief pm vote xtal rx/tx anabuf ctrl
 *
 * pm vote xtal rx/tx anabuf ctrl
 *
 * @attention
 * - This API is used to vote whether xtal rx/tx anabuf should keep normal mode.
 * - Use PM_XTAL_RX_TX_ANABUF_ENTER_SLEEP to enter sleep-mode control.
 * - Use PM_XTAL_RX_TX_ANABUF_EXIT_SLEEP to exit sleep-mode control.
 *
 * @param
 * -module:pm_xtal_rx_tx_anabuf_module_name_e module ID (0~31). Application can
 *  cast private IDs based on PM_XTAL_RX_TX_ANABUF_MODULE_NAME_USER_BASE.
 * -sleep_state:PM_XTAL_RX_TX_ANABUF_ENTER_SLEEP;PM_XTAL_RX_TX_ANABUF_EXIT_SLEEP
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 *
 */
bk_err_t bk_pm_module_vote_xtal_rx_tx_anabuf_ctrl(pm_xtal_rx_tx_anabuf_module_name_e module, pm_xtal_rx_tx_anabuf_state_e sleep_state);

/**
 * @brief get xtal rx/tx anabuf vote state
 *
 * @return
 * - bit map of modules voting xtal rx/tx anabuf normal mode.
 */
uint32_t bk_pm_get_xtal_rx_tx_anabuf_ctrl_state(void);

/**
 * @brief pm suppress ticks and sleep
 *
 * when the rtos enter idle task and sleep time > 2*(time per tick),it will call the function enter power manager
 *
 * @attention
 * - This API is used for power manager.
 *
 * @param
 * -sleep_ticks:sleep time using tick unit
 * @return
 * - uint64_t: missed_ticks
 * missed_ticks is the number of missed ticks during the sleep time
 */
uint64_t bk_pm_suppress_ticks_and_sleep(uint32_t sleep_ticks);
/**
 * @brief enter sleep
 *
 * the function is for adapting harmony os
 *
 * @attention
 * - This API is used for power manager.
 *
 * @param
 * - void
 * @return
 * - void
 *
 */
void  bk_pm_enter_sleep(void);
/**
 * @brief sleep vote init
 *
 * the function is for init the enter low voltage sleep vote
 *
 * @attention
 * - This API is used for init the enter low voltage sleep vote
 *
 * @param
 * - void
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 *
 */
static inline bk_err_t pm_sleep_vote_init(void)
{
	uint32_t i = 0;
	pm_sleep_module_name_e enter_low_vol_modules[] = PM_ENTER_LOW_VOL_MODULES_CONFIG;

	for(i=0;i<sizeof(enter_low_vol_modules)/sizeof(pm_sleep_module_name_e);i++)
	{
		if((enter_low_vol_modules[i] == PM_SLEEP_MODULE_NAME_BTSP)||(enter_low_vol_modules[i] == PM_SLEEP_MODULE_NAME_WIFIP_MAC)||(enter_low_vol_modules[i] == PM_SLEEP_MODULE_NAME_APP))
		{
			continue;
		}
		bk_pm_module_vote_sleep_ctrl(enter_low_vol_modules[i],0x1,0);
	}

    return BK_OK;
}
/**
 * @brief 32k source switch
 *
 * When the external 32k clock source is enabled, this api used to switch the 32k clock source
 *
 * @param
 * - pm_lpo_src_e  taget 32k clock source
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 *
 */
bk_err_t pm_clk_32k_source_switch(pm_lpo_src_e lpo_src);
/**
 * @brief register callback function
 *
 * BT WIFI module registered callback function, used to switch synchronization when switching clock source
 *
 * @param
 * - pm_cb_extern32k_cfg_t   Structure pointer
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 *
 */
bk_err_t pm_extern32k_register_cb(pm_cb_extern32k_cfg_t *cfg);
/**
 * @brief unregister callback function
 *
 * @param
 * - pm_cb_extern32k_cfg_t   Structure pointer
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 *
 */
bk_err_t pm_extern32k_unregister_cb(pm_cb_extern32k_cfg_t *cfg);
/**
 * @brief register rtc callback for wifi
 *
 * @param
 * -tick:tick cnt for lv
 * -callback:callback when exit lv
 * @return
 * - void
 *
 */
void bk_pm_wifi_rtc_set(uint32_t tick, void *callback);
/**
 * @brief unregister rtc callback for wifi
 *
 * @return
 * - void
 *
 */
void bk_pm_wifi_rtc_clear(void);
/**
 * @brief pm hardware init
 *
 * @param
 * void
 * @return
 * - void
 *
 */
void pm_hardware_init();

#ifdef __cplusplus
}
#endif

#endif
