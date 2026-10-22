/*
 * Console entry for the intf3 AT channel: one command to send anything, and a
 * fixed read-only sequence for first bring-up. Test scaffolding only - nothing
 * in the data path calls into here.
 */
#include <string.h>
#include <stdio.h>
#include <components/log.h>
#include <bk_private/bk_cli.h>
#include "modem_at.h"
#include "modem_at_cli.h"

#define TAG "modem_at"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)

#define AT_CLI_TIMEOUT_MS 3000U

/* Read-only, side-effect free: safe to run against a module in any state. */
static const char *const s_probe_cmds[] = {
    "AT",
    "ATI",
    "AT+CPIN?",
    "AT+CSQ",
    "AT+CESQ",
    "AT+CEREG?",
    "AT+CGATT?",
    "AT+CGCONTRDP=1",
    "AT+CGPADDR=1",
    "AT+ECSTATUS",
};

void modem_at_log_exchange(char *tag, const char *cmd, int ret, const char *resp)
{
    const char *tail = NULL;

    if (resp == NULL) {
        resp = "";
    }
    if (ret == 0) {
        tail = "OK";
    } else if (ret == -1) {
        tail = resp[0] ? NULL : "ERROR";
    } else if (ret == -2) {
        tail = "timeout";
    } else if (ret == -3) {
        tail = "offline";
    } else if (ret == -4) {
        tail = "write failed";
    } else if (ret == -5) {
        tail = "busy";
    } else {
        tail = "failed";
    }

    if (resp[0] && tail) {
        BK_LOGI(tag, "%s\r\n%s\r\n%s\r\n", cmd, resp, tail);
    } else if (resp[0]) {
        BK_LOGI(tag, "%s\r\n%s\r\n", cmd, resp);
    } else {
        BK_LOGI(tag, "%s\r\n%s\r\n", cmd, tail);
    }
}

static void at_cli_run(const char *cmd)
{
    static char resp[MODEM_AT_LINE_MAX + 1];
    int ret = modem_at_send(cmd, resp, sizeof(resp), AT_CLI_TIMEOUT_MS);

    /* resp carries the informational lines on success and the error line on
     * -1; on the other codes it holds whatever arrived before the command was
     * abandoned, which is usually nothing. */
    modem_at_log_exchange(TAG, cmd, ret, resp);
}

static void at_cli_cmd(char *buf, int len, int argc, char **argv)
{
    if (argc < 2) {
        LOGI("usage: modem_at probe | modem_at <command>\r\n");
        LOGI("link: %s\r\n", modem_at_is_online() ? "online" : "offline");
        return;
    }

    if (argc == 2 && !strcmp(argv[1], "probe")) {
        for (uint32_t i = 0; i < sizeof(s_probe_cmds) / sizeof(s_probe_cmds[0]); i++) {
            at_cli_run(s_probe_cmds[i]);
        }
        return;
    }

    /* The console splits on spaces; rejoin so commands with arguments survive. */
    static char cmd[MODEM_AT_LINE_MAX + 1];
    uint32_t pos = 0;

    for (int i = 1; i < argc; i++) {
        uint32_t n = strlen(argv[i]);

        if (pos + n + (i > 1 ? 1 : 0) >= sizeof(cmd)) {
            LOGI("command too long\r\n");
            return;
        }
        if (i > 1) {
            cmd[pos++] = ' ';
        }
        memcpy(&cmd[pos], argv[i], n);
        pos += n;
    }
    cmd[pos] = '\0';
    at_cli_run(cmd);
}

static const struct cli_command s_at_commands[] = {
    { "modem_at", "modem_at probe | modem_at <command>", at_cli_cmd },
};

int modem_at_cli_init(void)
{
    return cli_register_commands(s_at_commands,
        sizeof(s_at_commands) / sizeof(struct cli_command));
}
