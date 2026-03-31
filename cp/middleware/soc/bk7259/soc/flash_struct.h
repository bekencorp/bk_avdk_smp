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

#ifdef __cplusplus
extern "C" {
#endif

#define SOC_SEC_ADDR_NUM 4

typedef volatile struct {
	/* REG_0x00 */
	uint32_t dev_id;

	/* REG_0x01 */
	uint32_t dev_version;

	/* REG_0x02 */
	union {
		struct {
			uint32_t soft_reset:      1; /**< bit[0] soft reset */
			uint32_t clk_gate_bypass: 1; /**< bit[1] bypass uart clock gate */
			uint32_t reserved:       30; /**< bit[2:31] reserved */
		};
		uint32_t v;
	} global_ctrl;

	/* REG_0x03 */
	uint32_t global_status;

	/* REG_0x04 */
	union {
		struct {
			uint32_t reserved:   29;  /**< bit[0:28] */
			uint32_t op_sw:       1;  /**< bit[29] Software operation startup trigger */
			uint32_t wp_value:    1;  /**< bit[30] wp output value */
			uint32_t busy_sw:     1;  /**< bit[31] flash status busy bit for software */
		};
		uint32_t v;
	} op_ctrl;

	/* REG_0x5 */
	uint32_t data_sw_flash; /**< data to be written from software to flash */

	/* REG_0x6 */
	uint32_t data_flash_sw; /**< data read from flash to software */

	/* REG_0x7 */
	union {
		struct {
			uint32_t wrsr_cmd_reg: 8; /**< bit[0:7] WRSR command constent */
			uint32_t rdsr_cmd_reg: 8; /**< bit[8:15] RDSR command constent */
			uint32_t wrsr_cmd_sel: 1; /**< bit[16] 0: use 05 cmd, 1: use wrsr_cmd_reg */
			uint32_t rdsr_cmd_sel: 1; /**< bit[17] 0: use 05 cmd, 1: use rdsr_cmd_reg */
			uint32_t reserved:    14; /**< bit[18:31] */
		};
		uint32_t v;
	} cmd_cfg;

	/* REG_0x8 */
	uint32_t rd_flash_id; /**< read flash id data */

	/* REG_0x9 */
	union {
		struct {
			uint32_t status_reg:  8; /**< bit[0:7] state register data from flash */
			uint32_t crc_err_num: 8; /**< bit[8:15] CRC error number */
			uint32_t byte_sel_rd: 3; /**< bit[16:18] No. byte software write to flash */
			uint32_t byte_sel_wr: 3; /**< bit[19:21] No. byte software read from flash */
			uint32_t m_value:     8; /**< bit[22:29] Quad M value */
			uint32_t pw_write:    1; /**< bit[30] page write enable */
			uint32_t otp_sel:     1; /**< bit[31] 0 - use GD type  1: use MAX type */
		};
		uint32_t v;
	} state;

	/* REG_0xA */
	union {
		struct {
			uint32_t clk_cfg:        4; /**< bit[0:3] flash clock */
			uint32_t mode_sel:       5; /**< bit[4:8] */
			uint32_t cpu_data_wr_en: 1; /**< bit[9] CPU data writing enable */
			uint32_t wrsr_data:     16; /**< bit[10:25] flash status register data to be written */
			uint32_t crc_en:         1; /**< bit[26] flash data to CPU CRC enable */
			uint32_t allff_reg:      1; /**< bit[27] read all FF crc error option, 0:error, 1:right */
			uint32_t fclk_gate_xaes: 1; /**< bit[28] xaes clkgate shutdown option, 1: not gating */
			uint32_t reserved:       3; /**< bit[29:31] */
		};
		uint32_t v;
	} config;

	/* REG_0xB */
	union {
		struct {
			uint32_t tres1_trdp_delay_cnt: 12; /**< bit[0:11] Tres1-Trdp delay time */
			uint32_t tdp_trdpdd_delay_cnt: 12; /**< bit[12:23] Tdp-Tdpdd delay time */
			uint32_t dpd_fbd:               1; /**< bit[24] deep power-down forbid */
			uint32_t prefetch_version:      1; /**< bit[25] prefetch version */
			uint32_t reserved:              5; /**< bit[26:30] prefetch version */
			uint32_t dpd_status:            1; /**< bit[31] deep power-down status */
		};
		uint32_t v;
	} ps_ctrl;

	/* REG_0xC */
	union {
		struct {
			uint32_t mem_data:      8; /**< bit[0:7] page write memory data */
			uint32_t reserved:     23; /**< bit[8:30] */
			uint32_t mem_addr_clr:  1; /**< bit[31] page write memory address clear to 0 */
		};
		uint32_t v;
	} page_ctrl;

	/* REG_0xd ~ REG_0x14 */
	struct {
		union {
			struct {
				uint32_t flash_sec_start_addr: 24; /**< bit[0:23] flash sec addr start */
				uint32_t flash_sec_addr_en:     1; /**< bit[24] flash sec addr enable */
				uint32_t reserved:              7; /**< bit[25:31] */
			};
			uint32_t v;
		} sec_addr_start;

		union {
			struct {
				uint32_t flash_sec_end_addr: 24; /**< bit[0:23] flash sec addr end */
				uint32_t reserved:            8; /**< bit[24:31] */
			};
			uint32_t v;
		} sec_addr_end;
	} sec_addr[SOC_SEC_ADDR_NUM];

	/* REG_0x15 */
	union {
		struct {
			uint32_t addr_sw_reg: 24; /**< bit[0:23] flash operation address */
			uint32_t op_type_sw:   5; /**< bit[24:28] flash operation command type */
			uint32_t reserved:     3; /**< bit[29:31] */
		};
		uint32_t v;
	} op_cmd;

	/* REG_0x16 */
	uint32_t offset_addr_begin; /**< start addr of imageA */

	/* REG_0x17 */
	uint32_t offset_addr_end; /**< end addr of imageA */

	/* REG_0x18 */
	uint32_t flash_addr_offset; /**< imageB offset */

	/* REG_0x19 */
	union {
		struct {
			uint32_t flash_offset_enable:  1; /**< bit[0] 0:ex from imageA 1:ex from imageB */
			uint32_t rdsta_omit:           1; /**< bit[1] 0:as usaul 1:no read status when wr/erase */
			uint32_t susres_cmd_sel:       1; /**< bit[2] 0: not select 1:select*/
			uint32_t susres_cmd_reg:       8; /**< bit[3:10] special command*/
			uint32_t flash_gate_disable:   1; /**< bit[11] 1:prefetch hclk_en gate disable 0:always on*/
			uint32_t reserved:             20; /**< bit[12:31] */
		};
		uint32_t v;
	} flash_ctrl;

	/* REG_0x1A */
	union {
		struct {
			uint32_t csn_iodrv                :  3; /**<bit[0 : 2] None */
			uint32_t sck_iodrv                :  3; /**<bit[3 : 5] None */
			uint32_t dat_iodrv                :  3; /**<bit[6 : 8] None */
			uint32_t dqs_iodrv                :  3; /**<bit[9 : 11] None */
			uint32_t dqs_dly                  :  3; /**<bit[12 : 14] None */
			uint32_t quad_dqs_dm              :  5; /**<bit[15 : 19] None */
			uint32_t quad_dqs_sel             :  1; /**<bit[20 : 20] None */
			uint32_t reserved_bit_21_23       :  3; /**<bit[21 : 23] reserved */
			uint32_t address_sw_hi            :  8; /**<bit[24 : 31] flash operation address[31:24] */
		};
		uint32_t v;
	} REG_0x1A;

	/* REG_0x1B */
	union {
		struct {
			uint32_t mem_a_rdid               :  8; /**<bit[0 : 7] rdid/fdp/uid memory access address */
			uint32_t mem_cen_rdid             :  1; /**<bit[8 : 8] rdid/fdp/uid chip enable low active */
			uint32_t mem_rdid                 :  1; /**<bit[9 : 9] 1:memory for rdid/fdp/uid 0:memory for page write */
			uint32_t reserved_bit_10_31       : 22; /**<bit[10 : 31] reserved */
		};
		uint32_t v;
	} REG_0x1B;

	/* REG_0x1C */
	union {
		struct {
			uint32_t rdid_cmd                 :  8; /**<bit[0 : 7] rdid/fdp/uid command defined */
			uint32_t se_cmd                   :  8; /**<bit[8 : 15] sector erase  command defined */
			uint32_t ce_cmd                   :  8; /**<bit[16 : 23] chip erase  command defined */
			uint32_t wren_cmd                 :  8; /**<bit[24 : 31] write enable  command defined */
		};
		uint32_t v;
	} REG_0x1C;

	/* REG_0x1D */
	union {
		struct {
			uint32_t read_cmd                 :  8; /**<bit[0 : 7] read memory  command defined */
			uint32_t pp_cmd                   :  8; /**<bit[8 : 15] page program  command defined */
			uint32_t address_4th_byte         :  8; /**<bit[16 : 23] NoUse */
			uint32_t rdid_256b                :  1; /**<bit[24 : 24] read fdp/uid read select 1:read 256bytes 0:read 4 bytes */
			uint32_t se_4b                    :  1; /**<bit[25 : 25] sector erase 4th byte select */
			uint32_t read_4b                  :  1; /**<bit[26 : 26] read memory 4th byte select */
			uint32_t pp_4b                    :  1; /**<bit[27 : 27] page program  4th byte select */
			uint32_t pp_no_wren               :  1; /**<bit[28 : 28] page program without write enable  */
			uint32_t pp_no_rdsr               :  1; /**<bit[29 : 29] page program without read state register  */
			uint32_t ce_no_wren               :  1; /**<bit[30 : 30] chip erase without write enable  */
			uint32_t ce_no_rdsr               :  1; /**<bit[31 : 31] chip erase without read state register  */
		};
		uint32_t v;
	} REG_0x1D;

	/* REG_0x1E */
	union {
		struct {
			uint32_t pp_cnt                   :  6; /**<bit[0 : 5] page program count used for write configuration register with count<32 */
			uint32_t se_no_wren               :  1; /**<bit[6 : 6] sector erase without write enable  */
			uint32_t se_no_rdsr               :  1; /**<bit[7 : 7] sector erase without read state register  */
			uint32_t rd8io_cmd                : 16; /**<bit[8 : 23] OPI DTR command */
			uint32_t read_8io_sel             :  1; /**<bit[24 : 24] set this bit before change configuration register by software and controller will change to OPI DTR after write configuration register finished */
			uint32_t rst8io_sel               :  1; /**<bit[25 : 25] used to set OPI reset(6699 then 9966) when software start with op_type=read, before start operation clear read_8io_sel first */
			uint32_t rddat_8io_sel            :  1; /**<bit[26 : 26] select MX type flash data arrangement(MX D1 first but GD D0 first output) */
			uint32_t phy_rdat_sel             :  1; /**<bit[27 : 27] set when start read id operation with rdid_256b=0(read 4 bytes id) */
			uint32_t qfr_cmd_lo               :  4; /**<bit[28 : 31] quad fast read command low 4 bit */
		};
		uint32_t v;
	} REG_0x1E;

	/* REG_0x1F */
	union {
		struct {
			uint32_t qfr_cmd_hi               :  4; /**<bit[0 : 3] quad fast read command high 4 bit */
			uint32_t dlrd_cmd                 :  8; /**<bit[4 : 11] dual read command */
			uint32_t quad_quit_sel            :  1; /**<bit[12 : 12] None */
			uint32_t rd8io_cmd_sw             : 16; /**<bit[13 : 28] OPI DTR read command read by software */
			uint32_t read_4io_sel             :  1; /**<bit[29 : 29] QUAD DTR read select */
			uint32_t rddat_4_8io_sw_sel       :  1; /**<bit[30 : 30] None */
			uint32_t cont_mode                :  1; /**<bit[31 : 31] 0:dfr, qfr, qwfr not enter continuous mode(m_value better set 0x00) 1:dfr, qfr, qwfr will enter continuous mode(m_value better set 0xa0 or others) */
		};
		uint32_t v;
	} REG_0x1F;

} flash_hw_t;

#ifdef __cplusplus
}
#endif

