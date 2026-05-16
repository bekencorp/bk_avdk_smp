/**
 * @brief UART-based car and gimbal control module.
 *
 * Sends fixed UART command frames to control: move left/right/forward/backward, gimbal up/down.
 * UART id and baud rate are set via car_control_init(&config); pass NULL for default (UART0, 9600).
 */

#pragma once

#include <common/bk_include.h>
#include <driver/uart_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <driver/pwm.h>
#include <driver/pwm_types.h>
#include <driver/gpio.h>
#include <driver/hal/hal_gpio_types.h>
#include "gpio_driver.h"

#define SERVO_CLK_SRC         320000000
#define SERVO_PSC             249
#define SERVO_EFFECTIVE_CLK   (SERVO_CLK_SRC / (SERVO_PSC + 1))
#define SERVO_FREQ            50
#define SERVO_PERIOD_CYCLE    (SERVO_EFFECTIVE_CLK / SERVO_FREQ)
#define SERVO_PULSE_MIN       (SERVO_EFFECTIVE_CLK * 5 / 10000)
#define SERVO_PULSE_MAX       (SERVO_EFFECTIVE_CLK * 25 / 10000)

#define SERVO_CHAN            0


void servo_init(void);
void servo_set_angle(uint32_t angle);


#ifdef __cplusplus
}
#endif
