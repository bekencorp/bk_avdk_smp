// Copyright 2021-2022 Beken
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

#include <common/bk_include.h>
#include "sys_types.h"

const clk_src_t flash_clk_src[] = 
{
	{FLASH_CLK_XTAL, CONFIG_XTAL_FREQ, 0},
	{FLASH_CLK_DPLL, 480000000, 1},
	{FLASH_CLK_APLL, 98000000, 2},
};

const clk_div_t flash_clk_div[] = 
{
	{FLASH_CLK_DIV_4, 4, 0x0},
	{FLASH_CLK_DIV_6, 6, 0x1},
	{FLASH_CLK_DIV_8, 8, 0x2},
	{FLASH_CLK_DIV_10, 10, 0x3},
};