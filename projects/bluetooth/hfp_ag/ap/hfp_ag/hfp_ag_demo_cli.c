/**
 * @file hfp_ag_demo_cli.c
 *
 * CLI commands for the HFP AG (Audio Gateway) demo. The actual demo logic
 * lives in hfp_ag_demo.c; this file only parses the `hfp_ag` console command
 * and dispatches to the demo helpers.
 */

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <stdint.h>
#include <stdio.h>

#include "cli.h"
#include "hfp_ag_demo.h"

static int parse_mac(const char *str, uint8_t *mac)
{
    unsigned int b[6];

    if (!str || !mac)
    {
        return -1;
    }

    if (sscanf(str, "%2x:%2x:%2x:%2x:%2x:%2x", &b[5], &b[4], &b[3], &b[2], &b[1], &b[0]) != 6)
    {
        return -1;
    }

    for (int i = 0; i < 6; i++)
    {
        mac[i] = (uint8_t)b[i];
    }

    return 0;
}

static void hfp_ag_usage(void)
{
    CLI_LOGI("HFP AG demo:\n"
             "  hfp_ag init [msbc]\n"
             "  hfp_ag connect xx:xx:xx:xx:xx:xx\n"
             "  hfp_ag disconnect\n"
             "  hfp_ag incoming [number]\n"
             "  hfp_ag answer\n"
             "  hfp_ag hangup\n"
             "  hfp_ag dial <number>\n"
             "  hfp_ag audio on|off   (two-way intercom: AG mic <-> HF over SCO)\n"
             "  hfp_ag codec cvsd|msbc   (choose SCO codec)\n"
             "  hfp_ag battery <0-5>   (report AG battery level to HF)\n"
             "  hfp_ag cmd <at-result-code>\n"
             "  hfp_ag test [at-string]   (self-test Apple parsers; no arg = built-in cases,\n"
             "                             e.g. hfp_ag test AT+IPHONEACCEV=2,1,9,2,1)\n");
}

static void cmd_hfp_ag(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *msg = NULL;
    uint8_t mac[6] = {0};

    if (argc == 1 || (argc >= 2 && os_strcmp(argv[1], "-h") == 0))
    {
        hfp_ag_usage();
        goto __ok;
    }

    if (os_strcmp(argv[1], "init") == 0)
    {
        if (hfp_ag_demo_init() != 0)
        {
            goto __err;
        }
    }
    else if (os_strcmp(argv[1], "connect") == 0)
    {
        if (argc < 3 || parse_mac(argv[2], mac) != 0)
        {
            goto __usage;
        }

        hfp_ag_demo_connect(mac);
    }
    else if (os_strcmp(argv[1], "disconnect") == 0)
    {
        hfp_ag_demo_disconnect();
    }
    else if (os_strcmp(argv[1], "incoming") == 0)
    {
        hfp_ag_demo_incoming_call((argc >= 3) ? argv[2] : "10010");
    }
    else if (os_strcmp(argv[1], "answer") == 0)
    {
        hfp_ag_demo_answer();
    }
    else if (os_strcmp(argv[1], "hangup") == 0)
    {
        hfp_ag_demo_hangup();
    }
    else if (os_strcmp(argv[1], "dial") == 0)
    {
        hfp_ag_demo_dial_out((argc >= 3) ? argv[2] : NULL);
    }
    else if (os_strcmp(argv[1], "audio") == 0)
    {
        if (argc < 3)
        {
            goto __usage;
        }

        hfp_ag_demo_audio(os_strcmp(argv[2], "on") == 0 ? 1 : 0);
    }
    else if (os_strcmp(argv[1], "codec") == 0)
    {
        if (argc < 3)
        {
            goto __usage;
        }

        hfp_ag_demo_set_codec(os_strcmp(argv[2], "msbc") == 0 ? 1 : 0);
    }
    else if (os_strcmp(argv[1], "battery") == 0)
    {
        if (argc < 3)
        {
            goto __usage;
        }

        hfp_ag_demo_set_battery((uint8_t)os_strtoul(argv[2], NULL, 10));
    }
    else if (os_strcmp(argv[1], "vgs") == 0)
    {
        if (argc < 3)
        {
            goto __usage;
        }

        hfp_ag_demo_send_vgs(os_strtoul(argv[2], NULL, 10));
    }
    else if (os_strcmp(argv[1], "vgm") == 0)
    {
        if (argc < 3)
        {
            goto __usage;
        }

        hfp_ag_demo_send_vgm(os_strtoul(argv[2], NULL, 10));
    }
    else if (os_strcmp(argv[1], "cmd") == 0)
    {
        if (argc < 3)
        {
            goto __usage;
        }

        hfp_ag_demo_custom_cmd(argv[2]);
    }
    else if (os_strcmp(argv[1], "gpio") == 0)
    {
        if (argc < 4)
        {
            goto __usage;
        }

        hfp_ag_demo_gpio(os_strtoul(argv[2], NULL, 10), os_strtoul(argv[3], NULL, 10));
    }
    else
    {
        goto __usage;
    }

__ok:
    msg = CLI_CMD_RSP_SUCCEED;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    return;

__usage:
    hfp_ag_usage();
__err:
    msg = CLI_CMD_RSP_ERROR;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

static struct cli_command s_hfp_ag_commands[] =
{
    { "hfp_ag", "HFP AG demo, see hfp_ag -h", cmd_hfp_ag },
};

int cli_hfp_ag_demo_init(void)
{
    return cli_register_commands(s_hfp_ag_commands, sizeof(s_hfp_ag_commands) / sizeof(s_hfp_ag_commands[0]));
}
