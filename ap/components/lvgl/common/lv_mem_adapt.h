#ifndef LV_MEM_ADAPT_H
#define LV_MEM_ADAPT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void *lv_malloc_core(size_t size);
void *lv_realloc_core(void *ptr, size_t size);
void lv_free_core(void *ptr);

#ifdef __cplusplus
}
#endif

#endif
