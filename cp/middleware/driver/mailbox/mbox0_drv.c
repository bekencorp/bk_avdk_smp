#include <common/bk_include.h>
#include "cmsis_gcc.h"
#include "sdkconfig.h"
#include "cpu_id.h"
#include "mbox0_drv.h"
#include "sys_driver.h"
#include "mbox0_fifo_cfg.h"

static mbox0_dev_t       mbox0_dev;
static u8                mbox0_init = 0;

static const mbox0_fifo_cfg_t  fifo_cfg[] = MBOX0_FIFO_CFG_TABLE;

static int mbox0_drv_recieve_message(mbox0_message_t* message)
{
	return mbox0_dev.chn_drv[SELF_CHNL]->chn_recv(&mbox0_dev.hal, message);
}

/* GCC 14+ requires compilation with general-regs-only for interrupt handlers
 * when FPU is enabled; the function attribute alone does not satisfy -Werror.
 */
#pragma GCC push_options
#pragma GCC target("general-regs-only")

static void __BK_IRQ mbox0_drv_isr_handler(void)
{
	uint32_t int_status;
	uint32_t fifo_status;
	mbox0_message_t message;

	int_status = mbox0_dev.chn_drv[SELF_CHNL]->chn_get_int_status(&mbox0_dev.hal);

	if(int_status == 0)
		return;

	do
	{
		fifo_status = mbox0_dev.chn_drv[SELF_CHNL]->chn_get_rx_fifo_stat(&mbox0_dev.hal);
		
		if(fifo_status & RX_FIFO_STAT_NOT_EMPTY)
		{
			mbox0_drv_recieve_message(&message);
			__DMB();

			if(message.data[1] != 0)  /* message data len is not 0. */
			{
				if(mbox0_dev.rx_callback != NULL)
					mbox0_dev.rx_callback(&message);
			}
			else
			{
				#if CONFIG_SOC_SMP
				extern void crosscore_smp_cmd_handler(uint8_t src_core, uint32_t cmd);
				
				crosscore_smp_cmd_handler(message.src_cpu, message.data[0]);
				#endif
			}
		}
		else
		{
			break;
		}
	} while(1);
	__DSB();
	
}

#pragma GCC pop_options

int mbox0_drv_get_send_stat(uint32_t dest_cpu, uint32_t *fifo_status)
{
	if((dest_cpu >= MBOX_CHNL_NUM) || (fifo_status == NULL))
	{
		return MBOX0_HAL_SW_PARAM_ERR;
	}

	*fifo_status = mbox0_dev.chn_drv[dest_cpu]->chn_get_rx_fifo_stat(&mbox0_dev.hal);

	return MBOX0_HAL_OK;
}

int mbox0_drv_send_message(mbox0_message_t* message)
{
	return mbox0_dev.chn_drv[SELF_CHNL]->chn_send(&mbox0_dev.hal, message);
}

int mbox0_drv_callback_register(mbox0_rx_callback_t callback)
{
	mbox0_dev.rx_callback = callback;

	return 0;
}

int mbox0_drv_core_int_enable(uint32_t core_id, uint32_t enable)
{
	if ((mbox0_init == 0) || (core_id >= MBOX_CHNL_NUM) || (mbox0_dev.chn_drv[core_id] == NULL))
		return MBOX0_HAL_SW_PARAM_ERR;

	mbox0_dev.chn_drv[core_id]->chn_int_enable(&mbox0_dev.hal, enable ? 1 : 0);
	sys_drv_set_int_en(core_id, INT_SRC_MAILBOX, enable ? 1 : 0);

	return 0;
}

int mbox0_init_on_current_core(int id)
{
	int int_src = INT_SRC_MAILBOX;

	if(CPU0_CORE_ID == id)
	{
		mbox0_hal_dev_init(&mbox0_dev.hal);  /* can only be initialized by CPU0. */
		mbox0_hal_set_reg_0x2_chn_pro_disable(&mbox0_dev.hal, 1); /* channel unprotect */

		for(int i = 0; i < ARRAY_SIZE(fifo_cfg); i++)
			mbox0_dev.chn_drv[i]->chn_cfg_fifo(&mbox0_dev.hal, fifo_cfg[i].start, fifo_cfg[i].len);
	}

	mbox0_dev.chn_drv[id]->chn_int_enable(&mbox0_dev.hal, 1);

	/* clear/empty RX FIFO. */
	while(1)
	{
		mbox0_message_t message;

		uint32_t fifo_status = mbox0_dev.chn_drv[id]->chn_get_rx_fifo_stat(&mbox0_dev.hal);
		
		if(fifo_status & RX_FIFO_STAT_NOT_EMPTY)
		{
			mbox0_dev.chn_drv[id]->chn_recv(&mbox0_dev.hal, &message);
		}
		else
		{
			break;
		}
	}

	/* enable system interrupt, every channel is hard-bound to a processor */
	sys_drv_set_int_en(id, int_src, 1);

	return 0;
}

int mbox0_drv_init(void)
{
	if(mbox0_init == 1)
		return 0;

	mbox0_dev.chn_drv[0] = &hal_chn0_drv;
	mbox0_dev.chn_drv[1] = &hal_chn1_drv;
	mbox0_dev.chn_drv[2] = &hal_chn2_drv;
#if (SOC_MAILBOX_CHAN_NUM > 3)
	mbox0_dev.chn_drv[3] = &hal_chn3_drv;
#endif

	mbox0_dev.rx_callback = NULL;

	mbox0_hal_init(&mbox0_dev.hal);

	int int_src = INT_SRC_MAILBOX;

	bk_int_isr_register(int_src, mbox0_drv_isr_handler, NULL);

#if CONFIG_SOC_SMP
	mbox0_init_on_current_core(CPU0_CORE_ID);
	mbox0_init_on_current_core(CPU1_CORE_ID);
#else
	mbox0_init_on_current_core(SELF_CHNL);
#endif

	mbox0_init = 1;

	return 0;
}

int mbox0_drv_deinit(void)
{
	if(mbox0_init == 0)
		return 0;

	uint8_t int_src = INT_SRC_MAILBOX;


#if CONFIG_SOC_SMP
	sys_drv_set_int_en(CPU0_CORE_ID, int_src, 0);
	sys_drv_set_int_en(CPU1_CORE_ID, int_src, 0);
	mbox0_dev.chn_drv[CPU0_CORE_ID]->chn_int_enable(&mbox0_dev.hal, 0);
	mbox0_dev.chn_drv[CPU1_CORE_ID]->chn_int_enable(&mbox0_dev.hal, 0);
#else
	sys_drv_set_int_en(rtos_get_core_id(), int_src, 0);
	mbox0_dev.chn_drv[SELF_CHNL]->chn_int_enable(&mbox0_dev.hal, 0);
#endif

	bk_int_isr_unregister(int_src);

#if CONFIG_SOC_SMP
	mbox0_hal_dev_deinit(&mbox0_dev.hal);  /* can only be de-initialized by CPU0. */
#else
	if(SELF_CHNL == 0)
		mbox0_hal_dev_deinit(&mbox0_dev.hal);  /* can only be de-initialized by CPU0. */
#endif

	mbox0_hal_deinit(&mbox0_dev.hal);
	
	mbox0_dev.rx_callback = NULL;

	mbox0_init = 0;

	return 0;
}

