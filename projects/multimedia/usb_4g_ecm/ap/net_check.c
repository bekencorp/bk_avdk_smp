/*
 * One connectivity check after DHCP. Resolves one name with the DNS servers
 * DHCP installed, then sends a short HTTP/1.0 request and checks the reply
 * starts with "HTTP/".
 */
#include <string.h>
#include <errno.h>
#include <os/os.h>
#include <components/log.h>
#include "lwip/netif.h"
#include "lwip/ip4_addr.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "net_check.h"

#define TAG "net_check"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define NET_CHECK_NAME "www.qq.com"
#define NET_CHECK_PORT "80"
#define NET_CHECK_WAIT_S 60

extern struct netif g_cdc_ecm_netif;

static const char s_req[] = "GET / HTTP/1.0\r\nHost: " NET_CHECK_NAME "\r\n\r\n";

static void net_check_task(void *arg)
{
    struct addrinfo hints;
    struct addrinfo *res = NULL;
    struct sockaddr_in *sin;
    struct timeval tv;
    char rx[64];
    int fd = -1;
    int n;
    int i;
    int err;

    (void)arg;
    for (i = 0; i < NET_CHECK_WAIT_S; i++) {
        if (!ip4_addr_isany_val(*netif_ip4_addr(&g_cdc_ecm_netif))) {
            break;
        }
        rtos_delay_milliseconds(1000);
    }
    if (ip4_addr_isany_val(*netif_ip4_addr(&g_cdc_ecm_netif))) {
        LOGE("no IPv4 after %d s\r\n", NET_CHECK_WAIT_S);
        goto out;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    err = getaddrinfo(NET_CHECK_NAME, NET_CHECK_PORT, &hints, &res);
    if (err != 0 || res == NULL) {
        LOGE("dns %s failed %d\r\n", NET_CHECK_NAME, err);
        goto out;
    }
    sin = (struct sockaddr_in *)res->ai_addr;
    LOGI("dns %s -> %s\r\n", NET_CHECK_NAME, inet_ntoa(sin->sin_addr));

    fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0) {
        LOGE("socket failed %d\r\n", errno);
        goto out;
    }
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    if (connect(fd, res->ai_addr, res->ai_addrlen) != 0) {
        LOGE("tcp connect failed %d\r\n", errno);
        goto out;
    }
    if (send(fd, s_req, sizeof(s_req) - 1, 0) < 0) {
        LOGE("tcp send failed %d\r\n", errno);
        goto out;
    }
    n = recv(fd, rx, sizeof(rx) - 1, 0);
    if (n < 5 || memcmp(rx, "HTTP/", 5) != 0) {
        LOGE("tcp recv %d\r\n", n);
        goto out;
    }
    rx[n] = '\0';
    LOGI("tcp %s:%s\r\n%s\r\n", NET_CHECK_NAME, NET_CHECK_PORT, rx);

out:
    if (fd >= 0) {
        closesocket(fd);
    }
    if (res != NULL) {
        freeaddrinfo(res);
    }
    rtos_delete_thread(NULL);
}

void net_check_start(void)
{
    beken_thread_t thread = NULL;

    if (rtos_create_thread(&thread, BEKEN_DEFAULT_WORKER_PRIORITY, "net_check",
                           (beken_thread_function_t)net_check_task, 8192, NULL) != BK_OK) {
        LOGE("create net_check failed\r\n");
    }
}
