/**
 * @file i3c_slave.h
 * @brief I3C Slave: config API + data transfer API (poll/interrupt)
 */
#ifndef I3C_SLAVE_H
#define I3C_SLAVE_H

#include <stdint.h>
#include "i3c_common.h"
#include <common/bk_err.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t i3c_slv_device_t;

typedef enum {
	I3C_SLV_MODE_SDR = 0,
	I3C_SLV_MODE_HDR = 1,
} i3c_slave_mode_t;

typedef struct {
	const i3c_platform_config_t *platform;  /**< NULL = default */
	i3c_slv_device_t slv_device;            /**< 0/1/2 select PID */
	i3c_slave_mode_t mode;                  /**< SDR or HDR */
	uint16_t max_mwl;                       /**< Max write length, 0 = default 128 */
	uint16_t max_mrl;                       /**< Max read length, 0 = default 128 */
	i3c_core_clk_src_t core_clk_src;        /**< Must match master / board (default APLL) */
	uint32_t core_hz;                       /**< 0 = default Hz for core_clk_src */
} i3c_slave_config_t;

/* ---------- Config API ---------- */
bk_err_t bk_i3c_slave_init(const i3c_slave_config_t *cfg);
bk_err_t bk_i3c_slave_deinit(void);
void i3c_slave_int_unregister(void);  /* Internal use */

/* ---------- Data transfer API (poll/interrupt) ---------- */
/** Write data (slave TX when master reads). timeout_ms: 0 = default */
bk_err_t bk_i3c_slave_write(const uint8_t *data, uint32_t len, uint32_t timeout_ms, i3c_xfer_mode_t xfer_mode);

/** Read data (slave RX when master writes). expect_bytes: HDR expected bytes, ignored in SDR; timeout_ms: 0 = default */
bk_err_t bk_i3c_slave_read(uint8_t *buf, uint32_t buf_size, uint32_t *recv_len, uint32_t expect_bytes,
                           uint32_t timeout_ms, i3c_xfer_mode_t xfer_mode);

#ifdef __cplusplus
}
#endif

#endif /* I3C_SLAVE_H */
