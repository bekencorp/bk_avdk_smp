/*
 * Downlink check, same idea as the Espressif ECM example: GET one HTTP file,
 * read the body and drop it, print the rate. Stops after 10 seconds so a
 * 20 MB file is not pulled to the end. One run at a time.
 */
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <os/os.h>
#include <components/log.h>
#include <bk_private/bk_cli.h>
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "speed_test.h"

#define TAG "dltest"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define DL_HOST "mirrors.ustc.edu.cn"
#define DL_PORT "80"
#define DL_PATH "/ros/ubuntu/db/references.db"
#define DL_MS   10000U

static const char s_req[] =
    "GET " DL_PATH " HTTP/1.0\r\n"
    "Host: " DL_HOST "\r\n"
    "\r\n";

static volatile int s_busy;

static int hdr_end(const char *buf, int n)
{
    int i;

    for (i = 0; i + 3 < n; i++) {
        if (buf[i] == '\r' && buf[i + 1] == '\n' &&
            buf[i + 2] == '\r' && buf[i + 3] == '\n') {
            return i + 4;
        }
    }
    return -1;
}

static uint32_t kbps_of(uint32_t bytes, uint32_t ms)
{
    if (ms == 0) {
        ms = 1;
    }
    return (uint32_t)(((uint64_t)bytes * 8) / ms);
}

static void close_now(int fd)
{
    struct linger ling;

    ling.l_onoff = 1;
    ling.l_linger = 0;
    setsockopt(fd, SOL_SOCKET, SO_LINGER, &ling, sizeof(ling));
    closesocket(fd);
}

static void dl_task(void *arg)
{
    struct addrinfo hints;
    struct addrinfo *res = NULL;
    struct sockaddr_in *sin;
    struct timeval tv;
    char hdr[768];
    char rx[1460];
    int fd = -1;
    int n;
    int hdr_n = 0;
    int body_at = -1;
    int err;
    uint32_t t0 = 0;
    uint32_t tick = 0;
    uint32_t sec_bytes = 0;
    uint32_t total = 0;
    uint32_t sec = 0;

    (void)arg;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    err = getaddrinfo(DL_HOST, DL_PORT, &hints, &res);
    if (err != 0 || res == NULL) {
        LOGE("dns %s failed %d\r\n", DL_HOST, err);
        goto out;
    }
    sin = (struct sockaddr_in *)res->ai_addr;
    LOGI("dns %s -> %s\r\n", DL_HOST, inet_ntoa(sin->sin_addr));

    fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0) {
        LOGE("socket failed %d\r\n", errno);
        goto out;
    }
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    {
        int nb = 1;
        int soerr = 0;
        socklen_t slen = sizeof(soerr);
        fd_set wfds;

        ioctlsocket(fd, FIONBIO, &nb);
        if (connect(fd, res->ai_addr, res->ai_addrlen) != 0 && errno != EINPROGRESS) {
            LOGE("connect failed %d\r\n", errno);
            goto out;
        }
        FD_ZERO(&wfds);
        FD_SET(fd, &wfds);
        if (select(fd + 1, NULL, &wfds, NULL, &tv) <= 0) {
            LOGE("connect timeout\r\n");
            goto out;
        }
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &soerr, &slen) != 0 || soerr != 0) {
            LOGE("connect failed %d\r\n", soerr != 0 ? soerr : errno);
            goto out;
        }
        nb = 0;
        ioctlsocket(fd, FIONBIO, &nb);
    }
    if (send(fd, s_req, sizeof(s_req) - 1, 0) < 0) {
        LOGE("send failed %d\r\n", errno);
        goto out;
    }

    while (hdr_n < (int)sizeof(hdr) - 1 && body_at < 0) {
        n = recv(fd, hdr + hdr_n, sizeof(hdr) - 1 - hdr_n, 0);
        if (n <= 0) {
            LOGE("header recv %d errno %d\r\n", n, errno);
            goto out;
        }
        hdr_n += n;
        hdr[hdr_n] = '\0';
        body_at = hdr_end(hdr, hdr_n);
    }
    if (body_at < 0 || strstr(hdr, " 200 ") == NULL) {
        hdr[hdr_n < 80 ? hdr_n : 80] = '\0';
        LOGE("status: %s\r\n", hdr);
        goto out;
    }

    tv.tv_sec = 1;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    t0 = rtos_get_time();
    tick = t0;
    if (hdr_n > body_at) {
        total = (uint32_t)(hdr_n - body_at);
        sec_bytes = total;
    }
    while (rtos_get_time() - t0 < DL_MS) {
        uint32_t now;

        n = recv(fd, rx, sizeof(rx), 0);
        now = rtos_get_time();
        if (n > 0) {
            total += (uint32_t)n;
            sec_bytes += (uint32_t)n;
        }
        if (now - tick >= 1000) {
            LOGI("[%u-%u] sec %u Kbits/sec\r\n",
                 sec, sec + 1, kbps_of(sec_bytes, now - tick));
            sec++;
            sec_bytes = 0;
            tick = now;
        }
        if (n == 0) {
            break;
        }
        if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            LOGE("recv failed %d\r\n", errno);
            break;
        }
    }
    LOGI("[0-%u] sec avg %u Kbits/sec, %u bytes\r\n",
         (rtos_get_time() - t0) / 1000, kbps_of(total, rtos_get_time() - t0), total);

out:
    if (fd >= 0) {
        close_now(fd);
    }
    if (res != NULL) {
        freeaddrinfo(res);
    }
    s_busy = 0;
    rtos_delete_thread(NULL);
}

static void dltest_cmd(char *buf, int len, int argc, char **argv)
{
    beken_thread_t thread = NULL;

    (void)buf;
    (void)len;
    (void)argc;
    (void)argv;
    if (s_busy) {
        LOGI("busy\r\n");
        return;
    }
    s_busy = 1;
    if (rtos_create_thread(&thread, BEKEN_DEFAULT_WORKER_PRIORITY, "dltest",
                           (beken_thread_function_t)dl_task, 8192, NULL) != BK_OK) {
        s_busy = 0;
        LOGE("create dltest failed\r\n");
    }
}

static const struct cli_command s_dl_commands[] = {
    { "dltest", "download 10s from mirrors.ustc.edu.cn and print the rate", dltest_cmd },
};

int speed_test_cli_init(void)
{
    return cli_register_commands(s_dl_commands,
        sizeof(s_dl_commands) / sizeof(struct cli_command));
}
