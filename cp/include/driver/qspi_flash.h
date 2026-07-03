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

/**
 * @brief     Init the QSPI flash
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_QSPI_NOT_INIT: QSPI driver not init
 *    - others: other errors.
 */
bk_err_t bk_qspi_flash_init(qspi_id_t id);

/**
 * @brief     Deinit the QSPI flash
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_QSPI_NOT_INIT: QSPI driver not init
 *    - others: other errors.
 */
bk_err_t bk_qspi_flash_deinit(qspi_id_t id);

/**
 * @brief      QSPI flash read id
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
uint32_t bk_qspi_flash_read_id(qspi_id_t id);

/**
 * @brief      QSPI flash set protect type as none to do erase / write
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_qspi_flash_set_protect_none(qspi_id_t id);

/**
 * @brief      QSPI flash erase sector
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_qspi_flash_erase_sector(qspi_id_t id, uint32_t addr);

/**
 * @brief      QSPI flash erase
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_qspi_flash_erase(qspi_id_t id, uint32_t addr, uint32_t size);

/**
 * @brief      QSPI flash enable quad mode
 *
 * @return
 *    - NA.
 */
bk_err_t bk_qspi_flash_quad_enable(qspi_id_t id);

/**
 * @brief      QSPI flash erase 32k
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_qspi_flash_erase_32k(qspi_id_t id, uint32_t addr);

/**
 * @brief      QSPI flash erase 64k
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_qspi_flash_erase_64k(qspi_id_t id, uint32_t addr);

/**
 * @brief      QSPI flash erase 4k/32k/64k with different type
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_qspi_flash_erase(qspi_id_t id, uint32_t addr, uint32_t type);

/**
 * @brief      QSPI flash quad write
 *
 * @param addr QSPI write address
 * @param data QSPI write data
 * @param size the size of QSPI write data
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_qspi_flash_quad_page_program(qspi_id_t id, uint32_t addr, const void *data, uint32_t size);

/**
 * @brief      QSPI flash single write
 *
 * @param addr QSPI write address
 * @param data QSPI write data
 * @param size the size of QSPI write data
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_qspi_flash_single_page_program(qspi_id_t id, uint32_t addr, const void *data, uint32_t size);

/**
 * @brief      QSPI flash quad read
 *
 * @param addr QSPI read address
 * @param data QSPI read data
 * @param size the size of QSPI read data
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_qspi_flash_quad_read(qspi_id_t id, uint32_t addr, void *data, uint32_t size);

/**
 * @brief      QSPI flash single read
 * @param addr QSPI read address
 * @param data QSPI read data
 * @param size the size of QSPI read data
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_qspi_flash_single_read(qspi_id_t id, uint32_t addr, void *data, uint32_t size);

/**
 * @brief      QSPI flash write data
 *
 * @param base_addr QSPI write address
 * @param data QSPI write data
 * @param size the size of QSPI write data
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_qspi_flash_write(qspi_id_t id, uint32_t base_addr, const void *data, uint32_t size);

/**
 * @brief      QSPI flash read data
 *
 * @param base_addr QSPI read address
 * @param data QSPI read data
 * @param size the size of QSPI read data
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_qspi_flash_read(qspi_id_t id, uint32_t base_addr, void *data, uint32_t size);

#if CONFIG_QSPI_NAND_FLASH
bk_err_t bk_qspi_flash_nand_get_id(qspi_id_t id, uint8_t *buf, uint32_t len);
bk_err_t bk_qspi_flash_nand_get_feature(qspi_id_t id, uint8_t addr, uint8_t *value);
bk_err_t bk_qspi_flash_nand_set_feature(qspi_id_t id, uint8_t addr, uint8_t value);
bk_err_t bk_qspi_flash_nand_get_block_lock(qspi_id_t id, uint8_t *value);
bk_err_t bk_qspi_flash_nand_set_block_lock(qspi_id_t id, uint8_t value);
bk_err_t bk_qspi_flash_nand_get_status(qspi_id_t id, uint8_t *value);
bk_err_t bk_qspi_flash_nand_get_feature_register(qspi_id_t id, uint8_t *value);
bk_err_t bk_qspi_flash_nand_set_feature_register(qspi_id_t id, uint8_t value);
bk_err_t bk_qspi_flash_nand_set_protect_none(qspi_id_t id);
bk_err_t bk_qspi_flash_nand_block_erase(qspi_id_t id, uint32_t block);
bk_err_t bk_qspi_flash_nand_page_program(qspi_id_t id, uint32_t page, uint32_t column, const uint8_t *buf, uint32_t len);
bk_err_t bk_qspi_flash_nand_page_read(qspi_id_t id, uint32_t page, uint32_t column, uint8_t *buf, uint32_t len);
bk_err_t bk_qspi_flash_nand_page_program_quad(qspi_id_t id, uint32_t page, uint32_t column, const uint8_t *buf, uint32_t len);
bk_err_t bk_qspi_flash_nand_page_read_quad(qspi_id_t id, uint32_t page, uint32_t column, uint8_t *buf, uint32_t len);
#endif /* CONFIG_QSPI_NAND_FLASH */

/**
 * @brief      QSPI flash read status register S0-S7
 *
 * @param id QSPI ID
 * @return Status register S0-S7 value
 */
uint32_t bk_qspi_flash_read_s0_s7(qspi_id_t id);

/**
 * @brief      QSPI flash read status register S8-S15
 *
 * @param id QSPI ID
 * @return Status register S8-S15 value
 */
uint32_t bk_qspi_flash_read_s8_s15(qspi_id_t id);

/**
 * @brief      QSPI flash read status register S16-S23
 *
 * @param id QSPI ID
 * @return Status register S16-S23 value
 */
uint32_t bk_qspi_flash_read_s16_s23(qspi_id_t id);

/**
 * @brief      QSPI flash write status register S16-S23
 *
 * @param id QSPI ID
 * @param status_reg_data Status register S16-S23 value to write
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_qspi_flash_write_s16_s23(qspi_id_t id, uint8_t status_reg_data);

#ifdef __cplusplus
}
#endif

