#pragma once

#include <stdint.h>

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

int mobilefacenet_init(void** handle, uint8_t* model, uint32_t tensor_arena_size);
int mobilefacenet_deinit(void* handle);
int mobilefacenet_run(void* handle, int8_t* data, uint16_t width, uint16_t height, uint16_t channels);

#ifdef  __cplusplus
}
#endif//__cplusplus
