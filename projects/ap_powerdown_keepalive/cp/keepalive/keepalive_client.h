#pragma once

#include "keepalive_msg.h"
#ifdef __cplusplus
extern "C" {
#endif


typedef struct
{
    bool keepalive_ongoing;
    uint8_t lp_state;
    struct bk_msg_keepalive_cfg_req keepalive_cfg;
    beken_timer_t timer;
    void *io_queue;
    void *tx_handle;
    void *rx_handle;
    beken_semaphore_t sema;
    uint8_t sock_retry_cnt;
    uint16_t tx_interval_s;
    int sock;
    alarm_info_t keepalive_rtc;
    ka_rx_msg_info_t rx_msg_info;
} ka_client_env_t;


bk_err_t ka_client_init(void);
bk_err_t ka_client_deinit(void);
void ka_client_hanlde_cli(int argc, char **argv);
extern bk_err_t ka_client_demo_init(void *arg);
#ifdef __cplusplus
}
#endif


