/*
 * Copyright (c) 2024, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "netif/etharp.h"
#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "lwip/pbuf.h"
#include "lwip/tcpip.h"
#if LWIP_DHCP
#include "lwip/dhcp.h"
#include "lwip/prot/dhcp.h"
#endif
#if LWIP_DNS
#include "lwip/dns.h"
#endif

#include "usbh_core.h"

#if LWIP_TCPIP_CORE_LOCKING_INPUT != 1
#warning suggest you to set LWIP_TCPIP_CORE_LOCKING_INPUT to 1, usb handles eth input with own thread
#endif

#if LWIP_TCPIP_CORE_LOCKING != 1
#error must set LWIP_TCPIP_CORE_LOCKING to 1
#endif

#if PBUF_POOL_BUFSIZE < 1600
#error PBUF_POOL_BUFSIZE must be larger than 1600
#endif

#if TCPIP_THREAD_STACKSIZE < 1024
#error TCPIP_THREAD_STACKSIZE must be >= 1024
#endif

#if !defined(CONFIG_USBHOST_PLATFORM_CDC_ECM) &&   \
    !defined(CONFIG_USBHOST_PLATFORM_CDC_RNDIS) && \
    !defined(CONFIG_USBHOST_PLATFORM_CDC_NCM) &&   \
    !defined(CONFIG_USBHOST_PLATFORM_ASIX) &&      \
    !defined(CONFIG_USBHOST_PLATFORM_RTL8152)
#error "Please enable at least one USB Ethernet platform in usb_config.h or Kconfig"
#endif

// #define CONFIG_USBHOST_PLATFORM_CDC_ECM
// #define CONFIG_USBHOST_PLATFORM_CDC_RNDIS
// #define CONFIG_USBHOST_PLATFORM_CDC_NCM
// #define CONFIG_USBHOST_PLATFORM_ASIX
// #define CONFIG_USBHOST_PLATFORM_RTL8152

ip_addr_t g_ipaddr;
ip_addr_t g_netmask;
ip_addr_t g_gateway;

void usbh_lwip_eth_output_common(struct pbuf *p, uint8_t *buf)
{
    struct pbuf *q;
    uint8_t *buffer;

    buffer = buf;
    for (q = p; q != NULL; q = q->next) {
        usb_memcpy(buffer, q->payload, q->len);
        buffer += q->len;
    }
}

void usbh_lwip_eth_input_common(struct netif *netif, uint8_t *buf, uint32_t len)
{
    struct pbuf *p;
    struct pbuf *q;
    const uint8_t *src = buf;
    uint32_t left = len;
    err_t err;

    /* Own the bytes. A PBUF_REF onto the caller's static RX buffer is reused
     * by the next USB IN while TCP still holds the pbuf. */
    p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
    if (p == NULL) {
        USB_LOG_ERR("No memory to alloc pbuf\r\n");
        return;
    }
    for (q = p; q != NULL && left != 0; q = q->next) {
        uint32_t n = q->len < left ? q->len : left;

        usb_memcpy(q->payload, src, n);
        src += n;
        left -= n;
    }
    err = netif->input(p, netif);
    if (err != ERR_OK) {
        pbuf_free(p);
    }
}

struct usb_osal_timer *dhcp_handle;

static void dhcp_timeout(void *arg)
{
    struct netif *netif = (struct netif *)arg;
#if LWIP_DHCP
    struct dhcp *dhcp;
#endif
    if (netif_is_up(netif)) {
#if LWIP_DHCP
        dhcp = netif_dhcp_data(netif);

        if (dhcp && (dhcp->state == DHCP_STATE_BOUND)) {
#endif
            USB_LOG_INFO("IPv4 Address     : %s\r\n", ipaddr_ntoa(&netif->ip_addr));
            USB_LOG_INFO("IPv4 Subnet mask : %s\r\n", ipaddr_ntoa(&netif->netmask));
            USB_LOG_INFO("IPv4 Gateway     : %s\r\n", ipaddr_ntoa(&netif->gw));
#if LWIP_DNS
            USB_LOG_INFO("IPv4 DNS         : %s\r\n", ipaddr_ntoa(dns_getserver(0)));
            USB_LOG_INFO("IPv4 DNS2        : %s\r\n", ipaddr_ntoa(dns_getserver(1)));
#endif
            USB_LOG_INFO("\r\n");

            usb_osal_timer_stop(dhcp_handle);
#if LWIP_DHCP
        }
#endif
    } else {
    }
}

#ifdef CONFIG_USBHOST_PLATFORM_CDC_ECM
#include "usbh_cdc_ecm.h"

struct netif g_cdc_ecm_netif;

/* Whether g_cdc_ecm_netif is currently registered with lwIP. run() can fail
 * after rolling its own work back, and disconnect still calls stop() for the
 * cases where the rx thread owned the teardown, so stop() needs to tell a live
 * interface from one that was never there. */
static bool g_cdc_ecm_netif_added;

static err_t usbh_cdc_ecm_linkoutput(struct netif *netif, struct pbuf *p)
{
    int ret;
    (void)netif;

    usbh_lwip_eth_output_common(p, usbh_cdc_ecm_get_eth_txbuf());
    ret = usbh_cdc_ecm_eth_output(p->tot_len);
    if (ret < 0) {
        return ERR_BUF;
    } else {
        return ERR_OK;
    }
}

void usbh_cdc_ecm_eth_input(uint8_t *buf, uint32_t buflen)
{
    usbh_lwip_eth_input_common(&g_cdc_ecm_netif, buf, buflen);
}

static err_t usbh_cdc_ecm_if_init(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));

    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
    netif->state = NULL;
    netif->name[0] = 'E';
    netif->name[1] = 'X';
    netif->output = etharp_output;
    netif->linkoutput = usbh_cdc_ecm_linkoutput;
    return ERR_OK;
}

/* This runs on the USB enumeration thread, so every netif/dhcp call below goes
 * through netifapi_* to execute inside the tcpip thread. LWIP_TCPIP_CORE_LOCKING
 * is 1 here (lwipopts.h leaves it at the opt.h default), which makes the raw
 * netif_add()/dhcp_start() calls this file used to make a data race against the
 * tcpip thread. netifapi_* is also what this SDK's own net.c uses for its netifs. */
int usbh_cdc_ecm_run(struct usbh_cdc_ecm *cdc_ecm_class)
{
    struct netif *netif = &g_cdc_ecm_netif;
    err_t err;

    netif->hwaddr_len = 6;
    memcpy(netif->hwaddr, cdc_ecm_class->mac, 6);

    IP4_ADDR(&g_ipaddr, 0, 0, 0, 0);
    IP4_ADDR(&g_netmask, 0, 0, 0, 0);
    IP4_ADDR(&g_gateway, 0, 0, 0, 0);

    err = netifapi_netif_add(netif, ip_2_ip4(&g_ipaddr), ip_2_ip4(&g_netmask), ip_2_ip4(&g_gateway),
                             NULL, usbh_cdc_ecm_if_init, tcpip_input);
    if (err != ERR_OK) {
        USB_LOG_ERR("Failed to add cdc ecm netif, err:%d\r\n", err);
        return -USB_ERR_INVAL;
    }
    g_cdc_ecm_netif_added = true;
    netifapi_netif_set_default(netif);

    /* usbh_cdc_ecm_if_init() raises NETIF_FLAG_UP itself and netifapi_netif_add()
     * only returns once the tcpip thread has run it, so this is a state check,
     * not a wait. Spinning here would hang the enumeration thread for good. */
    if (!netif_is_up(netif)) {
        USB_LOG_ERR("cdc ecm netif did not come up\r\n");
        goto err_remove_netif;
    }

    dhcp_handle = usb_osal_timer_create("dhcp", 200, dhcp_timeout, netif, true);
    if (dhcp_handle == NULL) {
        USB_LOG_ERR("timer creation failed! \r\n");
        goto err_remove_netif;
    }

    /* DHCP is started before the rx thread on purpose: rolling a failed
     * dhcp_start() back would otherwise have to stop a thread that is already
     * running, which duplicates disconnect's exit handshake inside this
     * function. dhcp_start() only queues a DISCOVER and arms lwIP's own retry
     * timers, so having the thread appear milliseconds later changes nothing -
     * the retries cover it. */
#if LWIP_DHCP
    err = netifapi_dhcp_start(netif);
    if (err != ERR_OK) {
        USB_LOG_ERR("Failed to start dhcp on cdc ecm netif, err:%d\r\n", err);
        goto err_stop_dhcp;
    }
#endif

    /* Arm the stop handshake before the thread exists: usbh_cdc_ecm_rx_thread()
     * gives the semaphore on its way out and disconnect waits for it. */
    if (usbh_cdc_ecm_rx_thread_prepare() < 0) {
        goto err_stop_dhcp;
    }

    if (usb_osal_thread_create("usbh_cdc_ecm_rx", 2048, CONFIG_USBHOST_PSC_PRIO + 1,
                               usbh_cdc_ecm_rx_thread, NULL) == NULL) {
        USB_LOG_ERR("Failed to create cdc ecm rx thread\r\n");
        usbh_cdc_ecm_rx_thread_abort();
        goto err_stop_dhcp;
    }

#if LWIP_DHCP
    usb_osal_timer_start(dhcp_handle);
#endif
    return 0;

    /* Unwind in reverse so a failure leaves nothing behind - no netif with no rx
     * thread feeding it, no timer polling a netif that is gone. The error goes
     * back to usbh_cdc_ecm_connect(), which then fails the interface outright. */
err_stop_dhcp:
#if LWIP_DHCP
    /* Safe after a failed dhcp_start() too: dhcp_release_and_stop() and
     * dhcp_cleanup() both return early when the netif has no dhcp struct. */
    netifapi_dhcp_stop(netif);
    netifapi_netif_common(netif, dhcp_cleanup, NULL);
#endif
    usb_osal_timer_delete(dhcp_handle);
    dhcp_handle = NULL;
err_remove_netif:
    netifapi_netif_set_down(netif);
    netifapi_netif_remove(netif);
    g_cdc_ecm_netif_added = false;
    return -USB_ERR_INVAL;
}

void usbh_cdc_ecm_stop(struct usbh_cdc_ecm *cdc_ecm_class)
{
    struct netif *netif = &g_cdc_ecm_netif;
    (void)cdc_ecm_class;

    /* run() already unwound everything, so there is nothing here to take down
     * and netif_remove() would be operating on an unregistered netif. */
    if (!g_cdc_ecm_netif_added) {
        return;
    }

#if LWIP_DHCP
    netifapi_dhcp_stop(netif);
    /* dhcp_cleanup() has no netifapi_ wrapper of its own; netifapi_netif_common()
     * is the generic form the other wrappers expand to. */
    netifapi_netif_common(netif, dhcp_cleanup, NULL);
#endif
    /* run() creates the timer unconditionally, so release it the same way.
     * NULL when run() already rolled back. */
    if (dhcp_handle) {
        usb_osal_timer_delete(dhcp_handle);
        dhcp_handle = NULL;
    }
    netifapi_netif_set_down(netif);
    netifapi_netif_remove(netif);
    g_cdc_ecm_netif_added = false;
}
#endif

#ifdef CONFIG_USBHOST_PLATFORM_CDC_RNDIS
#include "usbh_rndis.h"

struct usb_osal_timer *timer_handle;

static void rndis_dev_keepalive_timeout(void *arg)
{
    struct usbh_rndis *rndis_class = (struct usbh_rndis *)arg;
    usbh_rndis_keepalive(rndis_class);
}

void timer_init(struct usbh_rndis *rndis_class)
{
    timer_handle = usb_osal_timer_create("rndis_keepalive", 5000, rndis_dev_keepalive_timeout, rndis_class, true);
    if (NULL != timer_handle) {
        usb_osal_timer_start(timer_handle);
    } else {
        USB_LOG_ERR("timer creation failed! \r\n");
        for (;;) {
            ;
        }
    }
}

struct netif g_rndis_netif;

static err_t usbh_rndis_linkoutput(struct netif *netif, struct pbuf *p)
{
    int ret;
    (void)netif;

    usbh_lwip_eth_output_common(p, usbh_rndis_get_eth_txbuf());
    ret = usbh_rndis_eth_output(p->tot_len);
    if (ret < 0) {
        return ERR_BUF;
    } else {
        return ERR_OK;
    }
}

void usbh_rndis_eth_input(uint8_t *buf, uint32_t buflen)
{
    usbh_lwip_eth_input_common(&g_rndis_netif, buf, buflen);
}

static err_t usbh_rndis_if_init(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));

    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
    netif->state = NULL;
    netif->name[0] = 'E';
    netif->name[1] = 'X';
    netif->output = etharp_output;
    netif->linkoutput = usbh_rndis_linkoutput;
    return ERR_OK;
}

void usbh_rndis_run(struct usbh_rndis *rndis_class)
{
    struct netif *netif = &g_rndis_netif;

    netif->hwaddr_len = 6;
    memcpy(netif->hwaddr, rndis_class->mac, 6);

    IP4_ADDR(&g_ipaddr, 0, 0, 0, 0);
    IP4_ADDR(&g_netmask, 0, 0, 0, 0);
    IP4_ADDR(&g_gateway, 0, 0, 0, 0);

    netif = netif_add(netif, &g_ipaddr, &g_netmask, &g_gateway, NULL, usbh_rndis_if_init, tcpip_input);
    netif_set_default(netif);
    while (!netif_is_up(netif)) {
    }

    dhcp_handle = usb_osal_timer_create("dhcp", 200, dhcp_timeout, netif, true);
    if (dhcp_handle == NULL) {
        USB_LOG_ERR("timer creation failed! \r\n");
        while (1) {
        }
    }

    usb_osal_thread_create("usbh_rndis_rx", 2048, CONFIG_USBHOST_PSC_PRIO + 1, usbh_rndis_rx_thread, NULL);

    //timer_init(rndis_class);

#if LWIP_DHCP
    dhcp_start(netif);
    usb_osal_timer_start(dhcp_handle);
#endif
}

void usbh_rndis_stop(struct usbh_rndis *rndis_class)
{
    struct netif *netif = &g_rndis_netif;
    (void)rndis_class;

#if LWIP_DHCP
    dhcp_stop(netif);
    dhcp_cleanup(netif);
    usb_osal_timer_delete(dhcp_handle);
#endif
    netif_set_down(netif);
    netif_remove(netif);
    // xTimerStop(timer_handle, 0);
    // xTimerDelete(timer_handle, 0);
}
#endif

#ifdef CONFIG_USBHOST_PLATFORM_CDC_NCM
#include "usbh_cdc_ncm.h"

struct netif g_cdc_ncm_netif;

static err_t usbh_cdc_ncm_linkoutput(struct netif *netif, struct pbuf *p)
{
    int ret;
    (void)netif;

    usbh_lwip_eth_output_common(p, usbh_cdc_ncm_get_eth_txbuf());
    ret = usbh_cdc_ncm_eth_output(p->tot_len);
    if (ret < 0) {
        return ERR_BUF;
    } else {
        return ERR_OK;
    }
}

void usbh_cdc_ncm_eth_input(uint8_t *buf, uint32_t buflen)
{
    usbh_lwip_eth_input_common(&g_cdc_ncm_netif, buf, buflen);
}

static err_t usbh_cdc_ncm_if_init(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));

    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
    netif->state = NULL;
    netif->name[0] = 'E';
    netif->name[1] = 'X';
    netif->output = etharp_output;
    netif->linkoutput = usbh_cdc_ncm_linkoutput;
    return ERR_OK;
}

void usbh_cdc_ncm_run(struct usbh_cdc_ncm *cdc_ncm_class)
{
    struct netif *netif = &g_cdc_ncm_netif;

    netif->hwaddr_len = 6;
    memcpy(netif->hwaddr, cdc_ncm_class->mac, 6);

    IP4_ADDR(&g_ipaddr, 0, 0, 0, 0);
    IP4_ADDR(&g_netmask, 0, 0, 0, 0);
    IP4_ADDR(&g_gateway, 0, 0, 0, 0);

    netif = netif_add(netif, &g_ipaddr, &g_netmask, &g_gateway, NULL, usbh_cdc_ncm_if_init, tcpip_input);
    netif_set_default(netif);
    while (!netif_is_up(netif)) {
    }

    dhcp_handle = usb_osal_timer_create("dhcp", 200, dhcp_timeout, netif, true);
    if (dhcp_handle == NULL) {
        USB_LOG_ERR("timer creation failed! \r\n");
        while (1) {
        }
    }

    usb_osal_thread_create("usbh_cdc_ncm_rx", 2048, CONFIG_USBHOST_PSC_PRIO + 1, usbh_cdc_ncm_rx_thread, NULL);
#if LWIP_DHCP
    dhcp_start(netif);
    usb_osal_timer_start(dhcp_handle);
#endif
}

void usbh_cdc_ncm_stop(struct usbh_cdc_ncm *cdc_ncm_class)
{
    struct netif *netif = &g_cdc_ncm_netif;
    (void)cdc_ncm_class;

#if LWIP_DHCP
    dhcp_stop(netif);
    dhcp_cleanup(netif);
    usb_osal_timer_delete(dhcp_handle);
#endif
    netif_set_down(netif);
    netif_remove(netif);
}
#endif

#ifdef CONFIG_USBHOST_PLATFORM_ASIX
#include "usbh_asix.h"

struct netif g_asix_netif;

static err_t usbh_asix_linkoutput(struct netif *netif, struct pbuf *p)
{
    int ret;
    (void)netif;

    usbh_lwip_eth_output_common(p, usbh_asix_get_eth_txbuf());
    ret = usbh_asix_eth_output(p->tot_len);
    if (ret < 0) {
        return ERR_BUF;
    } else {
        return ERR_OK;
    }
}

void usbh_asix_eth_input(uint8_t *buf, uint32_t buflen)
{
    usbh_lwip_eth_input_common(&g_asix_netif, buf, buflen);
}

static err_t usbh_asix_if_init(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));

    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
    netif->state = NULL;
    netif->name[0] = 'E';
    netif->name[1] = 'X';
    netif->output = etharp_output;
    netif->linkoutput = usbh_asix_linkoutput;
    return ERR_OK;
}

void usbh_asix_run(struct usbh_asix *asix_class)
{
    struct netif *netif = &g_asix_netif;

    netif->hwaddr_len = 6;
    memcpy(netif->hwaddr, asix_class->mac, 6);

    IP4_ADDR(&g_ipaddr, 0, 0, 0, 0);
    IP4_ADDR(&g_netmask, 0, 0, 0, 0);
    IP4_ADDR(&g_gateway, 0, 0, 0, 0);

    netif = netif_add(netif, &g_ipaddr, &g_netmask, &g_gateway, NULL, usbh_asix_if_init, tcpip_input);
    netif_set_default(netif);
    while (!netif_is_up(netif)) {
    }

    dhcp_handle = usb_osal_timer_create("dhcp", 200, dhcp_timeout, netif, true);
    if (dhcp_handle == NULL) {
        USB_LOG_ERR("timer creation failed! \r\n");
        while (1) {
        }
    }

    usb_osal_thread_create("usbh_asix_rx", 2048, CONFIG_USBHOST_PSC_PRIO + 1, usbh_asix_rx_thread, NULL);
#if LWIP_DHCP
    dhcp_start(netif);
    usb_osal_timer_start(dhcp_handle);
#endif
}

void usbh_asix_stop(struct usbh_asix *asix_class)
{
    struct netif *netif = &g_asix_netif;
    (void)asix_class;

#if LWIP_DHCP
    dhcp_stop(netif);
    dhcp_cleanup(netif);
    usb_osal_timer_delete(dhcp_handle);
#endif
    netif_set_down(netif);
    netif_remove(netif);
}
#endif

#ifdef CONFIG_USBHOST_PLATFORM_RTL8152
#include "usbh_rtl8152.h"

struct netif g_rtl8152_netif;

static err_t usbh_rtl8152_linkoutput(struct netif *netif, struct pbuf *p)
{
    int ret;
    (void)netif;

    usbh_lwip_eth_output_common(p, usbh_rtl8152_get_eth_txbuf());
    ret = usbh_rtl8152_eth_output(p->tot_len);
    if (ret < 0) {
        return ERR_BUF;
    } else {
        return ERR_OK;
    }
}

void usbh_rtl8152_eth_input(uint8_t *buf, uint32_t buflen)
{
    usbh_lwip_eth_input_common(&g_rtl8152_netif, buf, buflen);
}

static err_t usbh_rtl8152_if_init(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));

    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
    netif->state = NULL;
    netif->name[0] = 'E';
    netif->name[1] = 'X';
    netif->output = etharp_output;
    netif->linkoutput = usbh_rtl8152_linkoutput;
    return ERR_OK;
}

void usbh_rtl8152_run(struct usbh_rtl8152 *rtl8152_class)
{
    struct netif *netif = &g_rtl8152_netif;

    netif->hwaddr_len = 6;
    memcpy(netif->hwaddr, rtl8152_class->mac, 6);

    IP4_ADDR(&g_ipaddr, 0, 0, 0, 0);
    IP4_ADDR(&g_netmask, 0, 0, 0, 0);
    IP4_ADDR(&g_gateway, 0, 0, 0, 0);

    netif = netif_add(netif, &g_ipaddr, &g_netmask, &g_gateway, NULL, usbh_rtl8152_if_init, tcpip_input);
    netif_set_default(netif);
    while (!netif_is_up(netif)) {
    }

    dhcp_handle = usb_osal_timer_create("dhcp", 200, dhcp_timeout, netif, true);
    if (dhcp_handle == NULL) {
        USB_LOG_ERR("timer creation failed! \r\n");
        while (1) {
        }
    }

    usb_osal_thread_create("usbh_rtl8152_rx", 2048, CONFIG_USBHOST_PSC_PRIO + 1, usbh_rtl8152_rx_thread, NULL);
#if LWIP_DHCP
    dhcp_start(netif);
    usb_osal_timer_start(dhcp_handle);
#endif
}

void usbh_rtl8152_stop(struct usbh_rtl8152 *rtl8152_class)
{
    struct netif *netif = &g_rtl8152_netif;
    (void)rtl8152_class;

#if LWIP_DHCP
    dhcp_stop(netif);
    dhcp_cleanup(netif);
    usb_osal_timer_delete(dhcp_handle);
#endif
    netif_set_down(netif);
    netif_remove(netif);
}
#endif
