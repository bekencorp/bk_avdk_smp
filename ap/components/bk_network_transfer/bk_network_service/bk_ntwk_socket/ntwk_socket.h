#pragma once

#include "lwip/sockets.h"
#include "net.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IP_QOS_PRIORITY_HIGHEST			(0xD0)
#define IP_QOS_PRIORITY_HIGH			(0xA0)
#define IP_QOS_PRIORITY_LOW				(0x20)
#define IP_QOS_PRIORITY_LOWEST			(0x00)

#define NTWK_SEND_MAX_RETRY (2000)
#define NTWK_SEND_MAX_DELAY (10)


typedef int (*ntwk_socket_abort_check_cb_t)(uint32_t user_data);

void ntwk_socket_register_abort_check_cb(ntwk_socket_abort_check_cb_t abort_check);
int ntwk_socket_set_qos(int fd, int qos);
int ntwk_socket_sendto(int *fd, const struct sockaddr *dst, uint8_t *data, uint32_t length);
int ntwk_socket_sendto_abortable(int *fd, const struct sockaddr *dst, uint8_t *data, uint32_t length,
                                 uint32_t user_data);
int ntwk_socket_write(int *fd, uint8_t *data, uint32_t length);
int ntwk_socket_write_abortable(int *fd, uint8_t *data, uint32_t length, uint32_t user_data);


#ifdef __cplusplus
}
#endif
