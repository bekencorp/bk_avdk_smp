#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <common/bk_err.h>
#include <stdbool.h>
#include <stdint.h>

/* Multiple independent WiFi dump channels, each with its own listen port and
 * client connection, so audio and SSL3 can be captured simultaneously.
 * Instance 0 is the audio AEC capture (reusing the original public API for
 * backward compatibility), instance 1 is the 3-mic SSL3 result report. */
typedef enum {
    WIFI_DUMP_INSTANCE_AUDIO = 0,
    WIFI_DUMP_INSTANCE_SSL3 = 1,
    WIFI_DUMP_INSTANCE_MAX = 2,
} wifi_dump_instance_t;

/* Per-instance API: explicitly select the channel. */
bk_err_t wifi_dump_util_start_at(wifi_dump_instance_t inst, uint16_t port);
void wifi_dump_util_close_client_at(wifi_dump_instance_t inst);
bool wifi_dump_util_is_connected_at(wifi_dump_instance_t inst);
bk_err_t wifi_dump_util_enqueue_at(wifi_dump_instance_t inst,
                                   const void *header, uint32_t header_len,
                                   const void *data0, uint32_t len0,
                                   const void *data1, uint32_t len1,
                                   const void *data2, uint32_t len2);

bk_err_t wifi_dump_util_start(uint16_t port);
void wifi_dump_util_close_client(void);
bool wifi_dump_util_is_connected(void);
bk_err_t wifi_dump_util_enqueue(const void *header, uint32_t header_len,
                                const void *data0, uint32_t len0,
                                const void *data1, uint32_t len1,
                                const void *data2, uint32_t len2);

#ifdef __cplusplus
}
#endif
