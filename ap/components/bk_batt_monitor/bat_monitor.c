#include <common/bk_include.h>
#include <common/bk_typedef.h>
#include "bk_arm_arch.h"
#include "bk_gpio.h"
#include "multi_button.h"
#include <os/os.h>
#include <os/mem.h>
#include <common/bk_kernel_err.h>
#include <driver/gpio.h>
#include <driver/hal/hal_gpio_types.h>
#include "gpio_driver.h"
#include "adc_hal.h"
#include "adc_statis.h"
#include "adc_driver.h"
#include <driver/adc.h>
#include "sys_driver.h"
#include "iot_adc.h"
#include "bk_saradc.h"
#include <bat_monitor.h>
// #include "app_event.h"

#if CONFIG_PM_ENABLE
#include <modules/pm.h>
#endif


/* SARADC sampling parameters (component-internal, not board-specific). */
#define BAT_DETECT_ONESHOT_TIMER          1
#define BAT_DETEC_ADC_CLK                 203125
#define BAT_DETEC_ADC_SAMPLE_RATE         0
#define BAT_DETEC_ADC_STEADY_CTRL         7

#define ADC_VOL_BUFFER_SIZE               (5 + 5)   /* first 5 samples are discarded */
#define ADC_READ_SEMAPHORE_WAIT_TIME      1000      /* ms */

/* Feature switches the solution can use to disable parts that the
 * physical hardware doesn't actually wire (e.g. fuel-gauge IC). */
#ifndef HARDWARE_SUPPORT_CURRENT
#define HARDWARE_SUPPORT_CURRENT          0
#endif
#ifndef HARDWARE_SUPPORT_VOLTAGE
#define HARDWARE_SUPPORT_VOLTAGE          1
#endif
#ifndef HARDWARE_SUPPORT_CHARGE_LVL
#define HARDWARE_SUPPORT_CHARGE_LVL       0
#endif
#ifndef HARDWARE_BATTERY_PRESENT
#define HARDWARE_BATTERY_PRESENT          1
#endif

/* --- Board-specific values come from Kconfig (with header fallbacks). --- */

#define BAT_MON_GPIO_CHARGE     ((gpio_id_t)CONFIG_BAT_MONITOR_GPIO_CHARGE)
#define BAT_MON_GPIO_FULL       ((gpio_id_t)CONFIG_BAT_MONITOR_GPIO_FULL)
#define BAT_MON_ADC_CHAN        ((adc_chan_t)CONFIG_BAT_MONITOR_ADC_CHAN)

#define BAT_MON_POLL_PERIOD_MS  CONFIG_BAT_MONITOR_POLL_PERIOD_MS
#define BAT_MON_LOW_PERCENT     CONFIG_BAT_MONITOR_LOW_PERCENT
#define BAT_MON_SHUTDOWN_PCT    CONFIG_BAT_MONITOR_SHUTDOWN_PERCENT
#define BAT_MON_FULL_PERCENT    CONFIG_BAT_MONITOR_FULL_PERCENT

/* Re-export with the legacy names used elsewhere in this file. */
#define BATTERY_STATE_MONITORING_PERIOD   BAT_MON_POLL_PERIOD_MS
#define SHUTDOWN_CAPACITY_THRESHOLD       BAT_MON_SHUTDOWN_PCT
#define LOW_CAPACITY_THRESHOLD            BAT_MON_LOW_PERCENT
#define FULL_CAPACITY_THRESHOLD           BAT_MON_FULL_PERCENT

/* Generic single-cell Li-ion fallback curve used when the solution
 * doesn't override battery_monitor_get_lut(). */
static const bat_lut_entry_t s_default_chargeLUT[] =
{
    {3000,   0},   /* 3.00V ->   0% */
    {3400,  10},   /* 3.40V ->  10% */
    {3450,  20},   /* 3.45V ->  20% */
    {3500,  30},   /* 3.50V ->  30% */
    {3550,  40},   /* 3.55V ->  40% */
    {3590,  50},   /* 3.59V ->  50% */
    {3650,  60},   /* 3.65V ->  60% */
    {3750,  70},   /* 3.75V ->  70% */
    {3880,  80},   /* 3.88V ->  80% */
    {3980,  90},   /* 3.98V ->  90% */
    {4100,  99},   /* 4.10V ->  99% */
};

/* Weak default — solutions can re-implement this in their own .c to
 * supply a battery-specific discharge curve. */
__attribute__((weak))
const bat_lut_entry_t * battery_monitor_get_lut(size_t *pCount)
{
    if (pCount) {
        *pCount = sizeof(s_default_chargeLUT) / sizeof(s_default_chargeLUT[0]);
    }
    return s_default_chargeLUT;
}

#if CONFIG_BAT_MONITOR

static bool s_charging_init_status_flag = false;
static beken_thread_t battery_monitor_thread_hdl = NULL;
static IotBatteryHandle_t xGlobalHandle = NULL;
IotBatteryDescriptor_t gxBatteryDescriptor[BATTERY_MAX_INSTANCE] = { 0 };

static uint16_t * s_raw_voltage_data = NULL;

static uint16_t  prvCalculateVoltage( void );
static bk_err_t  prvStartBatteryAdcOneTime( uint16_t * vol );
static void      prvCheckChargeStatus( IotBatteryHandle_t xHandle );
static void      prvBatteryMonitorTaskMain( void );
static bk_err_t  prvBatteryMonitorTaskInit( void );

static battery_event_callback_t s_battery_event_callback = NULL;
int battery_event_callback_register(battery_event_callback_t callback)
{
	s_battery_event_callback = callback;
	return 0;
}

int32_t battery_get_voltage(uint16_t *pVoltage)
{
    if (xGlobalHandle == NULL || pVoltage == NULL)
    {
        return IOT_BATTERY_INVALID_VALUE;
    }

    return iot_battery_voltage(xGlobalHandle, pVoltage);
}

int32_t battery_get_current(uint16_t *pCurrent)
{
    if (xGlobalHandle == NULL || pCurrent == NULL)
    {
        return IOT_BATTERY_INVALID_VALUE;
    }

    return iot_battery_current(xGlobalHandle, pCurrent);
}

int32_t battery_get_charge_level(uint8_t *pLevel)
{
    if (xGlobalHandle == NULL || pLevel == NULL)
    {
        return IOT_BATTERY_INVALID_VALUE;
    }

    return iot_battery_chargeLevel(xGlobalHandle, pLevel);
}

/* Normalise the two charging-detect GPIOs to a polarity-independent
 * pair of booleans:
 *   bExternalPower : true when external power (VBUS) is present.
 *   bChargingNow   : true when the charger reports "still charging"
 *                    (i.e. NOT in standby/full state).
 * The actual electrical polarity of the two pins is controlled by
 * CONFIG_BAT_MONITOR_CHARGE_ACTIVE_HIGH and
 * CONFIG_BAT_MONITOR_FULL_ACTIVE_HIGH so the component itself stays
 * board-agnostic. */
static inline void prvReadChargeGpios(bool *pbExternalPower, bool *pbChargingNow)
{
    int chg = bk_gpio_get_input(BAT_MON_GPIO_CHARGE);
    int ful = bk_gpio_get_input(BAT_MON_GPIO_FULL);

#if CONFIG_BAT_MONITOR_CHARGE_ACTIVE_HIGH
    *pbExternalPower = (chg == 1);
#else
    *pbExternalPower = (chg == 0);
#endif

#if CONFIG_BAT_MONITOR_FULL_ACTIVE_HIGH
    *pbChargingNow   = (ful == 1);
#else
    *pbChargingNow   = (ful == 0);
#endif
}

static inline IotBatteryStatus_t battery_get_status_from_gpio(void)
{
    if(!xGlobalHandle)
    {
        return eBatteryUnknown;
    }

    bool bExternalPower = false;
    bool bChargingNow   = false;
    prvReadChargeGpios(&bExternalPower, &bChargingNow);

    if (bExternalPower)
    {
        return bChargingNow ? eBatteryCharging : eBatteryChargeFull;
    }
    return eBatteryDischarging;
}

bool battery_if_is_charging(void)
{
    if (!xGlobalHandle)
    {
        return false;
    }

    IotBatteryStatus_t status = battery_get_status_from_gpio();
    return (status == eBatteryCharging);
}

IotBatteryInfo_t * battery_if_get_info(void)
{
    if (!xGlobalHandle)
    {
        return NULL;
    }
    return iot_battery_getInfo(xGlobalHandle);
}

static int hardware_read_voltage( uint16_t * pusVoltage )
{
    if( !HARDWARE_SUPPORT_VOLTAGE )
    {
        return -1;
    }
    /* for test:3800mV */
    //*pusVoltage = 3800;

	if (pusVoltage == NULL) {
        BAT_MONITOR_WPRT("Error: pusVoltage pointer is NULL\r\n");
        return -1;
    }

	bk_err_t ret = prvStartBatteryAdcOneTime(pusVoltage);
	if (ret != BK_OK) {
		return -1;
	}

	/*
	 * pusVoltage is the voltage at the ADC pin in mV (after
	 * prvStartBatteryAdcOneTime applies bk_adc_data_calculate()).
	 *
	 * Convert to the true VBAT using the external divider configured by
	 * the solution:
	 *   VBAT = V_pin * CONFIG_BAT_MONITOR_ADC_DIVIDER_X100 / 100
	 *
	 * If the divider-compensated VBAT exceeds CONFIG_BAT_MONITOR_VBAT_SANITY_MAX_MV
	 * (a value impossible for a healthy single-cell Li-ion), fall back
	 * to V_pin directly — this handles boards where the divider has
	 * been DNP'd / shorted at production.  Finally hard-clamp to
	 * CONFIG_BAT_MONITOR_VBAT_HARD_CLAMP_MV so callers never see garbage.
	 */
	uint16_t mv_at_pin = *pusVoltage;
	uint32_t vbat = (uint32_t)mv_at_pin * (uint32_t)CONFIG_BAT_MONITOR_ADC_DIVIDER_X100 / 100U;
	if (vbat > (uint32_t)CONFIG_BAT_MONITOR_VBAT_SANITY_MAX_MV) {
		vbat = mv_at_pin;
	}
	if (vbat > (uint32_t)CONFIG_BAT_MONITOR_VBAT_HARD_CLAMP_MV) {
		vbat = (uint32_t)CONFIG_BAT_MONITOR_VBAT_HARD_CLAMP_MV;
	}
	*pusVoltage = (uint16_t)vbat;
	return 0;
}

static int hardware_read_current( uint16_t * pusCurrent )
{
    if( !HARDWARE_SUPPORT_CURRENT )
    {
        return -1;
    }
    /* for test: 500mA */
    *pusCurrent = 500;

    /*TODO: user needs to implement it themselves  */
    return 0;
}

static int hardware_read_charge_level( uint8_t * pucChargeLevel )
{
    if( !HARDWARE_SUPPORT_CHARGE_LVL )
    {
        return -1;
    }
    /* for test: 50% */
    *pucChargeLevel = 50;

    /*TODO: user needs to implement it themselves  */
    return 0;
}

static bool hardware_battery_present( void )
{
    return (HARDWARE_BATTERY_PRESENT != 0);
}


/**
 * @brief Open and initialize battery/power management system
 */
IotBatteryHandle_t iot_battery_open( int32_t lBatteryInstance )
{

    if( lBatteryInstance < 0 || lBatteryInstance >= BATTERY_MAX_INSTANCE )
    {
        return NULL;
    }

    IotBatteryDescriptor_t * pxDesc = &gxBatteryDescriptor[lBatteryInstance];
    if( pxDesc->bIsOpen )
    {
        return NULL;
    }

    os_memset( pxDesc, 0, sizeof(*pxDesc) );
    pxDesc->bIsOpen = true;
    pxDesc->lInstance = lBatteryInstance;

    /* Simulate to determine if hardware has a battery */
    pxDesc->bBatteryPresent = hardware_battery_present();

    /* Set default battery information */
    pxDesc->xBatteryInfo.xBatteryType     = eBatteryChargeable;
    pxDesc->xBatteryInfo.usMinVoltage     = CONFIG_BAT_MONITOR_BAT_MIN_MV;
    pxDesc->xBatteryInfo.usMaxVoltage     = CONFIG_BAT_MONITOR_BAT_MAX_MV;
    pxDesc->xBatteryInfo.sMinTemperature  = 0;
    pxDesc->xBatteryInfo.lMaxTemperature  = 50;
    pxDesc->xBatteryInfo.usMaxCapacity    = 100;    /* Calculate based on 100% */
    pxDesc->xBatteryInfo.ucAsyncSupported = 1;      /* support for asynchronous Initialize measurement data */

    /* Initialize measurement data */
    pxDesc->usCurrentVoltage = 0;
    pxDesc->usCurrent        = 0;
    pxDesc->ucChargeLevel    = 0;
	pxDesc->xBatteryInfo.xBatteryStatus    = eBatteryUnknown;

    return (IotBatteryHandle_t) pxDesc;
}



/**
 * @brief Get battery information pointer
 */
IotBatteryInfo_t * iot_battery_getInfo( IotBatteryHandle_t const pxBatteryHandle )
{
    if( pxBatteryHandle == NULL )
    {
        return NULL;
    }

    IotBatteryDescriptor_t * pxDesc = (IotBatteryDescriptor_t *) pxBatteryHandle;
    if( !pxDesc->bIsOpen )
    {
        return NULL;
    }

    return &( pxDesc->xBatteryInfo );
}

/**
 * @brief Get battery current (mA)
 */
int32_t iot_battery_current( IotBatteryHandle_t const pxBatteryHandle,
                             uint16_t * pusCurrent )
{
    if( ( pxBatteryHandle == NULL ) || ( pusCurrent == NULL ) )
    {
        return IOT_BATTERY_INVALID_VALUE;
    }

    IotBatteryDescriptor_t * pxDesc = (IotBatteryDescriptor_t *) pxBatteryHandle;
    if( !pxDesc->bIsOpen )
    {
        return IOT_BATTERY_INVALID_VALUE;
    }

    if( !pxDesc->bBatteryPresent )
    {
        return IOT_BATTERY_NOT_EXIST;
    }

    if( !HARDWARE_SUPPORT_CURRENT )
    {
        return IOT_BATTERY_FUNCTION_NOT_SUPPORTED;
    }

    if( hardware_read_current( pusCurrent ) < 0 )
    {
        return IOT_BATTERY_READ_FAILED;
    }

    pxDesc->usCurrent = *pusCurrent;
    return IOT_BATTERY_SUCCESS;
}

/**
 * @brief Get battery voltage (mV)
 */
int32_t iot_battery_voltage( IotBatteryHandle_t const pxBatteryHandle,
                             uint16_t * pusVoltage )
{
    if( ( pxBatteryHandle == NULL ) || ( pusVoltage == NULL ) )
    {
        return IOT_BATTERY_INVALID_VALUE;
    }

    IotBatteryDescriptor_t * pxDesc = (IotBatteryDescriptor_t *) pxBatteryHandle;
    if( !pxDesc->bIsOpen )
    {
        return IOT_BATTERY_INVALID_VALUE;
    }

    if( !pxDesc->bBatteryPresent )
    {
        return IOT_BATTERY_NOT_EXIST;
    }

    if( !HARDWARE_SUPPORT_VOLTAGE )
    {
        return IOT_BATTERY_FUNCTION_NOT_SUPPORTED;
    }

    if( hardware_read_voltage( pusVoltage ) < 0 )
    {
        return IOT_BATTERY_READ_FAILED;
    }

    /* hardware_read_voltage() already returns VBAT in mV
     * (SDK calibration + R16/R13 divider compensation applied inside). */
    pxDesc->usCurrentVoltage = *pusVoltage;

    return IOT_BATTERY_SUCCESS;
}

static uint8_t battery_voltage_to_percent(uint16_t voltageMV)
{
    size_t lut_size = 0;
    const bat_lut_entry_t *lut = battery_monitor_get_lut(&lut_size);

    if (lut == NULL || lut_size == 0) {
        return 0;
    }

    /* If it is below the minimum value, directly return the minimum percentage in the table*/
    if(voltageMV <= lut[0].voltageMV)
    {
        return lut[0].percent;
    }

    /* If the value exceeds the maximum value, return the maximum percentage */
    if(voltageMV >= lut[lut_size - 1].voltageMV)
    {
        return lut[lut_size - 1].percent;
    }

    /* Perform linear interpolation within the interval */
    for(size_t i = 0; i < lut_size - 1; i++)
    {
        uint16_t v1 = lut[i].voltageMV;
        uint16_t v2 = lut[i+1].voltageMV;

        if(voltageMV >= v1 && voltageMV <= v2)
        {
            uint8_t p1 = lut[i].percent;
            uint8_t p2 = lut[i+1].percent;

            uint16_t dist  = (v2 - v1);
            uint16_t delta = (voltageMV - v1);

            /* ratio: 0.0 ~ 1.0 */
            float ratio = (float)delta / (float)dist;
            float pf    = p1 + ratio * (p2 - p1);

            /* Round to the nearest integer */
            if(pf < 0)   pf = 0;
            if(pf > 100) pf = 100;

            return (uint8_t)(pf + 0.5f);
        }
    }

    /* According to theory, it wouldn't be here for safety. */
    return lut[lut_size - 1].percent;
}

/**
 * @brief Get battery remaining charge (%)，range 1~100
 */
int32_t iot_battery_chargeLevel( IotBatteryHandle_t const pxBatteryHandle,
                                 uint8_t * pucChargeLevel )
{
    if( ( pxBatteryHandle == NULL ) || ( pucChargeLevel == NULL ) )
    {
        return IOT_BATTERY_INVALID_VALUE;
    }

    IotBatteryDescriptor_t * pxDesc = (IotBatteryDescriptor_t *) pxBatteryHandle;
    if( !pxDesc->bIsOpen )
    {
        return IOT_BATTERY_INVALID_VALUE;
    }

    if( !pxDesc->bBatteryPresent )
    {
        return IOT_BATTERY_NOT_EXIST;
    }

    if (HARDWARE_SUPPORT_CHARGE_LVL)
    {
        if (hardware_read_charge_level( pucChargeLevel ) < 0 )
        {
            return IOT_BATTERY_READ_FAILED;
        }
        pxDesc->ucChargeLevel = *pucChargeLevel;
        return IOT_BATTERY_SUCCESS;
    }
    else if (HARDWARE_SUPPORT_VOLTAGE)
    {
        // Using voltage interpolation
        /* get vlotage ( mV ) */
        uint16_t voltageMV = 0;
        int32_t ret = iot_battery_voltage(pxBatteryHandle, &voltageMV);
        if(ret != IOT_BATTERY_SUCCESS)
        {
            return ret;
        }

        /* Call the interpolation function to convert voltageMV to percentage. */
        uint8_t batteryPercent = battery_voltage_to_percent(voltageMV);

        IotBatteryDescriptor_t *pxDesc = (IotBatteryDescriptor_t *) pxBatteryHandle;
        if (pxDesc->xBatteryInfo.xBatteryStatus == eBatteryCharging) {
            bool isReallyFull = (pxDesc->xBatteryInfo.xBatteryStatus == eBatteryChargeFull);
            if (!isReallyFull) {
                if (batteryPercent >= 99) {
                    batteryPercent = 99;
                }
            }
        }

        if (pxDesc->xBatteryInfo.xBatteryStatus == eBatteryChargeFull) {
            batteryPercent = 100;
        }

        /* update chargeLevel */
        pxDesc->ucChargeLevel = batteryPercent;

        *pucChargeLevel = batteryPercent;

        return IOT_BATTERY_SUCCESS;
    }
    else
    {
        return IOT_BATTERY_FUNCTION_NOT_SUPPORTED;
    }

}


/*
 * Calculate the average of the filtered list
 * Skip the first 5 samples，Filter out any values that are 0 or 2048
 */
static uint16_t prvCalculateVoltage( void )
{
    if( s_raw_voltage_data == NULL )
    {
        BAT_MONITOR_WPRT("s_raw_voltage_data is NULL.\r\n");
        return 0;
    }

    uint32_t sum   = 0;
    uint32_t count = 0;

    for( uint32_t i = 5; i < ADC_VOL_BUFFER_SIZE; i++ )
    {
        if( ( s_raw_voltage_data[i] != 0 ) &&
            ( s_raw_voltage_data[i] != 2048 ) )
        {
            sum += s_raw_voltage_data[i];
            count++;
        }
    }

    if( count == 0 )
    {
        s_raw_voltage_data[0] = 0;
    }
    else
    {
        s_raw_voltage_data[0] = (uint16_t)( sum / count );
    }

    return s_raw_voltage_data[0];
}

/*
 * Simultaneous sampling and calculation of voltage
 */
static bk_err_t prvStartBatteryAdcOneTime( uint16_t * vol )
{
	if (vol == NULL) {
        BAT_MONITOR_WPRT("Error: vol pointer is NULL\r\n");
        return BK_FAIL;
    }

    BK_LOG_ON_ERR( bk_adc_acquire() );
    /* GPIO-to-analog remap is done once in battery_monitor_init(); calling it
     * every cycle floods the log with harmless but noisy "gpio_dev_unprotect_map"
     * errors. */
    BK_LOG_ON_ERR( bk_adc_init( BAT_MON_ADC_CHAN ) );

    adc_config_t config;
    os_memset( &config, 0, sizeof(config) );

    config.chan          = BAT_MON_ADC_CHAN;
    config.adc_mode      = ADC_CONTINUOUS_MODE;
    config.src_clk       = ADC_SCLK_XTAL;
    config.clk           = BAT_DETEC_ADC_CLK;
    config.saturate_mode = ADC_SATURATE_MODE_3;
    config.steady_ctrl   = BAT_DETEC_ADC_STEADY_CTRL;
    config.adc_filter    = 0;
    config.sample_rate   = BAT_DETEC_ADC_SAMPLE_RATE;

    if( config.adc_mode == ADC_CONTINUOUS_MODE )
    {
        config.sample_rate = 0;
    }

    BK_LOG_ON_ERR( bk_adc_set_config( &config ) );
    BK_LOG_ON_ERR( bk_adc_enable_bypass_clalibration() );
    BK_LOG_ON_ERR( bk_adc_start() );

    bk_err_t ret = bk_adc_read_raw( s_raw_voltage_data,
                                    ADC_VOL_BUFFER_SIZE,
                                    ADC_READ_SEMAPHORE_WAIT_TIME );
    if( ret != BK_OK )
    {
        BAT_MONITOR_WPRT("Failed to read ADC data, err: %d\r\n", ret );
		*vol = 0;
        goto ADC_EXIT;
    }

    *vol = prvCalculateVoltage();
    //BAT_MONITOR_PRT("ADC VALUE: %d .\r\n", *vol);

ADC_EXIT:
    BK_LOG_ON_ERR( bk_adc_stop() );
    BK_LOG_ON_ERR( bk_adc_deinit( BAT_MON_ADC_CHAN ) );
    BK_LOG_ON_ERR( bk_adc_release() );

    /*
     * Convert raw SARADC value to mV at the ADC pin using the SDK helper
     * (which knows about per-channel cwt calibration and Vref).  The driver's
     * original linear "raw * 0.667 + 40" formula was tuned for a different
     * board and produced 10000+ mV on this hardware (raw values are ~16 bit
     * here, not 12 bit).  After this conversion we still need to apply the
     * external R16(3.3M)/R13(1M) divider in iot_battery_voltage().
     */
    if( ret == BK_OK && *vol != 0 )
    {
        uint16_t mv_at_pin = bk_adc_data_calculate( *vol, BAT_MON_ADC_CHAN );
        /* Rate-limit to once per ~10 reads so the log isn't flooded when the
         * page_2 timer is polling at 1 Hz. */
        static uint32_t s_log_div;
        if ((s_log_div++ % 10) == 0) {
            BAT_MONITOR_PRT("bat raw=%u, mv@pin=%u\r\n", (unsigned)(*vol), mv_at_pin);
        }
        *vol = mv_at_pin;
    }

	return ret;
}

/*
 * Check the GPIO pin to determine whether it is charging, fully charged, or without an external power source
 */
static void prvCheckChargeStatus( IotBatteryHandle_t xHandle )
{

    IotBatteryDescriptor_t * pxDesc = (IotBatteryDescriptor_t *) xHandle;
    if(pxDesc == NULL)
    {
        BAT_MONITOR_WPRT("Invalid battery handle in prvCheckChargeStatus\r\n");
        return;
    }

    bool bExternalPower = false;
    bool bChargingNow   = false;
    prvReadChargeGpios(&bExternalPower, &bChargingNow);

    if( bExternalPower )
    {
        if( bChargingNow )
        {
            pxDesc->xBatteryInfo.xBatteryStatus = eBatteryCharging;
            BAT_MONITOR_PRT("Device is charging...\r\n");
        }
        else
        {
            pxDesc->xBatteryInfo.xBatteryStatus = eBatteryChargeFull;
            BAT_MONITOR_PRT("Battery is full.\r\n");
        }
    }
    else
    {
        pxDesc->xBatteryInfo.xBatteryStatus = eBatteryDischarging;
        BAT_MONITOR_PRT("Battery powered.\r\n");
    }

}

int32_t iot_battery_close(IotBatteryHandle_t pxBatteryHandle)
{
    if(pxBatteryHandle == NULL)
    {
        return IOT_BATTERY_INVALID_VALUE;
    }

    IotBatteryDescriptor_t *pxDesc = (IotBatteryDescriptor_t *)pxBatteryHandle;

    if(!pxDesc->bIsOpen)
    {
        // Already closed or not opened
        return IOT_BATTERY_INVALID_VALUE;
    }

    pxDesc->bIsOpen = false;
    return IOT_BATTERY_SUCCESS;
}


/*
 * Monitoring Thread
 */
static void prvBatteryMonitorTaskMain( void )
{
    static bool bLowVoltageTriggered = false;  // Low Battery Status Indicator
    static bool bShutdownTriggered = false;

    xGlobalHandle = iot_battery_open( 0 );
    if( xGlobalHandle == NULL )
    {
        BAT_MONITOR_WPRT("Failed to open battery driver!\r\n");
        goto TASK_EXIT;
    }

    /* Get basic information and print it */
    IotBatteryInfo_t * pxInfo = iot_battery_getInfo( xGlobalHandle );

    if( pxInfo )
    {
        BAT_MONITOR_PRT("Battery info: Type=%d, MinVolt=%d, MaxVolt=%d\r\n",
             pxInfo->xBatteryType,
             pxInfo->usMinVoltage,
             pxInfo->usMaxVoltage );
    }

    /* Every BATTERY_STATE_MONITORING_PERIOD, check the charging state, sample the voltage, evaluate the state */
    while( s_charging_init_status_flag )
    {
        /* Check charging status */
        prvCheckChargeStatus( xGlobalHandle );
        if (pxInfo->xBatteryStatus == eBatteryCharging)
        {
            if (s_battery_event_callback) {
                s_battery_event_callback(EVT_BATTERY_CHARGING);
            }
            bLowVoltageTriggered = false;
            bShutdownTriggered = false;
        }

        {
            uint16_t usVoltage   = 0;
            uint16_t usCurrent   = 0;
            uint8_t  ucCharge    = 0;

            if( iot_battery_voltage( xGlobalHandle, &usVoltage ) == IOT_BATTERY_SUCCESS )
            {
                if(pxInfo->xBatteryStatus == eBatteryCharging)
                    BAT_MONITOR_PRT("Supply voltage: %u mV\r\n", usVoltage);
                else
                    BAT_MONITOR_PRT("Battery voltage: %u mV\r\n", usVoltage);
            }
            if( iot_battery_current( xGlobalHandle, &usCurrent ) == IOT_BATTERY_SUCCESS )
            {
                BAT_MONITOR_PRT("Battery current: %u mA\r\n", usCurrent);
            }
            if( iot_battery_chargeLevel( xGlobalHandle, &ucCharge ) == IOT_BATTERY_SUCCESS )
            {
                /* Low battery detection logic */
                if ((ucCharge <= SHUTDOWN_CAPACITY_THRESHOLD) && (pxInfo->xBatteryStatus != eBatteryCharging))
                {
                    if (!bShutdownTriggered)
                    {
                        if (s_battery_event_callback) {
                            s_battery_event_callback(EVT_SHUTDOWN_LOW_BATTERY);
                        }
                        BAT_MONITOR_WPRT("Shutdown due to critical battery level!\r\n");
                        bShutdownTriggered = true;

                        // if you want to shutdown immdiately,can runnning this fake function here：
                        // system_shutdown();
                    }
                }
                else if ((ucCharge <= LOW_CAPACITY_THRESHOLD) && (pxInfo->xBatteryStatus != eBatteryCharging))
                {
                    if (!bLowVoltageTriggered)
                    {
                        if (s_battery_event_callback) {
                            s_battery_event_callback(EVT_BATTERY_LOW_VOLTAGE);
                        }
                        BAT_MONITOR_WPRT("Low voltage event triggered!\r\n");
                        bLowVoltageTriggered = true;
                    }
                    bShutdownTriggered = false;
                }
                else
                {
                    bLowVoltageTriggered = false;  // When charging resumes, reset the flag
                    bShutdownTriggered = false;
                }
                if(pxInfo->xBatteryStatus != eBatteryCharging)
                    BAT_MONITOR_PRT("Battery level: %u%%\r\n", ucCharge);
            }
        }

        rtos_delay_milliseconds( BATTERY_STATE_MONITORING_PERIOD );
    }

TASK_EXIT:

    if (xGlobalHandle) {
        iot_battery_close(xGlobalHandle);
        xGlobalHandle = NULL;
    }

    if( s_raw_voltage_data )
    {
        os_free( s_raw_voltage_data );
        s_raw_voltage_data = NULL;
    }

    battery_monitor_thread_hdl = NULL;
    rtos_delete_thread( NULL );
}

/**
 * @brief Create a battery monitoring thread and allocate a buffer
 */

static bk_err_t prvBatteryMonitorTaskInit( void )
{
    if( battery_monitor_thread_hdl != NULL )
    {
        BAT_MONITOR_PRT("Battery monitor task already running.\r\n");
        return BK_OK;
    }

    s_raw_voltage_data = (uint16_t *) os_malloc( ADC_VOL_BUFFER_SIZE * sizeof(uint16_t) );
    if( s_raw_voltage_data == NULL )
    {
        BAT_MONITOR_WPRT("Failed to allocate memory for s_raw_voltage_data\r\n");
        return BK_ERR_NO_MEM;
    }

#if CONFIG_PSRAM_AS_SYS_MEMORY
    bk_err_t ret = rtos_create_psram_thread( &battery_monitor_thread_hdl,
                                       4,
                                       "battery_monitor",
                                       (beken_thread_function_t)prvBatteryMonitorTaskMain,
                                       1536,
                                       (beken_thread_arg_t)NULL );
#else
    bk_err_t ret = rtos_create_thread( &battery_monitor_thread_hdl,
                                       4,
                                       "battery_monitor",
                                       (beken_thread_function_t)prvBatteryMonitorTaskMain,
                                       1536,
                                       (beken_thread_arg_t)NULL );
#endif

    if( ret != BK_OK )
    {
        battery_monitor_thread_hdl = NULL;
        os_free( s_raw_voltage_data );
        s_raw_voltage_data = NULL;
        BAT_MONITOR_WPRT("Failed to create battery_monitor task, err=%d\r\n", ret );
        return BK_ERR_NOT_INIT;
    }

    return BK_OK;
}

static void prvInitChargeGpios(void)
{
    /*
     * 5V_DET / FULL_DET are configured as digital inputs. The pull-up
     * policy is selectable from Kconfig:
     *   - external pull-up on the board   -> CONFIG_BAT_MONITOR_FULL_PULL_UP=n
     *   - rely on the MCU's internal pull -> CONFIG_BAT_MONITOR_FULL_PULL_UP=y
     */
    gpio_config_t cfg_chg = {
        .io_mode   = GPIO_INPUT_ENABLE,
        .pull_mode = GPIO_PULL_DISABLE,
        .func_mode = GPIO_SECOND_FUNC_DISABLE,
    };
    BK_LOG_ON_ERR(bk_gpio_set_config(BAT_MON_GPIO_CHARGE, &cfg_chg));

    gpio_config_t cfg_full = {
        .io_mode   = GPIO_INPUT_ENABLE,
#if CONFIG_BAT_MONITOR_FULL_PULL_UP
        .pull_mode = GPIO_PULL_UP_EN,
#else
        .pull_mode = GPIO_PULL_DISABLE,
#endif
        .func_mode = GPIO_SECOND_FUNC_DISABLE,
    };
    BK_LOG_ON_ERR(bk_gpio_set_config(BAT_MON_GPIO_FULL, &cfg_full));
}

static void prvInitBatteryAdcPin(void)
{
    /* Remap the BAT_ADC pin from default GPIO to analog input.
     * This is a one-shot operation; doing it on every ADC read causes the
     * gpio_dev_unprotect_map / "GPIO device N not supported" warnings to
     * flood the log. */
    BK_LOG_ON_ERR( bk_adc_chan_init_gpio( BAT_MON_ADC_CHAN ) );
}

void battery_monitor_init( void )
{
    if( s_charging_init_status_flag )
    {
        BAT_MONITOR_PRT("Battery monitor has already been initialized.\n");
        return;
    }

    prvInitChargeGpios();
    prvInitBatteryAdcPin();

    bk_err_t ret = prvBatteryMonitorTaskInit();
    if( ret != BK_OK )
    {
        BAT_MONITOR_PRT("Battery monitor task create failed!\n");
        return;
    }

    s_charging_init_status_flag = true;
    BAT_MONITOR_PRT("Battery monitor initialized.\n");
}

void battery_monitor_deinit(void)
{
    if (!s_charging_init_status_flag) {
        BAT_MONITOR_PRT("Battery monitor already deinitialized.\n");
        return;
    }

    s_charging_init_status_flag = false;

    if (battery_monitor_thread_hdl) {
        rtos_delete_thread(&battery_monitor_thread_hdl);
        battery_monitor_thread_hdl = NULL;
    }

    if (xGlobalHandle) {
        iot_battery_close(xGlobalHandle);
        xGlobalHandle = NULL;
    }

    if (s_raw_voltage_data) {
        os_free(s_raw_voltage_data);
        s_raw_voltage_data = NULL;
    }

    BAT_MONITOR_PRT("Battery monitor deinitialized.\n");
}

#endif /* CONFIG_BAT_MONITOR */
