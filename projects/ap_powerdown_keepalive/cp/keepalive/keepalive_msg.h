#pragma once
#include <stdlib.h>
#include <string.h>
#include <common/bk_include.h>
#include "cli_config.h"
#include "bk_wifi_types.h"
#include "bk_wifi.h"
#include <os/str.h>
#include <os/mem.h>
#include <os/os.h>
#include <common/bk_err.h>
#include <components/log.h>
#include <components/event.h>
#include <common/sys_config.h>
#include <driver/uart.h>
#include "bk_uart.h"
#include <driver/uart.h>
#include <components/netif.h>
#include "bk_cli.h"
#include "cli.h"
#include "bk_private/bk_wifi.h"
#include "bk_wifi_private.h"
#include <driver/gpio.h>
#include "bk_pm_internal_api.h"
#include "controller_wifi_if.h"
#include <lwip/sockets.h>
#include <components/netif.h>
#include "net.h"
#include "conv_utf8_pub.h"
#include "gpio_driver.h"
#include <driver/aon_rtc_types.h>
#include <driver/aon_rtc.h>
#ifdef __cplusplus
extern "C" {
#endif


#define KA_MAX_MSG_STR_LEN               (50)

#define KA_QUEUE_LEN                          64
#define KA_TASK_PRIO                          4

#define KA_SOCKET_MAX_RETRY_CNT               (5)

#define KEEPALIVE_RX_DISCONNECT_TIME_SEC   (1800)
#define KEEPALIVE_START_LVSLEEP_DELAY      (4000)
#define KEEPALIVE_TX_INTERVAL_SEC          (20)
#define KEEPALIVE_TX_DATA_SIZE             (100)
#define KEEPALIVE_RX_DATA_SIZE             (100)
#define KEEPALIVE_TX_TIMEOUT_SEC           (3)
#define KEEPALIVE_RX_TIMEOUT_SEC           (3)
#define KEEPALIVE_RX_MAX_RETRY_CNT         20
#define KA_MSG_MAX_LEN                        sizeof(ka_msg_t)
#define KA_RTC_TIMER_THRESHOLD               (500)

#define KA_INVALID_INDEX                      (-1)

#define KA_DEFAULT_SOCKET_PORT                (5006)

#define KA_CO_TIMER_INTVL                         10*1000     //10s
#define KEEPALIVE_WITH_HOST_TIME               1*60*1000   //1min
#define KEEPALIVE_WITH_SERVER_FAIL_TIME        4*60*1000   //4min

#define KA_INVALID_KEEPALIVE_CNT                  0xFFFF

typedef enum
{
    KA_CLIENT = 0,
    KA_SERVER = 1
}ka_role;

typedef enum
{
    KA_ACTIVE = 0,
    KA_KEEPALIVE_SLEEP = 1,
    KA_DEEP_SLEEP = 2,
}ka_lp_state;
typedef enum
{
    KA_MSG_KEEPALIVE_REQ = 0,
    KA_MSG_KEEPALIVE_RSP = 1,
    KA_MSG_WAKEUP_HOST_REQ = 2,
    KA_MSG_WAKEUP_HOST_RSP = 3,
    KA_MSG_PWRDOWN_HOST_REQ = 4,
    KA_MSG_PWRDOWN_HOST_RSP = 5,
    KA_MSG_ENTER_LVSLEEP_REQ = 6,
    KA_MSG_ENTER_LVSLEEP_RSP = 7,
    KA_MSG_EXIT_LVSLEEP_REQ = 8,
    KA_MSG_EXIT_LVSLEEP_RSP = 9,
    KA_MSG_ENTER_DEEPSLEEP_REQ = 10,
    KA_MSG_ENTER_DEEPSLEEP_RSP = 11,

    KA_MSG_BUTT
}ka_msg_id_e;

typedef struct
{
    uint16_t len;
    ka_msg_id_e msg_id;
    uint32_t data;
}ka_msg_t;
typedef struct
{
    uint8_t *rx_buffer;
    int16_t rx_size;
}ka_rx_msg_info_t;

typedef enum
{
    KEEPALIVE_WITH_HOSTS =0,
    KEEPALIVE_WITH_SERVER_FAILED,
}ka_common_e;


typedef struct
{
    uint32_t queue_item_count;
    beken_queue_t msg_queue;
    beken_thread_t handle;
    uint32_t stack_size;
    beken2_timer_t timer;
    uint16_t host_keepalive_cnt;
    uint16_t server_keepalive_cnt;
} ka_common_t;


extern ka_common_t ka_common;

//disable keep alive with host
inline static void ka_disable_keepalive_with_host()
{
    ka_common.host_keepalive_cnt = KA_INVALID_KEEPALIVE_CNT;
}
//reset keep alive count to avoid timout leading to reset host
inline static void ka_reset_keepalive_with_host()
{
    ka_common.host_keepalive_cnt = 0x0;
}

//disable keepalive with server
inline static void ka_disable_keepalive_with_server()
{
    ka_common.server_keepalive_cnt = KA_INVALID_KEEPALIVE_CNT;
}

//reset keepalive with host count to avoid timout leading to reset host
inline static void ka_reset_keepalive_with_server()
{
    ka_common.server_keepalive_cnt = 0x0;
}

void ka_set_sock_opt(int sock);
int ka_send_data(int sock, uint8_t *data, uint16_t len);
int ka_send_msg(int sock, ka_msg_id_e msg_id);
ka_msg_id_e ka_parse_rx_msg(int sock, ka_rx_msg_info_t *rx_msg_info);
int ka_recv_msg_handle(int sock, ka_rx_msg_info_t *rx_msg_info);
int ka_param_find_id(int argc, char **argv, char *param);
unsigned int ka_common_init(void);

#ifdef __cplusplus
}
#endif


