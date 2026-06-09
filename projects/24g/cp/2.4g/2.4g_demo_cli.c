#include "cli.h"
#include "bk_24g_api.h"

#include "2.4g_demo.h"
#include "system_hw.h"
// #include "gpio_hal.h"
#include "reg_base.h"

#include "os/os.h"
#include <os/mem.h>
#include <os/str.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include "components/bk24/bk_24g.h"

#define GPIO(id, high) do{if(high) (*(volatile uint32_t *)(SOC_AON_GPIO_REG_BASE + id * 4)) |= (1 << 1) | (2 << 24); else (*(volatile uint32_t *)(SOC_AON_GPIO_REG_BASE + id * 4)) &= ~(uint32_t)(1 << 1);} while(0)
#define STRCMP(x, y) ((strlen(x) == strlen(y) && !os_strcmp(x, y)) ? 0 : 1)

void cli_24g_test_data(uint8_t len)
{
    //static
    uint8_t data = 1;
    uint8_t tmp[len];

    memset(tmp, 0, len);
    tmp[0] = 0xff;

    for (size_t i = 1; i < 10 && i < len; i++)
    {
        tmp[i] = data;
    }

    data++;

    bk24_set_ack_payload(tmp, len, 0);
}

static void usage(void)
{
    CLI_LOGI("Usage:\n"
            );

    return;
}

static void cmd_parse(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *msg = NULL;
    int ret = 0;

    if (argc == 1)
    {
        goto __usage;
    }
    else if (STRCMP(argv[1], "-h") == 0)
    {
        goto __usage;
    }
    else if (STRCMP(argv[1], "gpio") == 0 && argc >= 3)
    {
        uint8_t id = 0;
        uint8_t high = 0;

        ret = sscanf(argv[2], "%hhu", &id);

        if (ret != 1)
        {
            CLI_LOGE("gpio id param err\n");
            goto __error;
        }

        if (argc >= 4)
        {
            ret = sscanf(argv[3], "%hhu", &high);

            if (ret != 1)
            {
                CLI_LOGE("gpio high param err\n");
                goto __error;
            }
        }

        GPIO(id, high);
    }
    else if (STRCMP(argv[1], "init") == 0)
    {
        uint8_t init = 1;

        if (argc >= 3)
        {
            ret = sscanf(argv[2], "%hhu", &init);

            if (ret != 1)
            {
                CLI_LOGE("init param err\n");
                goto __error;
            }
        }

        if (init)
        {
            bk_24g_os_adapter_init();
            bk24_init();
        }
        else
        {
            bk24_deinit();
        }
    }
    else if (STRCMP(argv[1], "send") == 0)
    {
        uint8_t len = 16;
        uint8_t *data = NULL;

        if (argc >= 3)
        {
            ret = sscanf(argv[2], "%hhu", &len);

            if (ret != 1)
            {
                CLI_LOGE("len param err\n");
                goto __error;
            }
        }

        data = os_zalloc(len);

        if (!data)
        {
            CLI_LOGE("alloc err %d\n", len);
            goto __error;
        }

        for (size_t i = 0; i < len; i++)
        {
            data[i] = i + 1;
        }

        bk24_switch_to_tx_rx(0);
        bk24_send_data(data, len);

        os_free(data);
    }
    else if (STRCMP(argv[1], "send_no_ack") == 0)
    {
        uint8_t len = 16;
        uint8_t *data = NULL;

        if (argc >= 3)
        {
            ret = sscanf(argv[2], "%hhu", &len);

            if (ret != 1)
            {
                CLI_LOGE("len param err\n");
                goto __error;
            }
        }

        data = os_zalloc(len);

        if (!data)
        {
            CLI_LOGE("alloc err %d\n", len);
            goto __error;
        }

        for (size_t i = 0; i < len; i++)
        {
            data[i] = i + 1;
        }

        bk24_switch_to_tx_rx(0);//need set to tx mode
        bk24_send_data_no_ack(data, len);

        os_free(data);
    }
    else if (STRCMP(argv[1], "set_ack") == 0)
    {
        uint8_t len = 16;
        uint8_t pipe = 0;

        if (argc >= 3)
        {
            ret = sscanf(argv[2], "%hhu", &len);

            if (ret != 1)
            {
                CLI_LOGE("len param err\n");
                goto __error;
            }
        }

        if (argc >= 4)
        {
            ret = sscanf(argv[2], "%hhu", &pipe);

            if (ret != 1)
            {
                CLI_LOGE("pipe param err\n");
                goto __error;
            }
        }

        uint8_t tmp[len];

        memset(tmp, 0, len);

        for (int32 i = len - 1; i >= 0; i--)
        {
            tmp[i] = i + 1;
        }

        bk24_set_ack_payload(tmp, len, 0);
    }
    else if (STRCMP(argv[1], "set_dr") == 0)
    {
        uint8_t dr = 0;

        if (argc >= 3)
        {
            ret = sscanf(argv[2], "%hhu", &dr);

            if (ret != 1)
            {
                CLI_LOGE("dr param err\n");

                goto __error;
            }
        }

        bk24_set_data_rate(dr);
    }
    else if (STRCMP(argv[1], "set_channel") == 0)
    {
        uint8_t channel = 0;

        if (argc >= 3)
        {
            ret = sscanf(argv[2], "%hhu", &channel);

            if (ret != 1)
            {
                CLI_LOGE("dr param err\n");

                goto __error;
            }
        }

        bk24_set_channel(channel);
    }
    else if (STRCMP(argv[1], "reset") == 0)
    {
        bk24_reset();
    }
    else if (STRCMP(argv[1], "txrx") == 0)
    {
        uint8_t txrx = 0; //0 tx 1 rx

        if (argc >= 3)
        {
            ret = sscanf(argv[2], "%hhu", &txrx);

            if (ret != 1)
            {
                CLI_LOGE("txrx param err\n");
                goto __error;
            }
        }

        bk24_switch_to_tx_rx(txrx);
        uint16_t len = 16;

        uint8_t tmp[len];

        memset(tmp, 0, len);

        for (int32 i = len - 1; i >= 0; i--)
        {
            tmp[i] = i + 1;
        }

        bk24_set_ack_payload(tmp, len, 0);
    }
    else
    {
        goto __usage;
    }

    msg = CLI_CMD_RSP_SUCCEED;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    return;

__usage:
    usage();

__error:
    msg = CLI_CMD_RSP_ERROR;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

static const struct cli_command s_commands[] =
{
    {"2.4g_demo", "see -h", cmd_parse},
};

int32_t cli_24g_demo_init(void)
{
    return cli_register_commands(s_commands, sizeof(s_commands) / sizeof(s_commands[0]));
}
