// Dual-interpreter (person + pet) parallel inference demo.
// See TFLM_多解释器多线程并行_设计文档.md for the full design.

#pragma once

// NOTE: only the self-contained, extern-"C"-guarded bk_err.h is included here.
// os/os.h has NO internal extern "C" guards, so it must never be pulled into a
// C++ TU through this header: the demo .cpp calls RTOS APIs (rtos_core*_create_
// psram_thread etc.) directly and would get C++-mangled references at link time.
#include <common/bk_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize both interpreters (person + pet) and the shared NPU.
 *
 * Must be called exactly once from the single-threaded phase (main) before
 * tflm_multi_demo_start(). Performs: Ethos-U driver init (global, once),
 * model copies (16-byte aligned), per-instance arena allocation, resolver
 * registration and AllocateTensors() for both models.
 *
 * @return bk_err_t BK_OK on success
 */
bk_err_t tflm_multi_demo_init(void);

/**
 * @brief Create the two core-pinned inference tasks.
 *
 * Task "tflm_person" runs on AP core0 (CPU2), task "tflm_pet" on AP core1
 * (CPU3), both with PSRAM stacks. Call after successful tflm_multi_demo_init().
 *
 * @return bk_err_t BK_OK on success
 */
bk_err_t tflm_multi_demo_start(void);

#ifdef __cplusplus
}
#endif
