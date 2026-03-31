#include <common/bk_include.h>
#include "bk_arm_arch.h"
#include <string.h>

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include <os/os.h>
#include "bk_uart.h"
#include <os/mem.h>
#include "arch_interrupt.h"

INT32 os_memcmp(const void *s1, const void *s2, UINT32 n)
{
	return memcmp(s1, s2, (unsigned int)n);
}

void *os_memmove(void *out, const void *in, UINT32 n)
{
	configASSERT(NULL != in);
	return memmove(out, in, n);
}

void *os_memcpy(void *out, const void *in, UINT32 n)
{
    if (out == NULL || in == NULL || n == 0)
        return out;

    if ((((uintptr_t)out | (uintptr_t)in | n) & 0x3) == 0)
    {
        os_memcpy_word((uint32_t *)out, (const uint32_t *)in, n);
        return out;
    }
    else
    {
        extern void *memcpy(void *dest, const void *src, size_t n);
        return memcpy(out, in, n);
    }
}

void *os_memset(void *b, int c, UINT32 len)
{
	return (void *)memset(b, c, (unsigned int)len);
}

int os_memcmp_const(const void *a, const void *b, size_t len)
{
	return memcmp(a, b, len);
}

char *
debug_strdup(const char *s, const char *file, int line)
{
    char *ret = strdup(s);
    BK_DUMP_OUT("%p: strdup(%p) at  %s :%d.\r\n", ret, s, file, line);
    return ret;
}

void* os_malloc_wifi_buffer(size_t size)
{
    return os_malloc_release(size);
}

extern void mem_show_info(void);
void os_show_memory_config_info(void)
{
	mem_show_info();
}
