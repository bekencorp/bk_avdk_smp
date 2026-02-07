#ifndef __MODULE_TEST_CLI_H_
#define __MODULE_TEST_CLI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <components/bk_voice_service.h>

// Forward declarations for external handles (defined in module_test_cli.c)
// These are used by video_record_cli.c to check for conflicts
extern voice_handle_t voice_service_handle;
extern voice_handle_t voice_loopback_handle;
extern voice_handle_t dvp_display_voice_handle;

// Module test command handler functions
void cli_lcd_display_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_dvp_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_voice_service_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_voice_loopback_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_dvp_display_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_sd_card_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_video_file_info_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif /* __MODULE_TEST_CLI_H_ */
