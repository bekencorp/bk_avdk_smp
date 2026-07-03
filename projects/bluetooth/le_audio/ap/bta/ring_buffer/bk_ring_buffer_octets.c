#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include "os/mem.h"
#include "components/log.h"
#include "driver/psram.h"

#include "bk_ring_buffer_octets.h"

#define USE_PSRAM 0

#define RB_MIN(x, y) (((x) > (y)) ? (y):(x))

#define rb_write_word(addr, val)                 do{*((volatile uint32_t *)(addr)) = val;} while(0)
#define rb_read_word(val, addr)                  do{(val) = *((volatile uint32_t *)(addr));} while(0)
#define rb_get_word(addr)                       *((volatile uint32_t *)(addr))

static inline uint32_t bk_ring_buffer_octets_ptr_plus(bk_ring_buffer_octets_context_t *ctx, uint32_t ptr, uint32_t len)
{
    return (ptr + len) % ctx->len;
}

int32_t bk_ring_buffer_octets_init(bk_ring_buffer_octets_context_t *ctx, uint8_t *buffer, uint32_t len)
{
    uint32_t final_len = len;

    if (!ctx || ctx->buffer)
    {
        BK_LOGE("rb", "%s param err\n", __func__);
        return -1;
    }

    ctx->buffer = buffer;
    ctx->len = final_len;
    ctx->rp = ctx->wp = 0;

    return 0;
}

int32_t bk_ring_buffer_octets_deinit(bk_ring_buffer_octets_context_t *ctx)
{
    os_memset(ctx, 0, sizeof(*ctx));
    return 0;
}

bool bk_ring_buffer_octets_is_init(bk_ring_buffer_octets_context_t *ctx)
{
    if(ctx->buffer && ctx->len)
    {
        return true;
    }

    return false;
}

int32_t bk_ring_buffer_octets_reset(bk_ring_buffer_octets_context_t *ctx)
{
    uint8_t *tmp_buff = ctx->buffer;

    os_memset(ctx, 0, sizeof(*ctx));
    ctx->buffer = tmp_buff;

    return 0;
}

int32_t bk_ring_buffer_octets_write(bk_ring_buffer_octets_context_t *ctx, uint8_t *data, uint32_t len)
{
    volatile uint32_t crp = ctx->rp, cwp = ctx->wp;
    uint32_t tmp_len = len;

    if (crp > cwp && cwp + tmp_len >= crp)
    {
        return -1;
    }

    if (crp <= cwp)
    {
        if ((cwp + tmp_len) / ctx->len >= 2 || ((cwp + tmp_len) / ctx->len == 1 && (cwp + tmp_len) % ctx->len >= crp))
        {
            return -1;
        }
    }

    if (crp > cwp)
    {
        os_memcpy(ctx->buffer + cwp, data, tmp_len);
        cwp += tmp_len;
    }
    else
    {
        if (tmp_len <= ctx->len - cwp)
        {
            os_memcpy(ctx->buffer + cwp, data, tmp_len);
            cwp = bk_ring_buffer_octets_ptr_plus(ctx, cwp, tmp_len);
        }
        else
        {
            os_memcpy(ctx->buffer + cwp, data, ctx->len - cwp);
            os_memcpy(ctx->buffer, data + (ctx->len - cwp), tmp_len - (ctx->len - cwp));
            cwp = bk_ring_buffer_octets_ptr_plus(ctx, cwp, tmp_len);
        }
    }

    ctx->wp = cwp;

    return 0;
}

int bk_ring_buffer_octets_read(bk_ring_buffer_octets_context_t *ctx, uint8_t *buff, uint32_t len)
{
    volatile uint32_t crp = ctx->rp, cwp = ctx->wp;
    int ret = 0;

    if (crp == cwp)
    {
        return ret;
    }

    if (crp < cwp)
    {
        uint32_t final_len = len < cwp - crp ? len : cwp - crp;
        os_memcpy(buff, ctx->buffer + crp, final_len);
        crp += final_len;
        ret = final_len;
    }
    else if (crp > cwp)
    {
        uint32_t first = 0, sec = 0;
        first += ctx->len - crp < len ? ctx->len - crp : len;
        sec += cwp - 0 < len - first ? cwp - 0 : len - first;
        os_memcpy(buff, ctx->buffer + crp, first);
        os_memcpy(buff + first, ctx->buffer, sec);

        ret = first + sec;
        crp = bk_ring_buffer_octets_ptr_plus(ctx, crp, first + sec);
    }

    ctx->rp = crp;

    return ret;
}

uint32_t bk_ring_buffer_octets_len(bk_ring_buffer_octets_context_t *ctx)
{
    volatile uint32_t crp = ctx->rp, cwp = ctx->wp;

    if (crp == cwp)
    {
        return 0;
    }

    if (crp < cwp)
    {
        return cwp - crp;
    }

    if (crp > cwp)
    {
        return ctx->len - (crp - cwp);
    }

    return 0;
}

void bk_ring_buffer_octets_debug(bk_ring_buffer_octets_context_t *ctx)
{
    BK_LOGI("rb", "%s %d %d %d %d\n", __func__, ctx->rp, ctx->wp, ctx->len, bk_ring_buffer_octets_len(ctx));
}
