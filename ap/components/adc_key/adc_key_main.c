#include <os/os.h>
#include <os/mem.h>
#include <string.h>
#include "adc_key_main.h"
#include "modules/pm.h"
#include <key_main.h>
#include <multi_button.h>


#if CONFIG_ADC_KEY

/* ======================== Generic multi-channel ADC key ======================== */

#define ADCKEY_EVENT_CB(ev)   adckey_event_notify(handle, (ev))

beken_timer_t g_adckey_timer;
beken_mutex_t g_adckey_mutex;
static ADCKEY_S *adckey_head_handle = NULL;
static adc_chan_t s_adc_chan = ADC_MAX;
static bool s_adckey_inited_flag = 0;

#define ADCKEY_ERR_LOG_INTERVAL        500

typedef struct {
	ADCKEY_S button;
	adc_chan_t adc_chan;
	adc_key_id_t key_id;
	void *callback_user_data;
	uint16_t short_ticks;
	uint16_t long_ticks;
	bool allocated;
	bool extended_api;
} adc_key_item_node_t;

static adc_key_item_node_t s_adc_key_item_pool[CONFIG_ADC_KEY_MAX_ITEMS];
static adc_chan_t s_adc_key_channels[CONFIG_ADC_KEY_MAX_CHANNELS];
static uint8_t s_adc_key_channel_count = 0U;
static uint16_t s_adc_key_sample_period_ms = CONFIG_ADC_KEY_SAMPLE_PERIOD_MS;
static uint8_t s_adc_key_max_channels = CONFIG_ADC_KEY_MAX_CHANNELS;
static uint8_t s_adc_key_max_items = CONFIG_ADC_KEY_MAX_ITEMS;
static bool s_legacy_default_channel = false;
static bool s_use_cp_sampler = false;
static uint32_t s_adc_err_count = 0;

static adc_key_item_node_t *adckey_node_from_handle(ADCKEY_S *handle)
{
	return (adc_key_item_node_t *)handle;
}

static void adckey_event_notify(ADCKEY_S *handle, ADCKEY_PRESS_EVT event)
{
	adc_key_item_node_t *node = adckey_node_from_handle(handle);
	adc_key_callback callback = handle->cb[event];

	if (callback == NULL) {
		return;
	}

	callback(node->extended_api ? node->callback_user_data : (void *)handle);
}

static bk_err_t adckey_sample_once(adc_chan_t chan, uint16_t *mv)
{
	adc_config_t config = {0};
	uint16_t raw = 0;
	bk_err_t ret;
	float cali_value;

	ret = bk_adc_acquire();
	if (ret != BK_OK) {
		return ret;
	}

	ret = bk_adc_init(chan);
	if (ret != BK_OK) {
		goto adc_exit;
	}

	config.chan = chan;
	config.adc_mode = ADC_CONTINUOUS_MODE;
	config.src_clk = ADC_SCLK_XTAL;
	config.clk = 3203125;
	config.saturate_mode = ADC_SATURATE_MODE_3;
	config.steady_ctrl = 7;
	config.adc_filter = 0;
	config.sample_rate = 0;

	ret = bk_adc_set_config(&config);
	if (ret != BK_OK) {
		goto adc_exit;
	}

	ret = bk_adc_enable_bypass_clalibration();
	if (ret != BK_OK) {
		goto adc_exit;
	}

	ret = bk_adc_start();
	if (ret != BK_OK) {
		goto adc_exit;
	}

	ret = bk_adc_read(&raw, 100);
	if (ret != BK_OK) {
		goto adc_exit;
	}

	cali_value = ((float)raw / 4096.0f * 2.0f) * 1.2f;
	*mv = (uint16_t)(cali_value * 1000.0f);

adc_exit:
	bk_adc_stop();
	bk_adc_deinit(chan);
	bk_adc_release();
	return ret;
}

uint32_t adc_key_get_gpio_voltage(adc_chan_t chan)
{
	bk_err_t ret;

	if (s_use_cp_sampler) {
		adc_key_sampler_sample_t samples[ADC_KEY_SAMPLER_MAX_CHANNELS];
		uint8_t count = 0U;

		ret = bk_adc_key_sampler_get_samples(
			samples, ADC_KEY_SAMPLER_MAX_CHANNELS, &count);
		if (ret == BK_OK) {
			for (uint8_t i = 0; i < count; i++) {
				if ((samples[i].channel == (uint8_t)chan) &&
				    (samples[i].status == 0U)) {
					return samples[i].mv;
				}
			}
		}
		return 9999;
	}

	uint16_t mv = 0;
	ret = adckey_sample_once(chan, &mv);
	if (ret != BK_OK) {
		s_adc_err_count++;
		if (s_adc_err_count == 1 || (s_adc_err_count % ADCKEY_ERR_LOG_INTERVAL) == 0) {
			ADC_KEY_LOGW("adc_key read fail: ret=%d, err_count=%d\r\n",
			             ret, s_adc_err_count);
		}
		return 9999;
	}

	s_adc_err_count = 0;
	ADC_KEY_LOGV("adc_key voltage chan=%d value=%dmv\r\n", chan, mv);
	return mv;
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

static void adckey_button_handler(ADCKEY_S *handle, uint32_t read_level)
{
	adc_key_item_node_t *node = adckey_node_from_handle(handle);
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
			ADC_KEY_LOGI("PRESS_DOWN: adc=%dmV range=[%d,%d] key=%u\r\n",
			             handle->adc_read_level, lowest, highest,
			             (unsigned)node->key_id);
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
		} else if (handle->ticks > node->long_ticks) {
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
		} else if (handle->ticks > node->short_ticks) {
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
			if (handle->ticks < node->short_ticks) {
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

static bool adc_key_channel_exists(const adc_chan_t *channels,
				   uint8_t count, adc_chan_t chan)
{
	for (uint8_t i = 0; i < count; i++) {
		if (channels[i] == chan) {
			return true;
		}
	}
	return false;
}

static bk_err_t adc_key_collect_channels_locked(void)
{
	ADCKEY_S *target;
	uint8_t count = 0U;

	if (s_legacy_default_channel && (s_adc_chan < ADC_MAX)) {
		s_adc_key_channels[count++] = s_adc_chan;
	}

	for (target = adckey_head_handle; target; target = target->next) {
		adc_key_item_node_t *node = adckey_node_from_handle(target);
		if (adc_key_channel_exists(s_adc_key_channels, count,
					   node->adc_chan)) {
			continue;
		}
		if (count >= s_adc_key_max_channels) {
			return BK_ERR_NO_MEM;
		}
		s_adc_key_channels[count++] = node->adc_chan;
	}

	s_adc_key_channel_count = count;
	return BK_OK;
}

static bk_err_t adc_key_refresh_sampler_locked(void)
{
	bk_err_t ret = adc_key_collect_channels_locked();

	if (ret != BK_OK) {
		return ret;
	}

	if (s_use_cp_sampler) {
		(void)bk_adc_key_sampler_stop();
		s_use_cp_sampler = false;
	}

	if (s_adc_key_channel_count == 0U) {
		return BK_OK;
	}

	ret = bk_adc_key_sampler_start_multi(s_adc_key_channels,
					     s_adc_key_channel_count,
					     s_adc_key_sample_period_ms);
	if (ret == BK_OK) {
		s_use_cp_sampler = true;
		ADC_KEY_LOGI("use CP sampler: channels=%d period=%dms\r\n",
			     s_adc_key_channel_count, s_adc_key_sample_period_ms);
	} else {
		ADC_KEY_LOGW("CP sampler unavailable, use AP one-shot ADC\r\n");
	}

	return BK_OK;
}

static uint32_t adc_key_find_sample(const adc_key_sampler_sample_t *samples,
				    uint8_t count, adc_chan_t chan)
{
	for (uint8_t i = 0; i < count; i++) {
		if ((samples[i].channel == (uint8_t)chan) &&
		    (samples[i].status == 0U)) {
			return samples[i].mv;
		}
	}
	return 9999U;
}

static void adc_key_ticks(void *param)
{
	adc_key_sampler_sample_t samples[ADC_KEY_SAMPLER_MAX_CHANNELS] = {0};
	adc_chan_t channels[ADC_KEY_SAMPLER_MAX_CHANNELS];
	uint8_t channel_count;
	uint8_t sample_count = 0U;
	ADCKEY_S *target;
	bk_err_t ret = BK_FAIL;

	(void)param;

	rtos_lock_mutex(&g_adckey_mutex);
	channel_count = s_adc_key_channel_count;
	memcpy(channels, s_adc_key_channels,
	       channel_count * sizeof(adc_chan_t));
	rtos_unlock_mutex(&g_adckey_mutex);

	if (s_use_cp_sampler && (channel_count > 0U)) {
		ret = bk_adc_key_sampler_get_samples(
			samples, ADC_KEY_SAMPLER_MAX_CHANNELS, &sample_count);
	}

	if ((ret != BK_OK) && (channel_count > 0U)) {
		sample_count = channel_count;
		for (uint8_t i = 0; i < channel_count; i++) {
			uint16_t mv = 0U;
			ret = adckey_sample_once(channels[i], &mv);
			samples[i].channel = (uint8_t)channels[i];
			samples[i].status = (ret == BK_OK) ? 0U : (uint8_t)ret;
			samples[i].mv = (ret == BK_OK) ? mv : 9999U;
		}
	}

	rtos_lock_mutex(&g_adckey_mutex);
	for (target = adckey_head_handle; target; target = target->next) {
		adc_key_item_node_t *node = adckey_node_from_handle(target);
		uint32_t voltage = adc_key_find_sample(
			samples, sample_count, node->adc_chan);
		adckey_button_handler(target, voltage);
	}
	rtos_unlock_mutex(&g_adckey_mutex);
}

static bk_err_t adc_key_configure(void)
{
	bk_err_t result;

	result = rtos_init_mutex(&g_adckey_mutex);
	if(kNoErr != result)
	{
		ADC_KEY_LOGE("rtos_init_mutex fail\r\n");
		return result;
	}

	result = rtos_init_timer(&g_adckey_timer,
							 s_adc_key_sample_period_ms,
							 adc_key_ticks,
							 (void *)0);
	if(kNoErr != result)
	{
		ADC_KEY_LOGE("rtos_init_timer fail\r\n");
		(void)rtos_deinit_mutex(&g_adckey_mutex);
		return result;
	}

	result = rtos_start_timer(&g_adckey_timer);
	if(kNoErr != result)
	{
		ADC_KEY_LOGE("rtos_start_timer fail\r\n");
		(void)rtos_deinit_timer(&g_adckey_timer);
		(void)rtos_deinit_mutex(&g_adckey_mutex);
		return result;
	}

	return BK_OK;
}

bk_err_t bk_adc_key_init_ex(const adc_key_driver_config_t *config)
{
	bk_err_t ret;

	if (s_adckey_inited_flag) {
		return BK_ERR_STATE;
	}

	s_adc_key_sample_period_ms = CONFIG_ADC_KEY_SAMPLE_PERIOD_MS;
	s_adc_key_max_channels = CONFIG_ADC_KEY_MAX_CHANNELS;
	s_adc_key_max_items = CONFIG_ADC_KEY_MAX_ITEMS;

	if (config != NULL) {
		if ((config->size != sizeof(adc_key_driver_config_t)) ||
		    (config->version != ADC_KEY_CONFIG_VERSION) ||
		    (config->sample_period_ms < 20U) ||
		    (config->max_channels == 0U) ||
		    (config->max_channels > CONFIG_ADC_KEY_MAX_CHANNELS) ||
		    (config->max_channels > ADC_KEY_SAMPLER_MAX_CHANNELS) ||
		    (config->max_items == 0U) ||
		    (config->max_items > CONFIG_ADC_KEY_MAX_ITEMS)) {
			return BK_ERR_PARAM;
		}
		s_adc_key_sample_period_ms = config->sample_period_ms;
		s_adc_key_max_channels = config->max_channels;
		s_adc_key_max_items = config->max_items;
	}

	memset(s_adc_key_item_pool, 0, sizeof(s_adc_key_item_pool));
	memset(s_adc_key_channels, 0, sizeof(s_adc_key_channels));
	adckey_head_handle = NULL;
	s_adc_key_channel_count = 0U;
	s_adc_chan = ADC_MAX;
	s_legacy_default_channel = false;
	s_use_cp_sampler = false;

	ret = adc_key_configure();
	if (ret != BK_OK) {
		return ret;
	}

	s_adckey_inited_flag = 1;
	ADC_KEY_LOGI("ADC key manager init: period=%dms channels=%d items=%d\r\n",
		     s_adc_key_sample_period_ms, s_adc_key_max_channels,
		     s_adc_key_max_items);
	return BK_OK;
}

void bk_adc_key_init(gpio_id_t gpio_id, adc_chan_t adc_chan)
{
	bk_err_t ret;

	if (s_adckey_inited_flag) {
		return;
	}
	if (adc_chan >= ADC_MAX) {
		ADC_KEY_LOGE("invalid legacy ADC channel: %d\r\n", adc_chan);
		return;
	}

	ret = bk_adc_key_init_ex(NULL);
	if (ret != BK_OK) {
		ADC_KEY_LOGE("ADC key init failed: %d\r\n", ret);
		return;
	}

	rtos_lock_mutex(&g_adckey_mutex);
	s_adc_chan = adc_chan;
	s_legacy_default_channel = true;
	ret = adc_key_refresh_sampler_locked();
	rtos_unlock_mutex(&g_adckey_mutex);
	if (ret != BK_OK) {
		ADC_KEY_LOGE("legacy sampler init failed: %d\r\n", ret);
	}

	ADC_KEY_LOGI("legacy ADC key init: gpio=%d chan=%d\r\n",
		     gpio_id, adc_chan);
}

static void adckey_unconfig(void)
{
	bk_err_t ret;

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

	ret = rtos_deinit_mutex(&g_adckey_mutex);
	if(kNoErr != ret)
	{
		ADC_KEY_LOGD("rtos_deinit_mutex fail\r\n");
	}
}

bk_err_t bk_adc_key_deinit_ex(void)
{
	if (!s_adckey_inited_flag) {
		return BK_ERR_NOT_INIT;
	}

	if (s_use_cp_sampler) {
		(void)bk_adc_key_sampler_stop();
		s_use_cp_sampler = false;
	}

	rtos_lock_mutex(&g_adckey_mutex);
	adckey_head_handle = NULL;
	memset(s_adc_key_item_pool, 0, sizeof(s_adc_key_item_pool));
	s_adc_key_channel_count = 0U;
	s_legacy_default_channel = false;
	s_adc_chan = ADC_MAX;
	rtos_unlock_mutex(&g_adckey_mutex);

	s_adckey_inited_flag = 0;
	adckey_unconfig();
	return BK_OK;
}

void bk_adc_key_deinit(void)
{
	(void)bk_adc_key_deinit_ex();
}

static adc_key_item_node_t *adc_key_item_alloc_locked(void)
{
	for (uint8_t i = 0; i < s_adc_key_max_items; i++) {
		if (!s_adc_key_item_pool[i].allocated) {
			memset(&s_adc_key_item_pool[i], 0,
			       sizeof(s_adc_key_item_pool[i]));
			s_adc_key_item_pool[i].allocated = true;
			return &s_adc_key_item_pool[i];
		}
	}
	return NULL;
}

static void adc_key_item_free_locked(adc_key_item_node_t *node)
{
	memset(node, 0, sizeof(*node));
}

static void adc_key_item_attach(ADCKEY_S *handle,
				ADCKEY_PRESS_EVT event,
				adc_key_callback callback)
{
	handle->cb[event] = callback;
}

static void adc_key_item_insert_locked(adc_key_item_node_t *node)
{
	ADCKEY_S *handle = &node->button;

	handle->next = adckey_head_handle;
	adckey_head_handle = handle;
}

static void adc_key_item_remove_locked(adc_key_item_node_t *node)
{
	ADCKEY_S **current = &adckey_head_handle;

	while (*current != NULL) {
		if (*current == &node->button) {
			*current = node->button.next;
			adc_key_item_free_locked(node);
			return;
		}
		current = &(*current)->next;
	}
}

static bool adc_key_ranges_overlap(uint16_t low_a, uint16_t high_a,
				   uint16_t low_b, uint16_t high_b)
{
	return !((high_a < low_b) || (high_b < low_a));
}

static bk_err_t adc_key_validate_item_locked(
	const adc_key_item_config_ex_t *config)
{
	ADCKEY_S *target;
	uint8_t channel_items = 0U;

	for (target = adckey_head_handle; target; target = target->next) {
		adc_key_item_node_t *node = adckey_node_from_handle(target);

		if (node->extended_api && (node->key_id == config->key_id)) {
			return BK_ERR_IS_EXIST;
		}
		if (node->adc_chan != config->adc_chan) {
			continue;
		}
		channel_items++;
		if (adc_key_ranges_overlap(target->lowest_active_level,
					   target->highest_active_level,
					   config->lowest_level,
					   config->highest_level)) {
			return BK_ERR_PARAM;
		}
	}

	if (channel_items >= CONFIG_ADC_KEY_MAX_ITEMS_PER_CHANNEL) {
		return BK_ERR_NO_MEM;
	}
	return BK_OK;
}

static void adc_key_item_init_callbacks(
	ADCKEY_S *handle,
	adc_key_callback short_press_cb,
	adc_key_callback double_press_cb,
	adc_key_callback long_press_cb,
	adc_key_callback hold_press_cb)
{
	adc_key_item_attach(handle, ADCKEY_SINGLE_CLICK, short_press_cb);
	adc_key_item_attach(handle, ADCKEY_DOUBLE_CLICK, double_press_cb);
	adc_key_item_attach(handle, ADCKEY_LONG_PRESS_START, long_press_cb);
	adc_key_item_attach(handle, ADCKEY_LONG_PRESS_HOLD, hold_press_cb);
}

uint32_t bk_adckey_item_configure(adckey_configure_t *config)
{
	adc_key_item_node_t *node;
	bk_err_t ret;

	if (!s_adckey_inited_flag || (config == NULL) ||
	    (config->lowest_level > config->highest_level) ||
	    (s_adc_chan >= ADC_MAX)) {
		return kGeneralErr;
	}

	rtos_lock_mutex(&g_adckey_mutex);
	node = adc_key_item_alloc_locked();
	if (node == NULL) {
		rtos_unlock_mutex(&g_adckey_mutex);
		return kNoMemoryErr;
	}

	node->adc_chan = s_adc_chan;
	node->key_id = (adc_key_id_t)config->user_index;
	node->callback_user_data = &node->button;
	node->short_ticks = (100U + s_adc_key_sample_period_ms - 1U) /
		s_adc_key_sample_period_ms;
	node->long_ticks = (CONFIG_ADC_KEY_LONG_PRESS_MS +
		s_adc_key_sample_period_ms - 1U) / s_adc_key_sample_period_ms;
	node->extended_api = false;
	node->button.event = (uint8_t)ADCKEY_NONE_PRESS;
	node->button.lowest_active_level = config->lowest_level;
	node->button.highest_active_level = config->highest_level;
	node->button.user_data = (void *)(uintptr_t)config->user_index;
	adc_key_item_init_callbacks(
		&node->button, config->short_press_cb, config->double_press_cb,
		config->long_press_cb, config->hold_press_cb);
	adc_key_item_insert_locked(node);

	ret = adc_key_refresh_sampler_locked();
	if (ret != BK_OK) {
		adc_key_item_remove_locked(node);
		(void)adc_key_refresh_sampler_locked();
		rtos_unlock_mutex(&g_adckey_mutex);
		return kGeneralErr;
	}
	rtos_unlock_mutex(&g_adckey_mutex);
	return kNoErr;
}

uint32_t bk_adckey_item_unconfigure(ADCKEY_INDEX user_data)
{
	ADCKEY_S *target;

	if (!s_adckey_inited_flag) {
		return kGeneralErr;
	}

	rtos_lock_mutex(&g_adckey_mutex);
	target = adckey_head_handle;
	while (target != NULL) {
		ADCKEY_S *next = target->next;
		adc_key_item_node_t *node = adckey_node_from_handle(target);
		if (!node->extended_api &&
		    (target->user_data == (void *)(uintptr_t)user_data)) {
			adc_key_item_remove_locked(node);
		}
		target = next;
	}
	(void)adc_key_refresh_sampler_locked();
	rtos_unlock_mutex(&g_adckey_mutex);

	return kNoErr;
}

bk_err_t bk_adc_key_item_configure_ex(
	const adc_key_item_config_ex_t *config, adc_key_handle_t *handle)
{
	adc_key_item_node_t *node;
	bk_err_t ret;

	if (!s_adckey_inited_flag) {
		return BK_ERR_NOT_INIT;
	}
	if ((config == NULL) ||
	    (config->size != sizeof(adc_key_item_config_ex_t)) ||
	    (config->version != ADC_KEY_CONFIG_VERSION) ||
	    (config->key_id == ADC_KEY_INVALID_ID) ||
	    (config->adc_chan >= ADC_MAX) ||
	    (config->lowest_level > config->highest_level)) {
		return BK_ERR_PARAM;
	}

	rtos_lock_mutex(&g_adckey_mutex);
	ret = adc_key_validate_item_locked(config);
	if (ret != BK_OK) {
		rtos_unlock_mutex(&g_adckey_mutex);
		return ret;
	}

	node = adc_key_item_alloc_locked();
	if (node == NULL) {
		rtos_unlock_mutex(&g_adckey_mutex);
		return BK_ERR_NO_MEM;
	}

	node->adc_chan = config->adc_chan;
	node->key_id = config->key_id;
	node->callback_user_data = config->user_data;
	node->short_ticks = (100U + s_adc_key_sample_period_ms - 1U) /
		s_adc_key_sample_period_ms;
	node->long_ticks = (CONFIG_ADC_KEY_LONG_PRESS_MS +
		s_adc_key_sample_period_ms - 1U) / s_adc_key_sample_period_ms;
	node->extended_api = true;
	node->button.event = (uint8_t)ADCKEY_NONE_PRESS;
	node->button.lowest_active_level = config->lowest_level;
	node->button.highest_active_level = config->highest_level;
	node->button.user_data = config->user_data;
	adc_key_item_init_callbacks(
		&node->button, config->short_press_cb, config->double_press_cb,
		config->long_press_cb, config->hold_press_cb);
	adc_key_item_insert_locked(node);

	ret = adc_key_refresh_sampler_locked();
	if (ret != BK_OK) {
		adc_key_item_remove_locked(node);
		(void)adc_key_refresh_sampler_locked();
		rtos_unlock_mutex(&g_adckey_mutex);
		return ret;
	}

	if (handle != NULL) {
		*handle = (adc_key_handle_t)node;
	}
	rtos_unlock_mutex(&g_adckey_mutex);
	return BK_OK;
}

bk_err_t bk_adc_key_item_unconfigure_ex(adc_key_handle_t handle)
{
	adc_key_item_node_t *node = (adc_key_item_node_t *)handle;
	uintptr_t node_addr = (uintptr_t)node;
	uintptr_t pool_start = (uintptr_t)&s_adc_key_item_pool[0];
	uintptr_t pool_end = (uintptr_t)&s_adc_key_item_pool[
		CONFIG_ADC_KEY_MAX_ITEMS];

	if (!s_adckey_inited_flag) {
		return BK_ERR_NOT_INIT;
	}
	if ((node == NULL) || (node_addr < pool_start) || (node_addr >= pool_end) ||
	    (((node_addr - pool_start) % sizeof(adc_key_item_node_t)) != 0U)) {
		return BK_ERR_PARAM;
	}

	rtos_lock_mutex(&g_adckey_mutex);
	if (!node->allocated || !node->extended_api) {
		rtos_unlock_mutex(&g_adckey_mutex);
		return BK_ERR_NOT_FOUND;
	}
	adc_key_item_remove_locked(node);
	(void)adc_key_refresh_sampler_locked();
	rtos_unlock_mutex(&g_adckey_mutex);
	return BK_OK;
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
	bk_err_t ret = bk_adc_key_init_ex(NULL);

	if ((ret != BK_OK) && (ret != BK_ERR_STATE)) {
		ADC_KEY_LOGE("dual ADC key manager init failed: %d\r\n", ret);
	}
}

void bk_adc_key_dual_deinit(void)
{
	(void)bk_adc_key_deinit_ex();
}

#endif /* CONFIG_ADC_KEY_DUAL_CHANNEL */

#endif /* CONFIG_ADC_KEY */
