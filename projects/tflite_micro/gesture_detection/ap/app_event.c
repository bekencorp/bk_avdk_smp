#include <common/bk_include.h>
#include <os/os.h>
#include <stdlib.h>
#include <driver/trng.h>

#include <components/bk_player_service.h>
#include <components/bk_player_service_types.h>

#include "tflm_gesture_detection.h"
#include "app_event.h"
#include "resource/prompt_tone/resource.h"
#include "common/avdk_pixel_types.h"

#include "lv_vendor.h"
#include "beken_ui.h"
#include "lvgl.h"

/**
 * @brief Internal log tag.
 */
#define TAG "app-event"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

/**
 * @brief Maximum length of event queue.
 */
#define APP_EVENT_QUEUE_LENGTH   (10)

/**
 * @brief Delay (ms) after game result is generated.
 *
 * This gives user some time to see the result on screen
 * before we declare game end.
 */
#define APP_EVENT_GAME_END_DELAY_MS   (400)

/**
 * @brief Game internal phase for one round.
 */
typedef enum
{
    GAME_PHASE_IDLE = 0,                 /**< No active game flow */
    GAME_PHASE_WAIT_FIRST_DETECTING_FINISH, /**< First detecting_wav is playing, wait for finish */
    GAME_PHASE_WAIT_GESTURE,             /**< 3s window: accept gesture result */
    GAME_PHASE_WAIT_PLS_STOP_FINISH,     /**< pls_stop_hand_wav is playing */
    GAME_PHASE_WAIT_SECOND_DETECTING_FINISH, /**< Second detecting_wav is playing */
    GAME_PHASE_PLAY_SEQUENCE,            /**< Playing result prompt sequence */
    GAME_PHASE_WAIT_CANT_DET_FINISH,     /**< cant_det_wav is playing */
} app_game_phase_t;

/**
 * @brief Application context.
 */
typedef struct
{
    beken_thread_t    event_task;
    beken_queue_t     event_queue;
    app_state_t       state;

    /* Game mode control: single round or loop mode */
    app_game_mode_t   game_mode;

    /* Prompt player handle */
    bk_player_handle_t player;

    /* Game flow state */
    app_game_phase_t  phase;

    /* Gesture window control */
    beken2_timer_t    gesture_timer;
    uint8_t           gesture_timer_inited;
    uint8_t           gesture_accept_enabled;
    uint8_t           has_user_gesture;
    uint8_t           gesture_timeout_happened;

    /* Cached gestures and result for current round */
    gesture_result_t  device_gesture;
    gesture_result_t  user_gesture;
    app_game_result_t game_result;

    /* Playback sequence step for result prompts */
    uint8_t           sequence_step;

    /* Image backup for display */
    uint8_t          *backup_image_data;
    uint32_t          backup_image_size;
    uint32_t          backup_image_width;
    uint32_t          backup_image_height;
    uint32_t          backup_image_format;
    uint8_t           need_backup_image;

    /* Converted RGB565 image for LVGL display */
    uint8_t          *rgb565_image_data;
    uint32_t          rgb565_image_size;
    lv_image_dsc_t   rgb565_image_dsc;
} app_event_ctx_t;

static app_event_ctx_t s_app_evt_ctx = {0};
static bk_err_t app_convert_backup_image_to_rgb565(void);

/**
 * @brief Reset game-related context to idle.
 */
static void app_game_reset(void)
{
    s_app_evt_ctx.state = APP_STATE_IDLE;
    s_app_evt_ctx.phase = GAME_PHASE_IDLE;
    s_app_evt_ctx.gesture_accept_enabled = 0;
    s_app_evt_ctx.has_user_gesture = 0;
    s_app_evt_ctx.gesture_timeout_happened = 0;
    s_app_evt_ctx.sequence_step = 0;
    s_app_evt_ctx.need_backup_image = 0;

    /* Free backup image if exists. */
    if (s_app_evt_ctx.backup_image_data != NULL)
    {
        psram_free(s_app_evt_ctx.backup_image_data);
        s_app_evt_ctx.backup_image_data = NULL;
        s_app_evt_ctx.backup_image_size = 0;
    }

    /* Free RGB565 image if exists. */
    if (s_app_evt_ctx.rgb565_image_data != NULL)
    {
        psram_free(s_app_evt_ctx.rgb565_image_data);
        s_app_evt_ctx.rgb565_image_data = NULL;
        s_app_evt_ctx.rgb565_image_size = 0;
    }

    if (s_app_evt_ctx.gesture_timer_inited)
    {
        rtos_stop_oneshot_timer(&s_app_evt_ctx.gesture_timer);
        /* Keep timer inited for reuse. */
    }
}

/**
 * @brief One-shot timer callback: gesture detect timeout (3s).
 *
 * When timeout happens without valid gesture, treat as "can not detect".
 */
static void app_gesture_timeout_handler(void *larg, void *rarg)
{
    /* Only valid in gesture waiting phase. */
    if (s_app_evt_ctx.state == APP_STATE_GAME &&
        s_app_evt_ctx.phase == GAME_PHASE_WAIT_GESTURE &&
        !s_app_evt_ctx.has_user_gesture)
    {
        /* 3s timeout without valid gesture:
         * Do not immediately end the game here. Instead, mark that timeout
         * has happened so that subsequent gesture results (including
         * GESTURE_NONE) can be treated as the final result for this round.
         * Also mark that we need to backup image when next image callback arrives.
         */
        s_app_evt_ctx.gesture_timeout_happened = 1;
        s_app_evt_ctx.need_backup_image = 1;
    }
}

/**
 * @brief Start 500ms gesture detection window.
 */
static void app_start_gesture_window(void)
{
    bk_err_t ret;

    s_app_evt_ctx.gesture_accept_enabled = 1;
    s_app_evt_ctx.has_user_gesture = 0;
    s_app_evt_ctx.gesture_timeout_happened = 0;
    s_app_evt_ctx.need_backup_image = 0;
    s_app_evt_ctx.phase = GAME_PHASE_WAIT_GESTURE;

    if (!s_app_evt_ctx.gesture_timer_inited)
    {
        ret = rtos_init_oneshot_timer(&s_app_evt_ctx.gesture_timer,
                                      500,
                                      app_gesture_timeout_handler,
                                      NULL,
                                      NULL);
        if (ret != BK_OK)
        {
            LOGE("rtos_init_oneshot_timer failed, ret: %d\n", ret);
            return;
        }
        s_app_evt_ctx.gesture_timer_inited = 1;
    }

    ret = rtos_start_oneshot_timer(&s_app_evt_ctx.gesture_timer);
    if (ret != BK_OK)
    {
        LOGE("rtos_start_oneshot_timer failed, ret: %d\n", ret);
    }
}

/**
 * @brief Event callback for player to handle playback finish.
 *
 * This callback is called when playback finishes, and it will post
 * APP_EVENT_PROMPT_FINISH event to app_event queue for cleanup.
 */
static int app_player_event_handler(int event, void *data, void *args)
{
    //BK_UNUSED(data);
    //BK_UNUSED(args);

    if (event == PLAYER_EVENT_FINISH)
    {
        LOGD("playback finished, posting APP_EVENT_PROMPT_FINISH event\n");
        /* Post event to app_event queue for cleanup */
        if (app_event_post(APP_EVENT_PROMPT_FINISH, 0) != BK_OK)
        {
            LOGE("post APP_EVENT_PROMPT_FINISH failed\n");
        }
    }

    return BK_OK;
}

/**
 * @brief Helper to safely stop current playing prompt and destroy player.
 */
static void app_prompt_stop(void)
{
    if (s_app_evt_ctx.player == NULL)
    {
        return;
    }

    /* Stop player first */
    if (bk_player_stop(s_app_evt_ctx.player) != BK_OK)
    {
        LOGW("bk_player_stop failed\n");
    }

    /* Destroy player to release all resources */
    if (bk_player_destroy(s_app_evt_ctx.player) != BK_OK)
    {
        LOGW("bk_player_destroy failed\n");
    }

    s_app_evt_ctx.player = NULL;
}

/**
 * @brief Helper to play one prompt tone from PCM array.
 *
 * Creates a new player instance for each playback to avoid state management issues.
 *
 * @param data PCM data pointer.
 * @param len  PCM data length in bytes.
 *
 * @return BK_OK on success, BK_FAIL on error.
 */
static bk_err_t app_prompt_play(const unsigned char *data, unsigned int len)
{
    if (data == NULL || len == 0)
    {
        LOGE("invalid prompt data, data: %p, len: %u\n", data, len);
        return BK_FAIL;
    }

    /* Stop and destroy previous player if exists */
    app_prompt_stop();

    /* Create a new player for this playback */
    bk_player_cfg_t player_cfg = DEFAULT_PLAYER_WITH_PLAYBACK_CONFIG();
    /* Set event callback to handle playback finish */
    player_cfg.spk_cfg.onboard_spk_cfg.ana_gain   = 0x07;
    player_cfg.spk_cfg.onboard_spk_cfg.pa_ctrl_en = true;
    player_cfg.spk_cfg.onboard_spk_cfg.pa_ctrl_gpio = 29;
    player_cfg.spk_cfg.onboard_spk_cfg.pa_on_level  = 1;
    player_cfg.spk_cfg.onboard_spk_cfg.pa_on_delay  = 2;
    player_cfg.spk_cfg.onboard_spk_cfg.pa_off_delay = 0;
    player_cfg.spk_cfg.onboard_spk_cfg.sample_rate[AUD_DAC_SOURCE_A2DP] = 16000;
    player_cfg.spk_cfg.onboard_spk_cfg.dac_source_bitmap = ONBOARD_SPEAKER_STREAM_DAC_SOURCE_A2DP_BIT;
    player_cfg.event_handle = app_player_event_handler;
    player_cfg.args = NULL;
    s_app_evt_ctx.player = bk_player_create(&player_cfg);
    if (s_app_evt_ctx.player == NULL)
    {
        LOGE("bk_player_create failed\n");
        return BK_FAIL;
    }

    /* All prompt tones are WAV format, set decoder type explicitly. */
    bk_err_t ret = bk_player_set_decode_type(s_app_evt_ctx.player, AUDIO_DEC_TYPE_WAV);
    if (ret != BK_OK)
    {
        LOGE("bk_player_set_decode_type failed, ret: %d\n", ret);
        bk_player_destroy(s_app_evt_ctx.player);
        s_app_evt_ctx.player = NULL;
        return BK_FAIL;
    }

    /* Set URI for the audio data */
    player_uri_info_t uri_info = {0};
    uri_info.uri_type = PLAYER_URI_TYPE_ARRAY;
    uri_info.uri = (char *)data;
    uri_info.total_len = len;

    ret = bk_player_set_uri(s_app_evt_ctx.player, &uri_info);
    if (ret != BK_OK)
    {
        LOGE("bk_player_set_uri failed, ret: %d\n", ret);
        bk_player_destroy(s_app_evt_ctx.player);
        s_app_evt_ctx.player = NULL;
        return BK_FAIL;
    }

    /* Start playback */
    ret = bk_player_start(s_app_evt_ctx.player);
    if (ret != BK_OK)
    {
        LOGE("bk_player_start failed, ret: %d\n", ret);
        bk_player_destroy(s_app_evt_ctx.player);
        s_app_evt_ctx.player = NULL;
        return BK_FAIL;
    }

    return BK_OK;
}

/**
 * @brief Calculate game result by device and user gestures.
 *
 * @param device_gesture  Gesture selected by device.
 * @param user_gesture    Gesture detected from user.
 *
 * @return app_game_result_t Game result.
 */
static app_game_result_t app_calc_game_result(gesture_result_t device_gesture, gesture_result_t user_gesture)
{
    if (device_gesture >= GESTURE_MAX ||
        user_gesture >= GESTURE_MAX ||
        user_gesture == GESTURE_NONE)
    {
        return APP_GAME_RESULT_UNKNOWN;
    }

    if (device_gesture == user_gesture)
    {
        return APP_GAME_RESULT_DRAW;
    }

    /* Check if user wins: rock beats scissors, paper beats rock, scissors beats paper */
    if ((user_gesture == GESTURE_ROCK && device_gesture == GESTURE_SCISSORS) ||
        (user_gesture == GESTURE_PAPER && device_gesture == GESTURE_ROCK) ||
        (user_gesture == GESTURE_SCISSORS && device_gesture == GESTURE_PAPER))
    {
        return APP_GAME_RESULT_WIN;
    }

    /* Otherwise, device wins (user loses) */
    return APP_GAME_RESULT_LOSS;
}

/**
 * @brief Select a random gesture for device.
 *
 * @return gesture_result_t Random gesture.
 */
static gesture_result_t app_random_gesture(void)
{
    /* Use C standard library rand() for random generation. */
    int rand_value = rand();
    int index = rand_value % 3;

    switch (index)
    {
        case 0:
            return GESTURE_ROCK;
        case 1:
            return GESTURE_PAPER;
        case 2:
        default:
            return GESTURE_SCISSORS;
    }
}

/**
 * @brief Handle APP_EVENT_GAME_RESULT event.
 *
 * @param result Game result.
 */
static void app_handle_game_result(app_game_result_t result)
{
    /* Keep for compatibility with CLI: directly play result prompt. */
    const unsigned char *pcm = NULL;
    unsigned int pcm_len = 0;

    switch (result)
    {
        case APP_GAME_RESULT_WIN:
            pcm = you_win_wav;
            pcm_len = you_win_wav_len;
            break;

        case APP_GAME_RESULT_LOSS:
            pcm = you_loss_wav;
            pcm_len = you_loss_wav_len;
            break;

        case APP_GAME_RESULT_DRAW:
            pcm = draw_wav;
            pcm_len = draw_wav_len;
            break;

        default:
            pcm = cant_det_wav;
            pcm_len = cant_det_wav_len;
            break;
    }

    if (app_prompt_play(pcm, pcm_len) != BK_OK)
    {
        LOGE("play game result failed, result: %d\n", result);
    }
}

/**
 * @brief Internal event task function.
 */
static void app_event_task(beken_thread_arg_t param)
{
    //BK_UNUSED(param);

    LOGI("app_event_task start\n");

    s_app_evt_ctx.state = APP_STATE_IDLE;

    while (1)
    {
        app_event_msg_t msg;
        bk_err_t ret = rtos_pop_from_queue(&s_app_evt_ctx.event_queue, &msg, BEKEN_WAIT_FOREVER);
        if (ret != kNoErr)
        {
            LOGE("rtos_pop_from_queue failed, ret: %d\n", ret);
            continue;
        }

        LOGD("event id: %d, param: %u, state: %d\n", msg.id, msg.param, s_app_evt_ctx.state);

        switch (msg.id)
        {
            case APP_EVENT_PROMPT_GAME_RULE:
                /* Only play rule description, no state change. */
                if (app_prompt_play(game_rule_wav, game_rule_wav_len) != BK_OK)
                {
                    LOGE("play game rule prompt failed\n");
                }
                break;

            case APP_EVENT_GAME_RESULT:
                app_handle_game_result((app_game_result_t)msg.param);

                /* After a short delay, declare game end and back to idle (single mode)
                 * or automatically start a new round (loop mode).
                 */
                rtos_delay_milliseconds(APP_EVENT_GAME_END_DELAY_MS);
                app_game_reset();
                if (s_app_evt_ctx.game_mode == APP_GAME_MODE_LOOP)
                {
                    if (app_event_post(APP_EVENT_PROMPT_GET_READY, 0) != BK_OK)
                    {
                        LOGE("post APP_EVENT_PROMPT_GET_READY failed (loop mode, GAME_RESULT)\n");
                    }
                }
                break;

            case APP_EVENT_GAME_END:
                /* Stop current prompt playback immediately */
                LOGD("Game end requested, stopping playback and resetting game\n");
                app_prompt_stop();
                /* Reset all game state and stop timers */
                app_game_reset();
                break;

            case APP_EVENT_PROMPT_FINISH:
                /* Prompt tone playback finished, destroy player to release resources. */
                LOGD("prompt playback finished, destroying player\n");
                app_prompt_stop();

                /* Handle state transitions based on current game phase. */
                switch (s_app_evt_ctx.phase)
                {
                    case GAME_PHASE_WAIT_FIRST_DETECTING_FINISH:
                        /* First detecting_wav finished, start 3s gesture window. */
                        app_start_gesture_window();
                        break;

                    case GAME_PHASE_WAIT_PLS_STOP_FINISH:
                        if (s_app_evt_ctx.has_user_gesture)
                        {
                            /* User gesture exists: play detecting_wav again and then result sequence. */
                            s_app_evt_ctx.phase = GAME_PHASE_WAIT_SECOND_DETECTING_FINISH;
                            if (app_event_post(APP_EVENT_PROMPT_DETECTING, 0) != BK_OK)
                            {
                                LOGE("post APP_EVENT_PROMPT_DETECTING failed\n");
                            }
                        }
                        else
                        {
                            /* No gesture (timeout path): after pls_stop_hand_wav, play cant_det_wav and end game. */
                            if (app_event_post(APP_EVENT_PROMPT_CANT_DET, 0) != BK_OK)
                            {
                                LOGE("post APP_EVENT_PROMPT_CANT_DET failed (timeout path)\n");
                            }
                        }
                        break;

                    case GAME_PHASE_WAIT_SECOND_DETECTING_FINISH:
                        /* Second detecting_wav finished, start result sequence. */
                        s_app_evt_ctx.phase = GAME_PHASE_PLAY_SEQUENCE;
                        s_app_evt_ctx.sequence_step = 0;
                        /* Start sequence: play my_show_wav first. */
                        if (app_event_post(APP_EVENT_PROMPT_MY_SHOW, 0) != BK_OK)
                        {
                            LOGE("post APP_EVENT_PROMPT_MY_SHOW failed\n");
                        }
                        break;

                    case GAME_PHASE_PLAY_SEQUENCE:
                        /* Handle sequence playback step by step. */
                        s_app_evt_ctx.sequence_step++;
                        switch (s_app_evt_ctx.sequence_step)
                        {
                            case 1:
                                /* Step 1: Play device gesture (rock/scissors/paper). */
                                {
                                    app_event_id_t gesture_event = APP_EVENT_NONE;
                                    switch (s_app_evt_ctx.device_gesture)
                                    {
                                        case GESTURE_ROCK:
                                            gesture_event = APP_EVENT_PROMPT_ROCK;
                                            break;
                                        case GESTURE_PAPER:
                                            gesture_event = APP_EVENT_PROMPT_PAPER;
                                            break;
                                        case GESTURE_SCISSORS:
                                            gesture_event = APP_EVENT_PROMPT_SCISSORS;
                                            break;
                                        default:
                                            LOGE("invalid device gesture: %d\n", s_app_evt_ctx.device_gesture);
                                            app_game_reset();
                                            break;
                                    }
                                    if (gesture_event != APP_EVENT_NONE)
                                    {
                                        if (app_event_post(gesture_event, 0) != BK_OK)
                                        {
                                            LOGE("post device gesture event failed\n");
                                        }
                                    }
                                }
                                break;

                            case 2:
                                /* Step 2: Play your_show_wav. */
                                if (app_event_post(APP_EVENT_PROMPT_YOUR_SHOW, 0) != BK_OK)
                                {
                                    LOGE("post APP_EVENT_PROMPT_YOUR_SHOW failed\n");
                                }
                                break;

                            case 3:
                                /* Step 3: Play user gesture (rock/scissors/paper). */
                                {
                                    app_event_id_t gesture_event = APP_EVENT_NONE;
                                    switch (s_app_evt_ctx.user_gesture)
                                    {
                                        case GESTURE_ROCK:
                                            gesture_event = APP_EVENT_PROMPT_ROCK;
                                            break;
                                        case GESTURE_PAPER:
                                            gesture_event = APP_EVENT_PROMPT_PAPER;
                                            break;
                                        case GESTURE_SCISSORS:
                                            gesture_event = APP_EVENT_PROMPT_SCISSORS;
                                            break;
                                        default:
                                            LOGE("invalid user gesture: %d\n", s_app_evt_ctx.user_gesture);
                                            app_game_reset();
                                            break;
                                    }
                                    if (gesture_event != APP_EVENT_NONE)
                                    {
                                        if (app_event_post(gesture_event, 0) != BK_OK)
                                        {
                                            LOGE("post user gesture event failed\n");
                                        }
                                    }
                                }
                                break;

                            case 4:
                                /* Step 4: Play game result (win/loss/draw). */
                                {
                                    app_event_id_t result_event = APP_EVENT_NONE;
                                    switch (s_app_evt_ctx.game_result)
                                    {
                                        case APP_GAME_RESULT_WIN:
                                            result_event = APP_EVENT_PROMPT_YOU_WIN;
                                            break;
                                        case APP_GAME_RESULT_LOSS:
                                            result_event = APP_EVENT_PROMPT_YOU_LOSS;
                                            break;
                                        case APP_GAME_RESULT_DRAW:
                                            result_event = APP_EVENT_PROMPT_DRAW;
                                            break;
                                        default:
                                            LOGE("invalid game result: %d\n", s_app_evt_ctx.game_result);
                                            app_game_reset();
                                            break;
                                    }
                                    if (result_event != APP_EVENT_NONE)
                                    {
                                        if (app_event_post(result_event, 0) != BK_OK)
                                        {
                                            LOGE("post game result event failed\n");
                                        }
                                    }
                                }
                                break;

                            case 5:
                                /* Sequence finished, reset game and return to idle (single mode)
                                 * or automatically start a new round (loop mode).
                                 */
                                LOGD("Game sequence finished\n");
                                app_game_reset();
                                if (s_app_evt_ctx.game_mode == APP_GAME_MODE_LOOP)
                                {
                                    if (app_event_post(APP_EVENT_PROMPT_GET_READY, 0) != BK_OK)
                                    {
                                        LOGE("post APP_EVENT_PROMPT_GET_READY failed (loop mode, sequence)\n");
                                    }
                                }
                                break;

                            default:
                                /* Should not reach here. */
                                LOGE("unexpected sequence step: %d\n", s_app_evt_ctx.sequence_step);
                                app_game_reset();
                                break;
                        }
                        break;

                    case GAME_PHASE_WAIT_CANT_DET_FINISH:
                        /* cant_det_wav finished: treat as game end and restart if in loop mode. */
                        app_game_reset();
                        if (s_app_evt_ctx.game_mode == APP_GAME_MODE_LOOP)
                        {
                            rtos_delay_milliseconds(APP_EVENT_GAME_END_DELAY_MS);
                            if (app_event_post(APP_EVENT_PROMPT_GET_READY, 0) != BK_OK)
                            {
                                LOGE("post APP_EVENT_PROMPT_GET_READY failed (loop mode, cant_det_finish)\n");
                            }
                        }
                        break;

                    default:
                        break;
                }
                break;

            /* Prompt tone playback events - these are sent when each prompt starts playing.
             * They can be used for logging, UI updates, or other side effects.
             * No action needed here as the prompt is already playing.
             */
            case APP_EVENT_PROMPT_CANT_DET:
                LOGD("Playing prompt: cant detect\n");
                /* Game ends on can-not-detect path. We only restart automatically
                 * in loop mode after cant_det_wav playback finishes.
                 */
                s_app_evt_ctx.phase = GAME_PHASE_WAIT_CANT_DET_FINISH;
                if (app_prompt_play(cant_det_wav, cant_det_wav_len) != BK_OK)
                {
                    LOGE("play cant detect prompt failed\n");
                }
                break;

            case APP_EVENT_PROMPT_DETECTING:
                LOGD("Playing prompt: detecting\n");
                if (app_prompt_play(detecting_wav, detecting_wav_len) != BK_OK)
                {
                    LOGE("play detecting prompt failed\n");
                }
                break;

            case APP_EVENT_PROMPT_GET_READY:
                gesture_detection_start();

                /* Start game, enter GAME state and play get_ready_wav. */
                LOGD("Game start, get_ready event, play get_ready\n");
                s_app_evt_ctx.state = APP_STATE_GAME;
                s_app_evt_ctx.phase = GAME_PHASE_WAIT_FIRST_DETECTING_FINISH;
                s_app_evt_ctx.gesture_accept_enabled = 0;
                s_app_evt_ctx.has_user_gesture = 0;
                if (app_prompt_play(get_ready_wav, get_ready_wav_len) != BK_OK)
                {
                    LOGE("play get_ready (from get_ready) failed\n");
                }
                break;

            case APP_EVENT_PROMPT_MY_SHOW:
                LOGD("Playing prompt: my show\n");
                if (app_prompt_play(my_show_wav, my_show_wav_len) != BK_OK)
                {
                    LOGE("play my show prompt failed\n");
                }
                break;

            case APP_EVENT_PROMPT_PAPER:
                LOGD("Playing prompt: paper\n");
                if (app_prompt_play(paper_wav, paper_wav_len) != BK_OK)
                {
                    LOGE("play paper prompt failed\n");
                }
                break;

            case APP_EVENT_PROMPT_PLS_STOP_HAND:
                gesture_detection_stop();

                /* Convert backup image to RGB565 format if available (do this outside lock to avoid blocking LVGL rendering). */
                if (s_app_evt_ctx.backup_image_data != NULL)
                {
                    if (app_convert_backup_image_to_rgb565() == BK_OK)
                    {
                        /* Lock LVGL display before modifying image sources to avoid assert during rendering. */
                        lv_vendor_disp_lock();
                        /* Use converted backup image for user gesture display. */
                        lv_image_set_src(bk_lv_tool_ui.page_1_image_1, &s_app_evt_ctx.rgb565_image_dsc);
                        lv_vendor_disp_unlock();
                        LOGD("using backup image for user gesture display\n");
                    }
                    else
                    {
                        /* Fallback to default images if conversion failed. */
                        lv_vendor_disp_lock();
                        switch(s_app_evt_ctx.user_gesture)
                        {
                            case GESTURE_PAPER:
                                lv_image_set_src(bk_lv_tool_ui.page_1_image_1, &paper_240x240_RGB565A8_NONE);
                                break;
                            case GESTURE_ROCK:
                                lv_image_set_src(bk_lv_tool_ui.page_1_image_1, &rock_240x240_RGB565A8_NONE);
                                break;
                            case GESTURE_SCISSORS:
                                lv_image_set_src(bk_lv_tool_ui.page_1_image_1, &scissors_240x240_RGB565A8_NONE);
                                break;
                            default:
                                break;
                        }
                        lv_vendor_disp_unlock();
                    }
                }
                else
                {
                    /* No backup image available, use default images. */
                    lv_vendor_disp_lock();
                    switch(s_app_evt_ctx.user_gesture)
                    {
                        case GESTURE_PAPER:
                            lv_image_set_src(bk_lv_tool_ui.page_1_image_1, &paper_240x240_RGB565A8_NONE);
                            break;
                        case GESTURE_ROCK:
                            lv_image_set_src(bk_lv_tool_ui.page_1_image_1, &rock_240x240_RGB565A8_NONE);
                            break;
                        case GESTURE_SCISSORS:
                            lv_image_set_src(bk_lv_tool_ui.page_1_image_1, &scissors_240x240_RGB565A8_NONE);
                            break;
                        default:
                            break;
                    }
                    lv_vendor_disp_unlock();
                }

                /* Device gesture always uses default images. */
                lv_vendor_disp_lock();
                switch(s_app_evt_ctx.device_gesture)
                {
                    case GESTURE_PAPER:
                        lv_image_set_src(bk_lv_tool_ui.page_1_image_2, &paper_240x240_RGB565A8_NONE);
                        break;
                    case GESTURE_ROCK:
                        lv_image_set_src(bk_lv_tool_ui.page_1_image_2, &rock_240x240_RGB565A8_NONE);
                        break;
                    case GESTURE_SCISSORS:
                        lv_image_set_src(bk_lv_tool_ui.page_1_image_2, &scissors_240x240_RGB565A8_NONE);
                        break;
                    default:
                        break;
                }
                lv_vendor_disp_unlock();

                LOGD("Playing prompt: please stop hand\n");
                if (app_prompt_play(pls_stop_hand_wav, pls_stop_hand_wav_len) != BK_OK)
                {
                    LOGE("play please stop hand prompt failed\n");
                }
                break;

            case APP_EVENT_PROMPT_ROCK:
                LOGD("Playing prompt: rock\n");
                if (app_prompt_play(rock_wav, rock_wav_len) != BK_OK)
                {
                    LOGE("play rock prompt failed\n");
                }
                break;

            case APP_EVENT_PROMPT_SCISSORS:
                LOGD("Playing prompt: scissors\n");
                if (app_prompt_play(scissors_wav, scissors_wav_len) != BK_OK)
                {
                    LOGE("play scissors prompt failed\n");
                }
                break;

            case APP_EVENT_PROMPT_YOU_LOSS:
                LOGD("Playing prompt: you loss\n");
                if (app_prompt_play(you_loss_wav, you_loss_wav_len) != BK_OK)
                {
                    LOGE("play you loss prompt failed\n");
                }
                break;

            case APP_EVENT_PROMPT_YOU_WIN:
                LOGD("Playing prompt: you win\n");
                if (app_prompt_play(you_win_wav, you_win_wav_len) != BK_OK)
                {
                    LOGE("play you win prompt failed\n");
                }
                break;

            case APP_EVENT_PROMPT_YOUR_SHOW:
                LOGD("Playing prompt: your show\n");
                if (app_prompt_play(your_show_wav, your_show_wav_len) != BK_OK)
                {
                    LOGE("play your show prompt failed\n");
                }
                break;

            case APP_EVENT_PROMPT_DRAW:
                LOGD("Playing prompt: draw\n");
                /* TODO: Create draw.c file with draw_wav for draw prompt tone.
                 * For now, using you_win_wav as placeholder.
                 */
                if (app_prompt_play(draw_wav, draw_wav_len) != BK_OK)
                {
                    LOGE("play draw prompt failed\n");
                }
                break;

            case APP_EVENT_NONE:
            default:
                /* Ignore unknown events. */
                LOGW("unknown event id: %d\n", msg.id);
                break;
        }
    }
}

/**
 * @brief Handle image callback from gesture detection model.
 *
 * This function is called from C++ code when image data is available.
 * It will backup the image if needed (valid gesture detected before timeout,
 * or after timeout for display).
 *
 * @param image_data Pointer to image pixel data.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param format Pixel format.
 * @param data_size Total size of image data in bytes.
 */
void app_event_on_image(const uint8_t *image_data, uint32_t width, uint32_t height, uint32_t format, uint32_t data_size)
{
    /* Only backup image during gesture detection window or immediately after gesture detected.
     * Allow backup in GAME_PHASE_WAIT_PLS_STOP_FINISH to handle race condition where
     * image callback arrives after gesture callback has changed the phase.
     */
    if (s_app_evt_ctx.state != APP_STATE_GAME ||
        (s_app_evt_ctx.phase != GAME_PHASE_WAIT_GESTURE &&
         s_app_evt_ctx.phase != GAME_PHASE_WAIT_PLS_STOP_FINISH))
    {
        return;
    }

    /* Backup image only when needed:
     *   - Valid gesture detected before timeout (need_backup_image == 1, timeout_happened == 0)
     *   - After timeout (need_backup_image == 1, timeout_happened == 1)
     */
    if (!s_app_evt_ctx.need_backup_image)
    {
        return;
    }

    /* Free previous backup if exists. */
    if (s_app_evt_ctx.backup_image_data != NULL)
    {
        psram_free(s_app_evt_ctx.backup_image_data);
        s_app_evt_ctx.backup_image_data = NULL;
    }

    /* Allocate memory for backup image from PSRAM. */
    s_app_evt_ctx.backup_image_data = (uint8_t *)psram_malloc(data_size);
    if (s_app_evt_ctx.backup_image_data == NULL)
    {
        LOGE("psram_malloc backup image failed, size: %u\n", data_size);
        return;
    }

    /* Copy image data. */
    os_memcpy(s_app_evt_ctx.backup_image_data, image_data, data_size);
    s_app_evt_ctx.backup_image_size = data_size;
    s_app_evt_ctx.backup_image_width = width;
    s_app_evt_ctx.backup_image_height = height;
    s_app_evt_ctx.backup_image_format = format;

    /* Clear the flag after backup. */
    s_app_evt_ctx.need_backup_image = 0;

    LOGD("image backed up: %ux%u, format: %u, size: %u\n", width, height, format, data_size);
}

/**
 * @brief Convert BGRA8888 image to RGB565 format.
 *
 * @param src_bgra Pointer to source BGRA8888 image data.
 * @param dst_rgb565 Pointer to destination RGB565 buffer.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 */
static void bgra8888_to_rgb565_convert(const uint8_t *src_bgra, uint16_t *dst_rgb565, uint32_t width, uint32_t height)
{
    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            uint32_t src_idx = (y * width + x) * 4;  /* BGRA8888: 4 bytes per pixel */
            uint8_t b = src_bgra[src_idx + 0];
            uint8_t g = src_bgra[src_idx + 1];
            uint8_t r = src_bgra[src_idx + 2];
            /* Alpha channel at src_bgra[src_idx + 3] is ignored */

            /* Convert to RGB565: RRRRR GGGGGG BBBBB */
            dst_rgb565[y * width + x] = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
        }
    }
}

/**
 * @brief Convert backup image to RGB565 format for LVGL display.
 *
 * This function converts the backup BGRA8888 image to RGB565 format
 * and creates an LVGL image descriptor for display.
 *
 * @return BK_OK on success, BK_FAIL on error.
 */
static bk_err_t app_convert_backup_image_to_rgb565(void)
{
    if (s_app_evt_ctx.backup_image_data == NULL)
    {
        LOGE("backup image data is NULL\n");
        return BK_FAIL;
    }

    if (s_app_evt_ctx.backup_image_format != BK_PIXEL_FORMAT_BGRA8888)
    {
        LOGE("backup image format is not BGRA8888: %u\n", s_app_evt_ctx.backup_image_format);
        return BK_FAIL;
    }

    uint32_t width = s_app_evt_ctx.backup_image_width;
    uint32_t height = s_app_evt_ctx.backup_image_height;

    /* Calculate RGB565 image size: 2 bytes per pixel */
    uint32_t rgb565_size = width * height * 2;

    /* Free previous RGB565 image if exists. */
    if (s_app_evt_ctx.rgb565_image_data != NULL)
    {
        psram_free(s_app_evt_ctx.rgb565_image_data);
        s_app_evt_ctx.rgb565_image_data = NULL;
    }

    /* Allocate memory for RGB565 image from PSRAM. */
    s_app_evt_ctx.rgb565_image_data = (uint8_t *)psram_malloc(rgb565_size);
    if (s_app_evt_ctx.rgb565_image_data == NULL)
    {
        LOGE("psram_malloc RGB565 image failed, size: %u\n", rgb565_size);
        return BK_FAIL;
    }

    /* Convert BGRA8888 to RGB565. */
    bgra8888_to_rgb565_convert(s_app_evt_ctx.backup_image_data,
                               (uint16_t *)s_app_evt_ctx.rgb565_image_data,
                               width, height);

    /* Create LVGL image descriptor. */
    s_app_evt_ctx.rgb565_image_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    s_app_evt_ctx.rgb565_image_dsc.header.cf = LV_COLOR_FORMAT_RGB565;
    s_app_evt_ctx.rgb565_image_dsc.header.flags = 0;
    s_app_evt_ctx.rgb565_image_dsc.header.w = (uint16_t)width;
    s_app_evt_ctx.rgb565_image_dsc.header.h = (uint16_t)height;
    s_app_evt_ctx.rgb565_image_dsc.header.stride = width * 2;  /* 2 bytes per pixel */
    s_app_evt_ctx.rgb565_image_dsc.data_size = rgb565_size;
    s_app_evt_ctx.rgb565_image_dsc.data = (const uint8_t *)s_app_evt_ctx.rgb565_image_data;

    s_app_evt_ctx.rgb565_image_size = rgb565_size;

    LOGD("converted backup image to RGB565: %ux%u, size: %u\n", width, height, rgb565_size);

    return BK_OK;
}

bk_err_t app_event_init(void)
{
    bk_err_t ret = BK_OK;

    os_memset(&s_app_evt_ctx, 0, sizeof(s_app_evt_ctx));

    /* Initialize random number generator seed using system time for better randomness. */
    {
        uint32_t time_ms = rtos_get_time();
        srand(time_ms);
    }

    /* Initialize game mode with default macro. */
    s_app_evt_ctx.game_mode = APP_EVENT_DEFAULT_GAME_MODE;

    /* Create event queue */
    ret = rtos_init_queue(&s_app_evt_ctx.event_queue,
                          "app_event_queue",
                          sizeof(app_event_msg_t),
                          APP_EVENT_QUEUE_LENGTH);
    if (ret != kNoErr)
    {
        LOGE("rtos_init_queue failed, ret: %d\n", ret);
        return BK_FAIL;
    }

    /* Player will be created on-demand in app_prompt_play() and destroyed in app_prompt_stop().
     * This avoids state management issues and ensures clean resource handling.
     */
    s_app_evt_ctx.player = NULL;

    /* Create event task
     * Stack size increased to 4096 to accommodate image conversion operations
     * (BGRA8888 to RGB565 conversion and LVGL display operations).
     */
    ret = rtos_create_thread(&s_app_evt_ctx.event_task,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "app_event_task",
                             (beken_thread_function_t)app_event_task,
                             4096,
                             NULL);
    if (ret != kNoErr)
    {
        LOGE("rtos_create_thread failed, ret: %d\n", ret);
        rtos_deinit_queue(&s_app_evt_ctx.event_queue);
        s_app_evt_ctx.event_queue = NULL;
        return BK_FAIL;
    }

    LOGI("app_event_init success\n");

    return BK_OK;
}

bk_err_t app_event_post(app_event_id_t event_id, uint32_t param)
{
    if (s_app_evt_ctx.event_queue == NULL)
    {
        LOGE("event queue is not initialized\n");
        return BK_FAIL;
    }

    app_event_msg_t msg;
    msg.id = event_id;
    msg.param = param;

    bk_err_t ret = rtos_push_to_queue(&s_app_evt_ctx.event_queue, &msg, 0);
    if (ret != kNoErr)
    {
        LOGE("rtos_push_to_queue failed, ret: %d, event id: %d\n", ret, event_id);
        return BK_FAIL;
    }

    return BK_OK;
}

void app_event_on_gesture(int gesture)
{
    gesture_result_t user_gesture = (gesture_result_t)gesture;

    /* Only process gestures in GAME state and in WAIT_GESTURE phase. */
    if (s_app_evt_ctx.state != APP_STATE_GAME ||
        s_app_evt_ctx.phase != GAME_PHASE_WAIT_GESTURE)
    {
        LOGD("gesture detected outside game state/phase, ignored\n");
        return;
    }

    if (!s_app_evt_ctx.gesture_accept_enabled)
    {
        LOGD("gesture detected when acceptance disabled, ignored\n");
        return;
    }

    /* Skip any further results once we have already accepted one. */
    if (s_app_evt_ctx.has_user_gesture)
    {
        return;
    }

    /* Filter by gesture value range. */
    if (user_gesture < GESTURE_ROCK || user_gesture >= GESTURE_MAX)
    {
        /* Out-of-range value, always ignore. */
        LOGD("gesture out of range: %d\n", gesture);
        return;
    }

    /* Before timeout: only accept valid gestures (ROCK/PAPER/SCISSORS).
     * GESTURE_NONE is ignored in this window.
     */
    if (!s_app_evt_ctx.gesture_timeout_happened && user_gesture == GESTURE_NONE)
    {
        LOGD("GESTURE_NONE before timeout, ignored\n");
        return;
    }

    /* From this point, we will treat this gesture as the final result for this round.
     * Disable further acceptance and stop the timeout timer.
     */
    s_app_evt_ctx.gesture_accept_enabled = 0;
    if (s_app_evt_ctx.gesture_timer_inited)
    {
        rtos_stop_oneshot_timer(&s_app_evt_ctx.gesture_timer);
    }

    if (user_gesture == GESTURE_NONE)
    {
        /* After timeout, accept GESTURE_NONE as "no gesture" final result.
         * Do not mark has_user_gesture so that the flow will go through
         * cant_det_wav path in GAME_PHASE_WAIT_PLS_STOP_FINISH.
         */
        s_app_evt_ctx.has_user_gesture = 0;
    }
    else
    {
        /* Valid ROCK/PAPER/SCISSORS result. */
        s_app_evt_ctx.device_gesture = app_random_gesture();
        s_app_evt_ctx.user_gesture = user_gesture;

        s_app_evt_ctx.game_result = app_calc_game_result(s_app_evt_ctx.device_gesture,
                                                         s_app_evt_ctx.user_gesture);
        if (s_app_evt_ctx.game_result == APP_GAME_RESULT_UNKNOWN)
        {
            LOGD("unknown game result, treat as no gesture\n");
            s_app_evt_ctx.has_user_gesture = 0;
        }
        else
        {
            s_app_evt_ctx.has_user_gesture = 1;
            /* Mark that we need to backup image when next image callback arrives.
             * This happens when valid gesture is detected before timeout.
             */
            s_app_evt_ctx.need_backup_image = 1;
        }
    }

    /* Inform user to stop hand first; subsequent flow depends on has_user_gesture:
     *   - has_user_gesture == 1: go through second detecting + full result sequence.
     *   - has_user_gesture == 0: go through cant_det_wav path.
     */
    s_app_evt_ctx.phase = GAME_PHASE_WAIT_PLS_STOP_FINISH;
    if (app_event_post(APP_EVENT_PROMPT_PLS_STOP_HAND, 0) != BK_OK)
    {
        LOGE("post APP_EVENT_PROMPT_PLS_STOP_HAND failed\n");
        return;
    }
}

void app_event_set_game_mode(app_game_mode_t mode)
{
    if (mode != APP_GAME_MODE_SINGLE && mode != APP_GAME_MODE_LOOP)
    {
        LOGE("invalid game mode: %d\n", mode);
        return;
    }

    s_app_evt_ctx.game_mode = mode;
}

app_game_mode_t app_event_get_game_mode(void)
{
    return s_app_evt_ctx.game_mode;
}
