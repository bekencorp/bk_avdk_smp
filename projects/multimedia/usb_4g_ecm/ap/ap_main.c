#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/log.h>
#include <components/usb.h>
#include <components/usb_types.h>
#include "modem_at.h"
#include "modem_at_cli.h"

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

/* Thin slice: bring the MUSB controller up in USB HOST mode only.
 *   bk_usb_driver_init()        registers the adaptation layer + class drivers
 *   bk_usb_open(USB_HOST_MODE)  powers the controller as an A-host (VBUS out)
 * No device gadget, no MSC/UVC. CDC-ECM enumeration + lwip glue + the intf3
 * vendor-AT bulk driver are wired in later steps; this task just proves the
 * host stack initialises cleanly on the new project. */
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

    ret = bk_usb_driver_init();
    if (ret != BK_OK) {
        /* Opening the controller on top of a failed adaptation-layer init would
         * hide the real cause behind a second, unrelated error code. */
        LOGE("bk_usb_driver_init failed ret=%d\r\n", ret);
        goto out;
    }

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
