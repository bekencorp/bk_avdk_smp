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
#include "components/bk_decode/bk_h264_decode_types.h"
#include "components/bk_frame_buffer.h"
#include "modules/vcdec/vcdec_h264_api.h"
#include "bk_flexa_bond_types.h"

#ifdef __cplusplus
extern "C" {
#endif


struct h264d_fbpool;

typedef struct {
	bk_flexa_bond_t *bond;
	uint8_t first_bond;
	uint32_t rd_blocks;
} bk_h264_decode_port_entry_t;

typedef struct {
	vcdec_handle vcdec_handle;
	vcdec_flexa_mode_e mode;
	vcdec_h264_decode_config_t decode_config;
	uint32_t decode_result;
	beken_semaphore_t decode_done_sem;
	beken_mutex_t osd_mutex;

	bk_h264_decode_frame_config_t config;
	bk_h264_decode_ctlr_t ops;
} private_h264_decode_frame_ctlr_t;

typedef struct {
	vcdec_handle vcdec_handle;
	vcdec_flexa_mode_e mode;
	vcdec_h264_decode_config_t decode_config;
	uint32_t decode_result;
	beken_semaphore_t decode_done_sem;

	struct h264d_fbpool *pool;   /* internally owned zero-copy frame pool */
	vcdec_fb_if_t fbif;          /* decode-side vtable exported by the pool */

	/* Frame-drop resync state (controller-owned policy). Set when a reference/
	 * IDR frame had to be dropped because a decode target could not be acquired
	 * within the wait budget: the reference chain is broken, so every following
	 * non-IDR access unit is skipped until the next IDR re-establishes a
	 * self-contained resync point. Accessed only on the hw-decoder worker
	 * thread (the decode callback), plus cleared on RESET. */
	uint8_t skip_until_idr;

	bk_h264_decode_frame_zerocopy_config_t config;
	bk_h264_decode_ctlr_t ops;
} private_h264_decode_frame_zerocopy_ctlr_t;

typedef struct {
	vcdec_handle vcdec_handle;
	vcdec_flexa_mode_e mode;
	vcdec_h264_decode_config_t decode_config;
	uint32_t decode_result;
	beken_semaphore_t decode_done_sem;
	beken_event_t port_done_events;
	uint32_t all_ports_min_rd;

	bk_h264_decode_port_entry_t port[BK_H264_DECODE_RD_PORT_MAX];

	bk_h264_decode_flexa_config_t config;
	bk_h264_decode_ctlr_t ops;
} private_h264_decode_flexa_ctlr_t;

static inline void *h264_decode_mem_malloc(uint32_t size)
{
	return bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, size);
}

static inline void h264_decode_mem_free(void *ptr)
{
	bk_frame_buffer_free(ptr);
}

/* Frame-pool slot allocator (h264d_fbpool_alloc_cb signature). The underlying
 * uncoded frame-buffer heap already returns DMA-aligned blocks, so the explicit
 * alignment hint is advisory and ignored here. */
static inline void *h264_decode_mem_malloc_align(uint32_t size, uint32_t align)
{
	(void)align;
	return bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, size);
}

#ifdef __cplusplus
}
#endif
