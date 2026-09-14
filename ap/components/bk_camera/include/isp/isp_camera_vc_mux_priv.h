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

#pragma once

#include <components/bk_isp_camera_types.h>
#include <components/bk_camera_isp_ctlr.h>
#include <driver/isp_types.h>
#include <os/os.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ISP_CAMERA_VC_MUX_NODE_MAX (7)
#define ISP_CAMERA_VC_MUX_QUEUE_MAX (2)
#define ISP_CAMERA_VC_MUX_NODE_INVALID (0xFF)
#define ISP_CAMERA_VC_MUX_APPLIED_INVALID (0xFF)

typedef struct
{
    VIDEO_BUF_S buf;
    uint8_t vc;
    uint8_t queued;
    uint8_t in_use;
    uint8_t valid;
    uint8_t next;
    uint32_t frame_size;
    uint32_t sequence;
} isp_camera_vc_mux_node_t;

typedef struct
{
    bk_isp_camera_ctlr_handle_t camera;
    uint8_t enable;
    uint8_t channel;
    uint8_t vc_enable_mask;
    uint8_t current_vc;
    uint8_t applied_vc;
    uint8_t default_discard_frames;
    uint8_t vc_discard_frames[2];
    uint8_t discard_remaining[2];
    uint8_t completed_vc;
    uint8_t completed_valid;
    uint8_t switch_pending;
    uint8_t drain_stale_remaining;
    uint8_t isr_registered;
    uint8_t timeout_count;
    uint8_t runtime_vc_switch;
    uint16_t width;
    uint16_t height;
    uint32_t frame_size;
    beken_mutex_t lock;
    uint8_t lock_inited;
    isp_camera_vc_mux_node_t node[ISP_CAMERA_VC_MUX_NODE_MAX];
    uint8_t free_head;
    uint8_t queue_head[2];
    uint8_t queue_tail[2];
    uint8_t queue_count[2];
    uint32_t sequence[2];
    uint8_t thread_enable;
    beken_thread_t thread;
    uint32_t pop_timeout_ms;
    bk_camera_vc_mux_ctlr_t ops;
} bk_camera_isp_vc_mux_t;

#ifdef __cplusplus
}
#endif
