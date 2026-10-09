#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int unicast_server_demo_init(void);
int unicast_server_cli_init(void);
int unicast_server_demo_adv(uint8_t on);
int unicast_server_demo_rx_ready(uint8_t ase_id);
int unicast_server_demo_release(uint8_t ase_id);
#if CONFIG_LE_AUDIO_VCP
int unicast_server_demo_set_volume(uint8_t volume);
int unicast_server_demo_set_mute(uint8_t mute);
#endif
#if CONFIG_LE_AUDIO_MCP
/* opcode is one of BK_MCP_OPC_* from bk_dm_mcp_types.h */
int unicast_server_demo_media_control(uint8_t opcode);
int unicast_server_demo_media_notify(uint8_t enable);
int unicast_server_demo_media_read(void);
#endif

#ifdef __cplusplus
}
#endif
