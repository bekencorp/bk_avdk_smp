#include <common/bk_include.h>
#include "cli.h"
// Ensure struct cli_command is visible for command table definition, even when include paths vary.
#include <bk_private/bk_cli.h>
#include "video_player_cli.h"

// This file only registers video playback CLI commands.
// Recording commands are registered by video_recorder_cli.c from the app entry.

// CLI commands registration
#define CMDS_COUNT  (sizeof(s_video_player_commands) / sizeof(struct cli_command))

// Forward declarations for CLI commands
void cli_video_play_engine_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_video_play_playlist_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

static const struct cli_command s_video_player_commands[] =
{
    {"video_play_engine", "video_play_engine (subcommands: see cli_video_play_engine_cmd)", cli_video_play_engine_cmd},
    {"video_play_playlist", "video_play_playlist (subcommands: see cli_video_play_playlist_cmd)", cli_video_play_playlist_cmd},
};

int cli_video_player_init(void)
{
    return cli_register_commands(s_video_player_commands, CMDS_COUNT);
}
