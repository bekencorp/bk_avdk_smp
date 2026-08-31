#ifndef __BK_TOF_VI5302_H__
#define __BK_TOF_VI5302_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Board pins (shared I2C1 with ICM-42670-P) */
#if CONFIG_TOF_I2C_ID == 0
#define TOF_I2C_ID              I2C_ID_0
#elif CONFIG_TOF_I2C_ID == 1
#define TOF_I2C_ID              I2C_ID_1
#elif CONFIG_TOF_I2C_ID == 2
#define TOF_I2C_ID              I2C_ID_2
#else
#error "CONFIG_TOF_I2C_ID must be 0, 1, or 2"
#endif

#define TOF_XSHUT_PIN           CONFIG_TOF_XSHUT_PIN
#define TOF_INT_PIN             CONFIG_TOF_INT_PIN


typedef struct {
	uint16_t distance_mm;
	uint8_t confidence;	/* official SDK: typically valid if > 30 */
	uint8_t status;
} tof_vi5302_result_t;

typedef void (*tof_vi5302_cb_t)(void *arg, const tof_vi5302_result_t *result);


/** Restore saved factory cal into chip (CG_Pos) + host offset. */
int bk_tof_vi5302_apply_cali(int8_t cg_pos, float offset);

/** Run official Xtalk + Offset calibration once (needs empty FOV / target). */
int bk_tof_vi5302_factory_calibrate(uint16_t offset_target_mm);

int bk_tof_vi5302_get_sp_flag(uint8_t *flag);
int bk_tof_vi5302_heartbeat(uint8_t *val);

/* Legacy stubs kept for build compatibility */
void bk_tof_vi5302_set_calibration(const uint8_t *data, uint32_t len);
int bk_tof_vi5302_download_calibration(const uint8_t *data, uint32_t len);


int bk_tof_vi5302_register_callback(tof_vi5302_cb_t cb, void *arg);

int bk_tof_vi5302_init(void);
void bk_tof_vi5302_deinit(void);

int bk_tof_vi5302_start_once(void);
int bk_tof_vi5302_start_continuous(void);
int bk_tof_vi5302_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* __BK_TOF_VI5302_H__ */
