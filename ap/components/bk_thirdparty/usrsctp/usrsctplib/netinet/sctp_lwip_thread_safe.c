#if defined(ESP_PLATFORM) && defined(SCTP_USE_LWIP)
#include <stdlib.h>
#include <string.h>

#include "lwip/tcpip.h"
#include "lwip/err.h"
#include "lwip/netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "sctp_os_userspace.h"
#include "sctp_pcb.h"
#include "sctp_var.h"
#include "sctp_constants.h"

#define LWIP_IF_NUM_MAX 256

// Structure to pass data between threads for init_ifns
typedef struct {
    uint32_t vrfid;
    SemaphoreHandle_t semaphore;
} init_ifns_thread_data_t;

// Function to run in TCPIP thread for init_ifns
static void init_ifns_tcpip_thread(void *arg)
{
    init_ifns_thread_data_t *thread_data = (init_ifns_thread_data_t *)arg;
    struct sctp_ifa *sctp_ifa;
    uint32_t ifa_flags;
    uint32_t if_num;
    char if_name[NETIF_NAMESIZE];

    /* Allocate for sockaddr_in6 which is larger than sockaddr_in */
    struct sockaddr* in_addr = malloc(sizeof(struct sockaddr_in6));
    if (in_addr == NULL) {
        xSemaphoreGive(thread_data->semaphore);
        return;
    }

    for(if_num = 1; if_num < LWIP_IF_NUM_MAX; if_num++) {
        struct netif* tmp_if = netif_get_by_index(if_num);
        char * tmp_name = netif_index_to_name(if_num, if_name);

        /* Zero out the larger sockaddr_in6 structure */
        memset(in_addr, 0, sizeof(struct sockaddr_in6));

        if(tmp_if != NULL) {
            if(ip_addr_isloopback(&tmp_if->ip_addr)) {
                continue;
            }

            if(tmp_if->ip_addr.type == IPADDR_TYPE_V4) {
                in_addr->sa_family = AF_INET;
                memcpy(&((struct sockaddr_in *)in_addr)->sin_addr, &tmp_if->ip_addr.u_addr, sizeof(uint32_t));
            } else {
                in_addr->sa_family = AF_INET6;
                memcpy(&((struct sockaddr_in6 *)in_addr)->sin6_addr, &tmp_if->ip_addr.u_addr, sizeof(uint32_t) * 4);
            }

#if !defined(INET)
            if (in_addr->sa_family != AF_INET6) {
                /* non inet6 skip */
                continue;
            }
#elif !defined(INET6)
            if (in_addr->sa_family != AF_INET) {
                /* non inet skip */
                continue;
            }
#else
            if ((in_addr->sa_family != AF_INET) && (in_addr->sa_family != AF_INET6)) {
                /* non inet/inet6 skip */
                continue;
            }
#endif
#if defined(INET6)
            if ((in_addr->sa_family == AF_INET6) &&
                IN6_IS_ADDR_UNSPECIFIED(&((struct sockaddr_in6 *)in_addr)->sin6_addr)) {
                /* skip unspecifed addresses */
                continue;
            }
#endif
#if defined(INET)
            if (in_addr->sa_family == AF_INET &&
                ((struct sockaddr_in *)in_addr)->sin_addr.s_addr == 0) {
                continue;
            }
#endif
            ifa_flags = 0;
            sctp_ifa = sctp_add_addr_to_vrf(thread_data->vrfid,
                                          NULL,
                                          if_num,
                                          0,
                                          tmp_name,
                                          NULL,
                                          (struct sockaddr*)in_addr,
                                          ifa_flags,
                                          0);
            if (sctp_ifa) {
                sctp_ifa->localifa_flags &= ~SCTP_ADDR_DEFER_USE;
            }
        } else {
            break;
        }
    }

    free(in_addr);
    xSemaphoreGive(thread_data->semaphore);
}

// Thread-safe wrapper for sctp_init_ifns_for_vrf
void sctp_lwip_init_ifns_for_vrf_safe(uint32_t vrfid)
{
    // Allocate the thread data on the heap to ensure it remains valid
    // when the TCPIP thread processes it
    init_ifns_thread_data_t *thread_data = malloc(sizeof(init_ifns_thread_data_t));
    if (thread_data == NULL) {
        return;
    }

    thread_data->vrfid = vrfid;

    // Create semaphore
    thread_data->semaphore = xSemaphoreCreateBinary();
    if (thread_data->semaphore == NULL) {
        free(thread_data);
        return;
    }

    // Execute in TCPIP thread
    err_t err = tcpip_callback(init_ifns_tcpip_thread, thread_data);
    if (err != ERR_OK) {
        vSemaphoreDelete(thread_data->semaphore);
        free(thread_data);
        return;
    }

    // Wait for completion (with timeout)
    if (xSemaphoreTake(thread_data->semaphore, pdMS_TO_TICKS(5000)) != pdTRUE) {
        // Even if timeout occurs, we can't free the data yet as the TCPIP thread might still be using it
        // Just return and accept the memory leak in this rare timeout case
        return;
    }

    vSemaphoreDelete(thread_data->semaphore);
    free(thread_data);
}
#endif
