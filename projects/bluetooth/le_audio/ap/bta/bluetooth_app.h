#pragma once

void bluetooth_app_init(void);
void bluetooth_app_broadcast(void);
void bluetooth_app_pairing(void);
void bluetooth_app_abs_volume(uint8_t per);
void bluetooth_app_volume_up(void);
void bluetooth_app_volume_down(void);
void bluetooth_app_play(void);
void bluetooth_app_on(void);
void bluetooth_app_off(void);
uint8_t bluetooth_app_is_on(void);
void bluetooth_gat_address(uint8_t *address);
void bluetooth_set_name(char *name);
