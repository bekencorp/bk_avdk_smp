#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <common/bk_err.h>
#include <stdbool.h>
#include <stdint.h>

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
