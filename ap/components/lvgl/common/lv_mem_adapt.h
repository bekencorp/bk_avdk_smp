#ifndef LV_MEM_ADAPT_H
#define LV_MEM_ADAPT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void *lv_mem_adapt_malloc(size_t size);
void *lv_mem_adapt_realloc(void *ptr, size_t size);
void lv_mem_adapt_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif
