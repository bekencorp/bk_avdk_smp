#pragma once

#include <stdbool.h>
#include "bk_ring_buffer_node.h"
#include "bk_ring_buffer_octets.h"
#include <os/os.h>
#include <components/bluetooth/bk_dm_bap_types.h>
#include <components/bluetooth/bk_assigned_numbers.h>
#include <modules/lc3_codec.h>

typedef void (*bta_resample_data_cb)(uint8_t *data, uint32_t length);

typedef struct
{
    uint32_t src_sample;
    uint32_t dst_sample;
    uint32_t duration; /* us */
    uint32_t channels;
    bta_resample_data_cb cb;
} bta_resample_config_t;

void bta_resample_start(bta_resample_config_t *config);
void bta_resample_stop(void);
void bta_resample_write(uint8 *data, uint32_t length);
uint8_t bta_resample_is_enabled(void);

