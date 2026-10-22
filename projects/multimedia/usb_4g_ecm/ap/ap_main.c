#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/log.h>
#include <components/usb.h>
#include <components/usb_types.h>
#include "modem_at.h"
#include "modem_at_cli.h"
#include "modem_ecm_cfg.h"
#include "net_check.h"
#include "speed_test.h"

#define TAG "usb4g"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define USB_HOST_INIT_TASK_PRIORITY    BEKEN_DEFAULT_WORKER_PRIORITY
#define USB_HOST_INIT_TASK_STACK_SIZE  (1024 * 8)
#define USB_HOST_INIT_TASK_NAME        "usb4g_init"

static beken_thread_t s_usb_init_thread = NULL;

/* lwip_intf_v2_1's net.c owns tcpip_init() behind a one-shot guard, but exposes
 * no header for it: every in-tree caller is a Wi-Fi or ETH init path that also
 * creates its own netif, which this project must not do. Declared here so the
 * guard stays the single owner -- a later Wi-Fi init will find it already done
 * instead of calling tcpip_init() twice. */
extern void net_ipv4stack_init(void);

/* Bring the MUSB controller up in USB HOST mode.
 *   bk_init()                   has already registered the adaptation layer + class drivers
 *   bk_usb_open(USB_HOST_MODE)  powers the controller as an A-host (VBUS out)
 * No device gadget, no MSC/UVC. CDC-ECM and the intf3 AT driver bind during
 * enumeration. The link hook checks the module profile; net_check runs once. */
static void usb_host_init_task(void *arg)
{
    /* Must precede enumeration: the ECM class driver hands packets straight to
     * netif->input, so the tcpip thread and its mbox have to exist first. */
    net_ipv4stack_init();
    LOGI("tcpip stack ready\r\n");

    /* Must also precede enumeration: it installs the intf3 transport callbacks,
     * which the class driver invokes from its connect path. */
    bk_err_t ret = modem_at_init(NULL);
    if (ret != BK_OK) {
        LOGE("modem_at_init failed ret=%d\r\n", ret);
        goto out;
    }
    modem_at_cli_init(); /* test entry; a failure here costs nothing but itself */
    speed_test_cli_init();
    modem_at_set_link_hook(modem_ecm_cfg_on_link);
    net_check_start();

    ret = bk_usb_open(USB_HOST_MODE);
    if (ret != BK_OK) {
        LOGE("bk_usb_open(HOST) failed ret=%d\r\n", ret);
    } else {
        LOGI("USB HOST mode active; waiting for device enumeration\r\n");
    }

out:
    s_usb_init_thread = NULL;
    rtos_delete_thread(NULL);
}

int main(void)
{
    bk_init();

    bk_err_t cret = rtos_create_thread(&s_usb_init_thread,
        USB_HOST_INIT_TASK_PRIORITY,
        USB_HOST_INIT_TASK_NAME,
        (beken_thread_function_t)usb_host_init_task,
        USB_HOST_INIT_TASK_STACK_SIZE,
        NULL);
    if (cret != BK_OK) {
        LOGE("create %s task FAILED ret=%d\r\n", USB_HOST_INIT_TASK_NAME, cret);
        return -1;
    }

    return 0;
}
