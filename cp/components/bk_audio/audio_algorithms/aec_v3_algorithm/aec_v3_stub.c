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

#include <os/mem.h>
#include <modules/aec_v3_1.h>

uint32_t aec_size(uint32_t delay)
{
	return sizeof(AECContext) + delay * sizeof(int16_t);
}

void aec_init(AECContext *aec, int16_t fs)
{
	if (aec == NULL) {
		return;
	}

	os_memset(aec, 0, sizeof(AECContext));
	aec->fs = fs;
	aec->frame_samples = fs / 1000 * 20;
	aec->flags = AEC_EC_FLAG_MSK | AEC_NS_FLAG_MSK | AEC_BPF_FLAG_MSK
		| AEC_DRC_FLAG_MSK | AEC_CNI_FLAG_MSK;
	aec->sin = (int16_t *)aec->tmp1;
	aec->rin = (int16_t *)aec->tmp2;
	aec->out = (int16_t *)aec->tmp3;
}

void aec_ctrl(AECContext *aec, uint32_t cmd, uint32_t arg)
{
	if (aec == NULL) {
		return;
	}

	switch (cmd) {
	case AEC_CTRL_CMD_GET_TX_BUF:
		*(uint32_t *)(uintptr_t)arg = (uint32_t)(uintptr_t)aec->sin;
		break;
	case AEC_CTRL_CMD_GET_RX_BUF:
		*(uint32_t *)(uintptr_t)arg = (uint32_t)(uintptr_t)aec->rin;
		break;
	case AEC_CTRL_CMD_GET_OUT_BUF:
		*(uint32_t *)(uintptr_t)arg = (uint32_t)(uintptr_t)aec->out;
		break;
	case AEC_CTRL_CMD_GET_FRAME_SAMPLE:
		*(uint32_t *)(uintptr_t)arg = aec->frame_samples;
		break;
	case AEC_CTRL_CMD_SET_FLAGS:
		aec->flags = (uint8_t)arg;
		break;
	case AEC_CTRL_CMD_SET_MIC_DELAY:
		aec->mic_delay = (int16_t)arg;
		break;
	case AEC_CTRL_CMD_SET_EC_DEPTH:
		aec->ec_depth = (int8_t)arg;
		break;
	case AEC_CTRL_CMD_SET_REF_SCALE:
		aec->ref_scale = (int8_t)arg;
		break;
	case AEC_CTRL_CMD_SET_VOL:
		aec->vol = (uint8_t)arg;
		break;
	case AEC_CTRL_CMD_SET_MAX_DELAY:
		aec->max_mic_delay = (int16_t)arg;
		break;
	case AEC_CTRL_CMD_SET_DELAY_BUFF:
		aec->rin_delay = (int16_t *)(uintptr_t)arg;
		break;
	default:
		break;
	}
}

void aec_proc(AECContext *aec, int16_t *rin, int16_t *sin, int16_t *out)
{
	(void)rin;

	if (aec == NULL || sin == NULL || out == NULL) {
		return;
	}

	os_memcpy(out, sin, aec->frame_samples * sizeof(int16_t));
}

uint32_t aec_ver(void)
{
	return 0;
}

void gtcrn_proc(void *pgtcrn, int32_t *spec, uint8_t *buff, uint8_t *relay)
{
	(void)pgtcrn;
	(void)spec;
	(void)buff;
	(void)relay;
}

uint32_t gtcrn_size(void)
{
	return 0;
}
