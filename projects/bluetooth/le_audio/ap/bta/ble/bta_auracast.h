#pragma once

#include <components/bluetooth/bk_dm_bap_types.h>

typedef void(*bta_auracast_sink_scan_result_cb)(bk_bap_source_announce_data_t *bk_bap_source_announce_data);
typedef void(*bta_auracast_sink_scan_stop_cb)(void *param);

typedef struct
{
    bta_auracast_sink_scan_result_cb result_cb;
    bta_auracast_sink_scan_stop_cb stop_cb;
} bta_auracast_scan_cfg_t;

enum
{
    AURACAST_STATE_TURN_OFF = 0,
    AURACAST_STATE_TURNING_OFF,
    AURACAST_STATE_TURNING_ON,
    AURACAST_STATE_TURN_ON
};

enum
{
    AURACAST_SCAN_IDLE = 0,
    AURACAST_SCANNING,
};


enum
{
    AURACAST_ROLE_NUKNOWN = 0,
    AURACAST_ROLE_SINK,
    AURACAST_ROLE_SOURCE,
};

bool bta_auracast_is_zero_address(uint8_t *address);
int bta_auracast_address_cmp(uint8_t *src, uint8_t *dst);



void bta_auracast_init(void);
void bta_auracast_boradcast_scan_start(bta_auracast_scan_cfg_t *cfg);
void bta_auracast_boradcast_scan_stop(void);
void bta_auracast_boradcast_associate(bk_bap_source_announce_data_t *bk_bap_source_announce_data);
void bta_auracast_boradcast_dissociate(void);
void bta_auracast_boradcast_enable(void);
void bta_auracast_boradcast_disable(void);

void bta_auracast_boradcast_start(void);
void bta_auracast_boradcast_setup_announcement(void);
void bta_auracast_data_send(uint8_t *data, uint16_t length);
void bta_auracast_switch(void);
void bta_auracast_lc3_dummy(void);
void bta_auracast_state_change(uint8_t state);

uint8_t bta_auracast_get_state(void);
uint8_t bta_auracast_get_scan_state(void);
uint8_t bta_auracast_get_role(void);




void bta_auracast_client_start(void);
void bta_auracast_client_stop(void);
void bta_auracast_server_start(void);
void bta_auracast_server_stop(void);


