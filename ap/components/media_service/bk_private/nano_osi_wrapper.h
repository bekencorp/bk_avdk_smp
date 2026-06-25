#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>


typedef enum
{
    BK_NANO_MODULE_VENC = 0,
    BK_NANO_MODULE_VDEC = 1,
    BK_NANO_MODULE_ISP = 2,
} bk_nano_module_t;

typedef struct
{
    const uint32_t gpu_base_addr;
    const uint32_t gpu_vg_lite_contiguous_mem_sz;
    const uint32_t gpu_vg_lite_command_buffer_size;
    void *(*malloc)(uint32_t size);
    void (*free)(void *ptr);
    void *(*memset)(void *s, int c, uint32_t n);
    void *(*memcpy)(void *dest, const void *src, uint32_t n);
    /* Hardware DMA copy. Returns 0 on success, non-zero on failure. */
    int (*dma_memcpy)(void *dest, const void *src, uint32_t n);
    int (*memcmp)(const void *s1, const void *s2, size_t n);
    int (*strcmp)(const char *s1, const char *s2);
    int (*vsnprintf)(char *str, uint32_t size, const char *format, va_list args);
    void (*log_printf)(int level, char *tag, const char *fmt, va_list args);
    void (*usleep)(uint32_t usec);
    void (*delay_milliseconds)(uint32_t milliseconds);

    // Mutex operations: use beken_mutex_t* directly (beken_mutex_t is void*)
    int (*mutex_create)(beken_mutex_t *os_mutex);
    int (*mutex_destroy)(beken_mutex_t *os_mutex);
    int (*mutex_lock)(beken_mutex_t *os_mutex);
    int (*mutex_unlock)(beken_mutex_t *os_mutex);

    // Queue operations: use beken_queue_t* directly (beken_queue_t is void*)
    int (*init_queue)(beken_queue_t *os_queue, const char *name, uint32_t size, uint32_t number_of_messages);
    int (*deinit_queue)(beken_queue_t *os_queue);
    int (*queue_send)(beken_queue_t *os_queue, void *data, uint32_t timeout);
    int (*queue_recv)(beken_queue_t *os_queue, void *data, uint32_t timeout);

    // Semaphore operations: use beken_semaphore_t* directly (beken_semaphore_t is void*)
    int (*sem_init)(beken_semaphore_t *os_sem, uint32_t value);
    int (*sem_post)(beken_semaphore_t *os_sem);
    int (*sem_wait)(beken_semaphore_t *os_sem, uint32_t ms);
    int (*sem_destroy)(beken_semaphore_t *os_sem);

    // Thread operations: use beken_thread_t* directly (beken_thread_t is void*)
    int (*thread_create)(beken_thread_t *thread,
                         uint8_t priority,
                         const char *name,
                         void (*function)(void *),
                         uint32_t stack_size,
                         void *arg);
    int (*thread_destroy)(beken_thread_t *thread);

    int (*int_isr_register)(uint8_t type, void* isr, void* arg);
    int (*int_isr_unregister)(uint8_t type);
    int (*int_enable)(uint32_t int_num, uint32_t int_en);

    // isr critical operations: use uint32_t for irq flags
    uint32_t (*enter_critical)(void);
    void (*exit_critical)(uint32_t flags);

    uint32_t (*module_enter_critical)( bk_nano_module_t module);
    void (*module_exit_critical)(bk_nano_module_t module, uint32_t flags);
} bk_nano_osi_funcs_t;

bk_err_t bk_nano_osi_funcs_init(void);

#ifdef __cplusplus
}
#endif


