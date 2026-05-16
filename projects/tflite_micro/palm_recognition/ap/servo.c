/**
 * @file car_control.c
 * @brief UART command implementation for car and gimbal control.
 *
 * Sends fixed-length binary frames (defined in car_control.h) to the car controller over UART.
 * UART id and baud rate are set via car_control_init(&config).
 */

#include <common/bk_include.h>
#include <driver/uart.h>
#include "gpio_driver.h"
#include <driver/gpio.h>
#include "servo.h"

#define TAG "servo"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

// #define SERVO_USE_CUSTOM_GPIO
#ifdef SERVO_USE_CUSTOM_GPIO
#define SERVO_CUSTOM_GPIO_ID    GPIO_6
#define SERVO_CUSTOM_GPIO_DEV   GPIO_DEV_PWM0
#endif

uint32_t servo_angle_to_duty(uint32_t angle)
{
	if (angle > 180)
		angle = 180;
	return SERVO_PULSE_MIN + angle * (SERVO_PULSE_MAX - SERVO_PULSE_MIN) / 180;
}

static void servo_remap_gpio(pwm_chan_t chan)
{
#ifdef SERVO_USE_CUSTOM_GPIO
	(void)chan;
	gpio_dev_unmap(SERVO_CUSTOM_GPIO_ID);
	gpio_dev_map(SERVO_CUSTOM_GPIO_ID, SERVO_CUSTOM_GPIO_DEV);
	bk_gpio_pull_up(SERVO_CUSTOM_GPIO_ID);
	bk_printf("[servo] GPIO remapped: chan=%d -> GPIO_%d (dev=%d)\r\n",
			  chan, SERVO_CUSTOM_GPIO_ID, SERVO_CUSTOM_GPIO_DEV);
#else
	(void)chan;
#endif
}

void servo_init(void)
{
	pwm_init_config_t init_cfg = {0};
	pwm_chan_t chan = SERVO_CHAN;

	bk_printf("[servo] self-test start, chan=%d\r\n", chan);

	BK_LOG_ON_ERR(bk_pwm_driver_init());

	init_cfg.period_cycle = SERVO_PERIOD_CYCLE;
	init_cfg.duty_cycle = servo_angle_to_duty(0);
	init_cfg.psc = SERVO_PSC;
	BK_LOG_ON_ERR(bk_pwm_init(chan, &init_cfg));

	servo_remap_gpio(chan);

	BK_LOG_ON_ERR(bk_pwm_start(chan));

	rtos_delay_milliseconds(500);
}

void servo_set_angle(uint32_t angle)
{
	pwm_period_duty_config_t duty_cfg = {0};
	duty_cfg.period_cycle = SERVO_PERIOD_CYCLE;
	duty_cfg.psc = SERVO_PSC;
	duty_cfg.duty_cycle = servo_angle_to_duty(angle);
	BK_LOG_ON_ERR(bk_pwm_set_period_duty(SERVO_CHAN, &duty_cfg));
}

