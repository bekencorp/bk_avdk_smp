#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Private bridge between the GA bearer reference (ga_brr_appl.c) and the
 * public bk_dm_le_audio_gap adapter (service/dm/ble/le_audio/bk_dm_le_audio_gap.c).
 *
 * appl_le_audio_* : down-calls implemented in ga_brr_appl.c / appl_ga.c
 *                   (call GA_brr_* / ga_brr_dyn_gatt_db_init_pl).
 */

/* ACL transport up/down from GA_BRR_TRANSPORT_UP/DOWN_IND. */
typedef void (*appl_le_audio_transport_cb_t)(uint8_t connected,
                                             uint8_t addr_type,
                                             const uint8_t *addr);

/* ---- down-calls (ga_brr_appl.c) ---- */
void appl_le_audio_register_transport_cb(appl_le_audio_transport_cb_t cb);

/* GA-native connectable extended advertising (BAP Table 3.7). */
uint32_t appl_le_audio_gap_adv(uint8_t enable,
                               uint8_t handle,
                               const uint8_t *adv_data,
                               uint8_t adv_len,
                               uint16_t interval_min,
                               uint16_t interval_max);

/* ---- down-calls (appl_ga.c) ---- */
/* Commit (register / re-snapshot) the dynamic LE Audio GATT DB. */
uint32_t appl_le_audio_ga_gatt_db_register(void);

#ifdef __cplusplus
}
#endif
