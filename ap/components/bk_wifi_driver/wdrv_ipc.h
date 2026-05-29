#pragma once

#ifdef __cplusplus
extern "C" {
#endif


//#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <driver/pwr_clk.h>
#include <components/log.h>
#include <driver/mailbox_channel.h>
#if CONFIG_SOC_SMP
#include "spinlock.h"
#endif
#include "wdrv_main.h"

#define WIFI_IPC_CMD_CHNL   MB_CHNL_WIFI_CMD
#define WIFI_IPC_DATA_CHNL  MB_CHNL_WIFI_DATA

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
    beken_semaphore_t sema;
    uint8_t sending_flag;
    struct co_list tx_list;
#if CONFIG_SOC_SMP
    volatile spinlock_t *tx_lock;
#endif
}wdrv_ipc_t;

typedef struct
{
    uint32_t         ipc_hdr;//for IPC use

    uint32_t         head;
    uint32_t         tail;
    uint8_t          channel;
    uint8_t          num;
    uint16_t         rsve;
} ipc_chnl_node_t;


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

static inline uint32_t wdrv_ipc_lock(wdrv_ipc_t *ipc)
{
	uint32_t flags = rtos_disable_int();
#if CONFIG_SOC_SMP
	spin_lock(ipc->tx_lock);
#endif
	return flags;
}

static inline void wdrv_ipc_unlock(wdrv_ipc_t *ipc, uint32_t flags)
{
#if CONFIG_SOC_SMP
	spin_unlock(ipc->tx_lock);
#endif
	rtos_enable_int(flags);
}

static inline void wdrv_ipc_isr_lock(wdrv_ipc_t *ipc)
{
#if CONFIG_SOC_SMP
	spin_lock(ipc->tx_lock);
#endif
}

static inline void wdrv_ipc_isr_unlock(wdrv_ipc_t *ipc)
{
#if CONFIG_SOC_SMP
	spin_unlock(ipc->tx_lock);
#endif
}

#define WDRV_IPC_LOCK(ipc, int_level)    do { int_level = wdrv_ipc_lock(ipc); } while(0)
#define WDRV_IPC_UNLOCK(ipc, int_level)  do { wdrv_ipc_unlock(ipc, int_level); } while(0)
#define WDRV_IPC_ISR_LOCK(ipc)           do { wdrv_ipc_isr_lock(ipc); } while(0)
#define WDRV_IPC_ISR_UNLOCK(ipc)         do { wdrv_ipc_isr_unlock(ipc); } while(0)

uint8_t wdrv_map_to_ipc_chnl(uint8_t channel);

extern bk_err_t wdrv_ipc_init();
extern wdrv_ipc_t wdrv_ipc_env[IPC_MAX];
#ifdef __cplusplus
}
#endif
