/**
 * @brief Application event module for rock-paper-scissors game.
 *
 * This module provides:
 *  - Application work state machine (IDLE/GAME).
 *  - Event queue and task for processing game events.
 *  - Prompt tone playback based on bk_player_service and PCM array resources.
 *  - Simple game result calculation helper.
 *
 * All interfaces are pure C so that both C and C++ code can use them.
 */

#pragma once

#include <common/bk_include.h>
#include <os/os.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Application work state.
 */
typedef enum
{
    APP_STATE_IDLE = 0,
    APP_STATE_GAME,
} app_state_t;

/**
 * @brief Game result from current round.
 */
typedef enum
{
    APP_GAME_RESULT_UNKNOWN = 0,
    APP_GAME_RESULT_WIN,
    APP_GAME_RESULT_LOSS,
    APP_GAME_RESULT_DRAW,
} app_game_result_t;

/**
 * @brief Game mode type.
 *
 * APP_GAME_MODE_SINGLE:
 *   - One gesture round is played for each start trigger.
 *   - After the round finishes (including can-not-detect path), app returns to IDLE.
 *
 * APP_GAME_MODE_LOOP:
 *   - One gesture round is played for each start trigger.
 *   - After a round finishes, a new round will be started automatically until GAME_END is posted.
 */
typedef enum
{
    APP_GAME_MODE_SINGLE = 0,
    APP_GAME_MODE_LOOP   = 1,
} app_game_mode_t;

/**
 * @brief Default game mode for app_event module.
 *
 * User can override this macro at build time by defining APP_EVENT_DEFAULT_GAME_MODE
 * to APP_GAME_MODE_SINGLE or APP_GAME_MODE_LOOP.
 */
#ifndef APP_EVENT_DEFAULT_GAME_MODE
#define APP_EVENT_DEFAULT_GAME_MODE  APP_GAME_MODE_LOOP
#endif

/**
 * @brief Application event id.
 */
typedef enum
{
    APP_EVENT_NONE = 0,

    APP_EVENT_GAME_END,        /**< Game finished, back to idle state. */
    APP_EVENT_GAME_RESULT,     /**< Game result, parameter is app_game_result_t. */
    APP_EVENT_PROMPT_FINISH,   /**< Prompt tone playback finished, destroy player. */

    /* Prompt tone playback events - sent when each prompt starts playing */
    APP_EVENT_PROMPT_CANT_DET,        /**< Playing: 未检测到手势 */
    APP_EVENT_PROMPT_DETECTING,       /**< Playing: 正在检测中 */
    APP_EVENT_PROMPT_GET_READY,       /**< Playing: 准备猜拳，请出拳，321 */
    APP_EVENT_PROMPT_MY_SHOW,         /**< Playing: 我出的是 */
    APP_EVENT_PROMPT_PAPER,           /**< Playing: 布 */
    APP_EVENT_PROMPT_PLS_STOP_HAND,   /**< Playing: 请收手 */
    APP_EVENT_PROMPT_ROCK,            /**< Playing: 石头 */
    APP_EVENT_PROMPT_SCISSORS,        /**< Playing: 剪刀 */
    APP_EVENT_PROMPT_YOU_LOSS,        /**< Playing: 你输了 */
    APP_EVENT_PROMPT_YOU_WIN,         /**< Playing: 你赢了 */
    APP_EVENT_PROMPT_YOUR_SHOW,       /**< Playing: 你出的是 */
    APP_EVENT_PROMPT_DRAW,            /**< Playing: 平局 */
    APP_EVENT_PROMPT_GAME_RULE,       /**< Playing: 游戏规则 */
} app_event_id_t;

/**
 * @brief Application event message.
 */
typedef struct
{
    app_event_id_t id;
    uint32_t param;        /**< Optional parameter, meaning depends on event id. */
} app_event_msg_t;

/**
 * @brief Initialize application event module.
 *
 * This will:
 *  - Create internal event queue and task.
 *  - Initialize prompt tone player based on bk_player_service.
 *
 * @return
 *  - BK_OK on success.
 *  - Other error codes on failure.
 */
bk_err_t app_event_init(void);

/**
 * @brief Post an application event.
 *
 * @param event_id Event id.
 * @param param    Optional parameter, will be delivered to event task.
 *
 * @return
 *  - BK_OK on success.
 *  - BK_FAIL if queue not ready or full.
 */
bk_err_t app_event_post(app_event_id_t event_id, uint32_t param);

/**
 * @brief Notify gesture result to the game logic.
 *
 * This function is designed to be called from gesture detection model.
 * It should be lightweight and only post events or update simple state.
 * C++ callers may pass (int)result; C uses gesture_result_t.
 *
 * @param gesture Gesture value: 0=ROCK, 1=PAPER, 2=SCISSORS, 3=NONE (no gesture), others>=GESTURE_MAX (invalid).
 */
void app_event_on_gesture(int gesture);

/**
 * @brief Notify image data from gesture detection model.
 *
 * This function is designed to be called from gesture detection model's image callback.
 * It will backup the image if needed (valid gesture detected before timeout,
 * or after timeout for display).
 *
 * @param image_data Pointer to image pixel data.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param format Pixel format (bk_pixel_format_t).
 * @param data_size Total size of image data in bytes.
 */
void app_event_on_image(const uint8_t *image_data, uint32_t width, uint32_t height, uint32_t format, uint32_t data_size);

/**
 * @brief Set current game mode.
 *
 * @param mode Game mode, must be APP_GAME_MODE_SINGLE or APP_GAME_MODE_LOOP.
 */
void app_event_set_game_mode(app_game_mode_t mode);

/**
 * @brief Get current game mode.
 *
 * @return Current game mode.
 */
app_game_mode_t app_event_get_game_mode(void);

/**
 * @brief Initialize test CLI commands for app_event module.
 *
 * This will register CLI commands like rps_start, rps_end, etc.
 * for testing the app_event module without physical buttons.
 *
 * @return 0 on success, non-zero on failure.
 */
int app_event_test_cli_init(void);

/**
 * @brief Start gesture detection inference.
 *
 * This will open camera and start inference thread.
 * Should be called when game starts.
 *
 * @return 0 on success, negative on error.
 */
int gesture_detection_start(void);

/**
 * @brief Stop gesture detection inference.
 *
 * This will stop inference thread and close camera.
 * Should be called when gesture detection is no longer needed.
 *
 * @return 0 on success, negative on error.
 */
int gesture_detection_stop(void);

#ifdef __cplusplus
}
#endif

