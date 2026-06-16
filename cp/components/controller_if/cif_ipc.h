#pragma once

#ifdef __cplusplus
extern "C" {
#endif


#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include "cli.h"
#include <driver/pwr_clk.h>
#include <components/log.h>
#include <driver/mailbox_channel.h>
#if CONFIG_SOC_SMP
#include "spinlock.h"
#endif
#include "cif_main.h"
#define WIFI_IPC_CMD_CHNL      MB_CHNL_WIFI_CMD
#define WIFI_IPC_DATA_CHNL     MB_CHNL_WIFI_DATA

typedef bk_err_t (*mb_open_t)(uint8_t channel,void * param);
typedef bk_err_t (*mb_close_t)(uint8_t channel);
typedef bk_err_t (*mb_config_t)(uint8_t channel,uint8_t cmd, void * param);
typedef bk_err_t (*mb_send_t)(uint8_t channel,mb_chnl_cmd_t * param);
typedef bk_err_t (*mb_close_t)(uint8_t channel);

typedef struct 
{
    uint8_t     channel;
    mb_open_t   open;
    mb_close_t  close;
    mb_config_t cb_register;
    mb_send_t   send;
    uint8_t     sending_flag;
    
    //For rx pending list
    struct co_list rx_list;//For rx data and tx confirm data, both is pbuf data.
#if CONFIG_SOC_SMP
    volatile spinlock_t *tx_lock;
#endif
}cif_ipc_t;

typedef struct
{
    uint32_t         ipc_hdr;//for IPC use

    void*            head;
    void*            tail;
    uint8_t          channel;
    uint8_t          num;
    uint16_t         rsve;
} cif_chnl_node_t;


enum tx_data_type
{
    TX_BK_CMD_DATA =   0,
    TX_CEVA_CMD_DATA = 1,
    TX_MSDU_DATA =     2,
    TX_MPDU_DATA =     3,
};
enum rx_data_type
{
    RX_BK_CMD_DATA =   0,
    RX_CEVA_CMD_DATA = 1,
    RX_MSDU_DATA =     2,
    RX_MPDU_DATA =     3,
};
enum wifi_data_type
{
    IPC_CMD,
    IPC_DATA,
    IPC_MAX
};


static inline uint32_t cif_ipc_lock(cif_ipc_t *ipc)
{
	uint32_t flags = rtos_disable_int();
#if CONFIG_SOC_SMP
	spin_lock(ipc->tx_lock);
#endif
	return flags;
}

static inline void cif_ipc_unlock(cif_ipc_t *ipc, uint32_t flags)
{
#if CONFIG_SOC_SMP
	spin_unlock(ipc->tx_lock);
#endif
	rtos_enable_int(flags);
}

static inline void cif_ipc_isr_lock(cif_ipc_t *ipc)
{
#if CONFIG_SOC_SMP
	spin_lock(ipc->tx_lock);
#endif
}

static inline void cif_ipc_isr_unlock(cif_ipc_t *ipc)
{
#if CONFIG_SOC_SMP
	spin_unlock(ipc->tx_lock);
#endif
}

#define CIF_IPC_LOCK(ipc, int_level)    do { int_level = cif_ipc_lock(ipc); } while(0)
#define CIF_IPC_UNLOCK(ipc, int_level)  do { cif_ipc_unlock(ipc, int_level); } while(0)
#define CIF_IPC_ISR_LOCK(ipc)           do { cif_ipc_isr_lock(ipc); } while(0)
#define CIF_IPC_ISR_UNLOCK(ipc)         do { cif_ipc_isr_unlock(ipc); } while(0)

__IRAM3 uint8_t cif_map_to_rx_wifi_type(uint8_t channel);
__IRAM3 uint8_t cif_map_to_ipc_chnl(uint8_t channel);
extern bk_err_t cif_ipc_init();
extern cif_ipc_t cif_ipc_env[IPC_MAX];
#ifdef __cplusplus
}
#endif
