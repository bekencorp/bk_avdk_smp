/* Optional crash observers. Default SDK builds provide no implementation.
 * Early observers must not allocate or block. Persist is called with local
 * IRQs disabled after the native exception coordination/prologue. */
#ifndef BK_FAULT_OBSERVER_H
#define BK_FAULT_OBSERVER_H
#include "stack_base.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Return zero to skip early interrupted-stack inspection. */
int bk_fault_context_enter(uint32_t reason, uint32_t exc_return, uint32_t stack);
void bk_fault_context_observe(uint32_t reason, const SAVED_CONTEXT *context);
void bk_fault_context_persist(uint32_t reason, const SAVED_CONTEXT *context);
#ifdef __cplusplus
}
#endif
#endif
