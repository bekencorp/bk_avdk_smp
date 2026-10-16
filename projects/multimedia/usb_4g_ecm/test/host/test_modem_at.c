/*
 * Host-side check of modem_at.c's byte-stream handling: line assembly across
 * packet boundaries, echo suppression, multi-line responses, URC vs response
 * attribution, error codes, overflow, timeout/late-answer and cancellation.
 *
 * Single-threaded by construction: the stubbed semaphore take is where the
 * simulated module speaks, which is exactly the window in which the real
 * module answers while the caller is blocked.
 */
#include <stdio.h>
#include <string.h>
#include <os/os.h>
#include "usb_modem_at.h"
#include "modem_at.h"

int stub_mutex_held;
int stub_sem_count;

static bool g_online = true;
static usb_modem_at_rx_cb_t g_rx_cb;
static usb_modem_at_link_cb_t g_link_cb;
static char g_last_tx[320];
static int g_tx_ret_override;

/* Scripted module output, fed on the Nth blocking semaphore take. */
#define SCRIPT_MAX 8
static const char *g_script[3][SCRIPT_MAX];
static int g_script_n[3];
static int g_take;

static char g_urc[MODEM_AT_LINE_MAX + 1];
static int g_urc_count;
static int g_reenter_ret;

void usb_modem_at_init(usb_modem_at_rx_cb_t rx_cb, usb_modem_at_link_cb_t link_cb)
{
    g_rx_cb = rx_cb;
    g_link_cb = link_cb;
}

bool usb_modem_at_is_online(void) { return g_online; }

int usb_modem_at_write(const uint8_t *data, uint32_t len, uint32_t timeout_ms)
{
    (void)timeout_ms;
    memcpy(g_last_tx, data, len);
    g_last_tx[len] = '\0';
    return g_tx_ret_override ? g_tx_ret_override : (int)len;
}

static void on_urc(const char *line)
{
    snprintf(g_urc, sizeof(g_urc), "%s", line);
    g_urc_count++;
}

/* The scripted chunks are fed here, one script slot per blocking take: slot 0
 * is the command's own wait, slot 1 the DRAIN wait. Two markers stand in for
 * events that are not module output. */
bk_err_t rtos_get_semaphore(beken_semaphore_t *s, uint32_t timeout_ms)
{
    int slot;

    (void)s;
    if (timeout_ms == BEKEN_NO_WAIT) {
        /* The pre-send flush. Drain whatever is counted, as the real one does. */
        if (stub_sem_count > 0) { stub_sem_count--; return BK_OK; }
        return BK_FAIL;
    }

    slot = g_take < 3 ? g_take : 2;
    g_take++;
    for (int i = 0; i < g_script_n[slot]; i++) {
        const char *chunk = g_script[slot][i];
        if (!strcmp(chunk, "<OFFLINE>")) {
            g_online = false;
            g_link_cb(false);
        } else if (!strcmp(chunk, "<REENTER>")) {
            g_reenter_ret = modem_at_send("AT", NULL, 0, 1000);
        } else {
            g_rx_cb((const uint8_t *)chunk, strlen(chunk));
        }
    }
    if (stub_sem_count > 0) { stub_sem_count--; return BK_OK; }
    return BK_FAIL; /* the wait expired */
}

static int g_fail;

static void expect(const char *what, int got, int want)
{
    printf("  %-28s got %d want %d  %s\n", what, got, want,
           got == want ? "ok" : "FAIL");
    if (got != want) g_fail++;
}

static void expect_str(const char *what, const char *got, const char *want)
{
    printf("  %-28s \"%s\" want \"%s\"  %s\n", what, got, want,
           !strcmp(got, want) ? "ok" : "FAIL");
    if (strcmp(got, want)) g_fail++;
}

/* Arms a fresh case: resets the scripts and the take counter. */
static void reset(void)
{
    memset(g_script_n, 0, sizeof(g_script_n));
    g_take = 0;
    g_tx_ret_override = 0;
    stub_sem_count = 0;
    g_urc_count = 0;
    g_urc[0] = '\0';
}

static void script(int slot, const char *chunk) { g_script[slot][g_script_n[slot]++] = chunk; }

static int test_tail(char *resp, char *longline)
{
    int ret;

    printf("6. stale OK is not counted as the next command's success\n");
    reset();
    g_rx_cb((const uint8_t *)"\r\nOK\r\n", 6); /* late answer, state is IDLE */
    expect("treated as urc", g_urc_count, 1);
    expect("no semaphore given", stub_sem_count, 0);
    ret = modem_at_send("AT+CSQ", resp, MODEM_AT_LINE_MAX + 1, 1000);
    expect("next command ret", ret, -2);
    expect_str("next command resp", resp, "");

    printf("7. over-long line is dropped whole, not truncated\n");
    reset();
    memset(longline, 'x', 302); /* 300 x's after the leading CRLF */
    memcpy(longline, "\r\n", 2);
    memcpy(longline + 302, "\r\n\r\nOK\r\n", 8);
    longline[310] = '\0';
    script(0, longline);
    ret = modem_at_send("AT+X", resp, MODEM_AT_LINE_MAX + 1, 1000);
    expect("ret", ret, 0);
    expect_str("resp", resp, "");

    printf("8. another command in flight is refused, not queued\n");
    reset();
    script(0, "<REENTER>");
    script(0, "\r\nOK\r\n");
    g_reenter_ret = 99;
    ret = modem_at_send("AT", resp, MODEM_AT_LINE_MAX + 1, 1000);
    expect("inner ret", g_reenter_ret, -5);
    expect("outer ret", ret, 0);

    printf("9. link loss during the wait cancels the command\n");
    reset();
    script(0, "<OFFLINE>");
    ret = modem_at_send("AT+CSQ", resp, MODEM_AT_LINE_MAX + 1, 1000);
    expect("ret", ret, -3);
    expect("online", modem_at_is_online(), 0);
    g_online = true;
    g_link_cb(true);

    printf("10. offline at entry fails before writing\n");
    reset();
    g_online = false;
    g_last_tx[0] = '\0';
    ret = modem_at_send("AT", resp, MODEM_AT_LINE_MAX + 1, 1000);
    expect("ret", ret, -3);
    expect_str("nothing written", g_last_tx, "");
    g_online = true;

    printf("\n%s (%d failure%s)\n", g_fail ? "FAILED" : "all checks passed",
           g_fail, g_fail == 1 ? "" : "s");
    return g_fail ? 1 : 0;
}

int main(void)
{
    static char resp[MODEM_AT_LINE_MAX + 1];
    static char longline[320];
    int ret;

    if (modem_at_init(on_urc) != BK_OK) { printf("init failed\n"); return 1; }
    printf("online=%d\n", modem_at_is_online());

    printf("1. response split across three packets, echo suppressed\n");
    reset();
    script(0, "AT+C");
    script(0, "SQ\r\n+CSQ: 20,9");
    script(0, "9\r\n\r\nOK\r\n");
    ret = modem_at_send("AT+CSQ", resp, sizeof(resp), 1000);
    expect("ret", ret, 0);
    expect_str("resp", resp, "+CSQ: 20,99");
    expect_str("tx", g_last_tx, "AT+CSQ\r");
    expect("urc count", g_urc_count, 0);

    printf("2. multi-line response joined with CRLF\n");
    reset();
    script(0, "\r\nATI\r\nManufacturer\r\nModel: X\r\n\r\nOK\r\n");
    ret = modem_at_send("ATI", resp, sizeof(resp), 1000);
    expect("ret", ret, 0);
    expect_str("resp", resp, "Manufacturer\r\nModel: X");

    printf("3. +CME ERROR ends the command and is kept\n");
    reset();
    script(0, "\r\n+CME ERROR: 30\r\n");
    ret = modem_at_send("AT+CPIN?", resp, sizeof(resp), 1000);
    expect("ret", ret, -1);
    expect_str("resp", resp, "+CME ERROR: 30");

    printf("4. line arriving with no command pending is a URC\n");
    reset();
    g_rx_cb((const uint8_t *)"\r\n+CGEV: ME DETACH\r\n", 20);
    expect("urc count", g_urc_count, 1);
    expect_str("urc", g_urc, "+CGEV: ME DETACH");
    expect("no semaphore given", stub_sem_count, 0);

    printf("5. OK arriving only in the drain window is still a timeout\n");
    reset();
    script(1, "\r\nOK\r\n"); /* nothing in slot 0: the command's wait expires */
    ret = modem_at_send("AT+COPS=?", resp, sizeof(resp), 1000);
    expect("ret", ret, -2);

    return test_tail(resp, longline);
}
