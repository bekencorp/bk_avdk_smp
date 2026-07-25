#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int unicast_client_demo_init(void);
int unicast_client_cli_init(void);
int unicast_client_demo_scan_peers(uint8_t on);
void unicast_client_demo_list_peers(void);
int unicast_client_demo_connect_peer(uint8_t index);
int unicast_client_demo_connect_addr(uint8_t *addr, uint8_t addr_type);
int unicast_client_demo_tone_start(uint16_t connection_handle);
int unicast_client_demo_tone_stop(void);
int unicast_client_demo_assistant_scan(uint8_t on);
void unicast_client_demo_assistant_list_sources(void);
int unicast_client_demo_assistant_connect(uint8_t *deleg_addr, uint8_t addr_type);
int unicast_client_demo_assistant_discover(uint8_t *deleg_addr, uint8_t addr_type);
int unicast_client_demo_assistant_add(uint8_t source_index, uint32_t bis_sync);
int unicast_client_demo_assistant_remove(uint8_t source_id);
void unicast_client_demo_assistant_stop(void);
int unicast_client_demo_set_broadcast_code(const uint8_t *code16);

#ifdef __cplusplus
}
#endif
