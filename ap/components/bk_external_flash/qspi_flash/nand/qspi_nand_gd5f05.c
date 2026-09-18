// Copyright 2024-2025 Beken
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

#include <driver/qspi_nand_dev.h>
#include <soc/soc.h>

#if CONFIG_QSPI_NAND_FLASH

/* GigaDevice GD5F05 family: 512Mbit (64MB), 2KB page, 64 pages/block, mfg C8h.
 * Command set and geometry match the ZB35 generic profile; only the JEDEC ID,
 * capacity and the A0h write-protect bit differ.
 *
 * ASSUMPTION: wp_enable is set to BIT(7) (BRWD) for the GD5F A0h register, which
 * differs from ZBit's BIT(1). Confirm against the GD5F05 datasheet before
 * production; set_protect_none() writes block_unlock_value (0x00) which clears
 * all protection regardless, so this bit only affects the quad_enable re-check. */
#define GD5F05_COMMON(_name, _did) { \
	.name = (_name), \
	.id = {0xC8, (_did)}, \
	.id_len = 2, \
	.geometry = { \
		.total_size = 64U * 1024U * 1024U, \
		.page_size = 2048U, \
		.spare_size = 64U, \
		.pages_per_block = 64U, \
	}, \
	.cmd = { \
		.reset = 0xFF, \
		.write_enable = 0x06, \
		.get_feature = 0x0F, \
		.set_feature = 0x1F, \
		.block_erase = 0xD8, \
		.program_load = 0x02, \
		.program_load_random = 0x84, \
		.program_execute = 0x10, \
		.page_read = 0x13, \
		.read_cache = 0x03, \
		.read_cache_x2 = 0x3B, \
		.read_cache_x4 = 0x6B, \
		.read_cache_quad = 0xEB, \
		.program_load_x4 = 0x32, \
		.program_load_random_x4 = 0x34, \
		.feature_addr_len = 1, \
		.column_addr_len = 2, \
		.row_addr_len = 3, \
		.feature_data_len = 1, \
		.read_cache_dummy_cycles = 8, \
		.read_cache_dummy_mode = 3, \
	}, \
	.reg = { \
		.block_lock = 0xA0, \
		.configuration = 0xB0, \
		.status = 0xC0, \
		.block_unlock_value = 0x00, \
	}, \
	.bit = { \
		.oip = BIT(0), \
		.wel = BIT(1), \
		.erase_fail = BIT(2), \
		.program_fail = BIT(3), \
		.ecc_enable = BIT(4), \
		.quad_enable = BIT(0), \
		.wp_enable = BIT(7), \
		.ecc_status_mask = BIT(4) | BIT(5), \
		.ecc_status_shift = 4, \
		.ecc_no_error = 0x0, \
		.ecc_corrected = 0x1, \
		.ecc_uncorrectable = 0x2, \
	}, \
}

const qspi_nand_dev_t qspi_nand_gd5f05m7ue_dev = GD5F05_COMMON("GD5F05M7UE", 0x90);
const qspi_nand_dev_t qspi_nand_gd5f05m7re_dev = GD5F05_COMMON("GD5F05M7RE", 0x80);

QSPI_NAND_DEVICE_SECTION(qspi_nand_gd5f05m7ue_dev);
QSPI_NAND_DEVICE_SECTION(qspi_nand_gd5f05m7re_dev);

#endif
