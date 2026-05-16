/**
 * @file servo.c
 * @brief Servo PWM driver + incremental palm-tracking controller.
 *
 * See servo.h for the public API and the tuning knobs (SERVO_TRACK_*).
 */

#include <common/bk_include.h>
#include <math.h>
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

/* ===================== Palm-tracking controller ===================== */
/* Last-set servo angle. Treated as "current physical angle" so each frame
 * applies an offset on top of it. Initialised to the neutral position. */
static uint32_t s_servo_angle = SERVO_CENTER_ANGLE;

void palm_track_servo(float palm_center_x, float palm_center_y,
                      uint16_t img_w, uint16_t img_h)
{
#if SERVO_TRACK_USE_CY
	const char *axis = "cy";
	float pos = palm_center_y;
	float dim = (float)img_h;
#else
	const char *axis = "cx";
	float pos = palm_center_x;
	float dim = (float)img_w;
#endif
	if (dim <= 0.0f) return;

	int prev = (int)s_servo_angle;

	/* Step 1: signed normalized offset from center.
	 *   norm < 0 : palm is on the LEFT  side of the center.
	 *   norm > 0 : palm is on the RIGHT side of the center. */
	float norm = (pos - dim * 0.5f) / dim;

	/* Dead band: palm close enough to center -> hold last angle. */
	if (fabsf(norm) < SERVO_TRACK_DEADBAND) {
		bk_printf("[track] axis=%s pos=%.1f/%u norm=%+.3f deadband, hold=%d\n",
		          axis, pos, (unsigned)dim, norm, prev);
		return;
	}

	/* Step 2: convert offset to a signed per-frame angle delta and clamp. */
	int offset = (int)lroundf((float)SERVO_TRACK_DIR * norm * SERVO_TRACK_GAIN);
	if (offset >  SERVO_TRACK_MAX_STEP) offset =  SERVO_TRACK_MAX_STEP;
	if (offset < -SERVO_TRACK_MAX_STEP) offset = -SERVO_TRACK_MAX_STEP;

	/* Step 3: apply on top of the *last* angle, clamp to mechanical limits. */
	int next = prev + offset;
	if (next < SERVO_MIN_ANGLE) next = SERVO_MIN_ANGLE;
	if (next > SERVO_MAX_ANGLE) next = SERVO_MAX_ANGLE;

	bk_printf("[track] axis=%s pos=%.1f/%u norm=%+.3f offset=%+d  %d -> %d%s\n",
	          axis, pos, (unsigned)dim, norm, offset, prev, next,
	          (next == prev) ? " (saturated)" : "");

	if ((uint32_t)next == s_servo_angle) return;

	s_servo_angle = (uint32_t)next;
	servo_set_angle(s_servo_angle);
}

