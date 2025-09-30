#ifndef __DRAW_OSD_TEST_H__
#define __DRAW_OSD_TEST_H__


#ifdef __cplusplus
extern "C" {
#endif

#define CLI_CMD_RSP_SUCCEED               "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR                 "CMDRSP:ERROR\r\n"

/* CLI命令处理函数声明 */
void cli_draw_osd_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

/* CLI初始化函数 */
int cli_draw_osd_test_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __DRAW_OSD_TEST_H__ */