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

/* This file is used to debug uac work status by collecting statistics on the uac mic and speaker. */
#include "sdkconfig.h"
#include <os/os.h>
#include <os/mem.h>
#include <components/bk_audio/audio_utils/uart_util.h>
#include <components/bk_audio/audio_utils/debug_dump_util.h>
#if CONFIG_ADK_WIFI_DUMP_UTIL
#include <components/bk_audio/audio_utils/wifi_dump_util.h>
#endif

#if CONFIG_ADK_DEBUG_DUMP_UTIL

#define AUD_DUMP_TAG "aud_dump"

#define LOGI(...) BK_LOGI(AUD_DUMP_TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(AUD_DUMP_TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(AUD_DUMP_TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(AUD_DUMP_TAG, ##__VA_ARGS__)
#if CONFIG_ADK_DEBUG_DUMP_DATA_TYPE_EXTENSION
volatile debug_dump_data_header_t dump_header[HEADER_ARRAY_CNT] = 
{
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 1,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_DEC_OUT_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
            .sample_rate = 16000,
            .frame_in_ms = 20,
            .ch_num = 1,
         },
        .seq_no = 0
    },
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 1,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_DEC_IN_DATA,
            .dump_file_type = DUMP_FILE_TYPE_G722,
            .len = 0,
            .sample_rate = 16000,
            .frame_in_ms = 20,
            .ch_num = 1,
         },
        .seq_no = 0
    },
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 1,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_ENC_OUT_DATA,
            .dump_file_type = DUMP_FILE_TYPE_G722,
            .len = 0,
            .sample_rate = 16000,
            .frame_in_ms = 20,
            .ch_num = 1,
         },
        .seq_no = 0
    },
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 3,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_AEC_MIC_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
            .sample_rate = 16000,
            .frame_in_ms = 20,
            .ch_num = 1,
         },
        .data_flow[1] = 
         {
            .dump_type = DUMP_TYPE_AEC_REF_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
            .sample_rate = 16000,
            .frame_in_ms = 20,
            .ch_num = 1,
         },
        .data_flow[2] = 
         {
            .dump_type = DUMP_TYPE_AEC_OUT_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
            .sample_rate = 16000,
            .frame_in_ms = 20,
            .ch_num = 1,
         },
        .seq_no = 0
    },
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 1,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_ENC_IN_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
            .sample_rate = 16000,
            .frame_in_ms = 20,
            .ch_num = 1,
         },
        .seq_no = 0
    },
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 1,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_EQ_IN_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
            .sample_rate = 16000,
            .frame_in_ms = 20,
            .ch_num = 1,
         },
        .seq_no = 0
    },
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 1,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_EQ_OUT_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
            .sample_rate = 16000,
            .frame_in_ms = 20,
            .ch_num = 1,
         },
        .seq_no = 0
    },
};
#else
volatile debug_dump_data_header_t dump_header[HEADER_ARRAY_CNT] = 
{
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 1,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_DEC_OUT_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
         },
        .seq_no = 0
    },
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 1,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_DEC_IN_DATA,
            .dump_file_type = DUMP_FILE_TYPE_G722,
            .len = 0,
         },
        .seq_no = 0
    },
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 1,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_ENC_OUT_DATA,
            .dump_file_type = DUMP_FILE_TYPE_G722,
            .len = 0,
         },
        .seq_no = 0
    },
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 3,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_AEC_MIC_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
         },
        .data_flow[1] = 
         {
            .dump_type = DUMP_TYPE_AEC_REF_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
         },
        .data_flow[2] = 
         {
            .dump_type = DUMP_TYPE_AEC_OUT_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
         },
        .seq_no = 0
    },
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 1,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_ENC_IN_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
         },
        .seq_no = 0
    },
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 1,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_EQ_IN_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
         },
        .seq_no = 0
    },
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 1,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_EQ_OUT_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
         },
        .seq_no = 0
    },
};
#endif

const uint8_t g_dump_type2header_array_idx[DUMP_TYPE_MAX] = 
{
    DUMP_TYPE_DEC_OUT_DATA,
    DUMP_TYPE_DEC_IN_DATA,
    DUMP_TYPE_ENC_OUT_DATA,
    DUMP_TYPE_AEC_MIC_DATA,
    DUMP_TYPE_AEC_MIC_DATA,
    DUMP_TYPE_AEC_MIC_DATA,
    DUMP_TYPE_ENC_IN_DATA-2,//AEC MIC/REF/OUT DATA use same HEADER
    DUMP_TYPE_EQ_IN_DATA-2,//AEC MIC/REF/OUT DATA use same HEADER
    DUMP_TYPE_EQ_OUT_DATA-2,//AEC MIC/REF/OUT DATA use same HEADER
};

struct uart_util g_debug_data_uart_util = {0};
uint16_t g_aud_data_dump_bitmap = 0;
static debug_dump_transport_t s_dump_transport = DEBUG_DUMP_TRANSPORT_UART;
#if CONFIG_ADK_WIFI_DUMP_UTIL
static uint16_t s_wifi_dump_port = CONFIG_ADK_WIFI_DUMP_PORT;
#endif

bk_err_t debug_data_dump_set_transport(debug_dump_transport_t transport,
                                        uint16_t port)
{
    if (get_aud_dump_bitmap() != 0)
    {
        LOGE("stop active dump before changing transport\n");
        return BK_FAIL;
    }

    if (transport == DEBUG_DUMP_TRANSPORT_UART)
    {
        s_dump_transport = transport;
        return BK_OK;
    }

#if CONFIG_ADK_WIFI_DUMP_UTIL
    if (transport == DEBUG_DUMP_TRANSPORT_WIFI)
    {
        if (port != 0)
        {
            s_wifi_dump_port = port;
        }
        if (wifi_dump_util_start(s_wifi_dump_port) != BK_OK)
        {
            LOGE("start WiFi dump server failed\n");
            return BK_FAIL;
        }
        s_dump_transport = transport;
        return BK_OK;
    }
#endif

    return BK_FAIL;
}

debug_dump_transport_t debug_data_dump_get_transport(void)
{
    return s_dump_transport;
}

bk_err_t debug_data_dump_open(void)
{
    if (s_dump_transport == DEBUG_DUMP_TRANSPORT_UART)
    {
        return uart_util_create(&g_debug_data_uart_util,
                                DEBUG_DATA_DUMP_UART_ID,
                                DEBUG_DATA_DUMP_UART_BAUD_RATE);
    }

#if CONFIG_ADK_WIFI_DUMP_UTIL
    return wifi_dump_util_is_connected() ? BK_OK : BK_FAIL;
#else
    return BK_FAIL;
#endif
}

void debug_data_dump_close(void)
{
    if (s_dump_transport == DEBUG_DUMP_TRANSPORT_UART)
    {
        uart_util_destroy(&g_debug_data_uart_util);
        return;
    }

#if CONFIG_ADK_WIFI_DUMP_UTIL
    wifi_dump_util_close_client();
#endif
}

void debug_data_dump_abort(void)
{
    g_aud_data_dump_bitmap = 0;
    debug_data_dump_close();
}

bool debug_data_dump_is_ready(void)
{
    if (s_dump_transport == DEBUG_DUMP_TRANSPORT_UART)
    {
        return true;
    }

#if CONFIG_ADK_WIFI_DUMP_UTIL
    return wifi_dump_util_is_connected();
#else
    return false;
#endif
}

bk_err_t debug_data_dump_send_aec(const void *mic, uint32_t mic_len,
                                  const void *ref, uint32_t ref_len,
                                  const void *out, uint32_t out_len)
{
    const void *header =
        (const void *)&dump_header[g_dump_type2header_array_idx[DUMP_TYPE_AEC_MIC_DATA]];

    if (s_dump_transport == DEBUG_DUMP_TRANSPORT_UART)
    {
        DEBUG_DATA_DUMP_SUSPEND_ALL;
        uart_util_tx_data(&g_debug_data_uart_util, (void *)header,
                          sizeof(debug_dump_data_header_t));
        uart_util_tx_data(&g_debug_data_uart_util, (void *)mic, mic_len);
        uart_util_tx_data(&g_debug_data_uart_util, (void *)ref, ref_len);
        uart_util_tx_data(&g_debug_data_uart_util, (void *)out, out_len);
        DEBUG_DATA_DUMP_RESUME_ALL;
        return BK_OK;
    }

#if CONFIG_ADK_WIFI_DUMP_UTIL
    return wifi_dump_util_enqueue(header, sizeof(debug_dump_data_header_t),
                                  mic, mic_len, ref, ref_len, out, out_len);
#else
    return BK_FAIL;
#endif
}

#endif /*CONFIG_ADK_DEBUG_DUMP_UTIL*/


