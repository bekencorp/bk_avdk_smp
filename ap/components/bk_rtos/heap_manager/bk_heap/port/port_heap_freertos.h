/*
 * port_heap_freertos.h - FreeRTOS platform implementation
 *
 * This file implements the platform abstraction interface for FreeRTOS.
 */

#ifndef PORT_HEAP_FREERTOS_H
#define PORT_HEAP_FREERTOS_H

#include "port_heap.h"
#include <os/os.h>
#include <os/mem.h>

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Thread Safety Implementation (FreeRTOS)
 * ============================================================================ */
#if CONFIG_SOC_SMP
void port_heap_enter_critical(void);
void port_heap_exit_critical(void);
#else
/**
 * @brief Enter critical section using FreeRTOS API
 */
static inline void port_heap_enter_critical(void)
{
    rtos_suspend_all_thread();
}

/**
 * @brief Exit critical section using FreeRTOS API
 */
static inline void port_heap_exit_critical(void)
{
    (void)rtos_resume_all_thread();
}
#endif

/* ============================================================================
 * Memory Operation Implementation
 * ============================================================================ */

/**
 * @brief Set memory using standard library
 */
static inline void *port_heap_memset(void *s, int c, size_t n)
{
    return os_memset(s, c, n);
}

/**
 * @brief Copy memory using standard library
 */
static inline void *port_heap_memcpy(void *dest, const void *src, size_t n)
{
    return os_memcpy(dest, src, n);
}

static inline const char *port_heap_get_task_name(void)
{
    if (rtos_is_scheduler_started())
        return pcTaskGetName(NULL);
    else
        return "NA";
}

#define PORT_HEAP_ASSERT(condition) BK_ASSERT(condition)

extern int arch_is_enter_exception(void);
static inline bool port_heap_in_risk_state(void)
{
    return rtos_is_in_interrupt_context() && (arch_is_enter_exception() == 0);
}

#ifdef __cplusplus
}
#endif

#endif /* PORT_HEAP_FREERTOS_H */

