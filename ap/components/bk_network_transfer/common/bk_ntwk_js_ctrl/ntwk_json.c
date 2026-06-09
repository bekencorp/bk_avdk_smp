#include <os/mem.h>
#include <components/log.h>

#include "network_type.h"
#include "network_transfer_internal.h"
#include "ntwk_json.h"

#define TAG "ntwk-json"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#ifndef CONFIG_NTWK_CTRL_JSON_RX_MAX_SIZE
#define CONFIG_NTWK_CTRL_JSON_RX_MAX_SIZE 8192
#endif

#define NTWK_JSON_MAGIC_CODE      (0x4A53)
#define NTWK_JSON_VERSION         (1)
#define NTWK_JSON_FLAG_EOF        (1 << 0)

typedef struct
{
    uint16_t magic;
    uint8_t version;
    uint8_t flags;
    uint16_t msg_id;
    uint16_t frag_index;
    uint16_t frag_count;
    uint32_t total_len;
    uint16_t payload_len;
    uint16_t reserved;
    uint8_t payload[];
} __attribute__((__packed__)) ntwk_json_head_t;

typedef struct
{
    bool initialized;
    uint16_t packet_size;
    uint16_t payload_size;
    uint16_t tx_msg_id;
    uint8_t *tx_buf;

    uint8_t *rx_pkt_buf;
    uint16_t rx_pkt_len;
    uint16_t rx_pkt_expected;

    uint8_t *rx_msg_buf;
    uint32_t rx_msg_size;
    uint32_t rx_msg_len;
    uint16_t rx_msg_id;
    uint16_t rx_frag_count;
    uint16_t rx_next_frag;

    ntwk_json_chan_cb_t send_cb;
    ntwk_json_chan_cb_t recv_cb;
} ntwk_json_chan_ctx_t;

static ntwk_json_chan_ctx_t *s_json_chan_mgr[NTWK_TRANS_CHAN_MAX] = {NULL};

int ntwk_json_get_header_size(void)
{
    return sizeof(ntwk_json_head_t);
}

static ntwk_json_chan_ctx_t *ntwk_json_get_ctx(chan_type_t chan_type)
{
    if (chan_type >= NTWK_TRANS_CHAN_MAX)
    {
        return NULL;
    }

    return s_json_chan_mgr[chan_type];
}

static void ntwk_json_reset_message(ntwk_json_chan_ctx_t *ctx)
{
    if (ctx == NULL)
    {
        return;
    }

    if (ctx->rx_msg_buf)
    {
        os_free(ctx->rx_msg_buf);
        ctx->rx_msg_buf = NULL;
    }

    ctx->rx_msg_size = 0;
    ctx->rx_msg_len = 0;
    ctx->rx_msg_id = 0;
    ctx->rx_frag_count = 0;
    ctx->rx_next_frag = 0;
}

static void ntwk_json_reset_packet(ntwk_json_chan_ctx_t *ctx)
{
    if (ctx == NULL)
    {
        return;
    }

    ctx->rx_pkt_len = 0;
    ctx->rx_pkt_expected = 0;
}

bk_err_t ntwk_json_clear_rx(chan_type_t chan_type)
{
    ntwk_json_chan_ctx_t *ctx = ntwk_json_get_ctx(chan_type);

    if (ctx == NULL)
    {
        return BK_OK;
    }

    ntwk_json_reset_packet(ctx);
    ntwk_json_reset_message(ctx);

    return BK_OK;
}

bk_err_t ntwk_json_init(chan_type_t chan_type)
{
    if (chan_type >= NTWK_TRANS_CHAN_MAX)
    {
        return BK_ERR_PARAM;
    }

    if (s_json_chan_mgr[chan_type] != NULL)
    {
        return BK_OK;
    }

    s_json_chan_mgr[chan_type] = ntwk_malloc(sizeof(ntwk_json_chan_ctx_t));
    if (s_json_chan_mgr[chan_type] == NULL)
    {
        LOGE("%s, malloc json chan %d failed\n", __func__, chan_type);
        return BK_ERR_NO_MEM;
    }

    os_memset(s_json_chan_mgr[chan_type], 0, sizeof(ntwk_json_chan_ctx_t));

    return BK_OK;
}

bk_err_t ntwk_json_deinit(chan_type_t chan_type)
{
    if (chan_type >= NTWK_TRANS_CHAN_MAX)
    {
        return BK_ERR_PARAM;
    }

    if (s_json_chan_mgr[chan_type] == NULL)
    {
        return BK_OK;
    }

    (void)ntwk_json_chan_stop(chan_type);
    os_free(s_json_chan_mgr[chan_type]);
    s_json_chan_mgr[chan_type] = NULL;

    return BK_OK;
}

bk_err_t ntwk_json_chan_start(chan_type_t chan_type, uint16_t packet_size)
{
    ntwk_json_chan_ctx_t *ctx = ntwk_json_get_ctx(chan_type);

    if (ctx == NULL)
    {
        return BK_ERR_PARAM;
    }

    if (ctx->initialized)
    {
        return BK_OK;
    }

    if (packet_size <= sizeof(ntwk_json_head_t))
    {
        LOGE("%s, packet size %u too small\n", __func__, packet_size);
        return BK_ERR_PARAM;
    }

    ctx->tx_buf = ntwk_malloc(packet_size);
    if (ctx->tx_buf == NULL)
    {
        LOGE("%s, malloc tx packet failed\n", __func__);
        return BK_ERR_NO_MEM;
    }

    ctx->rx_pkt_buf = ntwk_malloc(packet_size);
    if (ctx->rx_pkt_buf == NULL)
    {
        LOGE("%s, malloc rx packet failed\n", __func__);
        os_free(ctx->tx_buf);
        ctx->tx_buf = NULL;
        return BK_ERR_NO_MEM;
    }

    ctx->packet_size = packet_size;
    ctx->payload_size = packet_size - sizeof(ntwk_json_head_t);
    ctx->initialized = true;

    LOGI("json chan %d started packet=%u payload=%u rx_max=%u\n",
         chan_type, ctx->packet_size, ctx->payload_size,
         CONFIG_NTWK_CTRL_JSON_RX_MAX_SIZE);

    return BK_OK;
}

bk_err_t ntwk_json_chan_stop(chan_type_t chan_type)
{
    ntwk_json_chan_ctx_t *ctx = ntwk_json_get_ctx(chan_type);

    if (ctx == NULL)
    {
        return BK_OK;
    }

    if (ctx->tx_buf)
    {
        os_free(ctx->tx_buf);
        ctx->tx_buf = NULL;
    }

    if (ctx->rx_pkt_buf)
    {
        os_free(ctx->rx_pkt_buf);
        ctx->rx_pkt_buf = NULL;
    }

    ntwk_json_reset_message(ctx);
    ntwk_json_reset_packet(ctx);
    ctx->packet_size = 0;
    ctx->payload_size = 0;
    ctx->initialized = false;

    return BK_OK;
}

bk_err_t ntwk_json_register_send_cb(chan_type_t chan_type, ntwk_json_chan_cb_t cb)
{
    ntwk_json_chan_ctx_t *ctx = ntwk_json_get_ctx(chan_type);

    if (ctx == NULL)
    {
        return BK_ERR_PARAM;
    }

    ctx->send_cb = cb;

    return BK_OK;
}

bk_err_t ntwk_json_register_recv_cb(chan_type_t chan_type, ntwk_json_chan_cb_t cb)
{
    ntwk_json_chan_ctx_t *ctx = ntwk_json_get_ctx(chan_type);

    if (ctx == NULL)
    {
        return BK_ERR_PARAM;
    }

    ctx->recv_cb = cb;

    return BK_OK;
}

static int ntwk_json_fragment_by_type(chan_type_t chan_type, uint8_t *data, uint32_t length)
{
    ntwk_json_chan_ctx_t *ctx = ntwk_json_get_ctx(chan_type);
    uint32_t offset = 0;
    uint16_t frag_count;
    uint16_t frag_index;
    uint16_t msg_id;

    if (ctx == NULL || !ctx->initialized)
    {
        return -1;
    }

    if (data == NULL || length == 0)
    {
        return -1;
    }

    if (length > CONFIG_NTWK_CTRL_JSON_RX_MAX_SIZE)
    {
        LOGE("%s, json length %u over max %u\n",
             __func__, length, CONFIG_NTWK_CTRL_JSON_RX_MAX_SIZE);
        return -1;
    }

    if (ctx->send_cb == NULL)
    {
        LOGE("%s, send callback not registered\n", __func__);
        return -1;
    }

    frag_count = (length + ctx->payload_size - 1) / ctx->payload_size;
    if (frag_count == 0)
    {
        return -1;
    }

    msg_id = ++ctx->tx_msg_id;

    for (frag_index = 0; frag_index < frag_count; frag_index++)
    {
        ntwk_json_head_t *head = (ntwk_json_head_t *)ctx->tx_buf;
        uint16_t payload_len = ctx->payload_size;
        uint16_t frame_len;
        int ret;

        if (length - offset < payload_len)
        {
            payload_len = length - offset;
        }

        head->magic = CHECK_ENDIAN_UINT16(NTWK_JSON_MAGIC_CODE);
        head->version = NTWK_JSON_VERSION;
        head->flags = (frag_index == frag_count - 1) ? NTWK_JSON_FLAG_EOF : 0;
        head->msg_id = CHECK_ENDIAN_UINT16(msg_id);
        head->frag_index = CHECK_ENDIAN_UINT16(frag_index);
        head->frag_count = CHECK_ENDIAN_UINT16(frag_count);
        head->total_len = CHECK_ENDIAN_UINT32(length);
        head->payload_len = CHECK_ENDIAN_UINT16(payload_len);
        head->reserved = 0;

        os_memcpy(head->payload, data + offset, payload_len);
        frame_len = sizeof(ntwk_json_head_t) + payload_len;

        ret = ctx->send_cb(chan_type, ctx->tx_buf, frame_len);
        if (ret < 0)
        {
            LOGE("%s, send fragment %u/%u failed ret=%d\n",
                 __func__, frag_index + 1, frag_count, ret);
            return -1;
        }

        offset += payload_len;
    }

    return length;
}

static int ntwk_json_deliver_message(chan_type_t chan_type, ntwk_json_chan_ctx_t *ctx)
{
    int ret = (int)ctx->rx_msg_len;

    if (ctx->rx_msg_buf == NULL || ctx->rx_msg_len == 0)
    {
        ntwk_json_reset_message(ctx);
        return -1;
    }

    ctx->rx_msg_buf[ctx->rx_msg_len] = '\0';

    if (ctx->recv_cb)
    {
        ret = ctx->recv_cb(chan_type, ctx->rx_msg_buf, ctx->rx_msg_len);
    }

    ntwk_json_reset_message(ctx);

    return ret;
}

static int ntwk_json_process_frame(chan_type_t chan_type, ntwk_json_chan_ctx_t *ctx)
{
    ntwk_json_head_t *head = (ntwk_json_head_t *)ctx->rx_pkt_buf;
    uint16_t msg_id = CHECK_ENDIAN_UINT16(head->msg_id);
    uint16_t frag_index = CHECK_ENDIAN_UINT16(head->frag_index);
    uint16_t frag_count = CHECK_ENDIAN_UINT16(head->frag_count);
    uint32_t total_len = CHECK_ENDIAN_UINT32(head->total_len);
    uint16_t payload_len = CHECK_ENDIAN_UINT16(head->payload_len);
    uint8_t *payload = head->payload;

    if (frag_index == 0)
    {
        ntwk_json_reset_message(ctx);
        ctx->rx_msg_buf = ntwk_malloc(total_len + 1);
        if (ctx->rx_msg_buf == NULL)
        {
            LOGE("%s, malloc rx json %u failed\n", __func__, total_len + 1);
            return -1;
        }

        ctx->rx_msg_size = total_len + 1;
        ctx->rx_msg_id = msg_id;
        ctx->rx_frag_count = frag_count;
        ctx->rx_next_frag = 0;
    }
    else if (ctx->rx_msg_buf == NULL ||
             ctx->rx_msg_id != msg_id ||
             ctx->rx_frag_count != frag_count ||
             ctx->rx_next_frag != frag_index)
    {
        LOGE("%s, json fragment order error id=%u idx=%u next=%u\n",
             __func__, msg_id, frag_index, ctx->rx_next_frag);
        ntwk_json_reset_message(ctx);
        return -1;
    }

    if (ctx->rx_msg_len + payload_len > ctx->rx_msg_size - 1)
    {
        LOGE("%s, json rx overflow len=%u payload=%u size=%u\n",
             __func__, ctx->rx_msg_len, payload_len, ctx->rx_msg_size);
        ntwk_json_reset_message(ctx);
        return -1;
    }

    os_memcpy(ctx->rx_msg_buf + ctx->rx_msg_len, payload, payload_len);
    ctx->rx_msg_len += payload_len;
    ctx->rx_next_frag++;

    if ((head->flags & NTWK_JSON_FLAG_EOF) || ctx->rx_next_frag == ctx->rx_frag_count)
    {
        if (ctx->rx_msg_len != total_len)
        {
            LOGE("%s, json length mismatch rx=%u total=%u\n",
                 __func__, ctx->rx_msg_len, total_len);
            ntwk_json_reset_message(ctx);
            return -1;
        }

        return ntwk_json_deliver_message(chan_type, ctx);
    }

    return 0;
}

static int ntwk_json_validate_header(ntwk_json_chan_ctx_t *ctx)
{
    ntwk_json_head_t *head = (ntwk_json_head_t *)ctx->rx_pkt_buf;
    uint16_t magic = CHECK_ENDIAN_UINT16(head->magic);
    uint16_t frag_index = CHECK_ENDIAN_UINT16(head->frag_index);
    uint16_t frag_count = CHECK_ENDIAN_UINT16(head->frag_count);
    uint32_t total_len = CHECK_ENDIAN_UINT32(head->total_len);
    uint16_t payload_len = CHECK_ENDIAN_UINT16(head->payload_len);

    if (magic != NTWK_JSON_MAGIC_CODE || head->version != NTWK_JSON_VERSION)
    {
        LOGE("%s, invalid json frame magic=0x%04x version=%u\n",
             __func__, magic, head->version);
        return -1;
    }

    if (frag_count == 0 || frag_index >= frag_count ||
        total_len == 0 || total_len > CONFIG_NTWK_CTRL_JSON_RX_MAX_SIZE ||
        payload_len == 0 || payload_len > ctx->payload_size)
    {
        LOGE("%s, invalid json frame idx=%u count=%u total=%u payload=%u\n",
             __func__, frag_index, frag_count, total_len, payload_len);
        return -1;
    }

    if ((head->flags & NTWK_JSON_FLAG_EOF) && frag_index != frag_count - 1)
    {
        LOGE("%s, eof flag on non-last fragment idx=%u count=%u\n",
             __func__, frag_index, frag_count);
        return -1;
    }

    if (sizeof(ntwk_json_head_t) + payload_len > ctx->packet_size)
    {
        LOGE("%s, json frame over packet size payload=%u packet=%u\n",
             __func__, payload_len, ctx->packet_size);
        return -1;
    }

    ctx->rx_pkt_expected = sizeof(ntwk_json_head_t) + payload_len;

    return 0;
}

static int ntwk_json_unfragment_by_type(chan_type_t chan_type, uint8_t *data, uint32_t length)
{
    ntwk_json_chan_ctx_t *ctx = ntwk_json_get_ctx(chan_type);
    uint8_t *p = data;
    uint32_t left = length;

    if (ctx == NULL || !ctx->initialized)
    {
        return -1;
    }

    if (data == NULL || length == 0)
    {
        return -1;
    }

    while (left > 0)
    {
        uint32_t need;
        uint32_t copy_len;

        if (ctx->rx_pkt_len < sizeof(ntwk_json_head_t))
        {
            need = sizeof(ntwk_json_head_t) - ctx->rx_pkt_len;
            copy_len = (left < need) ? left : need;
            os_memcpy(ctx->rx_pkt_buf + ctx->rx_pkt_len, p, copy_len);
            ctx->rx_pkt_len += copy_len;
            p += copy_len;
            left -= copy_len;

            if (ctx->rx_pkt_len < sizeof(ntwk_json_head_t))
            {
                continue;
            }

            if (ntwk_json_validate_header(ctx) != 0)
            {
                ntwk_json_clear_rx(chan_type);
                return -1;
            }
        }

        need = ctx->rx_pkt_expected - ctx->rx_pkt_len;
        copy_len = (left < need) ? left : need;
        if (copy_len > 0)
        {
            os_memcpy(ctx->rx_pkt_buf + ctx->rx_pkt_len, p, copy_len);
            ctx->rx_pkt_len += copy_len;
            p += copy_len;
            left -= copy_len;
        }

        if (ctx->rx_pkt_expected != 0 && ctx->rx_pkt_len == ctx->rx_pkt_expected)
        {
            int ret = ntwk_json_process_frame(chan_type, ctx);
            ntwk_json_reset_packet(ctx);
            if (ret < 0)
            {
                return ret;
            }
        }
    }

    return length;
}

int ntwk_json_ctrl_fragment(uint8_t *data, uint32_t length)
{
    return ntwk_json_fragment_by_type(NTWK_TRANS_CHAN_CTRL, data, length);
}

int ntwk_json_ctrl_unfragment(uint8_t *data, uint32_t length)
{
    return ntwk_json_unfragment_by_type(NTWK_TRANS_CHAN_CTRL, data, length);
}
