#include <driver/dma.h>
#include <bk_general_dma.h>
#include <driver/audio_ring_buff.h>
#include <stdlib.h>
#include <string.h>
#include "cache.h"


//#define RWP_SAFE_INTERVAL   (4)
#define RWP_SAFE_INTERVAL   (0)

//#define DMA_WRITE_DEBUG       //DMA carry data from FIFO to memory

/*
 * PSRAM L2 cache coherency for DMA audio ring buffers.
 *
 * On BK7259 the AP-side audio DMA ring buffers may live in PSRAM heap/data
 * (MPU attr 5: L2 cacheable / L1 non-cacheable). The audio ADC/DAC DMA accesses
 * PSRAM physical memory directly and bypasses the AP L2 cache, so software must
 * maintain coherency on every frame copy:
 *   - RB_DMA_TYPE_WRITE (DMA produces, AP consumes): invalidate the region in
 *     PSRAM before AP reads it, otherwise AP may read stale L2 data.
 *   - RB_DMA_TYPE_READ  (AP produces, DMA consumes): clean (write back) the
 *     region after AP writes it, otherwise DMA may fetch stale PSRAM data.
 *
 * flush_dcache() does an L1+L2 clean & invalidate over the range (with a
 * trailing DSB/ISB) so it covers both directions, and it is a no-op on memory
 * that is not actually cached (e.g. SRAM-backed buffers), so it is safe to call
 * unconditionally on the DMA ring buffer region. Only DMA-backed ring buffers
 * (dma_id valid + matching dma_type) trigger maintenance; pure software ring
 * buffers are left untouched.
 */
static inline void rb_dcache_sync_before_read(const RingBufferContext *rb, uint32_t offset, uint32_t bytes)
{
    if (bytes && (rb->dma_id != DMA_ID_MAX) && (rb->dma_type == RB_DMA_TYPE_WRITE))
    {
        flush_dcache((void *)&rb->address[offset], (long)bytes);
    }
}

static inline void rb_dcache_sync_after_write(const RingBufferContext *rb, uint32_t offset, uint32_t bytes)
{
    if (bytes && (rb->dma_id != DMA_ID_MAX) && (rb->dma_type == RB_DMA_TYPE_READ))
    {
        __DSB();
        flush_dcache((void *)&rb->address[offset], (long)bytes);
    }
}

void ring_buffer_init(RingBufferContext* rb, uint8_t* addr, uint32_t capacity, dma_id_t dma_id, uint32_t dma_type)
{
    rb->address  = addr;
    rb->capacity = capacity;
    rb->wp       = 0;
    rb->rp       = 0;
    rb->dma_id      = dma_id;
    rb->dma_type = dma_type;

    if(dma_id != DMA_ID_MAX)
    {
        if(rb->dma_type == RB_DMA_TYPE_READ)
        {
            dma_set_src_pause_addr(dma_id, (uint32_t)addr);
        }
        else if(rb->dma_type == RB_DMA_TYPE_WRITE)
        {
            dma_set_dst_pause_addr(dma_id, (uint32_t)addr + capacity - 8);
        }
    }
}

void ring_buffer_clear(RingBufferContext* rb)
{
    rb->wp = 0;
    rb->rp = 0;

    if(rb->dma_id != DMA_ID_MAX)
    {
        if(rb->dma_type == RB_DMA_TYPE_READ)
        {
            //ring buffer数据都清空了，所以设置读暂停的地址为ring buffer起始地址,上来就暂停
            dma_set_src_pause_addr(rb->dma_id, (uint32_t)rb->address);
        }
        else if(rb->dma_type == RB_DMA_TYPE_WRITE)
        {
            //ring buffer数据都清空了，所以设置写暂停的地址为ring buffer结束地址,上来可以写满ring buffer
            /* Note: pause address is not same as loop end address */
            dma_set_dst_pause_addr(rb->dma_id, (uint32_t)rb->address + rb->capacity - 4);
        }
    }
}

uint32_t ring_buffer_read(RingBufferContext* rb, uint8_t* buffer, uint32_t size)
{
    uint32_t required_bytes = size;
    uint32_t read_bytes;
    uint32_t remain_bytes;
    uint32_t wp;

#ifdef DMA_WRITE_DEBUG
    static uint32_t wp_old = 0;
    static uint32_t rp_old = 0;
    static uint32_t wp_new = 0;
    static uint32_t rp_new = 0;
#endif

    if((rb->dma_id != DMA_ID_MAX) && (rb->dma_type == RB_DMA_TYPE_WRITE))
    {
        //读取DMA寄存器[dma1_dest_wr_addr],获取此时已经读到的地址
        wp = rb->wp = bk_dma_get_enable_status(rb->dma_id) ? dma_get_dest_write_addr(rb->dma_id) - (uint32_t)rb->address : 0;
    }
    else
    {
        wp = rb->wp;
    }

#ifdef DMA_WRITE_DEBUG
    if (rb->dma_type == RB_DMA_TYPE_WRITE)
    {
        wp_old = wp_new;
        rp_old = rp_new;
        wp_new = wp;
        rp_new = rb->rp;
    }
#endif

    if(required_bytes == 0) return 0;

    if(wp > rb->rp)
    {
        remain_bytes = wp - rb->rp;

        if(required_bytes > remain_bytes)
        {
            read_bytes = remain_bytes;
#ifdef DMA_WRITE_DEBUG
            BK_LOGD(NULL, "-----------[rb error 0]--------------\n");
            BK_LOGD(NULL, "wp_old: %d, rp_old: %d, wp_new: %d, rp_new: %d, address: 0x%x, capacity: %d\n", wp_old, rp_old, wp_new, rp_new, rb->address, rb->capacity);
#endif
            rb_dcache_sync_before_read(rb, rb->rp, read_bytes);
            memcpy(buffer, &rb->address[rb->rp], read_bytes);
            //rb->rp += read_bytes;
            rb->rp = (rb->rp + read_bytes) % rb->capacity;
        }
        else
        {
            read_bytes = required_bytes;
            rb_dcache_sync_before_read(rb, rb->rp, read_bytes);
            memcpy(buffer, &rb->address[rb->rp], read_bytes);
            //rb->rp += read_bytes;
            rb->rp = (rb->rp + read_bytes) % rb->capacity;
        }
    }
    else
    {
        if (wp == rb->rp && rb->dma_type != RB_DMA_TYPE_WRITE)
        {
            return 0;
        }

        remain_bytes = rb->capacity - rb->rp;

        if(required_bytes > remain_bytes)
        {
            read_bytes = remain_bytes;
            rb_dcache_sync_before_read(rb, rb->rp, read_bytes);
            memcpy(buffer, &rb->address[rb->rp], read_bytes);

            if(required_bytes - read_bytes > wp)
            {
#ifdef DMA_WRITE_DEBUG
                BK_LOGD(NULL, "-----------[rb error 1]--------------\n");
                BK_LOGD(NULL, "wp_old: %d, rp_old: %d, wp_new: %d, rp_new: %d, address: 0x%x, capacity: %d\n", wp_old, rp_old, wp_new, rp_new, rb->address, rb->capacity);
#endif
                rb_dcache_sync_before_read(rb, 0, wp);
                memcpy(buffer + read_bytes, &rb->address[0], wp);
                rb->rp = wp;
                read_bytes += wp;
            }
            else
            {
                rb_dcache_sync_before_read(rb, 0, required_bytes - read_bytes);
                memcpy(buffer + read_bytes, &rb->address[0], required_bytes - read_bytes);
                rb->rp = required_bytes - read_bytes;
                read_bytes = required_bytes;
            }
        }
        else
        {
            read_bytes = required_bytes;
            rb_dcache_sync_before_read(rb, rb->rp, read_bytes);
            memcpy(buffer, &rb->address[rb->rp], read_bytes);
            //rb->rp += read_bytes;
            rb->rp = (rb->rp + read_bytes) % rb->capacity;
        }
    }

    if((rb->dma_id != DMA_ID_MAX) && (rb->dma_type == RB_DMA_TYPE_WRITE))
    {

        uint32_t pause_ptr = 0;

        if (rb->rp == wp)
        {
            if (wp >= 4)
            {
                pause_ptr = wp - 4;
            }
            else
            {
                pause_ptr = rb->capacity + 4 - rb->rp;
            }
        }
        else
        {
            pause_ptr = rb->rp;
        }

        dma_set_dst_pause_addr(rb->dma_id, (uint32_t)&rb->address[pause_ptr]);

        //BK_LOGD(NULL, "dst_pause_addr: %d, rp: %d, wp: %d\n", pause_ptr, rb->rp, wp);
        //BK_LOGD(NULL, "wp_old: %d, rp_old: %d, wp_new: %d, rp_new: %d, address: 0x%x, capacity: %d\n", wp_old, rp_old, wp_new, rp_new, rb->address, rb->capacity);
    }

    return read_bytes;
}

uint32_t ring_buffer_write(RingBufferContext* rb, uint8_t* buffer, uint32_t size)
{
    uint32_t remain_bytes;
    uint32_t write_bytes = size;
    uint32_t rp;

    if(write_bytes == 0) return 0;

    if((rb->dma_id != DMA_ID_MAX) && (rb->dma_type == RB_DMA_TYPE_READ))
    {
        //读取DMA寄存器[dma0_src_rd_addr],获取此时已经读到的地址
        rp = rb->rp = bk_dma_get_enable_status(rb->dma_id) ? dma_get_src_read_addr(rb->dma_id) - (uint32_t)rb->address : 0;
    }
    else
    {
        rp = rb->rp;
    }

    if(rb->wp >= rp)
    {
        remain_bytes = rb->capacity - rb->wp + rp;

        if(remain_bytes >= write_bytes + RWP_SAFE_INTERVAL)
        {
            remain_bytes = rb->capacity - rb->wp;

            if(remain_bytes >= write_bytes)
            {
                memcpy(&rb->address[rb->wp], buffer, write_bytes);
                rb_dcache_sync_after_write(rb, rb->wp, write_bytes);
                //rb->wp += write_bytes;
                rb->wp = (rb->wp + write_bytes) % rb->capacity;
            }
            else
            {
                memcpy(&rb->address[rb->wp], buffer, remain_bytes);
                rb_dcache_sync_after_write(rb, rb->wp, remain_bytes);
                rb->wp = write_bytes - remain_bytes;
                memcpy(&rb->address[0], &buffer[remain_bytes], rb->wp);
                rb_dcache_sync_after_write(rb, 0, rb->wp);
            }
        }
        else
        {
            return 0;
        }
    }
    else
    {
        remain_bytes = rp - rb->wp;

        if(remain_bytes >= write_bytes + RWP_SAFE_INTERVAL)
        {
            memcpy(&rb->address[rb->wp], buffer, write_bytes);
            rb_dcache_sync_after_write(rb, rb->wp, write_bytes);
            //rb->wp += write_bytes;
            rb->wp = (rb->wp + write_bytes) % rb->capacity;
        }
        else
        {
            return 0;
        }
    }

    if(rb->wp >= rb->capacity && rb->rp)
    {
        rb->wp = 0;
    }

    if((rb->dma_id != DMA_ID_MAX) && (rb->dma_type == RB_DMA_TYPE_READ))
    {
        dma_set_src_pause_addr(rb->dma_id, (uint32_t)&rb->address[rb->wp]);
    }

    return write_bytes;
}

uint32_t ring_buffer_get_fill_size(RingBufferContext* rb)
{
    uint32_t rp, wp;
    uint32_t fill_size;
    bool pause_flag = false;

    if(rb->dma_id != DMA_ID_MAX)
    {
        if(rb->dma_type == RB_DMA_TYPE_READ)
        {
            rp = rb->rp = bk_dma_get_enable_status(rb->dma_id) ? dma_get_src_read_addr(rb->dma_id) - (uint32_t)rb->address : 0;
            wp = rb->wp;
        }
        else if(rb->dma_type == RB_DMA_TYPE_WRITE)
        {
            rp = rb->rp;
            wp = rb->wp = bk_dma_get_enable_status(rb->dma_id) ? dma_get_dest_write_addr(rb->dma_id) - (uint32_t)rb->address : 0;
        }
        else
        {
            rp = rb->rp;
            wp = rb->wp;
        }
    }
    else
    {
        rp = rb->rp;
        wp = rb->wp;
    }

    if ((rb->dma_type == RB_DMA_TYPE_WRITE && bk_dma_get_repeat_wr_pause(rb->dma_id))
        || (rb->dma_type == RB_DMA_TYPE_READ && bk_dma_get_repeat_rd_pause(rb->dma_id)))
    {
        pause_flag = true;
    }

    fill_size = wp >= rp ? wp - rp : rb->capacity - rp + wp;

    if (pause_flag && fill_size == 0 && rb->dma_type == RB_DMA_TYPE_WRITE)
    {
        fill_size = rb->capacity - fill_size;
    }

    return fill_size;
}

uint32_t ring_buffer_get_free_size(RingBufferContext* rb)
{
    uint32_t free_size;

    free_size = rb->capacity - ring_buffer_get_fill_size(rb);

    return free_size > RWP_SAFE_INTERVAL ? free_size - RWP_SAFE_INTERVAL : 0;
}

