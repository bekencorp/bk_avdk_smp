#ifndef _BK_RLK_IPERF_H_
#define _BK_RLK_IPERF_H_

#include "rlk_common.h"


#define RLK_IPERF_BUFSZ             (1470)

#define RLK_IPERF_RX_BUFSZ             (4 * 1024)

#define RLK_IPERF_DEFAULT_TIME     30

#define RLK_IPERF_PRIORITY            4
#define RLK_IPERF_REPORT_TASK_PRIORITY      3

#define RLK_IPERF_THREAD_SIZ          (4 * 1024)
#define RLK_IPERF_RECV_MSG_NUM            32
#define RLK_IPERF_MAX_RX_RETRY      10
#define RLK_IPERF_MAX_TX_RETRY      10

#define RLK_IPERF_REPORT_TASK_NAME "rlk_iperf_report_task"

#define RLK_IPERF_REPORT_TASK_STACK 2048

#define RLK_IPERF_KILO_UNIT              0x400      /* 1024 */
#define RLK_IPERF_MILLION_UNIT           1000000    /* 1000 * 1000 */

#define RLK_IPERF_REPORT_INTERVAL   1

#define RLK_IPERF_CONNECITON_SIZE   32

#define RLK_IPERF_DEFAULT_SPEED_LIMIT   (-1)


enum {
    RLK_IPERF_STATE_STOPPED = 0,
    RLK_IPERF_STATE_STOPPING,
    RLK_IPERF_STATE_STARTED,
};

enum {
    RLK_IPERF_MODE_NONE = 0,
    RLK_IPERF_MODE_SERVER,
    RLK_IPERF_MODE_CLIENT,
    RLK_IPERF_MODE_UNKNOWN,
};

typedef struct
{
  /** mbox where received packets are stored until they are fetched
      by the netconn application thread (can grow quite big) */
  beken_queue_t recvmbox;
}RLK_IPERF_CONN;

typedef struct {
    int state;
    char mac_str[RLK_MAC_STRING_LEN];
    uint8_t mac_addr[RLK_WIFI_MAC_ADDR_LEN];
    uint8_t mac_addr_rsp[RLK_WIFI_MAC_ADDR_LEN];
    uint8_t local_mac_addr[RLK_WIFI_MAC_ADDR_LEN];
    int mode;
    RLK_IPERF_CONN conn;
    uint32_t size;
    uint32_t interval;
    uint32_t s_time;
    uint32_t connected;
    uint32_t connected_seq;
    uint32_t connected_cnt;
    int speed_limit;
} RLK_IPERF_PARAM;

extern RLK_IPERF_PARAM rlk_iperf_env;

void rlk_iperf_defaults_set(void);
void rlk_iperf_usage(void);
void rlk_iperf_stop(void);
bk_err_t rlk_iperf_send_disconnection(void);
bk_err_t rlk_iperf_send_connection_req(void);
bk_err_t rlk_iperf_disconnection_handler(bk_rlk_recv_info_t *rx_info);
bk_err_t rlk_iperf_connection_req_handler(bk_rlk_recv_info_t *rx_info);
void rlk_iperf_start(int mode);

#endif //_BK_RLK_IPERF_H_
// eof

