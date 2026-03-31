#include "include/isp_cli.h"


void cli_isp_api_func_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    avdk_err_t ret = AVDK_ERR_UNSUPPORTED;
    char *msg = NULL;

    if (argc < 2)
    {
        LOGE("Usage: isp <command>\n");
        goto exit;
    }

    if (os_strcmp(argv[1], "open") == 0)
    {
        ret = AVDK_ERR_UNSUPPORTED;
    }
    else if (os_strcmp(argv[1], "close") == 0)
    {
        ret = AVDK_ERR_UNSUPPORTED;
    }
    else
    {
        LOGE("Usage: isp <command>\n");
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
