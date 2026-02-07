#pragma once


#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    POWERUP_POWER_WAKEUP_FLAG = 1,                   //Normal power-on startup
    POWERUP_KEEPALIVE_DISCONNECTION = 2,             //Keepalive disconnection startup
    POWERUP_KEEPALIVE_WAKEUP_FLAG = 3,               //Keepalive wakeup
}POWERUP_FLAGS;


void keepalive_handle_wakeup_reason(void);
bk_err_t keepalive_stop_cp_keepalive(void);
bk_err_t keepalive_send_control_cmd(const char *control_param);
bk_err_t keepalive_send_keepalive_cmd(char *ip, char *port);

#ifdef __cplusplus
}
#endif


