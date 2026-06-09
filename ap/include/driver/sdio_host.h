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

#include <common/bk_include.h>
#include <driver/sdio_host_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief     Init the sdio host controller
 *
 * Bring up the specified sdio host controller (power up, allocate the shared
 * driver resources, configure the identification clock and bus width). After
 * this call the host is ready to send commands / transfer data.
 *
 * @param id  the sdio host controller id
 * @param cfg the host configuration (card type, init clock, bus width)
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_PARAM: id out of range
 *    - others: other errors.
 */
bk_err_t bk_sdio_host_init(sdio_host_id_t id, const sdio_host_cfg_t *cfg);

/**
 * @brief     Deinit the sdio host controller
 *
 * Reset the specified host controller and release the shared driver resources.
 *
 * @param id the sdio host controller id
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_PARAM: id out of range
 *    - others: other errors.
 */
bk_err_t bk_sdio_host_deinit(sdio_host_id_t id);

/**
 * @brief     Reset the sdio host controller
 *
 * @param id the sdio host controller id
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_PARAM: id out of range
 */
bk_err_t bk_sdio_host_reset(sdio_host_id_t id);

/**
 * @brief     Set the sdio host transfer clock frequency
 *
 * @param id      the sdio host controller id
 * @param freq_hz target clock frequency in Hz (0 falls back to 400KHz)
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_PARAM: id out of range
 */
bk_err_t bk_sdio_host_set_clock(sdio_host_id_t id, uint32_t freq_hz);

/**
 * @brief     Set the sdio host data bus width
 *
 * @param id    the sdio host controller id
 * @param width the bus width (1/4/8 line)
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_PARAM: id out of range
 */
bk_err_t bk_sdio_host_set_bus_width(sdio_host_id_t id, sdio_host_bus_width2_t width);

/**
 * @brief     Set the sdio host timing/speed mode
 *
 * @param id     the sdio host controller id
 * @param timing the timing mode (SDR/DDR/HS/HS200/HS400 ...)
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_PARAM: id out of range
 */
bk_err_t bk_sdio_host_set_timing(sdio_host_id_t id, sdio_host_timing_t timing);

/**
 * @brief     Set the sdio host signaling voltage
 *
 * @param id   the sdio host controller id
 * @param volt the signaling voltage (3.3V / 1.8V)
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_PARAM: id out of range
 */
bk_err_t bk_sdio_host_set_signal_voltage(sdio_host_id_t id, sdio_host_signal_voltage_t volt);

/**
 * @brief     Check whether a card is present on the host controller
 *
 * @param id the sdio host controller id
 *
 * @return
 *    - true: card inserted
 *    - false: no card or id out of range
 */
bool bk_sdio_host_card_present(sdio_host_id_t id);

/**
 * @brief     Get the capabilities of the host controller
 *
 * @param id   the sdio host controller id
 * @param caps output, the host capabilities
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_PARAM: id out of range or caps is NULL
 */
bk_err_t bk_sdio_host_get_caps(sdio_host_id_t id, sdio_host_caps_t *caps);

/**
 * @brief     Send a command to the card and (optionally) collect the response
 *
 * @param id   the sdio host controller id
 * @param cmd  the command to send
 * @param resp output, the command response (may be NULL)
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_PARAM: id out of range or cmd is NULL
 *    - BK_ERR_SDIO_HOST_CMD_RSP_TIMEOUT: wait command response timeout
 *    - others: other errors.
 */
bk_err_t bk_sdio_host_send_cmd(sdio_host_id_t id, const sdio_host_cmd_t *cmd,
			       sdio_host_resp_t *resp);

/**
 * @brief     Run a data transfer (command + data phase)
 *
 * @param id   the sdio host controller id
 * @param cmd  the command associated with the transfer
 * @param data the data descriptor (direction, mode, buffer, block info)
 * @param resp output, the command response (may be NULL)
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_PARAM: id out of range, cmd/data/buffer is NULL
 *    - others: other errors.
 */
bk_err_t bk_sdio_host_xfer(sdio_host_id_t id, const sdio_host_cmd_t *cmd,
			   sdio_host_data_t *data, sdio_host_resp_t *resp);

/**
 * @brief     Register the async SDIO card-interrupt callback
 *
 * @param id  the sdio host controller id
 * @param cb  the callback invoked on a card interrupt
 * @param arg user argument passed back to the callback
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_PARAM: id out of range
 */
bk_err_t bk_sdio_host_register_sdio_irq(sdio_host_id_t id, sdio_host_irq_cb_t cb, void *arg);

/**
 * @brief     Enable/disable the SDIO card interrupt
 *
 * @param id     the sdio host controller id
 * @param enable true to enable, false to disable
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_PARAM: id out of range
 */
bk_err_t bk_sdio_host_enable_sdio_irq(sdio_host_id_t id, bool enable);

#ifdef __cplusplus
}
#endif

