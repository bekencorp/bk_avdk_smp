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

#ifdef __cplusplus
extern "C" {
#endif


typedef volatile union {
	struct {
		uint32_t spk0_a2dp                : 24; /**<bit[0 : 23] */
		uint32_t reserved_bit_24_31       :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} audio_fifo_spk0_a2dp_port_t;


typedef volatile union {
	struct {
		uint32_t spk0_call                : 24; /**<bit[0 : 23] */
		uint32_t reserved_bit_24_31       :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} audio_fifo_spk0_call_port_t;


typedef volatile union {
	struct {
		uint32_t spk0_hint                : 24; /**<bit[0 : 23] */
		uint32_t reserved_bit_24_31       :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} audio_fifo_spk0_hint_port_t;


typedef volatile union {
	struct {
		uint32_t spk1_a2dp                : 24; /**<bit[0 : 23] */
		uint32_t reserved_bit_24_31       :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} audio_fifo_spk1_a2dp_port_t;


typedef volatile union {
	struct {
		uint32_t spk1_call                : 24; /**<bit[0 : 23] */
		uint32_t reserved_bit_24_31       :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} audio_fifo_spk1_call_port_t;


typedef volatile union {
	struct {
		uint32_t spk1_hint                : 24; /**<bit[0 : 23] */
		uint32_t reserved_bit_24_31       :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} audio_fifo_spk1_hint_port_t;


typedef volatile union {
	struct {
		uint32_t mic0_data_bus            : 24; /**<bit[0 : 23] */
		uint32_t reserved_bit_24_31       :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} audio_fifo_mic0_data_bus_t;


typedef volatile union {
	struct {
		uint32_t mic1_data_bus            : 24; /**<bit[0 : 23] */
		uint32_t reserved_bit_24_31       :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} audio_fifo_mic1_data_bus_t;

typedef volatile struct {
	volatile audio_fifo_spk0_a2dp_port_t spk0_a2dp_port;
	volatile audio_fifo_spk0_call_port_t spk0_call_port;
	volatile audio_fifo_spk0_hint_port_t spk0_hint_port;
	volatile audio_fifo_spk1_a2dp_port_t spk1_a2dp_port;
	volatile audio_fifo_spk1_call_port_t spk1_call_port;
	volatile audio_fifo_spk1_hint_port_t spk1_hint_port;
	volatile audio_fifo_mic0_data_bus_t mic0_data_bus;
	volatile audio_fifo_mic1_data_bus_t mic1_data_bus;
} audio_fifo_hw_t;

#ifdef __cplusplus
}
#endif
