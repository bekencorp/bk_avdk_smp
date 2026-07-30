#include <stdio.h>
#include <os/os.h>
#include <key_main.h>
#include <key_adapter.h>
#include <driver/gpio.h>
#include <gpio_driver.h>

#include <components/log.h>

#if CONFIG_ADC_KEY
#include "adc_key_main.h"
#endif

#define TAG "key"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define KEY_MSG_QUEUE_NAME "key_queue"
#define KEY_MSG_QUEUE_COUNT (10)
static beken_queue_t s_key_msgqueue = NULL;


#define KEY_THREAD_PRIORITY (4)
#define KEY_THREAD_NAME "key_thread"
#define KEY_THREAD_STACK_SIZE (0x2<<10)
static beken_thread_t s_key_thread = NULL;

static KeyConfig_t *key_configs = NULL;
static key_handler_t global_handler = NULL;
static uint8_t key_count = 0;



/*
 * multi_button fires LONG_PRESS_HOLD on every tick (~6ms) while the key stays
 * pressed, which would flood the key queue. Throttle it to a sane repeat rate.
 */
#define HOLD_REPEAT_INTERVAL_MS (200)
static uint32_t s_last_hold_ms = 0;

static void short_press_cb(void *param);
static void double_press_cb(void *param);
static void long_press_cb(void *param);
static void hold_press_cb(void *param);
static void long_press_up_cb(void *param);
static void key_thread(void *param);

void bk_init_keys()
{
    key_initialization();
    
}

void bk_deinit_keys()
{
    key_uninitialization();
 
}


void bk_configure_key(KeyConfig_t *KeyConfig)
{
    
    bk_err_t ret;

    /*
     * HOLD (LONG_PRESS_HOLD) is currently unused by the project. Attaching a
     * callback here makes multi_button fire it on every tick while any key is
     * held, flooding the queue/log with "no registered callback" for keys that
     * have no hold action. Pass NULL so no HOLD event is generated at all; the
     * HOLD plumbing (types/fields/hold_press_cb) is kept for future use.
     */
    ret = key_item_configure(KeyConfig->gpio_id, 
                                 KeyConfig->active_level, 
                                 short_press_cb, 
                                 double_press_cb, 
                                 long_press_cb, 
                                 NULL,
                                 long_press_up_cb);
    
    if (ret != BK_OK)
    {
        LOGI("key_config failed\r\n");
        return;
    }
}


void bk_key_driver_init(KeyConfig_t* configs, uint8_t num_keys) {

    bk_err_t ret;

    bk_init_keys();
    key_configs = configs;
    key_count = num_keys;
    for (uint8_t i = 0; i < num_keys; i++) {
        bk_configure_key(configs+i);
        LOGI("key id is %d\r\n",(configs+i)->gpio_id);
    }

    ret = rtos_init_queue(
							&s_key_msgqueue,
							KEY_MSG_QUEUE_NAME,
							sizeof(KeyEventMsg_t),
							KEY_MSG_QUEUE_COUNT
						);

	if (kNoErr != ret)
	{
		LOGI("init queue ret=%d", ret);
        goto err_exit;
	}

#if CONFIG_PSRAM_AS_SYS_MEMORY
    	ret = rtos_create_psram_thread(
								&s_key_thread,
								KEY_THREAD_PRIORITY,
								KEY_THREAD_NAME,
								key_thread,
								KEY_THREAD_STACK_SIZE,
								NULL
							);
#else
    	ret = rtos_create_thread(
								&s_key_thread,
								KEY_THREAD_PRIORITY,
								KEY_THREAD_NAME,
								key_thread,
								KEY_THREAD_STACK_SIZE,
								NULL
							);
#endif


    if (kNoErr != ret)
	{
		LOGI("init thread ret=%d", ret);
		goto err_exit;
	}
    
    return ;

err_exit:
	if(s_key_msgqueue)
	{
		rtos_deinit_queue(&s_key_msgqueue);
	}

	if(s_key_thread)
	{
		rtos_delete_thread(&s_key_thread);
        s_key_thread = NULL;
	}

    return ;
}

void bk_key_driver_deinit(KeyConfig_t* configs, uint8_t num_keys){

    for (uint8_t i = 0; i < num_keys; i++) {
        key_item_unconfigure((configs+i)->gpio_id);
    }

    bk_deinit_keys();
}

void bk_key_register_event_handler(key_handler_t handler) {
    if (handler == NULL){
        LOGI("null ptr funtion is %s\r\n",__func__);
    }
    global_handler = handler;
}

// 内部事件处理
static void process_key_event(uint8_t gpio_id, key_action_t action) {
    
    if(!global_handler) {
        return;
    }

    for(uint8_t i=0; i<key_count; i++) {
        if(key_configs[i].gpio_id == gpio_id) {
            key_event_t event = EVENT_NONE;
            switch(action) {
                case SHORT_PRESS: 
                    event = key_configs[i].short_event; 
                    break;
                case DOUBLE_PRESS: 
                    event = key_configs[i].double_event; 
                    break;
                case LONG_PRESS: 
                    event = key_configs[i].long_event; 
                    break;
                case LONG_PRESS_UP:
                    event = key_configs[i].long_press_up_event;
                    break;
                case HOLD_PRESS:
                    event = key_configs[i].hold_event;
                    break;
                default:
                    break;
            }

            if(event != EVENT_NONE) {
                LOGI("current key_id is %d\r\n",gpio_id);
                global_handler(event);
                LOGI("end processing key enevt\r\n");
            }
            break;
        }
    }
}

static void key_thread(void *param){

    KeyEventMsg_t rec_msg;

    while (1)
    {
        if(rtos_pop_from_queue(&s_key_msgqueue, &rec_msg, BEKEN_WAIT_FOREVER) == kNoErr){
            LOGI("start processing key enevt\r\n");
            process_key_event(rec_msg.gpio_id, rec_msg.action);
        }
    }
    
}

void short_press_cb(void *param) {

    LOGI("enter short press cb\r\n");
    bk_err_t ret;

    BUTTON_S * handle = (BUTTON_S *)param;
   
    uint32_t gpio_id = (uint32_t)(handle->user_data);

        KeyEventMsg_t msg = {
        .gpio_id = gpio_id,
        .action = SHORT_PRESS
    };

   ret = rtos_push_to_queue(&s_key_msgqueue, &msg, 1000);

    if (kNoErr != ret){
		LOGI("key send msg failed");
	}

   
}

void double_press_cb(void *param) {
    LOGI("enter double press cb\r\n");
    bk_err_t ret;

    BUTTON_S * handle = (BUTTON_S *)param;
    uint32_t gpio_id = (uint32_t)(handle->user_data);
    
    KeyEventMsg_t msg = {
        .gpio_id = gpio_id,
        .action = DOUBLE_PRESS
    };

    ret = rtos_push_to_queue(&s_key_msgqueue, &msg, 1000);

    if (kNoErr != ret){
		LOGI("key send msg failed");
	}

     
}

void long_press_cb(void *param) {
    LOGI("enter long press cb\r\n");
    bk_err_t ret;

    BUTTON_S * handle = (BUTTON_S *)param;
    uint32_t gpio_id = (uint32_t)(handle->user_data);

    KeyEventMsg_t msg = {
        .gpio_id = gpio_id,
        .action = LONG_PRESS
    };

    ret = rtos_push_to_queue(&s_key_msgqueue, &msg, 1000);

    if (kNoErr != ret){
		LOGI("key send msg failed");
	}
    
   
}

void hold_press_cb(void *param) {
    bk_err_t ret;
    uint32_t now_ms = rtos_get_time();

    /* Runs in the key timer context, fired every tick while held.
     * Throttle to a repeat interval and never block the timer callback. */
    if ((s_last_hold_ms != 0) &&
        ((now_ms - s_last_hold_ms) < HOLD_REPEAT_INTERVAL_MS)) {
        return;
    }
    s_last_hold_ms = now_ms;

    BUTTON_S * handle = (BUTTON_S *)param;
    uint32_t gpio_id = (uint32_t)(handle->user_data);

    KeyEventMsg_t msg = {
        .gpio_id = gpio_id,
        .action = HOLD_PRESS
    };

    ret = rtos_push_to_queue(&s_key_msgqueue, &msg, 0);

    if (kNoErr != ret){
		LOGI("key send msg failed");
	}
}

void long_press_up_cb(void *param) {
    LOGI("enter long press up cb\r\n");
    bk_err_t ret;

    BUTTON_S * handle = (BUTTON_S *)param;
    uint32_t gpio_id = (uint32_t)(handle->user_data);

    KeyEventMsg_t msg = {
        .gpio_id = gpio_id,
        .action = LONG_PRESS_UP
    };

    ret = rtos_push_to_queue(&s_key_msgqueue, &msg, 1000);

    if (kNoErr != ret){
		LOGI("key send msg failed");
	}
    
   
}

/* ======================== ADC + GPIO Key Integration ======================== */

#if CONFIG_ADC_KEY

/*
 * ADC Key callbacks (KEY2: S4, S5 via GPIO28/ADC4)
 * These fire from the adc_key timer context, dispatch to global_handler.
 */
static void adckey_s4_short_cb(void *param) {
    LOGI("ADC KEY S4 short press\r\n");
    if(global_handler) global_handler(ADC_KEY_S4_SHORT);
}
static void adckey_s4_double_cb(void *param) {
    LOGI("ADC KEY S4 double press\r\n");
    if(global_handler) global_handler(ADC_KEY_S4_DOUBLE);
}
static void adckey_s4_long_cb(void *param) {
    LOGI("ADC KEY S4 long press\r\n");
    if(global_handler) global_handler(ADC_KEY_S4_LONG);
}
static void adckey_s5_short_cb(void *param) {
    LOGI("ADC KEY S5 short press\r\n");
    if(global_handler) global_handler(ADC_KEY_S5_SHORT);
}
static void adckey_s5_double_cb(void *param) {
    LOGI("ADC KEY S5 double press\r\n");
    if(global_handler) global_handler(ADC_KEY_S5_DOUBLE);
}
static void adckey_s5_long_cb(void *param) {
    LOGI("ADC KEY S5 long press\r\n");
    if(global_handler) global_handler(ADC_KEY_S5_LONG);
}

/*
 * GPIO Key callbacks (KEY1: GPIO39, any-press of S2/S3)
 * Fires from the multi_button timer context.
 */
static void gpio_key1_short_cb(void *param) {
    LOGI("GPIO KEY1 any short press\r\n");
    if(global_handler) global_handler(GPIO_KEY1_ANY_SHORT);
}
static void gpio_key1_double_cb(void *param) {
    LOGI("GPIO KEY1 any double press\r\n");
    if(global_handler) global_handler(GPIO_KEY1_ANY_DOUBLE);
}
static void gpio_key1_long_cb(void *param) {
    LOGI("GPIO KEY1 any long press\r\n");
    if(global_handler) global_handler(GPIO_KEY1_ANY_LONG);
}

/* ADC voltage ranges for KEY2 buttons (configurable via Kconfig) */
#define S4_VOLTAGE_LOW    CONFIG_ADC_KEY_S4_VOLTAGE_LOW
#define S4_VOLTAGE_HIGH   CONFIG_ADC_KEY_S4_VOLTAGE_HIGH
#define S5_VOLTAGE_LOW    CONFIG_ADC_KEY_S5_VOLTAGE_LOW
#define S5_VOLTAGE_HIGH   CONFIG_ADC_KEY_S5_VOLTAGE_HIGH

void bk_all_keys_init(key_handler_t handler)
{
    bk_key_register_event_handler(handler);

    /* --- KEY2: ADC key (S4, S5) via GPIO28/ADC4 --- */
    bk_adc_key_init(ADC_KEY2_GPIO_ID, ADC_KEY2_SADC_CHAN_ID);

    adckey_configure_t s4_config = {
        .lowest_level = S4_VOLTAGE_LOW,
        .highest_level = S4_VOLTAGE_HIGH,
        .user_index = ADCKEY_S4,
        .short_press_cb = adckey_s4_short_cb,
        .double_press_cb = adckey_s4_double_cb,
        .long_press_cb = adckey_s4_long_cb,
        .hold_press_cb = NULL,
    };
    LOGI("S4 config: range=%d~%dmV index=%d short_cb=%p\r\n",
         s4_config.lowest_level, s4_config.highest_level,
         s4_config.user_index, s4_config.short_press_cb);
    bk_adckey_item_configure(&s4_config);

    adckey_configure_t s5_config = {
        .lowest_level = S5_VOLTAGE_LOW,
        .highest_level = S5_VOLTAGE_HIGH,
        .user_index = ADCKEY_S5,
        .short_press_cb = adckey_s5_short_cb,
        .double_press_cb = adckey_s5_double_cb,
        .long_press_cb = adckey_s5_long_cb,
        .hold_press_cb = NULL,
    };
    LOGI("S5 config: range=%d~%dmV index=%d short_cb=%p\r\n",
         s5_config.lowest_level, s5_config.highest_level,
         s5_config.user_index, s5_config.short_press_cb);
    bk_adckey_item_configure(&s5_config);

    /* --- KEY1: GPIO key (S2+S3, any-press) via GPIO39 --- */
#if !CONFIG_ADC_KEY_DUAL_CHANNEL
    bk_init_keys();
    bk_gpio_key_init(GPIO_KEY1_GPIO_ID, GPIO_KEY1_ACTIVE_LEVEL);

    gpio_key_configure_t gpio_key1_config = {
        .gpio_id = GPIO_KEY1_GPIO_ID,
        .active_level = GPIO_KEY1_ACTIVE_LEVEL,
        .short_press_cb = gpio_key1_short_cb,
        .double_press_cb = gpio_key1_double_cb,
        .long_press_cb = gpio_key1_long_cb,
        .hold_press_cb = NULL,
    };
    bk_gpio_key_configure(&gpio_key1_config);
#else
    /*
     * Dual ADC channel mode: KEY1 is also on ADC.
     * TODO: When PCB is fixed, add KEY1 ADC channel item configs here.
     */
    LOGI("Dual ADC mode: KEY1 uses ADC channel %d\r\n", ADC_KEY1_SADC_CHAN_ID);
#endif

    LOGI("All keys initialized\r\n");
}

void bk_all_keys_deinit(void)
{
    bk_adc_key_deinit();

#if !CONFIG_ADC_KEY_DUAL_CHANNEL
    bk_gpio_key_deinit();
    bk_deinit_keys();
#endif

    LOGI("All keys deinitialized\r\n");
}

#endif /* CONFIG_ADC_KEY */