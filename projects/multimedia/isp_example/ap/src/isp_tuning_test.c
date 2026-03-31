#include "include/isp_cli.h"

// External function declarations for ISP tuning server
extern void isp_tuning_server_init(void);
extern void isp_tuning_server_deinit(void);

/**
 * @brief CLI command handler for ISP tuning server
 * @param pcWriteBuffer Buffer to write command response
 * @param xWriteBufferLen Length of the write buffer
 * @param argc Number of command arguments
 * @param argv Command arguments array
 */
void cli_isp_tuning_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    avdk_err_t ret = AVDK_ERR_UNSUPPORTED;
    char *msg = NULL;

    if (argc < 2)
    {
        LOGE("Usage: isp_tuning <start|stop>\n");
        ret = AVDK_ERR_UNSUPPORTED;
        goto exit;
    }

    if (os_strcmp(argv[1], "start") == 0)
    {
        // Start ISP tuning server
        LOGI("Starting ISP tuning server...\n");
        isp_tuning_server_init();
        LOGI("ISP tuning server started successfully\n");
        ret = AVDK_ERR_OK;
    }
    else if (os_strcmp(argv[1], "stop") == 0)
    {
        // Stop ISP tuning server
        LOGI("Stopping ISP tuning server...\n");
        isp_tuning_server_deinit();
        LOGI("ISP tuning server stopped successfully\n");
        ret = AVDK_ERR_OK;
    }
    else
    {
        LOGE("Invalid command: %s (expected start or stop)\n", argv[1]);
        LOGE("Usage: isp_tuning <start|stop>\n");
        ret = AVDK_ERR_UNSUPPORTED;
        goto exit;
    }

exit:

    if (ret != AVDK_ERR_OK)
    {
        msg = CLI_CMD_RSP_ERROR;
    }
    else
    {
        msg = CLI_CMD_RSP_SUCCEED;
    }

    LOGI("%s ---complete\n", __func__);

    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}
