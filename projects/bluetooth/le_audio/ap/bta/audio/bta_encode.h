#pragma once

#include <stdbool.h>
#include "bk_ring_buffer_node.h"
#include "bk_ring_buffer_octets.h"
#include <os/os.h>
#include <components/bluetooth/bk_dm_bap_types.h>
#include <components/bluetooth/bk_assigned_numbers.h>
#include <modules/lc3_codec.h>

typedef void (*bta_encode_data_cb)(uint8_t *data, uint32_t length);

typedef struct
{
    uint8_t format;
    uint8_t channels;
    uint32_t sample_rate;
    uint32_t frame_length;
    uint32_t duration;

    bta_encode_data_cb cb;
} bta_encode_config_t;

void bta_encode_start(bta_encode_config_t *config);
void bta_encode_stop(void);
void bta_encode_write(uint8 *data, uint32_t length);
uint8_t bta_encode_is_enabled(void);

