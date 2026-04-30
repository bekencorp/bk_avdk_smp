#pragma once


#include <stdlib.h>
#include <string.h>
#include <common/bk_include.h>
#include "bk_wifi_types.h"
#include "bk_wifi.h"
#include <os/str.h>
#include <os/mem.h>
#include <os/os.h>



#ifdef __cplusplus
extern "C" {
#endif

#define LP_IPC_MAX_MSG_SIZE    256

typedef enum{
    LP_IPC_CMD_KEEPALIVESTART             = 0x0001,
    LP_IPC_CMD_KEEPALIVESTOP              = 0x0002,
    LP_IPC_CMD_CONTROL                    = 0x0003,
    LP_IPC_CMD_GET_WAKEUP_ENV_ADDR        = 0x0004,
}lp_ipc_cmd_event;

typedef struct
{
    uint8_t      infotype;
    char         server[32];
    uint16_t     port;
    uint8_t      devId[32];
    uint8_t      idLen;
    uint8_t      key[64];
    uint8_t      keyLen;
}lp_ipc_keepalive_cfg_t;

typedef struct
{
    uint32_t wakeup_reason;
    beken2_timer_t timer;
    uint8_t delay_action;
    uint32_t delay_arg1;
    uint32_t delay_arg2;
}pl_wakeup_t;

typedef struct
{
    uint16_t cmd_id;
    uint16_t len;
}lp_ipc_msg_hdr_t;

extern pl_wakeup_t *pl_wakeup_env;

int lp_ipc_cli_init(void);
int lp_ipc_wakeup_env_init(void);
void lp_ipc_wakeup_env_response(uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif