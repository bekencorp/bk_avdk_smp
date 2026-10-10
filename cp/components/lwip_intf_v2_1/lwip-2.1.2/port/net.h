#ifndef _NET_H_
#define _NET_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "lwip_netif_address.h"
#include <components/netif_types.h>

extern void uap_ip_down(void);
extern void uap_ip_start(void);
#if CONFIG_P2P
extern void p2p_go_ip_down(void);
extern void p2p_go_ip_start(void);
extern void p2p_gc_ip_down(void);
extern void p2p_gc_ip_start(void);
extern uint32_t p2p_go_ip_is_start(void);
extern uint32_t p2p_gc_ip_is_start(void);
extern void net_get_p2p_go_cfg_addr(struct wlan_ip_config *addr);
extern void *net_get_p2p_go_handle(void);
extern void *net_get_p2p_gc_handle(void);
#endif
extern void sta_ip_down(void);
extern void sta_ip_start(void);
extern uint32_t uap_ip_is_start(void);
extern uint32_t sta_ip_is_start(void);
extern void *net_get_sta_handle(void);
extern void *net_get_uap_handle(void);
extern int net_wlan_remove_netif(uint8_t *mac);
extern int net_get_if_macaddr(void *macaddr, void *intrfc_handle);
extern int net_get_if_addr(struct wlan_ip_config *addr, void *intrfc_handle);
extern void ip_address_set(int iface, int dhcp, char *ip, char *mask, char*gw, char*dns);
extern void sta_ip_mode_set(int dhcp);
#if CONFIG_WIFI6_CODE_STACK
extern bool etharp_tmr_flag;
extern void net_begin_send_arp_reply(bool is_send_arp, bool is_allow_send_req);
#endif
extern void net_restart_dhcp(void);
#if CONFIG_BRIDGE
extern void bridge_set_ip_start_flag(bool enable);
extern void bridge_ip_start(void);
extern void bridge_ip_stop(void);
extern void bridge_netif_teardown(void);
extern uint32_t bridge_ip_is_start(void);
extern void *net_get_br_handle(void);
#endif

/** Sticky DNS override used by STA sync / DHCP / RDNSS. */
int net_dns_override_set(const char *servers[], uint8_t count);
#if CONFIG_IPV6
int net_dns_override_set_v6(const char *servers[], uint8_t count);
#endif
void net_dns_override_clear(void);
void net_dns_override_apply_v4(void);
#if CONFIG_IPV6
void net_dns_override_apply_v6(void);
#endif
int net_dns_override_enabled(void);
uint32_t net_dns_override_get(uint8_t idx);
/* Import sticky snapshot from AP (STA_SET_DNS_SERVER). */
int net_dns_override_from_config(const netif_dns_config_t *cfg);

#ifdef __cplusplus
}
#endif

#endif // _NET_H_
// eof

