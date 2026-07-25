#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int broadcast_source_demo_init(void);
int broadcast_source_cli_init(void);
int broadcast_source_demo_start(void);
int broadcast_source_demo_stop(void);
int broadcast_source_demo_set_broadcast_code(const uint8_t *code16);
const uint8_t *broadcast_source_demo_broadcast_code(void);

#ifdef __cplusplus
}
#endif
