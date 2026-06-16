#include <os/os.h>
#include <os/mem.h>
#include "adc_key_main.h"
#include "modules/pm.h"
#include <key_main.h>
#include <multi_button.h>
#include "sys_sw_regs.h"


#if CONFIG_ADC_KEY

/* ======================== ADC Key (KEY2) ======================== */

#define ADCKEY_EVENT_CB(ev)   if(handle->cb[ev])handle->cb[ev]((ADCKEY_S*)handle)

beken_timer_t g_adckey_timer;
beken_mutex_t g_adckey_mutex;
static ADCKEY_S *adckey_head_handle = NULL;
static adc_chan_t s_adc_chan = ADC_MAX;
static bool s_adckey_inited_flag = 0;

#if CONFIG_ADC_KEY_DUAL_CHANNEL
static adc_chan_t s_adc_chan2 = ADC_MAX;
#endif

#define ADCKEY_ERR_LOG_INTERVAL        500
#define ADCKEY_REINIT_INTERVAL_TICKS   25
#define ADCKEY_REINIT_MAX_ATTEMPTS     0

static bool s_adc_chan_ready = false;
static uint32_t s_adc_err_count = 0;
static uint32_t s_reinit_tick_counter = 0;
static uint32_t s_reinit_attempts = 0;
static bool s_use_cp_sampler = false;

static bool adckey_try_adc_init(adc_chan_t chan)
{
	adc_config_t config = {0};
	os_memset(&config, 0, sizeof(adc_config_t));

	config.chan = chan;
	config.adc_mode = ADC_CONTINUOUS_MODE;
	config.src_clk = ADC_SCLK_XTAL;
	config.clk = 3203125;
	config.saturate_mode = ADC_SATURATE_MODE_3;
	config.steady_ctrl= 7;
	config.adc_filter = 0;

	if (bk_adc_init(chan) == BK_OK &&
	    bk_adc_set_config(&config) == BK_OK &&
	    bk_adc_enable_bypass_clalibration() == BK_OK) {
		s_adc_chan_ready = true;
		s_adc_err_count = 0;
		ADC_KEY_LOGI("ADC chan %d init OK (attempt %d)\r\n",
		             chan, s_reinit_attempts + 1);
		return true;
	}
	return false;
}

uint32_t adc_key_get_gpio_voltage(adc_chan_t chan)
{
	uint32_t value = 0;
	float cali_value = 0;
	bk_err_t ret;

	if (s_use_cp_sampler) {
		adc_key_sample_info_t sample = {0};
		if (bk_sys_sw_regs_get_adc_key_sample(&sample) &&
		    sample.channel == (uint8_t)chan &&
		    sample.status == 0) {
			return sample.mv;
		}
		return 9999;
	}

	if (!s_adc_chan_ready) {
		s_reinit_tick_counter++;
		if (s_reinit_tick_counter >= ADCKEY_REINIT_INTERVAL_TICKS) {
			s_reinit_tick_counter = 0;
			s_reinit_attempts++;
			if (ADCKEY_REINIT_MAX_ATTEMPTS == 0 ||
			    s_reinit_attempts <= ADCKEY_REINIT_MAX_ATTEMPTS) {
				if (adckey_try_adc_init(chan))
					goto do_read;
				ADC_KEY_LOGW("ADC reinit attempt %d failed\r\n",
				             s_reinit_attempts);
			}
		}
		return 9999;
	}

do_read:
	ret = bk_adc_acquire();
	if (ret != BK_OK)
		goto err_out;

	ret = bk_adc_start();
	if (ret != BK_OK) {
		bk_adc_release();
		goto err_out;
	}

	ret = bk_adc_set_channel(chan);
	if (ret != BK_OK) {
		bk_adc_stop();
		bk_adc_release();
		goto err_out;
	}

	ret = bk_adc_read((uint16_t *)&value, 100);
	if (ret != BK_OK) {
		bk_adc_stop();
		bk_adc_release();
		goto err_out;
	}

	cali_value = ((float)value/4096*2)*1.2;
	value = cali_value * 1000;

	bk_adc_stop();
	bk_adc_release();
	s_adc_err_count = 0;
	ADC_KEY_LOGV("adc_key voltage chan=%d value=%dmv\r\n", chan, value);
	return value;

err_out:
	s_adc_err_count++;
	if (s_adc_err_count == 1 || (s_adc_err_count % ADCKEY_ERR_LOG_INTERVAL) == 0)
		ADC_KEY_LOGW("adc_key read fail: ret=%d, err_count=%d\r\n",
		             ret, s_adc_err_count);
	return 9999;
}

/*
 * Classify raw ADC voltage into a logical zone:
 *   1 = within this handle's active range [lowest, highest]
 *   0 = outside (idle or another key)
 */
static uint8_t adckey_in_range(uint32_t voltage, uint32_t lowest, uint32_t highest)
{
	return (voltage >= lowest && voltage <= highest) ? 1 : 0;
}

void adckey_button_handler(ADCKEY_S *handle)
{
	uint32_t read_level = adc_key_get_gpio_voltage(s_adc_chan);
	uint32_t lowest = handle->lowest_active_level;
	uint32_t highest = handle->highest_active_level;

	uint8_t cur_in = adckey_in_range(read_level, lowest, highest);
	uint8_t prev_in = adckey_in_range(handle->adc_read_level, lowest, highest);

	if ((handle->state) > 0) handle->ticks++;

	/*------------button debounce handle---------------*/
	if (cur_in != prev_in) {
		if (++(handle->debounce_cnt) >= ADCKEY_DEBOUNCE_TICKS) {
			handle->adc_read_level = read_level;
			handle->debounce_cnt = 0;
		}
	} else {
		handle->debounce_cnt = 0;
		handle->adc_read_level = read_level;
	}

	uint8_t pressed = adckey_in_range(handle->adc_read_level, lowest, highest);

	/*-----------------State machine-------------------*/
	switch (handle->state) {
	case 0:
		if (pressed) {
			ADC_KEY_LOGI("PRESS_DOWN: adc=%dmV range=[%d,%d] user=%d\r\n",
			             handle->adc_read_level, lowest, highest,
			             (int)(uint32_t)handle->user_data);
			handle->event = (uint8_t)ADCKEY_PRESS_DOWN;
			ADCKEY_EVENT_CB(ADCKEY_PRESS_DOWN);
			handle->ticks = 0;
			handle->repeat = 1;
			handle->state = 1;
		} else
			handle->event = (uint8_t)ADCKEY_NONE_PRESS;
		break;

	case 1:
		if (!pressed) {
			handle->event = (uint8_t)ADCKEY_PRESS_UP;
			ADCKEY_EVENT_CB(ADCKEY_PRESS_UP);
			handle->ticks = 0;
			handle->state = 2;
		} else if (handle->ticks > ADCKEY_LONG_TICKS) {
			handle->event = (uint8_t)ADCKEY_LONG_PRESS_START;
			ADCKEY_EVENT_CB(ADCKEY_LONG_PRESS_START);
			handle->state = 5;
		}
		break;

	case 2:
		if (pressed) {
			handle->event = (uint8_t)ADCKEY_PRESS_DOWN;
			ADCKEY_EVENT_CB(ADCKEY_PRESS_DOWN);
			handle->repeat++;
			if (handle->repeat == 2) {
				ADCKEY_EVENT_CB(ADCKEY_DOUBLE_CLICK);
			}
			ADCKEY_EVENT_CB(ADCKEY_PRESS_REPEAT);
			handle->ticks = 0;
			handle->state = 3;
		} else if (handle->ticks > ADCKEY_SHORT_TICKS) {
			if (handle->repeat == 1) {
				handle->event = (uint8_t)ADCKEY_SINGLE_CLICK;
				ADCKEY_EVENT_CB(ADCKEY_SINGLE_CLICK);
			} else if (handle->repeat == 2)
				handle->event = (uint8_t)ADCKEY_DOUBLE_CLICK;
			handle->state = 0;
		}
		break;

	case 3:
		if (!pressed) {
			handle->event = (uint8_t)ADCKEY_PRESS_UP;
			ADCKEY_EVENT_CB(ADCKEY_PRESS_UP);
			if (handle->ticks < ADCKEY_SHORT_TICKS) {
				handle->ticks = 0;
				handle->state = 2;
			} else
				handle->state = 0;
		}
		break;

	case 5:
		if (pressed) {
			handle->event = (uint8_t)ADCKEY_LONG_PRESS_HOLD;
			ADCKEY_EVENT_CB(ADCKEY_LONG_PRESS_HOLD);
		} else {
			handle->event = (uint8_t)ADCKEY_PRESS_UP;
			ADCKEY_EVENT_CB(ADCKEY_PRESS_UP);
			handle->state = 0;
		}
		break;
	}
}

static void adc_key_ticks(void *param)
{
	ADCKEY_S *target;
	rtos_lock_mutex(&g_adckey_mutex);
	for (target = adckey_head_handle; target; target = target->next)
		adckey_button_handler(target);
	rtos_unlock_mutex(&g_adckey_mutex);
}

static void adc_key_configure()
{
	bk_err_t result;

	result = rtos_init_mutex(&g_adckey_mutex);
	if(kNoErr != result)
	{
		ADC_KEY_LOGD("rtos_init_mutex fail\r\n");
		return;
	}

	result = rtos_init_timer(&g_adckey_timer,
							 ADCKEY_TMR_DURATION,
							 adc_key_ticks,
							 (void *)0);
	if(kNoErr != result)
	{
		ADC_KEY_LOGD("rtos_init_timer fail\r\n");
		return;
	}

	result = rtos_start_timer(&g_adckey_timer);
	if(kNoErr != result)
	{
		ADC_KEY_LOGD("rtos_start_timer fail\r\n");
		return;
	}

}

void bk_adc_key_init(gpio_id_t gpio_id, adc_chan_t adc_chan)
{
	if(s_adckey_inited_flag)
		return;

	s_adc_chan = adc_chan;
	s_use_cp_sampler = (bk_adc_key_sampler_start(adc_chan, ADCKEY_TMR_DURATION) == BK_OK);
	if (s_use_cp_sampler) {
		ADC_KEY_LOGI("ADC key use CP sampler: chan=%d period=%dms\r\n", adc_chan, ADCKEY_TMR_DURATION);
	} else if (!adckey_try_adc_init(adc_chan)) {
		ADC_KEY_LOGW("ADC init deferred, will retry at runtime\r\n");
	}

	adc_key_configure();

	s_adckey_inited_flag = 1;
	ADC_KEY_LOGI("ADC key init: gpio=%d chan=%d ready=%d\r\n",
	             gpio_id, adc_chan, s_adc_chan_ready);
}

static void adckey_unconfig(void)
{
	bk_err_t ret;

	ret = rtos_deinit_mutex(&g_adckey_mutex);
	if(kNoErr != ret)
	{
		ADC_KEY_LOGD("rtos_deinit_mutex fail\r\n");
		return;
	}

	if (rtos_is_timer_init(&g_adckey_timer)) {
		if (rtos_is_timer_running(&g_adckey_timer)) {
			ret = rtos_stop_timer(&g_adckey_timer);
			if(kNoErr != ret)
			{
				ADC_KEY_LOGD("rtos_stop_timer fail\r\n");
				return;
			}
		}

		ret = rtos_deinit_timer(&g_adckey_timer);
		if(kNoErr != ret)
		{
			ADC_KEY_LOGD("rtos_deinit_timer fail\r\n");
			return;
		}
	}
}

void bk_adc_key_deinit(void)
{
	if(s_adckey_inited_flag)
		s_adckey_inited_flag = 0;
	else
		return;

	if (s_use_cp_sampler) {
		bk_adc_key_sampler_stop();
		s_use_cp_sampler = false;
	} else {
		bk_adc_deinit(s_adc_chan);
	}
	adckey_unconfig();
}

void adckey_button_init(ADCKEY_S *handle, adckey_configure_t *config)
{
	handle->event = (uint8_t)ADCKEY_NONE_PRESS;
	handle->lowest_active_level = config->lowest_level;
	handle->highest_active_level = config->highest_level;
	handle->user_data = (void *)(config->user_index);
}
void adckey_button_attach(ADCKEY_S *handle, ADCKEY_PRESS_EVT event, adc_key_callback cb)
{
	handle->cb[event] = cb;
}
int adckey_button_start(ADCKEY_S *handle)
{
	ADCKEY_S *target;
	rtos_lock_mutex(&g_adckey_mutex);
	target = adckey_head_handle;
	while (target) {
		if (target == handle) {
			rtos_unlock_mutex(&g_adckey_mutex);
			return -1;
		}
		target = target->next;
	}
	handle->next = adckey_head_handle;
	adckey_head_handle = handle;
	rtos_unlock_mutex(&g_adckey_mutex);
	return 0;
}

uint32_t bk_adckey_item_configure(adckey_configure_t *config)
{
	if(!s_adckey_inited_flag)
		return kGeneralErr;

	ADCKEY_S *handle;
	int result = 0;

	handle = os_malloc(sizeof(ADCKEY_S));
	if (NULL == handle)
		return kNoMemoryErr;
	os_memset(handle, 0, sizeof(ADCKEY_S));
	rtos_lock_mutex(&g_adckey_mutex);

	adckey_button_init(handle, config);
	adckey_button_attach(handle, ADCKEY_SINGLE_CLICK, (adc_key_callback)config->short_press_cb);
	adckey_button_attach(handle, ADCKEY_DOUBLE_CLICK, (adc_key_callback)config->double_press_cb);
	adckey_button_attach(handle, ADCKEY_LONG_PRESS_START,	(adc_key_callback)config->long_press_cb);
	adckey_button_attach(handle, ADCKEY_LONG_PRESS_HOLD, (adc_key_callback)config->hold_press_cb);

	rtos_unlock_mutex(&g_adckey_mutex);
	result = adckey_button_start(handle);
	if (result < 0) {
		ADC_KEY_LOGD("button_start failed\n");
		os_free(handle);
		return kGeneralErr;
	}

	return kNoErr;
}

ADCKEY_S *adckey_button_find_with_user_data(void *user_data)
{
	ADCKEY_S *entry = NULL;

	rtos_lock_mutex(&g_adckey_mutex);
	for (entry = adckey_head_handle; entry; entry = entry->next) {
		if (entry->user_data == user_data)
			break;
	}
	rtos_unlock_mutex(&g_adckey_mutex);

	return entry;
}

void adckey_button_stop(ADCKEY_S *handle)
{
	ADCKEY_S **curr;

	rtos_lock_mutex(&g_adckey_mutex);
	for (curr = &adckey_head_handle; *curr;) {
		ADCKEY_S *entry = *curr;
		if (entry == handle)
			*curr = entry->next;
		else
			curr = &entry->next;
	}
	rtos_unlock_mutex(&g_adckey_mutex);
}

uint32_t bk_adckey_item_unconfigure(ADCKEY_INDEX user_data)
{
	if(!s_adckey_inited_flag)
		return kGeneralErr;

	ADCKEY_S *handle = NULL;
	while (1) {
		handle = adckey_button_find_with_user_data((void *)user_data);
		if (NULL == handle)
			break;
		adckey_button_stop(handle);
		os_free(handle);
	}

	return kNoErr;
}

/* ======================== GPIO Key (KEY1) ======================== */
/*
 * KEY1 (GPIO39) can only detect any-button-press (S2 or S3),
 * cannot distinguish which one due to PCB issue (no ADC on GPIO39).
 * Uses the existing multi_button framework from the 'key' component.
 */

#if CONFIG_BUTTON

static bool s_gpio_key_inited = false;
extern beken_mutex_t g_key_mutex;
extern beken2_timer_t g_key_timer;

static void gpio_key_pin_config(gpio_id_t gpio_id, uint8_t active_level)
{
	BK_LOG_ON_ERR(bk_gpio_disable_output(gpio_id));
	BK_LOG_ON_ERR(bk_gpio_enable_input(gpio_id));
	BK_LOG_ON_ERR(bk_gpio_enable_pull(gpio_id));
	if(active_level)
		BK_LOG_ON_ERR(bk_gpio_pull_down(gpio_id));
	else
		BK_LOG_ON_ERR(bk_gpio_pull_up(gpio_id));
}

static uint8_t gpio_key1_get_value(BUTTON_S *handle)
{
	return bk_gpio_get_input((uint32_t)handle->user_data);
}

void bk_gpio_key_init(gpio_id_t gpio_id, uint8_t active_level)
{
	if(s_gpio_key_inited)
		return;

	s_gpio_key_inited = true;
	ADC_KEY_LOGI("GPIO key init: gpio=%d active=%d\r\n", gpio_id, active_level);
}

void bk_gpio_key_deinit(void)
{
	if(!s_gpio_key_inited)
		return;
	s_gpio_key_inited = false;
}

uint32_t bk_gpio_key_configure(gpio_key_configure_t *config)
{
	if(!s_gpio_key_inited)
		return kGeneralErr;

	BUTTON_S *handle;
	int result;

	handle = os_malloc(sizeof(BUTTON_S));
	if (NULL == handle)
		return kNoMemoryErr;

	rtos_lock_mutex(&g_key_mutex);

	gpio_key_pin_config(config->gpio_id, config->active_level);
	button_init(handle, gpio_key1_get_value, config->active_level,
	            (void *)(uint32_t)config->gpio_id);
	button_attach(handle, SINGLE_CLICK, (btn_callback)config->short_press_cb);
	button_attach(handle, DOUBLE_CLICK, (btn_callback)config->double_press_cb);
	button_attach(handle, LONG_PRESS_START, (btn_callback)config->long_press_cb);
	button_attach(handle, LONG_PRESS_HOLD, (btn_callback)config->hold_press_cb);

	rtos_unlock_mutex(&g_key_mutex);
	result = button_start(handle);
	if (result < 0) {
		ADC_KEY_LOGE("gpio key button_start failed\n");
		os_free(handle);
		return kGeneralErr;
	}

	ADC_KEY_LOGI("GPIO key configured: gpio=%d\r\n", config->gpio_id);
	return kNoErr;
}

#else /* !CONFIG_BUTTON */

void bk_gpio_key_init(gpio_id_t gpio_id, uint8_t active_level)
{
	ADC_KEY_LOGE("GPIO key requires CONFIG_BUTTON\r\n");
}

void bk_gpio_key_deinit(void)
{
}

uint32_t bk_gpio_key_configure(gpio_key_configure_t *config)
{
	ADC_KEY_LOGE("GPIO key requires CONFIG_BUTTON\r\n");
	return kGeneralErr;
}

#endif /* CONFIG_BUTTON */

/* ======================== Dual Channel (future PCB) ======================== */

#if CONFIG_ADC_KEY_DUAL_CHANNEL

void bk_adc_key_dual_init(void)
{
	bk_adc_key_init(ADC_KEY2_GPIO_ID, ADC_KEY2_SADC_CHAN_ID);

	s_adc_chan2 = ADC_KEY1_SADC_CHAN_ID;
	if (!adckey_try_adc_init(s_adc_chan2))
		ADC_KEY_LOGW("ADC chan2 init deferred\r\n");
	ADC_KEY_LOGI("Dual ADC key init: ch1_gpio=%d ch1_adc=%d, ch2_gpio=%d ch2_adc=%d\r\n",
	             ADC_KEY1_GPIO_ID, ADC_KEY1_SADC_CHAN_ID,
	             ADC_KEY2_GPIO_ID, ADC_KEY2_SADC_CHAN_ID);
}

void bk_adc_key_dual_deinit(void)
{
	bk_adc_key_deinit();
	if(s_adc_chan2 != ADC_MAX) {
		bk_adc_deinit(s_adc_chan2);
		s_adc_chan2 = ADC_MAX;
	}
}

#endif /* CONFIG_ADC_KEY_DUAL_CHANNEL */

#endif /* CONFIG_ADC_KEY */
