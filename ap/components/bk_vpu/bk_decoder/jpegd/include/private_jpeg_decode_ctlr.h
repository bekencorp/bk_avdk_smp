// Copyright 2020-2021 Beken
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

#pragma once

#include "os/os.h"
#include "components/bk_decode/bk_jpeg_decode_types.h"
#include "modules/vcdec/vcdec_jpeg_types.h"
#include "bk_flexa_bond_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bk_flexa_bond_t *bond;
    uint8_t first_bond;
    uint8_t done;           /* Port reported the full-frame value this frame (abort/end); excluded from backpressure min; cleared every frame */
    uint32_t rd_blocks;
} bk_jpeg_decode_port_entry_t;

typedef struct {
    vcdec_handle vcdec_handle;      /* from vcdec_jpeg_open */
    bk_jpeg_decode_flexa_mode_t mode;        /* flexa mode */
    vcdec_jpeg_decode_config_t decode_config;  /* filled per decode */
    uint32_t decode_result;
    beken_semaphore_t decode_done_sem;

    bk_jpeg_decode_frame_config_t config;
    bk_jpeg_decode_ctlr_t ops;
} private_jpeg_decode_frame_ctlr_t;

typedef struct {
    vcdec_handle vcdec_handle;  /* from vcdec_jpeg_open */
    bk_jpeg_decode_flexa_mode_t mode;        /* flexa mode */
    vcdec_jpeg_decode_config_t decode_config;  /* filled per decode */
    uint32_t decode_result;
    beken_semaphore_t decode_done_sem;
    /** Flexa completion flags for each output port: bit i corresponds to port_id == i (BK_JPEG_DECODE_RD_PORT_MAX ports). */
    beken_event_t port_done_events;
    uint32_t all_ports_min_rd;
    uint32_t last_flexa_line;
    uint32_t frame_seq;         /* Monotonic decode frame counter (never 0); +1 at each frame boundary. Delivered to consumers via bond->last_seq so their reports can be seq-gated against the current frame. */

    bk_jpeg_decode_port_entry_t port[BK_JPEG_DECODE_RD_PORT_MAX];

    bk_jpeg_decode_flexa_config_t config;
    bk_jpeg_decode_ctlr_t ops;
} private_jpeg_decode_flexa_ctlr_t;

#ifdef __cplusplus
}
#endif
