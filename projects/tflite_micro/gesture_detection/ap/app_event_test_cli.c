/**
 * @file app_event_test_cli.c
 * @brief CLI commands to simulate button events for app_event (RPS game) testing.
 *
 * Use these commands when the device has no physical buttons to trigger
 * game start/end and other events. Example: "rps_start" to start a game,
 * "rps_end" to end and return to idle.
 */

#include <common/bk_include.h>
#include <os/os.h>
#include <os/str.h>
#include "cli.h"
#include "app_event.h"

#define TAG "rps-cli"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)

/**
 * @brief CLI handler: rps_start - post APP_EVENT_PROMPT_GET_READY (start game).
 */
static void cli_rps_start_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_err_t ret = app_event_post(APP_EVENT_PROMPT_GET_READY, 0);
    if (ret == BK_OK)
    {
        snprintf(pcWriteBuffer, xWriteBufferLen, "rps: start event posted\n");
        LOGI("rps_start: event posted\n");
    }
    else
    {
        snprintf(pcWriteBuffer, xWriteBufferLen, "rps: post start failed %d\n", ret);
    }
}

/**
 * @brief CLI handler: rps_end - post APP_EVENT_GAME_END (end game, back to idle).
 */
static void cli_rps_end_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_err_t ret = app_event_post(APP_EVENT_GAME_END, 0);
    if (ret == BK_OK)
    {
        snprintf(pcWriteBuffer, xWriteBufferLen, "rps: end event posted\n");
        LOGI("rps_end: event posted\n");
    }
    else
    {
        snprintf(pcWriteBuffer, xWriteBufferLen, "rps: post end failed %d\n", ret);
    }
}

/**
 * @brief CLI handler: rps_rule - post APP_EVENT_PROMPT_GAME_RULE (play rule prompt).
 */
static void cli_rps_rule_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_err_t ret = app_event_post(APP_EVENT_PROMPT_GAME_RULE, 0);
    if (ret == BK_OK)
    {
        snprintf(pcWriteBuffer, xWriteBufferLen, "rps: rule event posted\n");
        LOGI("rps_rule: event posted\n");
    }
    else
    {
        snprintf(pcWriteBuffer, xWriteBufferLen, "rps: post rule failed %d\n", ret);
    }
}

/**
 * @brief CLI handler: rps_gesture <rock|paper|scissors> - simulate AI gesture detection result.
 *
 * This command simulates the AI gesture detection module calling app_event_on_gesture()
 * to push gesture detection results to app_event.
 */
static void cli_rps_gesture_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc < 2)
    {
        snprintf(pcWriteBuffer, xWriteBufferLen, "usage: rps_gesture rock|paper|scissors\n");
        return;
    }

    int gesture = -1;
    if (os_strcmp(argv[1], "rock") == 0)
    {
        gesture = 0;  /* GESTURE_ROCK */
    }
    else if (os_strcmp(argv[1], "paper") == 0)
    {
        gesture = 1;  /* GESTURE_PAPER */
    }
    else if (os_strcmp(argv[1], "scissors") == 0)
    {
        gesture = 2;  /* GESTURE_SCISSORS */
    }
    else
    {
        snprintf(pcWriteBuffer, xWriteBufferLen, "rps: unknown gesture '%s', use rock|paper|scissors\n", argv[1]);
        return;
    }

    /* Simulate AI gesture detection by calling app_event_on_gesture() */
    app_event_on_gesture(gesture);
    snprintf(pcWriteBuffer, xWriteBufferLen, "rps: gesture '%s' (value: %d) sent to app_event\n", argv[1], gesture);
    LOGI("rps_gesture: %s (value: %d) sent\n", argv[1], gesture);
}

/**
 * @brief CLI handler: rps_prompt <name> - play specified prompt tone.
 *
 * Available prompt names:
 *   cant_det, detecting, get_ready, my_show, paper, pls_stop_hand,
 *   rock, scissors, you_loss, you_win, your_show, draw, game_rule
 */
static void cli_rps_prompt_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc < 2)
    {
        snprintf(pcWriteBuffer, xWriteBufferLen,
                 "usage: rps_prompt <name>\n"
                 "Available names: cant_det, detecting, get_ready, my_show, paper, pls_stop_hand, "
                 "rock, scissors, you_loss, you_win, your_show, draw, game_rule\n");
        return;
    }

    app_event_id_t event_id = APP_EVENT_NONE;

    if (os_strcmp(argv[1], "cant_det") == 0)
    {
        event_id = APP_EVENT_PROMPT_CANT_DET;
    }
    else if (os_strcmp(argv[1], "detecting") == 0)
    {
        event_id = APP_EVENT_PROMPT_DETECTING;
    }
    else if (os_strcmp(argv[1], "get_ready") == 0)
    {
        event_id = APP_EVENT_PROMPT_GET_READY;
    }
    else if (os_strcmp(argv[1], "my_show") == 0)
    {
        event_id = APP_EVENT_PROMPT_MY_SHOW;
    }
    else if (os_strcmp(argv[1], "paper") == 0)
    {
        event_id = APP_EVENT_PROMPT_PAPER;
    }
    else if (os_strcmp(argv[1], "pls_stop_hand") == 0)
    {
        event_id = APP_EVENT_PROMPT_PLS_STOP_HAND;
    }
    else if (os_strcmp(argv[1], "rock") == 0)
    {
        event_id = APP_EVENT_PROMPT_ROCK;
    }
    else if (os_strcmp(argv[1], "scissors") == 0)
    {
        event_id = APP_EVENT_PROMPT_SCISSORS;
    }
    else if (os_strcmp(argv[1], "you_loss") == 0)
    {
        event_id = APP_EVENT_PROMPT_YOU_LOSS;
    }
    else if (os_strcmp(argv[1], "you_win") == 0)
    {
        event_id = APP_EVENT_PROMPT_YOU_WIN;
    }
    else if (os_strcmp(argv[1], "your_show") == 0)
    {
        event_id = APP_EVENT_PROMPT_YOUR_SHOW;
    }
    else if (os_strcmp(argv[1], "draw") == 0)
    {
        event_id = APP_EVENT_PROMPT_DRAW;
    }
    else if (os_strcmp(argv[1], "game_rule") == 0)
    {
        event_id = APP_EVENT_PROMPT_GAME_RULE;
    }
    else
    {
        snprintf(pcWriteBuffer, xWriteBufferLen,
                 "rps: unknown prompt name '%s'\n"
                 "Available names: cant_det, detecting, get_ready, my_show, paper, pls_stop_hand, "
                 "rock, scissors, you_loss, you_win, your_show, draw, game_rule\n",
                 argv[1]);
        return;
    }

    bk_err_t ret = app_event_post(event_id, 0);
    if (ret == BK_OK)
    {
        snprintf(pcWriteBuffer, xWriteBufferLen, "rps: prompt '%s' event posted\n", argv[1]);
        LOGI("rps_prompt: %s posted\n", argv[1]);
    }
    else
    {
        snprintf(pcWriteBuffer, xWriteBufferLen, "rps: post prompt '%s' failed %d\n", argv[1], ret);
    }
}

static const struct cli_command s_rps_test_commands[] =
{
    { "rps_start",     "start game (PROMPT_GET_READY)",           cli_rps_start_cmd     },
    { "rps_end",       "simulate end game (GAME_END)",            cli_rps_end_cmd       },
    { "rps_rule",      "play game rule prompt (PROMPT_GAME_RULE)",cli_rps_rule_cmd      },
    { "rps_gesture",   "rps_gesture rock|paper|scissors - simulate AI gesture detection", cli_rps_gesture_cmd    },
    { "rps_prompt",    "rps_prompt <name> - play specified prompt tone", cli_rps_prompt_cmd    },
};

#define RPS_TEST_CMD_CNT  (sizeof(s_rps_test_commands) / sizeof(struct cli_command))

/**
 * @brief Register RPS test CLI commands. Call once after app_event_init().
 *
 * @return 0 on success, non-zero on failure.
 */
int app_event_test_cli_init(void)
{
    int ret = cli_register_commands(s_rps_test_commands, RPS_TEST_CMD_CNT);
    if (ret == 0)
    {
        LOGI("rps test CLI registered: rps_start, rps_end, rps_rule, rps_gesture, rps_prompt\n");
    }
    return ret;
}
