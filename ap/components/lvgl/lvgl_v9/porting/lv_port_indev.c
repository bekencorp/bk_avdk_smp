/**
 * @file lv_port_indev.c
 *
 */

/*Copy this file as "lv_port_indev.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "os/os.h"
#include "lv_port_indev.h"
#include "driver/drv_tp.h"
#include "lv_vendor.h"

#ifndef CONFIG_LVGL_USE_KEYPAD
#define CONFIG_LVGL_USE_KEYPAD 0
#endif

/*********************
 *      DEFINES
 *********************/
#define INDEV_RESET_TIMEOUT_COUNT (5)  // timeout = INDEV_RESET_TIMEOUT_COUNT * LV_INDEV_DEF_READ_PERIOD
#define KEYPAD_FIFO_SIZE          16

/**********************
 *      TYPEDEFS
 **********************/
#if CONFIG_LVGL_USE_KEYPAD
typedef struct {
    uint32_t key;
    lv_indev_state_t state;
} keypad_sample_t;
#endif

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void touchpad_init(void);
static void touchpad_read(lv_indev_t * indev, lv_indev_data_t * data);
#if CONFIG_LVGL_USE_KEYPAD
static void keypad_init(void);
static void keypad_read(lv_indev_t * indev, lv_indev_data_t * data);
#endif

int __attribute__((weak)) drv_tp_read(tp_point_infor_t *point)
{
    return kGeneralErr;
}

#if 0
static void mouse_init(void);
static void mouse_read(lv_indev_t * indev, lv_indev_data_t * data);
static bool mouse_is_pressed(void);
static void mouse_get_xy(int32_t * x, int32_t * y);

static void keypad_init(void);
static void keypad_read(lv_indev_t * indev, lv_indev_data_t * data);
static uint32_t keypad_get_key(void);

static void encoder_init(void);
static void encoder_read(lv_indev_t * indev, lv_indev_data_t * data);
static void encoder_handler(void);

static void button_init(void);
static void button_read(lv_indev_t * indev, lv_indev_data_t * data);
static int8_t button_get_pressed_id(void);
static bool button_is_pressed(uint8_t id);
#endif

/**********************
 *  STATIC VARIABLES
 **********************/
lv_indev_t * indev_touchpad;
#if CONFIG_LVGL_USE_KEYPAD
static lv_indev_t *indev_keypad;
static lv_group_t *s_keypad_default_group;
static lv_group_t *s_keypad_active_group;
static keypad_sample_t s_keypad_fifo[KEYPAD_FIFO_SIZE];
static uint8_t s_keypad_fifo_head;
static uint8_t s_keypad_fifo_tail;
static uint8_t s_keypad_fifo_count;
#endif
extern lv_vnd_config_t vendor_config;

#if 0
lv_indev_t * indev_mouse;
lv_indev_t * indev_keypad_example;
lv_indev_t * indev_encoder;
lv_indev_t * indev_button;

static int32_t encoder_diff;
static lv_indev_state_t encoder_state;
#endif

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_indev_init(void)
{
    /**
     * Here you will find example implementation of input devices supported by LittelvGL:
     *  - Touchpad
     *  - Mouse (with cursor support)
     *  - Keypad (supports GUI usage only with key)
     *  - Encoder (supports GUI usage only with: left, right, push)
     *  - Button (external buttons to press points on the screen)
     *
     *  The `..._read()` function are only examples.
     *  You should shape them according to your hardware
     */

    /*------------------
     * Touchpad
     * -----------------*/

    /*Initialize your touchpad if you have*/
    touchpad_init();

    /*Register a touchpad input device*/
    indev_touchpad = lv_indev_create();
    lv_indev_set_type(indev_touchpad, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_touchpad, touchpad_read);

#if CONFIG_LVGL_USE_KEYPAD
    keypad_init();
    indev_keypad = lv_indev_create();
    if (indev_keypad != NULL) {
        lv_indev_set_type(indev_keypad, LV_INDEV_TYPE_KEYPAD);
        lv_indev_set_read_cb(indev_keypad, keypad_read);
        lv_indev_set_group(indev_keypad, s_keypad_active_group);
    }
#endif

#if 0
    /*------------------
     * Mouse
     * -----------------*/

    /*Initialize your mouse if you have*/
    mouse_init();

    /*Register a mouse input device*/
    indev_mouse = lv_indev_create();
    lv_indev_set_type(indev_mouse, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_mouse, mouse_read);

    /*Set cursor. For simplicity set a HOME symbol now.*/
    lv_obj_t * mouse_cursor = lv_image_create(lv_screen_active());
    lv_image_set_src(mouse_cursor, LV_SYMBOL_HOME);
    lv_indev_set_cursor(indev_mouse, mouse_cursor);

    /*------------------
     * Encoder
     * -----------------*/

    /*Initialize your encoder if you have*/
    encoder_init();

    /*Register a encoder input device*/
    indev_encoder = lv_indev_create();
    lv_indev_set_type(indev_encoder, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(indev_encoder, encoder_read);

    /*Later you should create group(s) with `lv_group_t * group = lv_group_create()`,
     *add objects to the group with `lv_group_add_obj(group, obj)`
     *and assign this input device to group to navigate in it:
     *`lv_indev_set_group(indev_encoder, group);`*/

    /*------------------
     * Button
     * -----------------*/

    /*Initialize your button if you have*/
    button_init();

    /*Register a button input device*/
    indev_button = lv_indev_create();
    lv_indev_set_type(indev_button, LV_INDEV_TYPE_BUTTON);
    lv_indev_set_read_cb(indev_button, button_read);

    /*Assign buttons to points on the screen*/
    static const lv_point_t btn_points[2] = {
        {10, 10},   /*Button 0 -> x:10; y:10*/
        {40, 100},  /*Button 1 -> x:40; y:100*/
    };
    lv_indev_set_button_points(indev_button, btn_points);
#endif
}

void lv_port_indev_deinit(void)
{
#if CONFIG_LVGL_USE_KEYPAD
    if (indev_keypad != NULL) {
        lv_indev_delete(indev_keypad);
        indev_keypad = NULL;
    }
#endif
    if (indev_touchpad != NULL) {
        lv_indev_delete(indev_touchpad);
        indev_touchpad = NULL;
    }
#if CONFIG_LVGL_USE_KEYPAD
    if (s_keypad_default_group != NULL) {
        if (lv_group_get_default() == s_keypad_default_group) {
            lv_group_set_default(NULL);
        }
        lv_group_delete(s_keypad_default_group);
        s_keypad_default_group = NULL;
        s_keypad_active_group = NULL;
    }
    lv_port_keypad_reset();
#endif
}

#if CONFIG_LVGL_USE_KEYPAD
bk_err_t lv_port_keypad_send_key(uint32_t key)
{
    GLOBAL_INT_DECLARATION();

    if (key == 0) {
        return BK_FAIL;
    }

    GLOBAL_INT_DISABLE();
    if (s_keypad_fifo_count > (KEYPAD_FIFO_SIZE - 2)) {
        GLOBAL_INT_RESTORE();
        return BK_ERR_NO_MEM;
    }

    s_keypad_fifo[s_keypad_fifo_tail].key = key;
    s_keypad_fifo[s_keypad_fifo_tail].state = LV_INDEV_STATE_PRESSED;
    s_keypad_fifo_tail = (s_keypad_fifo_tail + 1) % KEYPAD_FIFO_SIZE;
    s_keypad_fifo_count++;

    s_keypad_fifo[s_keypad_fifo_tail].key = key;
    s_keypad_fifo[s_keypad_fifo_tail].state = LV_INDEV_STATE_RELEASED;
    s_keypad_fifo_tail = (s_keypad_fifo_tail + 1) % KEYPAD_FIFO_SIZE;
    s_keypad_fifo_count++;
    GLOBAL_INT_RESTORE();

    return BK_OK;
}

bk_err_t lv_port_keypad_send_key_state(uint32_t key, lv_indev_state_t state)
{
    GLOBAL_INT_DECLARATION();

    if (key == 0) {
        return BK_FAIL;
    }
    if (state != LV_INDEV_STATE_PRESSED && state != LV_INDEV_STATE_RELEASED) {
        return BK_FAIL;
    }

    GLOBAL_INT_DISABLE();
    if (s_keypad_fifo_count >= KEYPAD_FIFO_SIZE) {
        GLOBAL_INT_RESTORE();
        return BK_ERR_NO_MEM;
    }
    s_keypad_fifo[s_keypad_fifo_tail].key = key;
    s_keypad_fifo[s_keypad_fifo_tail].state = state;
    s_keypad_fifo_tail = (s_keypad_fifo_tail + 1) % KEYPAD_FIFO_SIZE;
    s_keypad_fifo_count++;
    GLOBAL_INT_RESTORE();

    return BK_OK;
}

void lv_port_keypad_reset(void)
{
    GLOBAL_INT_DECLARATION();

    GLOBAL_INT_DISABLE();
    s_keypad_fifo_head = 0;
    s_keypad_fifo_tail = 0;
    s_keypad_fifo_count = 0;
    GLOBAL_INT_RESTORE();
}

lv_indev_t *lv_port_keypad_get_indev(void)
{
    return indev_keypad;
}

lv_group_t *lv_port_keypad_get_default_group(void)
{
    return s_keypad_default_group;
}

bk_err_t lv_port_keypad_set_group(lv_group_t *group)
{
    lv_group_t *target = group != NULL ? group : s_keypad_default_group;

    if (target == NULL) {
        return BK_FAIL;
    }

    s_keypad_active_group = target;
    if (indev_keypad != NULL) {
        lv_indev_set_group(indev_keypad, target);
    }

    return BK_OK;
}

#else

bk_err_t lv_port_keypad_send_key(uint32_t key)
{
    (void)key;
    return BK_ERR_NOT_SUPPORT;
}

bk_err_t lv_port_keypad_send_key_state(uint32_t key, lv_indev_state_t state)
{
    (void)key;
    (void)state;
    return BK_ERR_NOT_SUPPORT;
}

void lv_port_keypad_reset(void)
{
}

lv_indev_t *lv_port_keypad_get_indev(void)
{
    return NULL;
}

lv_group_t *lv_port_keypad_get_default_group(void)
{
    return NULL;
}

bk_err_t lv_port_keypad_set_group(lv_group_t *group)
{
    (void)group;
    return BK_ERR_NOT_SUPPORT;
}

#endif

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*------------------
 * Touchpad
 * -----------------*/

/*Initialize your touchpad*/
static void touchpad_init(void)
{
    /*Your code comes here*/
}

/*Will be called by the library to read the touchpad*/
static void touchpad_read(lv_indev_t * indev_drv, lv_indev_data_t * data)
{
	int ret = kNoErr;
	static uint8_t indev_reset_count = 0;
    static int32_t last_x = 0;
    static int32_t last_y = 0;
    static lv_indev_state_t last_state = LV_INDEV_STATE_RELEASED;
    tp_point_infor_t point;

    do {
        ret = drv_tp_read(&point);
        if (kNoErr != ret)
        {
            if (LV_INDEV_STATE_REL != last_state)
            {
                indev_reset_count++;
                if (indev_reset_count >= INDEV_RESET_TIMEOUT_COUNT)
                {
                    indev_reset_count = 0;
                    last_state = LV_INDEV_STATE_REL;
                    last_x = 0;
                    last_y = 0;
                }
            }
            break;
        }

        indev_reset_count = 0;

        last_x = point.m_x;
        last_y = point.m_y;
        last_state = point.m_state? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;

        if (point.m_need_continue)
        {
            data->continue_reading = true;
        }
    } while(0);

    /*Set the last pressed coordinates*/
    data->point.x = last_x;
    data->point.y = last_y;
    data->state = last_state;
}

/*------------------
 * Keypad
 * -----------------*/

#if CONFIG_LVGL_USE_KEYPAD
static bool keypad_fifo_pop(keypad_sample_t *sample)
{
    GLOBAL_INT_DECLARATION();

    if (sample == NULL) {
        return false;
    }

    GLOBAL_INT_DISABLE();
    if (s_keypad_fifo_count == 0) {
        GLOBAL_INT_RESTORE();
        return false;
    }

    *sample = s_keypad_fifo[s_keypad_fifo_head];
    s_keypad_fifo_head = (s_keypad_fifo_head + 1) % KEYPAD_FIFO_SIZE;
    s_keypad_fifo_count--;
    GLOBAL_INT_RESTORE();

    return true;
}

static bool keypad_fifo_has_pending(void)
{
    GLOBAL_INT_DECLARATION();
    bool has_pending;

    GLOBAL_INT_DISABLE();
    has_pending = (s_keypad_fifo_count > 0);
    GLOBAL_INT_RESTORE();

    return has_pending;
}

static void keypad_init(void)
{
    lv_port_keypad_reset();

    if (s_keypad_default_group == NULL) {
        s_keypad_default_group = lv_group_create();
    }
    s_keypad_active_group = s_keypad_default_group;

    if (s_keypad_default_group != NULL && lv_group_get_default() == NULL) {
        lv_group_set_default(s_keypad_default_group);
    }
}

static void keypad_read(lv_indev_t * indev, lv_indev_data_t * data)
{
    static uint32_t last_key = LV_KEY_ENTER;
    keypad_sample_t sample;

    (void)indev;

    if (keypad_fifo_pop(&sample)) {
        last_key = sample.key;
        data->key = sample.key;
        data->state = sample.state;
        data->continue_reading = keypad_fifo_has_pending();
        return;
    }

    data->key = last_key;
    data->state = LV_INDEV_STATE_RELEASED;
    data->continue_reading = false;
}
#endif

#if 0
/*------------------
 * Mouse
 * -----------------*/

/*Initialize your mouse*/
static void mouse_init(void)
{
    /*Your code comes here*/
}

/*Will be called by the library to read the mouse*/
static void mouse_read(lv_indev_t * indev_drv, lv_indev_data_t * data)
{
    /*Get the current x and y coordinates*/
    mouse_get_xy(&data->point.x, &data->point.y);

    /*Get whether the mouse button is pressed or released*/
    if(mouse_is_pressed()) {
        data->state = LV_INDEV_STATE_PRESSED;
    }
    else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

/*Return true is the mouse button is pressed*/
static bool mouse_is_pressed(void)
{
    /*Your code comes here*/

    return false;
}

/*Get the x and y coordinates if the mouse is pressed*/
static void mouse_get_xy(int32_t * x, int32_t * y)
{
    /*Your code comes here*/

    (*x) = 0;
    (*y) = 0;
}

/*------------------
 * Keypad
 * -----------------*/

/*Initialize your keypad*/
static void keypad_init(void)
{
    /*Your code comes here*/
}

/*Will be called by the library to read the mouse*/
static void keypad_read(lv_indev_t * indev_drv, lv_indev_data_t * data)
{
    static uint32_t last_key = 0;

    /*Get the current x and y coordinates*/
    mouse_get_xy(&data->point.x, &data->point.y);

    /*Get whether the a key is pressed and save the pressed key*/
    uint32_t act_key = keypad_get_key();
    if(act_key != 0) {
        data->state = LV_INDEV_STATE_PRESSED;

        /*Translate the keys to LVGL control characters according to your key definitions*/
        switch(act_key) {
            case 1:
                act_key = LV_KEY_NEXT;
                break;
            case 2:
                act_key = LV_KEY_PREV;
                break;
            case 3:
                act_key = LV_KEY_LEFT;
                break;
            case 4:
                act_key = LV_KEY_RIGHT;
                break;
            case 5:
                act_key = LV_KEY_ENTER;
                break;
        }

        last_key = act_key;
    }
    else {
        data->state = LV_INDEV_STATE_RELEASED;
    }

    data->key = last_key;
}

/*Get the currently being pressed key.  0 if no key is pressed*/
static uint32_t keypad_get_key(void)
{
    /*Your code comes here*/

    return 0;
}

/*------------------
 * Encoder
 * -----------------*/

/*Initialize your encoder*/
static void encoder_init(void)
{
    /*Your code comes here*/
}

/*Will be called by the library to read the encoder*/
static void encoder_read(lv_indev_t * indev_drv, lv_indev_data_t * data)
{

    data->enc_diff = encoder_diff;
    data->state = encoder_state;
}

/*Call this function in an interrupt to process encoder events (turn, press)*/
static void encoder_handler(void)
{
    /*Your code comes here*/

    encoder_diff += 0;
    encoder_state = LV_INDEV_STATE_RELEASED;
}

/*------------------
 * Button
 * -----------------*/

/*Initialize your buttons*/
static void button_init(void)
{
    /*Your code comes here*/
}

/*Will be called by the library to read the button*/
static void button_read(lv_indev_t * indev_drv, lv_indev_data_t * data)
{

    static uint8_t last_btn = 0;

    /*Get the pressed button's ID*/
    int8_t btn_act = button_get_pressed_id();

    if(btn_act >= 0) {
        data->state = LV_INDEV_STATE_PRESSED;
        last_btn = btn_act;
    }
    else {
        data->state = LV_INDEV_STATE_RELEASED;
    }

    /*Save the last pressed button's ID*/
    data->btn_id = last_btn;
}

/*Get ID  (0, 1, 2 ..) of the pressed button*/
static int8_t button_get_pressed_id(void)
{
    uint8_t i;

    /*Check to buttons see which is being pressed (assume there are 2 buttons)*/
    for(i = 0; i < 2; i++) {
        /*Return the pressed button's ID*/
        if(button_is_pressed(i)) {
            return i;
        }
    }

    /*No button pressed*/
    return -1;
}

/*Test if `id` button is pressed or not*/
static bool button_is_pressed(uint8_t id)
{

    /*Your code comes here*/

    return false;
}
#endif

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
