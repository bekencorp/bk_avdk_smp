/**
 * @file lv_port_indev_templ.c
 *
 */

 /*Copy this file as "lv_port_indev.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include <os/os.h>
#include "lv_port_indev.h"
#include "driver/drv_tp.h"
#include "lv_port_disp.h"

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
static void touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);
#if CONFIG_LVGL_USE_KEYPAD
static void keypad_init(void);
static void keypad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);
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

/*Initialize your touchpad*/
static void touchpad_init(void)
{
    /*Your code comes here*/

}

int __attribute__((weak)) drv_tp_read(tp_point_infor_t *point)
{
    return kGeneralErr;
}

/*Will be called by the library to read the touchpad*/
static void touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    static uint8_t indev_reset_count = 0;
    static lv_coord_t last_x = 0;
    static lv_coord_t last_y = 0;
    static lv_indev_state_t last_state = LV_INDEV_STATE_REL;
    tp_point_infor_t point;
    int ret = kNoErr;

    do{
        ret = drv_tp_read(&point);
        if(kNoErr != ret)
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
        last_state = point.m_state? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;

        if(point.m_need_continue)
        {
            data->continue_reading = true;
        }
    } while(0);

    /*Set the last pressed coordinates*/
    data->point.x = last_x;
    data->point.y = last_y;
    data->state = last_state;
}


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
    static lv_indev_drv_t indev_drv;
#if CONFIG_LVGL_USE_KEYPAD
    static lv_indev_drv_t keypad_drv;
#endif
    /*------------------
     * Touchpad
     * -----------------*/

    /*Initialize your touchpad if you have*/
    touchpad_init();

    /*Register a touchpad input device*/
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    indev_touchpad = lv_indev_drv_register(&indev_drv);

#if CONFIG_LVGL_USE_KEYPAD
    keypad_init();
    lv_indev_drv_init(&keypad_drv);
    keypad_drv.type = LV_INDEV_TYPE_KEYPAD;
    keypad_drv.read_cb = keypad_read;
    indev_keypad = lv_indev_drv_register(&keypad_drv);
    if (indev_keypad != NULL) {
        lv_indev_set_group(indev_keypad, s_keypad_active_group);
    }
#endif
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*------------------
 * Touchpad
 * -----------------*/
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
        lv_group_del(s_keypad_default_group);
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

static void keypad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    static uint32_t last_key = LV_KEY_ENTER;
    keypad_sample_t sample;

    (void)indev_drv;

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

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
