#pragma once

#include <stdint.h>

enum
{
	BTM_POWER_STATUS_OFF = 0,
	BTM_POWER_STATUS_OFFING,
	BTM_POWER_STATUS_ONING,
	BTM_POWER_STATUS_ON,
};

void bta_manager_init(void);

void bta_manager_pairing_mode(void);
void bta_manager_broadcast(void);
void bta_manager_play(void);

void bta_manager_on(void);
void bta_manager_off(void);
uint8_t bta_manager_is_turn_on(void);
void bta_manager_volume_up(void);
void bta_manager_volume_down(void);

void bta_manager_event_dispather(uint32_t event, uint32_t param, uint32_t extra);
void bta_manager_get_address(uint8_t *address);
void bta_manager_set_name(char *name);
void bta_manager_set_white_list(uint8_t *address);
