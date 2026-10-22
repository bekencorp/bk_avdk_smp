/*
 * Bring the Lierda module up as an ECM NIC for this USB host.
 *
 * From 《Lierda LTE Standard 系列 网卡上网应用指导》:
 *   usbCtrl 0          enumerate the network interface
 *   usbNet 1           ECM (Linux / USB-host). 0 is RNDIS.
 *   ECNETCFG nat 1     NIC mode, host address 192.168.10.100.
 *                      nat 0 is bridge mode and gives the host the LTE address.
 *   ECNETDEVCTL 3,1,1  auto-dial cid 1 on boot, saved in NVM.
 * ECPCFG and ECNETCFG take effect after AT+LRST. A value that is already
 * right is left alone, so a matching module is not rebooted every connect.
 * No APN is invented: cid 1 is defined as IP only when it does not exist.
 */
#include <string.h>
#include <os/os.h>
#include <components/log.h>
#include "modem_at.h"
#include "modem_at_cli.h"

#define TAG "modem_cfg"
#define CFG_TIMEOUT_MS 5000U

static volatile bool s_stop;
static volatile bool s_running;

/* Digit after key, or -1 when that option is not in the response. A missing
 * line is left unchanged so a short read cannot force a module reset. */
static int field_digit(const char *resp, const char *key)
{
    const char *p = strstr(resp, key);

    if (!p) {
        return -1;
    }
    p += strlen(key);
    while (*p == ' ' || *p == ':' || *p == '"' || *p == ',') {
        p++;
    }
    if (*p < '0' || *p > '9' || (p[1] >= '0' && p[1] <= '9')) {
        return -1;
    }
    return *p - '0';
}

static bool nat_is_nic(const char *resp)
{
    const char *p = strstr(resp, "\"nat\"");

    if (!p) {
        return false;
    }
    return strstr(p, ",1,\"192.168.10.100\"") != NULL ||
           strstr(p, ",1, \"192.168.10.100\"") != NULL;
}

static int cfg_send(char *resp, uint32_t resp_size, const char *cmd)
{
    int ret;

    if (s_stop || !modem_at_is_online()) {
        return -3;
    }
    resp[0] = '\0';
    ret = modem_at_send(cmd, resp, resp_size, CFG_TIMEOUT_MS);
    modem_at_log_exchange(TAG, cmd, ret, resp);
    return ret;
}

static void modem_ecm_cfg_task(void *arg)
{
    static char resp[1280];
    bool restart = false;
    int ret;

    (void)arg;

    ret = cfg_send(resp, sizeof(resp), "AT+ECPCFG?");
    if (ret == 0) {
        int usb_ctrl = field_digit(resp, "\"usbCtrl\"");
        int usb_net = field_digit(resp, "\"usbNet\"");

        if (usb_ctrl >= 0 && usb_ctrl != 0 &&
            cfg_send(resp, sizeof(resp), "AT+ECPCFG=\"usbCtrl\",0") == 0) {
            restart = true;
        }
        if (!s_stop && usb_net >= 0 && usb_net != 1 &&
            cfg_send(resp, sizeof(resp), "AT+ECPCFG=\"usbNet\",1") == 0) {
            restart = true;
        }
    }

    if (!s_stop) {
        ret = cfg_send(resp, sizeof(resp), "AT+ECNETCFG?");
        if (ret == 0 && strstr(resp, "\"nat\"") && !nat_is_nic(resp)) {
            if (cfg_send(resp, sizeof(resp),
                         "AT+ECNETCFG=\"nat\",1,\"192.168.10.100\"") == 0) {
                restart = true;
            }
        }
    }

    if (!s_stop) {
        ret = cfg_send(resp, sizeof(resp), "AT+CGDCONT?");
        if (ret == 0 && !strstr(resp, "+CGDCONT: 1,")) {
            cfg_send(resp, sizeof(resp), "AT+CGDCONT=1,\"IP\"");
        }
    }

    if (!s_stop) {
        ret = cfg_send(resp, sizeof(resp), "AT+ECNETDEVCTL?");
        if (ret == 0 && !strstr(resp, "+ECNETDEVCTL: 3,1,1")) {
            cfg_send(resp, sizeof(resp), "AT+ECNETDEVCTL=3,1,1");
        }
    }

    /* AT+LRST often ends the channel without a final OK. One attempt is enough;
     * the following enumeration reads the saved values back and does not reset
     * again when they already match. */
    if (restart && !s_stop && modem_at_is_online()) {
        cfg_send(resp, sizeof(resp), "AT+LRST");
    }

    s_running = false;
    rtos_delete_thread(NULL);
}

void modem_ecm_cfg_on_link(bool online)
{
    beken_thread_t thread = NULL;

    if (!online) {
        s_stop = true;
        return;
    }
    if (s_running) {
        return;
    }
    s_stop = false;
    s_running = true;
    if (rtos_create_thread(&thread, BEKEN_DEFAULT_WORKER_PRIORITY, "modem_ecm_cfg",
                           (beken_thread_function_t)modem_ecm_cfg_task, 4096, NULL) != BK_OK) {
        s_running = false;
    }
}
