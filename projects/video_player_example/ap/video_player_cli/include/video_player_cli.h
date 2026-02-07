#ifndef __VIDEO_PLAYER_CLI_H_
#define __VIDEO_PLAYER_CLI_H_

#ifdef __cplusplus
extern "C" {
#endif

#define CLI_CMD_RSP_SUCCEED               "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR                 "CMDRSP:ERROR\r\n"

// CLI command handler functions
void cli_lcd_display_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_dvp_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_voice_service_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_voice_loopback_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_dvp_display_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_sd_card_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_video_record_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

// CLI initialization function
int cli_video_player_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __VIDEO_PLAYER_CLI_H_ */
