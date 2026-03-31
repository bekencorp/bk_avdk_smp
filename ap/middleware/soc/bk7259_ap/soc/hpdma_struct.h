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

typedef volatile struct {
	/* REG_0x0 */
	uint32_t device_id;

	/* REG_0x1 */
	uint32_t version_id;

	/* REG_0x2 */
	union {
		struct {
			uint32_t soft_reset: 1;
			uint32_t bps_clk_gate: 1;
			uint32_t prio_mode: 1;
			uint32_t reserved: 29;
		};
		uint32_t v;
	} prio_mode;

	uint32_t reg_gap_0;

	/* REG_0x4 */
	union {
		struct {
			uint32_t attr: 4;
			uint32_t reserved: 28;
		};
		uint32_t v;
	} secure_attr;

	/* REG_0x5 */
	union {
		struct {
			uint32_t attr: 4;
			uint32_t reserved: 28;
		};
		uint32_t v;
	} privileged_attr;

	/* REG_0x6 */
	union {
		struct {
			uint32_t status: 4;
			uint32_t reserved: 28;
		};
		uint32_t v;
	} int_status0;

	/* REG_0x7 */
	union {
		struct {
			uint32_t status: 4;
			uint32_t reserved: 28;
		};
		uint32_t v;
	} int_status1;

	/* REG_0x8 */
	union {
		struct {
			uint32_t status: 4;
			uint32_t reserved: 28;
		};
		uint32_t v;
	} int_status2;
	/* REG_0x9 */
	union {
		struct {
			uint32_t status: 4;
			uint32_t reserved: 28;
		};
		uint32_t v;
	} int_status3;
	/* REG_0x0A */
	union {
		struct {
			uint32_t status: 4;
			uint32_t reserved: 28;
		};
		uint32_t v;
	} int_status4;
	/* REG_0X0B */
	union {
		struct {
			uint32_t status: 12;
			uint32_t reserved: 20;
		};
		uint32_t v;
	} int_allocate;

	uint32_t reg_gap_1[4];

	/* REG_CHANN(x) */
	struct {
		/* REG_0x10 */
		union {
			struct {
				uint32_t enable: 1;
				uint32_t mode: 1;
				uint32_t src_data_width: 3;
				uint32_t dest_data_width: 3;
				uint32_t src_addr_inc_en: 1;
				uint32_t dest_addr_inc_en: 1;
				uint32_t src_addr_loop_en: 1;
				uint32_t dest_addr_loop_en: 1;
				uint32_t chan_prio: 3;
				uint32_t fast_mode: 1;
				uint32_t cfg_cache: 4;
				uint32_t reserved: 12;
			};
			uint32_t v;
		} ctrl;

		/* REG_0x11 */
		uint32_t dest_start_addr;

		/* REG_0x12 */
		uint32_t src_start_addr;

		/* REG_0x13 */
		uint32_t dest_loop_end_addr;

		/* REG_0x14 */
		union {
			struct {
				uint32_t src_xsize: 16;      /**< Source X-direction length (bits [15:0]) */
				uint32_t dest_xsize: 16;     /**< Destination X-direction length (bits [31:16]) */
			} xsize;
			uint32_t v;
		} xsize_reg;

		/* REG_0x15 */
		uint32_t src_loop_end_addr;

		/* REG_0x16 */
		union {
			struct {
				uint32_t src_ysize: 16;      /**< Source Y-direction length (bits [15:0]) */
				uint32_t dest_ysize: 16;     /**< Destination Y-direction length (bits [31:16]) */
			} ysize;
			uint32_t v;
		} ysize_reg;

		/* REG_0x17 */
		union {
			struct {
				uint32_t src_req_mux: 6;
				uint32_t dest_req_mux: 6;
				uint32_t src_read_interval: 4;
				uint32_t dest_write_interval: 4;
				uint32_t src_sec_attr: 1;
				uint32_t dest_sec_attr: 1;
				uint32_t bus_err_int_en: 1;
				uint32_t fifo_err_int_en: 1;
				uint32_t src_burst_len: 2;
				uint32_t dtst_burst_len: 2;
                uint32_t pixel_trans_type: 2;
				uint32_t finish_int_en: 1;
				uint32_t half_finish_int_en: 1;
			};
			uint32_t v;
		} req_mux;

		/* REG_0x18 */
		uint32_t src_pause_addr;

		/* REG_0x19 */
		uint32_t dest_pause_addr;

		/* REG_0x1A */
		uint32_t src_rd_addr;

		/* REG_0x1B */
		uint32_t dest_wr_addr;

		/* REG_0x1C */
		union {
			struct {
				uint32_t desc_num: 16;
                uint32_t reserved0: 1;
				uint32_t fifo_err_int: 1;
                uint32_t half_finish_int: 1;
				uint32_t finish_int: 1;
				uint32_t bus_err_int: 1;
                uint32_t reserved1: 1;
				uint32_t repeat_wr_pause: 1;
				uint32_t repeat_rd_pause: 1;
				uint32_t finish_int_counter: 4;
				uint32_t half_finish_int_counter: 4;
			};
			uint32_t v;
		} status;

		/* REG_0x1D */
		union {
			struct {
				uint32_t src_step: 15;       /**< Source address step between adjacent 1D data (bits [14:0]) */
				uint32_t dest_step: 15;     /**< Destination address step between adjacent 1D data (bits [29:15]) */
				uint32_t reserved1: 2;     
			} step;
			uint32_t v;
		} step_reg;

		/* REG_0x1E */
		union {
			struct {
				uint32_t remain_len: 30;
				uint32_t reserved: 2;
			};
			uint32_t v;
		} remain_length;

		/* REG_0x1F */
        uint32_t next_ll_addr;
	} config_group[SOC_HPDMA_CHAN_NUM_PER_UNIT];
} hpdma_hw_t;

#ifdef __cplusplus
}
#endif

