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

// XMC XM25QH256 SPI-NOR descriptor. 256Mbit (32MB), 4KB sector, 256B page.
// JEDEC ID 9Fh: 20h (XMC) 40h (SPI NOR) 19h (256Mbit).
//
// Full 32MB > the 16MB (24-bit) limit of 3-byte addressing, so this part uses
// 4-byte addressing. We follow the industry-preferred, reset-safe scheme:
// dedicated 4-byte-address opcodes (READ4B 0x13, PP4B 0x12, QPP4B 0x34,
// SE4B 0x21, BE32-4B 0x5C, BE64-4B 0xDC) with addr_bytes = 4, and WITHOUT
// switching the chip into stateful Enter-4B mode (enter_4byte = 0). Each
// command self-describes its address width, so a watchdog/boot reset that
// leaves the chip in its default 3-byte mode cannot desync the driver.
//
// Reads are forced single-line (read_force_single): the BK7259 QSPI controller's
// quad read is unreliable with 4-byte addressing (both Quad I/O 0xEC and Quad
// Output 0x6C corrupt multi-chunk sector reads, e.g. FatFs 4096B sectors),
// while the single-line 4-byte read (0x13) is solid. Writes still use the quad
// page program (0x34), verified correct via single-line read-back. QE bit (SR2
// bit1 via WRSR2 0x31) and BP layout match the common GD/XMC family template.

#include <driver/qspi_nor_dev.h>
#include <soc/soc.h>

#if CONFIG_QSPI_NOR_FLASH

const qspi_nor_dev_t qspi_nor_xm25qh256_dev = {
	.name = "XM25QH256",
	.id = {0x20, 0x40, 0x19},
	.id_len = 3,
	.geometry = {
		.total_size = 32U * 1024U * 1024U,
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
		/* 4-byte-address opcode variants (see file header). */
		.page_program = 0x12,      /* PP4B    */
		.quad_page_program = 0x34, /* QPP4B   */
		.read = 0x13,              /* READ4B  */
		.dual_read = 0x3C,         /* DOFR4B  */
		/* Quad Output Fast Read 4B (0x6C): address on a single wire, data on
		 * four wires, no 4-wire mode byte. Chosen over Quad I/O 4B (0xEC)
		 * because the controller's 4-wire address framing is tuned for 3-byte
		 * addressing and mis-times the data phase (8-byte shift) with a 4-byte
		 * address. Single-wire address matches the working 0x13/0x34 path. */
		.quad_read = 0x6C,         /* QOFR4B (addr 1-wire) */
		.erase_sector = 0x21,      /* SE4B (4KB)   */
		.erase_32k = 0x5C,         /* BE32-4B      */
		.erase_64k = 0xDC,         /* BE64-4B      */
		.enter_4byte = 0x00,       /* dedicated 4B opcodes; no Enter-4B mode */
		.addr_bytes = 4,
		.dual_read_dummy_cycle = 8,
		.dual_read_dummy_mode = 4,
		.quad_read_dummy_cycle = 14, /* (unused: read_force_single routes reads to 0x13) */
		.quad_read_dummy_mode = 4,
		.quad_read_addr_1wire = 1,
		/* BK7259 QSPI quad read is unreliable with 4-byte addressing (corrupts
		 * multi-chunk sector reads, e.g. FatFs 4096B sectors), while the
		 * single-line 4-byte read (0x13) is solid. Read single-line; writes
		 * still use the quad page program (0x34), verified correct. */
		.read_force_single = 1,
	},
	.reg = {
		.qe_sr_index = 1,
		.qe_bit = BIT(1),
		.srp0_bit = BIT(7),
		.srp1_bit = BIT(0),
		.sr1_preserve_mask = 0x83,
		.bp_shift = 2,
		.bp_mask = 0x0F,
	},
};

QSPI_NOR_DEVICE_SECTION(qspi_nor_xm25qh256_dev);

#endif
