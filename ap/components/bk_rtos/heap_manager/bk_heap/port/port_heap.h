/*
 * port_heap.h - Platform abstraction interface for heap management
 *
 * This file defines the platform abstraction interface that allows the heap
 * management system to work with different RTOS platforms.
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
 * Platform Implementation Selection
 * ============================================================================ */

/* Automatically include the appropriate platform implementation based on
 * compile-time configuration. This allows .c files to simply include port_heap.h
 * without needing to know which platform is being used.
 * 
 * Note: In test environment, test/unit/mock/port_heap.h will be found first
 * (due to include path order), which includes mock_port_heap.h instead.
 */
#if defined(CONFIG_FREERTOS) && (CONFIG_FREERTOS == 1)
#include "port_heap_freertos.h"
#else
#error "Unknown platform"
#endif

#ifdef __cplusplus
}
#endif

#endif /* PORT_HEAP_H */

