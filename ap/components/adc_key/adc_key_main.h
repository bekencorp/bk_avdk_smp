// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <stdbool.h>
#include "bk_gpio.h"
#include <driver/gpio.h>
#include "gpio_driver.h"
#include "bk_saradc.h"
#include <driver/adc.h>
#include "adc_statis.h"
#include <driver/hal/hal_gpio_types.h>
#include <driver/hal/hal_adc_types.h>

#ifdef __cplusplus
extern "C" {
#endif
#define ADC_KEY_TAG "adc_key"

#define ADC_KEY_LOGI(...) BK_LOGI(ADC_KEY_TAG, ##__VA_ARGS__)
#define ADC_KEY_LOGW(...) BK_LOGW(ADC_KEY_TAG, ##__VA_ARGS__)
#define ADC_KEY_LOGE(...) BK_LOGE(ADC_KEY_TAG, ##__VA_ARGS__)
#define ADC_KEY_LOGD(...) BK_LOGD(ADC_KEY_TAG, ##__VA_ARGS__)
#define ADC_KEY_LOGV(...) BK_LOGV(ADC_KEY_TAG, ##__VA_ARGS__)

/* KEY2: ADC key pin and channel (configurable via Kconfig) */
#define ADC_KEY2_GPIO_ID      CONFIG_ADC_KEY2_GPIO
#define ADC_KEY2_SADC_CHAN_ID  CONFIG_ADC_KEY2_ADC_CHAN

/* KEY1: GPIO-only key pin (configurable via Kconfig) */
#define GPIO_KEY1_GPIO_ID     CONFIG_GPIO_KEY1_PIN
#define GPIO_KEY1_ACTIVE_LEVEL  CONFIG_GPIO_KEY1_ACTIVE_LEVEL

#if CONFIG_ADC_KEY_DUAL_CHANNEL
/* Dual ADC channel mode (configurable via Kconfig) */
#define ADC_KEY1_GPIO_ID      CONFIG_ADC_KEY1_GPIO
#define ADC_KEY1_SADC_CHAN_ID  CONFIG_ADC_KEY1_ADC_CHAN
#endif

/*
 * ADC key timing aligned with GPIO key (multi_button) for comparable
 * short-press latency. CP sampler clamps period to >= 20 ms
 * (ADC_KEY_SAMPLER_PERIOD_MS_MIN in cp/.../saradc_server.c), so 20 ms
 * is the practical lower bound on AP side as well. If ADC noise causes
 * state-machine bounce after this tuning, raise ADCKEY_DEBOUNCE_TICKS
 * to 2 or 3 (max 8 -- limited by debounce_cnt:uint8_t).
 */
#define ADCKEY_TMR_DURATION      20
#define ADCKEY_TICKS_INTERVAL    20	//ms, must equal ADCKEY_TMR_DURATION
#define ADCKEY_DEBOUNCE_TICKS    1	//MAX 8 (1 tick = 20ms, vs GPIO 18ms)
#define ADCKEY_SHORT_TICKS       (100 / ADCKEY_TICKS_INTERVAL)
#define ADCKEY_LONG_TICKS        ((CONFIG_ADC_KEY_LONG_PRESS_MS + ADCKEY_TICKS_INTERVAL - 1) / ADCKEY_TICKS_INTERVAL)

typedef void (*adc_key_callback)(void *);

typedef enum {
	ADCKEY_PRESS_DOWN = 0,
	ADCKEY_PRESS_UP,
	ADCKEY_PRESS_REPEAT,
	ADCKEY_SINGLE_CLICK,
	ADCKEY_DOUBLE_CLICK,
	ADCKEY_LONG_PRESS_START,
	ADCKEY_LONG_PRESS_HOLD,
	ADCKEY_NUMBER_OF_EVENT,
	ADCKEY_NONE_PRESS
} ADCKEY_PRESS_EVT;

typedef struct _adckey_ {
	uint16_t ticks;
	uint16_t lowest_active_level;
	uint16_t highest_active_level;
	uint16_t adc_read_level;
	uint8_t repeat;
	uint8_t event;
	uint8_t state;
	uint8_t debounce_cnt;

	void *user_data;
	adc_key_callback  cb[ADCKEY_NUMBER_OF_EVENT];
	struct _adckey_ *next;
} ADCKEY_S;

typedef enum {
	ADCKEY_S4 = 0,
	ADCKEY_S5,
	ADCKEY_NULL,
} ADCKEY_INDEX;

typedef struct
{
	uint16_t lowest_level;
	uint16_t highest_level;
	ADCKEY_INDEX user_index;
	adc_key_callback short_press_cb;
	adc_key_callback double_press_cb;
	adc_key_callback long_press_cb;
	adc_key_callback hold_press_cb;
} adckey_configure_t;

/*
 * GPIO key (KEY1) - can only detect any-press, not which button.
 * Reuses the same state machine as ADCKEY_S via the BUTTON_S framework.
 */
typedef struct {
	gpio_id_t gpio_id;
	uint8_t active_level;
	adc_key_callback short_press_cb;
	adc_key_callback double_press_cb;
	adc_key_callback long_press_cb;
	adc_key_callback hold_press_cb;
} gpio_key_configure_t;

/* ADC voltage read (shared, usable by monitor tasks) */
uint32_t adc_key_get_gpio_voltage(adc_chan_t chan);

/* ADC key API (KEY2 channel) */
void bk_adc_key_init(gpio_id_t gpio_id, adc_chan_t adc_chan);
void bk_adc_key_deinit(void);
uint32_t bk_adckey_item_configure(adckey_configure_t *config);
uint32_t bk_adckey_item_unconfigure(ADCKEY_INDEX user_data);

/* GPIO key API (KEY1) */
void bk_gpio_key_init(gpio_id_t gpio_id, uint8_t active_level);
void bk_gpio_key_deinit(void);
uint32_t bk_gpio_key_configure(gpio_key_configure_t *config);

#if CONFIG_ADC_KEY_DUAL_CHANNEL
void bk_adc_key_dual_init(void);
void bk_adc_key_dual_deinit(void);
#endif

#ifdef __cplusplus
}
#endif
