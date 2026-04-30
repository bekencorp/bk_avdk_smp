#pragma once

#include "keepalive_msg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    bool keepalive_ongoing;
    uint16_t port;
    beken_timer_t timer;
    void *io_queue;
    void *handle;
    uint8_t sock_retry_cnt;
    int client_sock;
    ka_rx_msg_info_t rx_msg_info;
}ka_server_env_t;

bk_err_t ka_server_init(uint16_t port);
extern void ka_server_hanlde_cli(int argc, char **argv);

#ifdef __cplusplus
}
#endif


