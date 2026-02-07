// Copyright 2025-2026 Beken
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

#include <os/os.h>
#include <components/log.h>
#include <components/bk_audio_player/bk_audio_player.h>
#include <common/bk_err.h>
#include <stdbool.h>
#include <driver/trng.h>
#include "cli.h"
#include "lvgl.h"
#include "lv_demo_music.h"
#include "lv_demo_music_main.h"
#include "cli_lv_demo_music.h"

#define TAG "lv_music_cli"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define CLI_CMD_RSP_SUCCEED   "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR     "CMDRSP:ERROR\r\n"

typedef struct {
    uint32_t second;
} lv_music_seek_req_t;

typedef struct {
    uint32_t track_id;
} lv_music_play_req_t;

typedef struct {
    uint32_t start_second;
    uint32_t end_second;
    uint32_t step;
    uint32_t delay_ms;
    uint32_t repeat;
    bool bidirectional;
    bool random_mode;
} lv_music_stress_cfg_t;

typedef struct {
    uint32_t step;
    uint32_t interval_ms;
    uint32_t settle_ms;
    uint32_t repeat;
} lv_music_playlist_cfg_t;

static beken_thread_t s_stress_thread = NULL;
static lv_music_stress_cfg_t s_stress_cfg = {0};
static volatile bool s_stress_stop = false;
static beken_thread_t s_playlist_thread = NULL;
static lv_music_playlist_cfg_t s_playlist_cfg = {0};
static volatile bool s_playlist_stop = false;

static bk_err_t lv_music_schedule_play(uint32_t track_id);

static void lv_music_seek_async_cb(void *user_data)
{
    lv_music_seek_req_t *req = (lv_music_seek_req_t *)user_data;
    uint32_t second = req ? req->second : 0;

    _lv_demo_music_update_progress(second);

    {
        bk_audio_player_handle_t handle = (bk_audio_player_handle_t)_lv_demo_music_get_player_handle();
        int ret = handle ? bk_audio_player_seek(handle, (int)second) : AUDIO_PLAYER_NOT_INIT;
        if (ret != AUDIO_PLAYER_OK) {
        LOGE("seek %u failed, ret=%d\n", second, ret);
        } else {
            LOGI("seek %u requested\n", second);
        }
    }

    if (req) {
        os_free(req);
    }
}

static bk_err_t lv_music_schedule_seek(uint32_t second)
{
    lv_music_seek_req_t *req = (lv_music_seek_req_t *)os_malloc(sizeof(lv_music_seek_req_t));
    if (!req) {
        LOGE("allocate async seek request failed\n");
        return BK_ERR_NO_MEM;
    }

    req->second = second;
    lv_res_t res = lv_async_call(lv_music_seek_async_cb, req);
    if (res != LV_RES_OK) {
        LOGE("lv_async_call failed (%d)\n", (int)res);
        os_free(req);
        return BK_FAIL;
    }

    return BK_OK;
}

static void lv_music_play_async_cb(void *user_data)
{
    lv_music_play_req_t *req = (lv_music_play_req_t *)user_data;
    uint32_t track = req ? req->track_id : 0;
    _lv_demo_music_play(track);
    if (req)
    {
        os_free(req);
    }
}

static bk_err_t lv_music_schedule_play(uint32_t track_id)
{
    lv_music_play_req_t *req = (lv_music_play_req_t *)os_malloc(sizeof(lv_music_play_req_t));
    if (!req)
    {
        LOGE("allocate async play request failed\n");
        return BK_ERR_NO_MEM;
    }

    req->track_id = track_id;
    lv_res_t res = lv_async_call(lv_music_play_async_cb, req);
    if (res != LV_RES_OK)
    {
        LOGE("lv_async_call(play) failed (%d)\n", (int)res);
        os_free(req);
        return BK_FAIL;
    }

    return BK_OK;
}

static void lv_music_stress_thread(void *param)
{
    (void)param;
    uint32_t executed_round = 0;
    const uint32_t repeat = s_stress_cfg.repeat;
    const bool infinite = (repeat == 0U);

    while (!s_stress_stop) {
        if (!infinite && executed_round >= repeat) {
            break;
        }

        if (s_stress_cfg.random_mode) {
            uint32_t range = (s_stress_cfg.end_second > s_stress_cfg.start_second)
                               ? (s_stress_cfg.end_second - s_stress_cfg.start_second)
                               : 0U;
            uint32_t offset = (range == 0U) ? 0U : (uint32_t)(bk_rand() % (range + 1U));
            uint32_t sec = s_stress_cfg.start_second + offset;
            lv_music_schedule_seek(sec);
            executed_round++;
            rtos_delay_milliseconds(s_stress_cfg.delay_ms);
            continue;
        }

        executed_round++;

        for (uint32_t sec = s_stress_cfg.start_second;
             !s_stress_stop && sec <= s_stress_cfg.end_second;
             sec += s_stress_cfg.step)
        {
            lv_music_schedule_seek(sec);
            rtos_delay_milliseconds(s_stress_cfg.delay_ms);
        }

        if (!s_stress_cfg.bidirectional) {
            continue;
        }

        uint32_t sec = s_stress_cfg.end_second;
        while (!s_stress_stop && sec > s_stress_cfg.start_second) {
            sec = (sec > s_stress_cfg.step) ? sec - s_stress_cfg.step : s_stress_cfg.start_second;
            lv_music_schedule_seek(sec);
            rtos_delay_milliseconds(s_stress_cfg.delay_ms);
            if (sec == s_stress_cfg.start_second) {
                break;
            }
        }
    }

    s_stress_thread = NULL;
    rtos_delete_thread(NULL);
}

static bk_err_t lv_music_start_stress(uint32_t start_second,
                                      uint32_t end_second,
                                      uint32_t step,
                                      uint32_t delay_ms,
                                      uint32_t repeat,
                                      bool bidirectional)
{
    if (s_stress_thread != NULL) {
        LOGW("stress task already running\n");
        return BK_ERR_BUSY;
    }

    if (end_second < start_second || step == 0) {
        LOGE("invalid parameters: start=%u end=%u step=%u\n", start_second, end_second, step);
        return BK_ERR_PARAM;
    }

    s_stress_cfg.start_second = start_second;
    s_stress_cfg.end_second = end_second;
    s_stress_cfg.step = step;
    s_stress_cfg.delay_ms = delay_ms;
    s_stress_cfg.repeat = repeat;
    s_stress_cfg.bidirectional = bidirectional;
    s_stress_cfg.random_mode = false;
    s_stress_stop = false;

    bk_err_t ret = rtos_create_thread(&s_stress_thread,
                                      BEKEN_DEFAULT_WORKER_PRIORITY,
                                      "lv_music_stress",
                                      (beken_thread_function_t)lv_music_stress_thread,
                                      4096,
                                      NULL);
    if (ret != BK_OK) {
        LOGE("create stress thread failed, ret=%d\n", ret);
        s_stress_thread = NULL;
    }

    return ret;
}

static void lv_music_stop_stress(void)
{
    if (s_stress_thread != NULL) {
        s_stress_stop = true;
        while (s_stress_thread != NULL) {
            rtos_delay_milliseconds(10);
        }
    }

    if (s_playlist_thread != NULL) {
        s_playlist_stop = true;
        while (s_playlist_thread != NULL) {
            rtos_delay_milliseconds(10);
        }
    }
}

static bk_err_t lv_music_start_random_stress(uint32_t start_second,
                                             uint32_t end_second,
                                             uint32_t delay_ms,
                                             uint32_t repeat)
{
    if (s_stress_thread != NULL) {
        LOGW("stress task already running\n");
        return BK_ERR_BUSY;
    }

    if (end_second < start_second) {
        LOGE("invalid range: start=%u end=%u\n", start_second, end_second);
        return BK_ERR_PARAM;
    }

    s_stress_cfg.start_second = start_second;
    s_stress_cfg.end_second = end_second;
    s_stress_cfg.step = 1;
    s_stress_cfg.delay_ms = delay_ms;
    s_stress_cfg.repeat = repeat;
    s_stress_cfg.bidirectional = false;
    s_stress_cfg.random_mode = true;
    s_stress_stop = false;

    bk_err_t ret = rtos_create_thread(&s_stress_thread,
                                      BEKEN_DEFAULT_WORKER_PRIORITY,
                                      "lv_music_rand",
                                      (beken_thread_function_t)lv_music_stress_thread,
                                      4096,
                                      NULL);
    if (ret != BK_OK) {
        LOGE("create random stress thread failed, ret=%d\n", ret);
        s_stress_thread = NULL;
    }

    return ret;
}

static void lv_music_playlist_thread(void *param)
{
    (void)param;
    uint32_t track_count = _lv_demo_music_get_track_count();
    if (track_count == 0) {
        goto exit;
    }

    const bool infinite = (s_playlist_cfg.repeat == 0U);
    uint32_t executed_round = 0;

    while (!s_playlist_stop && (infinite || executed_round < s_playlist_cfg.repeat)) {
        for (uint32_t track = 0; track < track_count && !s_playlist_stop; ++track) {
            if (lv_music_schedule_play(track) != BK_OK) {
                continue;
            }

            if (s_playlist_cfg.settle_ms > 0) {
                rtos_delay_milliseconds(s_playlist_cfg.settle_ms);
            }

            uint32_t track_len = _lv_demo_music_get_track_length(track);
            if (track_len <= s_playlist_cfg.step) {
                continue;
            }

            uint32_t position = 0;
            while (!s_playlist_stop) {
                if (position + s_playlist_cfg.step >= track_len) {
                    break;
                }
                position += s_playlist_cfg.step;
                lv_music_schedule_seek(position);
                rtos_delay_milliseconds(s_playlist_cfg.interval_ms);
            }
        }

        executed_round++;
    }

exit:
    s_playlist_thread = NULL;
    s_playlist_stop = false;
    rtos_delete_thread(NULL);
}

static bk_err_t lv_music_start_playlist_stress(uint32_t step,
                                               uint32_t interval_ms,
                                               uint32_t settle_ms,
                                               uint32_t repeat)
{
    if (step == 0 || interval_ms == 0) {
        LOGE("invalid playlist parameters: step=%u interval=%u\n", step, interval_ms);
        return BK_ERR_PARAM;
    }

    if (_lv_demo_music_get_track_count() == 0) {
        LOGE("playlist is empty\n");
        return BK_FAIL;
    }

    if (s_stress_thread != NULL || s_playlist_thread != NULL) {
        LOGW("stress task already running\n");
        return BK_ERR_BUSY;
    }

    s_playlist_cfg.step = step;
    s_playlist_cfg.interval_ms = interval_ms;
    s_playlist_cfg.settle_ms = (settle_ms == 0U) ? 1000U : settle_ms;
    s_playlist_cfg.repeat = repeat;
    s_playlist_stop = false;

    bk_err_t ret = rtos_create_thread(&s_playlist_thread,
                                      BEKEN_DEFAULT_WORKER_PRIORITY,
                                      "lv_music_pl",
                                      (beken_thread_function_t)lv_music_playlist_thread,
                                      4096,
                                      NULL);
    if (ret != BK_OK) {
        LOGE("create playlist stress thread failed, ret=%d\n", ret);
        s_playlist_thread = NULL;
    }

    return ret;
}

static void cli_lv_music_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *response = CLI_CMD_RSP_ERROR;

    if (argc < 2) {
        goto exit;
    }

    if (os_strcmp(argv[1], "seek") == 0) {
        if (argc < 3) {
            LOGE("usage: lv_music seek <second>\n");
            goto exit;
        }
        uint32_t second = os_strtoul(argv[2], NULL, 10);
        if (lv_music_schedule_seek(second) == BK_OK) {
            response = CLI_CMD_RSP_SUCCEED;
        }
    } else if (os_strcmp(argv[1], "stress") == 0) {
        if (argc < 6) {
            LOGE("usage: lv_music stress <start_s> <end_s> <step_s> <delay_ms> [repeat] [pingpong]\n");
            goto exit;
        }
        uint32_t start_s = os_strtoul(argv[2], NULL, 10);
        uint32_t end_s = os_strtoul(argv[3], NULL, 10);
        uint32_t step = os_strtoul(argv[4], NULL, 10);
        uint32_t delay_ms = os_strtoul(argv[5], NULL, 10);
        uint32_t repeat = (argc > 6) ? os_strtoul(argv[6], NULL, 10) : 0U;
        bool pingpong = (argc > 7) ? (os_strcmp(argv[7], "pingpong") == 0 || os_strcmp(argv[7], "1") == 0) : true;

        if (lv_music_start_stress(start_s, end_s, step, delay_ms, repeat, pingpong) == BK_OK) {
            response = CLI_CMD_RSP_SUCCEED;
        }
    } else if (os_strcmp(argv[1], "stress_rand") == 0) {
        if (argc < 5) {
            LOGE("usage: lv_music stress_rand <start_s> <end_s> <delay_ms> [repeat]\n");
            goto exit;
        }
        uint32_t start_s = os_strtoul(argv[2], NULL, 10);
        uint32_t end_s = os_strtoul(argv[3], NULL, 10);
        uint32_t delay_ms = os_strtoul(argv[4], NULL, 10);
        uint32_t repeat = (argc > 5) ? os_strtoul(argv[5], NULL, 10) : 0U;

        if (lv_music_start_random_stress(start_s, end_s, delay_ms, repeat) == BK_OK) {
            response = CLI_CMD_RSP_SUCCEED;
        }
    } else if (os_strcmp(argv[1], "stress_playlist") == 0) {
        if (argc < 4) {
            LOGE("usage: lv_music stress_playlist <step_s> <interval_ms> [settle_ms] [repeat]\n");
            goto exit;
        }
        uint32_t step = os_strtoul(argv[2], NULL, 10);
        uint32_t interval_ms = os_strtoul(argv[3], NULL, 10);
        uint32_t settle_ms = (argc > 4) ? os_strtoul(argv[4], NULL, 10) : 1000U;
        uint32_t repeat = (argc > 5) ? os_strtoul(argv[5], NULL, 10) : 1U;

        if (lv_music_start_playlist_stress(step, interval_ms, settle_ms, repeat) == BK_OK) {
            response = CLI_CMD_RSP_SUCCEED;
        }
    } else if (os_strcmp(argv[1], "stop") == 0) {
        lv_music_stop_stress();
        response = CLI_CMD_RSP_SUCCEED;
    } else {
        LOGE("unknown subcommand: %s\n", argv[1]);
    }

exit:
    os_memcpy(pcWriteBuffer, response, os_strlen(response));
}


#define LV_MUSIC_CMD_CNT  (sizeof(s_lv_music_commands) / sizeof(struct cli_command))
static const struct cli_command s_lv_music_commands[] =
{
    {"lv_music", "lv_music seek/stress/stress_rand/stress_playlist/stop ...", cli_lv_music_cmd},
};

int cli_lv_demo_music_init(void)
{
    return cli_register_commands(s_lv_music_commands, LV_MUSIC_CMD_CNT);
}
