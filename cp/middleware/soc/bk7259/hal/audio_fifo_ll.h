// Copyright 2022-2025 Beken
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

// This is a generated file, if you need to modify it, use the script to
// generate and modify all the struct.h, ll.h, reg.h, debug_dump.c files!

#pragma once

#include <soc/soc.h>
#include "hal_port.h"
#include "audio_fifo_hw.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AUDIO_FIFO_LL_REG_BASE   SOC_AUDIO_FIFO_REG_BASE

//reg spk0_a2dp_port:

static inline void audio_fifo_ll_set_spk0_a2dp_port_value(uint32_t v) {
	audio_fifo_spk0_a2dp_port_t *r = (audio_fifo_spk0_a2dp_port_t*)(SOC_AUDIO_FIFO_REG_BASE + (0x40 << 2));
	r->v = v;
}

static inline uint32_t audio_fifo_ll_get_spk0_a2dp_port_value(void) {
	audio_fifo_spk0_a2dp_port_t *r = (audio_fifo_spk0_a2dp_port_t*)(SOC_AUDIO_FIFO_REG_BASE + (0x40 << 2));
	return r->v;
}

static inline void audio_fifo_ll_set_spk0_a2dp_port_spk0_a2dp(uint32_t v) {
	audio_fifo_spk0_a2dp_port_t *r = (audio_fifo_spk0_a2dp_port_t*)(SOC_AUDIO_FIFO_REG_BASE + (0x40 << 2));
	r->spk0_a2dp = v;
}

static inline uint32_t audio_fifo_ll_get_spk0_a2dp_port_spk0_a2dp(void) {
	audio_fifo_spk0_a2dp_port_t *r = (audio_fifo_spk0_a2dp_port_t*)(SOC_AUDIO_FIFO_REG_BASE + (0x40 << 2));
	return r->spk0_a2dp;
}

//reg spk0_call_port:

static inline void audio_fifo_ll_set_spk0_call_port_value(uint32_t v) {
	audio_fifo_spk0_call_port_t *r = (audio_fifo_spk0_call_port_t*)(SOC_AUDIO_FIFO_REG_BASE + (0x41 << 2));
	r->v = v;
}

static inline uint32_t audio_fifo_ll_get_spk0_call_port_value(void) {
	audio_fifo_spk0_call_port_t *r = (audio_fifo_spk0_call_port_t*)(SOC_AUDIO_FIFO_REG_BASE + (0x41 << 2));
	return r->v;
}

static inline void audio_fifo_ll_set_spk0_call_port_spk0_call(uint32_t v) {
	audio_fifo_spk0_call_port_t *r = (audio_fifo_spk0_call_port_t*)(SOC_AUDIO_FIFO_REG_BASE + (0x41 << 2));
	r->spk0_call = v;
}

static inline uint32_t audio_fifo_ll_get_spk0_call_port_spk0_call(void) {
	audio_fifo_spk0_call_port_t *r = (audio_fifo_spk0_call_port_t*)(SOC_AUDIO_FIFO_REG_BASE + (0x41 << 2));
	return r->spk0_call;
}

//reg spk0_hint_port:

static inline void audio_fifo_ll_set_spk0_hint_port_value(uint32_t v) {
	audio_fifo_spk0_hint_port_t *r = (audio_fifo_spk0_hint_port_t*)(SOC_AUDIO_FIFO_REG_BASE + (0x42 << 2));
	r->v = v;
}

static inline uint32_t audio_fifo_ll_get_spk0_hint_port_value(void) {
	audio_fifo_spk0_hint_port_t *r = (audio_fifo_spk0_hint_port_t*)(SOC_AUDIO_FIFO_REG_BASE + (0x42 << 2));
	return r->v;
}

static inline void audio_fifo_ll_set_spk0_hint_port_spk0_hint(uint32_t v) {
	audio_fifo_spk0_hint_port_t *r = (audio_fifo_spk0_hint_port_t*)(SOC_AUDIO_FIFO_REG_BASE + (0x42 << 2));
	r->spk0_hint = v;
}

static inline uint32_t audio_fifo_ll_get_spk0_hint_port_spk0_hint(void) {
	audio_fifo_spk0_hint_port_t *r = (audio_fifo_spk0_hint_port_t*)(SOC_AUDIO_FIFO_REG_BASE + (0x42 << 2));
	return r->spk0_hint;
}

//reg spk1_a2dp_port:

static inline void audio_fifo_ll_set_spk1_a2dp_port_value(uint32_t v) {
	audio_fifo_spk1_a2dp_port_t *r = (audio_fifo_spk1_a2dp_port_t*)(SOC_AUDIO_FIFO_REG_BASE + (0x43 << 2));
	r->v = v;
}

static inline uint32_t audio_fifo_ll_get_spk1_a2dp_port_value(void) {
	audio_fifo_spk1_a2dp_port_t *r = (audio_fifo_spk1_a2dp_port_t*)(SOC_AUDIO_FIFO_REG_BASE + (0x43 << 2));
	return r->v;
}

static inline void audio_fifo_ll_set_spk1_a2dp_port_spk1_a2dp(uint32_t v) {
	audio_fifo_spk1_a2dp_port_t *r = (audio_fifo_spk1_a2dp_port_t*)(SOC_AUDIO_FIFO_REG_BASE + (0x43 << 2));
	r->spk1_a2dp = v;
}

static inline uint32_t audio_fifo_ll_get_spk1_a2dp_port_spk1_a2dp(void) {
	audio_fifo_spk1_a2dp_port_t *r = (audio_fifo_spk1_a2dp_port_t*)(SOC_AUDIO_FIFO_REG_BASE + (0x43 << 2));
	return r->spk1_a2dp;
}

//reg spk1_call_port:

static inline void audio_fifo_ll_set_spk1_call_port_value(uint32_t v) {
	audio_fifo_spk1_call_port_t *r = (audio_fifo_spk1_call_port_t*)(SOC_AUDIO_FIFO_REG_BASE + (0x44 << 2));
	r->v = v;
}

static inline uint32_t audio_fifo_ll_get_spk1_call_port_value(void) {
	audio_fifo_spk1_call_port_t *r = (audio_fifo_spk1_call_port_t*)(SOC_AUDIO_FIFO_REG_BASE + (0x44 << 2));
	return r->v;
}

static inline void audio_fifo_ll_set_spk1_call_port_spk1_call(uint32_t v) {
	audio_fifo_spk1_call_port_t *r = (audio_fifo_spk1_call_port_t*)(SOC_AUDIO_FIFO_REG_BASE + (0x44 << 2));
	r->spk1_call = v;
}

static inline uint32_t audio_fifo_ll_get_spk1_call_port_spk1_call(void) {
	audio_fifo_spk1_call_port_t *r = (audio_fifo_spk1_call_port_t*)(SOC_AUDIO_FIFO_REG_BASE + (0x44 << 2));
	return r->spk1_call;
}

//reg spk1_hint_port:

static inline void audio_fifo_ll_set_spk1_hint_port_value(uint32_t v) {
	audio_fifo_spk1_hint_port_t *r = (audio_fifo_spk1_hint_port_t*)(SOC_AUDIO_FIFO_REG_BASE + (0x45 << 2));
	r->v = v;
}

static inline uint32_t audio_fifo_ll_get_spk1_hint_port_value(void) {
	audio_fifo_spk1_hint_port_t *r = (audio_fifo_spk1_hint_port_t*)(SOC_AUDIO_FIFO_REG_BASE + (0x45 << 2));
	return r->v;
}

static inline void audio_fifo_ll_set_spk1_hint_port_spk1_hint(uint32_t v) {
	audio_fifo_spk1_hint_port_t *r = (audio_fifo_spk1_hint_port_t*)(SOC_AUDIO_FIFO_REG_BASE + (0x45 << 2));
	r->spk1_hint = v;
}

static inline uint32_t audio_fifo_ll_get_spk1_hint_port_spk1_hint(void) {
	audio_fifo_spk1_hint_port_t *r = (audio_fifo_spk1_hint_port_t*)(SOC_AUDIO_FIFO_REG_BASE + (0x45 << 2));
	return r->spk1_hint;
}

//reg mic0_data_bus:

static inline void audio_fifo_ll_set_mic0_data_bus_value(uint32_t v) {
	audio_fifo_mic0_data_bus_t *r = (audio_fifo_mic0_data_bus_t*)(SOC_AUDIO_FIFO_REG_BASE + (0x46 << 2));
	r->v = v;
}

static inline uint32_t audio_fifo_ll_get_mic0_data_bus_value(void) {
	audio_fifo_mic0_data_bus_t *r = (audio_fifo_mic0_data_bus_t*)(SOC_AUDIO_FIFO_REG_BASE + (0x46 << 2));
	return r->v;
}

static inline uint32_t audio_fifo_ll_get_mic0_data_bus_mic0_data_bus(void) {
	audio_fifo_mic0_data_bus_t *r = (audio_fifo_mic0_data_bus_t*)(SOC_AUDIO_FIFO_REG_BASE + (0x46 << 2));
	return r->mic0_data_bus;
}

//reg mic1_data_bus:

static inline void audio_fifo_ll_set_mic1_data_bus_value(uint32_t v) {
	audio_fifo_mic1_data_bus_t *r = (audio_fifo_mic1_data_bus_t*)(SOC_AUDIO_FIFO_REG_BASE + (0x47 << 2));
	r->v = v;
}

static inline uint32_t audio_fifo_ll_get_mic1_data_bus_value(void) {
	audio_fifo_mic1_data_bus_t *r = (audio_fifo_mic1_data_bus_t*)(SOC_AUDIO_FIFO_REG_BASE + (0x47 << 2));
	return r->v;
}

static inline uint32_t audio_fifo_ll_get_mic1_data_bus_mic1_data_bus(void) {
	audio_fifo_mic1_data_bus_t *r = (audio_fifo_mic1_data_bus_t*)(SOC_AUDIO_FIFO_REG_BASE + (0x47 << 2));
	return r->mic1_data_bus;
}
#ifdef __cplusplus
}
#endif
