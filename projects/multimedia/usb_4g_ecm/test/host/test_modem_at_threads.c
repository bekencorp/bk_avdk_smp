/* RX runs on a real thread; callbacks also re-enter send to check lock ordering. */
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdatomic.h>
#include <os/os.h>
#include "usb_modem_at.h"
#include "modem_at.h"

static usb_modem_at_rx_cb_t rx_cb;
static usb_modem_at_link_cb_t link_cb;
static pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t queue_cv = PTHREAD_COND_INITIALIZER;
static int pending, quit;
static atomic_int urcs;
static atomic_bool online = true;

void usb_modem_at_init(usb_modem_at_rx_cb_t rx, usb_modem_at_link_cb_t link) {
    rx_cb = rx; link_cb = link;
}
bool usb_modem_at_is_online(void) { return atomic_load(&online); }
int usb_modem_at_write(const uint8_t *data, uint32_t len, uint32_t timeout) {
    (void)data; (void)timeout;
    pthread_mutex_lock(&queue_lock); pending++;
    pthread_cond_signal(&queue_cv); pthread_mutex_unlock(&queue_lock); return (int)len;
}
static void on_urc(const char *line) {
    assert(!strcmp(line, "+CGEV: ME DETACH"));
    assert(modem_at_send("AT", NULL, 0, 10) == -5);
    atomic_fetch_add(&urcs, 1);
}
static void *receiver(void *unused) {
    (void)unused;
    for (int n = 0;; n++) {
        pthread_mutex_lock(&queue_lock);
        while (!pending && !quit) pthread_cond_wait(&queue_cv, &queue_lock);
        if (quit) { pthread_mutex_unlock(&queue_lock); return NULL; }
        pending--; pthread_mutex_unlock(&queue_lock);
        if (n % 7 == 0) {
            atomic_store(&online, false); link_cb(false);
        } else {
            const char *chunks[] = { "AT+CEREG?\r\n+CERE", "G: 2,1\r\n", "+CGEV: ME DETACH\r\n", "OK\r\n" };
            for (unsigned i = 0; i < 4; i++) rx_cb((const uint8_t *)chunks[i], strlen(chunks[i]));
        }
    }
}
int main(void) {
    pthread_t rx; unsigned successes = 0;
    assert(modem_at_init(on_urc) == BK_OK);
    assert(!pthread_create(&rx, NULL, receiver, NULL));
    for (int n = 0; n < 500; n++) {
        char cmd[] = "AT+CEREG?", resp[64];
        int ret = modem_at_send(cmd, resp, sizeof(resp), 1000);
        if (n % 7 == 0) {
            assert(ret == -3); atomic_store(&online, true);
            /* The reset is already performed by the offline callback. */
        } else {
            assert(ret == 0); assert(!strcmp(resp, "+CEREG: 2,1")); successes++;
        }
        memset(cmd, 0xa5, sizeof(cmd));
        memset(resp, 0xa5, sizeof(resp));
    }
    pthread_mutex_lock(&queue_lock); quit = 1; pthread_cond_signal(&queue_cv);
    pthread_mutex_unlock(&queue_lock); pthread_join(rx, NULL);
    assert((unsigned)atomic_load(&urcs) == successes);
    puts("500 threaded command/RX/disconnect iterations passed"); return 0;
}
