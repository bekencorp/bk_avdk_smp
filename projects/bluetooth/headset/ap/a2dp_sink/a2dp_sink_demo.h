#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
    uint8_t type;
    uint16_t len;
    uint8_t *data;
} bt_audio_sink_msg_t;

typedef struct
{
    uint32_t attr_id;
    uint16_t attr_text_charset;
    uint32_t attr_length;
    uint8_t *attr_text;
} a2dp_sink_avrcp_attr_t;

typedef struct
{
    uint8_t status;
    uint8_t attr_count;
    uint8_t remote_bda[6];
    a2dp_sink_avrcp_attr_t attr_array[];
} a2dp_sink_avrcp_elem_attr_msg_t;

int a2dp_sink_demo_init(uint8_t aac_supported, uint8_t auto_accept_conn);
int a2dp_sink_demo_deinit(void);

int32_t a2dp_sink_demo_wait_player_end(void);

void a2dp_sink_demo_audio_spk_enable(uint8_t enable);
void a2dp_sink_demo_set_mix(uint8_t enable);

int32_t a2dp_sink_demo_try_connect(void);
int32_t a2dp_sink_demo_try_disconnect_current(void);

#ifdef __cplusplus
}
#endif
