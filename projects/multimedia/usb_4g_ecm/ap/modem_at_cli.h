/* Console entry for bringing up the intf3 AT channel by hand. */
#ifndef _MODEM_AT_CLI_H_
#define _MODEM_AT_CLI_H_

int modem_at_cli_init(void);

/* Print one exchange the way the module said it: command, info lines, then
 * OK / the error line / timeout. The numeric return stays inside the caller. */
void modem_at_log_exchange(char *tag, const char *cmd, int ret, const char *resp);

#endif /* _MODEM_AT_CLI_H_ */
