/**
 * @file servo.h
 * @brief Servo PWM driver + incremental palm-tracking controller.
 *
 * Two layers are exposed here:
 *
 *   1. Low-level PWM driver:
 *        servo_init()              -- bring up PWM channel + initial angle
 *        servo_set_angle(angle)    -- snap servo to an absolute angle
 *
 *   2. High-level palm-tracking controller (used by detection callbacks):
 *        palm_track_servo(cx, cy, img_w, img_h)
 *      Computes a per-frame angle delta from the palm's offset relative to the
 *      image center and applies it on top of the last servo angle so the
 *      servo continuously follows the palm.
 */

#pragma once

#include <common/bk_include.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <driver/pwm.h>
#include <driver/pwm_types.h>
#include <driver/gpio.h>
#include <driver/hal/hal_gpio_types.h>
#include "gpio_driver.h"

/* ===================== Low-level PWM parameters ===================== */
#define SERVO_CLK_SRC         320000000
#define SERVO_PSC             249
#define SERVO_EFFECTIVE_CLK   (SERVO_CLK_SRC / (SERVO_PSC + 1))
#define SERVO_FREQ            50
#define SERVO_PERIOD_CYCLE    (SERVO_EFFECTIVE_CLK / SERVO_FREQ)
#define SERVO_PULSE_MIN       (SERVO_EFFECTIVE_CLK * 5 / 10000)
#define SERVO_PULSE_MAX       (SERVO_EFFECTIVE_CLK * 25 / 10000)

#define SERVO_CHAN            0

/* ===================== Servo mechanical limits ===================== */
/* Initial / neutral angle (also used as the "centered" position). */
#define SERVO_CENTER_ANGLE    90
#define SERVO_MIN_ANGLE       0
#define SERVO_MAX_ANGLE       180

/* ===================== Palm-tracking tuning knobs ===================== */
/* Gain that turns the normalized palm offset (in [-0.5, +0.5]) into a
 * per-frame angle offset. Larger -> turns more aggressively. */
#ifndef SERVO_TRACK_GAIN
#define SERVO_TRACK_GAIN      20.0f
#endif

/* Hard cap on |offset| each frame, to keep motion smooth. */
#ifndef SERVO_TRACK_MAX_STEP
#define SERVO_TRACK_MAX_STEP  5
#endif

/* Dead band on normalized palm offset; below this the palm is considered
 * already centered and the servo holds its previous angle. */
#ifndef SERVO_TRACK_DEADBAND
#define SERVO_TRACK_DEADBAND  0.06f
#endif

/* Direction sign mapping "palm offset" -> "servo angle delta".
 *   +1 : palm-on-right (norm > 0) increases the servo angle.
 *   -1 : palm-on-right decreases the servo angle (flip if servo turns the
 *        wrong way physically). */
#ifndef SERVO_TRACK_DIR
#define SERVO_TRACK_DIR       (+1)
#endif

/* Pick which model axis represents the palm's "left/right" relative to the
 * camera. With a typical pipeline (camera -> NN model -> GPU rotates for
 * display), the camera-horizontal direction is model X (cx).
 *   1 : use cy  (model Y)
 *   0 : use cx  (model X)  <-- default for this hardware */
#ifndef SERVO_TRACK_USE_CY
#define SERVO_TRACK_USE_CY    0
#endif

/* ===================== Public API ===================== */
void servo_init(void);
void servo_set_angle(uint32_t angle);

/**
 * @brief Incremental palm-tracking servo controller.
 *
 *   1. Compute the palm's signed offset from the image center
 *      (norm in [-0.5, +0.5]; negative = left/up, positive = right/down).
 *   2. Convert it into a small per-frame angle offset.
 *   3. Add the offset on top of the last servo angle and update the servo.
 *
 * Effect: the servo follows the palm. Palm on the left -> servo rotates
 * left; palm on the right -> servo rotates right.
 *
 * @param palm_center_x  Palm center X in model coordinates (e.g. 256x256).
 * @param palm_center_y  Palm center Y in model coordinates.
 * @param img_w          Model image width  (e.g. model->getWidth()).
 * @param img_h          Model image height (e.g. model->getHeight()).
 */
void palm_track_servo(float palm_center_x, float palm_center_y,
                      uint16_t img_w, uint16_t img_h);


#ifdef __cplusplus
}
#endif
