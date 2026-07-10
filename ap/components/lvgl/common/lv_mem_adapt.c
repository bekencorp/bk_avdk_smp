#include "lv_mem_adapt.h"

#include <os/mem.h>

void *lv_mem_adapt_malloc(size_t size)
{
    return hsram_malloc(size);
}

void *lv_mem_adapt_realloc(void *ptr, size_t size)
{
    return hsram_realloc(ptr, size);
}

void lv_mem_adapt_free(void *ptr)
{
    hsram_free(ptr);
}
