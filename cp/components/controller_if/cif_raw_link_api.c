#include "net.h"
#include "bk_wifi_types.h"
#include "modules/wifi.h"
#include "bk_wifi.h"
#include <stdlib.h>
#include <string.h>
#include "cif_wifi_api.h"
#include "cif_main.h"
#include "cif_ipc.h"
#if CONFIG_BK_RAW_LINK
#include <modules/raw_link.h>
#include "cif_raw_link_api.h"
#endif

#if CONFIG_BK_RAW_LINK

bk_err_t cif_common_msg_sender(struct pbuf *p_copy, uint8_t s_type, uint8_t s_special_type, uint16_t s_id, uint16_t param_length)
{
    struct ctrl_cmd_hdr *cpdu = NULL;
    bk_err_t ret =BK_OK;

    cpdu = (struct ctrl_cmd_hdr*)(p_copy + 1);
    cpdu->co_hdr.length = p_copy->len - sizeof(struct pbuf);
    cpdu->co_hdr.type = s_type;
    cpdu->co_hdr.need_free = 0;
    cpdu->co_hdr.vif_idx = 0;
    cpdu->co_hdr.special_type = s_special_type;
    cpdu->msg_hdr.id = s_id;
    cpdu->msg_hdr.param_len = param_length;
    
    ret = cif_msg_sender(cpdu,CIF_TASK_MSG_RX_DATA,0);
    return ret;
}

bk_err_t bk_rlk_scan_cfm_cp_cb(bk_rlk_scan_result_t *result)
{
    struct rlk_scan_cfm_struct
    {
        cpdu_t cp;
        struct bk_rx_msg_hdr rx_msg_hdr;
        uint32_t para[3];
        uint32_t payload[1];
    };

    uint16_t param_length = sizeof(bk_rlk_scan_result_t)+ sizeof(bk_rlk_scan_master_info_t) + sizeof(uint32_t);
    uint32_t total_len = sizeof(struct rlk_scan_cfm_struct) + param_length;

    struct pbuf* p_copy = NULL;
    uint8_t* temp_payload = NULL;
    bk_err_t ret =BK_OK;
    struct rlk_scan_cfm_struct* hdr = NULL;

    p_copy = pbuf_alloc(PBUF_RAW,total_len,PBUF_RAM_RX);
    hdr = (struct rlk_scan_cfm_struct*)(p_copy + 1);

    if(p_copy)
    {
        //result
        memcpy(hdr->payload,result,sizeof(bk_rlk_scan_result_t));
        hdr->para[0] = (uint32_t)hdr->payload;

        temp_payload = (uint8_t*)hdr->payload + sizeof(bk_rlk_scan_result_t);
        memcpy(temp_payload,result->masters,sizeof(bk_rlk_scan_master_info_t));
        ((bk_rlk_scan_result_t *)hdr->payload)->masters = (bk_rlk_scan_master_info_t *)temp_payload;
    }
    else
    {
        return BK_FAIL;
    }

    ret = cif_common_msg_sender(p_copy, RX_MSDU_DATA, RX_RAW_LINK_TYPE, RLK_REGISTER_SCAN_CFM_CB_IND, param_length);

    if(ret != BK_OK)
    {
        #if CONFIG_CONTROLLER_RX_DIRECT_PSH
        pbuf_free(p_copy);
        #else
        //If rxbuf push fail, free it immediately
        cif_free_rx_buf((uint32_t)p_copy);
        #endif
    }
    return ret;
}

bk_err_t bk_rlk_acs_cfm_cp_cb(const uint32_t chanstatus[],uint32_t num_channel,uint32_t best_channel)
{
    struct rlk_acs_cfm_struct
    {
        cpdu_t cp;
        struct bk_rx_msg_hdr rx_msg_hdr;
        uint32_t para[3];
        uint32_t payload[1];
    };

    uint16_t param_length = num_channel * sizeof(uint32_t) + sizeof(uint32_t)+ sizeof(uint32_t) + sizeof(uint32_t);
    uint32_t total_len = sizeof(struct rlk_acs_cfm_struct) + param_length;

    struct pbuf* p_copy = NULL;
    bk_err_t ret =BK_OK;
    struct rlk_acs_cfm_struct* hdr = NULL;

    p_copy = pbuf_alloc(PBUF_RAW,total_len,PBUF_RAM_RX);
    hdr = (struct rlk_acs_cfm_struct*)(p_copy + 1);

    if(p_copy)
    {
        //chanstatus[]
        memcpy(hdr->payload,chanstatus,(num_channel * sizeof(uint32_t)));
        hdr->para[0] = (uint32_t)hdr->payload;
        //num_channel
        hdr->para[1] = num_channel;
        //best_channel
        hdr->para[2] = best_channel;
    }
    else
    {
        return BK_FAIL;
    }

    ret = cif_common_msg_sender(p_copy, RX_MSDU_DATA, RX_RAW_LINK_TYPE, RLK_REGISTER_ACS_CFM_CB_IND, param_length);

    if(ret != BK_OK)
    {
        #if CONFIG_CONTROLLER_RX_DIRECT_PSH
        pbuf_free(p_copy);
        #else
        //If rxbuf push fail, free it immediately
        cif_free_rx_buf((uint32_t)p_copy);
        #endif
    }
    return ret;
}

bk_err_t bk_rlk_recv_cp_cb(bk_rlk_recv_info_t *rx_info)
{
    struct rlk_recv_struct
    {
        cpdu_t cp;
        struct bk_rx_msg_hdr rx_msg_hdr;
        uint32_t para[3];
        uint32_t payload[1];
    };

    uint16_t param_length = sizeof(bk_rlk_recv_info_t) + rx_info->len + sizeof(uint32_t);
    uint32_t total_len = sizeof(struct rlk_recv_struct) + param_length;

    struct pbuf* p_copy = NULL;
    uint8_t* temp_payload = NULL;
    bk_err_t ret =BK_OK;
    struct rlk_recv_struct* hdr = NULL;

    p_copy = pbuf_alloc(PBUF_RAW,total_len,PBUF_RAM_RX);
    hdr = (struct rlk_recv_struct*)(p_copy + 1);

    if(p_copy)
    {
        //rx_info
        memcpy(hdr->payload,rx_info,sizeof(bk_rlk_recv_info_t));
        hdr->para[0] = (uint32_t)hdr->payload;
        //data
        temp_payload = (uint8_t*)hdr->payload + sizeof(bk_rlk_recv_info_t);
        memcpy(temp_payload,rx_info->data,rx_info->len);
        ((bk_rlk_recv_info_t *)hdr->payload)->data = (uint8_t *)temp_payload;
    }
    else
    {
        return BK_FAIL;
    }

    ret = cif_common_msg_sender(p_copy, RX_MSDU_DATA, RX_RAW_LINK_TYPE, RLK_REGISTER_RECV_CB_IND, param_length);

    if(ret != BK_OK)
    {
        #if CONFIG_CONTROLLER_RX_DIRECT_PSH
        pbuf_free(p_copy);
        #else
        //If rxbuf push fail, free it immediately
        cif_free_rx_buf((uint32_t)p_copy);
        #endif
    }
    return ret;
}

void bk_rlk_send_ex_cp_cb(void *args, bool status)
{
    struct rlk_send_ex_struct
    {
        cpdu_t cp;
        struct bk_rx_msg_hdr rx_msg_hdr;
        uint32_t para[3];
        uint32_t payload[1];
    };

    uint16_t param_length = sizeof(uint32_t) + sizeof(bool) + sizeof(uint32_t);
    uint32_t total_len = sizeof(struct rlk_send_ex_struct) + param_length;

    struct pbuf* p_copy = NULL;
    bk_err_t ret =BK_OK;
    struct rlk_send_ex_struct* hdr = NULL;

    p_copy = pbuf_alloc(PBUF_RAW,total_len,PBUF_RAM_RX);
    hdr = (struct rlk_send_ex_struct*)(p_copy + 1);

    if(p_copy)
    {
        //void *args
        hdr->para[0] = (uint32_t)args;
        //status
        hdr->para[1] = (uint32_t)status;
        //bk_mem_dump("cp",(uint32_t)p_copy,200);
        //BK_LOGD(NULL,"%s,%d,frame:0x%x,len:%d,frame_info:0x%x\n",hdr->para[2],hdr->para[0],hdr->para[1]);
    }
    else
    {
        return;
    }

    ret = cif_common_msg_sender(p_copy, RX_MSDU_DATA, RX_RAW_LINK_TYPE, RLK_REGISTER_SEND_EX_CB_IND, param_length);

    if(ret != BK_OK)
    {
        #if CONFIG_CONTROLLER_RX_DIRECT_PSH
        pbuf_free(p_copy);
        #else
        //If rxbuf push fail, free it immediately
        cif_free_rx_buf((uint32_t)p_copy);
        #endif
    }
}

bk_err_t bk_rlk_send_cp_cb(const uint8_t *peer_mac_addr, bk_rlk_send_status_t status)
{
    struct rlk_send_struct
    {
        cpdu_t cp;
        struct bk_rx_msg_hdr rx_msg_hdr;
        uint32_t para[3];
        uint32_t payload[1];
    };

    uint16_t param_length = CIF_RAW_LINK_MEM_ALIGN_SIZE(RLK_WIFI_MAC_ADDR_LEN) + sizeof(bk_rlk_send_status_t) + sizeof(uint32_t);
    uint32_t total_len = sizeof(struct rlk_send_struct) + param_length;

    struct pbuf* p_copy = NULL;
    uint8_t* temp_payload = NULL;
    bk_err_t ret =BK_OK;
    struct rlk_send_struct* hdr = NULL;

    p_copy = pbuf_alloc(PBUF_RAW,total_len,PBUF_RAM_RX);
    hdr = (struct rlk_send_struct*)(p_copy + 1);

    if(p_copy)
    {
        //peer_mac_addr
        memcpy(hdr->payload,peer_mac_addr,RLK_WIFI_MAC_ADDR_LEN);
        hdr->para[0] = (uint32_t)hdr->payload;
        //status
        temp_payload = (uint8_t*)hdr->payload + CIF_RAW_LINK_MEM_ALIGN_SIZE(RLK_WIFI_MAC_ADDR_LEN);
        memcpy(temp_payload,&status,sizeof(bk_rlk_send_status_t));
        hdr->para[1] = (uint32_t)temp_payload;
        //bk_mem_dump("cp",(uint32_t)p_copy,200);
        //BK_LOGD(NULL,"%s,%d,frame:0x%x,len:%d,frame_info:0x%x\n",hdr->para[2],hdr->para[0],hdr->para[1]);
    }
    else
    {
        return BK_FAIL;
    }

    ret = cif_common_msg_sender(p_copy, RX_MSDU_DATA, RX_RAW_LINK_TYPE, RLK_REGISTER_SEND_CB_IND, param_length);

    if(ret != BK_OK)
    {
        #if CONFIG_CONTROLLER_RX_DIRECT_PSH
        pbuf_free(p_copy);
        #else
        //If rxbuf push fail, free it immediately
        cif_free_rx_buf((uint32_t)p_copy);
        #endif
    }
    return ret;
}

#endif
