#ifndef __TFLM_MICRO_H__
#define __TFLM_MICRO_H__

#include <stdint.h>

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

int person_detection_init(void** handle, uint8_t* model, uint32_t tensor_arena_size);
int person_detection_deinit(void* handle);
int person_detection_run(void* handle, uint8_t* data);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__TFLM_MICRO_H__
