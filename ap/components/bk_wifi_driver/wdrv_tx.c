#include "wdrv_tx.h"
#include "wdrv_ipc.h"
#include "wdrv_main.h"
#include "wdrv_cntrl.h"
#if CONFIG_CACHE_MAINTENANCE
#include "cache.h"
#endif
#if CONFIG_CONTROLLER_AP_BUFFER_COPY
cp_mem_addr_info_t g_cp_mem_addr_info = {0};
#endif
void __asm_flush_dcache_range(void* begin, void* end);

#if CONFIG_CONTROLLER_AP_BUFFER_COPY
static void wdrv_flush_pbuf_for_cp(struct pbuf *p)
{
    uint32_t start;
    uint32_t end;

    if (p == NULL) {
        return;
    }

    if (p->next != NULL) {
        WDRV_LOGW("%s chained pbuf is not supported, flush head only\r\n", __func__);
    }

    start = PTR_TO_U32(p);
    end = start + sizeof(struct pbuf);
    if (p->payload != NULL) {
        uint32_t payload_end = PTR_TO_U32(p->payload) + p->len;
        if (payload_end > end) {
            end = payload_end;
        }
    }

#if CONFIG_CACHE_MAINTENANCE
    flush_dcache(p, (long)(end - start));
#endif
}

static void wdrv_flush_tx_buffer_for_cp(uint8_t channel, void *head, uint8_t num)
{
    struct cpdu_t *cpdu = (struct cpdu_t *)head;

    if (cpdu == NULL) {
        return;
    }

    if (channel == TX_BK_CMD_DATA) {
        if (cpdu->co_hdr.is_buf_bank) {
            ipc_addr_bank_t *bank = (ipc_addr_bank_t *)cpdu;
            uint32_t bank_len = sizeof(ipc_addr_bank_t);

            for (uint8_t i = 0; i < bank->num; i++) {
                wdrv_flush_pbuf_for_cp(PTR_FROM_U32(struct pbuf, bank->addr[i]));
            }
            if (bank->num > 1) {
                bank_len += (bank->num - 1) * sizeof(bank->addr[0]);
            }
#if CONFIG_CACHE_MAINTENANCE
            flush_dcache(bank, (long)bank_len);
#endif
        } else {
#if CONFIG_CACHE_MAINTENANCE
            flush_dcache(cpdu, cpdu->co_hdr.length);
#endif
        }
        return;
    }

    if (channel != TX_MSDU_DATA) {
        return;
    }

    for (uint8_t i = 0; (cpdu != NULL) && (i < num); i++) {
        cpdu_t *next = cpdu->next;

        if (cpdu->co_hdr.special_type == 0) {
            struct pbuf *p = ((struct pbuf *)cpdu) - 1;
            wdrv_flush_pbuf_for_cp(p);
        } else {
#if CONFIG_CACHE_MAINTENANCE
            flush_dcache(cpdu, cpdu->co_hdr.length + sizeof(struct ctrl_cmd_hdr));
#endif
        }

        cpdu = next;
    }
}

static uint32_t wdrv_cp_read_u32_addr(uint32_t addr, uint32_t size)
{
    if (addr == 0) {
        return 0;
    }

    if (size == sizeof(uint16_t)) {
        return *(volatile uint16_t *)addr;
    }
    return *(volatile uint32_t *)addr;
}

static bool wdrv_cp_mem_addr_ready(void)
{
    return ((g_cp_mem_addr_info.magic == CP_MEM_SNAPSHOT_MAGIC) &&
            (g_cp_mem_addr_info.version == CP_MEM_SNAPSHOT_VERSION) &&
            (g_cp_mem_addr_info.size == sizeof(cp_mem_addr_info_t)));
}

bool wdrv_cp_mem_tx_allowed(void)
{
    uint32_t tx_used, tx_avail, mem_used, mem_avail;
    uint32_t heap_free, heap_min_rsv;
    uint32_t tx_pct = 0, mem_pct = 0;
    bool lwip_avail, heap_low, heap_ok;
    bool mem_tight, mem_eased;

    if (!wdrv_cp_mem_addr_ready()) {
        return !wdrv_env.is_controlled;
    }

    tx_avail     = wdrv_cp_read_u32_addr(g_cp_mem_addr_info.lwip_tx_avail_addr,
                                         g_cp_mem_addr_info.lwip_mem_value_size);
    tx_used      = wdrv_cp_read_u32_addr(g_cp_mem_addr_info.lwip_tx_used_addr,
                                         g_cp_mem_addr_info.lwip_mem_value_size);
    mem_avail    = wdrv_cp_read_u32_addr(g_cp_mem_addr_info.lwip_avail_addr,
                                         g_cp_mem_addr_info.lwip_mem_value_size);
    mem_used     = wdrv_cp_read_u32_addr(g_cp_mem_addr_info.lwip_used_addr,
                                         g_cp_mem_addr_info.lwip_mem_value_size);
    heap_free    = wdrv_cp_read_u32_addr(g_cp_mem_addr_info.heap_free_addr,
                                         g_cp_mem_addr_info.heap_value_size);
    heap_min_rsv = wdrv_cp_read_u32_addr(g_cp_mem_addr_info.heap_min_rsv_addr,
                                         g_cp_mem_addr_info.heap_min_rsv_value_size);

    /* lwIP availability is decided by the pool CAPACITY (tx_avail / mem_avail):
     * a zero capacity means CP did not publish the metric / stats are off. */
    lwip_avail = ((tx_avail != 0) && (mem_avail != 0));
    if (lwip_avail) {
        tx_pct  = 100 * tx_used / tx_avail;
        mem_pct = 100 * mem_used / mem_avail;
    }

    /* CP OS heap addresses are validated once at init (wdrv_rx_handle_cmd_confirm),
     * so when wdrv_cp_mem_addr_ready() passes they are guaranteed valid here.
     * heap_free == 0 is a valid CRITICAL state (heap exhausted) -> must stop TX. */
    /* low water: at/below the reserve (heap_free == 0 lands here) -> throttle */
    heap_low = (heap_free <= heap_min_rsv);
    /* high water: 25% above the reserve -> allow recovery (avoids flapping) */
    heap_ok  = (heap_free > (heap_min_rsv + heap_min_rsv / 4));

    /* memory is tight when the lwIP TX pool OR the lwIP total heap crosses the
     * high water mark, OR the CP OS heap drops to/below its reserve */
    mem_tight = (lwip_avail && ((tx_pct >= 75) || (mem_pct > 80))) || heap_low;
    /* memory has eased only when BOTH lwIP metrics are below the low water mark
     * AND the CP OS heap is comfortably above its reserve */
    mem_eased = (((tx_pct < 60) && (mem_pct < 70)) && heap_ok);

    if (wdrv_env.is_controlled) {
        if (mem_eased) {
            wdrv_env.is_controlled = 0;
        }
    } else {
        if (mem_tight) {
            wdrv_env.is_controlled = 1;
        }
    }

    return !wdrv_env.is_controlled;
}
#endif

#if CONFIG_BK_RAW_LINK
int wdrv_special_txdata_sender(void *head, uint32_t vif_idx)
{
	bk_err_t ret;
	struct wdrv_msg msg;
	struct cpdu_t * cpdu = (struct cpdu_t *)head;

    WDRV_LOGV("%s head:%d\r\n",__func__, head);
	msg.type = WDRV_TASK_MSG_TXDATA;
	msg.arg = (uint32_t)cpdu;
	msg.retry_flag = 0;

	cpdu->co_hdr.vif_idx = vif_idx;
	cpdu->co_hdr.type = TX_MSDU_DATA;
	cpdu->next = NULL;

    if(!cpdu->co_hdr.need_free)
    {
        WDRV_STATS_INC(wdrv_tx_cnt,1);
        WDRV_STATS_INC(tx_alloc_num,1);
    }
    else
    {
        WDRV_STATS_INC(wdrv_rxc_cnt,1);
    }

	ret = rtos_push_to_queue(&wdrv_env.io_queue, &msg, 1 * SECONDS);
	if (kNoErr != ret) {
		WDRV_LOGE("%s failed, ret=%d\r\n",__func__, ret);
        WDRV_STATS_INC(wdrv_tx_snder_fail,1);
		os_free(head);
	}

	return ret;
}
#endif

int wdrv_txdata_sender(struct pbuf *p, uint32_t vif_idx)
{
	bk_err_t ret;
	struct wdrv_msg msg;
	struct cpdu_t * cpdu = (struct cpdu_t *)(p + 1);
	
//    if(((void*)cpdu< (void*)&__wifi_start) || ((void*)cpdu> (void*)&__wifi_end))
//    {
//        BK_ASSERT(0);
//    }
#if CONFIG_CONTROLLER_AP_BUFFER_COPY
    // Only perform flow-control checks for buffers that do NOT require free
    if(!cpdu->co_hdr.need_free)
    {
        /* Single decision over lwIP + CP OS heap (updates is_controlled). */
        if(!wdrv_cp_mem_tx_allowed())
        {
            return BK_ERR_NO_MEM;
        }
    }
#endif
    WDRV_LOGV("%s p:%x next:%x payload%x sizeof:%d\r\n",__func__, p, p->next, p->payload, sizeof(struct pbuf));
	msg.type = WDRV_TASK_MSG_TXDATA;
	msg.arg = (uint32_t)cpdu;
	msg.retry_flag = 0;

	cpdu->co_hdr.vif_idx = vif_idx;
	cpdu->co_hdr.type = TX_MSDU_DATA;
	cpdu->next = NULL;
#if CONFIG_CONTROLLER_DEBUG
    if(!cpdu->co_hdr.need_free)
        TRACK_PBUF_ALLOC(p);
#endif
    if(!cpdu->co_hdr.need_free)
    {
        WDRV_STATS_INC(wdrv_tx_cnt,1);
        WDRV_STATS_INC(tx_alloc_num,1);
    }
    else
    {
        WDRV_STATS_INC(wdrv_rxc_cnt,1);
    }
	pbuf_ref(p);
	ret = rtos_push_to_queue(&wdrv_env.io_queue, &msg, 1 * SECONDS);
	if (kNoErr != ret) {
		WDRV_LOGE("%s failed, ret=%d\r\n",__func__, ret);
        WDRV_STATS_INC(wdrv_tx_snder_fail,1);
		pbuf_free(p);
	}

	return ret;
}

bk_err_t wdrv_txbuf_push(uint8_t channel,void* head,void* tail,uint8_t num)
{
    //struct cpdu_t* buf = (struct cpdu_t*)head;
    ipc_chnl_node_t ipc_node = {0};
    bk_err_t ret = BK_OK;
    BK_ASSERT(head);

    // if((head< (void*)&__wifi_start) || (head> (void*)&__wifi_end))
    // {
    //     BK_ASSERT(0);
    // }
    // if((tail< (void*)&__wifi_start) || (tail> (void*)&__wifi_end))
    // {
    //     BK_ASSERT(0);
    // }
   // __asm_flush_dcache_range(head-64,head+2048);

    //bk_mem_dump("push",PTR_TO_U32(head),20);
    WDRV_LOGV("%s, channel=%d,head=0x%x,tail=0x%x,num:%d\n",__func__, channel,head,tail,num);

    switch(channel)
    {
        case TX_BK_CMD_DATA:
        {
            ipc_node.channel = RX_BK_CMD_DATA;
            ipc_node.head = PTR_TO_U32(head);
            ipc_node.tail = PTR_TO_U32(tail);
            ipc_node.num = num;

            ret = wdrv_ipc_env[IPC_CMD].send(WIFI_IPC_CMD_CHNL,(mb_chnl_cmd_t*)&ipc_node);

            break;
        }
        case TX_MSDU_DATA:
        {
            ipc_node.channel = TX_MSDU_DATA;
            ipc_node.head = PTR_TO_U32(head);
            ipc_node.tail = PTR_TO_U32(tail);
            ipc_node.num = num;
            
            WDRV_LOGV("%s,%d,ipc_node head=0x%x,ipc_node channel=0x%x \n",__func__,__LINE__,ipc_node.head,*(&ipc_node.tail + 1));
            
            //bk_mem_dump("A_TX",PTR_TO_U32((struct pbuf*)head -1),100);
            ret = wdrv_ipc_env[IPC_DATA].send(WIFI_IPC_DATA_CHNL,(mb_chnl_cmd_t*)&ipc_node);

            if(ret == BK_OK)
            {
                wdrv_stats_ptr->ipc_tx_cnt++;
            }else{
                WDRV_LOGE("%s,%d,error type:%d\n",__func__,__LINE__,ret);
                wdrv_stats_ptr->ipc_tx_fail_cnt++;
            }
//            if(ret != BK_OK)
//            {
//                WDRV_LOGE("%s,%d,error type:%d\n",__func__,__LINE__,ret);
//                pbuf_free((void*)((struct cpdu_t*)head - 1));
//                break;
//            }
            break;
        }
        default:
        {
            WDRV_LOGE("%s,error data type\n",__func__);
            BK_ASSERT(0);
        }
    }
    return ret;
}

void wdrv_txdata_pre_process(uint8_t channel, void* head,uint8_t need_retry)
{
    bk_err_t ret = BK_OK;
    uint32_t int_level;
    
    
    void* first = head;
    void* last = head;
    uint8_t num = 1;
    
    uint8_t ipc_chnl = wdrv_map_to_ipc_chnl(channel);

    if(!need_retry)
    {
        //BK_LOGD(NULL, "%s,%d,p:0x%x\n",__func__,__LINE__,(struct pbuf*)head-1);
        //add to tx pending list tail

        if(channel == TX_MSDU_DATA)
        {
            WDRV_STATS_INC(wdrv_tx_process_cnt,1);

            WDRV_STATS_INC(tx_list_num,1);
        }
        //bk_mem_dump("process",PTR_TO_U32(head),50);

        WDRV_IPC_LOCK(&wdrv_ipc_env[ipc_chnl], int_level);
        co_list_push_back((struct co_list *)&wdrv_ipc_env[ipc_chnl].tx_list,(struct co_list_hdr *)head);
        WDRV_IPC_UNLOCK(&wdrv_ipc_env[ipc_chnl], int_level);
    }

    WDRV_IPC_LOCK(&wdrv_ipc_env[ipc_chnl], int_level);
    if(wdrv_ipc_env[ipc_chnl].sending_flag) 
    {
        WDRV_IPC_UNLOCK(&wdrv_ipc_env[ipc_chnl], int_level);
        return;
    }
    else
    {
        wdrv_ipc_env[ipc_chnl].sending_flag =1;
    }

    first = (void*)wdrv_ipc_env[ipc_chnl].tx_list.first;
    last = (void*)wdrv_ipc_env[ipc_chnl].tx_list.last;
    num = co_list_cnt((void*)&wdrv_ipc_env[ipc_chnl].tx_list);
    WDRV_IPC_UNLOCK(&wdrv_ipc_env[ipc_chnl], int_level);

    if(((first != NULL)&&(last!= NULL))&&(num == 0)) BK_ASSERT(0);

    if(first == NULL) goto ERR_EXIT;

    WDRV_LOGV("%s,%d,p:0x%x,p:0x%x,num:%d\n",__func__,__LINE__,(struct pbuf*)first-1,(struct pbuf*)last-1,num);

#if CONFIG_CONTROLLER_AP_BUFFER_COPY
    /* List links and headers are final here; flush once before notifying CP. */
    wdrv_flush_tx_buffer_for_cp(channel, first, num);
#endif

    ret = wdrv_txbuf_push(channel,first,last,num);
    
    if(ret != BK_OK){
        WDRV_LOGE("%s,ipc send fail,0x%x\n",__func__,ret);
    }
    else{
        WDRV_IPC_LOCK(&wdrv_ipc_env[ipc_chnl], int_level);
        co_list_init((void*)&wdrv_ipc_env[ipc_chnl].tx_list);
        WDRV_IPC_UNLOCK(&wdrv_ipc_env[ipc_chnl], int_level);
        if(channel == TX_MSDU_DATA)
            WDRV_STATS_RESET(tx_list_num,0);
        return;
    }
ERR_EXIT:
    WDRV_IPC_LOCK(&wdrv_ipc_env[ipc_chnl], int_level);
    wdrv_ipc_env[ipc_chnl].sending_flag = 0;
    WDRV_IPC_UNLOCK(&wdrv_ipc_env[ipc_chnl], int_level);
    if(ret != BK_OK) {
        BK_LOGD(NULL, "%s,%d,set_sema fail\n",__func__,__LINE__);
    }
}

void wdrv_tx_complete(void *param, mb_chnl_ack_t *ack_buf)
{
    WDRV_IPC_ISR_LOCK(&wdrv_ipc_env[IPC_DATA]);
    wdrv_ipc_env[IPC_DATA].sending_flag = 0;
    wdrv_stats_ptr->ipc_txc_cnt++;
    if(wdrv_ipc_env[IPC_DATA].tx_list.first != NULL)
    {
        WDRV_IPC_ISR_UNLOCK(&wdrv_ipc_env[IPC_DATA]);
        wdrv_msg_sender(0,WDRV_TASK_MSG_TXDATA,1);
    }
    else
    {
        WDRV_IPC_ISR_UNLOCK(&wdrv_ipc_env[IPC_DATA]);
    }
}
void wdrv_tx_msg_complete(void *param, mb_chnl_ack_t *ack_buf)
{
    WDRV_IPC_ISR_LOCK(&wdrv_ipc_env[IPC_CMD]);
    wdrv_ipc_env[IPC_CMD].sending_flag = 0;
    if(wdrv_ipc_env[IPC_CMD].tx_list.first != NULL)
    {
        WDRV_IPC_ISR_UNLOCK(&wdrv_ipc_env[IPC_CMD]);
        wdrv_msg_sender(0,WDRV_TASK_MSG_CMD,1);
    }
    else
    {
        WDRV_IPC_ISR_UNLOCK(&wdrv_ipc_env[IPC_CMD]);
    }
}

int wdrv_tx_msg_send(uint8_t *msg, uint16_t msg_len,uint8_t waitcfm)
{
    bk_err_t ret = BK_OK;
    struct cpdu_t * cpdu = (struct cpdu_t *)wdrv_get_cmd_buffer(CMD_BUF);

    if(cpdu == NULL) return BK_FAIL;

    WDRV_LOGV("%s msg:%x len:%d cfm:%d\r\n",__func__, msg, msg_len, waitcfm);
    memcpy(cpdu+1, msg,msg_len);
    
    cpdu->co_hdr.type = TX_BK_CMD_DATA;
    cpdu->co_hdr.length = msg_len + sizeof(struct cpdu_t);
    cpdu->co_hdr.is_buf_bank = 0;
    
    //bk_mem_dump("msg",PTR_TO_U32(msg),50);
    //bk_mem_dump("process",PTR_TO_U32(cpdu),50);

    //wdrv_txbuf_push(TX_BK_CMD_DATA,(void*)cpdu,(void*)cpdu,1);
    ret = wdrv_msg_sender((uint32_t)cpdu,WDRV_TASK_MSG_CMD, 0);

    if(ret != BK_OK)
    {
        WDRV_LOGE("%s,%d,push msg fail type:%d\n",__func__,__LINE__,ret);
        
        wdrv_free_cmd_buffer((uint8_t*)cpdu);
    }

    return BK_OK;
}

int wdrv_tx_msg(uint8_t *msg, uint16_t msg_len, wdrv_cmd_cfm *cfm, uint8_t *result)
{
    int ret = 0;
    uint32_t int_level = 0;
    wdrv_cmd_hdr *hdr = NULL;
    static uint16_t s_cmd_sn = 0;
    WDRV_LOGV("%s msg:%x len:%d\r\n",__func__, msg, msg_len);
    BK_ASSERT(msg_len < MAX_CMD_BUF_PAYLOAD);
    if (!msg) {
        WDRV_LOGE("%s: warning msg is null.\n", __func__);
        ret = -2;
        return ret;
    }
    hdr = (wdrv_cmd_hdr *)msg;
    WDRV_ENTER_TXMSG_CRITICAL(int_level);
    s_cmd_sn++;
    hdr->cmd_sn = s_cmd_sn;
    WDRV_EXIT_TXMSG_CRITICAL(int_level);
    WDRV_LOGD("%s: msg_id:0x%x len:%d sn:%d waitcfm:%d cfm_id:%x cfm_sn:%d \n", __func__, 
            hdr->cmd_id, msg_len, hdr->cmd_sn, cfm->waitcfm, cfm->cfm_id, cfm->cfm_sn);

    if (cfm->waitcfm == WDRV_CMD_WAITCFM) {

        ret = rtos_init_semaphore(&cfm->sema, 1);
        if(ret == BK_OK) 
        {
            cfm->cfm_buf = (uint8_t *)result;
            cfm->cfm_id  = hdr->cmd_id + WDRV_CMD_CFM_OFFSET;
            cfm->waitcfm = WDRV_CMD_WAITCFM;
            cfm->cfm_sn  = hdr->cmd_sn;

            WDRV_ENTER_TXMSG_CRITICAL(int_level);
            co_list_push_back((struct co_list *)&wdrv_host_env.cfm_pending_list,(struct co_list_hdr *)&cfm->list);
            WDRV_EXIT_TXMSG_CRITICAL(int_level);

            wdrv_tx_msg_send(msg, msg_len, WDRV_CMD_WAITCFM);

            // The len of result-buff is PRIVATE_COMMAND_DEF_LEN.
            if ((rtos_get_semaphore(&cfm->sema, WDRV_CMDCFM_TIMEOUT)) != 0) {

                rtos_lock_mutex(&wdrv_host_env.cfm_lock);
                WDRV_ENTER_TXMSG_CRITICAL(int_level);
                co_list_extract((struct co_list *)&wdrv_host_env.cfm_pending_list,(struct co_list_hdr *)&cfm->list);
                WDRV_EXIT_TXMSG_CRITICAL(int_level);
                rtos_unlock_mutex(&wdrv_host_env.cfm_lock);

                //Print AP/CP debug statistics
                wdrv_print_debug_info();
                wdrv_cntrl_get_cif_stats();

                WDRV_LOGW("%s: cmd confirm timeout.\n", __func__);
                ret = -3;
            } else {
                // receive cmd-cfm result
                ret = cfm->cfm_len;
            }

            rtos_deinit_semaphore(&cfm->sema);
        }
        else
        {
            BK_LOGD(NULL, "%s,%d,sema_init fail,send msg fail\n",__func__,__LINE__);
        }
    } else if (cfm->waitcfm == WDRV_CMD_NOWAITCFM) {
        // cmd send direct.
        wdrv_tx_msg_send(msg, msg_len, WDRV_CMD_NOWAITCFM);
    } else {
        WDRV_LOGE("waitcfm param err\n");
        ret = -1;
    }

    return ret;
}

