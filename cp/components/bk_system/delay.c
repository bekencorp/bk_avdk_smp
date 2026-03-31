#include <common/bk_include.h>
#include "bk_arm_arch.h"

#include "bk_misc.h"
#include "bk_sys_ctrl.h"
#include <components/system.h>
#include "sys_driver.h"
#include "arch_delay.h"
#include "timer_driver.h"

void delay(INT32 num)
{
	volatile INT32 i, j;

	for (i = 0; i < num; i ++) {
		for (j = 0; j < 100; j ++)
			;
	}
}

__IRAM_SEC void bk_delay_us(UINT32 us)
{
#if CONFIG_TIMER_US
	bk_timer_delay_us(us);
#else
	arch_delay_us(us);
#endif
}

void delay_ms(UINT32 ms)
{
#if 0//CONFIG_TIMER_US bk7259 v2 bringup,to do
	bk_timer_delay_us(1000 * ms);
#else
	arch_delay_us(1000 * ms);
#endif
}

void delay_tick(UINT32 tick_count)
{
	UINT32 t0;
	UINT32 t1;

	t0 = bk_get_tick();
	while (1) {
		t1 = bk_get_tick();
		if (t1 - t0 >= 1)
			break;
	}
}
