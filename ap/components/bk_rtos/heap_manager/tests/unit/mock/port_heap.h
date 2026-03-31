/*
 * port_heap.h - Platform abstraction interface for heap management (Test Environment)
 *
 * This is the test environment version of port_heap.h.
 * It includes the mock implementation instead of the real platform implementation.
 */

#ifndef PORT_HEAP_H
#define PORT_HEAP_H

#include <stddef.h>
#include <stdint.h>
#include "bk_heap_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Include Mock Implementation
 * ============================================================================ */

/* In test environment, use mock implementation */
#include "mock_port_heap.h"

/* ============================================================================
 * Thread Safety Interface
 * ============================================================================ */

/**
 * @brief Enter critical section
 *
 * This function enters a critical section to ensure thread safety during
 * heap operations. The implementation depends on the target RTOS platform.
 */
void port_heap_enter_critical(void);

/**
 * @brief Exit critical section
 *
 * This function exits the critical section after heap operations are complete.
 * Must be called in pair with port_heap_enter_critical().
 */
void port_heap_exit_critical(void);

/* ============================================================================
 * Memory Operation Interface
 * ============================================================================ */

/**
 * @brief Set memory to a specific value
 *
 * @param s Pointer to the memory block
 * @param c Value to set (converted to unsigned char)
 * @param n Number of bytes to set
 * @return Pointer to the memory block
 */
void *port_heap_memset(void *s, int c, size_t n);

/**
 * @brief Copy memory from source to destination
 *
 * @param dest Pointer to destination memory
 * @param src Pointer to source memory
 * @param n Number of bytes to copy
 * @return Pointer to destination memory
 */
void *port_heap_memcpy(void *dest, const void *src, size_t n);

/* ============================================================================
 * Assertion and Error Handling Interface
 * ============================================================================ */

/**
 * @brief Assert a condition
 *
 * If the condition is false, this function should trigger an assertion failure.
 * The behavior depends on the platform implementation (abort, log, etc.).
 *
 * @param condition Condition to check (should be non-zero if true)
 * @param file Source file name (for debugging)
 * @param line Line number (for debugging)
 */
void port_heap_assert(int condition, const char *file, int line);

/* ============================================================================
 * Alignment Macros
 * ============================================================================ */

/* Byte alignment (8 bytes) */
#define PORT_HEAP_BYTE_ALIGNMENT           BK_HEAP_BYTE_ALIGNMENT

/* Byte alignment mask */
#define PORT_HEAP_BYTE_ALIGNMENT_MASK      BK_HEAP_BYTE_ALIGNMENT_MASK

/* Align address up to byte boundary */
#define PORT_HEAP_ALIGN_UP(addr)           BK_HEAP_ALIGN_UP(addr)

/* Align address down to byte boundary */
#define PORT_HEAP_ALIGN_DOWN(addr)         BK_HEAP_ALIGN_DOWN(addr)

/* Check if address is aligned */
#define PORT_HEAP_IS_ALIGNED(addr)         BK_HEAP_IS_ALIGNED(addr)

/* ============================================================================
 * Assertion Macro
 * ============================================================================ */

/**
 * @brief Assert macro for heap operations
 *
 * Usage: PORT_HEAP_ASSERT(condition);
 */
#define PORT_HEAP_ASSERT(condition) \
    port_heap_assert((condition) ? 1 : 0, __FILE__, __LINE__)

#ifdef __cplusplus
}
#endif

#endif /* PORT_HEAP_H */

