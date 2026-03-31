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

#include "hal_config.h"
#include "audio_fifo_hw.h"
#include "audio_fifo_hal.h"

typedef void (*audio_fifo_dump_fn_t)(void);
typedef struct {
	uint32_t start;
	uint32_t end;
	audio_fifo_dump_fn_t fn;
} audio_fifo_reg_fn_map_t;

static void audio_fifo_dump_spk0_a2dp_port(void)
{
	audio_fifo_spk0_a2dp_port_t *r = (audio_fifo_spk0_a2dp_port_t *)(SOC_AUDIO_FIFO_REG_BASE + (0x40 << 2));

	SOC_LOGI("spk0_a2dp_port: %8x\r\n", REG_READ(SOC_AUDIO_FIFO_REG_BASE + (0x40 << 2)));
	SOC_LOGI("	spk0_a2dp: %8x\r\n", r->spk0_a2dp);
	SOC_LOGI("	reserved_bit_24_31: %8x\r\n", r->reserved_bit_24_31);
}

static void audio_fifo_dump_spk0_call_port(void)
{
	audio_fifo_spk0_call_port_t *r = (audio_fifo_spk0_call_port_t *)(SOC_AUDIO_FIFO_REG_BASE + (0x41 << 2));

	SOC_LOGI("spk0_call_port: %8x\r\n", REG_READ(SOC_AUDIO_FIFO_REG_BASE + (0x41 << 2)));
	SOC_LOGI("	spk0_call: %8x\r\n", r->spk0_call);
	SOC_LOGI("	reserved_bit_24_31: %8x\r\n", r->reserved_bit_24_31);
}

static void audio_fifo_dump_spk0_hint_port(void)
{
	audio_fifo_spk0_hint_port_t *r = (audio_fifo_spk0_hint_port_t *)(SOC_AUDIO_FIFO_REG_BASE + (0x42 << 2));

	SOC_LOGI("spk0_hint_port: %8x\r\n", REG_READ(SOC_AUDIO_FIFO_REG_BASE + (0x42 << 2)));
	SOC_LOGI("	spk0_hint: %8x\r\n", r->spk0_hint);
	SOC_LOGI("	reserved_bit_24_31: %8x\r\n", r->reserved_bit_24_31);
}

static void audio_fifo_dump_spk1_a2dp_port(void)
{
	audio_fifo_spk1_a2dp_port_t *r = (audio_fifo_spk1_a2dp_port_t *)(SOC_AUDIO_FIFO_REG_BASE + (0x43 << 2));

	SOC_LOGI("spk1_a2dp_port: %8x\r\n", REG_READ(SOC_AUDIO_FIFO_REG_BASE + (0x43 << 2)));
	SOC_LOGI("	spk1_a2dp: %8x\r\n", r->spk1_a2dp);
	SOC_LOGI("	reserved_bit_24_31: %8x\r\n", r->reserved_bit_24_31);
}

static void audio_fifo_dump_spk1_call_port(void)
{
	audio_fifo_spk1_call_port_t *r = (audio_fifo_spk1_call_port_t *)(SOC_AUDIO_FIFO_REG_BASE + (0x44 << 2));

	SOC_LOGI("spk1_call_port: %8x\r\n", REG_READ(SOC_AUDIO_FIFO_REG_BASE + (0x44 << 2)));
	SOC_LOGI("	spk1_call: %8x\r\n", r->spk1_call);
	SOC_LOGI("	reserved_bit_24_31: %8x\r\n", r->reserved_bit_24_31);
}

static void audio_fifo_dump_spk1_hint_port(void)
{
	audio_fifo_spk1_hint_port_t *r = (audio_fifo_spk1_hint_port_t *)(SOC_AUDIO_FIFO_REG_BASE + (0x45 << 2));

	SOC_LOGI("spk1_hint_port: %8x\r\n", REG_READ(SOC_AUDIO_FIFO_REG_BASE + (0x45 << 2)));
	SOC_LOGI("	spk1_hint: %8x\r\n", r->spk1_hint);
	SOC_LOGI("	reserved_bit_24_31: %8x\r\n", r->reserved_bit_24_31);
}

static void audio_fifo_dump_mic0_data_bus(void)
{
	audio_fifo_mic0_data_bus_t *r = (audio_fifo_mic0_data_bus_t *)(SOC_AUDIO_FIFO_REG_BASE + (0x46 << 2));

	SOC_LOGI("mic0_data_bus: %8x\r\n", REG_READ(SOC_AUDIO_FIFO_REG_BASE + (0x46 << 2)));
	SOC_LOGI("	mic0_data_bus: %8x\r\n", r->mic0_data_bus);
	SOC_LOGI("	reserved_bit_24_31: %8x\r\n", r->reserved_bit_24_31);
}

static void audio_fifo_dump_mic1_data_bus(void)
{
	audio_fifo_mic1_data_bus_t *r = (audio_fifo_mic1_data_bus_t *)(SOC_AUDIO_FIFO_REG_BASE + (0x47 << 2));

	SOC_LOGI("mic1_data_bus: %8x\r\n", REG_READ(SOC_AUDIO_FIFO_REG_BASE + (0x47 << 2)));
	SOC_LOGI("	mic1_data_bus: %8x\r\n", r->mic1_data_bus);
	SOC_LOGI("	reserved_bit_24_31: %8x\r\n", r->reserved_bit_24_31);
}

static audio_fifo_reg_fn_map_t s_fn[] =
{
	{0x40, 0x40, audio_fifo_dump_spk0_a2dp_port},
	{0x41, 0x41, audio_fifo_dump_spk0_call_port},
	{0x42, 0x42, audio_fifo_dump_spk0_hint_port},
	{0x43, 0x43, audio_fifo_dump_spk1_a2dp_port},
	{0x44, 0x44, audio_fifo_dump_spk1_call_port},
	{0x45, 0x45, audio_fifo_dump_spk1_hint_port},
	{0x46, 0x46, audio_fifo_dump_mic0_data_bus},
	{0x47, 0x47, audio_fifo_dump_mic1_data_bus},
	{-1, -1, 0}
};

void audio_fifo_struct_dump(uint32_t start, uint32_t end)
{
	uint32_t dump_fn_cnt = sizeof(s_fn)/sizeof(s_fn[0]) - 1;

	for (uint32_t idx = 0; idx < dump_fn_cnt; idx++) {
		if ((start <= s_fn[idx].start) && (end >= s_fn[idx].end)) {
			s_fn[idx].fn();
		}
	}
}
