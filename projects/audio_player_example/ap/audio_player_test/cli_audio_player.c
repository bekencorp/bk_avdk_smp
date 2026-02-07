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
#include "os/str.h"
#include "cli.h"
#include "bk_vfs.h"
#include "bk_posix.h"
#include <components/bk_audio_player/bk_audio_player.h>
#include <components/bk_audio_player/plugins/decoders/bk_audio_player_mp3_decoder.h>
#include <components/bk_audio_player/plugins/decoders/bk_audio_player_wav_decoder.h>
#include <components/bk_audio_player/plugins/decoders/bk_audio_player_ts_decoder.h>
#include <components/bk_audio_player/plugins/decoders/bk_audio_player_aac_decoder.h>
#include <components/bk_audio_player/plugins/sources/bk_audio_player_file_source.h>
#include <components/bk_audio_player/plugins/sources/bk_audio_player_net_source.h>
#include <components/bk_audio_player/plugins/sources/bk_audio_player_hls_source.h>
#include <components/bk_audio_player/plugins/sinks/bk_audio_player_onboard_speaker_sink.h>
#include "bk_audio_player_private.h"

#define TAG "ap_cli"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)


#define CLI_CMD_RSP_SUCCEED               "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR                 "CMDRSP:ERROR\r\n"

static bk_audio_player_handle_t s_player_handle = NULL;

/* mount sdcard */
static int vfs_mount_sd0_fatfs(void)
{
	int ret = BK_OK;
	static bool is_mounted = false;

	if(!is_mounted) {
		struct bk_fatfs_partition partition;
		char *fs_name = NULL;
		fs_name = "fatfs";
		partition.part_type = FATFS_DEVICE;
		partition.part_dev.device_name = FATFS_DEV_SDCARD;
		partition.mount_path = VFS_SD_0_PATITION_0;
		ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);
		is_mounted = true;
        BK_LOGI(TAG, "func %s, mount /sd0 \n", __func__);
	}
	return ret;
}

static bk_err_t vfs_unmount_sd0_fatfs(void)
{
    return umount(VFS_SD_0_PATITION_0);
}

static int scan_media_files(bk_audio_player_handle_t handle, const char *path, int depth)
{
    char full_path[64];
    struct dirent *entry;
    char *ext_name;

    DIR *dir = opendir(path);
    if (dir == NULL)
    {
        LOGE("Error opening directory %s\n", path);
        return AUDIO_PLAYER_ERR;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        snprintf(full_path, MAX_PATH_LEN, "%s/%s", path, entry->d_name);
        ext_name = entry->d_name + strlen(entry->d_name) - 4;
        if (entry->d_type == DT_DIR)
        {
            if (depth > 0)
            {
                scan_media_files(handle, full_path, depth - 1);
            }
            else
            {
                LOGD("end:%s\n", full_path);
            }
        }
        else if (entry->d_type == DT_REG)
        {
            if ((strcasecmp(ext_name, ".mp3") == 0) ||
                (strcasecmp(ext_name, ".wav") == 0)  ||
                (strcasecmp(ext_name, ".m4a") == 0)  ||
                (strcasecmp(ext_name, ".amr") == 0)  ||
                (strcasecmp(ext_name + 1, ".ts") == 0)  ||
                (strcasecmp(ext_name, ".aac") == 0))
            {
                LOGD("=== %s====\r\n", full_path);
                if (handle)
                {
                    bk_audio_player_add_music(handle, entry->d_name, full_path);
                }
            }
        }
    }
    closedir(dir);

    return AUDIO_PLAYER_OK;
}

/* Register default sources, sinks and decoders used by this example. */
static int audio_player_register_default_plugins(bk_audio_player_handle_t handle)
{
    int ret;

    if (!handle)
    {
        return AUDIO_PLAYER_ERR;
    }

    /* Register built-in audio sources */
    {
        ret = bk_audio_player_register_source(handle, bk_audio_player_get_file_source_ops());
        if (ret != AUDIO_PLAYER_OK)
        {
            LOGE("bk_audio_player_register_source(file) failed, ret=%d\n", ret);
            return ret;
        }

        ret = bk_audio_player_register_source(handle, bk_audio_player_get_hls_source_ops());
        if (ret != AUDIO_PLAYER_OK)
        {
            LOGE("bk_audio_player_register_source(hls) failed, ret=%d\n", ret);
            return ret;
        }

        ret = bk_audio_player_register_source(handle, bk_audio_player_get_net_source_ops());
        if (ret != AUDIO_PLAYER_OK)
        {
            LOGE("bk_audio_player_register_source(net) failed, ret=%d\n", ret);
            return ret;
        }
    }

    /* Register built-in audio sinks */
    {
        ret = bk_audio_player_register_sink(handle, bk_audio_player_get_onboard_speaker_sink_ops());
        if (ret != AUDIO_PLAYER_OK)
        {
            LOGE("bk_audio_player_register_sink(device) failed, ret=%d\n", ret);
            return ret;
        }
    }

    /* Register built-in audio decoders */
    ret = bk_audio_player_register_decoder(handle, bk_audio_player_get_mp3_decoder_ops());
    if (ret != AUDIO_PLAYER_OK)
    {
        LOGE("bk_audio_player_register_decoder(mp3) failed, ret=%d\n", ret);
        return ret;
    }

    ret = bk_audio_player_register_decoder(handle, bk_audio_player_get_wav_decoder_ops());
    if (ret != AUDIO_PLAYER_OK)
    {
        LOGE("bk_audio_player_register_decoder(wav) failed, ret=%d\n", ret);
        return ret;
    }

    ret = bk_audio_player_register_decoder(handle, bk_audio_player_get_ts_decoder_ops());
    if (ret != AUDIO_PLAYER_OK)
    {
        LOGE("bk_audio_player_register_decoder(ts) failed, ret=%d\n", ret);
        return ret;
    }

    ret = bk_audio_player_register_decoder(handle, bk_audio_player_get_aac_decoder_ops());
    if (ret != AUDIO_PLAYER_OK)
    {
        LOGE("bk_audio_player_register_decoder(aac) failed, ret=%d\n", ret);
        return ret;
    }

    return AUDIO_PLAYER_OK;
}


void cli_audio_player_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *msg = CLI_CMD_RSP_ERROR;
    int ret = AUDIO_PLAYER_OK;

    if (os_strcmp(argv[1], "init") == 0)
    {
        bk_audio_player_cfg_t cfg = DEFAULT_AUDIO_PLAYER_CONFIG();
        ret = bk_audio_player_new(&s_player_handle, &cfg);
        if (ret == AUDIO_PLAYER_OK && s_player_handle)
        {
            ret = audio_player_register_default_plugins(s_player_handle);
        }
    }
    else if (os_strcmp(argv[1], "deinit") == 0)
    {
        if (s_player_handle)
        {
            bk_audio_player_delete(s_player_handle);
            s_player_handle = NULL;
        }
    }
    else if (os_strcmp(argv[1], "add") == 0)
    {
        ret = (s_player_handle ? bk_audio_player_add_music(s_player_handle, argv[2], argv[3]) : AUDIO_PLAYER_NOT_INIT);
    }
    else if (os_strcmp(argv[1], "rm") == 0)
    {
        ret = (s_player_handle ? bk_audio_player_del_music_by_uri(s_player_handle, argv[2]) : AUDIO_PLAYER_NOT_INIT);
    }
    else if (os_strcmp(argv[1], "clear") == 0)
    {
        ret = (s_player_handle ? bk_audio_player_clear_music_list(s_player_handle) : AUDIO_PLAYER_NOT_INIT);
    }
    else if (os_strcmp(argv[1], "dump") == 0)
    {
        ret = (s_player_handle ? bk_audio_player_dump_music_list(s_player_handle) : AUDIO_PLAYER_NOT_INIT);
    }
    else if (os_strcmp(argv[1], "mode") == 0)
    {
        audio_player_mode_t mode = (audio_player_mode_t)os_strtoul(argv[2], NULL, 10);
        ret = (s_player_handle ? bk_audio_player_set_play_mode(s_player_handle, mode) : AUDIO_PLAYER_NOT_INIT);
    }
    else if (os_strcmp(argv[1], "volume") == 0)
    {
        if (s_player_handle)
        {
            if (argc > 2)
            {
                int value = os_strtoul(argv[2], NULL, 10);
                ret = bk_audio_player_set_volume(s_player_handle, value);
            }
            else
            {
                ret = bk_audio_player_get_volume(s_player_handle);
            }
        }
        else
        {
            ret = AUDIO_PLAYER_NOT_INIT;
        }
    }
    else if (os_strcmp(argv[1], "start") == 0)
    {
        ret = (s_player_handle ? bk_audio_player_start(s_player_handle) : AUDIO_PLAYER_NOT_INIT);
    }
    else if (os_strcmp(argv[1], "stop") == 0)
    {
        ret = (s_player_handle ? bk_audio_player_stop(s_player_handle) : AUDIO_PLAYER_NOT_INIT);
    }
    else if (os_strcmp(argv[1], "pause") == 0)
    {
        ret = (s_player_handle ? bk_audio_player_pause(s_player_handle) : AUDIO_PLAYER_NOT_INIT);
    }
    else if (os_strcmp(argv[1], "resume") == 0)
    {
        ret = (s_player_handle ? bk_audio_player_resume(s_player_handle) : AUDIO_PLAYER_NOT_INIT);
    }
    else if (os_strcmp(argv[1], "prev") == 0)
    {
        ret = (s_player_handle ? bk_audio_player_prev(s_player_handle) : AUDIO_PLAYER_NOT_INIT);
    }
    else if (os_strcmp(argv[1], "next") == 0)
    {
        ret = (s_player_handle ? bk_audio_player_next(s_player_handle) : AUDIO_PLAYER_NOT_INIT);
    }
    else if (os_strcmp(argv[1], "jump") == 0)
    {
        if (argc > 2 && s_player_handle)
        {
            int id = os_strtoul(argv[2], NULL, 10);
            ret = bk_audio_player_jumpto(s_player_handle, id);
        }
        else
        {
            ret = AUDIO_PLAYER_NOT_INIT;
        }
    }
    else if (os_strcmp(argv[1], "sd_scan") == 0)
    {
        char path[] = "/sd0";
        ret = scan_media_files(s_player_handle, path, 1);
    }
    else if (os_strcmp(argv[1], "sd_mount") == 0)
    {
        ret = vfs_mount_sd0_fatfs();
    }
    else if (os_strcmp(argv[1], "sd_unmount") == 0)
    {
        ret = vfs_unmount_sd0_fatfs();
    }
    else
    {
        //nothing
    }

    LOGD("audio_player ret=%d\n", ret);

    if (ret == AUDIO_PLAYER_OK)
    {
        msg = CLI_CMD_RSP_SUCCEED;
    }
    else
    {
        if (s_player_handle)
        {
            bk_audio_player_stop(s_player_handle);
            bk_audio_player_delete(s_player_handle);
            s_player_handle = NULL;
        }
        vfs_unmount_sd0_fatfs();
        msg = CLI_CMD_RSP_ERROR;
    }
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

#define AUDIO_PLAYER_CMD_CNT  (sizeof(s_audio_player_commands) / sizeof(struct cli_command))
static const struct cli_command s_audio_player_commands[] =
{
    {"audio_player", "audio_player ...", cli_audio_player_cmd},
};

int cli_audio_player_init(void)
{
    return cli_register_commands(s_audio_player_commands, AUDIO_PLAYER_CMD_CNT);
}
