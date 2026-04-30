#pragma once

#include "network_transfer.h"

#ifdef __cplusplus
extern "C" {
#endif


bk_err_t bk_udp_trans_service_init(char *service_name);
bk_err_t bk_udp_trans_service_deinit(void);

bk_err_t bk_tcp_trans_service_init(char *service_name);
bk_err_t bk_tcp_trans_service_deinit(void);

bk_err_t bk_cs2_trans_service_init(char *service_name);
bk_err_t bk_cs2_trans_service_deinit(void);

#if CONFIG_KVS_NTWK_BRIDGE
bk_err_t bk_kvs_trans_service_init(char *service_name);
bk_err_t bk_kvs_trans_service_deinit(void);
#endif

#ifdef __cplusplus
}
#endif