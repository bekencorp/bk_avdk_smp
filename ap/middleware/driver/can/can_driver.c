// Copyright 2023-2024 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/os.h>
#include <os/mem.h>
#include <driver/gpio.h>
#include <driver/int.h>
#include <driver/can.h>
#include <driver/can_types.h>
#include "clock_driver.h"
#include "gpio_driver.h"
#include "power_driver.h"
#include "can_statis.h"
#include "can_driver.h"
#include "can_hal.h"
#include "sys_driver.h"
#include "interrupt.h"
#include "FreeRTOS.h"
#include "task.h"

#if CONFIG_CAN_PM_CB_SUPPORT
#include <modules/pm.h>
#endif

#define CAN_ERR_RECOVER_STACK_SIZE      1024

#define CAN_RETURN_ON_DEVICE_NOT_INIT() do { \
	if (!s_can_hw_is_init) { \
			CAN_LOGE("can hw not init\r\n"); \
			return BK_ERR_CAN_NOT_INIT; \
		} \
	} while(0)

#define DEFAULT_FIFO_SIZE          (1 << 8)
#define CAN_SPINLOCK_TIMEOUT_MS 20

static can_env_t *s_can_env;
static can_dev_t s_can_dev;
static bool s_can_driver_is_init = false;
static bool s_can_hw_is_init = false;
static can_callback_des_t s_can_isr_user_rx_cb;
static can_callback_des_t s_can_isr_user_tx_cb;
static can_callback_des_t s_can_isr_user_err_cb;
static volatile uint32_t s_can_err_pending;
static beken_semaphore_t s_can_err_sem;
static beken_thread_t s_can_err_thread;

static bk_err_t can_err_recover_init(void);
static void can_err_recover_deinit(void);
static void can_apply_protocol(can_protocol_e protocol);
static void can_err_int(void *param);
static void can_fill_default_config(can_dev_t *dev);
#if (CONFIG_CAN_PM_CB_SUPPORT)
static int bk_can_backup(uint64_t sleep_time_ms, void *args);
static int bk_can_restore(uint64_t sleep_time_ms, void *args);
#endif
#if CONFIG_USR_GPIO_CFG_EN
#define CAN_SET_PIN(chn) do { \
	if ((chn) == CAN_CHAN_0) { \
		BK_LOG_ON_ERR(gpio_dev_map_by_func(GPIO_DEV_CAN_TX)); \
		BK_LOG_ON_ERR(gpio_dev_map_by_func(GPIO_DEV_CAN_RX)); \
		BK_LOG_ON_ERR(gpio_dev_map_by_func(GPIO_DEV_CAN_STANDBY)); \
	} \
} while (0)

#define CAN_UNSET_PIN(chn) do { \
	if ((chn) == CAN_CHAN_0) { \
		BK_LOG_ON_ERR(gpio_dev_unmap_by_func(GPIO_DEV_CAN_TX)); \
		BK_LOG_ON_ERR(gpio_dev_unmap_by_func(GPIO_DEV_CAN_RX)); \
		BK_LOG_ON_ERR(gpio_dev_unmap_by_func(GPIO_DEV_CAN_STANDBY)); \
	} \
} while (0)
#endif

static void can_init_gpio(can_channel_t chn)
{
	if (chn >= CAN_CHAN_MAX || chn < CAN_CHAN_0) {
		CAN_LOGV("unsupported can chnnal\r\n");
		return;
	}
#if CONFIG_USR_GPIO_CFG_EN
	CAN_SET_PIN(chn);
#endif
}

static void can_deinit_gpio(can_channel_t chn)
{
	if (chn >= CAN_CHAN_MAX || chn < CAN_CHAN_0) {
		CAN_LOGV("unsupported can chnnal\r\n");
		return;
	}
#if CONFIG_USR_GPIO_CFG_EN
	CAN_UNSET_PIN(chn);
#endif
}

bk_err_t bk_can_clock_enable(void)
{
	/* Program the cksel mux to match the HAL's currently selected source clock
	 * (picked per requested bit rate). Also keeps the PM restore path correct,
	 * since bk_can_restore reuses this to re-power the controller. */
	uint32_t cksel = (can_hal_get_clk_hz() == CAN_CLK_HZ_120M)
		? CKSEL_SYS_XTAL_120M_120M : CKSEL_SYS_XTAL_120M_XTAL;
	sys_hal_can0_cksel_set(cksel);
	bk_pm_clock_ctrl(CLK_PWR_ID_CAN0, CLK_PWR_CTRL_PWR_UP);

	return BK_OK;
}

bk_err_t bk_can_clock_disable(void)
{
	bk_pm_clock_ctrl(CLK_PWR_ID_CAN0, CLK_PWR_CTRL_PWR_DOWN);

	return BK_OK;
}

static void bk_can_base_init(void)
{
    bk_can_clock_enable();

    can_init_gpio(CAN_CHAN_0);
}

static void bk_can_base_deinit(void)
{
    can_deinit_gpio(CAN_CHAN_0);

    bk_can_clock_disable();
}

static void can_rx_cb(void *param)
{
    rtos_set_semaphore(&(s_can_env->rx_semphr));
}

static void can_tx_cb(void *param)
{
    rtos_set_semaphore(&(s_can_env->tx_semphr));
}

static uint32_t can_tx_fifo_get(uint8_t *buf, uint32_t len)
{
    uint32_t size = 0;

    if (s_can_env->can_f.tx) {
        size = kfifo_get(s_can_env->can_f.tx, buf, len);
    }

    return size;
}

static uint32_t can_tx_fifo_put(uint8_t *buf, uint32_t len)
{
    uint32_t size = 0;

    if (s_can_env->can_f.tx) {
        size = kfifo_put(s_can_env->can_f.tx, buf, len);
    }

    return size;
}

static uint32_t can_tx_size_get(void)
{
    uint32_t size = 0;

    if (s_can_env->can_f.tx) {
        size = kfifo_data_size(s_can_env->can_f.tx);
    }

    return size;
}

static void can_tx_fifo_clr(void)
{
    if (s_can_env->can_f.tx) {
        // kfifo_clear(s_can_env->can_f.tx);
        s_can_env->can_f.tx->in = 0;
        s_can_env->can_f.tx->out = 0;
    }
}

static uint32_t can_rx_fifo_get(uint8_t *buf, uint32_t len)
{
    uint32_t size = 0;
    uint32_t flag;
	flag = rtos_disable_int();
	spinlock_acquire(&s_can_env->rx_spin, CAN_SPINLOCK_TIMEOUT_MS);
    if (s_can_env->can_f.rx) {
        size = kfifo_get(s_can_env->can_f.rx, buf, len);
    }

	spinlock_release(&s_can_env->rx_spin, flag);
	rtos_enable_int(flag);
    return size;
}

static uint32_t can_rx_fifo_put(uint8_t *buf, uint32_t len)
{
    uint32_t size = 0;
    uint32_t flag;
	flag = rtos_disable_int();
	spinlock_acquire(&s_can_env->rx_spin, CAN_SPINLOCK_TIMEOUT_MS);

    if (s_can_env->can_f.rx) {
        size = kfifo_put(s_can_env->can_f.rx, buf, len);
    }

	spinlock_release(&s_can_env->rx_spin, flag);
	rtos_enable_int(flag);
    return size;
}

static uint32_t can_rx_size_get(void)
{
    uint32_t size = 0;
    uint32_t flag;
	flag = rtos_disable_int();
	spinlock_acquire(&s_can_env->rx_spin, CAN_SPINLOCK_TIMEOUT_MS);

    if (s_can_env->can_f.rx) {
        size = kfifo_data_size(s_can_env->can_f.rx);
    }

	spinlock_release(&s_can_env->rx_spin, flag);
	rtos_enable_int(flag);

    return size;
}

bk_err_t bk_can_receive(uint8_t *data, uint32_t expect_size, uint32_t *recv_size, uint32_t timeout)
{
    uint32_t rx_size = 0, read_size = 0;
    bk_err_t ret = BK_OK;

    if (data == NULL || 0 == expect_size || NULL == s_can_env || NULL == s_can_env->can_f.rx) {
        return BK_ERR_PARAM;
    }

    CAN_RETURN_ON_DEVICE_NOT_INIT();

    if (recv_size) {
        *recv_size = 0;
    }

    while (1) {
        rx_size = can_rx_size_get();

        read_size = min(rx_size, expect_size);
        if (read_size) {
            can_rx_fifo_get(data, read_size);

            expect_size -= read_size;
            data += read_size;
            if (recv_size) {
                *recv_size += read_size;
            }
        }

        if (expect_size) {
            if (rtos_get_semaphore(&(s_can_env->rx_semphr), timeout) != BK_OK) {
                ret = BK_ERR_TIMEOUT;
                break;
            }
        } else {
            ret = BK_OK;
            break;
        }
    }

    return ret;
}

bk_err_t bk_can_send_ptb(can_frame_s* frame)
{
    CAN_RETURN_ON_DEVICE_NOT_INIT();
    if (frame == NULL) {
        return BK_ERR_NULL_PARAM;
    }
    if ((frame->tag.fdf == CAN_PROTO_20) && (frame->size > 8)) {
        return BK_ERR_PARAM;
    }

    if ((frame->tag.fdf == CAN_PROTO_20) && (frame->tag.brs == CAN_BIT_RATE_FAST)) {
        return BK_ERR_PARAM;
    }

    if ((frame->tag.fdf == CAN_PROTO_FD) && ((frame->size > 64) || ((frame->size > 8) && (frame->size%4)))) {
        return BK_ERR_PARAM;
    }

    if ((frame->tag.fdf == CAN_PROTO_FD) && (frame->tag.rtr == CAN_FRAME_REMT)) {
        return BK_ERR_PARAM;
    }

    can_hal_ctrl(CMD_CAN_PTB_INBUF, frame);

    uint32_t param = CAN_TPE;
    can_hal_ctrl(CMD_CAN_TRANS_SWITCH, (void *)param);

    return BK_OK;
}

bk_err_t bk_can_abort_ptb(void)
{
    uint32_t param = CAN_TPA;

    CAN_RETURN_ON_DEVICE_NOT_INIT();
    can_hal_ctrl(CMD_CAN_TRANS_SWITCH, (void *)param);

    return BK_OK;
}

bk_err_t bk_can_send(can_frame_s* frame, uint32_t timeout)
{
    uint32_t t_size = 0;
    uint32_t param = 0;
    uint32_t flag;

	CAN_RETURN_ON_DEVICE_NOT_INIT();
    if (frame == NULL || frame->data == NULL) {
        return BK_ERR_NULL_PARAM;
    }
    if (s_can_env == NULL || s_can_env->can_f.tx == NULL) {
        return BK_ERR_NULL_PARAM;
    }

    uint8_t *data = frame->data;
    uint32_t size = frame->size;
    can_frame_tag_t tag = frame->tag;
    can_hal_ctrl(CMD_CAN_SET_TX_FRAME_TAG, &tag);

    while (size) {
        flag = rtos_disable_int();
        spinlock_acquire(&s_can_env->tx_spin, CAN_SPINLOCK_TIMEOUT_MS);
        t_size = kfifo_unused(s_can_env->can_f.tx);
        if (t_size > 0) {
            if (t_size >= size) {
                t_size = size;
            }
            can_tx_fifo_put(data, t_size);
            size -= t_size;
            data += t_size;
        }
        spinlock_release(&s_can_env->tx_spin, flag);
        rtos_enable_int(flag);

        if (t_size == 0) {
            can_hal_ctrl(CMD_CAN_STB_INBUF, &param);
            if (rtos_get_semaphore(&(s_can_env->tx_semphr), timeout) != BK_OK) {
                return BK_ERR_TIMEOUT;
            }
        }
    }

    can_hal_ctrl(CMD_CAN_STB_INBUF, &param);
    param = CAN_TSALL;
    can_hal_ctrl(CMD_CAN_TRANS_SWITCH, (void *)param);

    return BK_OK;
}

bk_err_t bk_can_abort_all(void)
{
    CAN_RETURN_ON_DEVICE_NOT_INIT();
    can_tx_fifo_clr();
    uint32_t param = CAN_TPA | CAN_TSA;
    can_hal_ctrl(CMD_CAN_TRANS_SWITCH, (void *)param);

    return BK_OK;
}

bk_err_t bk_can_set_loopback_internal(bool enable)
{
    CAN_RETURN_ON_DEVICE_NOT_INIT();
    can_hal_set_lbmi(enable ? 1 : 0);

    return BK_OK;
}

bk_err_t bk_can_set_loopback_external(bool enable)
{
    CAN_RETURN_ON_DEVICE_NOT_INIT();
    can_hal_set_lbme(enable ? 1 : 0);
    /* External loopback puts the frame on the real bus; a single node has no
     * peer to acknowledge it, so enable self-ACK to let the node ACK its own
     * frames. Cleared on exit to keep normal bus operation ACK-correct. */
    can_hal_set_sack(enable ? 1 : 0);

    return BK_OK;
}

bk_err_t bk_can_acc_filter_set(can_acc_filter_cmd_s* cmd)
{
	CAN_RETURN_ON_DEVICE_NOT_INIT();
    can_hal_ctrl(CMD_CAN_RESET_REQ, (void *)CAN_RESET_REQ_EN);

    can_hal_ctrl(CMD_CAN_ACC_FILTER_SET, (void *)cmd);

    can_hal_ctrl(CMD_CAN_RESET_REQ, (void *)CAN_RESET_REQ_NO);

    return BK_OK;
}



static bk_err_t bk_can_busoff_clr(void)
{
    CAN_RETURN_ON_DEVICE_NOT_INIT();
    can_hal_ctrl(CMD_CAN_BUSOFF_CLR, NULL);

    return BK_OK;
}

static void can_err_recover_task(void *arg)
{
	(void)arg;

	while (1) {
		rtos_get_semaphore(&s_can_err_sem, BEKEN_WAIT_FOREVER);
		if (s_can_err_pending & CAN_ERRINT_WARN_LIM) {
			bk_can_abort_all();
			bk_can_busoff_clr();
		}
		s_can_err_pending = 0;
	}
}

static bk_err_t can_err_recover_init(void)
{
	bk_err_t ret;

	ret = rtos_init_semaphore(&s_can_err_sem, 1);
	if (ret != BK_OK) {
		return ret;
	}

	ret = rtos_create_thread(&s_can_err_thread, BEKEN_DEFAULT_WORKER_PRIORITY,
		"can_err", can_err_recover_task, CAN_ERR_RECOVER_STACK_SIZE, NULL);
	if (ret != BK_OK) {
		rtos_deinit_semaphore(&s_can_err_sem);
	}

	return ret;
}

static void can_err_recover_deinit(void)
{
	if (s_can_err_thread) {
		rtos_delete_thread(&s_can_err_thread);
		s_can_err_thread = NULL;
	}
	if (s_can_err_sem) {
		rtos_deinit_semaphore(&s_can_err_sem);
		s_can_err_sem = NULL;
	}
	s_can_err_pending = 0;
}

static void can_fill_default_config(can_dev_t *dev)
{
	if (dev == NULL) {
		return;
	}
	dev->config.protocol = CAN_PROTO_FD;
	dev->config.s_speed = CAN_BR_1M;
	dev->config.f_speed = CAN_BR_2M;
	dev->config.rx_size = DEFAULT_FIFO_SIZE;
	dev->config.tx_size = DEFAULT_FIFO_SIZE;
	dev->err_cb.cb = can_err_int;
	dev->err_cb.param = NULL;
}

static void can_apply_protocol(can_protocol_e protocol)
{
	if (protocol == CAN_PROTO_FD) {
		can_hal_set_fd_enable(1);
	} else {
		can_hal_set_fd_enable(0);
	}
}

static can_koer_code_e bk_can_get_koer(void)
{
    can_koer_code_e koer_c = CAN_KOER_NO;

    if (!s_can_hw_is_init) {
        return CAN_KOER_NO;
    }
    can_hal_ctrl(CMD_CAN_GET_KOER, &koer_c);

    return koer_c;
}

void bk_can_register_err_callback(can_callback_des_t *err_cb)
{
	if(err_cb) {
		s_can_isr_user_err_cb.cb = err_cb->cb;
		s_can_isr_user_err_cb.param = err_cb->param;
	}
}

static void can_err_int(void *param)
{
    uint32_t err_code = (uint32_t)param;
    /* KOER (kind of error) pinpoints the cause: 4=ACK (no peer/listen-only or
     * bit-rate not synced), 5=CRC / 2=FORM / 3=STUFF (FD/ISO mismatch or bit
     * timing), 1=BIT. Helps triage real-bus errors that only show as BUS ERROR. */
    CAN_LOGD("%s,%d err code 0x%x koer 0x%x\r\n", __func__, __LINE__, err_code, bk_can_get_koer());

    if(s_can_isr_user_err_cb.cb) {
        s_can_isr_user_err_cb.cb(s_can_isr_user_err_cb.param);
    }

    if (err_code & CAN_ERRINT_WARN_LIM) {
        s_can_err_pending |= err_code;
        rtos_set_semaphore(&s_can_err_sem);
    }
}

void bk_can_register_isr_callback(can_callback_des_t *rx_cb, can_callback_des_t *tx_cb)
{
	if(rx_cb) {
		s_can_isr_user_rx_cb.cb = rx_cb->cb;
		s_can_isr_user_rx_cb.param = rx_cb->param;
	}

	if(tx_cb) {
		s_can_isr_user_tx_cb.cb = tx_cb->cb;
		s_can_isr_user_tx_cb.param = tx_cb->param;
	}
}

void can_isr(void)
{
    uint32_t intc_stat;
    uint32_t err_c = 0;
    __attribute__((__unused__)) can_statis_t *can_statis = can_statis_get_statis();

    intc_stat = can_hal_get_ie_value();
    CAN_LOGV(" %s, %d intc_stat 0x%x\r\n\r\n", __func__, __LINE__, intc_stat);
    CAN_STATIS_INC(can_statis->isr_cnt);

    if (intc_stat & RX_INT_FLAG_GROUP) {
        CAN_LOGV("RECV message\r\n");
        can_hal_receive_frame();
        CAN_LOGV("%s,%d\r\n", __func__, __LINE__);
        CAN_STATIS_INC(can_statis->rx_cnt);
        if(s_can_isr_user_rx_cb.cb) {
            s_can_isr_user_rx_cb.cb(s_can_isr_user_rx_cb.param);
        }
    }

    if (intc_stat & TX_INT_FLAG_GROUP) {
        CAN_LOGV("SEND message\r\n");
        can_hal_send_frame();
        CAN_STATIS_INC(can_statis->tx_cnt);
        if(s_can_isr_user_tx_cb.cb){
            s_can_isr_user_tx_cb.cb(s_can_isr_user_tx_cb.param);
        }
    }

    if (intc_stat & ERR_INT_FLAG_GROUP) {
        if (intc_stat & (1 << CAN_IE_BEIF_POS)) {
            CAN_LOGD(" BUS ERROR\r\n\r\n");
            err_c |= CAN_ERRINT_BUS;
            CAN_STATIS_INC(can_statis->beif_cnt);
        }

        if (intc_stat & (1 << CAN_IE_ALIF_POS)) {
            CAN_LOGD(" Arbitration Lost\r\n\r\n");
            err_c |= CAN_ERRINT_ARB_LOST;
            CAN_STATIS_INC(can_statis->alif_cnt);
        }

        if (intc_stat & (1 << CAN_IE_EPIF_POS)) {
            CAN_LOGD(" Error Passive\r\n\r\n");
            err_c |= CAN_ERRINT_PASSIVE;
            CAN_STATIS_INC(can_statis->epif_cnt);
        }

        if (intc_stat & (1 << CAN_IE_EWARN_POS)) {
            CAN_LOGD(" Error Warning Limit\r\n\r\n");
            err_c |= CAN_ERRINT_WARN_LIM;
            CAN_STATIS_INC(can_statis->ewarn_cnt);
        }

        can_hal_error_analysis(err_c);
    }

    if (intc_stat & (1 << CAN_IE_AIF_POS)) {
        CAN_LOGD("Abort Handled \r\n\r\n");
        CAN_STATIS_INC(can_statis->aif_cnt);
    }

	can_hal_set_ie_value(intc_stat);
}

/* Read back the live bit-timing registers and report the *actual* on-wire bit
 * rate and sample point, so a test operator can confirm the controller really
 * runs at the nominal rate (e.g. that "1M" is 1.000M, not 967.7k) instead of
 * trusting the requested enum. Computed from real registers, not the table.
 * The Tq base clock is the currently selected source (26M XTAL or 120M PLL,
 * picked per bit rate), read from the HAL so it always matches the live cksel. */
void bk_can_dump_bit_rate(void)
{
    uint32_t clk_hz = can_hal_get_clk_hz();
    uint32_t s_seg1 = can_hal_get_sseg1();
    uint32_t s_seg2 = can_hal_get_sseg2();
    uint32_t s_presc = can_hal_get_spresc();
    uint32_t f_seg1 = can_hal_get_fseg1();
    uint32_t f_seg2 = can_hal_get_fseg2();
    uint32_t f_presc = can_hal_get_fpresc();

    uint32_t s_tq = s_seg1 + s_seg2 + 2;
    uint32_t f_tq = f_seg1 + f_seg2 + 2;
    uint32_t s_bps = clk_hz / ((s_presc + 1) * s_tq);
    uint32_t f_bps = clk_hz / ((f_presc + 1) * f_tq);
    /* sample point in 0.1% units: (seg1+1)/total_tq */
    uint32_t s_sp = (s_seg1 + 1) * 1000 / s_tq;
    uint32_t f_sp = (f_seg1 + 1) * 1000 / f_tq;

    CAN_LOGI("bit rate: arb=%u bps (sp %u.%u%%), data=%u bps (sp %u.%u%%) sspoff=%u tdcen=%u iso=%u\r\n",
             s_bps, s_sp / 10, s_sp % 10, f_bps, f_sp / 10, f_sp % 10,
             can_hal_get_sspoff(), can_hal_get_tdcen(), can_hal_get_fd_iso());
}

/* Switch ISO vs non-ISO (BOSCH) CAN-FD at runtime so a bench operator can match
 * the analyzer. The fd_iso bit lives in the cfg register; write it in reset. */
void bk_can_set_iso(uint32_t iso)
{
    can_hal_set_reset(1);
    can_hal_set_fd_iso(iso ? 1 : 0);
    can_hal_set_reset(0);
    bk_can_dump_bit_rate();
}

/* Live override of the FD data-phase secondary sample point (SSP), in fast-phase
 * Tq, for bench-tuning 4M/5M BRS against a real transceiver without a rebuild.
 * Must be called after the bit rate is configured (config recomputes SSPOFF). */
void bk_can_set_ssp(uint32_t sspoff_tq)
{
    /* SSPOFF/TDCEN live in the CAP register, which only latches writes while the
     * controller is held in reset (same as can_driver_bit_rate_config); writing
     * it in normal mode is silently ignored. */
    can_hal_set_reset(1);
    can_hal_set_sspoff(sspoff_tq);
    can_hal_set_tdcen(1);
    can_hal_set_reset(0);
    bk_can_dump_bit_rate();
}

bk_err_t can_driver_bit_rate_config(can_bit_rate_e s_speed, can_bit_rate_e f_speed)
{
    if (s_speed < CAN_BR_250K || s_speed > CAN_BR_5M || f_speed < CAN_BR_250K || f_speed > CAN_BR_5M) {
        CAN_LOGE("beyond configurable range!!!\r\n");
        return BK_ERR_PARAM;
    }
    can_hal_set_reset(1);
    /* can_hal_bit_rate_config picks the source clock for these rates and loads the
     * matching table; re-program the cksel mux to match while still in reset so a
     * runtime rate change that flips 26M<->120M doesn't glitch a running clock. */
    can_hal_bit_rate_config(s_speed, f_speed);
    bk_can_clock_enable();
    can_hal_set_reset(0);

    bk_can_dump_bit_rate();

    return BK_OK;
}

bk_err_t bk_can_init(can_dev_t *can)
{
    bk_err_t ret = BK_OK;
    can_dev_t default_dev;

    if (s_can_hw_is_init) {
        return BK_OK;
    }

    /* A NULL config means "bring up with the driver's built-in defaults", so the
     * internal default error callback (can_err_int) stays inside the driver and
     * callers/CLI can enable the hardware with a single argument-less call. */
    if (can == NULL) {
        can_fill_default_config(&default_dev);
        can = &default_dev;
    }

    if (can->config.s_speed > CAN_BR_1M) {
        return BK_ERR_PARAM;
    }

    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_AHBP_CAN, PM_POWER_MODULE_STATE_ON);

    if (s_can_env == NULL) {
        s_can_env = os_zalloc(sizeof(can_env_t));
        if(!s_can_env) {
            CAN_LOGE("%s,%d s_can_env malloc fail\r\n", __func__, __LINE__);
            bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_AHBP_CAN, PM_POWER_MODULE_STATE_OFF);
            return BK_ERR_CAN_CHK_ERROR;
        }
    }

    /* Pre-select the source clock from the requested rates so bk_can_base_init
     * powers up with the right cksel mux, before the controller leaves reset. */
    can_hal_select_clk(can->config.s_speed, can->config.f_speed);
    bk_can_base_init();
    ret = can_err_recover_init();
    if (ret != BK_OK) {
        bk_can_base_deinit();
        bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_AHBP_CAN, PM_POWER_MODULE_STATE_OFF);
        return ret;
    }
    rtos_init_semaphore(&(s_can_env->rx_semphr), 1);
    rtos_init_semaphore(&(s_can_env->tx_semphr), 1);
    spinlock_init(&s_can_env->rx_spin);
    spinlock_init(&s_can_env->tx_spin);

    can_speed_t can_speed;
    can_speed.s_speed = can->config.s_speed;
    can_speed.f_speed = can->config.f_speed;
    can_hal_ctrl(CMD_CAN_MODUILE_INIT, &can_speed);
    can_apply_protocol(can->config.protocol);
    can_hal_int_enable();

    s_can_env->status = CAN_STATUS_IDLE;

    if (can->config.rx_size > 0) {
        if (s_can_env->can_f.rx) {
            kfifo_free(s_can_env->can_f.rx);
        }
        s_can_env->can_f.rx = kfifo_alloc(can->config.rx_size);
        can_hal_ctrl(CMD_CAN_SET_RX_FIFO, can_rx_fifo_put);
    }

    if (can->config.tx_size > 0) {
        if (s_can_env->can_f.tx) {
            kfifo_free(s_can_env->can_f.tx);
        }
        s_can_env->can_f.tx = kfifo_alloc(can->config.tx_size);
        can_hal_ctrl(CMD_CAN_SET_TX_FIFO, can_tx_fifo_get);
        can_hal_ctrl(CMD_CAN_GET_TX_SIZE, can_tx_size_get);
    }

    if (can->err_cb.cb != NULL) {
        can_hal_ctrl(CMD_CAN_SET_ERR_CALLBACK, &can->err_cb);
    }

    can_callback_des_t reg_cb;

    reg_cb.cb = can_rx_cb;
    reg_cb.param = NULL;
    can_hal_ctrl(CMD_CAN_SET_RX_CALLBACK, &reg_cb);

    reg_cb.cb = can_tx_cb;
    reg_cb.param = NULL;
    can_hal_ctrl(CMD_CAN_SET_TX_CALLBACK, &reg_cb);

    bk_interrupt_register_m55sub_int(INT_SRC_CP_CAN, can_isr);

    s_can_hw_is_init = true;

#if (CONFIG_CAN_PM_CB_SUPPORT)
    pm_cb_conf_t enter_config = {bk_can_backup, NULL};
    pm_cb_conf_t exit_config = {bk_can_restore, NULL};
    bk_pm_sleep_register_cb(PM_MODE_LOW_VOLTAGE, PM_DEV_ID_CAN, &enter_config, &exit_config);
#endif

    return BK_OK;
}

bk_err_t bk_can_deinit(void)
{
    if (!s_can_hw_is_init) {
        return BK_OK;
    }
    s_can_hw_is_init = false;
#if (CONFIG_CAN_PM_CB_SUPPORT)
    bk_pm_sleep_unregister_cb(PM_MODE_LOW_VOLTAGE, PM_DEV_ID_CAN, true, true);
#endif
    can_err_recover_deinit();
    can_hal_int_disable();
    bk_interrupt_unregister_m55sub_int(INT_SRC_CP_CAN);
    bk_can_base_deinit();

    if (s_can_env != NULL) {
        rtos_deinit_semaphore(&(s_can_env->rx_semphr));
        rtos_deinit_semaphore(&(s_can_env->tx_semphr));

        if (s_can_env->can_f.rx) {
            kfifo_free(s_can_env->can_f.rx);
            s_can_env->can_f.rx = NULL;
        }

        if (s_can_env->can_f.tx) {
            kfifo_free(s_can_env->can_f.tx);
            s_can_env->can_f.tx = NULL;
        }
        os_free(s_can_env);
        s_can_env = NULL;
    }

    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_AHBP_CAN, PM_POWER_MODULE_STATE_OFF);

    return BK_OK;
}

#if (CONFIG_CAN_PM_CB_SUPPORT)
static uint32_t s_can_pm_backup[16];
static int bk_can_backup(uint64_t sleep_time_ms, void *args)
{
	CAN_RETURN_ON_DEVICE_NOT_INIT();

	s_can_pm_backup[0] = can_hal_get_fd_enable();
	s_can_pm_backup[1] = can_hal_get_tid_esi_value();
	s_can_pm_backup[2] = can_hal_get_tbuf_ctrl_value();
	s_can_pm_backup[3] = can_hal_get_cfg_value();
	s_can_pm_backup[4] = can_hal_get_ie_value();
	s_can_pm_backup[5] = can_hal_get_sseg_value();
	s_can_pm_backup[6] = can_hal_get_fseg_value();
	s_can_pm_backup[7] = can_hal_get_cap_value();
	s_can_pm_backup[8] = can_hal_get_acf_value();
	s_can_pm_backup[9] = can_hal_get_aid_value();
	s_can_pm_backup[10] = can_hal_get_ttcfg_value();
	s_can_pm_backup[11] = can_hal_get_ref_msg_value();
	s_can_pm_backup[12] = can_hal_get_trig_cfg_value();
	s_can_pm_backup[13] = can_hal_get_mem_stat_value();
	s_can_pm_backup[14] = can_hal_get_mem_es_value();
	s_can_pm_backup[15] = can_hal_get_scfg_value();

	bk_can_clock_disable();

	return BK_OK;
}

static int bk_can_restore(uint64_t sleep_time_ms, void *args)
{
	CAN_RETURN_ON_DEVICE_NOT_INIT();

	bk_can_clock_enable();
	can_hal_set_fd_enable(s_can_pm_backup[0]);
	can_hal_set_tid_esi_value(s_can_pm_backup[1]);
	can_hal_set_tbuf_ctrl_value(s_can_pm_backup[2]);
	can_hal_set_cfg_value(s_can_pm_backup[3]);
	can_hal_set_ie_value(s_can_pm_backup[4]);
	can_hal_set_sseg_value(s_can_pm_backup[5]);
	can_hal_set_fseg_value(s_can_pm_backup[6]);
	can_hal_set_cap_value(s_can_pm_backup[7]);
	can_hal_set_acf_value(s_can_pm_backup[8]);
	can_hal_set_aid_value(s_can_pm_backup[9]);
	can_hal_set_ttcfg_value(s_can_pm_backup[10]);
	can_hal_set_ref_msg_value(s_can_pm_backup[11]);
	can_hal_set_trig_cfg_value(s_can_pm_backup[12]);
	can_hal_set_mem_stat_value(s_can_pm_backup[13]);
	can_hal_set_mem_es_value(s_can_pm_backup[14]);
	can_hal_set_scfg_value(s_can_pm_backup[15]);

	return BK_OK;
}
#endif

/* Software-layer load only: fill the default config and register CLI, with no
 * hardware side effect (no power-up, no GPIO map, no interrupt). This runs at
 * boot under CONFIG_CAN so merely enabling the macro does NOT bring the CAN
 * controller live. The hardware is enabled later, on demand, by bk_can_init(). */
bk_err_t bk_can_driver_init(void)
{
	if (s_can_driver_is_init) {
		return BK_OK;
	}

	can_fill_default_config(&s_can_dev);

#if CONFIG_CAN_TEST
    int bk_can_register_cli_test_feature(void);
    bk_can_register_cli_test_feature();
#endif

#if CONFIG_CAN_DEMO
    int bk_can_register_cli_demo(void);
    bk_can_register_cli_demo();
#endif

	s_can_driver_is_init = true;

	return BK_OK;
}

bk_err_t bk_can_driver_deinit(void)
{
	if (!s_can_driver_is_init) {
		return BK_OK;
	}

	/* Tear the hardware down first if some caller left it enabled, so unloading
	 * the driver never leaves the controller powered/interrupting. */
	if (s_can_hw_is_init) {
		bk_can_deinit();
	}

	s_can_driver_is_init = false;

	return BK_OK;
}