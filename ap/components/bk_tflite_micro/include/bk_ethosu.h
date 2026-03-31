#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int bk_ethosu_init(void *fast_memory, uint32_t fast_memory_size);

void bk_ethosu_deinit(void);


#ifdef __cplusplus
}
#endif