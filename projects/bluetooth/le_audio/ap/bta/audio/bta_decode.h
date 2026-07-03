#pragma once

#include <stdbool.h>
#include "bk_ring_buffer_node.h"
#include "bk_ring_buffer_octets.h"
#include <os/os.h>
#include <components/bluetooth/bk_dm_bap_types.h>
#include <components/bluetooth/bk_assigned_numbers.h>
#include <modules/lc3_codec.h>

#include <bk_list.h>

typedef void (*bta_decode_data_cb)(uint8_t *data, uint32_t length);

typedef struct
{
    uint8_t format;
    uint8_t channels;
    uint32_t sample_rate;
    uint32_t frame_length;
    uint32_t duration;

    bta_decode_data_cb cb;
} bta_decode_config_t;

int bta_decode_start(bta_decode_config_t *cfg);
int bta_decode_stop(void);
int bta_decode_write(uint8_t *data, uint16_t length);
