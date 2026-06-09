#pragma once

#include <common/bk_err.h>

#ifdef __cplusplus
extern "C" {
#endif

void bk_cherryusb_register_host_classes(void);
void bk_cherryusb_register_device_classes(void);

bk_err_t bk_cherryusb_host_open(void);
bk_err_t bk_cherryusb_host_close(void);
bk_err_t bk_cherryusb_device_open(void);
bk_err_t bk_cherryusb_device_close(void);

#ifdef __cplusplus
}
#endif
