#ifndef __UVC_CLI_H__
#define __UVC_CLI_H__


#ifdef __cplusplus
extern "C" {
#endif

#define CLI_CMD_RSP_SUCCEED               "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR                 "CMDRSP:ERROR\r\n"

int cli_uvc_test_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __UVC_CLI_H__ */
