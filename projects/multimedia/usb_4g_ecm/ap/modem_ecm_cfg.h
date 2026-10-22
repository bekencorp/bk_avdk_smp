#ifndef _MODEM_ECM_CFG_H_
#define _MODEM_ECM_CFG_H_

#include <stdbool.h>

/* Link hook for modem_at_set_link_hook(). Returns immediately. */
void modem_ecm_cfg_on_link(bool online);

#endif /* _MODEM_ECM_CFG_H_ */
