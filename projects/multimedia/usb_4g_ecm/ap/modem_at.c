/*
 * Minimal AT command manager. See modem_at.h for the contract.
 *
 * Command state and response storage are protected from RX/caller races.
 * Known URCs are separated from command replies; callbacks run without locks.
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

static uint8_t g_state = AT_STATE_IDLE;
static uint8_t g_result = AT_RESULT_NONE;

/* Serialises callers; g_state alone cannot, two callers could both see IDLE. */
static beken_mutex_t g_cmd_lock = NULL;
/* Protects shared state between RX thread and caller: s_cmd_buf, s_resp_buf,
 * s_resp_len, g_result. Held briefly to snapshot or update, never held while
 * waiting for response or calling URC callback. */
static beken_mutex_t g_state_lock = NULL;
/* Given by the receive thread when a final result code lands. */
static beken_semaphore_t g_done_sem = NULL;

static modem_at_urc_cb_t g_urc_cb = NULL;
static modem_at_link_cb_t g_link_hook = NULL;

/* Partial line state is shared with link reset; protected by g_state_lock. */
static char g_line[MODEM_AT_LINE_MAX + 1];
static uint32_t g_line_len = 0;
static bool g_line_over = false; /* current line exceeded the buffer */

/* Internal command buffer. The caller's cmd pointer is copied here under
 * g_state_lock, so the RX thread never dereferences the caller's stack. */
static char s_cmd_buf[USB_MODEM_AT_TX_MAX];
/* Internal response buffer. The receive thread writes here, not to the caller's
 * stack - a pointer into the caller's stack becomes a dangling pointer the
 * instant the caller is woken. The caller copies out under g_state_lock. */
/* AT+ECPCFG? answers with one line per option and "usbNet" is near the end.
 * 257 bytes drops that line, so a later check cannot tell ECM from RNDIS. */
static char s_resp_buf[1280];
static uint32_t s_resp_len = 0;

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
    uint32_t need = len + (s_resp_len ? 2 : 0);

    if (s_resp_len + need + 1 > sizeof(s_resp_buf)) {
        LOGD("resp full, drop: %s\r\n", line);
        return;
    }
    if (s_resp_len) {
        s_resp_buf[s_resp_len++] = '\r';
        s_resp_buf[s_resp_len++] = '\n';
    }
    memcpy(&s_resp_buf[s_resp_len], line, len);
    s_resp_len += len;
    s_resp_buf[s_resp_len] = '\0';
}

/* Called with state lock held. Query replies take precedence over URCs. */
static bool at_is_urc(const char *line)
{
    static const char *const queries[] = { "AT+CREG?", "AT+CGREG?", "AT+CEREG?" };
    static const char *const prefixes[] = { "+CREG:", "+CGREG:", "+CEREG:" };
    for (unsigned i = 0; i < sizeof(queries) / sizeof(queries[0]); i++) {
        if (!strncmp(line, prefixes[i], strlen(prefixes[i]))) {
            return strcmp(s_cmd_buf, queries[i]) != 0;
        }
    }
    return !strncmp(line, "+CGEV:", 6) || !strcmp(line, "RING") ||
           !strcmp(line, "NO CARRIER");
}

static void at_on_line(const char *line)
{
    bool urc = false;
    int result = at_classify(line);

    rtos_lock_mutex(&g_state_lock);
    if (g_state == AT_STATE_IDLE ||
        at_is_urc(line)) {
        urc = true;
    } else if (g_state == AT_STATE_DRAIN) {
        if (result != AT_RESULT_NONE) {
            rtos_set_semaphore(&g_done_sem);
        }
    } else if (g_result == AT_RESULT_NONE && strcmp(line, s_cmd_buf)) {
        if (result != AT_RESULT_NONE) {
            if (result == AT_RESULT_ERROR) {
                at_resp_append(line);
            }
            g_result = (uint8_t)result;
            /* Signal before unlocking: timeout/new-command cannot overtake it. */
            rtos_set_semaphore(&g_done_sem);
        } else {
            at_resp_append(line);
        }
    }
    rtos_unlock_mutex(&g_state_lock);
    if (urc) {
        if (g_urc_cb) {
            g_urc_cb(line);
        } else {
            LOGD("urc: %s\r\n", line);
        }
    }
}

static void at_on_rx(const uint8_t *data, uint32_t len)
{
    char line[MODEM_AT_LINE_MAX + 1];
    for (uint32_t i = 0; i < len; i++) {
        bool complete = false;
        char c = (char)data[i];
        rtos_lock_mutex(&g_state_lock);
        if (c == '\r' || c == '\n') {
            if (!g_line_over && g_line_len) {
                memcpy(line, g_line, g_line_len);
                line[g_line_len] = '\0';
                complete = true;
            }
            g_line_len = 0;
            g_line_over = false;
        } else if (g_line_len < MODEM_AT_LINE_MAX) {
            g_line[g_line_len++] = c;
        } else {
            g_line_over = true;
        }
        rtos_unlock_mutex(&g_state_lock);
        if (complete) {
            at_on_line(line);
        }
    }
}

static void at_on_link(bool online)
{
    LOGI("AT channel %s\r\n", online ? "online" : "offline");
    rtos_lock_mutex(&g_state_lock);
    g_line_len = 0;
    g_line_over = false;
    if (!online && g_state != AT_STATE_IDLE) {
        /* Cancel the transaction even if a result arrived at the same edge. */
        g_result = AT_RESULT_NONE;
        g_state = AT_STATE_IDLE;
        rtos_set_semaphore(&g_done_sem);
    }
    rtos_unlock_mutex(&g_state_lock);
    if (g_link_hook) {
        g_link_hook(online);
    }
}

void modem_at_set_link_hook(modem_at_link_cb_t hook)
{
    g_link_hook = hook;
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
    err = rtos_init_mutex(&g_state_lock);
    if (err != BK_OK) {
        LOGE("state lock init failed %d\r\n", err);
        rtos_deinit_mutex(&g_cmd_lock);
        g_cmd_lock = NULL;
        return err;
    }
    err = rtos_init_semaphore(&g_done_sem, 1);
    if (err != BK_OK) {
        LOGE("sem init failed %d\r\n", err);
        rtos_deinit_mutex(&g_state_lock);
        g_state_lock = NULL;
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

    rtos_lock_mutex(&g_state_lock);
    while (rtos_get_semaphore(&g_done_sem, BEKEN_NO_WAIT) == BK_OK) {
    }
    memcpy(tx, cmd, cmd_len);
    tx[cmd_len] = '\r';
    memcpy(s_cmd_buf, cmd, cmd_len + 1);
    s_resp_len = 0;
    g_result = AT_RESULT_NONE;
    g_state = AT_STATE_WAIT;
    rtos_unlock_mutex(&g_state_lock);

    written = usb_modem_at_write((const uint8_t *)tx, cmd_len + 1, timeout_ms);
    if (written != (int)(cmd_len + 1)) {
        ret = usb_modem_at_is_online() ? -4 : -3;
        rtos_lock_mutex(&g_state_lock);
        goto finish;
    }

    if (rtos_get_semaphore(&g_done_sem, timeout_ms) != BK_OK) {
        rtos_lock_mutex(&g_state_lock);
        if (g_result == AT_RESULT_NONE && g_state == AT_STATE_WAIT) {
            g_state = AT_STATE_DRAIN;
            rtos_unlock_mutex(&g_state_lock);
            rtos_get_semaphore(&g_done_sem, MODEM_AT_DRAIN_MS);
            rtos_lock_mutex(&g_state_lock);
            ret = g_state == AT_STATE_IDLE ? -3 : -2;
            goto finish;
        }
        /* A final result or offline notification won the timeout race. */
    } else {
        rtos_lock_mutex(&g_state_lock);
    }

    ret = g_result == AT_RESULT_OK ? 0 :
          g_result == AT_RESULT_ERROR ? -1 : -3;
    if ((ret == 0 || ret == -1) && resp && resp_size) {
        uint32_t n = s_resp_len < resp_size - 1 ? s_resp_len : resp_size - 1;
        memcpy(resp, s_resp_buf, n);
        resp[n] = '\0';
    }
finish:
    g_state = AT_STATE_IDLE;
    rtos_unlock_mutex(&g_state_lock);
unlock:
    rtos_unlock_mutex(&g_cmd_lock);
    return ret;
}
