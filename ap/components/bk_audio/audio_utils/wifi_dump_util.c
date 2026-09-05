#include "sdkconfig.h"

#if CONFIG_ADK_WIFI_DUMP_UTIL

#include <common/bk_include.h>
#include <common/bk_err.h>
#include <os/os.h>
#include <os/mem.h>
#include "lwip/sockets.h"
#include <components/bk_audio/audio_utils/wifi_dump_util.h>

#define WIFI_DUMP_TAG "wifi_dump"
#define WIFI_DUMP_PACKET_MAX_SIZE (4096)

#ifndef CONFIG_ADK_WIFI_DUMP_QUEUE_DEPTH
#define CONFIG_ADK_WIFI_DUMP_QUEUE_DEPTH (24)
#endif

typedef struct
{
    uint16_t len;
    uint8_t data[WIFI_DUMP_PACKET_MAX_SIZE];
} wifi_dump_packet_t;

typedef struct
{
    bool initialized;
    uint16_t port;
    volatile int client_fd;
    beken_thread_t thread;
    beken_queue_t queue;
    beken_mutex_t pool_lock;
    uint32_t pool_used;
    uint32_t enqueue_fail_count;
    wifi_dump_packet_t *packets;
} wifi_dump_context_t;

static wifi_dump_context_t s_wifi_dump = {
    .client_fd = -1,
};

static wifi_dump_packet_t *wifi_dump_alloc_packets(void)
{
    uint32_t size = sizeof(wifi_dump_packet_t) * CONFIG_ADK_WIFI_DUMP_QUEUE_DEPTH;
#if CONFIG_PSRAM
    return (wifi_dump_packet_t *)psram_malloc(size);
#else
    return (wifi_dump_packet_t *)os_malloc(size);
#endif
}

static void wifi_dump_free_packets(wifi_dump_packet_t *packets)
{
#if CONFIG_PSRAM
    psram_free(packets);
#else
    os_free(packets);
#endif
}

static void wifi_dump_release_packet(uint8_t index)
{
    rtos_lock_mutex(&s_wifi_dump.pool_lock);
    s_wifi_dump.pool_used &= ~(1UL << index);
    rtos_unlock_mutex(&s_wifi_dump.pool_lock);
}

static int wifi_dump_acquire_packet(void)
{
    int index = -1;

    rtos_lock_mutex(&s_wifi_dump.pool_lock);
    for (int i = 0; i < CONFIG_ADK_WIFI_DUMP_QUEUE_DEPTH; i++)
    {
        if ((s_wifi_dump.pool_used & (1UL << i)) == 0)
        {
            s_wifi_dump.pool_used |= (1UL << i);
            index = i;
            break;
        }
    }
    rtos_unlock_mutex(&s_wifi_dump.pool_lock);
    return index;
}

static void wifi_dump_flush_queue(void)
{
    uint8_t index;

    while (rtos_pop_from_queue(&s_wifi_dump.queue, &index, BEKEN_NO_WAIT) == BK_OK)
    {
        wifi_dump_release_packet(index);
    }
}

static int wifi_dump_send_all(int fd, const uint8_t *data, uint32_t len)
{
    uint32_t sent = 0;

    while (sent < len)
    {
        int ret = send(fd, data + sent, len - sent, 0);
        if (ret <= 0)
        {
            return -1;
        }
        sent += (uint32_t)ret;
    }
    return 0;
}

static void wifi_dump_server_thread(beken_thread_arg_t arg)
{
    int listen_fd = -1;
    struct sockaddr_in server_addr;
    (void)arg;

    listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_fd < 0)
    {
        BK_LOGE(WIFI_DUMP_TAG, "socket create failed\n");
        goto exit;
    }

    int reuse = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    os_memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(s_wifi_dump.port);

    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0
        || listen(listen_fd, 1) < 0)
    {
        BK_LOGE(WIFI_DUMP_TAG, "listen on port %u failed\n", s_wifi_dump.port);
        goto exit;
    }

    BK_LOGI(WIFI_DUMP_TAG, "TCP server listening on port %u\n", s_wifi_dump.port);
    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd < 0)
        {
            continue;
        }

        struct timeval timeout = {
            .tv_sec = 2,
            .tv_usec = 0,
        };
        setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
        s_wifi_dump.client_fd = client_fd;
        s_wifi_dump.enqueue_fail_count = 0;
        wifi_dump_flush_queue();
        BK_LOGI(WIFI_DUMP_TAG, "client %s connected\n", inet_ntoa(client_addr.sin_addr));

        while (s_wifi_dump.client_fd == client_fd)
        {
            uint8_t index;
            if (rtos_pop_from_queue(&s_wifi_dump.queue, &index, 200) != BK_OK)
            {
                continue;
            }

            wifi_dump_packet_t *packet = &s_wifi_dump.packets[index];
            int ret = wifi_dump_send_all(client_fd, packet->data, packet->len);
            wifi_dump_release_packet(index);
            if (ret != 0)
            {
                BK_LOGE(WIFI_DUMP_TAG, "TCP send failed, capture stopped\n");
                break;
            }
        }

        if (s_wifi_dump.client_fd == client_fd)
        {
            s_wifi_dump.client_fd = -1;
        }
        close(client_fd);
        wifi_dump_flush_queue();
        BK_LOGI(WIFI_DUMP_TAG, "client disconnected\n");
    }

exit:
    if (listen_fd >= 0)
    {
        close(listen_fd);
    }
    s_wifi_dump.initialized = false;
    s_wifi_dump.thread = NULL;
    rtos_delete_thread(NULL);
}

bk_err_t wifi_dump_util_start(uint16_t port)
{
    bk_err_t ret;

    if (s_wifi_dump.initialized)
    {
        return s_wifi_dump.port == port ? BK_OK : BK_FAIL;
    }
    if (port == 0)
    {
        return BK_ERR_PARAM;
    }

    os_memset(&s_wifi_dump, 0, sizeof(s_wifi_dump));
    s_wifi_dump.client_fd = -1;
    s_wifi_dump.port = port;
    s_wifi_dump.packets = wifi_dump_alloc_packets();
    if (!s_wifi_dump.packets)
    {
        BK_LOGE(WIFI_DUMP_TAG, "allocate %u packet slots failed\n",
                CONFIG_ADK_WIFI_DUMP_QUEUE_DEPTH);
        return BK_ERR_NO_MEM;
    }

    ret = rtos_init_mutex(&s_wifi_dump.pool_lock);
    if (ret != BK_OK)
    {
        wifi_dump_free_packets(s_wifi_dump.packets);
        s_wifi_dump.packets = NULL;
        return ret;
    }
    ret = rtos_init_queue(&s_wifi_dump.queue, "wifi_dump_q", sizeof(uint8_t),
                          CONFIG_ADK_WIFI_DUMP_QUEUE_DEPTH);
    if (ret != BK_OK)
    {
        rtos_deinit_mutex(&s_wifi_dump.pool_lock);
        wifi_dump_free_packets(s_wifi_dump.packets);
        s_wifi_dump.packets = NULL;
        return ret;
    }

    s_wifi_dump.initialized = true;
    ret = rtos_create_thread(&s_wifi_dump.thread, BEKEN_APPLICATION_PRIORITY,
                             "wifi_dump", wifi_dump_server_thread, 4096, NULL);
    if (ret != BK_OK)
    {
        s_wifi_dump.initialized = false;
        rtos_deinit_queue(&s_wifi_dump.queue);
        rtos_deinit_mutex(&s_wifi_dump.pool_lock);
        wifi_dump_free_packets(s_wifi_dump.packets);
        s_wifi_dump.packets = NULL;
        return ret;
    }
    return BK_OK;
}

void wifi_dump_util_close_client(void)
{
    int client_fd = s_wifi_dump.client_fd;

    s_wifi_dump.client_fd = -1;
    if (client_fd >= 0)
    {
        shutdown(client_fd, SHUT_RDWR);
    }
    if (s_wifi_dump.initialized)
    {
        wifi_dump_flush_queue();
    }
}

bool wifi_dump_util_is_connected(void)
{
    return s_wifi_dump.initialized && s_wifi_dump.client_fd >= 0;
}

bk_err_t wifi_dump_util_enqueue(const void *header, uint32_t header_len,
                                const void *data0, uint32_t len0,
                                const void *data1, uint32_t len1,
                                const void *data2, uint32_t len2)
{
    uint32_t total_len = header_len + len0 + len1 + len2;
    uint32_t offset = 0;
    uint8_t queue_index;
    int index;

    if (!wifi_dump_util_is_connected())
    {
        return BK_FAIL;
    }
    if (!header || !data0 || !data1 || !data2
        || total_len > WIFI_DUMP_PACKET_MAX_SIZE)
    {
        return BK_ERR_PARAM;
    }

    index = wifi_dump_acquire_packet();
    if (index < 0)
    {
        s_wifi_dump.enqueue_fail_count++;
        if (s_wifi_dump.enqueue_fail_count == 1)
        {
            BK_LOGE(WIFI_DUMP_TAG, "queue overflow, capture data is incomplete\n");
        }
        return BK_ERR_NO_MEM;
    }

    wifi_dump_packet_t *packet = &s_wifi_dump.packets[index];
    os_memcpy(packet->data + offset, header, header_len);
    offset += header_len;
    os_memcpy(packet->data + offset, data0, len0);
    offset += len0;
    os_memcpy(packet->data + offset, data1, len1);
    offset += len1;
    os_memcpy(packet->data + offset, data2, len2);
    packet->len = (uint16_t)total_len;

    queue_index = (uint8_t)index;
    if (rtos_push_to_queue(&s_wifi_dump.queue, &queue_index, BEKEN_NO_WAIT) != BK_OK)
    {
        wifi_dump_release_packet(queue_index);
        return BK_FAIL;
    }
    return BK_OK;
}

#endif /* CONFIG_ADK_WIFI_DUMP_UTIL */
