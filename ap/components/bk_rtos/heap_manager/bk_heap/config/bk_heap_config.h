/*
 * bk_heap_config.h - Heap management configuration header file
 *
 * This file contains all configurable macro definitions used to control
 * the behavior of the heap management system.
 */

#ifndef BK_HEAP_CONFIG_H
#define BK_HEAP_CONFIG_H

#include <stddef.h>
#include <stdint.h>

/* ============================================================================
 * Region Count Configuration
 * ============================================================================ */

/* Maximum number of heap regions (default: 4) */
#ifndef BK_HEAP_MAX_REGION
#define BK_HEAP_MAX_REGION        (4)
#endif

/* ============================================================================
 * Memory Alignment Configuration
 * ============================================================================ */

/* Memory alignment in bytes (fixed at 8 bytes) */
#define BK_HEAP_BYTE_ALIGNMENT        (8)

/* Memory alignment mask */
#define BK_HEAP_BYTE_ALIGNMENT_MASK   (0x0007)


/* ============================================================================
 * Feature Switch Configuration
 * ============================================================================ */

/* Enable statistics (enabled by default) */
#ifndef BK_HEAP_ENABLE_STATS
#define BK_HEAP_ENABLE_STATS          (1)
#endif

/* ============================================================================
 * Helper Macro Definitions
 * ============================================================================ */

/* Align address up to 8-byte boundary */
#define BK_HEAP_ALIGN_UP(addr) \
    (((size_t)(addr) + (BK_HEAP_BYTE_ALIGNMENT - 1)) & ~(BK_HEAP_BYTE_ALIGNMENT_MASK))

/* Align address down to 8-byte boundary */
#define BK_HEAP_ALIGN_DOWN(addr) \
    ((size_t)(addr) & ~(BK_HEAP_BYTE_ALIGNMENT_MASK))

/* Check if address is aligned */
#define BK_HEAP_IS_ALIGNED(addr) \
    (((size_t)(addr) & BK_HEAP_BYTE_ALIGNMENT_MASK) == 0)

#endif /* BK_HEAP_CONFIG_H */

