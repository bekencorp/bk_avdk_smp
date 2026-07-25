#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int broadcast_sink_demo_init(void);
int broadcast_sink_cli_init(void);
int broadcast_sink_demo_scan(uint8_t on);
void broadcast_sink_demo_list_sources(void);
int broadcast_sink_demo_sync(uint8_t source_index, uint8_t bis_index);
int broadcast_sink_demo_delegator_adv(uint8_t on);
void broadcast_sink_demo_stop(void);
int broadcast_sink_demo_set_broadcast_code(const uint8_t *code16);
const uint8_t *broadcast_sink_demo_broadcast_code(void);

#ifdef __cplusplus
}
#endif
