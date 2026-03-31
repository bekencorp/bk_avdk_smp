#if CONFIG_SOC_SMP
#include "os/os.h"
#include "spinlock.h"
#include "FreeRTOS.h"

static SPINLOCK_SECTION spinlock_t s_spinlock_heap = SPIN_LOCK_ACQUIRE_INIT;

void port_heap_enter_critical(void)
{
    vPortEnterCritical(&s_spinlock_heap);
}

void port_heap_exit_critical(void)
{
    vPortExitCritical(&s_spinlock_heap);
}
#endif