#include "sdkconfig.h"

#if CONFIG_ADK_WIFI_DUMP_UTIL || CONFIG_ADK_SSL3_WIFI_REPORT

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

static const char *const s_thread_names[WIFI_DUMP_INSTANCE_MAX] = {
    "wifi_dump0",
    "wifi_dump1",
};

static const char *const s_queue_names[WIFI_DUMP_INSTANCE_MAX] = {
    "wifi_dump_q0",
    "wifi_dump_q1",
};

static wifi_dump_context_t s_wifi_dump[WIFI_DUMP_INSTANCE_MAX] = {
    { .client_fd = -1 },
    { .client_fd = -1 },
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

static void wifi_dump_release_packet(wifi_dump_context_t *ctx, uint8_t index)
{
    rtos_lock_mutex(&ctx->pool_lock);
    ctx->pool_used &= ~(1UL << index);
    rtos_unlock_mutex(&ctx->pool_lock);
}

static int wifi_dump_acquire_packet(wifi_dump_context_t *ctx)
{
    int index = -1;

    rtos_lock_mutex(&ctx->pool_lock);
    for (int i = 0; i < CONFIG_ADK_WIFI_DUMP_QUEUE_DEPTH; i++)
    {
        if ((ctx->pool_used & (1UL << i)) == 0)
        {
            ctx->pool_used |= (1UL << i);
            index = i;
            break;
        }
    }
    rtos_unlock_mutex(&ctx->pool_lock);
    return index;
}

static void wifi_dump_flush_queue(wifi_dump_context_t *ctx)
{
    uint8_t index;

    while (rtos_pop_from_queue(&ctx->queue, &index, BEKEN_NO_WAIT) == BK_OK)
    {
        wifi_dump_release_packet(ctx, index);
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
    int inst = (int)(uintptr_t)arg;
    int listen_fd = -1;
    struct sockaddr_in server_addr;
    wifi_dump_context_t *ctx;

    if (inst < 0 || inst >= WIFI_DUMP_INSTANCE_MAX)
    {
        BK_LOGE(WIFI_DUMP_TAG, "invalid instance %d\n", inst);
        rtos_delete_thread(NULL);
        return;
    }
    ctx = &s_wifi_dump[inst];

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
    server_addr.sin_port = htons(ctx->port);

    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0
        || listen(listen_fd, 1) < 0)
    {
        BK_LOGE(WIFI_DUMP_TAG, "listen on port %u failed\n", ctx->port);
        goto exit;
    }

    BK_LOGI(WIFI_DUMP_TAG, "TCP server [%d] listening on port %u\n", inst, ctx->port);
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
        ctx->client_fd = client_fd;
        ctx->enqueue_fail_count = 0;
        wifi_dump_flush_queue(ctx);
        BK_LOGI(WIFI_DUMP_TAG, "client [%d] %s connected\n",
                inst, inet_ntoa(client_addr.sin_addr));

        while (ctx->client_fd == client_fd)
        {
            uint8_t index;
            if (rtos_pop_from_queue(&ctx->queue, &index, 200) != BK_OK)
            {
                continue;
            }

            wifi_dump_packet_t *packet = &ctx->packets[index];
            int ret = wifi_dump_send_all(client_fd, packet->data, packet->len);
            wifi_dump_release_packet(ctx, index);
            if (ret != 0)
            {
                BK_LOGE(WIFI_DUMP_TAG, "TCP send failed, capture stopped\n");
                break;
            }
        }

        if (ctx->client_fd == client_fd)
        {
            ctx->client_fd = -1;
        }
        close(client_fd);
        wifi_dump_flush_queue(ctx);
        BK_LOGI(WIFI_DUMP_TAG, "client [%d] disconnected\n", inst);
    }

exit:
    if (listen_fd >= 0)
    {
        close(listen_fd);
    }
    ctx->initialized = false;
    ctx->client_fd = -1;
    wifi_dump_flush_queue(ctx);
    rtos_deinit_queue(&ctx->queue);
    rtos_deinit_mutex(&ctx->pool_lock);
    if (ctx->packets != NULL)
    {
        wifi_dump_free_packets(ctx->packets);
        ctx->packets = NULL;
    }
    ctx->thread = NULL;
    rtos_delete_thread(NULL);
}

bk_err_t wifi_dump_util_start_at(wifi_dump_instance_t inst, uint16_t port)
{
    bk_err_t ret;
    wifi_dump_context_t *ctx;

    if (inst < 0 || inst >= WIFI_DUMP_INSTANCE_MAX || port == 0)
    {
        return BK_ERR_PARAM;
    }
    ctx = &s_wifi_dump[inst];
    if (ctx->initialized)
    {
        return ctx->port == port ? BK_OK : BK_FAIL;
    }

    os_memset(ctx, 0, sizeof(*ctx));
    ctx->client_fd = -1;
    ctx->port = port;
    ctx->packets = wifi_dump_alloc_packets();
    if (!ctx->packets)
    {
        BK_LOGE(WIFI_DUMP_TAG, "allocate %u packet slots failed\n",
                CONFIG_ADK_WIFI_DUMP_QUEUE_DEPTH);
        return BK_ERR_NO_MEM;
    }

    ret = rtos_init_mutex(&ctx->pool_lock);
    if (ret != BK_OK)
    {
        wifi_dump_free_packets(ctx->packets);
        ctx->packets = NULL;
        return ret;
    }
    ret = rtos_init_queue(&ctx->queue, s_queue_names[inst], sizeof(uint8_t),
                          CONFIG_ADK_WIFI_DUMP_QUEUE_DEPTH);
    if (ret != BK_OK)
    {
        rtos_deinit_mutex(&ctx->pool_lock);
        wifi_dump_free_packets(ctx->packets);
        ctx->packets = NULL;
        return ret;
    }

    ctx->initialized = true;
    ret = rtos_create_thread(&ctx->thread, BEKEN_APPLICATION_PRIORITY,
                             s_thread_names[inst], wifi_dump_server_thread,
                             4096, (beken_thread_arg_t)(uintptr_t)inst);
    if (ret != BK_OK)
    {
        ctx->initialized = false;
        rtos_deinit_queue(&ctx->queue);
        rtos_deinit_mutex(&ctx->pool_lock);
        wifi_dump_free_packets(ctx->packets);
        ctx->packets = NULL;
        return ret;
    }
    return BK_OK;
}

void wifi_dump_util_close_client_at(wifi_dump_instance_t inst)
{
    wifi_dump_context_t *ctx;
    int client_fd;

    if (inst < 0 || inst >= WIFI_DUMP_INSTANCE_MAX)
    {
        return;
    }
    ctx = &s_wifi_dump[inst];
    client_fd = ctx->client_fd;

    ctx->client_fd = -1;
    if (client_fd >= 0)
    {
        shutdown(client_fd, SHUT_RDWR);
    }
    if (ctx->initialized)
    {
        wifi_dump_flush_queue(ctx);
    }
}

bool wifi_dump_util_is_connected_at(wifi_dump_instance_t inst)
{
    if (inst < 0 || inst >= WIFI_DUMP_INSTANCE_MAX)
    {
        return false;
    }
    return s_wifi_dump[inst].initialized && s_wifi_dump[inst].client_fd >= 0;
}

bk_err_t wifi_dump_util_enqueue_at(wifi_dump_instance_t inst,
                                   const void *header, uint32_t header_len,
                                   const void *data0, uint32_t len0,
                                   const void *data1, uint32_t len1,
                                   const void *data2, uint32_t len2)
{
    uint32_t total_len = header_len + len0 + len1 + len2;
    uint32_t offset = 0;
    uint8_t queue_index;
    int index;
    wifi_dump_context_t *ctx;

    if (inst < 0 || inst >= WIFI_DUMP_INSTANCE_MAX)
    {
        return BK_ERR_PARAM;
    }
    ctx = &s_wifi_dump[inst];

    if (!wifi_dump_util_is_connected_at(inst))
    {
        return BK_FAIL;
    }
    if (!header || !data0 || !data1 || !data2
        || total_len > WIFI_DUMP_PACKET_MAX_SIZE)
    {
        return BK_ERR_PARAM;
    }

    index = wifi_dump_acquire_packet(ctx);
    if (index < 0)
    {
        ctx->enqueue_fail_count++;
        if (ctx->enqueue_fail_count == 1)
        {
            BK_LOGE(WIFI_DUMP_TAG, "queue overflow, capture data is incomplete\n");
        }
        return BK_ERR_NO_MEM;
    }

    wifi_dump_packet_t *packet = &ctx->packets[index];
    os_memcpy(packet->data + offset, header, header_len);
    offset += header_len;
    os_memcpy(packet->data + offset, data0, len0);
    offset += len0;
    os_memcpy(packet->data + offset, data1, len1);
    offset += len1;
    os_memcpy(packet->data + offset, data2, len2);
    packet->len = (uint16_t)total_len;

    queue_index = (uint8_t)index;
    if (rtos_push_to_queue(&ctx->queue, &queue_index, BEKEN_NO_WAIT) != BK_OK)
    {
        wifi_dump_release_packet(ctx, queue_index);
        return BK_FAIL;
    }
    return BK_OK;
}

/* Backward-compatible public API: operates on the AUDIO instance. */
bk_err_t wifi_dump_util_start(uint16_t port)
{
    return wifi_dump_util_start_at(WIFI_DUMP_INSTANCE_AUDIO, port);
}

void wifi_dump_util_close_client(void)
{
    wifi_dump_util_close_client_at(WIFI_DUMP_INSTANCE_AUDIO);
}

bool wifi_dump_util_is_connected(void)
{
    return wifi_dump_util_is_connected_at(WIFI_DUMP_INSTANCE_AUDIO);
}

bk_err_t wifi_dump_util_enqueue(const void *header, uint32_t header_len,
                                const void *data0, uint32_t len0,
                                const void *data1, uint32_t len1,
                                const void *data2, uint32_t len2)
{
    return wifi_dump_util_enqueue_at(WIFI_DUMP_INSTANCE_AUDIO,
                                     header, header_len,
                                     data0, len0,
                                     data1, len1,
                                     data2, len2);
}

#endif /* CONFIG_ADK_WIFI_DUMP_UTIL || CONFIG_ADK_SSL3_WIFI_REPORT */
