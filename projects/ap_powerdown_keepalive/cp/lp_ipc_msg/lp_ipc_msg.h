#pragma once

#include <stdlib.h>
#include <string.h>
#include <common/bk_include.h>

#ifdef __cplusplus
extern "C" {
#endif


#define LP_IPC_PATTERN  (0xa5a6)
#define LP_IPC_MAX_MSG_SIZE   256


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
    uint16_t magic;
    uint16_t cid;
    uint16_t ctl;
    uint16_t seq;
    uint16_t checksum;
    uint16_t len;
}lp_ipc_prote_hdr_t;

typedef struct
{
    lp_ipc_prote_hdr_t header;
    uint8_t data[LP_IPC_MAX_MSG_SIZE];
}lp_ipc_evt_t;

typedef struct
{
    uint16_t cmd_id;
    uint16_t len;
    uint8_t payload[0];
}lp_ipc_msg_hdr_t;


extern int lp_ipc_send_event(uint16_t event_id, uint8_t *data, uint16_t len);
extern void lp_ipc_msg_init(void);

#ifdef __cplusplus
}
#endif
