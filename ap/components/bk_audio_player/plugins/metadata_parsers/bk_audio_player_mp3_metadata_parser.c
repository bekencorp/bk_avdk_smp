// Copyright 2024-2025 Beken
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

//#include "audio_metadata_parser.h"
#include "audio_metadata_parser_common.h"
#include "player_mem.h"
#include "player_osal.h"
//#include "play_manager.h"

#include <os/mem.h>
#include <os/str.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <bk_posix.h>

typedef struct mp3_parser_context_s
{
    audio_metadata_t *metadata;
    int fd;
} mp3_parser_context_t;

static int mp3_parse_id3v1(mp3_parser_context_t *ctx)
{
    uint8_t tag_data[128];
    int read_size;
    int file_size;

    file_size = lseek(ctx->fd, 0, SEEK_END);
    if (file_size < (int)sizeof(tag_data))
    {
        return -1;
    }

    if (lseek(ctx->fd, -(off_t)sizeof(tag_data), SEEK_END) < 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "MP3 metadata: seek to ID3v1 failed\n");
        return -1;
    }

    read_size = read(ctx->fd, tag_data, sizeof(tag_data));
    if (read_size != (int)sizeof(tag_data))
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "MP3 metadata: read ID3v1 failed, bytes=%d\n", read_size);
        return -1;
    }

    if (tag_data[0] != 'T' || tag_data[1] != 'A' || tag_data[2] != 'G')
    {
        return -1;
    }

    if (ctx->metadata->title[0] == '\0')
    {
        metadata_safe_string_copy(ctx->metadata->title, (char *)&tag_data[3], 30);
    }

    if (ctx->metadata->artist[0] == '\0')
    {
        metadata_safe_string_copy(ctx->metadata->artist, (char *)&tag_data[33], 30);
    }

    if (ctx->metadata->album[0] == '\0')
    {
        metadata_safe_string_copy(ctx->metadata->album, (char *)&tag_data[63], 30);
    }

    if (ctx->metadata->year[0] == '\0')
    {
        metadata_safe_string_copy(ctx->metadata->year, (char *)&tag_data[93], 4);
    }

    if (ctx->metadata->genre[0] == '\0' && tag_data[127] < 255)
    {
        os_snprintf(ctx->metadata->genre, AUDIO_METADATA_MAX_STRING_LEN, "%u", tag_data[127]);
    }

    if (ctx->metadata->track_number[0] == '\0' && tag_data[125] == 0 && tag_data[126] != 0)
    {
        os_snprintf(ctx->metadata->track_number, AUDIO_METADATA_MAX_STRING_LEN, "%u", tag_data[126]);
    }

    BK_LOGI(AUDIO_PLAYER_TAG, "MP3 metadata: ID3v1 parsed\n");
    return 0;
}

static int mp3_get_bitrate_from_frame(mp3_parser_context_t *ctx)
{
    static const int bitrate_table_v1_l3[] = {
        0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0
    };
    static const int bitrate_table_v2_l3[] = {
        0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0
    };

    const int buffer_size = 4096;
    uint8_t *buffer = NULL;
    int read_size;
    int i;
    int version;
    int layer;
    int bitrate_index;
    int ret = -1;

    buffer = (uint8_t *)os_malloc(buffer_size);
    if (!buffer)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "MP3 metadata: MP3 frame buffer alloc failed\n");
        return -1;
    }

    if (lseek(ctx->fd, 0, SEEK_SET) < 0)
    {
        os_free(buffer);
        return -1;
    }

    read_size = read(ctx->fd, buffer, buffer_size);
    if (read_size < 4)
    {
        os_free(buffer);
        return -1;
    }

    for (i = 0; i < read_size - 3; i++)
    {
        if (buffer[i] == 0xFF && (buffer[i + 1] & 0xE0) == 0xE0)
        {
            version = (buffer[i + 1] >> 3) & 0x03;
            layer = (buffer[i + 1] >> 1) & 0x03;
            bitrate_index = (buffer[i + 2] >> 4) & 0x0F;

            if (bitrate_index == 0 || bitrate_index == 15)
            {
                continue;
            }

            if (version == 3 && layer == 1)
            {
                ctx->metadata->bitrate = bitrate_table_v1_l3[bitrate_index] * 1000;
                ret = 0;
                break;
            }
            else if ((version == 2 || version == 0) && layer == 1)
            {
                ctx->metadata->bitrate = bitrate_table_v2_l3[bitrate_index] * 1000;
                ret = 0;
                break;
            }
        }
    }

    if (ret != 0)
    {
        BK_LOGW(AUDIO_PLAYER_TAG, "MP3 metadata: unable to detect bitrate from frame\n");
        ctx->metadata->bitrate = 128000;
    }

    os_free(buffer);
    return ret;
}

static int mp3_calculate_duration(mp3_parser_context_t *ctx)
{
    int file_size;
    uint32_t audio_data_size;
    uint64_t temp;
    uint32_t duration_ms;

    file_size = lseek(ctx->fd, 0, SEEK_END);
    if (file_size <= 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "MP3 metadata: invalid file size %d\n", file_size);
        return -1;
    }

    audio_data_size = (uint32_t)file_size;

    if (ctx->metadata->bitrate == 0)
    {
        ctx->metadata->bitrate = 128000;
    }

    temp = (uint64_t)audio_data_size * 8000;
    duration_ms = (uint32_t)(temp / ctx->metadata->bitrate);
    ctx->metadata->duration = (double)duration_ms;
    return 0;
}

static int mp3_metadata_parse(int fd, const char *filepath, audio_metadata_t *metadata)
{
    mp3_parser_context_t ctx = {
        .metadata = metadata,
        .fd = fd,
    };

    if (metadata_parse_id3v2(fd, metadata) == AUDIO_PLAYER_OK)
    {
        BK_LOGI(AUDIO_PLAYER_TAG, "MP3 metadata: ID3v2 duration %u ms\n", (uint32_t)metadata->duration);
    }

    if (mp3_parse_id3v1(&ctx) == 0)
    {
        metadata->has_id3v1 = 1;
    }

    if (metadata->bitrate == 0)
    {
        mp3_get_bitrate_from_frame(&ctx);
    }

    if (metadata->duration == 0.0)
    {
        mp3_calculate_duration(&ctx);
    }

    metadata_fill_title_from_path(filepath, metadata);
    return AUDIO_PLAYER_OK;
}

const bk_audio_player_metadata_parser_ops_t mp3_metadata_parser_ops = {
    .name = "mp3",
    .format = AUDIO_FORMAT_MP3,
    .probe = NULL,
    .parse = mp3_metadata_parse,
};

/* Get MP3 metadata parser operations structure */
const bk_audio_player_metadata_parser_ops_t *bk_audio_player_get_mp3_metadata_parser_ops(void)
{
    return &mp3_metadata_parser_ops;
}
