#ifndef _SK_INTF_H_
#define _SK_INTF_H_

#include "common.h"
#include "fake_socket.h"

#define SK_INTF_MGMT_SOCKET_NUM              (PF_PACKET + SOCK_RAW + ETH_P_ALL)
#define SK_INTF_IOCTL_SOCKET_NUM             (PF_INET + SOCK_DGRAM + 0)
#if CONFIG_WAPI_SUPPORT
#define SK_INTF_DATA_SOCKET_BASE_NUM              (PF_PACKET + SOCK_RAW)
#else
#define SK_INTF_DATA_SOCKET_NUM              (PF_PACKET + SOCK_RAW + ETH_P_PAE)
#endif

#if CONFIG_WAPI_SUPPORT
void sk_intf_data_set_wapi_enable(bool wapi);
bool sk_intf_data_get_wapi_enable(void);
#endif

/**
 * Structure for mgmt packet TX parameters
 */
struct ke_sk_params {
	unsigned char *buf; /* Buffer pointer */
	int len;            /* Buffer length */
	int flag;           /* VIF index / flag */
	uint32_t freq;      /* Frequency in MHz */
};

extern int ke_mgmt_peek_rxed_next_payload_size(int flag);
extern int ke_mgmt_packet_rx(struct ke_sk_params *params);
extern int ke_mgmt_packet_tx(const struct ke_sk_params *params);
extern int ke_l2_packet_tx(const struct ke_sk_params *params);
extern int ke_l2_packet_rx(struct ke_sk_params *params);
extern int ke_data_peek_txed_next_payload_size(int flag);
extern int ke_data_peek_rxed_next_payload_size(int flag);
extern int ws_mgmt_peek_rxed_next_payload_size(int flag);
extern int ws_get_mgmt_packet(struct ke_sk_params *params);
extern int ws_data_peek_rxed_next_payload_size(int flag);
extern int ws_get_data_packet(struct ke_sk_params *params);
extern SOCKET mgmt_get_socket_num(u8 vif_idx);
extern SOCKET data_get_socket_num(u8 vif_idx);
extern SOCKET ioctl_get_socket_num(u8 vif_idx);
#endif
// eof

