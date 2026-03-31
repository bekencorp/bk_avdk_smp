/*
 * mock_port_heap.c - Mock implementation of port heap interface
 *
 * This file implements the mock functions for unit testing.
 */

#include "mock_port_heap.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ============================================================================
 * Mock State Tracking (for test verification)
 * ============================================================================ */

static int mock_enter_critical_count = 0;
static int mock_exit_critical_count = 0;
static int mock_assert_fail_count = 0;

/* ============================================================================
 * Mock State Query Functions
 * ============================================================================ */

int mock_get_enter_critical_count(void)
{
    return mock_enter_critical_count;
}

int mock_get_exit_critical_count(void)
{
    return mock_exit_critical_count;
}

int mock_get_assert_fail_count(void)
{
    return mock_assert_fail_count;
}

void mock_reset_counters(void)
{
    mock_enter_critical_count = 0;
    mock_exit_critical_count = 0;
    mock_assert_fail_count = 0;
}

/* ============================================================================
 * Mock Implementation
 * ============================================================================ */

void mock_port_heap_enter_critical(void)
{
    mock_enter_critical_count++;
    /* In single-threaded test environment, can be empty or just record the call */
}

void mock_port_heap_exit_critical(void)
{
    mock_exit_critical_count++;
    /* In single-threaded test environment, can be empty or just record the call */
}

void *mock_port_heap_memset(void *s, int c, size_t n)
{
    return memset(s, c, n);
}

void *mock_port_heap_memcpy(void *dest, const void *src, size_t n)
{
    return memcpy(dest, src, n);
}

void mock_port_heap_assert(int condition, const char *file, int line)
{
    if (!condition) {
        mock_assert_fail_count++;
        fprintf(stderr, "Assertion failed: %s:%d\n", file, line);
        /* In test environment, can choose to abort or continue */
        #ifdef MOCK_ASSERT_ABORT
        abort();
        #endif
    }
}

void mock_port_heap_error_handler(int error_code)
{
    fprintf(stderr, "Error handler called with code: %d\n", error_code);
    /* In test environment, record the error */
}

