#include "cif_wifi_dp.h"

#include "lwip/prot/ethernet.h"
#include "lwip/prot/ip4.h"
#include "lwip/prot/ip.h"
#include "lwip/prot/udp.h"
#include "lwip/prot/tcp.h"
#include "lwip/prot/icmp.h"
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
    uint8_t vif_id = cpdu->co_hdr.vif_idx + 0xF;
    BK_ASSERT(vif_id < 17);
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

    // if (!cif_env.host_wifi_init)
    // {
    //      CIF_LOGV("%s AP Wi-Fi does not start, upload to controller\r\n",__func__);
    //      return true;
    // }

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
            p_copy = pbuf_alloc(PBUF_RAW,p->len+sizeof(cpdu_t),PBUF_RAM_RX);
#if CONFIG_BRIDGE
            upload2ctrl = false;
#endif
            if(p_copy)
            {
                pbuf_header(p_copy, -(s16)sizeof(struct cpdu_t));
                memcpy(p_copy->payload,p->payload,p->len);
            }
            else
            {
                return upload2ctrl;
            }
#if CONFIG_BRIDGE
            pbuf_free(p);
#else
            if(cif_is_arp_request(p))
            {
                upload2ctrl = false;
                pbuf_free(p);
            }
#endif
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
            cpdu->co_hdr.vif_idx = wifi_netif_vif_to_netif_type(vif);
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
                cpdu->co_hdr.vif_idx = wifi_netif_vif_to_netif_type(vif);
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
        default:
        {
            upload2ctrl = true;
            break;
        }
    }

    return upload2ctrl;
}
