#ifndef _MULTI_BUTTON_H_
#define _MULTI_BUTTON_H_

#include "stdint.h"
#include "string.h"
#include <components/log.h>


#define BUTTON_TAG "button"
#define BUTTON_LOGI(...) BK_LOGI(BUTTON_TAG, ##__VA_ARGS__)
#define BUTTON_LOGW(...) BK_LOGW(BUTTON_TAG, ##__VA_ARGS__)
#define BUTTON_LOGE(...) BK_LOGE(BUTTON_TAG, ##__VA_ARGS__)
#define BUTTON_LOGD(...) BK_LOGD(BUTTON_TAG, ##__VA_ARGS__)
//According to your need to modify the constants.
#define TICKS_INTERVAL    6	//ms
#define DEBOUNCE_TICKS    3	//MAX 8
/*
 * SHORT_TICKS is the inter-click gap window: after a key is released the state
 * machine waits this long for the next press to arrive before committing to a
 * SINGLE_CLICK. It therefore also defines how relaxed a double click can be.
 * 96ms was far too tight for a human double click (natural gap is 200~400ms),
 * so a normal double click was usually parsed as two separate single clicks.
 * Widen the window to 300ms; the cost is that single-click reporting is delayed
 * by the same amount, which is acceptable for a dashboard UI.
 */
#ifdef CONFIG_GPIO_KEY_SHORT_PRESS_TICKS
#define SHORT_TICKS       CONFIG_GPIO_KEY_SHORT_PRESS_TICKS
#else
#define SHORT_TICKS       (300 / TICKS_INTERVAL)
#endif
#ifdef CONFIG_GPIO_KEY_LONG_PRESS_TICKS
#define LONG_TICKS        CONFIG_GPIO_KEY_LONG_PRESS_TICKS
#else
#define LONG_TICKS        100
#endif


typedef void (*btn_callback)(void *);

typedef enum {
	PRESS_DOWN = 0,
	PRESS_UP,
	PRESS_REPEAT,
	SINGLE_CLICK,
	DOUBLE_CLICK,
	LONG_PRESS_START,
	LONG_PRESS_HOLD,
	NONE_PRESS,
	LONG_PRESS_UP_EVENT,


	MAX_NUMBER_OF_EVEVT,
} PRESS_EVT;

typedef enum {
	LOW_LEVEL_TRIGGER = 0,
	HIGH_LEVEL_TRIGGER,
	ACTIVE_LEVEL_INVALID,
} ACTIVE_LEVELS_EVT;

typedef struct _button_ {
	uint16_t ticks;
	uint8_t  repeat : 4;
	uint8_t  event : 4;
	uint8_t  state : 3;
	uint8_t  debounce_cnt : 3;
	uint8_t  active_level : 1;
	uint8_t  button_level : 1;
	uint8_t  press_up_flag : 1;

	void *user_data;
	uint8_t (*hal_button_Level)(struct _button_ *);
	btn_callback  cb[MAX_NUMBER_OF_EVEVT];
	struct _button_ *next;
} BUTTON_S;

#ifdef __cplusplus
extern "C" {
#endif

void button_init(BUTTON_S *handle, uint8_t(*pin_level)(struct _button_ *), uint8_t active_level, void *user_data);
void button_attach(BUTTON_S *handle, PRESS_EVT event, btn_callback cb);
PRESS_EVT button_get_event(BUTTON_S *handle);
int  button_start(BUTTON_S *handle);
void button_stop(BUTTON_S *handle);
void button_ticks(void *param1, void *param2);
BUTTON_S *button_find_with_user_data(void *user_data);

#ifdef __cplusplus
}
#endif

#endif
