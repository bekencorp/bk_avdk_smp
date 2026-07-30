
/**
 * @file lv_port_indev_templ.h
 *
 */

 /*Copy this file as "lv_port_indev.h" and set this value to "1" to enable content*/
#if 1

#ifndef LV_PORT_INDEV_H
#define LV_PORT_INDEV_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <common/bk_err.h>
#include "lvgl.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void lv_port_indev_init(void);

void lv_port_indev_deinit(void);

/**
 * @brief Queue one complete LVGL keypad click.
 *
 * The key value must be one of LV_KEY_* (for example LV_KEY_ENTER,
 * LV_KEY_NEXT, LV_KEY_PREV, LV_KEY_ESC, LV_KEY_UP/DOWN/LEFT/RIGHT).
 * The port layer emits a PRESSED sample followed by a RELEASED sample.
 */
bk_err_t lv_port_keypad_send_key(uint32_t key);

/**
 * @brief Queue an explicit keypad state sample.
 *
 * Use this API when a hardware driver needs to model press-and-hold by
 * sending LV_INDEV_STATE_PRESSED first and LV_INDEV_STATE_RELEASED later.
 */
bk_err_t lv_port_keypad_send_key_state(uint32_t key, lv_indev_state_t state);

/**
 * @brief Clear pending keypad samples.
 */
void lv_port_keypad_reset(void);

/**
 * @brief Return the registered LVGL keypad input device, or NULL before init.
 */
lv_indev_t *lv_port_keypad_get_indev(void);

/**
 * @brief Return the shared default group created for the keypad.
 *
 * Pages that do not manage their own group can add focusable widgets to this
 * group with lv_group_add_obj().
 */
lv_group_t *lv_port_keypad_get_default_group(void);

/**
 * @brief Bind the keypad input device to a group.
 *
 * Passing NULL restores the shared default group.
 */
bk_err_t lv_port_keypad_set_group(lv_group_t *group);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_PORT_INDEV_TEMPL_H*/

#endif /*Disable/Enable content*/
