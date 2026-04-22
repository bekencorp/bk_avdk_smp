
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

#pragma once

#include <common/bk_err.h>

#ifdef __cplusplus
extern "C" {
#endif

void hal_dsi_sys_clk_switch(uint8_t enable);

void hal_dsi_wait_fpga_dphy_done(void);

void hal_dsi_wait_for_dphy_pwrup(void);

void hal_dsi_dphy_power_down(void);


void hal_dsi_host_reset(void);

void hal_dsi_host_power_up(void);

void hal_dsi_operation_mode_set(uint32_t mode);

void hal_dsi_config(uint8_t n_lanes, 
                    uint16_t width, 
                    uint16_t height, 
                    uint16_t hsa_time, 
                    uint16_t hbp_time, 
                    uint16_t hline_time, 
                    uint16_t vsa_line, 
                    uint16_t vbp_line, 
                    uint16_t vfp_line
                );


/**
 * Program Naneng D-PHY PLL (reg_NN_PHY_R5c) and analog timing from panel pixel clock.
 * Bandwidth: min lane rate so that n_lanes * lane_bps >= pclk_hz * bpp * (1 + overhead).
 * PLL: FVCO = 26MHz * 8 * (NI + NF/1024), lane_hs = FVCO / 2^rate, DPI pclk = lane_hs / (pixdiv+2).
 * @param out_lane_mbps HS bit rate per data lane in Mbps (for DSI host byte-cycle timing)
 */
bk_err_t hal_dsi_dphy_init_for_panel(uint64_t pclk_hz, uint8_t n_lanes, uint16_t bpp,
                                     uint32_t overhead_permille, uint32_t *out_lane_mbps);


/**
 * Send READ packet to peripheral using the generic interface
 * This will force command mode and stop video mode (because of BTA)
 * @param vc destination virtual channel
 * @param data_type generic command type
 * @param lsb_byte first command parameter, (if DCS, it is the DCS command)
 * @param msb_byte second command parameter, (only parameter of short DCS packet)
 * @param bytes_to_read no of bytes to read (expected to arrive at buffer)
 * @param read_buffer pointer to 8-bit array to hold the read buffer words
 * return status
 * @note this function will enable BTA
 */
uint16_t hal_dsi_gen_read_pkt(  uint8_t vc, uint8_t data_type, 
                            uint8_t msb_byte, uint8_t lsb_byte, 
                            uint8_t bytes_to_read, uint8_t *read_buffer);

/**
 * Send a packet on the generic interface
 * @param vc destination virtual channel
 * @param data_type type of command, inserted in first byte of header
 * @param lsb_byte first command parameter, (if DCS, it is the DCS command)
 * @param msb_byte second command parameter, (only parameter of short DCS packet)
 * @param params byte array of command parameters
 * @param param_length length of the above array
 * @return error code
 * @note the controller restricts the sending of .
 * This function will not be able to send Null and Blanking packets due to
 *  controller restriction
 */
uint16_t hal_dsi_gen_write_pkt( uint8_t vc, uint8_t data_type, 
                            uint8_t msb_byte, uint8_t lsb_byte, 
                            uint16_t param_length, const uint8_t *params);

void hal_dsi_factory_test(void);


#ifdef __cplusplus
}
#endif



