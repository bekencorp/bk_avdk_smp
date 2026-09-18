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

// Puya P25Q32 SPI-NOR descriptor. 32Mbit (4MB), 4KB sector, 256B page, 3-byte
// addressing. JEDEC ID 9Fh: 85h (Puya) 20h 16h (32Mbit). Command set, QE bit
// (SR2 bit1 via WRSR2 0x31) and BP protection layout match the generic default.

#include <driver/qspi_nor_dev.h>
#include <soc/soc.h>

#if CONFIG_QSPI_NOR_FLASH

const qspi_nor_dev_t qspi_nor_p25q32_dev = {
	.name = "P25Q32",
	.id = {0x85, 0x20, 0x16},
	.id_len = 3,
	.geometry = {
		.total_size = 4U * 1024U * 1024U,
		.sector_size = 4U * 1024U,
		.page_size = 256U,
	},
	.cmd = {
		.read_id = 0x9F,
		.write_enable = 0x06,
		.read_sr1 = 0x05,
		.read_sr2 = 0x35,
		.read_sr3 = 0x15,
		.write_sr1 = 0x01,
		.write_sr2 = 0x31,
		.write_sr3 = 0x11,
		.page_program = 0x02,
		.quad_page_program = 0x32,
		.read = 0x03,
		.dual_read = 0x3B,
		.quad_read = 0xEB,
		.erase_sector = 0x20,
		.erase_32k = 0x52,
		.erase_64k = 0xD8,
		.enter_4byte = 0x00,
		.addr_bytes = 3,
		.dual_read_dummy_cycle = 8,
		.dual_read_dummy_mode = 4,
		.quad_read_dummy_cycle = 4,
		.quad_read_dummy_mode = 5,
	},
	.reg = {
		.qe_sr_index = 1,
		.qe_bit = BIT(1),
		.srp0_bit = BIT(7),
		.srp1_bit = BIT(0),
		.sr1_preserve_mask = 0x83,
		.bp_shift = 2,
		.bp_mask = 0x07,
	},
};

QSPI_NOR_DEVICE_SECTION(qspi_nor_p25q32_dev);

#endif
