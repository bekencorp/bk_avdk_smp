/*
 * Minimal AT command manager. See modem_at.h for the contract.
 *
 * Attribution rule, and the only interesting thing in here: a response line
 * belongs to the pending command, and a line that arrives with no command
 * pending is a URC. The prefix is never used to decide that - "+CSQ: 20,99" is
 * a response when we asked and a URC when we did not, and keying on '+' would
 * get both cases wrong in one direction or the other.
 */
#include <string.h>
#include <os/os.h>
#include <components/log.h>
#include "usb_modem_at.h"
#include "modem_at.h"

#define TAG "modem_at"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

/* How long a timed-out command keeps swallowing output. Covers the common case
 * where the module answers just after we gave up; bounded so a module that
 * never answers cannot wedge the channel. */
#define MODEM_AT_DRAIN_MS 300U

enum {
    AT_STATE_IDLE = 0, /* no command in flight - any line is a URC */
    AT_STATE_WAIT,     /* command in flight - lines belong to it */
    AT_STATE_DRAIN,    /* command timed out - late lines are dropped */
};

enum {
    AT_RESULT_NONE = 0,
    AT_RESULT_OK,
    AT_RESULT_ERROR,
};

static volatile uint8_t g_state = AT_STATE_IDLE;
static volatile uint8_t g_result = AT_RESULT_NONE;

/* Serialises callers; g_state alone cannot, two callers could both see IDLE. */
static beken_mutex_t g_cmd_lock = NULL;
/* Given by the receive thread when a final result code lands. */
static beken_semaphore_t g_done_sem = NULL;

static modem_at_urc_cb_t g_urc_cb = NULL;

/* Line assembly across USB packets. Owned by the receive thread alone. */
static char g_line[MODEM_AT_LINE_MAX + 1];
static uint32_t g_line_len = 0;
static bool g_line_over = false; /* current line exceeded the buffer */

/* The command in flight. Valid only while the caller is blocked in
 * modem_at_send(), which is exactly the WAIT and DRAIN window. */
static const char *g_cmd = NULL;
static char *g_resp = NULL;
static uint32_t g_resp_size = 0;
static uint32_t g_resp_len = 0;

/* "+CME ERROR: 30" style failures carry a code, plain "ERROR" does not; both
 * end the command the same way. */
static int at_classify(const char *line)
{
    if (!strcmp(line, "OK")) {
        return AT_RESULT_OK;
    }
    if (!strcmp(line, "ERROR") ||
        !strncmp(line, "+CME ERROR", 10) ||
        !strncmp(line, "+CMS ERROR", 10)) {
        return AT_RESULT_ERROR;
    }
    return AT_RESULT_NONE;
}

/* Informational lines are concatenated CRLF-separated. A line that does not
 * fit is dropped whole rather than cut mid-way, so the caller never parses a
 * half line as a complete one. */
static void at_resp_append(const char *line)
{
    uint32_t len = strlen(line);
    uint32_t need = len + (g_resp_len ? 2 : 0);

    if (!g_resp || g_resp_len + need + 1 > g_resp_size) {
        LOGD("resp full, drop: %s\r\n", line);
        return;
    }
    if (g_resp_len) {
        g_resp[g_resp_len++] = '\r';
        g_resp[g_resp_len++] = '\n';
    }
    memcpy(&g_resp[g_resp_len], line, len);
    g_resp_len += len;
    g_resp[g_resp_len] = '\0';
}

/* One complete line, CRLF already stripped, never empty. Receive thread. */
static void at_on_line(const char *line)
{
    int result;

    if (g_state == AT_STATE_IDLE) {
        if (g_urc_cb) {
            g_urc_cb(line);
        } else {
            LOGD("urc: %s\r\n", line);
        }
        return;
    }

    result = at_classify(line);

    if (g_state == AT_STATE_DRAIN) {
        /* Belongs to a command whose caller already gave up. Dropping it here
         * is the whole point: left in place it would be read as the next
         * command's answer. */
        LOGD("late: %s\r\n", line);
        if (result != AT_RESULT_NONE) {
            rtos_set_semaphore(&g_done_sem);
        }
        return;
    }

    /* The module echoes the command back before answering. */
    if (g_cmd && !strcmp(line, g_cmd)) {
        return;
    }

    if (result != AT_RESULT_NONE) {
        if (result == AT_RESULT_ERROR) {
            at_resp_append(line); /* the caller wants to see which error */
        }
        g_result = (uint8_t)result;
        rtos_set_semaphore(&g_done_sem);
        return;
    }

    at_resp_append(line);
}

static void at_on_rx(const uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        char c = (char)data[i];

        if (c == '\r' || c == '\n') {
            if (g_line_over) {
                LOGE("line over %u bytes, dropped\r\n", MODEM_AT_LINE_MAX);
            } else if (g_line_len) {
                g_line[g_line_len] = '\0';
                at_on_line(g_line);
            }
            /* Empty lines are the CRLF padding around every response. */
            g_line_len = 0;
            g_line_over = false;
            continue;
        }
        if (g_line_len < MODEM_AT_LINE_MAX) {
            g_line[g_line_len++] = c;
        } else {
            g_line_over = true;
        }
    }
}

static void at_on_link(bool online)
{
    LOGI("AT channel %s\r\n", online ? "online" : "offline");

    if (online) {
        g_line_len = 0;
        g_line_over = false;
        return;
    }
    /* Release a waiting caller now; it rechecks the link and returns -3. The
     * partial line is dropped so a reconnect does not resume mid-sentence. */
    g_line_len = 0;
    g_line_over = false;
    if (g_state != AT_STATE_IDLE) {
        rtos_set_semaphore(&g_done_sem);
    }
}

int modem_at_init(modem_at_urc_cb_t urc_cb)
{
    bk_err_t err;

    if (g_cmd_lock) {
        return BK_OK; /* idempotent: callers may not know who runs first */
    }
    err = rtos_init_mutex(&g_cmd_lock);
    if (err != BK_OK) {
        LOGE("mutex init failed %d\r\n", err);
        return err;
    }
    err = rtos_init_semaphore(&g_done_sem, 1);
    if (err != BK_OK) {
        LOGE("sem init failed %d\r\n", err);
        rtos_deinit_mutex(&g_cmd_lock);
        g_cmd_lock = NULL;
        return err;
    }
    g_urc_cb = urc_cb;
    usb_modem_at_init(at_on_rx, at_on_link);
    return BK_OK;
}

bool modem_at_is_online(void)
{
    return usb_modem_at_is_online();
}

int modem_at_send(const char *cmd, char *resp, uint32_t resp_size, uint32_t timeout_ms)
{
    /* cmd plus the terminating CR. */
    static char tx[USB_MODEM_AT_TX_MAX];
    uint32_t cmd_len;
    int ret, written;

    if (!cmd || !timeout_ms || !g_cmd_lock) {
        return -3;
    }
    cmd_len = strlen(cmd);
    if (!cmd_len || cmd_len + 1 > sizeof(tx)) {
        return -3;
    }
    if (resp && resp_size) {
        resp[0] = '\0';
    }
    /* Fails instead of queueing: one command at a time is the contract, and a
     * caller that wanted to wait can wait on its own terms. */
    if (rtos_lock_mutex_timeout(&g_cmd_lock, BEKEN_NO_WAIT) != BK_OK) {
        return -5;
    }
    if (!usb_modem_at_is_online()) {
        ret = -3;
        goto unlock;
    }

    /* A result code that landed after the previous command gave up would
     * otherwise satisfy this command's wait immediately. */
    while (rtos_get_semaphore(&g_done_sem, BEKEN_NO_WAIT) == BK_OK) {
    }

    memcpy(tx, cmd, cmd_len);
    tx[cmd_len] = '\r';

    g_cmd = cmd;
    g_resp = (resp && resp_size) ? resp : NULL;
    g_resp_size = g_resp ? resp_size : 0;
    g_resp_len = 0;
    g_result = AT_RESULT_NONE;
    /* Published before the write: the answer can arrive while it is still in
     * the controller's FIFO. */
    g_state = AT_STATE_WAIT;

    written = usb_modem_at_write((const uint8_t *)tx, cmd_len + 1, timeout_ms);
    if (written != (int)(cmd_len + 1)) {
        LOGE("write %s failed %d\r\n", cmd, written);
        g_state = AT_STATE_IDLE;
        ret = usb_modem_at_is_online() ? -4 : -3;
        goto unlock;
    }

    if (rtos_get_semaphore(&g_done_sem, timeout_ms) != BK_OK) {
        /* Swallow a late answer so it is not read as the next command's. */
        g_state = AT_STATE_DRAIN;
        rtos_get_semaphore(&g_done_sem, MODEM_AT_DRAIN_MS);
        g_state = AT_STATE_IDLE;
        /* The result code can land in the gap between the wait expiring and
         * DRAIN being published, in which case the answer is complete and in
         * resp already - reporting a timeout would throw it away. */
        if (g_result == AT_RESULT_NONE) {
            LOGE("%s timeout\r\n", cmd);
            ret = -2;
            goto unlock;
        }
    }
    g_state = AT_STATE_IDLE;

    if (g_result == AT_RESULT_OK) {
        ret = 0;
    } else if (g_result == AT_RESULT_ERROR) {
        ret = -1;
    } else {
        /* Woken by the offline edge, not by a result code. */
        ret = -3;
    }

unlock:
    g_cmd = NULL;
    g_resp = NULL;
    g_resp_size = 0;
    rtos_unlock_mutex(&g_cmd_lock);
    return ret;
}
