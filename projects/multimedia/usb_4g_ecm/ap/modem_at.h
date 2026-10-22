/*
 * Minimal AT command manager over the intf3 vendor channel.
 *
 * Scope: one command at a time, line assembly across USB packets, the final
 * result code, URC delivery, and cancellation when the module goes away. It is
 * not a script engine and holds no knowledge of any particular command.
 */
#ifndef _MODEM_AT_H_
#define _MODEM_AT_H_

#include <stdint.h>
#include <stdbool.h>

/* Longest single response line kept; longer lines are truncated and flagged. */
#define MODEM_AT_LINE_MAX 256U

/* Unsolicited line from the module, delivered with the trailing CRLF stripped.
 * Runs on the USB receive thread, so it must return promptly - anything slow
 * belongs on the caller's own task. */
typedef void (*modem_at_urc_cb_t)(const char *line);

/* Installs the USB callbacks and the URC sink. Call once at startup, before
 * enumeration can reach intf3. urc_cb may be NULL. */
int modem_at_init(modem_at_urc_cb_t urc_cb);

/* Called on the USB connect thread when intf3 is claimed or released. Must
 * return immediately. NULL clears the hook. Set it before the host is opened. */
typedef void (*modem_at_link_cb_t)(bool online);
void modem_at_set_link_hook(modem_at_link_cb_t hook);

/* True while intf3 is claimed. */
bool modem_at_is_online(void);

/* Send one command and wait for its result code.
 *
 *   cmd        command text without the terminating CR, e.g. "AT+CSQ"
 *   resp       optional buffer for the informational lines, CRLF-separated and
 *              NUL-terminated; the echo and the final result code are not
 *              copied into it
 *   resp_size  size of resp, ignored when resp is NULL
 *   timeout_ms wait for the result code, counted from the end of the write
 *
 * Returns 0 when the module answered OK. Negative values: -1 on a module error
 * (ERROR, +CME ERROR, +CMS ERROR - the error line is still copied to resp),
 * -2 on timeout, -3 when offline or disconnected mid-wait, -4 on a USB write
 * failure, -5 when another command is still in flight.
 */
int modem_at_send(const char *cmd, char *resp, uint32_t resp_size, uint32_t timeout_ms);

#endif /* _MODEM_AT_H_ */
