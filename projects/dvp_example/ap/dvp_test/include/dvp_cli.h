#ifndef __DVP_CLI_H_
#define __DVP_CLI_H_

#ifdef __cplusplus
extern "C" {
#endif

#define CLI_CMD_RSP_SUCCEED               "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR                 "CMDRSP:ERROR\r\n"
#define CMD_CONTAIN(value) cmd_contain(argc, argv, value)

#define GPIO_INVALID_ID           (0xFF)
#ifdef CONFIG_DVP_CTRL_POWER_GPIO_ID
#define DVP_POWER_GPIO_ID CONFIG_DVP_CTRL_POWER_GPIO_ID
#else
#define DVP_POWER_GPIO_ID GPIO_INVALID_ID
#endif

bool cmd_contain(int argc, char **argv, char *string);
int cli_dvp_test_init(void);
void cli_dvp_api_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_dvp_func_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif
