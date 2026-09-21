// Copyright 2020-2024 Beken
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

/* AON_PMU r7c chip ID (BK7259) */
#define BK7259_CHIP_ID_V2_MPW        (0x25910020U) /* BK7259V2 / MPW */
#define BK7259_CHIP_ID_V3A           (0x26800820U) /* BK72593A */
#define BK7259_CHIP_ID_V3B           (0x26800920U) /* BK72593B */
#define BK7259_CHIP_ID_SERIES_MASK   (0xFFFF0000U)

typedef enum {
	BK7259_CHIP_MODEL_V2_MPW = 0,
	BK7259_CHIP_MODEL_V3A,
	BK7259_CHIP_MODEL_V3B,
	BK7259_CHIP_MODEL_UNKNOWN,
} bk7259_chip_model_e;
