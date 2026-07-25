#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int unicast_server_demo_init(void);
int unicast_server_cli_init(void);
int unicast_server_demo_adv(uint8_t on);
int unicast_server_demo_rx_ready(uint8_t ase_id);
int unicast_server_demo_release(uint8_t ase_id);

#ifdef __cplusplus
}
#endif
