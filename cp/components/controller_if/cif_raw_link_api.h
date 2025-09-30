#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "cif_main.h"

#define WIFI_API_IPC_COM_REQ_MAX_ARGC           6

/**
 * WIFI_API_MEM_ALIGNMENT: should be set to the alignment of the CPU
 *    4 byte alignment -> #define WIFI_API_MEM_ALIGNMENT 4
 *    2 byte alignment -> #define WIFI_API_MEM_ALIGNMENT 2
 */
#define CIF_RAW_LINK_MEM_ALIGNMENT                   4

/** Calculate memory size for an aligned buffer - returns the next highest
 * multiple of WIFI_API_MEM_ALIGNMENT (e.g. LWIP_MEM_ALIGN_SIZE(3) and
 * LWIP_MEM_ALIGN_SIZE(4) will both yield 4 for WIFI_API_MEM_ALIGNMENT == 4).
 */
#define CIF_RAW_LINK_MEM_ALIGN_SIZE(size) (((size) + CIF_RAW_LINK_MEM_ALIGNMENT - 1U) & ~(CIF_RAW_LINK_MEM_ALIGNMENT-1U))

bk_err_t bk_rlk_scan_cfm_cp_cb(bk_rlk_scan_result_t *result);
bk_err_t bk_rlk_acs_cfm_cp_cb(const uint32_t chanstatus[],uint32_t num_channel,uint32_t best_channel);
bk_err_t bk_rlk_recv_cp_cb(bk_rlk_recv_info_t *rx_info);
void bk_rlk_send_ex_cp_cb(void *args, bool status);
bk_err_t bk_rlk_send_cp_cb(const uint8_t *peer_mac_addr, bk_rlk_send_status_t status);

#ifdef __cplusplus
}
#endif
