/**
 * @file i3c_common.h
 * @brief I3C common: types, platform config, HDR helpers. Shared by master and slave.
 *        Register access via i3c_hal.h (Driver->HAL->LL->SoC model).
 */
#ifndef I3C_COMMON_H
#define I3C_COMMON_H

#include <stdint.h>
#include <components/log.h>
#include "sdkconfig.h"
#if defined(CONFIG_SOC_BK7259)
#include "soc/bk7259/i2s_cap.h"
#endif

#if defined(CONFIG_I3C)
#include "i3c_hal.h"
#include "i3c_reg.h"
#endif

/* Log tags */
#define I3C_TEST_TAG "I3C_TEST"
#define I3C_MST_TAG  "I3C_MST"
#define I3C_SLV_TAG  "I3C_SLV"

#define I3C_TEST_LOGI(...) BK_LOGI(I3C_TEST_TAG, ##__VA_ARGS__)
#define I3C_TEST_LOGW(...) BK_LOGW(I3C_TEST_TAG, ##__VA_ARGS__)
#define I3C_TEST_LOGE(...) BK_LOGE(I3C_TEST_TAG, ##__VA_ARGS__)
#define I3C_TEST_LOGD(...) BK_LOGD(I3C_TEST_TAG, ##__VA_ARGS__)
#define I3C_TEST_LOGV(...) BK_LOGV(I3C_TEST_TAG, ##__VA_ARGS__)

#define I3C_MST_LOGI(...) BK_LOGI(I3C_MST_TAG, ##__VA_ARGS__)
#define I3C_MST_LOGW(...) BK_LOGW(I3C_MST_TAG, ##__VA_ARGS__)
#define I3C_MST_LOGE(...) BK_LOGE(I3C_MST_TAG, ##__VA_ARGS__)
#define I3C_MST_LOGD(...) BK_LOGD(I3C_MST_TAG, ##__VA_ARGS__)
#define I3C_MST_LOGV(...) BK_LOGV(I3C_MST_TAG, ##__VA_ARGS__)

#define I3C_SLV_LOGI(...) BK_LOGI(I3C_SLV_TAG, ##__VA_ARGS__)
#define I3C_SLV_LOGW(...) BK_LOGW(I3C_SLV_TAG, ##__VA_ARGS__)
#define I3C_SLV_LOGE(...) BK_LOGE(I3C_SLV_TAG, ##__VA_ARGS__)
#define I3C_SLV_LOGD(...) BK_LOGD(I3C_SLV_TAG, ##__VA_ARGS__)
#define I3C_SLV_LOGV(...) BK_LOGV(I3C_SLV_TAG, ##__VA_ARGS__)

#ifdef __cplusplus
extern "C" {
#endif

/** Default I3C unit ID (multi-instance ready). */
#define I3C_UNIT_ID  I3C_HAL_DEFAULT_ID

/* Beken device role (for HAL) */
#define I3C_BEKEN_PS_DEVICE_ROLE_MAIN_MASTER  0x0u
#define I3C_BEKEN_PS_DEVICE_ROLE_SEC_MASTER   0x1u
#define I3C_BEKEN_PS_DEVICE_ROLE_SLAVE        0x2u

#if defined(CONFIG_I3C)
void i3c_beken_set_sw_reset(uint32_t val);
void i3c_beken_set_clkg_bps_cdn(uint32_t val);
void i3c_beken_set_clkg_bps_beken(uint32_t val);
void i3c_beken_set_ps_device_role(uint32_t val);
void i3c_beken_set_ps_dcr(uint32_t val);
void i3c_beken_set_ps_mrl(uint32_t val);
void i3c_beken_set_ps_mwl(uint32_t val);
void i3c_beken_set_ps_mxds_limited(uint32_t val);
void i3c_beken_set_ps_mxds_maxwr(uint32_t val);
void i3c_beken_set_ps_mxds_maxrd(uint32_t val);
void i3c_beken_set_ps_pid_mfr_id(uint32_t val);
void i3c_beken_set_ps_pid_instance_id(uint32_t val);
void i3c_beken_set_ps_bus_avail_timer(uint32_t val);
void i3c_beken_set_ps_bus_idle_timer(uint32_t val);
void i3c_beken_set_ps_stat_addr(uint32_t val);
void i3c_beken_set_ps_flow_ctrl_pr_dis(uint32_t val);
void i3c_beken_set_ps_flow_ctrl_pw_dis(uint32_t val);
void i3c_beken_set_ps_fpf_pw_sel(uint32_t val);
void i3c_beken_set_ps_alt_mode_en(uint32_t val);
void i3c_beken_set_ps_hj_in_use(uint32_t val);
void i3c_beken_set_ps_ibi_mdb_prn(uint32_t val);
void i3c_beken_set_ps_rx_data_fifo_mode(uint32_t val);
void i3c_beken_set_ps_periph_rst_ret_time(uint32_t val);
void i3c_beken_set_ps_chip_rst_ret_time(uint32_t val);
void i3c_beken_set_ps_xtime_freq_byte(uint32_t val);
void i3c_beken_set_ps_xtime_inacc_byte(uint32_t val);
void i3c_beken_set_tgt_tcam0_t_c1_xdel(uint32_t val);
uint32_t i3c_beken_get_reg(uint32_t off);
#endif

/** Poll/interrupt mode for data transfer */
typedef enum {
	I3C_XFER_POLL = 0,
	I3C_XFER_INT  = 1,
} i3c_xfer_mode_t;

/** Platform config: GPIO for SCL/SDA/PURN */
typedef struct {
	uint32_t gpio_scl;
	uint32_t gpio_sda;
	uint32_t gpio_purn;
} i3c_platform_config_t;

/** I3C IP core clock source (Beken strap + prescaler f_core). See i3c_core_clk_src_apply(). */
typedef enum {
	I3C_CORE_CLK_SRC_APLL = 0,       /**< I3C controller hclk from APLL: clkg_bps_cdn=1, clkg_bps_beken=0 (reference / default) */
	I3C_CORE_CLK_SRC_XTAL_26M = 1,   /**< 26 MHz crystal domain: clkg_bps_cdn=0, clkg_bps_beken=1 */
} i3c_core_clk_src_t;

/**
 * IP core clock (Hz) for prescaler math when APLL path is used after i3c_open_apll sequence.
 * Uses SOC_I2S_APLL_RATE (bk7259) — same nominal rate as APLL used for I2S (not a magic constant).
 */
#if defined(CONFIG_SOC_BK7259)
#define I3C_CORE_HZ_APLL       SOC_I2S_APLL_RATE
#else
#define I3C_CORE_HZ_APLL       98304000u
#endif
/** XTAL core Hz from Kconfig (reference test_i3c uses 26 MHz crystal domain). */
#define I3C_CORE_HZ_XTAL_26M   CONFIG_XTAL_FREQ

#if defined(CONFIG_I3C)
void i3c_core_clk_src_apply(i3c_core_clk_src_t src, uint32_t core_hz_override);
uint32_t i3c_core_hz_get(void);
#endif

void i3c_platform_init(const i3c_platform_config_t *cfg);
void i3c_platform_deinit(void);

void i3c_delay_ms(uint32_t ms);
#ifndef I3C_DELAY_MS
#define I3C_DELAY_MS(ms)  i3c_delay_ms(ms)
#endif

/* HDR (HDR-DDR): CRC5 and parity */
uint8_t i3c_hdr_crc5(uint8_t crc_in, uint16_t data_word);
uint16_t i3c_hdr_parity_odd_even(uint16_t data, uint16_t is_even);
uint32_t i3c_hdr_tx_word(uint8_t preamble, uint8_t data_hi, uint8_t data_lo);

/* Prescaler / SCL rate constants (used by driver with HAL). Default SCL example uses APLL core Hz + legacy PP. */
#define I3C_PRESCL_PP_HIGH_DEFAULT    0x19u
#define I3C_PRESCL_PP_LOW_DEFAULT     1u
#define I3C_SCL_HZ_DEFAULT            (I3C_CORE_HZ_APLL / (2u * (I3C_PRESCL_PP_HIGH_DEFAULT + 1u) * (I3C_PRESCL_PP_LOW_DEFAULT + 1u)))
/** Legacy name: 26 MHz crystal domain Hz (not the APLL default). */
#define I3C_XTAL_HZ_DEFAULT           I3C_CORE_HZ_XTAL_26M
#define I3C_SCL_HZ_FROM_PRESCL(f_core, pp_high, pp_low) \
	((f_core) / (2u * ((pp_high) + 1u) * ((pp_low) + 1u)))

#ifdef __cplusplus
}
#endif

#endif /* I3C_COMMON_H */
