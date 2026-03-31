#pragma once

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <common/avdk_pixel_types.h>
#include <components/log.h>
#include <avdk_error.h>

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define TAG "isp-cli"

#define CLI_CMD_RSP_SUCCEED               "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR                 "CMDRSP:ERROR\r\n"

int cli_isp_test_init(void);
void cli_isp_func_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_isp_api_func_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_isp_tuning_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_isp_dump_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

void isp_ini(uint8_t* Param);
void mipi_controller_init(uint32_t width, uint32_t height, uint8_t date_type);