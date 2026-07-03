// Copyright 2020-2025 Beken
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

#include <stdbool.h>
#include <string.h>
#include "common/bk_err.h"
#include "components/system.h"
#include "lw_mac802154_interface.h"
#include "mac802154_adapter.h"

//#include "driver/aon_rtc.h"
#include "bk_802154.h"
#include "assert.h"
#include "bk_ieee802154_ack.h"

#include "driver/hal/hal_timer_types.h"
#include "driver/timer_types.h"
#include "os/os.h"
#include "os/mem.h"
#include "os/str.h"
#include "sys_driver.h"
#include "bk_rf_internal.h"

#include "bk_openthread_coex.h"
#include "sys_ll.h"

#define MAC802154_TAG "OT"
#define MAC802154_LOGI(...) BK_LOGI(MAC802154_TAG, ##__VA_ARGS__)
#define MAC802154_LOGW(...) BK_LOGW(MAC802154_TAG, ##__VA_ARGS__)
#define MAC802154_LOGE(...) BK_LOGE(MAC802154_TAG, ##__VA_ARGS__)
#define MAC802154_LOGD(...) BK_LOGD(MAC802154_TAG, ##__VA_ARGS__)

/* Thread packet debug print, disabled by default */
#define BK_THREAD_RX_PKT_PRINT_EN    0
#define BK_THREAD_TX_PKT_PRINT_EN    0
#define BK_THREAD_RX_ABORT_PRINT_EN  1

#if BK_THREAD_RX_PKT_PRINT_EN
#define BK_THREAD_RX_PKT_PRINT(...) MAC802154_LOGI(__VA_ARGS__)
#else
#define BK_THREAD_RX_PKT_PRINT(...)
#endif

#if BK_THREAD_TX_PKT_PRINT_EN
#define BK_THREAD_TX_PKT_PRINT(...) MAC802154_LOGI(__VA_ARGS__)
#else
#define BK_THREAD_TX_PKT_PRINT(...)
#endif

#if BK_THREAD_RX_ABORT_PRINT_EN
#define BK_THREAD_RX_ABORT_PRINT(...) MAC802154_LOGI(__VA_ARGS__)
#else
#define BK_THREAD_RX_ABORT_PRINT(...)
#endif

#define BK_SELF_TEST_ENABLE 0

#define RF_COEX_TRIPLE_EN   0
#if RF_COEX_TRIPLE_EN
#define MAC802154_COEX_PRINT    MAC802154_LOGE
#else
#define MAC802154_COEX_PRINT    MAC802154_LOGD
#endif
#define BK_ED_DURATION                      8      //number of symbol duration

#define BK_FRAME_BUFFER_MAX_SIZE            8 //Must be 2^n

#define MSG_QUEUE_SIZE                      16

#define BK_MAC_MIN_BE                       3
#define BK_MAC_MAX_BE                       5
#define BK_MAC_MAX_CSMA_BACKOFFS            4

#define ACK_TIMER_ID                        3

beken_queue_t g_802154_msg_queue;

enum {
    TIMER_NO_PENDING = 0,
    TIMER_TX_PENDING = 1,
    TIMER_RX_PENDING = 2,
    TIMER_TX_TIMEOUT = 3,
};


static bk_802154_frame_t s_frame_buf[BK_FRAME_BUFFER_MAX_SIZE] = {0};

typedef struct {
    bk_802154_frame_t rx_frame[BK_FRAME_BUFFER_MAX_SIZE];
    volatile uint32_t head;
    volatile uint32_t tail;
} rx_pkt_ringbuffer;

static bool s_rx_when_idle = false;
static uint8_t s_timer_pending = TIMER_NO_PENDING;
volatile bk_802154_state_t s_802154_state = BK_802154_STATE_IDLE;
static int8_t s_recent_rssi = 0;
static bool s_cca_channel_clear = false;
static bk_802154_frame_t s_ack_frame;
static uint8_t s_pending_tx_frame[128]; //Transmitting
static uint8_t s_delay_send = 0; //cached tx frame
static uint32_t bk_s_tx_no_rcv_cnt = 0;
beken2_timer_t frame_protect_tmr = {0};
static volatile bool s_frame_protect_in_progress = false;

static rx_pkt_ringbuffer g_rx_buffer;
static bk_802154_tx_err_t s_tx_err;
static bk_802154_frame_t* rx_pkt_with_ack = NULL;

#if BK_THREAD_RX_PKT_PRINT_EN
static void bk_thread_pkt_print_rx(const uint8_t *mac, uint8_t mac_len, int8_t rssi, uint8_t lqi)
{
    uint8_t sn = (mac_len >= 3) ? mac[2] : 0;
    BK_THREAD_RX_PKT_PRINT("[RX] len:%u sn:%u rssi:%d lqi:%u\n", mac_len, sn, rssi, lqi);
}
#else
#define bk_thread_pkt_print_rx(mac, mac_len, rssi, lqi)
#endif

#if BK_THREAD_TX_PKT_PRINT_EN
static void bk_thread_pkt_print_tx(const uint8_t *frame)
{
    BK_THREAD_TX_PKT_PRINT("[TX] len:%u sn:%u ack_req:%u\n",
                           frame[0],
                           (frame[0] >= 4) ? frame[3] : 0,
                           (frame[0] >= 2) ? ((frame[1] >> 5) & 1) : 0);
}
#else
#define bk_thread_pkt_print_tx(frame)
#endif

#if BK_THREAD_RX_ABORT_PRINT_EN
static const char *bk_thread_rx_abort_reason_str(uint16_t reason)
{
    switch (reason & 0x000F) {
    case BK_802154_RX_ABORT_SFD_TIMEOUT:     return "sfd_timeout";
    case BK_802154_RX_ABORT_CRC_ERR:         return "crc_err";
    case BK_802154_RX_ABORT_INVALID_LEN:     return "invalid_len";
    case BK_802154_RX_ABORT_ACK_TIMEOUT:     return "ack_timeout";
    case BK_802154_RX_ABORT_ACK_SN_MISMATCH: return "ack_sn_mismatch";
    case BK_802154_RX_ABORT_ED_STOP_CMD:     return "ed_stop";
    case BK_802154_RX_ABORT_RX_STOP_CMD:     return "rx_stop";
    case BK_802154_RX_ABORT_HW_FILTER_FAIL:  return "hw_filter_fail";
    case BK_802154_RX_ABORT_PHY_NO_ED:       return "phy_no_ed";
    default:                                 return "unknown";
    }
}

static void bk_thread_rx_abort_print(uint16_t status, const char *from)
{
    BK_THREAD_RX_ABORT_PRINT("[RX ABORT] %s status:0x%04x reason:%s state:%d\n",
                             from, status, bk_thread_rx_abort_reason_str(status), s_802154_state);
}
#else
#define bk_thread_rx_abort_print(status, from)
#endif

extern uint64_t bk_aon_rtc_get_us(void);
extern bk_err_t bk_timer_stop(timer_id_t timer_id);
extern bk_err_t bk_timer_delay_with_callback(timer_id_t timer_id, uint64_t time_us, timer_isr_t callback);
extern uint64_t otPlatTimeGet(void);

#define ATOMIC_STATE(s) \
    ((s)==BK_802154_STATE_RECEIVE_BUSY || \
     (s)==BK_802154_STATE_TRANSMIT || \
     (s)==BK_802154_STATE_TRANSMIT_CCA || \
     (s)==BK_802154_STATE_TRANSMIT_IMM_ACK || \
     (s)==BK_802154_STATE_TRANSMIT_ENH_ACK)

#if 0
static __attribute__((optimize("O0"))) bk_802154_frame_t* bk_ieee802154_get_rx_write_buffer()
{
    bk_802154_frame_t* buf;
    uint32_t next_head = (g_rx_buffer.head + 1) & (BK_FRAME_BUFFER_MAX_SIZE - 1);
    if (next_head == g_rx_buffer.tail)
    {
        bk_802154_log("rx buffer is full!\r\n");
        g_rx_buffer.head = next_head;
    }
    bk_802154_log("write buf idx:%d\r\n", g_rx_buffer.head);
    buf = &g_rx_buffer.rx_frame[g_rx_buffer.head];
    g_rx_buffer.head = next_head;
    return buf;
}

__attribute__((optimize("O0"))) bk_802154_frame_t* bk_ieee802154_get_rx_read_buffer()
{
    bk_802154_frame_t* buf;
    uint32_t next_tail = g_rx_buffer.tail;
    if (g_rx_buffer.head == next_tail)
    {
        bk_802154_log("rx buffer is empty!\r\n");
        return NULL;
    }

    bk_802154_log("read buf idx:%d\r\n", next_tail);
    buf = &g_rx_buffer.rx_frame[next_tail];
    g_rx_buffer.tail = (next_tail + 1) & (BK_FRAME_BUFFER_MAX_SIZE - 1);
    return buf;
}
#endif

#define ED_SCAN_DURATION_MIN (200)
#define ED_SCAN_RSSI_INVALID (99)
bool g_ckeck_thread_rf_leave_flag = false;
bool g_thread_ed_scan_start_enable = false;
uint32_t g_thread_ed_scan_duration_store;
uint8_t g_thread_ed_scan_channel_store;
uint64_t g_thread_ed_scan_start_time_store;
int8_t g_thread_ed_scan_rssi_max = ED_SCAN_RSSI_INVALID;

void bk_ieee802154_thread_ed_scan_info(uint32_t duration,uint8_t channel)
{
    g_thread_ed_scan_start_enable = true;
    g_thread_ed_scan_duration_store = duration;
    g_thread_ed_scan_channel_store = channel;
    g_thread_ed_scan_start_time_store = otPlatTimeGet();
    g_thread_ed_scan_rssi_max = ED_SCAN_RSSI_INVALID;
    MAC802154_LOGD("[%s], energy detect duration %d channel %d\r\n",__func__,duration,channel);
}
void bk_ieee802154_check_ed_scan_stop(void)
{
    if (g_thread_ed_scan_start_enable && !g_ckeck_thread_rf_leave_flag)
    {
        lw_mac802154_thread_ed_scan_stop();		
        int8_t rssi = lw_mac802154_ll_handle_get_ed_scan_rssi();
        if(g_thread_ed_scan_rssi_max == ED_SCAN_RSSI_INVALID)
        {
            g_thread_ed_scan_rssi_max = rssi;
        }
        else
        {
            if(rssi > g_thread_ed_scan_rssi_max)
            {
                g_thread_ed_scan_rssi_max = rssi;
            }
        }
        MAC802154_LOGD("ed rssi %d (max %d)\r\n",rssi,g_thread_ed_scan_rssi_max);
    }
    g_ckeck_thread_rf_leave_flag = true;
}
bool bk_ieee802154_check_ed_scan_start(void)
{
	bool ret = true;
    if (g_thread_ed_scan_start_enable)
	{
        uint32_t duration = g_thread_ed_scan_duration_store;
		uint64_t current_time = otPlatTimeGet();
        if(current_time > g_thread_ed_scan_start_time_store)
		{
			uint64_t diff = (current_time - g_thread_ed_scan_start_time_store)/16;
			if(diff < g_thread_ed_scan_duration_store)
			{
				duration = g_thread_ed_scan_duration_store - diff;
			}
            else
            {
                duration = (uint32_t)ED_SCAN_DURATION_MIN;
                MAC802154_LOGD("ed (d %d ,diff %d)\r\n",g_thread_ed_scan_duration_store,(uint32_t)diff);
            }
		}
        else
        {
            duration = (uint32_t)ED_SCAN_DURATION_MIN;
            MAC802154_LOGD("ed (d %d ,t %d)\r\n",g_thread_ed_scan_duration_store,current_time);
        }

		bk_ieee802154_channel_set(g_thread_ed_scan_channel_store);
		lw_mac802154_lw_macl_start_ed_pl(duration, 0);
		s_802154_state = BK_802154_STATE_ED;
		ret = false;
	}
    g_ckeck_thread_rf_leave_flag = false;
	return ret;
}
int8_t bk_802154_rssi_convert(int8_t rssi)
{
    int8_t rssi_val = rssi;
    if (rssi < LW_MAC_RSSI_LOWER_LIMIT) {
        rssi_val = LW_MAC_RSSI_LOWER_LIMIT;
    } else if (rssi > LW_MAC_RSSI_UPPER_LIMIT) {
        rssi_val = LW_MAC_RSSI_UPPER_LIMIT;
    }
    return rssi_val;
}
static RF_PLL_CTRL_RESULT_T bk_ieee802154_thread_rf_apply_impl(uint8_t rf_prio, uint8_t task_type,
                                                               uint32_t dur_us, bool adjust,
                                                               const char *caller)
{
    RF_PLL_CTRL_RESULT_T rlt;

    rlt = rf_pll_ctrl(MODULE_TYPE_THREAD, RF_OPERATION_APPLY,
        RF_PATH_THREAD_IQ, RF_PLL_LOW, rf_prio, task_type, true, dur_us, adjust);
    if(rlt.result != RF_ARBIT_RESULT_SUCCESS)
    {
        MAC802154_COEX_PRINT("[Error]%s failed to apply rf\n", caller);
    }
    else
        MAC802154_COEX_PRINT("[Succ]%s apply type:%d prio:%d\n", caller, task_type, rf_prio);

    return rlt;
}
#define bk_ieee802154_thread_rf_apply(rf_prio, task_type, dur_us, adjust) \
    bk_ieee802154_thread_rf_apply_impl((rf_prio), (task_type), (dur_us), (adjust), __func__)

void bk_ieee802154_thread_rf_free_impl(uint8_t rf_prio, uint8_t task_type, bool adjust, const char *caller)
{
    RF_PLL_CTRL_RESULT_T  rlt = rf_pll_ctrl(MODULE_TYPE_THREAD, RF_OPERATION_FREE,
        RF_PATH_THREAD_IQ, RF_PLL_LOW, rf_prio, task_type, true, 0, adjust);
    if(rlt.result != RF_ARBIT_RESULT_SUCCESS)
    {
        if (task_type == RF_TASK_TYPE_THREAD_FRAME_PROTECT)
            MAC802154_LOGE("[Error]%s failed to free frame protect rf\n", caller);
        else
            MAC802154_COEX_PRINT("[Error]%s failed to free rf\n", caller);
    }
    else
        MAC802154_COEX_PRINT("[Succ]%s free type:%d prio:%d\n", caller, task_type, rf_prio);

    if ((rlt.result == RF_ARBIT_RESULT_SUCCESS) &&
        s_frame_protect_in_progress &&
        (rf_prio == RF_PRIORITY_THREAD_HIGH) &&
        (task_type == RF_TASK_TYPE_THREAD_FREE_ALL))
    {
        (void)bk_ieee802154_thread_rf_apply_impl(RF_PRIORITY_THREAD_HIGH, RF_TASK_TYPE_THREAD_FRAME_PROTECT, 20000, true, caller);
    }
}

static bool bk_ieee802154_frame_protect_notify_ot_thread(uint8_t action, uint32_t dur_ms)
{
    bk_err_t ret;
    bk_802154_msg_t data_msg = {0};

    data_msg.msg_type = BK_802154_MSG_FRAME_PROTECT;
    data_msg.msg.frame_protect.action = action;
    data_msg.msg.frame_protect.dur_ms = dur_ms;
    ret = rtos_push_to_queue(&g_802154_msg_queue, &data_msg, 0);
    if (BK_OK != ret) {
        MAC802154_LOGE("[%s] notify failed\n", __func__);
        return false;
    }

    return true;
}

static void bk_ieee802154_frame_protect_defer_to_ot_thread(void)
{
    if (!s_frame_protect_in_progress)
    {
        return;
    }

    bk_ieee802154_frame_protect_notify_ot_thread(BK_802154_FRAME_PROTECT_DONE, 0);
}

static bool bk_ieee802154_frame_start_protect_timer_on_ot_thread(uint32_t dur_ms)
{
    bk_err_t err = kNoErr;

    if (frame_protect_tmr.handle != NULL)
    {
        return true;
    }

    err = rtos_init_oneshot_timer(&frame_protect_tmr,
                                dur_ms,
                                (timer_2handler_t)bk_ieee802154_frame_protect_defer_to_ot_thread,
                                NULL,
                                NULL);

    if (kNoErr == err)
    {
        err = rtos_start_oneshot_timer(&frame_protect_tmr);
        if (kNoErr != err) {
            rtos_deinit_oneshot_timer(&frame_protect_tmr);
            s_frame_protect_in_progress = false;
            MAC802154_LOGE("[%s] failed to start frame protect timer\n", __func__);
            return false;
        }
        else {
            RF_PLL_CTRL_RESULT_T rlt;

            rlt = bk_ieee802154_thread_rf_apply(RF_PRIORITY_THREAD_HIGH, RF_TASK_TYPE_THREAD_FRAME_PROTECT, dur_ms * 1000, true);
            if (rlt.result != RF_ARBIT_RESULT_SUCCESS)
            {
                rtos_stop_oneshot_timer(&frame_protect_tmr);
                rtos_deinit_oneshot_timer(&frame_protect_tmr);
                return false;
            }
            s_frame_protect_in_progress = true;
            return true;
        }
    }
    else {
        MAC802154_LOGE("[%s] failed to init frame protect timer\n", __func__);
    }
    return false;
}

void bk_ieee802154_frame_protect_handle_msg(uint8_t action, uint32_t dur_ms)
{
    if (action == BK_802154_FRAME_PROTECT_START)
    {
        if (!bk_ieee802154_frame_start_protect_timer_on_ot_thread(dur_ms))
        {
            s_frame_protect_in_progress = false;
        }
        return;
    }

    if (action == BK_802154_FRAME_PROTECT_DONE)
    {
        if (frame_protect_tmr.handle == NULL)
        {
            MAC802154_LOGE("[%s] frame protect timer already deinit\n", __func__);
            return;
        }

        rtos_stop_oneshot_timer(&frame_protect_tmr);
        rtos_deinit_oneshot_timer(&frame_protect_tmr);
        s_frame_protect_in_progress = false;
        bk_ieee802154_thread_rf_free(RF_PRIORITY_THREAD_HIGH, RF_TASK_TYPE_THREAD_FRAME_PROTECT, false);
        return;
    }

    MAC802154_LOGE("[%s] unknown frame protect action:%d\n", __func__, action);
}

bool bk_ieee802154_frame_start_protect_timer(uint32_t dur_ms)
{
    if (s_frame_protect_in_progress)
    {
        return true;
    }

    if (!bk_ieee802154_frame_protect_notify_ot_thread(BK_802154_FRAME_PROTECT_START, dur_ms))
    {
        return false;
    }

    return true;
}

bk_802154_frame_t* bk_ieee802154_get_ack_buffer()
{
    return &s_ack_frame;
}

uint8_t bk_ieee802154_check_delayed_send()
{
    return s_delay_send;
}

static void bk_802154_transmit_failed(bk_802154_tx_err_t err)
{
    bk_err_t ret;
    bk_802154_msg_t data_msg = {0};

    data_msg.msg_type = BK_802154_MSG_TX_FAIL;
    ret = rtos_push_to_queue(&g_802154_msg_queue, &data_msg, 0);
    if (BK_OK != ret){
        MAC802154_LOGE("[%s] notify failed\n", __func__);
    }
    s_tx_err = err;
#if CONFIG_OT_TRIP_COEX_EN
    bk_ieee802154_thread_rf_free(bk_ot_get_rf_priority(), bk_ot_get_thread_task_type(), false);
#endif
}

static void bk_802154_transmit_done(bk_802154_frame_t *data_p)
{
    bk_err_t ret;
    bk_802154_msg_t data_msg = {0};

    if (data_p)
    {
        data_msg.msg_type = BK_802154_MSG_TX_DONE_WITH_ACK;
        data_msg.msg.frame = data_p;
    }
    else
    {
        data_msg.msg_type = BK_802154_MSG_TX_DONE_NO_ACK;
    }
    ret = rtos_push_to_queue(&g_802154_msg_queue, &data_msg, 0);
    if (BK_OK != ret)
    {
        MAC802154_LOGE("[%s] notify failed\n", __func__);
    }
#if CONFIG_OT_TRIP_COEX_EN
    bk_ieee802154_thread_rf_free(bk_ot_get_rf_priority(), bk_ot_get_thread_task_type(), false);
#endif
}

static void bk_802154_energy_detected(int8_t result)
{
    bk_err_t ret;

    bk_802154_msg_t data_msg = {0};

    data_msg.msg_type = BK_802154_MSG_ED_DONE;
    ret = rtos_push_to_queue(&g_802154_msg_queue, &data_msg, 0);
    if (BK_OK != ret){
        MAC802154_LOGE("[%s] notify failed\n", __func__);
    }
#if CONFIG_OT_TRIP_COEX_EN
    bk_ieee802154_thread_rf_free(bk_ot_get_rf_priority(), bk_ot_get_thread_task_type(), false);
#endif

}

static void bk_802154_received_done(bk_802154_frame_t *data_p)
{
    bk_err_t ret;

    bk_802154_msg_t data_msg = {0};

    data_msg.msg_type = BK_802154_MSG_RX_DONE;
    data_msg.msg.frame = data_p;
    ret = rtos_push_to_queue(&g_802154_msg_queue, &data_msg, 0);
    if (BK_OK != ret){
        MAC802154_LOGE("[%s] notify failed\n", __func__);
    }
#if CONFIG_OT_TRIP_COEX_EN
    bk_ieee802154_thread_rf_free(bk_ot_get_rf_priority(), bk_ot_get_thread_task_type(), false);
#endif

}

static void bk_802154_receive_failed(bk_802154_rx_err_t error)
{
    bk_err_t ret;

    if (error == BK_802154_RX_ERR_ABORT) {
        bk_thread_rx_abort_print(0, "rx_fail_abort");
    }

    bk_802154_msg_t data_msg = {0};

    data_msg.msg_type = BK_802154_MSG_RX_FAIL;
    ret = rtos_push_to_queue(&g_802154_msg_queue, &data_msg, 0);
    if (BK_OK != ret){
        MAC802154_LOGE("[%s] notify failed\n", __func__);
    }
    s_tx_err = error;

#if CONFIG_OT_TRIP_COEX_EN
    bk_ieee802154_thread_rf_free(bk_ot_get_rf_priority(), bk_ot_get_thread_task_type(), false);
#endif

}

static int8_t find_active_buffer_index(void)
{
    uint8_t index = 0xFF;
    for (uint8_t i = 0; i < BK_FRAME_BUFFER_MAX_SIZE; i++) {
        if (!s_frame_buf[i].used){
            index = i;
            s_frame_buf[index].used = true;
            break;
        }
    }

    if (0xFF == index) {
        MAC802154_LOGE("No available rx buffer found\n");
        return -1;
    }

    return index;
}

static void init_frame_buffer(void)
{
    os_memset(&g_rx_buffer, 0, sizeof(rx_pkt_ringbuffer));
}

static void ieee802154_timer0_start(void)
{
    lw_mac802154_lw_timer0_start();
}

static void ieee802154_timer0_stop(void)
{
    lw_mac802154_lw_timer0_stop();
}

static void ieee802154_timer0_set_max_value(uint32_t value)
{
    lw_mac802154_lw_timer0_set_max_value(value);
}

static void isr_handle_delay_tx(void)
{
    MAC802154_LOGD("handle delay tx\n");
    s_timer_pending = TIMER_NO_PENDING;
    /* If PHY is in atomic state, cache OT request and return success */
    if (ATOMIC_STATE(s_802154_state))
    {
        s_delay_send = 1;
        MAC802154_LOGE("[%s]tx when invalid state(%d), send later",__func__, s_802154_state);
    }
    else
    {
        s_802154_state = BK_802154_STATE_TRANSMIT;
        s_delay_send = 0;
#if CONFIG_OT_TRIP_COEX_EN
    bk_ieee802154_thread_rf_apply(bk_ot_update_rf_priority(RF_PRIORITY_THREAD_HIGH), bk_ot_set_thread_task_type(RF_TASK_TYPE_THREAD_TIMER), 6000, true);
#endif
        bk_thread_pkt_print_tx(s_pending_tx_frame);
        lw_mac802154_lw_macl_tx_frame_pl((s_pending_tx_frame + 1), s_pending_tx_frame[0] - 2, 0);
    }
}

static void isr_handle_delay_rx(void)
{
    MAC802154_LOGD("handle delay rx\n");
    s_timer_pending = TIMER_NO_PENDING;
#if CONFIG_OT_TRIP_COEX_EN
    bk_ieee802154_thread_rf_apply(bk_ot_update_rf_priority(RF_PRIORITY_THREAD_HIGH), bk_ot_set_thread_task_type(RF_TASK_TYPE_THREAD_TIMER), 0, true);
#endif

    lw_mac802154_lw_macl_rx_config_pl(LW_TRUE, 1);
}

static int8_t bk_802154_convert_lqi_to_rssi(uint8_t lqi_val)
{
    int8_t rssi_val;
    if (lqi_val == 0) {
        rssi_val = LW_MAC_RSSI_LOWER_LIMIT;
    } else if (lqi_val == 0xFF) {
        rssi_val = LW_MAC_RSSI_UPPER_LIMIT;
    } else {
        rssi_val = (lqi_val / LW_MAC_RSSI_TO_LQI_OFFSET_MULTIPLIER) - LW_MAC_RSSI_TO_LQI_OFFSET;
    }
    return rssi_val;
}
static uint8_t uHwTimerFlag = 0; // 0: cca 1:tx timeout
static uint8_t uMaclCbEventType = 0;//

static void bk_timer_overflow_cb(timer_id_t id)
{
#if 0
    bk_printf("ack timeout...\n");
    if (s_802154_state == BK_802154_STATE_RECEIVE_ACK) {
       bk_printf("recevive ack time out...\n");
       bk_802154_transmit_failed(s_tx_frame, BK_802154_TX_ERR_NO_ACK);
       s_802154_state = BK_802154_STATE_IDLE;
    }
#endif
    if (s_timer_pending == TIMER_TX_PENDING) {
       isr_handle_delay_tx();
    }
    if (s_timer_pending == TIMER_RX_PENDING) {
       s_timer_pending = TIMER_NO_PENDING;
       isr_handle_delay_rx();
    }
    if(s_timer_pending == TIMER_TX_TIMEOUT)
    {
       s_timer_pending = TIMER_NO_PENDING;
       uHwTimerFlag = 0;
       if(s_802154_state == BK_802154_STATE_TRANSMIT_IMM_ACK)
       {
            if(uMaclCbEventType != MACL_ISR_TX_COMPLETE)
            {//if already enter macl_cb timer_cb is not active
                bk_802154_receive_failed(BK_802154_RX_ERR_ABORT);
                rx_pkt_with_ack->used = false;
                s_802154_state = BK_802154_STATE_IDLE;
            }
       }
       else if(s_802154_state == BK_802154_STATE_TRANSMIT_CCA)
       {
            if((uMaclCbEventType != MACL_ISR_TX_COMPLETE) && (uMaclCbEventType != MACL_ISR_RX_ACK))
            {
                bk_802154_transmit_failed(BK_802154_TX_ERR_ABORT);
                s_802154_state = BK_802154_STATE_IDLE;
            }
        }
    }
    bk_timer_stop(ACK_TIMER_ID);
}

extern void bk_802154_transmit_done_with_ack_rapid(bk_802154_frame_t *data_p);

static API_RESULT macl_cb(uint8_t event_type, uint16_t event_result, void *data, uint16_t datalen)
{
    static uint8_t cca_nb = 0;
    static uint8_t cca_be = BK_MAC_MIN_BE;
    uint32_t cca_random_delay = 0;
    uint8_t *data_buf = (uint8_t *)data;
    static uint64_t rx_sfd_timestamp;
    MAC802154_COEX_PRINT("[macl_cb]:tp:0x%x st:%d\n", event_type, s_802154_state);
    uMaclCbEventType = event_type;//recored event_type

    switch (event_type)
    {
        case MACL_ISR_TX_COMPLETE:
        {
#if CONFIG_OT_TRIP_COEX_EN
                bk_ot_rf_counter_preempted();
                if(uHwTimerFlag)
                {
                    MAC802154_COEX_PRINT("TX_COMPLETE ln:%d\r\n", __LINE__);
                    uHwTimerFlag = 0;
                    if(s_timer_pending == TIMER_TX_TIMEOUT)
                    {
                        s_timer_pending = TIMER_NO_PENDING;
                        // lw_mac802154_lw_macl_stop_hw_timer_pl();
                        bk_timer_stop(ACK_TIMER_ID);
                    }
                }
#endif
            if ((BK_802154_STATE_TRANSMIT_IMM_ACK == s_802154_state) ||
                (BK_802154_STATE_TRANSMIT_ENH_ACK == s_802154_state))
            {
                // event after sending imm-ack & enh-ack
                bk_802154_received_done(rx_pkt_with_ack);
            }
            else if ((BK_802154_STATE_TRANSMIT == s_802154_state) ||
                    (BK_802154_STATE_TRANSMIT_CCA == s_802154_state))
            {
                // event after sending packet without ack
                if ((datalen == 2) && (data_buf[0] == LW_MAC_ENUM_NO_ACK) && (data_buf[1] == 0))
                {
                    bk_802154_transmit_failed(BK_802154_TX_ERR_NO_ACK);
                }
                else
                {
                    //MAC802154_LOGE("frame tx done...\n");
                    bk_802154_transmit_done(NULL);
                }
            }
            s_802154_state = BK_802154_STATE_IDLE;
         }
        break;
        case MACL_ISR_RX_ACK:
        {
#if 0
            int8_t index = find_active_buffer_index();
            if (index < 0)
            {
                MAC802154_LOGE("No rx buffer\r\n");
            }
            bk_802154_frame_t* rx_ack = &s_frame_buf[index];
#endif

            if((BK_802154_STATE_TRANSMIT != s_802154_state) &&
            (BK_802154_STATE_TRANSMIT_CCA != s_802154_state))
                break;
#if 1
            static bk_802154_frame_t s_rx_ack = {0};
            memset(&s_rx_ack, 0, sizeof(bk_802154_frame_t));
            bk_802154_frame_t *rx_ack = &s_rx_ack;
#endif
#if CONFIG_OT_TRIP_COEX_EN
            if(uHwTimerFlag)
            {
                MAC802154_COEX_PRINT("RX_ACK ln:%d\r\n", __LINE__);
                uHwTimerFlag = 0;
                if(s_timer_pending == TIMER_TX_TIMEOUT)
                {
                    s_timer_pending = TIMER_NO_PENDING;
                    // lw_mac802154_lw_macl_stop_hw_timer_pl();
                    bk_timer_stop(ACK_TIMER_ID);
                }
            }
#endif

            rx_ack->frame[0] = datalen + 1;  //add 1 byte for crc, 1 byte is LQI which is already added.
            MAC802154_LOGD(">>>>received rx ack: %d, datalen:%d", data_buf[2], datalen);
            memcpy(&rx_ack->frame[1], data, datalen);
            rx_ack->pending = data_buf[0] & 0x10;
            rx_ack->lqi = data_buf[datalen-1];
            rx_ack->rssi = bk_802154_convert_lqi_to_rssi(data_buf[datalen-1]);
            bk_thread_pkt_print_rx(data, datalen, rx_ack->rssi, rx_ack->lqi);
            //rx_ack->time = otPlatTimeGet() - datalen * 32 - 120;
            rx_ack->time = rx_sfd_timestamp;
            //bk_802154_transmit_done(&s_frame_buf[index]);
            bk_802154_transmit_done_with_ack_rapid(&s_rx_ack);
            s_802154_state = BK_802154_STATE_IDLE;
         }
         break;
        case MACL_ISR_RX_FRAME:
        {
            if (s_802154_state == BK_802154_STATE_TRANSMIT_CCA)
            {
                MAC802154_COEX_PRINT("Tx process forbit rx process\r\n");
                break;
            }
            s_802154_state  = BK_802154_STATE_RECEIVE_BUSY;
            int8_t index = find_active_buffer_index();
            if (index < 0)
            {
                MAC802154_LOGE("No rx buffer\r\n");
                s_802154_state  = BK_802154_STATE_IDLE;
                break;
            }
            bk_s_tx_no_rcv_cnt = 0;
            bk_ieee802154_frame_protect_defer_to_ot_thread();
            bk_802154_frame_t* rx_pkt = &s_frame_buf[index];
            rx_pkt->enh_ack = false;
            rx_pkt->frame[0] = datalen + 1;  //add 1 byte for crc, 1 byte is LQI which is already added.
            memcpy(&rx_pkt->frame[1], data, datalen);
            MAC802154_LOGD("received frame, sn:%d, datalen:%d",rx_pkt->frame[3],datalen);
            //rx_pkt->time = otPlatTimeGet() - datalen * 32 - 120;
            rx_pkt->time = rx_sfd_timestamp;
            rx_pkt->lqi = data_buf[datalen-1];
            rx_pkt->rssi = bk_802154_convert_lqi_to_rssi(data_buf[datalen-1]);
            s_recent_rssi = rx_pkt->rssi;
            MAC802154_LOGD(">>>>>>>>lqi:%d, rssi:%d\n", rx_pkt->lqi, rx_pkt->rssi);
            bk_thread_pkt_print_rx(data, datalen, rx_pkt->rssi, rx_pkt->lqi);
            if (rx_pkt->frame[1] & 0x20) 
            {
                s_802154_state = BK_802154_STATE_TRANSMIT_IMM_ACK;
                rx_pkt->pending = ieee802154_ack_config_pending_bit(data);
                if (ieee802154_frame_get_version(data) < IEEE802154_FRAME_VERSION_2)
                {
                    MAC802154_LOGD("send Imm-ACK");
                    uint8_t * ack_data = bk_ieee802154_imm_ack_generator_create(data, rx_pkt->pending);
                    lw_mac802154_lw_macl_tx_frame_pl(ack_data, IEEE802154_IMM_ACK_LENGTH - 2, 0);
                    {
                        uHwTimerFlag = 1;
                        s_timer_pending = TIMER_TX_TIMEOUT;
                        bk_timer_stop(ACK_TIMER_ID);
                        bk_timer_delay_with_callback(ACK_TIMER_ID, 3000, bk_timer_overflow_cb);
                    }
                }
                else
                {
                    MAC802154_LOGD("send Enh-ACK");
                    s_802154_state = BK_802154_STATE_TRANSMIT_ENH_ACK;
                    lw_mac802154_lw_macl_tx_frame_pl_ack_start_tx((uint8_t*)rx_pkt, rx_pkt->frame[0], 0);
                    extern uint8_t* bk_802154_send_enh_ack(bk_802154_frame_t *data_p);
                    uint8_t * enh_ack = bk_802154_send_enh_ack(rx_pkt);
                    if(enh_ack != NULL)
                    {
                        lw_mac802154_lw_macl_tx_frame_pl_ack_payload(enh_ack + 1, enh_ack[0] - 2, 0);
                        rx_pkt->enh_ack = true;    
                    }
                    else
                    {// This packet is error, discard
                        bk_802154_receive_failed(BK_802154_RX_ERR_MALFORMED);
                        rx_pkt->used = false;
                        break;
                    }
                }
                rx_pkt_with_ack = rx_pkt;
                break;
            }
            else
            {
                bk_802154_received_done(&s_frame_buf[index]);
                s_802154_state = BK_802154_STATE_IDLE;
            }
        }
        break;
        case MACL_ISR_CH_ED:
        {
            MAC802154_LOGD("ed done...\n");
            s_recent_rssi = bk_802154_convert_lqi_to_rssi(*((uint8_t *)data));
            #if CONFIG_OT_TRIP_COEX_EN
            if(g_thread_ed_scan_rssi_max != ED_SCAN_RSSI_INVALID)
            {
                g_thread_ed_scan_rssi_max = bk_802154_rssi_convert(g_thread_ed_scan_rssi_max);
                if(s_recent_rssi < g_thread_ed_scan_rssi_max)
                {
                    s_recent_rssi = g_thread_ed_scan_rssi_max;
                }
                g_thread_ed_scan_rssi_max = ED_SCAN_RSSI_INVALID;
            }
            g_thread_ed_scan_start_enable = false;
            MAC802154_LOGD("ed done... rssi %d\n",(int8_t)s_recent_rssi);
            #endif
            bk_802154_energy_detected(s_recent_rssi);
            s_802154_state = BK_802154_STATE_IDLE;
        }
        break;
        case MACL_ISR_CH_CCA:
        {
            uint8_t channel_state;
            MAC802154_LOGD("cca done...\n");
            if (s_802154_state == BK_802154_STATE_TRANSMIT_CCA)
            {
                channel_state = ((uint8_t *)data)[0];
#if CONFIG_OT_TRIP_COEX_EN
                if (( LW_FALSE == channel_state) && (bk_ot_rf_is_thread()))
#else
                if ( LW_FALSE == channel_state)
#endif
                {
                    MAC802154_COEX_PRINT("channel is free ack:%d\n", (s_pending_tx_frame[1] & 0x20));
                    cca_nb = 0;
                    cca_be = BK_MAC_MIN_BE;
                    {//re-entry
                        bk_timer_stop(ACK_TIMER_ID);
                        uHwTimerFlag = 1;
                        s_timer_pending = TIMER_TX_TIMEOUT;
                        bk_timer_delay_with_callback(ACK_TIMER_ID, 32000, bk_timer_overflow_cb);
                    }
                    bk_thread_pkt_print_tx(s_pending_tx_frame);
                    lw_mac802154_lw_macl_tx_frame_pl((s_pending_tx_frame + 1), s_pending_tx_frame[0] - 2, 0);
                }
                else
                {
                    //bk_802154_transmit_failed(BK_802154_TX_ERR_CCA_BUSY);
                    //break;
                    MAC802154_LOGE("channel is busy\n");
#if 1
                    //TODO back-off is not implemented for now, to do if necessary.
                    s_cca_channel_clear = false;
                    cca_nb += 1;
                    cca_be += 1;
                    if (cca_be > BK_MAC_MAX_BE)
                    {
                        cca_be = BK_MAC_MAX_BE;
                    }
                    if (cca_nb > BK_MAC_MAX_CSMA_BACKOFFS)
                    {
                        MAC802154_LOGE("failed:exceed max number of CSMA retries...\n");
                        cca_nb = 0;
                        cca_be = BK_MAC_MIN_BE;
                        s_802154_state = BK_802154_STATE_IDLE;
                        bk_802154_transmit_failed(BK_802154_TX_ERR_CCA_BUSY);
                    }
                    else
                    {
                        cca_random_delay = lw_mac802154_lw_macl_generate_random_number_pl((1<<cca_be)-1);
                        if(s_timer_pending == TIMER_TX_TIMEOUT)
                        {
                            s_timer_pending = TIMER_NO_PENDING;
                            bk_timer_stop(ACK_TIMER_ID);
                        }
                        uHwTimerFlag = 0;
#if CONFIG_OT_TRIP_COEX_EN
                        uint32_t uTgtMs = 0;
                        uint32_t uTgtUs = 0;
                        uint32_t uDly = cca_random_delay * LW_MAC_A_UNIT_BACKOFF_PERIOD * 16;
                        extern void bk_ot_target_val(uint32_t *tgtMs, uint32_t *tgtUs);
                        bk_ot_target_val(&uTgtMs, &uTgtUs);
                        if((uTgtMs * 1000 < uDly) && (uTgtUs < uDly))
                        {
                            s_802154_state = BK_802154_STATE_IDLE;
                            bk_802154_transmit_failed(BK_802154_TX_ERR_CCA_BUSY);
                            MAC802154_LOGD("[Dbg]cca retry not enough time!!!\n");
                            break;
                        }
#endif
                        lw_mac802154_lw_macl_start_hw_timer_pl(cca_random_delay * LW_MAC_A_UNIT_BACKOFF_PERIOD,
                                                  0x00,
                                                  0x01);
                    }
#endif
                }
            }
        }
        break;
        case MACL_ISR_HW_TIMEOUT:
        {
            MAC802154_LOGE("hw time out...%d Flag:%d\n",s_802154_state, uHwTimerFlag);
            if ((s_802154_state == BK_802154_STATE_TRANSMIT_CCA) && (!s_cca_channel_clear))
            {
                MAC802154_LOGD("cca backoff...\n");
                // if(uHwTimerFlag)
                // {
                //     uHwTimerFlag = 0;
                //     s_802154_state = BK_802154_STATE_IDLE;
                //     bk_802154_transmit_failed(BK_802154_TX_ERR_ABORT);
                // }
                // else
                    lw_mac802154_lw_macl_start_cca_pl (BK_ED_DURATION);
            }
#if 0
            if (s_timer_pending == TIMER_TX_PENDING)
            {
                s_timer_pending = TIMER_NO_PENDING;
                isr_handle_delay_tx();
            }
            if (s_timer_pending == TIMER_RX_PENDING)
            {
                s_timer_pending = TIMER_NO_PENDING;
                isr_handle_delay_rx();
            }
#endif
        }
        break;
        case MACL_ISR_RX_SFD:
        {

            rx_sfd_timestamp = otPlatTimeGet();


        }
        break;
        case MACL_ISR_RX_ABORT:
        {
#if CONFIG_OT_TRIP_COEX_EN
            bk_ot_rf_counter_preempted();
#endif
            uint16_t val = lw_mac802154_read_register_rx_abort_status();
            if (0x0007 != (val & 0x000F))
            {
                bk_thread_rx_abort_print(val, "macl_isr");
                lw_mac802154_lw_macl_rx_config_pl_ext(LW_TRUE);
            }
        }
        break;
        default:
            MAC802154_LOGE("undefined event...\n");
        break;
    }
    uMaclCbEventType = 0; //restore eventtype recorder
    return 0;
}
#if CONFIG_OT_TRIP_COEX_EN
void bk_ieee802154_rf_init(bool enable)
{
    if(enable)
    {
        bk_ieee802154_thread_rf_apply(bk_ot_update_rf_priority(RF_PRIORITY_THREAD_NORMAL), RF_TASK_TYPE_THREAD_INIT, 0, true);
    }
    else
    {
        RF_PLL_CTRL_RESULT_T rlt = rf_pll_ctrl(MODULE_TYPE_THREAD, RF_OPERATION_FREE, RF_PATH_THREAD_IQ, 
            RF_PLL_LOW, RF_PRIORITY_THREAD_NORMAL, RF_TASK_TYPE_THREAD_FREE_ALL, true, 0, true);
        if(rlt.result != RF_ARBIT_RESULT_SUCCESS)
            MAC802154_LOGD("[Error]%s:%d failed to get rf\n", __func__, __LINE__);
        else
            MAC802154_LOGD("[Succ]%s apply type:%d prio:%d\n", __func__, bk_ot_get_thread_task_type(), bk_ot_get_rf_priority());
    
        rlt = rf_pll_ctrl(MODULE_TYPE_THREAD, RF_OPERATION_FREE, RF_PATH_THREAD_IQ, 
            RF_PLL_LOW, RF_PRIORITY_THREAD_HIGH, RF_TASK_TYPE_THREAD_FREE_ALL, true, 0, true);
        if(rlt.result != RF_ARBIT_RESULT_SUCCESS)
            MAC802154_LOGD("[Error]%s:%d failed to get rf\n", __func__, __LINE__);
        else
            MAC802154_LOGD("[Succ]%s apply type:%d prio:%d\n", __func__, bk_ot_get_thread_task_type(), bk_ot_get_rf_priority());
    }
}
#endif
bk_err_t bk_ieee802154_enable(void)
{
    init_frame_buffer();
    rtos_init_queue(&g_802154_msg_queue, "802154 msg queue", sizeof(bk_802154_msg_t), MSG_QUEUE_SIZE);

#if CONFIG_OT_TRIP_COEX_EN
    bk_ieee802154_rf_init(true);
#else
    sys_drv_thread_rf_ctrl(1);
    rf_module_vote_ctrl(1, RF_BY_THREAD_BIT);
#endif

    /* MACL platform init */
    lw_mac802154_lw_macl_init_pl();
    lw_mac802154_set_register_thread_intr_en(0x09FF);
     //by default, it's false because Thread needs to send 802154-2015 Enh-ACK by software.
    lw_mac802154_lw_macl_set_auto_tx_ack(0);

    /* Register ISR */
    lw_mac802154_lw_macl_register_isr_pl(macl_cb);

    s_802154_state = BK_802154_STATE_IDLE;
    return BK_OK;
}

bk_err_t bk_ieee802154_disable(void)
{
    mac802154_mac_deinit();
    s_802154_state = BK_802154_STATE_DISABLE;
    return BK_OK;
}

uint8_t bk_ieee802154_channel_get(void)
{
    uint8_t value = 0;
    lw_mac802154_lw_mac_get_cur_channel_pl(&value);
    value = value/5 + 10;
    return value;
}

bk_err_t bk_ieee802154_channel_set(uint8_t channel)
{
    if (channel == bk_ieee802154_channel_get())
    {
        return BK_OK;
    }
    if ((channel < 11) || (channel > 26))
    {
        MAC802154_LOGE("[Error]%s channel%d out of range\r\n",__func__,channel);
        return BK_ERR_PARAM;
    }
    lw_mac802154_lw_mac_set_channel_pl(channel*5-50);
    return BK_OK;
}

int8_t bk_ieee802154_get_txpower(void)
{
    return BK_OK; //TODO
}

bk_err_t bk_ieee802154_set_txpower(int8_t power)
{
    return BK_OK; //TODO
}

bool bk_ieee802154_get_promiscuous(void)
{
    uint16_t val = 0;
    val = lw_mac802154_le_read_THRAD_CTRL_CONFIG();
    val = val & 0x0080;

    return (val == 0x0080 ? true : false);
}

bk_err_t bk_ieee802154_set_promiscuous(bool enable)
{
    uint16_t val;
    val = lw_mac802154_le_read_THRAD_CTRL_CONFIG();
    if (enable)
    {
        /* Set 7th bit position */
        val = (val | (0x0080));
    }
    else
    {
        /* Reset 7th bit position */
        val = (val & 0xFF7F);
    }

    lw_mac802154_le_write_THREAD_CTRL_CONFIG(val);
    return BK_OK;
}

bk_err_t bk_ieee802154_receive(void)
{
    if (ATOMIC_STATE(s_802154_state))
    {
        return BK_OK;
    }
    int i = 100;
    s_802154_state = BK_802154_STATE_RECEIVE;
    lw_mac802154_thread_ed_scan_stop();
    g_thread_ed_scan_start_enable = false;
    lw_mac802154_thread_tx_stop();
    lw_mac802154_le_write_command_rx_stop();
    while (i > 0)
    {
        i--;
    }
#if CONFIG_OT_TRIP_COEX_EN
    bk_ieee802154_thread_rf_free(RF_PRIORITY_THREAD_HIGH, RF_TASK_TYPE_THREAD_FREE_ALL, true);

    bk_ot_update_rf_lvl();
    bk_ieee802154_thread_rf_apply(bk_ot_update_rf_priority(RF_PRIORITY_THREAD_NORMAL), bk_ot_get_thread_task_type(), 6000, true);

#endif

    lw_mac802154_lw_macl_rx_config_pl(LW_TRUE, 1);
    return BK_OK;
}
#if CONFIG_OT_TRIP_COEX_EN
void bk_ieee802154_rf_apply(void)
{
    bk_s_tx_no_rcv_cnt++;

    bool apply_high = bk_ot_update_rf_lvl();

    if(bk_s_tx_no_rcv_cnt > 2)
    {
        bk_s_tx_no_rcv_cnt = 0;
        if (bk_ieee802154_frame_start_protect_timer(20))
        {
            return;
        }
        apply_high = true;
    }

    if(apply_high)
    {
        bk_ieee802154_thread_rf_apply(bk_ot_update_rf_priority(RF_PRIORITY_THREAD_HIGH), bk_ot_get_thread_task_type(), 6000, true);
    }
    else
    {
        bk_ieee802154_thread_rf_apply(bk_ot_update_rf_priority(RF_PRIORITY_THREAD_NORMAL), bk_ot_get_thread_task_type(), 6000, true);
    }

}
#endif
bk_err_t bk_ieee802154_transmit(const uint8_t *frame, bool cca)
{
    /* If PHY is in atomic state, cache OT request and return success */
    if (ATOMIC_STATE(s_802154_state))
    {
        s_delay_send = 1;
        MAC802154_LOGE("[%s]tx when invalid state(%d), send later",__func__, s_802154_state);
        return BK_OK;
    }
    s_delay_send = 0;
    //cache the transmitting frame, in case failure
    memcpy(s_pending_tx_frame, frame, frame[0] + 1);
#if CONFIG_OT_TRIP_COEX_EN
    bk_ieee802154_rf_apply();
#endif
    lw_mac802154_thread_ed_scan_stop();
    g_thread_ed_scan_start_enable = false;
    lw_mac802154_le_write_command_rx_stop();
    if (cca)
    {
        MAC802154_LOGD("cca mode\n");
        s_802154_state = BK_802154_STATE_TRANSMIT_CCA;
        lw_mac802154_lw_macl_start_cca_pl(BK_ED_DURATION);
    }
    else
    {
        s_802154_state = BK_802154_STATE_TRANSMIT;
        bk_thread_pkt_print_tx(frame);
        lw_mac802154_lw_macl_tx_frame_pl((uint8_t *)frame + 1, frame[0]-2, 1);
    }
    return BK_OK;
}

bk_err_t bk_ieee802154_receive_at(uint32_t time)
{
    s_802154_state = BK_802154_STATE_RECEIVE;
    uint64_t current_time;
    current_time = bk_aon_rtc_get_us();
#if CONFIG_OT_TRIP_COEX_EN
    bk_ot_update_rf_lvl();
#endif
    s_timer_pending = TIMER_RX_PENDING;
    bk_timer_stop(ACK_TIMER_ID);
    bk_timer_delay_with_callback(ACK_TIMER_ID,(time > current_time) ? (time - current_time) : 0, bk_timer_overflow_cb);
#if 0
    ieee802154_timer0_stop();
    ieee802154_timer0_set_max_value((time > current_time) ? (time - current_time) : 0);
    ieee802154_timer0_start();
#endif
    return BK_OK;
}

bk_err_t bk_ieee802154_transmit_at(const uint8_t *frame, bool cca, uint32_t time)
{
    memcpy(s_pending_tx_frame, frame, frame[0] + 1);

    uint64_t current_time;
    //uint32_t fire_time;
    current_time = otPlatTimeGet();
    if (cca) {
        //when the time is over
        //need to wait cca to trigger tx
        MAC802154_LOGD("cca mode\n");
        s_802154_state = BK_802154_STATE_TRANSMIT_CCA;
#if CONFIG_OT_TRIP_COEX_EN
        bk_ieee802154_rf_apply();
#endif
        lw_mac802154_lw_macl_start_cca_pl(BK_ED_DURATION);
    } else {
        //when the time is over
        //set flag
        s_timer_pending = TIMER_TX_PENDING;
        bk_timer_stop(ACK_TIMER_ID);
#if 0
        fire_time = (time > current_time) ? (time - current_time) : 0;
        if (0 == fire_time)
            MAC802154_LOGE("!\n");
#endif
        bk_timer_delay_with_callback(ACK_TIMER_ID, (time > current_time) ? (time - current_time) : 0, bk_timer_overflow_cb);
#if 0
        ieee802154_timer0_stop();
        ieee802154_timer0_set_max_value((time > current_time) ? (time - current_time) : 0);
        ieee802154_timer0_start();
#endif
    }

    return BK_OK;
}

uint16_t bk_ieee802154_get_panid(void)
{
    return lw_mac802154_get_mac_pan_id();
}

bk_err_t bk_ieee802154_set_panid(uint16_t panid)
{
    lw_mac802154_set_mac_pan_id(panid);
    return BK_OK;
}

uint16_t bk_ieee802154_get_short_address(void)
{
    return lw_mac802154_get_short_address();
}

bk_err_t bk_ieee802154_set_short_address(uint16_t short_address)
{
    lw_mac802154_set_short_address(short_address);
    return BK_OK;
}

bk_err_t bk_ieee802154_get_extended_address(uint8_t *ext_addr)
{
    uint16_t temp = 0;
    temp = lw_mac802154_get_mac_extnd_addr0();
    ext_addr[0] = temp & 0x00FF;
    ext_addr[1] = temp >> 8;
    temp = lw_mac802154_get_mac_extnd_addr1();
    ext_addr[2] = temp & 0x00FF;
    ext_addr[3] = temp >> 8;
    temp = lw_mac802154_get_mac_extnd_addr2();
    ext_addr[4] = temp & 0x00FF;
    ext_addr[5] = temp >> 8;
    temp = lw_mac802154_get_mac_extnd_addr3();
    ext_addr[6] = temp & 0x00FF;
    ext_addr[7] = temp >> 8;

    return BK_OK;
}

bk_err_t bk_ieee802154_set_extended_address(const uint8_t *ext_addr)
{
    lw_mac802154_set_mac_extnd_addr0( (ext_addr[0]) | (ext_addr[1] << 8));
    lw_mac802154_set_mac_extnd_addr1( (ext_addr[2]) | (ext_addr[3] << 8));
    lw_mac802154_set_mac_extnd_addr2( (ext_addr[4]) | (ext_addr[5] << 8));
    lw_mac802154_set_mac_extnd_addr3( (ext_addr[6]) | (ext_addr[7] << 8));

    return BK_OK;
}

bk_ieee802154_pending_mode_t bk_ieee802154_get_pending_mode(void)
{
    return ieee802154_get_pending_mode();
}

bk_err_t bk_ieee802154_set_pending_mode(bool pending_mode)
{
    ieee802154_set_pending_mode(pending_mode);
#if 0
    uint16_t value = le_read_register(THREAD_CTRL_CONFIG);
    if(!pending_mode){
        le_write_register(THREAD_CTRL_CONFIG, value | (1 << 10));
    }else{
        le_write_register(THREAD_CTRL_CONFIG, value & (~(1 << 10)));
    }
#endif
    return BK_OK;
}

bk_err_t bk_ieee802154_add_pending_addr(const uint8_t *addr, bool is_short)
{
    return ieee802154_add_pending_addr(addr, is_short);;
}

bk_err_t bk_ieee802154_clear_pending_addr(const uint8_t *addr, bool is_short)
{
    return ieee802154_clear_pending_addr(addr, is_short);;
}

bk_err_t bk_ieee802154_reset_pending_table(bool is_short)
{
    ieee802154_reset_pending_table(is_short);
    return BK_OK;
}

int8_t bk_ieee802154_get_cca_threshold(void)
{
    uint16_t value = lw_mac802154_get_cca_threshold();
    return (int8_t) (value & 0xFF);
}

bk_err_t bk_ieee802154_set_cca_threshold(int8_t cca_threshold)
{
    lw_mac802154_set_cca_threshold( (uint16_t)cca_threshold);
    return BK_OK;
}

bk_err_t bk_ieee802154_set_rx_when_idle(bool enable)
{
    s_rx_when_idle = enable;
    if (enable) {
        lw_mac802154_lw_macl_rx_config_pl_ext(LW_TRUE);
    } else {
        lw_mac802154_lw_macl_rx_config_pl_ext(LW_FALSE);
    }
    return BK_OK;
}

bool bk_ieee802154_get_rx_when_idle(void)
{
    return s_rx_when_idle;
}

bk_err_t bk_ieee802154_energy_detect(uint32_t duration)
{
    lw_mac802154_lw_macl_start_ed_pl(duration, 0);
    s_802154_state = BK_802154_STATE_ED;
    return BK_OK;
}

bk_802154_state_t bk_ieee802154_state_get(void)
{
    return s_802154_state;
}

void bk_ieee802154_energy_detect_done(int8_t power)
{}

int8_t bk_ieee802154_get_recent_rssi(void)
{
    return s_recent_rssi;
}

uint8_t bk_ieee802154_get_recent_lqi(void)
{
    return lw_mac802154_lw_mac_convert_rssi_to_lqi(bk_ieee802154_get_recent_rssi());
}

bk_802154_rx_err_t bk_ieee802154_get_tx_err()
{
    return s_tx_err;
}

#if !CONFIG_CLI
int bk_cli_init(void)
{
    MAC802154_LOGD("openthread not use default cli\n");
    return 0;
}
#endif


#if 0
void bk_ieee802154_transmit_security_config(uint8_t *frame, uint8_t *key, uint8_t *addr)
{
}
#endif

#if BK_SELF_TEST_ENABLE
//self test
//
//as test code
static void lp_init(void)
{
    // BK7236_TRX_REG.REG0x13->bits.lpfouttsten = 1;
    // BK7236_TRX_REG.REG0x13->bits.entst = 1;
    volatile uint32_t *trx = (volatile uint32_t*)(0x4980c24c);
    *trx |= ((1 <<0 ) | (1 << 4));

    //config p21/p22 9 function
    uint32_t temp = *(volatile uint32_t*)(0x44010000 + 0x32 * 4);
    temp &= ~(0xF << 24 | 0xF << 20);
    temp |= ((9 << 24) | (9 << 20));
    *(volatile uint32_t*)(0x44010000 + 0x32 * 4) = temp;
}

uint8_t mac802154_mac_init(void)
{
    mac802154_mac_enable();

    lp_init();

#ifdef LW_USE_eOSAL
    /* Initialize OSAL */
    EM_timer_init();
    timer_em_init();
#endif /* BT_USE_eOSAL */

    LW_init();

#ifdef LW_SUPPORT_GET_VERSION_INFO
    LW_VERSION_NUMBER  version;
    LW_get_version_number (&version);
    printf (
            "IEEE 802.15.4 Stack Version %03d:%03d:%03d\r\n",
            version.major,version.minor,version.subminor);
#endif /* LW_SUPPORT_GET_VERSION_INFO */

    return LW_TRUE;
}

#define THREAD_TX_LENGTH1           9
#define THREAD_TX_LENGTH2           10
#define THREAD_TX_LENGTH3           11
#define THREAD_ACK_LENGTH           5
#define TX_FRAME_CONTROL_LSB        0x63
#define TX_FRAME_CONTROL_MSB        0x23
#define TX_SEQUENCE_NUMBER          0x12

uint8_t tx_data[THREAD_TX_LENGTH1+1] = {THREAD_TX_LENGTH1,TX_FRAME_CONTROL_LSB,TX_FRAME_CONTROL_MSB,TX_SEQUENCE_NUMBER,0x45,0x14,0xAA,0XBB};



void cli_mac802154(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (strcmp(argv[1], "tx") == 0) {

        //THREAD_CHANNEL REG hold freq offset value.example THREAD_CHANNEL = 0x5 is 2405Mhz
        uint8_t channel = 0x05;
        if (argc > 2) {
            channel = os_strtoul(argv[2], NULL, 10);
        }

        MAC802154_LOGD("tx channel %d\r\n",channel);
        bk_ieee802154_channel_set(channel);
#if 0
        sys_int_isr_register(lw_mac_hw_isr_handler,NULL);
        lw_macl_register_isr_pl(macl_isr);
#endif
        bk_ieee802154_transmit(tx_data, 0);
    } else if (strcmp(argv[1], "rx") == 0) {
        uint8_t channel = 0x05;
        uint8_t rx_mode = 0;
        static uint8_t rx_mode_save = 0;

        if (argc > 2) {
            channel = os_strtoul(argv[2], NULL, 10);
        }

        if (argc > 3) {
            rx_mode = (os_strtoul(argv[3], NULL, 10) > 0) ? 1 : 0;
        }

        MAC802154_LOGD("rx channel %d rx_mode %d\r\n",channel,rx_mode);
#if 0
        sys_int_isr_register(lw_mac_hw_isr_handler,NULL);
        lw_macl_register_isr_pl(macl_isr);
#endif

        if (rx_mode_save == 1) {
            le_write_register(COMMAND_REGISTER, CONT_RX_STOP);
        } else if (rx_mode_save == 2) {
            le_write_register(COMMAND_REGISTER, RX_STOP);
        }

        lw_macl_set_channel(channel);

        thread_hw_filter_disable();

        os_delay_milliseconds(10);

        if (rx_mode) {
            thread_continuous_rx_start();
            rx_mode_save = 1;
        } else {
            thread_rx_start();
            rx_mode_save = 2;
        }
    } else if (strcmp(argv[1], "ed") == 0) {
        uint8_t channel = 5;
        if (argc > 2) {
            channel = os_strtoul(argv[2], NULL, 10);
        }
        lw_macl_set_channel(channel);
        le_write_register(COMMAND_REGISTER, ED_START);
    } else if (strcmp(argv[1], "tx_at") == 0) {
        MAC802154_LOGD("test tramsit at specific time\n");
        uint8_t channel = 0x05;
        uint8_t time = 20;
        if (argc > 2) {
            channel = os_strtoul(argv[2], NULL, 10);
        }

        if (argc > 3) {
            time = os_strtoul(argv[3], NULL, 10);
        }
        MAC802154_LOGD("tx channel %d\r\n",channel);
        bk_ieee802154_channel_set(channel);
        bk_ieee802154_transmit_at(tx_data, 0, time);
    } else if (strcmp(argv[1], "set_get_ch") == 0) {
        MAC802154_LOGD("test set and get channel\n");
        uint8_t channel_set = 6;
        bk_ieee802154_channel_set(channel_set);
        uint8_t channel_get = bk_ieee802154_channel_get();
        if (channel_get == channel_set){
            MAC802154_LOGD(" test pass \n");
        }
    } else if (strcmp(argv[1], "set_get_ch") == 0) {
        MAC802154_LOGD("test set and get cca threshold\n");
        int8_t cca_threshold_set = -85;
        bk_ieee802154_set_cca_threshold(cca_threshold_set);
        int8_t cca_threshold_get = bk_ieee802154_get_cca_threshold();
        if(cca_threshold_get == cca_threshold_set){
            MAC802154_LOGD(" test pass \n");
        }else {
            MAC802154_LOGD(" test failed, get threshold %d\n", cca_threshold_get);
        }
    } else if (strcmp(argv[1], "set_get_panid") == 0) {
        MAC802154_LOGD("test set and get panid\n");
        uint16_t panid_set = 0x7;
        bk_ieee802154_set_panid(panid_set);
        uint16_t panid_get = bk_ieee802154_get_panid();
        if(panid_get == panid_set){
            MAC802154_LOGD(" test pass \n");
        }
    } else if (strcmp(argv[1], "set_get_short_addr") == 0) {
        MAC802154_LOGD("test set and get short addr\n");
        uint16_t short_addr_set = 0x1234;
        bk_ieee802154_set_short_address(short_addr_set);
        uint16_t short_addr_get = 0;
        short_addr_get = bk_ieee802154_get_short_address();
        if(short_addr_get == short_addr_set){
            MAC802154_LOGD(" test pass \n");
        }
    } else if (strcmp(argv[1], "set_get_extend_addr") == 0) {
        MAC802154_LOGD("test set and get extend_addr\n");
        const uint8_t extend_addr_set[8] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
        bk_ieee802154_set_extended_address(extend_addr_set);
        uint8_t extend_addr_get[8] = { 0 };
        bk_ieee802154_get_extended_address(extend_addr_get);
        if(0 == memcmp(extend_addr_get, extend_addr_set, 8)){
            MAC802154_LOGD(" test pass \n");
        }
    } else if (strcmp(argv[1], "set_get_promiscuous") == 0) {
        MAC802154_LOGD("test set and get promiscuous\n");
        bool promiscuous_set = true;
        bk_ieee802154_set_promiscuous(promiscuous_set);
        bool promiscuous_get = bk_ieee802154_get_promiscuous();
        if ( promiscuous_get == promiscuous_set){
            MAC802154_LOGD(" test pass \n");
        }
    } else if (strcmp(argv[1], "set_get_txpower") == 0) {
        MAC802154_LOGD("test set and get txpower\n");
        int8_t txpower_set = 0;
        bk_ieee802154_set_txpower(txpower_set);
        int8_t txpower_get = bk_ieee802154_get_txpower();
        if ( txpower_get == txpower_set ){
            MAC802154_LOGD(" test pass \n");
        }
        //} else if (strcmp(argv[1], "") == 0) {
} else {
    printf("cmd not support!!!\r\n");
}
}
#endif
