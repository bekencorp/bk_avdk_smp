#include "cif_wifi_dp.h"

#include "lwip/prot/ethernet.h"
#include "lwip/prot/ip4.h"
#include "lwip/prot/ip.h"
#include "lwip/prot/udp.h"
#include "lwip/prot/tcp.h"
#include "lwip/prot/icmp.h"
#if CONFIG_IPV6
#include "lwip/prot/ip6.h"
#include "lwip/prot/icmp6.h"
#endif
#include "lwip/ping.h"

#include "../../dhcpd/dhcp-bootp.h"
#include "cif_ipc.h"
#include "cif_co_list.h"
#if CONFIG_BK_RAW_LINK
#include "cif_wifi_api.h"
#include <modules/raw_link.h>
#include "cif_raw_link_api.h"
#endif

extern int bmsg_tx_sender(struct pbuf *p, uint32_t vif_idx);
extern void stack_mem_dump(uint32_t stack_top, uint32_t stack_bottom);
extern uint8_t vif_mgmt_get_sta_vif_index();
extern uint8_t vif_mgmt_get_softap_vif_index();
extern bk_err_t dma_memcpy(void *out, const void *in, uint32_t len);
extern uint8_t get_ping_state();
extern uint8_t iperf_get_state();
extern int wifi_netif_vif_to_vifid(void *vif);
extern void *mac_vif_mgmt_first_vif();
extern void *mac_vif_mgmt_next_vif(void *vif);
#if CONFIG_P2P
extern bool mac_vif_mgmt_interface_is_configured_for_p2p(void *vif);
#endif

static inline bool cif_vif_is_p2p(void *vif)
{
#if CONFIG_P2P
    return mac_vif_mgmt_interface_is_configured_for_p2p(vif);
#else
    (void)vif;
    return false;
#endif
}

/*
 * Coexistence netif wire id (2-bit co_hdr.vif_idx). Roles are NOT pinned to a
 * fixed LMAC VIF index any more (P2P GO/GC float on vif0/vif1 so the MCC
 * channel scheduler can handle them). We therefore key the wire id off the
 * (netif type, p2p) role instead of the LMAC index:
 *
 *   wire 0 = infra STA   (STA-type, !p2p) -> host g_mlan
 *   wire 1 = SoftAP      (AP-type,  !p2p) -> host g_uap
 *   wire 2 = P2P GO      (AP-type,   p2p) -> host g_p2p_go
 *   wire 3 = P2P GC      (STA-type,  p2p) -> host g_p2p_gc
 *
 * The TX path is symmetric: host tags the wire id, CP maps it back to the
 * current LMAC VIF index via cif_wire_to_lmac_vif() (no longer identity).
 */
static uint8_t cif_vif_to_netif_wire(void *vif)
{
    netif_if_t t = wifi_netif_vif_to_netif_type(vif);
    bool p2p = cif_vif_is_p2p(vif);

    if ((t == NETIF_IF_AP) && p2p)
        return 2;
    if ((t == NETIF_IF_STA) && p2p)
        return 3;

    return (uint8_t)t;
}

/*
 * Reverse of cif_vif_to_netif_wire(): map a host wire id back to the LMAC VIF
 * index currently held by that role. Since GO/GC float on vif0/vif1, the old
 * identity mapping (wire==index) no longer holds, so look the role up in the
 * active VIF table. Falls back to the wire id if no active match is found.
 */
static uint8_t cif_wire_to_lmac_vif(uint8_t wire)
{
    netif_if_t want_type;
    bool want_p2p;

    switch (wire)
    {
        case 0: want_type = NETIF_IF_STA; want_p2p = false; break;
        case 1: want_type = NETIF_IF_AP;  want_p2p = false; break;
        case 2: want_type = NETIF_IF_AP;  want_p2p = true;  break;
        case 3: want_type = NETIF_IF_STA; want_p2p = true;  break;
        default: return wire;
    }

    for (void *vif = mac_vif_mgmt_first_vif(); vif != NULL;
         vif = mac_vif_mgmt_next_vif(vif))
    {
        if ((wifi_netif_vif_to_netif_type(vif) == want_type) &&
            (cif_vif_is_p2p(vif) == want_p2p))
            return wifi_netif_vif_to_vifid(vif);
    }

    return wire;
}

static uint8_t cif_vif_id_route()
{
    uint8_t sta_vif_id = vif_mgmt_get_sta_vif_index();
    uint8_t sap_vif_id = vif_mgmt_get_softap_vif_index();
    uint8_t ap_id = INVALID_STA_IDX;
    void *vif = NULL;

    if ((sta_vif_id != INVALID_VIF_IDX) && (sap_vif_id != INVALID_VIF_IDX))
    {
        vif = mac_vif_mgmt_get_entry(sta_vif_id);
        if (vif)
            ap_id = mac_vif_mgmt_get_u_sta_ap_id(vif);

        if (ap_id != INVALID_STA_IDX)
        {
            return sta_vif_id + 0xF;
        }
        else
        {
            return sap_vif_id + 0xF;
        }
    }
    else if ((sta_vif_id != INVALID_VIF_IDX) && (sap_vif_id == INVALID_VIF_IDX))
    {
        return sta_vif_id + 0xF;
    }
    else if ((sta_vif_id == INVALID_VIF_IDX) && (sap_vif_id != INVALID_VIF_IDX))
    {
        return sap_vif_id + 0xF;
    }

    return INVALID_VIF_IDX;
}

bk_err_t cif_handle_txdata(void *head)
{
    uint8_t ret = BK_OK;
    struct pbuf* pbuf = NULL;

    cpdu_t* cpdu = (cpdu_t*)head;
    /* Map the host wire id (0..3) to the LMAC VIF index this role currently
     * holds (P2P GO/GC float on vif0/vif1), then tag it with the +0xF offset
     * the downstream TX path expects. */
    uint8_t vif_id = cif_wire_to_lmac_vif(cpdu->co_hdr.vif_idx) + 0xF;
    BK_ASSERT(vif_id < 19);
#if CONFIG_BK_RAW_LINK
    if (cpdu->co_hdr.special_type == TX_RAW_LINK_TYPE)
    {
        struct ctrl_cmd_hdr *cpdu = (struct ctrl_cmd_hdr*)head;
        uint32_t align_mac_len = CIF_RAW_LINK_MEM_ALIGN_SIZE(RLK_WIFI_MAC_ADDR_LEN);

        if (cpdu->msg_hdr.id == RLK_TX_SEND_EVT)
        {
             ret = bk_rlk_send((uint8_t *)head + sizeof(struct ctrl_cmd_hdr), 
                    (uint8_t *)head + sizeof(struct ctrl_cmd_hdr) + align_mac_len, cpdu->co_hdr.length);

            cpdu->co_hdr.special_type = TX_RLK_FREE_MEM_TYPE;
            // Notify AP side to free memory instead of freeing on CP side
            cif_send_mem_free_req(head);
        }

        return ret;
    }
#endif

    pbuf = (struct pbuf*)((uint8_t*)head - sizeof(struct pbuf));
#if CONFIG_CONTROLLER_RX_DIRECT_PSH
    if(cpdu->co_hdr.need_free)
    {
        cif_stats_ptr->cif_rxc_cnt++;
        CIF_LOGV("%s free p:%x,p->ref:%d\r\n",__func__, pbuf,pbuf->ref);
        pbuf->ref--;
        pbuf_free(pbuf);
        return BK_OK;
    }
#endif

    CIF_STATS_INC(buf_in_txdata);
    cif_stats_ptr->cif_tx_cnt++;
    CIF_LOGV("%s p:%x next:%x payload%x sizeof:%d\r\n",__func__, pbuf, pbuf->next, pbuf->payload, sizeof(struct pbuf));
    CIF_LOGV("%s p:%x,vif_id=%d\r\n",__func__, pbuf,vif_id);

#if CONFIG_CONTROLLER_DEBUG
    TRACK_PBUF_ALLOC(pbuf);
#endif

#if CONFIG_CONTROLLER_AP_BUFFER_COPY
    /*
     * In bridge mode AP-side bridgeif_send_to_port() already pbuf_clone's into
     * a PBUF_RAW_TX TX-headroom buffer (see bridgeif.c:1386). Cloning again on
     * CP wastes ~1.5KB heap per frame and serializes TX. Skip the second clone
     * when bridge is ENABLED and pass the pbuf to bmsg_tx_sender directly.
     */
#if CONFIG_BRIDGE
    if (bk_wifi_get_bridge_state() == BRIDGE_STATE_ENABLED)
    {
        ret = bmsg_tx_sender(pbuf, vif_id);
        if (ret != BK_OK)
        {
            cif_free_ap_txbuf(pbuf);
            ret = false;
        }
        return ret;
    }
#endif

    struct pbuf* p_copy = pbuf_clone(PBUF_RAW_TX,PBUF_RAM,pbuf);

    if((p_copy == NULL) || (p_copy->payload == NULL))
    {
        cif_free_ap_txbuf(pbuf);
        return BK_OK;
    }

    memcpy((void*)(p_copy+1),(void*)(pbuf+1),sizeof(cpdu_t));
    cif_free_ap_txbuf(pbuf);
    ret = bmsg_tx_sender(p_copy, vif_id);

    if(ret != BK_OK)
    {
        //CIF_LOGE("%s %d,p:0x%x\r\n",__func__,__LINE__,p_copy);
        pbuf_free(p_copy);
    }

#else
    ret = bmsg_tx_sender(pbuf, vif_id);
    if(ret != BK_OK)
    {
        cif_free_ap_txbuf(pbuf);
        ret = false;
    }
#endif
    return ret;
}

void cif_filter_add_customer_filter(uint32_t ip, uint16_t port)
{
    cif_env.filter.src_ip = ip;
    cif_env.filter.src_port = port;
}
static bool cif_filter_check_customer_filter(struct ip_hdr *iphdr, uint32_t src_port, uint32_t dst_port)
{
    if (cif_env.filter.src_ip == 0)
        return false;
    
    if (cif_env.filter.src_port == 0)
        return false;

    if ((iphdr->src.addr == cif_env.filter.src_ip) && (src_port == cif_env.filter.src_port))
    {
        return true;
    }

    return false;
}
static bool cif_filter_check_bk_filter(uint32_t src_port, uint32_t dst_port)
{
    if ((dst_port == DHCP_SERVER_PORT) || (dst_port == DHCP_CLIENT_PORT) || (dst_port == NAMESERVER_PORT))
    {
        return true;
    }

    if((dst_port >= LOCAL_PORT_RANGE_START) && (dst_port <= LOCAL_PORT_RANGE_END))
    {
        return true;
    }

    return false;
}
static bool cif_filter_check_ip_and_port(struct ip_hdr *iphdr, uint32_t src_port, uint32_t dst_port)
{
#if CONFIG_DEMOS_IPERF
    if(iperf_get_state() != 0)
    {
        return true;
    }
#endif
    if (cif_filter_check_customer_filter(iphdr, src_port, dst_port))
    {
        return true;
    }

    if (cif_filter_check_bk_filter(src_port, dst_port))
    {
        return true;
    }

    return false;
}
bool cif_filter_check_ip_data(struct pbuf *p)
{
    bool upload2ctrl = false;
    u16_t iphdr_hlen;
    u16_t offset_flags;
//	u16_t iphdr_len;
    uint32_t dest_port=0;
    uint32_t src_port=0;

    struct ip_hdr *iphdr;
    struct udp_hdr *udphdr;
    struct tcp_hdr *tcphdr;
    struct icmp_echo_hdr *iecho;
    if ((p->len <= SIZEOF_ETH_HDR) )//|| pbuf_header(p, (s16_t)-SIZEOF_ETH_HDR)) 
    {
        BK_ASSERT(0);
        return 0;
    }
    //stack_mem_dump((uint32_t)p->payload,(uint32_t)p->payload + 300);
    iphdr = (struct ip_hdr *)(p->payload + SIZEOF_ETH_HDR);
    
    /* obtain IP header length in number of 32-bit words */
    iphdr_hlen = IPH_HL(iphdr);
    /* calculate IP header length in bytes */
    iphdr_hlen *= 4;

    offset_flags = lwip_ntohs(IPH_OFFSET(iphdr));
    if (offset_flags & (IP_OFFMASK | IP_MF))
    {
        return false;
    }
  
    /* obtain ip length in bytes */
//	iphdr_len = lwip_ntohs(IPH_LEN(iphdr));
  
    CIF_LOGV("iphdr_hlen=%d,p->len=%d,p->tot_len=%d,IP_HLEN=%d\n",iphdr_hlen,p->len,p->tot_len,IP_HLEN);
    CIF_LOGV("IP RX dest_port = 0x%x\n",dest_port);
//   /* header length exceeds first pbuf length, or ip length exceeds total pbuf length? */
//   if ((iphdr_hlen > p->len) || (iphdr_len > p->tot_len) || (iphdr_hlen < IP_HLEN)) 
//   {
// 	BK_ASSERT(0);
// 	 return 0;
//   }
#if CONFIG_BRIDGE
    if (bk_wifi_get_bridge_state() == BRIDGE_STATE_ENABLED) {
        upload2ctrl = false;
        return upload2ctrl;
    }
#endif

  switch (IPH_PROTO(iphdr)) 
  {
     case IP_PROTO_UDP:
         udphdr = (struct udp_hdr *)(p->payload+(s16_t)iphdr_hlen+SIZEOF_ETH_HDR);
         dest_port=lwip_ntohs(udphdr->dest);
         src_port=lwip_ntohs(udphdr->src);
         CIF_LOGV("IP_PROTO_UDP dest_port:%d,src:%d\r\n",dest_port,src_port);
         upload2ctrl = cif_filter_check_ip_and_port(iphdr, src_port, dest_port);
         break;

     case IP_PROTO_TCP:
         tcphdr = (struct tcp_hdr *)(p->payload+(s16_t)iphdr_hlen+SIZEOF_ETH_HDR);
         dest_port=lwip_ntohs(tcphdr->dest);
         src_port=lwip_ntohs(tcphdr->src);
         CIF_LOGV("IP_PROTO_TCP port:%d\r\n",dest_port);
         upload2ctrl = cif_filter_check_ip_and_port(iphdr, src_port, dest_port);
         //BK_LOGD(NULL,"RX TCP src_ip:%x, src_port:%d\n", iphdr->src.addr, src_port);
         break;

    case IP_PROTO_ICMP:
        iecho = (struct icmp_echo_hdr *)((p->payload+(s16_t)iphdr_hlen+SIZEOF_ETH_HDR));
        if (iecho->type == ICMP_ECHO)
        {
            upload2ctrl = true;
        }
        else if (2 == get_ping_state()) //PING_STATE_STARTED
        {
            upload2ctrl = true;
        }
        else
        {
            upload2ctrl = false;
        }
        CIF_LOGV("IP_PROTO_ICMP,%p\r\n",p->payload);
        break;

    case IP_PROTO_IGMP:
        upload2ctrl = false;
        CIF_LOGV("IP_PROTO_IGMP\r\n");
        break; 
     default:
         break;
    }
    CIF_LOGV("%s %d dest_port = %d\r\n",__func__,__LINE__,dest_port);

    return upload2ctrl;
}

#if CONFIG_IPV6
/* IPv6 收包分流判定，语义与 cif_filter_check_ip_data() 一致：
 *   true  —— CP 核自己的协议栈仍需处理该包
 *   false —— 交由 AP 核独占处理，CP 核不得再收
 *
 * TCP/UDP 必须返回 false。CP 核与 AP 核的 netif 持有完全相同的 IPv6 全局地址，
 * 若两边都收，CP 核查不到对应 PCB，会按 lwIP 的 no-matching-PCB 流程回 RST，
 * 把 AP 核上正常的连接打死（表现为 TLS ClientHello 之前收到 ECONNRESET）。
 * ICMPv6（RA/RS/NS/NA/MLD）两边都要：CP 核靠它跑 SLAAC/ND，AP 核靠它维护自己的邻居表。
 * 解析失败或遇到未知情况一律返回 true，退回改动前的行为，避免误伤。
 */
static bool cif_filter_check_ip6_data(struct pbuf *p)
{
    u16_t offset;
    u8_t nexth;
    u8_t icmp6_type;
    int i;

    if (p->len < (SIZEOF_ETH_HDR + IP6_HLEN)) {
        return true;
    }

    /* 按固定偏移取 next header，不经由 struct ip6_hdr 指针。
     * 以太网头 14 字节，IPv6 头起始地址不是 4 字节对齐，而该结构体首成员是 u32，
     * 通过它访问会构成非对齐访问，在部分平台上触发对齐异常。
     * IPv6 固定头中 next header 位于偏移 6。 */
    nexth  = *((u8_t *)p->payload + SIZEOF_ETH_HDR + 6);
    offset = SIZEOF_ETH_HDR + IP6_HLEN;

    /* 逐个跳过扩展头（MLD 报文带 Hop-by-Hop）。限定层数，防畸形包死循环 */
    for (i = 0; i < 8; i++) {
        u8_t *ext;
        u16_t extlen;

        if ((nexth == IP6_NEXTH_TCP) || (nexth == IP6_NEXTH_UDP) ||
            (nexth == IP6_NEXTH_UDPLITE)) {
            CIF_LOGV("IPV6 L4 nexth:%d -> host only\r\n", nexth);
            return false;
        }

        /* 分片头必须单独处理：它固定 8 字节，第 2 个字节是 Reserved 而不是长度，
         * 套用下面的通用公式会算错。若漏掉它，分片的 TCP/UDP 会退回双核都收，
         * 而 CP 侧 LWIP_IPV6_REASS 是开的，重组后同样查不到 PCB 并回 RST。 */
        if (nexth == IP6_NEXTH_FRAGMENT) {
            if ((offset + 8) > p->len) {
                return true;
            }
            ext    = (u8_t *)p->payload + offset;
            nexth  = ext[0];
            offset = offset + 8;
            continue;
        }

        if (nexth == IP6_NEXTH_ICMP6) {
            /* ICMPv6 需要细分：只有 Echo Request 会产生「响应」，两个核都应答会让
             * 对端看到重复的 echo reply(ping6 显示 DUP!)。其余类型两边都收是安全的：
             *   - NS 虽会触发 NA，但两核回的 NA 内容完全一致(同 MAC、同 target)，
             *     对端只是重复更新同一条邻居缓存；而 CP 核跑 DAD 必须收 NS
             *   - MLD Query 同理，重复的成员报告对路由器无害
             *   - 差错报文(DUR/PTB/TE/PP)本身不产生响应，且 AP 核的连接需要 PTB 做 PMTU
             * 走到这里时 offset 正指向 ICMPv6 头，其首字节即 type。 */
            if ((offset + 1) > p->len) {
                return true;
            }
            icmp6_type = *((u8_t *)p->payload + offset);

            if (ICMP6_TYPE_EREQ == icmp6_type) {
                /* 交由 AP 核独占应答。此处与 IPv4 相反（IPv4 的 echo 给 CP 核），
                 * 原因不是「业务在 AP 核」——按那个逻辑 IPv4 也该给 AP——
                 * 而是两侧低功耗保活能力不对称：
                 *   IPv4：CP 核有 ARP 代答（net.c:1188 etharp_reply），AP 睡眠时二层仍可达，
                 *         echo 交给 CP 核才能构成完整的「睡眠可达」链路；
                 *   IPv6：CP 核无任何 NS/ND 代答（nd6_na_output / ns_offload 全仓零实现，
                 *         cif_low_power_handler() 亦是空壳），AP 睡眠后对端 NS 无人应答，
                 *         邻居缓存一过期对端连 ping 都发不出，echo 给 CP 核只能换来
                 *         缓存有效期内的一小段窗口，换不到 IPv4 那种睡眠可达性。
                 * 若日后给 CP 核补上 IPv6 邻居代答，应连同 echo 一起搬到 CP 核，与 IPv4 对齐。 */
                CIF_LOGV("IPV6 echo request -> host only\r\n");
                return false;
            }
            if (ICMP6_TYPE_EREP == icmp6_type) {
                /* 仅当 CP 核自己在跑 ping6 时才需要，与 IPv4 分支的判定保持一致 */
                return (2 == get_ping_state()); /* PING_STATE_STARTED */
            }

            return true;
        }

        if ((nexth != IP6_NEXTH_HOPBYHOP) &&
            (nexth != IP6_NEXTH_ROUTING)  &&
            (nexth != IP6_NEXTH_DESTOPTS)) {
            /* ESP/AH 等无法安全解析长度的头：维持两边都收 */
            return true;
        }

        if ((offset + 2) > p->len) {
            return true;
        }
        ext    = (u8_t *)p->payload + offset;
        extlen = ((u16_t)ext[1] + 1) * 8;
        nexth  = ext[0];
        offset = offset + extlen;

        if (offset >= p->len) {
            return true;
        }
    }

    return true;
}
#endif

#if CONFIG_BK_RAW_LINK
/**
 * @brief Send memory free request to AP side
 * @param mem_addr Memory address to be freed
 */
static void cif_send_mem_free_req(void *mem_addr)
{
    //CIF_LOGD("CP Send memory free request: addr=%p\r\n", mem_addr);

    if(cif_msg_sender(mem_addr,CIF_TASK_MSG_RX_DATA,0) != BK_OK)
    {
        CIF_STATS_INC(cif_tx_buf_leak);
        CIF_LOGE("%s,%d,addr send fail mem_leak:%d\n",__func__,__LINE__,mem_addr);
    }
}
#endif

static bk_err_t cif_upload_rx_packet_to_host(struct pbuf *p, void *vif, uint8_t dst_idx)
{
    struct pbuf *p_copy = NULL;
    struct cpdu_t *cpdu;
    bk_err_t ret;

#if CONFIG_CONTROLLER_RX_DIRECT_PSH
    p_copy = pbuf_alloc(PBUF_RAW, p->len + sizeof(cpdu_t), PBUF_RAM_RX);
    if (p_copy) {
        pbuf_header(p_copy, -(s16)sizeof(struct cpdu_t));
        memcpy(p_copy->payload, p->payload, p->len);
    }
#else
    p_copy = (struct pbuf *)cif_maclloc_rx_buf();
    if (p_copy) {
#ifdef CONFIG_CONTROLLER_WAR
        memcpy(p_copy->payload, p->payload, p->len);
#else
        dma_memcpy(p_copy->payload, p->payload, p->len);
#endif
        p_copy->len = p->len;
    }
#endif

    if (p_copy == NULL) {
        CIF_LOGV("%s,%d,alloc fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    cpdu = (struct cpdu_t *)(p_copy + 1);
    cpdu->co_hdr.length = p_copy->len - sizeof(struct pbuf);
    cpdu->co_hdr.type = RX_MSDU_DATA;
    cpdu->co_hdr.need_free = 0;
    cpdu->co_hdr.special_type = 0;
    cpdu->co_hdr.vif_idx = wifi_netif_vif_to_netif_type(vif);
    cpdu->co_hdr.dst_index = dst_idx;

    ret = cif_msg_sender(cpdu, CIF_TASK_MSG_RX_DATA, 0);
    if (ret != BK_OK) {
#if CONFIG_CONTROLLER_RX_DIRECT_PSH
        pbuf_free(p_copy);
#else
        cif_free_rx_buf((uint32_t)p_copy);
#endif
    } else {
        cif_stats_ptr->cif_rx_cnt++;
    }

    return ret;
}

bool cif_rx_local_packet_check(struct pbuf **p_ptr, struct eth_hdr * ethhdr,void* vif, uint8_t dst_idx)
{
    bool upload2ctrl = true;
    struct pbuf *p = *p_ptr;
    bk_err_t ret = BK_OK;

    CIF_LOGV("%s p:%x next:0x%x payload:0x%x sizeof:%d\r\n",__func__, p, p->next, p->payload, sizeof(struct pbuf));

    if (cif_env.no_host)
    {
         CIF_LOGV("%s no host connected, upload to controller\r\n",__func__, upload2ctrl);
         return true;
    }

    if (!cif_env.host_wifi_init)
    {
        CIF_LOGV("%s AP Wi-Fi does not start, upload to controller\r\n",__func__);
        return true;
    }

    switch (htons(ethhdr->type))
    {
        case ETHTYPE_EAPOL:
        {
            CIF_LOGV("ETHTYPE_EAPOL RX\n");
            upload2ctrl = true;
            break;
        }
        case ETHTYPE_ARP:
        {
            struct pbuf* p_copy = NULL;
            CIF_LOGV("ARP RX\n");

#if CONFIG_CONTROLLER_RX_DIRECT_PSH
#if CONFIG_BRIDGE
            /*
             * Zero-copy ARP when bridge is ENABLED — same pattern as the
             * IP fast-path above. ARP processing (proxy reply / flood) is
             * owned by AP bridgeif, so we pass the original pbuf directly.
             */
            if (bk_wifi_get_bridge_state() == BRIDGE_STATE_ENABLED)
            {
                p_copy = p;
                upload2ctrl = false;
            }
            else
#endif
            {
                p_copy = pbuf_alloc(PBUF_RAW,p->len+sizeof(cpdu_t),PBUF_RAM_RX);
                if(p_copy)
                {
                    pbuf_header(p_copy, -(s16)sizeof(struct cpdu_t));
                    memcpy(p_copy->payload,p->payload,p->len);
                }
                else
                {
                    return upload2ctrl;
                }
                if(cif_is_arp_request(p))
                {
                    upload2ctrl = false;
                    pbuf_free(p);
                }
            }
#else
            p_copy = (struct pbuf*)cif_maclloc_rx_buf();

            upload2ctrl = true;

            if (p_copy == NULL)
            {
                CIF_LOGV("%s,%d,alloc fail\n",__func__,__LINE__);
                return upload2ctrl;
            }
            else
            {
                //pbuf_copy(p_copy, p);
                #ifdef CONFIG_CONTROLLER_WAR
                memcpy(p_copy->payload,p->payload,p->len);
                #else
                dma_memcpy(p_copy->payload,p->payload,p->len);
                #endif
                p_copy->len = p->len;
            }
#endif            
            //bk_mem_dump("Meth input p",(uint32_t)p,sizeof(struct pbuf)+8);
            //bk_mem_dump("Meth input payload",(uint32_t)p->payload,30);
            
            CIF_LOGV("%s,%d p:%p next:%p payload:%p len:%d\r\n",
                __func__,__LINE__, p_copy, p_copy->next, p_copy->payload, p_copy->tot_len);

            //pbuf_header_force(p, (s16)macif_get_rxl_payload_offset() + sizeof(struct cpdu_t));

            struct cpdu_t *cpdu = (struct cpdu_t*)(p_copy + 1);
            cpdu->co_hdr.length = p_copy->len - sizeof(struct pbuf);
            cpdu->co_hdr.type = RX_MSDU_DATA;
            cpdu->co_hdr.need_free = 0;
            cpdu->co_hdr.special_type = 0;
            cpdu->co_hdr.vif_idx = cif_vif_to_netif_wire(vif);
            cpdu->co_hdr.dst_index = dst_idx;
            //bk_mem_dump("cif_filter before snder",(uint32_t)p_copy->payload,100);
            ret = cif_msg_sender(cpdu,CIF_TASK_MSG_RX_DATA,0);
            if(ret != BK_OK)
            {
                #if CONFIG_CONTROLLER_RX_DIRECT_PSH
                pbuf_free(p_copy);
                #else
                //If rxbuf push fail, free it immediately
                cif_free_rx_buf((uint32_t)p_copy);
                #endif
            }else
            {
                cif_stats_ptr->cif_rx_cnt++;
            }

            break;
        }
        case ETHTYPE_IP:
        {
            if (cif_filter_check_ip_data(p) == false)
            {
                //pbuf_header_force(p, (s16)macif_get_rxl_payload_offset() + sizeof(struct cpdu_t));
                /*
                * +-----  host_id (struct pbuf{} *)
                * |
                * V
                * +--------------+-------------+---------------------+
                * |  common hdr  | fhost hdr   | IEEE 802.3 Data     |
                * +--------------+-------------+---------------------+
                */
                struct pbuf* p_copy = NULL;

#if CONFIG_CONTROLLER_RX_DIRECT_PSH
                p_copy =  p;
                upload2ctrl = false;
#else
                p_copy = (struct pbuf*)cif_maclloc_rx_buf();
                
                if (p_copy == NULL)
                {
                    CIF_LOGV("%s,%d,alloc fail\n",__func__,__LINE__);
                    pbuf_free(p);
                    upload2ctrl = false;
                    return upload2ctrl;
                }

                BK_ASSERT(p_copy->payload);
                #ifdef CONFIG_CONTROLLER_WAR
                memcpy(p_copy->payload,p->payload,p->len);
                #else
                dma_memcpy(p_copy->payload,p->payload,p->len);
                #endif
                p_copy->len = p->len;

                pbuf_free(p);
#endif
                struct cpdu_t *cpdu = (struct cpdu_t*)(p_copy + 1);
                cpdu->co_hdr.length = p_copy->len - sizeof(struct pbuf);
                cpdu->co_hdr.type = RX_MSDU_DATA;
                cpdu->co_hdr.need_free = 0;
                cpdu->co_hdr.special_type = 0;
                cpdu->co_hdr.vif_idx = cif_vif_to_netif_wire(vif);
                cpdu->co_hdr.dst_index = dst_idx;
                CIF_LOGV("%s,%d p:%p next:%p payload:%p len:%d\r\n",
                    __func__,__LINE__, p_copy, p_copy->next, p_copy->payload, p_copy->tot_len);

                ret = cif_msg_sender(cpdu,CIF_TASK_MSG_RX_DATA,0);
                if(ret != BK_OK)
                {
                    #if CONFIG_CONTROLLER_RX_DIRECT_PSH
                    pbuf_free(p_copy);
                    #else
                    //If rxbuf push fail, free it immediately
                    cif_free_rx_buf((uint32_t)p_copy);
                    #endif
                }else
                {
                    cif_stats_ptr->cif_rx_cnt++;
                }


                upload2ctrl = false;
            }
            else
            {
                upload2ctrl = true;
            }
            break;
        }
#if CONFIG_IPV6
        case ETHTYPE_IPV6:
        {
            CIF_LOGV("ETHTYPE_IPV6 RX\n");
            if (cif_filter_check_ip6_data(p) == false)
            {
                /* TCP/UDP：与 ETHTYPE_IP 一致，零拷贝独占转交 AP 核，
                 * CP 核不得再收，否则会因查不到 PCB 而回 RST 打死对端连接。 */
                struct pbuf* p_copy = NULL;

#if CONFIG_CONTROLLER_RX_DIRECT_PSH
                p_copy = p;
                upload2ctrl = false;
#else
                p_copy = (struct pbuf*)cif_maclloc_rx_buf();

                if (p_copy == NULL)
                {
                    CIF_LOGV("%s,%d,alloc fail\n",__func__,__LINE__);
                    pbuf_free(p);
                    upload2ctrl = false;
                    return upload2ctrl;
                }

                BK_ASSERT(p_copy->payload);
                #ifdef CONFIG_CONTROLLER_WAR
                memcpy(p_copy->payload,p->payload,p->len);
                #else
                dma_memcpy(p_copy->payload,p->payload,p->len);
                #endif
                p_copy->len = p->len;

                pbuf_free(p);
#endif
                struct cpdu_t *cpdu = (struct cpdu_t*)(p_copy + 1);
                cpdu->co_hdr.length = p_copy->len - sizeof(struct pbuf);
                cpdu->co_hdr.type = RX_MSDU_DATA;
                cpdu->co_hdr.need_free = 0;
                cpdu->co_hdr.special_type = 0;
                cpdu->co_hdr.vif_idx = wifi_netif_vif_to_netif_type(vif);
                cpdu->co_hdr.dst_index = dst_idx;
                CIF_LOGV("%s,%d ipv6 p:%p next:%p payload:%p len:%d\r\n",
                    __func__,__LINE__, p_copy, p_copy->next, p_copy->payload, p_copy->tot_len);

                ret = cif_msg_sender(cpdu,CIF_TASK_MSG_RX_DATA,0);
                if(ret != BK_OK)
                {
                    #if CONFIG_CONTROLLER_RX_DIRECT_PSH
                    pbuf_free(p_copy);
                    #else
                    //If rxbuf push fail, free it immediately
                    cif_free_rx_buf((uint32_t)p_copy);
                    #endif
                }else
                {
                    cif_stats_ptr->cif_rx_cnt++;
                }

                upload2ctrl = false;
            }
            else
            {
                /* ICMPv6（RA/RS/NS/NA/MLD）等：复制一份给 AP 核维护其邻居表，
                 * 原包仍进 CP 核协议栈，CP 核的 SLAAC/ND 依赖它。 */
                cif_upload_rx_packet_to_host(p, vif, dst_idx);
                upload2ctrl = true;
            }
            break;
        }
#endif
        default:
        {
            upload2ctrl = true;
            break;
        }
    }

    return upload2ctrl;
}
