#include <common/bk_include.h>
#include "cli.h"
// Ensure struct cli_command is visible for command table definition, even when include paths vary.
#include <bk_private/bk_cli.h>
#include "video_player_cli.h"
#include "video_recorder_cli.h"
#include "module_test_cli.h"
#include "video_player_common.h"

// This file now only contains CLI command registration
// All command handlers have been moved to:
// - video_record_cli.c: video recording functionality
// - module_test_cli.c: module testing functionality (LCD, DVP, voice, SD card)
// - video_player_common.c: shared helper functions

// CLI commands registration
#define CMDS_COUNT  (sizeof(s_video_player_commands) / sizeof(struct cli_command))

// Forward declarations for CLI commands
void cli_video_play_engine_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_video_play_playlist_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

static const struct cli_command s_video_player_commands[] =
{
    {"lcd_display", "lcd_display open/close", cli_lcd_display_cmd},
    {"dvp", "dvp open width height format/close", cli_dvp_cmd},
    {"voice_service", "voice_service start/stop onboard 8000 1 g711a g711a onboard 8000 0", cli_voice_service_cmd},
    {"voice_loopback", "voice_loopback start/stop", cli_voice_loopback_cmd},
    {"dvp_display", "dvp_display start [width] [height]/stop", cli_dvp_display_cmd},
    {"sd_card_test", "sd_card_test mount/unmount/scan/load [path]", cli_sd_card_cmd},
    {"video_file_info", "video_file_info [file_path]", cli_video_file_info_cmd},
    {"video_record", "video_record start [file_path] [width] [height] [format] [type]/stop", cli_video_record_cmd},
    {"video_play_engine", "video_play_engine (subcommands: see cli_video_play_engine_cmd)", cli_video_play_engine_cmd},
    {"video_play_playlist", "video_play_playlist (subcommands: see cli_video_play_playlist_cmd)", cli_video_play_playlist_cmd},
};

int cli_video_player_init(void)
{
    return cli_register_commands(s_video_player_commands, CMDS_COUNT);
}
