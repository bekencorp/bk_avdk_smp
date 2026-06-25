#ifndef __VIDEO_RECORDER_CLI_H_
#define __VIDEO_RECORDER_CLI_H_

#ifdef __cplusplus
extern "C" {
#endif

// Video record command handler function
void cli_video_record_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

int cli_video_recorder_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __VIDEO_RECORD_CLI_H_ */
