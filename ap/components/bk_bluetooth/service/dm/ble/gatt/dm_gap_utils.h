#pragma once

/**
 * @file dm_gap_utils.h
 *
 * @brief BLE GAP helper validation APIs.
 */

/**
 * @brief Check whether a BLE address is valid.
 *
 * @param addr BLE address buffer. The buffer length must be 6 bytes.
 *
 * @return Non-zero if the address is valid, otherwise zero.
 */
uint8_t dm_gap_is_addr_valid(uint8_t *addr);

/**
 * @brief Check whether a data buffer is valid.
 *
 * @param data Data buffer.
 * @param len Data length in bytes.
 *
 * @return Non-zero if the buffer is valid, otherwise zero.
 */
uint8_t dm_gap_is_data_valid(uint8_t *data, uint32_t len);
