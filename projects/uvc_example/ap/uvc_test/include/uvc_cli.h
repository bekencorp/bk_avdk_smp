#ifndef __UVC_CLI_H__
#define __UVC_CLI_H__


#ifdef __cplusplus
extern "C" {
#endif

#define CLI_CMD_RSP_SUCCEED               "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR                 "CMDRSP:ERROR\r\n"

#define GET_PPI(value)     get_ppi_from_cmd(argc, argv, value)
#define CMD_CONTAIN(value) cmd_contain(argc, argv, value)

/**
 * @brief Initialize the UVC test CLI commands.
 * 
 * This function initializes the UVC test CLI commands by registering them with the CLI system.
 * 
 * @return int Returns 0 on success, or a negative error code on failure.
 */
int cli_uvc_test_init(void);


#ifdef __cplusplus
}
#endif

#endif /* __UVC_CLI_H__ */
