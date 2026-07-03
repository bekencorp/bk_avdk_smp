// Copyright 2020-2025 Beken
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

#include "common/bk_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define US_PER_SYMBLE  16

/**
 * @brief The radio state types.
 */
typedef enum {
    BK_802154_STATE_DISABLE = 0,
    BK_802154_STATE_IDLE,
    BK_802154_STATE_SLEEP,
    BK_802154_STATE_RECEIVE,            // Receiver is on but SFD not yet detected
    BK_802154_STATE_RECEIVE_BUSY,       // Receiving an RX packet
    BK_802154_STATE_TRANSMIT,           // Transmitting a data frame
    BK_802154_STATE_TRANSMIT_IMM_ACK,   // Ttransmiting an Immediate ACK
    BK_802154_STATE_TRANSMIT_ENH_ACK,   // Ttransmiting an Enhanced ACK
    BK_802154_STATE_TRANSMIT_CCA,       // Performing CCA detection before TX
    BK_802154_STATE_ED,                 // Energy Detect
    BK_802154_STATE_MAX
} bk_802154_state_t;

/**
 * @brief The possible errors during frame transmission.
 */
typedef enum {
    BK_802154_TX_ERR_NONE = 0,
    BK_802154_TX_ERR_CCA_BUSY,
    BK_802154_TX_ERR_ABORT,
    BK_802154_TX_ERR_NO_ACK,
    BK_802154_TX_ERR_INVALID_ACK,
    BK_802154_TX_ERR_NO_BUFFER,
} bk_802154_tx_err_t;

/**
 * @brief The possible errors during frame reception.
 */
typedef enum {
    BK_802154_RX_ERR_NONE = 0,
    BK_802154_RX_ERR_ABORT,
    BK_802154_RX_ERR_MALFORMED,
    BK_802154_RX_ERR_NO_BUFFER,
} bk_802154_rx_err_t;

/**
 * @brief Rx abort reasons.
 */
typedef enum {
    BK_802154_RX_ABORT_SFD_TIMEOUT     = 0b0001,
    BK_802154_RX_ABORT_CRC_ERR	       = 0b0010,
    BK_802154_RX_ABORT_INVALID_LEN     = 0b0011,
    BK_802154_RX_ABORT_ACK_TIMEOUT     = 0b0100,
    BK_802154_RX_ABORT_ACK_SN_MISMATCH = 0b0101,
    BK_802154_RX_ABORT_ED_STOP_CMD     = 0b0110,
    BK_802154_RX_ABORT_RX_STOP_CMD     = 0b0111,
    BK_802154_RX_ABORT_HW_FILTER_FAIL  = 0b1000,
    BK_802154_RX_ABORT_PHY_NO_ED       = 0b1001,
} bk_802154_rx_abort_reason_t;

/**
 * @brief The data structure of tx/rx/ack frame.
 */
typedef struct {
    uint8_t frame[128];
    bool used;  //True:used and not available, False:available
    bool pending;
    bool enh_ack;
    int8_t rssi;
    uint8_t lqi;
    uint64_t time;
} bk_802154_frame_t;

/**
 * @brief The msg type
 */
typedef enum {
    BK_802154_MSG_TX_DONE_NO_ACK = 0,
    BK_802154_MSG_TX_DONE_WITH_ACK = 1,
    BK_802154_MSG_RX_DONE = 2,
    BK_802154_MSG_TX_FAIL = 3,
    BK_802154_MSG_RX_FAIL = 4,
    BK_802154_MSG_ED_DONE = 5,
    BK_802154_MSG_FRAME_PROTECT = 6,
    BK_802154_MSG_MAX,
} bk_802154_msg_type_t;

typedef enum {
    BK_802154_FRAME_PROTECT_START = 0,
    BK_802154_FRAME_PROTECT_DONE,
} bk_802154_frame_protect_action_t;

/**
 * @brief The msg queue structure.
 */
typedef struct {
    bk_802154_msg_type_t msg_type;
    union {
        bk_802154_frame_t *frame;
        int8_t ed_rssi;
        struct {
            uint8_t action;
            uint32_t dur_ms;
        } frame_protect;
    } msg;
    union {
        bk_802154_rx_err_t rx_err;
        bk_802154_tx_err_t tx_err;
    } err;
} bk_802154_msg_t;

/**
 * @brief  Initialize the IEEE 802.15.4 subsystem.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 *
 */
bk_err_t bk_ieee802154_enable(void);

/**
 * @brief  Deinitialize the IEEE 802.15.4 subsystem.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 */
bk_err_t bk_ieee802154_disable(void);

/**
 * @brief  Get the operational channel.
 *
 * @return The channel number (11~26).
 *
 */
uint8_t bk_ieee802154_channel_get(void);

/**
 * @brief  Set the operational channel.
 *
 * @param[in]  channel  The channel number (11-26).
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 */
bk_err_t bk_ieee802154_channel_set(uint8_t channel);

/**
 * @brief  Get the transmit power.
 *
 * @return The transmit power in dBm.
 *
 */
int8_t bk_ieee802154_get_txpower(void);

/**
 * @brief  Set the transmit power.
 *
 * @param[in]  power  The transmit power in dBm.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 */
bk_err_t bk_ieee802154_set_txpower(int8_t power);

/**
 * @brief  Get the promiscuous mode.
 *
 * @return
 *      - True   The promiscuous mode is enabled.
 *      - False  The promiscuous mode is disabled.
 *
 */
bool bk_ieee802154_get_promiscuous(void);

/**
 * @brief  Set the promiscuous mode.
 *
 * @param[in]  enable  The promiscuous mode to be set.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 */
bk_err_t bk_ieee802154_set_promiscuous(bool enable);

#if 1
/**
 * @brief  Get the IEEE 802.15.4 Radio state.
 *
 * @return  The IEEE 802.15.4 Radio state, refer to bk_ieee802154_state_t.
 *
 */
bk_802154_state_t bk_ieee802154_state_get(void);

/**
 * @brief  Set the IEEE 802.15.4 Radio to sleep state.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure due to invalid state.
 *
 */
bk_err_t bk_ieee802154_sleep(void);
#endif

/**
 * @brief  Set the IEEE 802.15.4 Radio to receive state.
 *
 * @note Radio will continue receiving until it receives a valid frame.
 *       Refer to `bk_ieee802154_receive_done()`.
 *
 * @return
 *      - BK_OK on success
 *      - BK_FAIL on failure due to invalid state.
 *
 */
bk_err_t bk_ieee802154_receive(void);

/**
 * @brief  Transmit the given frame.
 *         The transmit result will be reported via `bk_ieee802154_transmit_done()`
 *         or `bk_ieee802154_transmit_failed()`.
 *
 * @param[in]  frame  The pointer to the frame, the frame format:
 *                    |-----------------------------------------------------------------------|
 *                    | Len | MHR |              MAC Payload                          |  FCS  |
 *                    |-----------------------------------------------------------------------|
 * @param[in]  cca    Perform CCA before transmission if it's true, otherwise transmit the frame directly.
 *
 * @note During transmission, the hardware calculates the FCS, and send it over the air right after the MAC payload,
 *       so you just need to prepare the length, mac header and mac payload content.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_ERR_INVALID_ARG on an invalid frame.
 *      - BK_FAIL on failure due to invalid state.
 *
 */
bk_err_t bk_ieee802154_transmit(const uint8_t *frame, bool cca);

/**
 * @brief  Set the IEEE 802.15.4 Radio to receive state at a specific time.
 *
 * @note   Radio will start receiving after the timestamp, and continue receiving until it receives a valid frame.
 *         Refer to `bk_ieee802154_receive_done()`.
 *
 * @param[in]  time  A specific timestamp for starting receiving.
 * @return
 *      - BK_OK on success
 *      - BK_FAIL on failure due to invalid state.
 *
 */
bk_err_t bk_ieee802154_receive_at(uint32_t time);

/**
 * @brief  Transmit the given frame at a specific time.
 *         The transmit result will be reported via `bk_ieee802154_transmit_done()`
 *         or `bk_ieee802154_transmit_failed()`.
 *
 * @param[in]  frame  The pointer to the frame. Refer to `bk_ieee802154_transmit()`.
 * @param[in]  cca    Perform CCA before transmission if it's true, otherwise transmit the frame directly.
 * @param[in]  time  A specific timestamp for starting transmission.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_ERR_INVALID_ARG on an invalid frame.
 *      - BK_FAIL on failure due to invalid state.
 *
 */
bk_err_t bk_ieee802154_transmit_at(const uint8_t *frame, bool cca, uint32_t time);

#if 0
/**
 * @brief  Set the time to wait for the ack frame.
 *
 * @param[in]  timeout  The time to wait for the ack frame, in symbol unit (16 us).
 *                      Default: 0x006C, Range: 0x0000 - 0xFFFF.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 */
bk_err_t bk_ieee802154_set_ack_timeout(uint32_t timeout);
#endif

/**
 * @brief  Get the device PAN ID.
 *
 * @return  The device PAN ID.
 *
 */
uint16_t bk_ieee802154_get_panid(void);

/**
 * @brief  Set the device PAN ID.
 *
 * @param[in]  panid  The device PAN ID.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 */
bk_err_t bk_ieee802154_set_panid(uint16_t panid);

/**
 * @brief  Get the device short address.
 *
 * @return  The device short address.
 *
 */
uint16_t bk_ieee802154_get_short_address(void);

/**
 * @brief  Set the device short address.
 *
 * @param[in]  short_address  The device short address.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 */
bk_err_t bk_ieee802154_set_short_address(uint16_t short_address);

/**
 * @brief  Get the device extended address.
 *
 * @param[out]  ext_addr  The pointer to the device extended address.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 */
bk_err_t bk_ieee802154_get_extended_address(uint8_t *ext_addr);

/**
 * @brief  Set the device extended address.
 *
 * @param[in]  ext_addr  The pointer to the device extended address.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 */
bk_err_t bk_ieee802154_set_extended_address(const uint8_t *ext_addr);

#if 0
/**
 * @brief  Get the device PAN ID for specific interface.
 *
 * @param[in]  index  The interface index.
 *
 * @return  The device PAN ID.
 *
 */
uint16_t bk_ieee802154_get_multipan_panid(bk_ieee802154_multipan_index_t index);

/**
 * @brief  Set the device PAN ID for specific interface.
 *
 * @param[in]  index  The interface index.
 * @param[in]  panid  The device PAN ID.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 */
bk_err_t bk_ieee802154_set_multipan_panid(bk_ieee802154_multipan_index_t index, uint16_t panid);

/**
 * @brief  Get the device short address for specific interface.
 *
 * @param[in]  index  The interface index.
 *
 * @return  The device short address.
 *
 */
uint16_t bk_ieee802154_get_multipan_short_address(bk_ieee802154_multipan_index_t index);

/**
 * @brief  Set the device short address for specific interface.
 *
 * @param[in]  index  The interface index.
 * @param[in]  short_address  The device short address.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 */
bk_err_t bk_ieee802154_set_multipan_short_address(bk_ieee802154_multipan_index_t index, uint16_t short_address);

/**
 * @brief  Get the device extended address for specific interface.
 *
 * @param[in]  index  The interface index.
 * @param[out]  ext_addr  The pointer to the device extended address.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 */
bk_err_t bk_ieee802154_get_multipan_extended_address(bk_ieee802154_multipan_index_t index, uint8_t *ext_addr);

/**
 * @brief  Set the device extended address for specific interface.
 *
 * @param[in]  index  The interface index.
 * @param[in]  ext_addr  The pointer to the device extended address.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 */
bk_err_t bk_ieee802154_set_multipan_extended_address(bk_ieee802154_multipan_index_t index, const uint8_t *ext_addr);

/**
 * @brief  Get the device current multipan interface enable mask.
 *
 * @return  Current multipan interface enable mask.
 *
 */
uint8_t bk_ieee802154_get_multipan_enable(void);

/**
 * @brief Enable specific interface for the device.
 *
 * As an example, call `bk_ieee802154_set_multipan_enable(BIT(BK_IEEE802154_MULTIPAN_0) | BIT(BK_IEEE802154_MULTIPAN_1));`
 * to enable multipan interface 0 and 1.
 *
 * @param[in]  mask  The multipan interface bit mask.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 */
bk_err_t bk_ieee802154_set_multipan_enable(uint8_t mask);
#endif


#if 0
/**
 * @brief  Get the device coordinator.
 *
 * @return
 *         - True   The coordinator is enabled.
 *         - False  The coordinator is disabled.
 *
 */
bool bk_ieee802154_get_coordinator(void);

/**
 * @brief  Set the device coordinator role.
 *
 * @param[in]  enable  The coordinator role to be set.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 */
bk_err_t bk_ieee802154_set_coordinator(bool enable);
#endif

/**
 * @brief  Set the auto frame pending mode.
 *
 * @param[in]  pending_mode  The auto frame pending mode, refer to bk_ieee802154_pending_mode_t.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 */
bk_err_t bk_ieee802154_set_pending_mode(bool pending_mode);

/**
 * @brief  Add address to the source matching table.
 *
 * @param[in]  addr      The pointer to the address.
 * @param[in]  is_short  Short address or Extended address.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_ERR_NO_MEM if the pending table is full.
 *
 */
bk_err_t bk_ieee802154_add_pending_addr(const uint8_t *addr, bool is_short);

/**
 * @brief  Remove address from the source matching table.
 *
 * @param[in]  addr      The pointer to the address.
 * @param[in]  is_short  Short address or Extended address.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_ERR_NOT_FOUND if the address was not found from the source matching table.
 *
 */
bk_err_t bk_ieee802154_clear_pending_addr(const uint8_t *addr, bool is_short);

/**
 * @brief  Clear the source matching table to empty.
 *
 * @param[in]  is_short  Clear Short address table or Extended address table.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 */
bk_err_t bk_ieee802154_reset_pending_table(bool is_short);

/**
 * @brief  Get the CCA threshold.
 *
 * @return  The CCA threshold in dBm.
 *
 */
int8_t bk_ieee802154_get_cca_threshold(void);

/**
 * @brief  Set the CCA threshold.
 *
 * @param[in]  cca_threshold  The CCA threshold in dBm.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 */
bk_err_t bk_ieee802154_set_cca_threshold(int8_t cca_threshold);

#if 0
/**
 * @brief  Get the CCA mode.
 *
 * @return  The CCA mode, refer to bk_ieee802154_cca_mode_t.
 *
 */
bk_ieee802154_cca_mode_t bk_ieee802154_get_cca_mode(void);

/**
 * @brief  Set the CCA mode.
 *
 * @param[in]  cca_mode  The CCA mode, refer to bk_ieee802154_cca_mode_t.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 */
bk_err_t bk_ieee802154_set_cca_mode(bk_ieee802154_cca_mode_t cca_mode);
#endif

/**
 * @brief  Enable rx_on_when_idle mode, radio will receive during idle.
 *
 * @param[in]  enable  Enable/Disable rx_on_when_idle mode.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 */
bk_err_t bk_ieee802154_set_rx_when_idle(bool enable);

/**
 * @brief  Get the rx_on_when_idle mode.
 *
 * @return  rx_on_when_idle mode.
 *
 */
bool bk_ieee802154_get_rx_when_idle(void);

/**
 * @brief  Perform energy detection.
 *
 * @param[in]  duration  The duration of energy detection, in symbol unit (16 us).
 *                       The result will be reported via bk_ieee802154_energy_detect_done().
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure due to invalid state.
 *
 */
bk_err_t bk_ieee802154_energy_detect(uint32_t duration);

/**
 * @brief  The energy detection done. Refer to `bk_ieee802154_energy_detect()`.
 *
 * @param[in]  power  The detected power level, in dBm.
 *
 */
void bk_ieee802154_energy_detect_done(int8_t power);

#if 0
 /**
 * @brief  Notify the IEEE 802.15.4 Radio that the frame is handled done by upper layer.
 *
 * @param[in]  frame  The pointer to the frame which was passed from the function `bk_ieee802154_receive_done()`
 *                    or ack frame from `bk_ieee802154_transmit_done()`.
 *
 * @return
 *      - BK_OK on success
 *      - BK_FAIL if frame is invalid.
 *
 */
bk_err_t bk_ieee802154_receive_handle_done(const uint8_t *frame);

/**
 * @brief  The SFD field of the frame was received.
 *
 */
void bk_ieee802154_receive_sfd_done(void);

/**
 * @brief  The SFD field of the frame was transmitted.
 *
 */
void bk_ieee802154_transmit_sfd_done(uint8_t *frame);

/**
 * @brief  The Frame Transmission succeeded.
 *
 * @note   If the ack frame is not null, user must call the function `bk_ieee802154_receive_handle_done()` to notify 802.15.4 driver
 *         after the ack frame is handled.
 *
 * @param[in]  frame           The pointer to the transmitted frame.
 * @param[in]  ack             The received ACK frame, it could be NULL if the transmitted frame's AR bit is not set.
 * @param[in]  ack_frame_info  More information of the ACK frame, refer to bk_ieee802154_frame_info_t.
 *
 */
void bk_ieee802154_transmit_done(const uint8_t *frame, const uint8_t *ack, bk_ieee802154_frame_info_t *ack_frame_info);

/**
 * @brief  The Frame Transmission failed. Refer to `bk_ieee802154_transmit()`.
 *
 * @param[in]  frame  The pointer to the frame.
 * @param[in]  error  The transmission failure reason, refer to bk_ieee802154_tx_error_t.
 *
 */
void bk_ieee802154_transmit_failed(const uint8_t *frame, bk_ieee802154_tx_error_t error);
#endif

/**
 * @brief  Get the RSSI of the most recent received frame.
 *
 * @return The value of RSSI.
 *
 */
int8_t bk_ieee802154_get_recent_rssi(void);

#if 0
/**
 * @brief  Get the LQI of the most recent received frame.
 *
 * @return The value of LQI.
 *
 */
uint8_t bk_ieee802154_get_recent_lqi(void);
#endif

#if 0
/**
 * @brief  Set the key and addr for a frame needs to be encrypted by HW.
 *
 * @param[in]  frame  A frame needs to be encrypted. Refer to `bk_ieee802154_transmit()`.
 * @param[in]  key    A 16-bytes key for encryption.
 * @param[in]  addr   An 8-bytes addr for HW to generate nonce, in general, is the device extended address.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 */
bk_err_t bk_ieee802154_set_transmit_security(uint8_t *frame, uint8_t *key, uint8_t *addr);
#endif

#if 0
/**
 * @brief  This function will be called when a received frame needs to be acked with Enh-Ack, the upper
 *         layer should generate the Enh-Ack frame in this callback function.
 *
 * @param[in]  frame          The received frame.
 * @param[in]  frame_info     The frame information. Refer to `bk_ieee802154_frame_info_t`.
 * @param[out] enhack_frame   The Enh-ack frame need to be generated via this function, HW will send it back after AIFS.
 *
 * @return
 *        - BK_OK if Enh-Ack generates done.
 *        - BK_FAIL if Enh-Ack generates failed.
 *
 */
bk_err_t bk_ieee802154_enh_ack_generator(uint8_t *frame, bk_ieee802154_frame_info_t *frame_info, uint8_t* enhack_frame)
#endif

bk_802154_rx_err_t bk_ieee802154_get_tx_err();
bk_802154_frame_t* bk_ieee802154_get_rx_read_buffer();
bk_802154_frame_t* bk_ieee802154_get_ack_buffer();
uint8_t bk_ieee802154_check_delayed_send();
bool bk_ieee802154_check_ed_scan_start(void);
void bk_ieee802154_check_ed_scan_stop(void);
void bk_ieee802154_thread_ed_scan_info(uint32_t duration,uint8_t channel);

#ifdef __cplusplus
}
#endif


