/**
 * @file pbap_pce_demo_cli.h
 */

#ifndef PBAP_PCE_DEMO_CLI_H
#define PBAP_PCE_DEMO_CLI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TELECOM_PHONEBOOK_OBJECT_NAME "telecom/pb.vcf"
#define TELECOM_MISSED_CALLED_HISTORY_OBJECT_NAME "telecom/mch.vcf"
#define TELECOM_OUTGOING_CALLED_HISTORY_OBJECT_NAME "telecom/och.vcf"
#define TELECOM_INCOMING_CALLED_HISTORY_OBJECT_NAME "telecom/ich.vcf"
#define TELECOM_COMBINED_CALLED_HISTORY_OBJECT_NAME "telecom/cch.vcf"
#define TELECOM_SPEED_DIAL_OBJECT_NAME "telecom/spd.vcf"
#define TELECOM_FAVORITES_CONTACTS_OBJECT_NAME "telecom/fav.vcf"

#define SIM_PHONEBOOK_OBJECT_NAME "SIM1/telecom/pb.vcf"
#define SIM_MISSED_CALLED_HISTORY_OBJECT_NAME "SIM1/telecom/mch.vcf"
#define SIM_OUTGOING_CALLED_HISTORY_OBJECT_NAME "SIM1/telecom/och.vcf"
#define SIM_INCOMING_CALLED_HISTORY_OBJECT_NAME "SIM1/telecom/ich.vcf"
#define SIM_COMBINED_CALLED_HISTORY_OBJECT_NAME "SIM1/telecom/cch.vcf"

/* CLI init */
int cli_pbap_pce_demo_init(void);

/* pbap_pce_demo.c function declarations */
int pbap_pce_demo_init(void);
void pbap_pce_demo_deinit(void);
void pbap_pce_demo_connect(const uint8_t *bd_addr, uint8_t auth_required, const char *pin, const char *user_id);
void pbap_pce_demo_disconnect(const uint8_t *bd_addr);
void pbap_pce_demo_send_auth_response(const uint8_t *bd_addr, const char *pin, const char *user_id);
void pbap_pce_demo_get_phonebook(const uint8_t *bd_addr, const char *object,
                                 uint16_t max_list_count, uint16_t list_start_offset);
void pbap_pce_demo_set_phonebook(const uint8_t *bd_addr, uint8_t path_type, const char *path_name);
void pbap_pce_demo_get_vcard_list(const uint8_t *bd_addr, const char *object,
                                  uint16_t max_list_count, uint16_t list_start_offset);
void pbap_pce_demo_get_vcard(const uint8_t *bd_addr, const char *object);
void pbap_pce_demo_get_size(const uint8_t *bd_addr, const char *object);
void pbap_pce_demo_abort(const uint8_t *bd_addr);

#ifdef __cplusplus
}
#endif

#endif /* PBAP_PCE_DEMO_CLI_H */
