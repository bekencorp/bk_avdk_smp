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

/*
 * Test / debug CLI for the audio_play component (HINT PCM prompt and the shared
 * speaker-service lifecycle). Kept inside the component so the demo clips
 * (hint_prompt_clips) and the temporary spk_init/spk_deinit hooks do not leak
 * into the product project CLI.
 *
 * Commands:
 *   aud hint prompt     play the real 16k PCM prompt clip (mixes over music/call)
 *   aud hint stop       stop the current HINT playback
 *   aud spk_deinit      TEST: tear down the shared speaker/DAC pipeline
 *   aud spk_init        TEST: bring the shared speaker/DAC pipeline back up
 */

#include "cli.h"
#include "hint_service.h"
#include "spk_service.h"
#include "hint_prompt_clips.h"

static void aud_usage(void)
{
    CLI_LOGI("Usage:\n"
             "aud hint prompt   (play real 16k PCM prompt clip, mixes over music/call)\n"
             "aud hint wav      (play WAV prompt clip via decode path)\n"
             "aud hint mp3      (play MP3 prompt clip via decode path)\n"
             "aud hint stop\n"
             "aud spk_deinit    (TEST: tear down shared speaker/DAC)\n"
             "aud spk_init      (TEST: bring shared speaker/DAC back up)\n");
}

static void cmd_audio_play_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *msg = NULL;
    int ret = 0;

    if (argc == 1 || os_strcmp(argv[1], "-h") == 0)
    {
        goto __usage;
    }
    else if (os_strcmp(argv[1], "spk_deinit") == 0)
    {
        /* temporary test hook: tear down the shared speaker/DAC pipeline */
        ret = spk_service_deinit();
        CLI_LOGI("%s spk_service_deinit ret %d\n", __func__, ret);
        if (ret)
        {
            goto __error;
        }
    }
    else if (os_strcmp(argv[1], "spk_init") == 0)
    {
        /* temporary test hook: bring the shared speaker/DAC pipeline back up */
        ret = spk_service_init();
        CLI_LOGI("%s spk_service_init ret %d\n", __func__, ret);
        if (ret)
        {
            goto __error;
        }
    }
    else if (os_strcmp(argv[1], "hint") == 0)
    {
        if (argc >= 3 && os_strcmp(argv[2], "stop") == 0)
        {
            ret = hint_service_stop();
        }
        else if (argc >= 3 && os_strcmp(argv[2], "prompt") == 0)
        {
            /* Real 16k/mono PCM prompt clip, hardware-mixed on the DAC HINT source
             * so it plays over any concurrent A2DP music / HFP call. */
            ret = hint_service_play_pcm(asr_wakeup_prompt_tone_array,
                                        asr_wakeup_prompt_tone_array_len, 16000);
        }
        else if (argc >= 3 && os_strcmp(argv[2], "wav") == 0)
        {
            /* WAV clip through the decode path (rate/chans auto-detected). */
            ret = hint_service_play(low_voltage_prompt_tone_array,
                                    low_voltage_prompt_tone_array_len,
                                    HINT_FMT_WAV, 0, 0);
        }
        else if (argc >= 3 && os_strcmp(argv[2], "mp3") == 0)
        {
            /* MP3 clip through the decode path (rate/chans auto-detected). */
            ret = hint_service_play(network_provision_prompt_tone_array,
                                    network_provision_prompt_tone_array_len,
                                    HINT_FMT_MP3, 0, 0);
        }
        else
        {
            goto __usage;
        }

        if (ret)
        {
            goto __error;
        }
    }
    else
    {
        goto __usage;
    }

    msg = CLI_CMD_RSP_SUCCEED;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    return;

__usage:
    aud_usage();

__error:
    msg = CLI_CMD_RSP_ERROR;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

static const struct cli_command s_audio_play_commands[] =
{
    {"aud", "see -h", cmd_audio_play_test},
};

int audio_play_cli_init(void)
{
    return cli_register_commands(s_audio_play_commands,
                                 sizeof(s_audio_play_commands) / sizeof(s_audio_play_commands[0]));
}
