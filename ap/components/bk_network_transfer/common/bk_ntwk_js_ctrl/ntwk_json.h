#pragma once

#include <os/os.h>
#include <common/bk_err.h>

#include "network_transfer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*ntwk_json_chan_cb_t)(chan_type_t chan_type, uint8_t *data, uint32_t length);

bk_err_t ntwk_json_init(chan_type_t chan_type);
bk_err_t ntwk_json_deinit(chan_type_t chan_type);
bk_err_t ntwk_json_chan_start(chan_type_t chan_type, uint16_t packet_size);
bk_err_t ntwk_json_chan_stop(chan_type_t chan_type);
bk_err_t ntwk_json_clear_rx(chan_type_t chan_type);

bk_err_t ntwk_json_register_send_cb(chan_type_t chan_type, ntwk_json_chan_cb_t cb);
bk_err_t ntwk_json_register_recv_cb(chan_type_t chan_type, ntwk_json_chan_cb_t cb);

int ntwk_json_ctrl_fragment(uint8_t *data, uint32_t length);
int ntwk_json_ctrl_unfragment(uint8_t *data, uint32_t length);

int ntwk_json_get_header_size(void);

#ifdef __cplusplus
}
#endif
