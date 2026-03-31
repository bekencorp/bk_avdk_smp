/*
 * mock_port_heap.h - Mock implementation of port heap interface
 *
 * This file provides a mock implementation of the port heap interface
 * for unit testing. It simulates the platform abstraction layer.
 */

#ifndef MOCK_PORT_HEAP_H_INCLUDED
#define MOCK_PORT_HEAP_H_INCLUDED

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Mock Platform Abstraction Interface
 * ============================================================================ */

/**
 * @brief Mock enter critical section
 */
void mock_port_heap_enter_critical(void);

/**
 * @brief Mock exit critical section
 */
void mock_port_heap_exit_critical(void);

/**
 * @brief Mock memory set
 */
void *mock_port_heap_memset(void *s, int c, size_t n);

/**
 * @brief Mock memory copy
 */
void *mock_port_heap_memcpy(void *dest, const void *src, size_t n);

/**
 * @brief Mock assertion
 */
void mock_port_heap_assert(int condition, const char *file, int line);

/**
 * @brief Mock error handler
 */
void mock_port_heap_error_handler(int error_code);

/* ============================================================================
 * Mock Configuration
 * ============================================================================ */

/* Note: PORT_HEAP_BYTE_ALIGNMENT and PORT_HEAP_BYTE_ALIGNMENT_MASK
 * are defined in port_heap.h, so we don't redefine them here to avoid conflicts.
 */

/* ============================================================================
 * Mock State Query Functions (for testing)
 * ============================================================================ */

/**
 * @brief Get enter critical count
 */
int mock_get_enter_critical_count(void);

/**
 * @brief Get exit critical count
 */
int mock_get_exit_critical_count(void);

/**
 * @brief Get assert fail count
 */
int mock_get_assert_fail_count(void);

/**
 * @brief Reset all counters
 */
void mock_reset_counters(void);

/* ============================================================================
 * Use Mock Implementation - Map port_heap_* calls to mock_port_heap_*
 * These macros must come AFTER the function declarations above.
 * ============================================================================ */

#define port_heap_enter_critical    mock_port_heap_enter_critical
#define port_heap_exit_critical     mock_port_heap_exit_critical
#define port_heap_memset            mock_port_heap_memset
#define port_heap_memcpy            mock_port_heap_memcpy
#define port_heap_assert            mock_port_heap_assert

#ifdef __cplusplus
}
#endif

#endif /* MOCK_PORT_HEAP_H_INCLUDED */

