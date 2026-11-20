/**
 * @if copyright_display
 *      Copyright (C), 2018-2021, Arm Technology (China) Co., Ltd.
 *      All rights reserved
 *
 *      The content of this file or document is CONFIDENTIAL and PROPRIETARY
 *      to Arm Technology (China) Co., Ltd. It is subject to the terms of a
 *      License Agreement between Licensee and Arm Technology (China) Co., Ltd
 *      restricting among other things, the use, reproduction, distribution
 *      and transfer.  Each of the embodiments, including this information and
 *      any derivative work shall retain this copyright notice.
 * @endif
 */

#include "dubhe_regs.h"
#include "dubhe_sca.h"
#include "dubhe_event.h"
#include "dubhe_driver.h"
#include "pal_time.h"
#include "pal_log.h"
#include "pal_string.h"
#include "dubhe_intr_handler.h"
#include "reg_base.h"
/* bk_dcache_addr_is_cacheable(): tells whether the CPU caches a buffer. */
#include "cache.h"
/* os_sram_malloc()/psram_malloc()/os_free(): DMA-visible staging arenas. */
#include <os/mem.h>

/* The engine fetches keys/IVs/input with its own DMA as soon as the command
 * queue word holding the address is written. The CPU stores that filled the
 * staging buffer may still be in the write buffer at that moment, so the DMA
 * could read a partly updated copy. This was seen as a GHASH seed whose first
 * 32-bit word was still the previous value, which made GCM decryption fail
 * intermittently. Drain the stores before handing the address to the engine. */
#define DUBHE_SCA_DMA_BARRIER( ) __asm volatile ( "dsb 0xf" ::: "memory" )

#define PROC_TIME_FOR_ONE_BLOCK  ( 10 )

#define DBH_SCA_DMA_ADDR(p) ((uint32_t)SOC_SRAM_PERI_ADDR((uintptr_t)(p)))

/*
 * The TE200 DMA master can reach SRAM (CPU alias 0x2Cxxxxxx, peripheral alias
 * 0x28/0x38xxxxxx), non-cacheable PSRAM and flash, but it does not snoop the
 * CPU caches: a buffer the CPU caches (AP_PSRAM_CACHE_HEAP, PSRAM code
 * section) is stale from the engine's point of view, and a buffer the engine
 * writes is stale from the CPU's point of view.
 *
 * libtuyaos.a allocates its mbedtls contexts and I/O buffers in PSRAM, so the
 * key/iv fields of the context, the small scratch blocks and any cacheable
 * caller buffer are staged. The static staging buffers below are .bss
 * (-> SRAM) and are only used while DBH_SCA_MUTEX is held; the large
 * input/output buffers use the SRAM heap for short records and the
 * non-cacheable PSRAM heap for long ones, so a single TLS record cannot
 * drain the SRAM heap.
 */
static uint8_t s_sca_ek1_stage[32] __attribute__( ( aligned( 16 ) ) );
static uint8_t s_sca_ek2_stage[16] __attribute__( ( aligned( 16 ) ) );
static uint8_t s_sca_ek3_stage[16] __attribute__( ( aligned( 16 ) ) );
/* The GHASH seed and the CTR counter use separate slots: the engine may still
 * be fetching one of them while the next SCA_init stages the other, and with a
 * single shared slot the two values could be mixed. */
static uint8_t s_sca_iv_stage[16] __attribute__( ( aligned( 16 ) ) );
static uint8_t s_sca_ctr_iv_stage[16] __attribute__( ( aligned( 16 ) ) );
static uint8_t s_sca_extra_stage[16] __attribute__( ( aligned( 16 ) ) );
static uint8_t s_sca_temp_stage[16] __attribute__( ( aligned( 16 ) ) );
static uint8_t s_sca_mac_stage[16] __attribute__( ( aligned( 16 ) ) );

static int dubhe_sca_dma_is_sram( const void *p )
{
    switch ( ( (uint32_t)(uintptr_t)p ) >> 24 ) {
    case 0x28:      /* SRAM, peripheral alias */
    case 0x2C:      /* SRAM, CPU direct alias (0x28 | Bit26) */
    case 0x38:      /* SRAM, non-secure peripheral alias */
    case 0x3C:      /* SRAM, non-secure CPU direct alias */
        return 1;
    default:
        return 0;
    }
}

/*
 * The engine's own DMA does not snoop the CPU caches, so a buffer can be
 * handed over as-is as long as the CPU never caches it: SRAM, non-cacheable
 * PSRAM (AP_PSRAM_HEAP / AP_PSRAM_DATA_SECTION) and flash (read-only input).
 * Cacheable PSRAM (AP_PSRAM_CACHE_HEAP, AP_PSRAM_CODE_SECTION) still has to
 * be staged into a DMA-visible arena. Address alignment is not checked: the
 * engine was validated with 4- and 8-byte aligned buffers.
 *
 * Only the (potentially large) input/output of the crypt operation uses this
 * check; the small key/iv/extra fields keep the SRAM staging.
 */
static int dubhe_sca_dma_can_raw( const void *p, size_t len )
{
    return !bk_dcache_addr_is_cacheable( p, len );
}

/* Records up to this size are staged on the (fast) SRAM heap; longer ones go
 * to the non-cacheable PSRAM heap so that one crypt call cannot drain the
 * SRAM heap. Both arenas are DMA-visible to the engine. */
#define DUBHE_SCA_SRAM_STAGE_MAX ( 4 * 1024 )

#if !DUBHE_USE_LEGACY_STATIC_STAGE
static unsigned char *dubhe_sca_stage_alloc_buf( size_t len )
{
    unsigned char *p = NULL;

    if ( len <= DUBHE_SCA_SRAM_STAGE_MAX )
        p = os_sram_malloc( len );

#ifdef CONFIG_AP_PSRAM_HEAP_ADDR
    if ( p == NULL ) {
        unsigned char *ps = psram_malloc( len );

        /* Only hand it to the engine when the heap really is non-cacheable. */
        if ( ps != NULL ) {
            if ( !bk_dcache_addr_is_cacheable( ps, len ) )
                p = ps;
            else
                os_free( ps );
        }
    }
#endif

    /* Last resort: the whole record on the SRAM heap (may be tight). */
    if ( ( p == NULL ) && ( len > DUBHE_SCA_SRAM_STAGE_MAX ) )
        p = os_sram_malloc( len );

    return p;
}
#endif /* !DUBHE_USE_LEGACY_STATIC_STAGE */

/* Return an engine-visible address for p, copying it into stage when p does
 * not live in SRAM. The engine only reads these parameters. */
static uint32_t dubhe_sca_stage_dma_addr( const void *p, void *stage, size_t len )
{
    if ( dubhe_sca_dma_is_sram( p ) )
        return DBH_SCA_DMA_ADDR( p );

    pal_memcpy( stage, p, len );
    DUBHE_SCA_DMA_BARRIER( );
    return DBH_SCA_DMA_ADDR( stage );
}

/* Staging copies need no alignment adjustment: the heap returns 8-byte
 * aligned blocks and the engine was validated with 4- and 8-byte aligned
 * buffers (mbedtls_align: 250/255/256/257 bytes, no write past the length). */

#if DUBHE_USE_LEGACY_STATIC_STAGE
/* Previous scheme: records up to 512 B were staged in these two private SRAM
 * buffers, longer ones fell back to the SRAM heap. Kept as the reference for
 * a future dynamic + static hybrid (static buffers for short records, heap
 * for the rest). */
#define DUBHE_SCA_STATIC_STAGE_MAX 512
#define DUBHE_SCA_STAGE_ALIGN      16

static unsigned char s_sca_in_stage[DUBHE_SCA_STATIC_STAGE_MAX + DUBHE_SCA_STAGE_ALIGN]
    __attribute__( ( aligned( DUBHE_SCA_STAGE_ALIGN ) ) );
static unsigned char s_sca_out_stage[DUBHE_SCA_STATIC_STAGE_MAX + DUBHE_SCA_STAGE_ALIGN]
    __attribute__( ( aligned( DUBHE_SCA_STAGE_ALIGN ) ) );

#define DUBHE_SCA_IN_STAGE  s_sca_in_stage
#define DUBHE_SCA_OUT_STAGE s_sca_out_stage
#else
#define DUBHE_SCA_IN_STAGE  NULL
#define DUBHE_SCA_OUT_STAGE NULL
#endif

/* When sram_buf is not NULL and the record is short enough, the private static
 * buffer is used and *base stays NULL (nothing to free). Otherwise a
 * DMA-visible staging buffer is allocated from the SRAM heap or, for long
 * records, from the non-cacheable PSRAM heap; *base owns the allocation. */
static unsigned char *dubhe_sca_stage_alloc( size_t len,
                                             const void *src,
                                             void **base,
                                             unsigned char *sram_buf )
{
    unsigned char *p;

#if DUBHE_USE_LEGACY_STATIC_STAGE
    if ( ( sram_buf != NULL ) && ( len <= DUBHE_SCA_STATIC_STAGE_MAX ) ) {
        p = sram_buf;
    } else {
        p = os_sram_malloc( len + DUBHE_SCA_STAGE_ALIGN );
        if ( p == NULL )
            return NULL;

        *base = p;
        p     = (unsigned char *)( ( (uintptr_t)p + DUBHE_SCA_STAGE_ALIGN - 1 )
                                   & ~(uintptr_t)( DUBHE_SCA_STAGE_ALIGN - 1 ) );
    }
#else
    (void)sram_buf;
    p = dubhe_sca_stage_alloc_buf( len );
    if ( p == NULL )
        return NULL;

    *base = p;
#endif
    if ( src != NULL ) {
        pal_memcpy( p, src, len );
        DUBHE_SCA_DMA_BARRIER( );
    }

    return p;
}

#define BIT_MASK( V, M ) ( ( V ) & ( M ) )
#define RETRY_COUNT 2

#define CHECK_RET( )                                                           \
    do {                                                                       \
        switch ( ret ) {                                                       \
        case 0:                                                                \
            break;                                                             \
        case DBH_SCA_BUS_ERR:                                                  \
            arm_ce_sca_driver_init( );                                         \
        default:                                                               \
            return ret;                                                        \
            break;                                                             \
        }                                                                      \
    } while ( 0 )

#define CHECK_RETRY( pctx, release_lock )                                      \
    do {                                                                       \
        if ( ret == DBH_SCA_BUS_ERR ) {                                        \
            PAL_LOG_ERR( "BUS ERROR retry? times(%d)\n", pctx->retry_count );  \
            if ( pctx->retry_count++ < RETRY_COUNT ) {                         \
                goto RETRY;                                                    \
            }                                                                  \
        } else if ( ret != 0 ) {                                               \
            if ( release_lock ) {                                              \
                goto clean_up;                                                 \
            } else {                                                           \
                goto exit;                                                     \
            }                                                                  \
            return ret;                                                        \
        }                                                                      \
    } while ( 0 )

static void dubhe_zeroize( void *v, size_t n )
{
    volatile unsigned char *p = (unsigned char *) v;
    while ( n-- )
        *p++ = 0;
}
static arm_ce_sca_context_t *_g_ce_sca_ctx = NULL;

static int arm_ce_sca_execute_sca_finish( arm_ce_sca_context_t *ctx,
                                          unsigned char *mac );

#if defined( DUBHE_FOR_RUNTIME )
static void arm_ce_sca_process_mask_intr( uint32_t mask )
{
    volatile uint32_t value = 0;

    /* Watermark interrupt should be masked */
    value = DBH_READ_REGISTER( SCA, SCA_INTR_MASK );
    DBH_REG_FLD_SET( SCA_INTR_MASK, CMD_INTR, value, (mask & 0x01) );
    DBH_REG_FLD_SET( SCA_INTR_MASK, INVALID_CMD, value, (mask & 0x01) );
    DBH_REG_FLD_SET( SCA_INTR_MASK, INVALID_KEY, value, (mask & 0x01) );
    DBH_REG_FLD_SET( SCA_INTR_MASK, BUS_ERR, value, (mask & 0x01) );
    DBH_REG_FLD_SET( SCA_INTR_MASK, WM_INTR, value, (mask & 0x01) );
    DBH_WRITE_REGISTER( SCA, SCA_INTR_MASK, value );
}

#define ARM_CE_SCA_DISABLE_INTERRUPT() do{                                      \
            arm_ce_sca_process_mask_intr( 0x01 );                               \
        }while(0)

#define ARM_CE_SCA_ENABLE_INTERRUPT() do{                                       \
            arm_ce_sca_process_mask_intr( 0x00 );                               \
        }while(0)
#endif

void arm_ce_sca_driver_init( void )
{
    volatile uint32_t value = 0;

#if defined( DUBHE_SECURE )
    dubhe_clk_enable( DBH_MODULE_SCA );

    /* Reset SCA module */
    value = DBH_READ_REGISTER( TOP_CTRL, RESET_CTRL );
    DBH_REG_FLD_SET( RESET_CTRL, SCA, value, 0x1 );
    DBH_WRITE_REGISTER( TOP_CTRL, RESET_CTRL, value );
    DBH_REG_FLD_SET( RESET_CTRL, SCA, value, 0x0 );
    DBH_WRITE_REGISTER( TOP_CTRL, RESET_CTRL, value );
#endif

#if defined( DUBHE_FOR_RUNTIME )
    /* Set command queue watermark, set 0, there's no watermark interrupt
     * generated */
    value = ( 0x0 << 1 );
    /* Unmask interrupts */
    ARM_CE_SCA_DISABLE_INTERRUPT( );
#endif

    /* Run */
    value |= 0x1;
    DBH_WRITE_REGISTER( SCA, SCA_CTRL, value );
}

int arm_ce_sca_init( arm_ce_sca_context_t *ctx )
{
    if ( ctx != NULL ) {
        dubhe_zeroize( ctx, sizeof( arm_ce_sca_context_t ) );
        ctx->magic = ARM_CE_SCA_MAGIC;
        ctx->mode = ARM_CE_SCA_OPERATION_NONE;
        ctx->ctx_init_flag = ARM_CE_SCA_CTX_NEED_INIT;
        return ( 0 );
    } else {
        return ( DBH_SCA_PARAM_INVAL_ERR );
    }
}

static int _validate_key_len( void *ctx, uint32_t key_len, uint8_t derived )
{
    int ret                    = 0;
    arm_ce_sca_context_t *pctx = (arm_ce_sca_context_t *) ctx;
    switch ( pctx->alg_type ) {
    case ARM_CE_AES:
        if ( derived ) {
            if ( key_len != ARM_CE_SCA_KEY_BITS_128
                 && key_len != ARM_CE_SCA_KEY_BITS_256 ) {
                ret = DBH_SCA_KEYBITS_INVAL_ERR;
            }
        } else {
            if ( key_len != ARM_CE_SCA_KEY_BITS_128
                 && key_len != ARM_CE_SCA_KEY_BITS_192
                 && key_len != ARM_CE_SCA_KEY_BITS_256 ) {
                ret = DBH_SCA_KEYBITS_INVAL_ERR;
            }
        }
        break;
    case ARM_CE_SM4:
    case ARM_CE_GHASH:
        if ( key_len != ARM_CE_SCA_KEY_BITS_128 ) {
            ret = DBH_SCA_KEYBITS_INVAL_ERR;
        }
        break;
    default:
        ret = DBH_SCA_PARAM_INVAL_ERR;
        break;
    }

    return ret;
}

int arm_ce_sca_set_key( arm_ce_sca_context_t *ctx,
                        arm_ce_sca_key_type_t type,
                        const unsigned char *key1,
                        const unsigned char *key2,
                        const unsigned char *key3,
                        unsigned int key1bits,
                        uint8_t derived )
{
    uint8_t key_bytes = 0;
    int ret           = 0;

    if ( ( ctx == NULL ) || ( ctx->magic != ARM_CE_SCA_MAGIC ) ) {
        PAL_LOG_ERR( "%s+%d crypto invalid parameter\n",__FILE__, __LINE__ );
        return ( DBH_SCA_PARAM_INVAL_ERR );
    }
    if ( ( derived == 1 ) && ( ( key2 == NULL ) || ( key3 == NULL ) ) )
        return ( DBH_SCA_PARAM_INVAL_ERR );

    ret           = _validate_key_len( ctx, key1bits, derived );

    if ( 0 != ret ) {
        PAL_LOG_ERR( "_validate_key_len(%d) failed(-%X)\n", key1bits, -ret );
        return ret;
    }

     /* same key return directly */
    if ( ( type == ctx->key_type ) && ( key1bits == ctx->keybits )
         && ( pal_memcmp( key1, ctx->ek1, ctx->keybits /8  ) == 0)
         && ( !derived || ( ( pal_memcmp(key2, ctx->ek2, 16 ) == 0 )
         && ( pal_memcmp(key3, ctx->ek3, 16 ) == 0 ) ) ) ) {
        return 0;
    }

    ctx->keybits  = key1bits;
    ctx->key_type = type;
#if defined( DUBHE_FOR_RUNTIME )
    /* Get command queue lock */
    dubhe_mutex_lock( DBH_SCA_MUTEX );
#endif
    if ( _g_ce_sca_ctx != NULL ) {
    RETRY:
        if ( _g_ce_sca_ctx->ctx_finish_flag == ARM_CE_SCA_CTX_NEED_FINISH ) {
            ret = arm_ce_sca_execute_sca_finish(
                _g_ce_sca_ctx, _g_ce_sca_ctx->inter_value.last_blk );
            CHECK_RETRY( _g_ce_sca_ctx, 1 );
            _g_ce_sca_ctx->retry_count = 0;
        }
        _g_ce_sca_ctx              = NULL;
    }

    _g_ce_sca_ctx = ctx;

    switch ( key1bits ) {
    case ARM_CE_SCA_KEY_BITS_128:
        key_bytes = ARM_CE_SCA_KEY_BITS_128 / 8;
        pal_memcpy( ctx->ek1, key1, key_bytes );
        if ( derived == 1 ) {
            pal_memcpy( ctx->ek2, key2, key_bytes );
            pal_memcpy( ctx->ek3, key3, key_bytes );
        }
        break;
    case ARM_CE_SCA_KEY_BITS_192:
        pal_memcpy( ctx->ek1, key1, ARM_CE_SCA_KEY_BITS_192 / 8 );
        break;
    case ARM_CE_SCA_KEY_BITS_256:
        key_bytes = ARM_CE_SCA_KEY_BITS_256 / 8;
        pal_memcpy( ctx->ek1, key1, key_bytes );
        key_bytes = ARM_CE_SCA_KEY_BITS_128 / 8;
        if ( derived == 1 ) {
            pal_memcpy( ctx->ek2, key2, key_bytes );
            pal_memcpy( ctx->ek3, key3, key_bytes );
        }
        break;
    default:
        ret = ( DBH_SCA_KEYBITS_INVAL_ERR );
        goto clean_up;
    }

clean_up:
#if defined( DUBHE_FOR_RUNTIME )
    dubhe_mutex_unlock( DBH_SCA_MUTEX );
#endif
exit:
    return ( ret );
}

static int _arm_ce_sca_intr_handler( void )
{
    uint32_t value = 0;
    int ret        = 0;

    value = DBH_READ_REGISTER( SCA, SCA_INTR_STAT );

    if ( BIT_MASK( value, DBH_SCA_CMD_INTR ) ) {
        ret = 0;
    }

    if ( BIT_MASK( value, DBH_SCA_BUS_ERR_INTR ) ) {
        ret = DBH_SCA_BUS_ERR;
        PAL_LOG_ERR( "value(%X) SCA process error \
                    unexpected interrupt DBH_SCA_BUS_ERR_INTR\n",
                     value );
        goto exit;
    }

    if ( BIT_MASK( value, DBH_SCA_INVAL_CMD_INTR ) ) {
        ret = DBH_SCA_INVALID_CMD_ERR;
        PAL_LOG_ERR( "value(%X) SCA process error \
                    unexpected interrupt DBH_SCA_INVAL_CMD_INTR\n",
                     value );
        goto exit;
    }

    if ( BIT_MASK( value, DBH_SCA_INVAL_KEY_ERR_INTR ) ) {
        ret = DBH_SCA_KEYBITS_INVAL_ERR;
        PAL_LOG_ERR( "value(%X) SCA process error \
                    unexpected interrupt DBH_SCA_INVAL_KEY_ERR_INTR\n",
                     value );
        goto exit;
    }

    if ( BIT_MASK( value, DBH_SCA_WM_INTR ) ) {
        PAL_LOG_ERR( "value(%X) SCA process error \
                    unexpected interrupt DBH_SCA_WM_INTR\n",
                     value );
        goto exit;
    }

exit:
    /*write clear the interrupt*/
    DBH_WRITE_REGISTER( SCA, SCA_INTR_STAT, value );

    return ( ret );
}

static int arm_ce_sca_read_sca_stat( void )
{
    volatile int stat;
    volatile uint8_t avail_slots = 0, engine_stat = 0;

    stat        = DBH_READ_REGISTER( SCA, SCA_STAT );
    avail_slots = ( stat >> DBH_SCA_QUEUE_AVAIL_SLOTS_BIT_SHIFT ) & 0xFF;
    engine_stat = stat & 0x3;

    if ( ( avail_slots == DBH_SCA_QUEUE_DEPTH )
         && ( engine_stat <= ARM_CE_SCA_ENG_WAIT_FOR_CMD ) ) {
        return ( DBH_SCA_ENGINE_IDLE );
    } else {
        return ( DBH_SCA_ENGINE_BUSY );
    }
}

static int arm_ce_sca_wait_engine_idle( void )
{
#if defined( DBH_SCA_DEBUG )
    PAL_LOG_ERR( "### POLLING SCA STAT_REG addr: 0x%08lx\n",
                 DBH_BASE_SCA + DBH_SCA_STAT_REG_OFFSET );
#endif
    int value = 0;
    int ret   = 0;

    for ( ;; ) {
        ret = _arm_ce_sca_intr_handler( );

        if ( ret != 0 ) {
            return ret;
        }

        value = arm_ce_sca_read_sca_stat( );

        if ( value != DBH_SCA_ENGINE_IDLE ) {
#if defined( DUBHE_FOR_RUNTIME )
            pal_udelay( PROC_TIME_FOR_ONE_BLOCK );
#endif
            continue;
        } else {
            break;
        }
    }

#if defined( DBH_SCA_DEBUG )
    PAL_LOG_ERR( "### POLLING SCA STAT_REG addr: 0x%08lx, val: 0x%08x\n",
                 DBH_BASE_SCA + DBH_SCA_STAT_REG_OFFSET,
                 value );

#endif
    return ret;
}

static void arm_ce_sca_update_ctr_iv( arm_ce_sca_context_t *ctx,
                                      unsigned char *iv,
                                      size_t length )
{
    uint32_t blk_num = 0, i = 0;

    if ( length % ARM_CE_SCA_BLK_SIZE == 0 ) {
        blk_num = length / ARM_CE_SCA_BLK_SIZE;
    } else {
        blk_num = length / ARM_CE_SCA_BLK_SIZE + 1;
    }

    while ( blk_num-- ) {
        for ( i = ARM_CE_SCA_IV_BYTES; i > 0; i-- ) {
            if ( ++iv[i - 1] != 0 ) {
                break;
            }
        }
    }
    /* Save IV */
    pal_memcpy( ctx->inter_value.ctr_value.cur_iv, iv, ARM_CE_SCA_IV_BYTES );
}

static int arm_ce_sca_get_info( arm_ce_sca_type_t sca_type,
                                uint8_t *load_iv,
                                uint8_t *sm4,
                                arm_ce_sca_mac_type_t mac_type,
                                int *aes_mode )
{
    switch ( sca_type ) {
    case ARM_CE_AES_ECB:
        *load_iv  = 0;
        *sm4      = 0;
        *aes_mode = DBH_SCA_MODE_ECB;
        break;
    case ARM_CE_AES_CBC:
        *load_iv  = 1;
        *sm4      = 0;
        *aes_mode = DBH_SCA_MODE_CBC;
        break;
    case ARM_CE_AES_CTR:
        *load_iv  = 1;
        *sm4      = 0;
        *aes_mode = DBH_SCA_MODE_CTR;
        break;
    case ARM_CE_SM4_ECB:
        *load_iv  = 0;
        *sm4      = 1;
        *aes_mode = DBH_SCA_MODE_ECB;
        break;
    case ARM_CE_SM4_CBC:
        *load_iv  = 1;
        *sm4      = 1;
        *aes_mode = DBH_SCA_MODE_CBC;
        break;
    case ARM_CE_SM4_CTR:
        *load_iv  = 1;
        *sm4      = 1;
        *aes_mode = DBH_SCA_MODE_CTR;
        break;
    case ARM_CE_AES_CBCMAC:
        *load_iv  = 1;
        *sm4      = 0;
        *aes_mode = DBH_SCA_MODE_CBC_MAC;
        break;
    case ARM_CE_AES_CMAC:
        *load_iv  = 1;
        *sm4      = 0;
        *aes_mode = DBH_SCA_MODE_CMAC;
        break;
    case ARM_CE_SM4_CBCMAC:
        *load_iv  = 1;
        *sm4      = 1;
        *aes_mode = DBH_SCA_MODE_CBC_MAC;
        break;
    case ARM_CE_SM4_CMAC:
        *load_iv  = 1;
        *sm4      = 1;
        *aes_mode = DBH_SCA_MODE_CMAC;
        break;
    case ARM_CE_GHASH_MAC:
        *load_iv  = 1;
        *sm4      = 0;
        *aes_mode = DBH_SCA_MODE_GHASH;
        break;
    default:
        return ( DBH_SCA_MODE_ERR );
    }

    return ( 0 );
}

/* Notes: ccm type should be noticed */
static int arm_ce_sca_execute_sca_init( arm_ce_sca_context_t *ctx,
                                        arm_ce_sca_type_t sca_type,
                                        arm_ce_sca_mac_type_t mac_type,
                                        size_t *nc_off,
                                        unsigned char *iv,
                                        unsigned char *stream_block )
{
    int ret = 0, aes_mode = 0;
    uint32_t value = 0, keybits;
    uint8_t opcode, sm4 = 0, load_iv = 0;
    arm_ce_sca_key_type_t key_type;
    opcode = DBH_SCA_INIT_OPCODE;
    value |= ( opcode << DBH_SCA_OPCODE_BIT_SHIFT );
    /* Key configuration */
    ret = arm_ce_sca_get_info( sca_type, &load_iv, &sm4, mac_type, &aes_mode );
    if ( ret < 0 ) {
        PAL_LOG_ERR( "ret:%d   %d\n", ret, __LINE__ );
        return ( ret );
    }
    /* Key source */
    key_type = ctx->key_type;
    if ( ( key_type > ARM_CE_SCA_EXTERNAL_KEY )
         || ( key_type < ARM_CE_SCA_MODEL_KEY ) ) {
        return ( DBH_SCA_KEY_TYPE_INVAL_ERR );
    }
    value |= ( key_type << DBH_SCA_KEY_SRC_BIT_SHIFT );

    /* Key length */
    keybits = ctx->keybits;
    if ( keybits == ARM_CE_SCA_KEY_BITS_128 )
        value |= ( ARM_CE_SCA_KEY_BITS_TYPE_128 << DBH_SCA_KEYBITS_BIT_SHIFT );
    else if ( keybits == ARM_CE_SCA_KEY_BITS_192 )
        value |= ( ARM_CE_SCA_KEY_BITS_TYPE_192 << DBH_SCA_KEYBITS_BIT_SHIFT );
    else if ( keybits == ARM_CE_SCA_KEY_BITS_256 )
        value |= ( ARM_CE_SCA_KEY_BITS_TYPE_256 << DBH_SCA_KEYBITS_BIT_SHIFT );
    else {
        PAL_LOG_ERR(
            "keybits:%d DBH_SCA_KEYBITS_INVAL_ERR   %d\n", keybits, __LINE__ );
        return ( DBH_SCA_KEYBITS_INVAL_ERR );
    }
    /* Algorithm standard, normal AES or SM4 */
    value |= sm4 << DBH_SCA_ALOG_STD_BIT_SHIFT;
    /* Key parameter format, is address*/
    value |= 0x1 << DBH_SCA_KEY_FORMAT_BIT_SHIFT;

    /* IV configuration */
    if ( load_iv == 1 ) {
        /* Load IV */
        value |= ( 0x1 << DBH_SCA_LOAD_IV_BIT_SHIFT );
        /* IV parameter is address */
        value |= ( 0x1 << DBH_SCA_IV_FORMAT_BIT_SHIFT );
    } else
        value &= 0xFFFF7FFF;

    /* Mode configuration */
    value |= aes_mode << 4;

    /* SCA_init will not trigger an interrupt */
    value &= 0xFFFFFFFE;
    /* Drain the CPU stores before the engine fetches the command and its
     * parameters: the key/IV may live in SRAM (stack context, the swap_ctx
     * copy, an IV filled right before this call) and would otherwise be
     * handed over as-is with the stores still in the write buffer. The
     * per-parameter staging path issues its own barrier when it copies. */
    DUBHE_SCA_DMA_BARRIER( );
    /* Write the SCA_init command function part */
    DBH_WRITE_REGISTER( SCA, SCA_QUEUE, value );

    /* Write parameter part */
    if ( key_type == ARM_CE_SCA_EXTERNAL_KEY )
        DBH_WRITE_REGISTER(
            SCA,
            SCA_QUEUE,
            dubhe_sca_stage_dma_addr(
                ctx->ek1, s_sca_ek1_stage, sizeof( ctx->ek1 ) ) );
    else {
        DBH_WRITE_REGISTER(
            SCA,
            SCA_QUEUE,
            dubhe_sca_stage_dma_addr(
                ctx->ek3, s_sca_ek3_stage, sizeof( ctx->ek3 ) ) );
        DBH_WRITE_REGISTER(
            SCA,
            SCA_QUEUE,
            dubhe_sca_stage_dma_addr(
                ctx->ek2, s_sca_ek2_stage, sizeof( ctx->ek2 ) ) );
        DBH_WRITE_REGISTER(
            SCA,
            SCA_QUEUE,
            dubhe_sca_stage_dma_addr(
                ctx->ek1, s_sca_ek1_stage, sizeof( ctx->ek1 ) ) );
    }
    if ( load_iv ) {
        void *iv_stage = ( aes_mode == DBH_SCA_MODE_CTR ) ? s_sca_ctr_iv_stage
                                                         : s_sca_iv_stage;

        DBH_WRITE_REGISTER(
            SCA,
            SCA_QUEUE,
            dubhe_sca_stage_dma_addr( iv, iv_stage, ARM_CE_SCA_IV_BYTES ) );

        /* For CTR calculate the iv, and save it */
        if ( aes_mode == DBH_SCA_MODE_CTR ) {
            ctx->inter_value.ctr_value.nc_off = *nc_off;
            pal_memcpy( ctx->inter_value.ctr_value.cur_iv,
                        iv,
                        ARM_CE_SCA_IV_BYTES );
            pal_memcpy( ctx->inter_value.ctr_value.cur_iv_ct,
                        stream_block,
                        ARM_CE_SCA_IV_BYTES );
        }
    }

    ret                  = arm_ce_sca_wait_engine_idle( );
    /* The engine has consumed (or aborted) the staged key/IV copies: wipe
     * them on success and on BUS_ERR alike so the static staging buffers do
     * not keep key material around. */
    dubhe_zeroize( s_sca_ek1_stage, sizeof( s_sca_ek1_stage ) );
    dubhe_zeroize( s_sca_ek2_stage, sizeof( s_sca_ek2_stage ) );
    dubhe_zeroize( s_sca_ek3_stage, sizeof( s_sca_ek3_stage ) );
    dubhe_zeroize( s_sca_iv_stage, sizeof( s_sca_iv_stage ) );
    dubhe_zeroize( s_sca_ctr_iv_stage, sizeof( s_sca_ctr_iv_stage ) );
    CHECK_RET( );
    ctx->ctx_init_flag   = ARM_CE_SCA_CTX_INITIALIZED;
    ctx->ctx_finish_flag = ARM_CE_SCA_CTX_NEED_FINISH;
    return ( ret );
}

static int arm_ce_sca_handle_input_data( arm_ce_sca_context_t *ctx,
                                         size_t ilen,
                                         unsigned char *iv,
                                         const unsigned char *input,
                                         unsigned char *output,
                                         unsigned char extra[16],
                                         arm_ce_sca_crypt_info_t *crypt_info )
{
    int c = 0, input_left_size = 0;
    const unsigned char *temp  = input;
    unsigned char *out         = output;
    arm_ce_sca_type_t sca_type = ctx->sca_type;
    size_t temp_ilen           = ilen;
    size_t nc_off_bytes = 0, nc_off = ctx->inter_value.ctr_value.nc_off;
    size_t extra_length = 0, bytes_left = 0, blk_num = 0, encrypt_length = 0;

    if ( ( sca_type == ARM_CE_AES_CTR ) || ( sca_type == ARM_CE_SM4_CTR ) ) {
        if ( nc_off == 0 ) {
            extra_length              = ilen % ARM_CE_SCA_BLK_SIZE;
            blk_num                   = ilen / ARM_CE_SCA_BLK_SIZE;
            crypt_info->output_offset = 0;
            crypt_info->input_offset  = 0;
            crypt_info->extra_len     = extra_length;
            if ( blk_num > 0 )
                crypt_info->input_data_length = ilen - extra_length;
            else
                crypt_info->input_data_length = 0;
            nc_off = extra_length;
            if ( nc_off != 0 ) {
                pal_memcpy( extra, temp + ilen - extra_length, extra_length );
                crypt_info->extra_encrypt_flag = DBH_SCA_CTR_EXTRA_VALID;
            } else
                crypt_info->extra_encrypt_flag = DBH_SCA_CTR_EXTRA_INVALID;
        } else {
            bytes_left   = ARM_CE_SCA_BLK_SIZE - nc_off;
            nc_off_bytes = bytes_left;
            /* If input data length less than one block. */
            if ( ilen <= ARM_CE_SCA_BLK_SIZE ) {
                /* Input data length less than the total nc_off size, first
                 * use nonce counter cipher text xor ( ARM_CE_SCA_BLK_SIZE -
                 * nc_off ) bytes data, then encrypt left data
                 */
                input_left_size = ilen - bytes_left;
                if ( temp_ilen <= bytes_left ) {
                    while ( temp_ilen > 0 ) {
                        c = *temp++;
                        *out++ =
                            (unsigned char) ( c
                                              ^ ( ctx->inter_value.ctr_value
                                                      .cur_iv_ct[nc_off] ) );
                        nc_off++;
                        temp_ilen--;
                    }

                    nc_off &= 0xF;
                    crypt_info->output_offset      = 0;
                    crypt_info->input_offset       = 0;
                    crypt_info->input_data_length  = 0;
                    crypt_info->extra_encrypt_flag = DBH_SCA_CTR_EXTRA_INVALID;
                    crypt_info->extra_len          = 0;
                } else {
                    while ( bytes_left > 0 ) {
                        c = *temp++;
                        *out++ =
                            (unsigned char) ( c
                                              ^ ( ctx->inter_value.ctr_value
                                                      .cur_iv_ct[nc_off] ) );
                        bytes_left--;
                        nc_off++;
                    }

                    pal_memcpy( extra, temp, input_left_size );
                    nc_off                         = input_left_size;
                    crypt_info->output_offset      = nc_off_bytes;
                    crypt_info->input_data_length  = 0;
                    crypt_info->input_offset       = 0;
                    crypt_info->extra_encrypt_flag = DBH_SCA_CTR_EXTRA_VALID;
                    crypt_info->extra_len          = input_left_size;
                }
            } else {
                encrypt_length = ilen - bytes_left;
                extra_length   = encrypt_length % ARM_CE_SCA_BLK_SIZE;
                blk_num        = encrypt_length / ARM_CE_SCA_BLK_SIZE;
                /* Crypt left nc_off data first */
                while ( bytes_left > 0 ) {
                    c      = *temp++;
                    *out++ = (unsigned char) ( c
                                               ^ ( ctx->inter_value.ctr_value
                                                       .cur_iv_ct[nc_off] ) );
                    bytes_left--;
                    nc_off++;
                }
                /* Then copy extra data */
                if ( extra_length != 0 ) {
                    if ( blk_num != 0 ) {
                        crypt_info->input_offset = nc_off_bytes;
                        crypt_info->input_data_length =
                            encrypt_length - extra_length;
                    } else {
                        crypt_info->input_offset      = 0;
                        crypt_info->input_data_length = 0;
                    }
                    nc_off = extra_length;
                    pal_memcpy( extra,
                                temp + encrypt_length - extra_length,
                                extra_length );
                    crypt_info->extra_encrypt_flag = DBH_SCA_CTR_EXTRA_VALID;
                    crypt_info->extra_len          = extra_length;
                } else {
                    /* Data is 16 bytes aligned */
                    crypt_info->input_offset       = nc_off_bytes;
                    crypt_info->input_data_length  = encrypt_length;
                    nc_off                         = 0;
                    crypt_info->extra_encrypt_flag = DBH_SCA_CTR_EXTRA_INVALID;
                    crypt_info->extra_len          = 0;
                }
                crypt_info->output_offset = nc_off_bytes;
            }
        }
        ctx->inter_value.ctr_value.nc_off = nc_off;
    } else {
        extra_length = ilen % ARM_CE_SCA_BLK_SIZE;
        if ( ( extra_length != 0 ) ) {
            /* For ECB, CBC, CBC_MAC, Ghash input data must be 16 bytes aligned
             * For CMAC last block data not have to be 16 bytes aligned
             */
            if ( ctx->ctx_last_block_flag != ARM_CE_SCA_CTX_LAST_BLOCK ) {
                return ( DBH_SCA_DATA_ALIGN_ERR );
            } else {
                if ( ctx->mac_type != ARM_CE_CMAC ) {
                    return ( DBH_SCA_DATA_ALIGN_ERR );
                }
            }
        }
        crypt_info->output_offset     = 0;
        crypt_info->input_offset      = 0;
        crypt_info->input_data_length = ilen;
    }

    return ( 0 );
}

static int
arm_ce_sca_write_process_paramters( arm_ce_sca_context_t *ctx,
                                    int mode,
                                    size_t length,
                                    unsigned char *iv,
                                    const unsigned char *input,
                                    unsigned char extra[16],
                                    unsigned char *output,
                                    arm_ce_sca_crypt_info_t *crypt_info,
                                    unsigned char *stream_block )
{
    int i                                   = 0;
    uint8_t opcode                          = DBH_SCA_PROCESS_OPCODE;
    unsigned char *temp                      = s_sca_temp_stage;
    uint32_t value                          = 0;
    uint32_t output_addr                    = DBH_SCA_DMA_ADDR( output );
    uint32_t input_addr                     = DBH_SCA_DMA_ADDR( input );
    uint32_t extra_addr                     = DBH_SCA_DMA_ADDR( extra );
    size_t nc_off                           = ctx->inter_value.ctr_value.nc_off;
    int ret                                 = 0;
    arm_ce_sca_type_t sca_type              = ctx->sca_type;
    uint8_t last_input[ARM_CE_SCA_BLK_SIZE] = {0};
    uint8_t *last_out                       = NULL;

    /* s_sca_temp_stage is shared between calls, clear what used to be a
     * zero-initialised stack array. */
    pal_memset( temp, 0, ARM_CE_SCA_BLK_SIZE );
    /* extra / temp were just filled by the CPU (handle_input_data), and the
     * CALLER's buffers may be freshly written as well: drain the stores before
     * the engine fetches them with its own DMA. */
    DUBHE_SCA_DMA_BARRIER( );

    value |= opcode << DBH_SCA_OPCODE_BIT_SHIFT;
    /* Mode configuration */
    /* Encryption or Decryption */
    value |= ( ( mode & 0x1 ) << DBH_SCA_OP_MODE_BIT_SHIFT );
    /* Is last data block */
    if ( ctx->ctx_last_block_flag == ARM_CE_SCA_CTX_LAST_BLOCK ) {
        value |= 1 << DBH_SCA_LAST_BLOCK_BIT_SHIFT;
        ctx->ctx_last_block_flag = 0;
    }

#if defined( DUBHE_FOR_RUNTIME )
    /* Interrupt mode */
    if ( length > DBH_SCA_TRIG_INTR_BLK_SIZE ){
        ARM_CE_SCA_ENABLE_INTERRUPT();
        value |= 0x1;
    }
    else
        value &= 0xFFFFFFFE;
#else
    value &= 0xFFFFFFFE;
#endif

    if ( sca_type == ARM_CE_AES_CTR || sca_type == ARM_CE_SM4_CTR ) {
        /* Crypt extra directly */
        if ( crypt_info->input_data_length == 0 ) {
            if ( crypt_info->extra_encrypt_flag != DBH_SCA_CTR_EXTRA_VALID ) {
                return 0;
            }

            DBH_WRITE_REGISTER( SCA, SCA_QUEUE, value );
            DBH_WRITE_REGISTER( SCA, SCA_QUEUE, extra_addr );
            DBH_WRITE_REGISTER( SCA, SCA_QUEUE, ARM_CE_SCA_BLK_SIZE );
            DBH_WRITE_REGISTER( SCA, SCA_QUEUE, DBH_SCA_DMA_ADDR( temp ) );
            ret = arm_ce_sca_wait_engine_idle( );
            CHECK_RET( );
            pal_memcpy( output + crypt_info->output_offset,
                        temp,
                        crypt_info->extra_len );
            arm_ce_sca_update_ctr_iv( ctx, iv, 1 );

            for ( i = 0; i < ARM_CE_SCA_BLK_SIZE; i++ ) {
                stream_block[i]                         = temp[i] ^ extra[i];
                ctx->inter_value.ctr_value.cur_iv_ct[i] = stream_block[i];
            }

            return 0;
        } else {
            /* Save the last input data block */
            pal_memcpy( last_input,
                        input +
                        crypt_info->input_offset +
                        crypt_info->input_data_length -
                        ARM_CE_SCA_BLK_SIZE,
                        ARM_CE_SCA_BLK_SIZE);
            /* First encrypt blocks of input data, then encrypt extra */
            DBH_WRITE_REGISTER( SCA, SCA_QUEUE, value );
            DBH_WRITE_REGISTER(
                SCA,
                SCA_QUEUE,
                DBH_SCA_DMA_ADDR( input + crypt_info->input_offset ) );
            DBH_WRITE_REGISTER( SCA, SCA_QUEUE, crypt_info->input_data_length );
            DBH_WRITE_REGISTER(
                SCA,
                SCA_QUEUE,
                DBH_SCA_DMA_ADDR( output + crypt_info->output_offset ) );
#if defined( DUBHE_FOR_RUNTIME )
            /* Wait for interrupt */
            if ( length > DBH_SCA_TRIG_INTR_BLK_SIZE ) {
                ret = dubhe_wait_event( DBH_EVENT_SCA_CMD_EXCUTED );
                ARM_CE_SCA_DISABLE_INTERRUPT();
                CHECK_RET( );
            } else {
                ret = arm_ce_sca_wait_engine_idle( );
                CHECK_RET( );
            }
#else
            ret = arm_ce_sca_wait_engine_idle( );
            CHECK_RET( );
#endif
            arm_ce_sca_update_ctr_iv( ctx, iv, length - crypt_info->input_offset );
            /* Get extra encryption data, then XOR extra to get stream_block. */
            if ( nc_off != 0 ) {
                /* Write SCA process function part, use polling mode for extra
                 * data */
                value &= 0xFFFFFFFE;
                DBH_WRITE_REGISTER( SCA, SCA_QUEUE, value );
                DBH_WRITE_REGISTER( SCA, SCA_QUEUE, extra_addr );
                DBH_WRITE_REGISTER( SCA, SCA_QUEUE, ARM_CE_SCA_BLK_SIZE );
                DBH_WRITE_REGISTER( SCA, SCA_QUEUE, DBH_SCA_DMA_ADDR( temp ) );
                ret = arm_ce_sca_wait_engine_idle( );
                CHECK_RET( );

                /* Save nonce counter cipher text */
                for ( i = 0; i < ARM_CE_SCA_BLK_SIZE; i++ ) {
                    stream_block[i] = temp[i] ^ extra[i];
                    ctx->inter_value.ctr_value.cur_iv_ct[i] = stream_block[i];
                }
                /* Update last padding block cipher text */
                pal_memcpy( output + length - ctx->inter_value.ctr_value.nc_off,
                            temp,
                            ctx->inter_value.ctr_value.nc_off );
            } else {
                /* nc_off = 0, XOR against the last out data block to produce the stream block */
                last_out = output +
                           crypt_info->output_offset +
                           crypt_info->input_data_length -
                           ARM_CE_SCA_BLK_SIZE;
                for ( i = 0; i < ARM_CE_SCA_BLK_SIZE; i++ ) {
                    stream_block[i] = last_out[i] ^ last_input[i];
                    ctx->inter_value.ctr_value.cur_iv_ct[i] = stream_block[i];
                }
            }
            return 0;
        }
    } else {
        /* Write SCA process function part */
        DBH_WRITE_REGISTER( SCA, SCA_QUEUE, value );
        DBH_WRITE_REGISTER( SCA, SCA_QUEUE, input_addr );
        DBH_WRITE_REGISTER( SCA, SCA_QUEUE, crypt_info->input_data_length );
        if ( ctx->mac_type == ARM_CE_NONE_MAC )
            DBH_WRITE_REGISTER( SCA, SCA_QUEUE, output_addr );

#if defined( DUBHE_FOR_RUNTIME )
        /* Wait for interrupt */
        if ( length > DBH_SCA_TRIG_INTR_BLK_SIZE ) {
            ret = dubhe_wait_event( DBH_EVENT_SCA_CMD_EXCUTED );
            ARM_CE_SCA_DISABLE_INTERRUPT();
            CHECK_RET( );
        } else {
            ret = arm_ce_sca_wait_engine_idle( );
            CHECK_RET( );
        }
#else
        ret = arm_ce_sca_wait_engine_idle( );
        CHECK_RET( );
#endif

    }
    return ret;
}

static int arm_ce_sca_execute_sca_process( arm_ce_sca_context_t *ctx,
                                           int mode,
                                           size_t length,
                                           unsigned char *iv,
                                           unsigned char *stream_block,
                                           const unsigned char *input,
                                           unsigned char *output )
{
    int ret                                  = 0;
    unsigned char *extra                     = s_sca_extra_stage;
    arm_ce_sca_crypt_info_t crypt_info;
    arm_ce_sca_type_t sca_type = ctx->sca_type;

    pal_memset( extra, 0, ARM_CE_SCA_BLK_SIZE );

    if ( ctx->ctx_init_flag != ARM_CE_SCA_CTX_INITIALIZED ) {
        PAL_LOG_ERR( " init first!\n" );
        return -1;
    }
    pal_memset( &crypt_info, 0, sizeof( arm_ce_sca_crypt_info_t ) );
    ret = arm_ce_sca_handle_input_data(
        ctx, length, iv, input, output, extra, &crypt_info );
    if ( ret < 0 )
        return ( ret );

    /* For CBC mode, store the last cipher data block as IV for Decryption */
    if ( ( sca_type == ARM_CE_AES_CBC ) || ( sca_type == ARM_CE_SM4_CBC ) ) {
        if ( mode == ARM_CE_SCA_OPERATION_DECRYPT ) {
            pal_memcpy( ctx->inter_value.last_blk,
                        input + length - ARM_CE_SCA_BLK_SIZE,
                        ARM_CE_SCA_BLK_SIZE );
        }
    }

    /* Write parameter part and update context */
    ret = arm_ce_sca_write_process_paramters( ctx,
                                              mode,
                                              length,
                                              iv,
                                              input,
                                              extra,
                                              output,
                                              &crypt_info,
                                              stream_block );

    if ( ret != 0 ) {  /* error occur need retry */
        return ret;
    }

    ctx->retry_count = 0;  /* once execute success reset retry counter */
    ret              = arm_ce_sca_wait_engine_idle( );
    CHECK_RET( );
    /* For CBC mode, store the last cipher data block as IV for Encryption */
    if ( ( sca_type == ARM_CE_AES_CBC ) || ( sca_type == ARM_CE_SM4_CBC ) ) {
        if ( mode == ARM_CE_SCA_OPERATION_ENCRYPT ) {
            pal_memcpy( ctx->inter_value.last_blk,
                        output + crypt_info.input_data_length - ARM_CE_SCA_BLK_SIZE,
                        ARM_CE_SCA_BLK_SIZE );
        }
        /* update iv in the user buffer */
        pal_memcpy( iv, ctx->inter_value.last_blk, ARM_CE_SCA_BLK_SIZE );
    }

    return ( 0 );
}

static int arm_ce_sca_execute_sca_finish( arm_ce_sca_context_t *ctx,
                                          unsigned char *mac )
{
    uint8_t opcode    = DBH_SCA_FINISH_OPCODE;
    uint32_t value    = 0;
    uint32_t mac_addr = 0;
    int mac_staged    = 0;
    int ret           = 0;

    if ( ( ctx->mac_type != ARM_CE_NONE_MAC ) && ( mac != NULL ) ) {
        if ( dubhe_sca_dma_is_sram( mac ) ) {
            mac_addr = DBH_SCA_DMA_ADDR( mac );
        } else {
            /* The engine writes the MAC straight to mac. */
            mac_addr   = DBH_SCA_DMA_ADDR( s_sca_mac_stage );
            mac_staged = 1;
        }
    }

    /* Validate the MAC buffer before the first queue word is written: the
     * FINISH command and its parameter word must be queued as a pair, a
     * half-written command would desynchronise the engine. */
    if ( ( ctx->mac_type != ARM_CE_NONE_MAC ) && ( mac == NULL ) )
        return ( DBH_SCA_PARAM_INVAL_ERR );

    value |= opcode << DBH_SCA_OPCODE_BIT_SHIFT;
    /* This command will not trigger an interrupt */
    /* Write function part */
    DBH_WRITE_REGISTER( SCA, SCA_QUEUE, value );
    if ( ctx->mac_type != ARM_CE_NONE_MAC )
        DBH_WRITE_REGISTER( SCA, SCA_QUEUE, mac_addr );

    ret = arm_ce_sca_wait_engine_idle( );
    if ( mac_staged ) {
        if ( ret == 0 )
            pal_memcpy( mac, s_sca_mac_stage, ARM_CE_SCA_IV_BYTES );
        /* Do not leave the MAC behind when the wait failed. */
        dubhe_zeroize( s_sca_mac_stage, sizeof( s_sca_mac_stage ) );
    }
    CHECK_RET( );
    ctx->ctx_finish_flag = 0;  /* reset finish flag */
    ctx->ctx_init_flag = ARM_CE_SCA_CTX_NEED_INIT;
    return ( 0 );
}

static bool _is_continuous_context( arm_ce_sca_context_t *ctx,
                                    int mode,
                                    arm_ce_sca_type_t type,
                                    arm_ce_sca_mac_type_t mac_type,
                                    size_t *nc_off,
                                    unsigned char *iv,
                                    unsigned char *stream_block )
{
    if( ctx->sca_type != type ){
        return false;
    }
    /* check if we are using the sample mode */
    if ( ctx->mode != mode ) {
        return false;
    }

    /* check AES-CBC IV */
    if ( ( type == ARM_CE_AES_CBC ) || ( type == ARM_CE_SM4_CBC ) ) {
        /* check if the input iv equals to our saved iv */
        if ( 0 != pal_memcmp( iv, ctx->inter_value.last_blk, 16 ) ) {
            return false;
        }
    } else if ( ( type == ARM_CE_AES_CTR ) || ( type == ARM_CE_SM4_CTR ) ) {
        /* check if the input iv and nc_off equals to our saved value */
        if ( ( 0 != pal_memcmp( iv, ctx->inter_value.ctr_value.cur_iv, 16 ) )
             || ( *nc_off != ctx->inter_value.ctr_value.nc_off ) ) {
            return false;
        }
    } else {
        return true;
    }
    return true;
}

int arm_ce_sca_crypt( arm_ce_sca_context_t *ctx,
                      int mode,
                      arm_ce_sca_type_t type,
                      arm_ce_sca_mac_type_t mac_type,
                      size_t length,
                      size_t *nc_off,
                      unsigned char *iv,
                      unsigned char *stream_block,
                      const unsigned char *input,
                      unsigned char *output )
{
    int ret = 0;
    arm_ce_sca_context_t swap_ctx;
    unsigned char *stage_out      = NULL;
    void *stage_in_base           = NULL;
    void *stage_out_base          = NULL;
    unsigned char *output_orig    = output;

    if ( ( ctx == NULL ) || ( ctx->magic != ARM_CE_SCA_MAGIC )
         || ( input == NULL ) || ( ARM_CE_SCA_OPERATION_DECRYPT != mode
         && ARM_CE_SCA_OPERATION_ENCRYPT != mode ) ) {
        PAL_LOG_ERR( "%s+%d crypto invalid parameter, mode:%d\n",
                        __FILE__, __LINE__, mode );
        ret = DBH_SCA_PARAM_INVAL_ERR;
        goto exit;
    }
    if ( ( type == ARM_CE_AES_CTR ) || ( type == ARM_CE_SM4_CTR ) ) {
        if ( nc_off == NULL || stream_block == NULL ) {
            PAL_LOG_ERR( "crypto invalid parameter, parameter does not match "
                         "the cipher mode\n" );
            ret = DBH_SCA_PARAM_INVAL_ERR;
            goto exit;
        }
    }
    if ( ( iv == NULL ) && ( type != ARM_CE_AES_ECB )
         && ( type != ARM_CE_SM4_ECB ) ) {
        PAL_LOG_ERR( "crypto invalid parameter, parameter does not match the "
                     "cipher mode: %d\n",
                     type );
        ret = DBH_SCA_PARAM_INVAL_ERR;
        goto exit;
    }
    if ( mac_type == ARM_CE_NONE_MAC ) {
        if ( output == NULL ) {
            PAL_LOG_ERR("crypto invalid parameter, output buffer is NULL\n");
            ret = DBH_SCA_PARAM_INVAL_ERR;
            goto exit;
        }
    }

    if ( length == 0 ) {
        goto exit;
    }

#if defined( DUBHE_FOR_RUNTIME )
    /* Get command queue lock */
    dubhe_mutex_lock( DBH_SCA_MUTEX );
#endif

    /* Buffers the CPU does not cache (SRAM, non-cacheable PSRAM, flash) are
     * DMA-visible, so hand them to the engine directly. Cacheable buffers
     * (libtuyaos.a hands us record buffers that live in the cacheable PSRAM
     * heap) are staged into a DMA-visible arena and the result is copied back
     * at clean_up before the staging buffer is released. */
    if ( !dubhe_sca_dma_can_raw( input, length ) ) {
        unsigned char *p = dubhe_sca_stage_alloc( length, input, &stage_in_base,
                                                  DUBHE_SCA_IN_STAGE );

        if ( p == NULL ) {
            PAL_LOG_ERR( "no staging buffer for input, len %u\n",
                         (unsigned int)length );
            ret = DBH_SCA_PARAM_INVAL_ERR;
            goto clean_up;
        }
        input    = p;
    }
    if ( ( output != NULL ) && !dubhe_sca_dma_can_raw( output, length ) ) {
        /* Preserve whatever the engine does not write back. */
        unsigned char *p = dubhe_sca_stage_alloc( length, output,
                                                  &stage_out_base,
                                                  DUBHE_SCA_OUT_STAGE );

        if ( p == NULL ) {
            PAL_LOG_ERR( "no staging buffer for output, len %u\n",
                         (unsigned int)length );
            ret = DBH_SCA_PARAM_INVAL_ERR;
            goto clean_up;
        }
        stage_out = p;
        output    = p;
    }

    /* The caller's stores into input/output must be in memory before the
     * engine fetches them with its own DMA. */
    DUBHE_SCA_DMA_BARRIER( );
    /* Multi-session context */
    if ( _g_ce_sca_ctx != ctx ) {
        if ( _g_ce_sca_ctx != NULL ) {
        RETRY:
            if ( _g_ce_sca_ctx->ctx_finish_flag == ARM_CE_SCA_CTX_NEED_FINISH ) {
                ret = arm_ce_sca_execute_sca_finish(
                    _g_ce_sca_ctx, _g_ce_sca_ctx->inter_value.last_blk );
                CHECK_RETRY( _g_ce_sca_ctx, 1 );
                _g_ce_sca_ctx->retry_count = 0;
            }
            _g_ce_sca_ctx              = NULL;
        }

        ctx->sca_type = type;
        ctx->mode = mode;
        ctx->mac_type = mac_type;

        if ( mac_type != ARM_CE_NONE_MAC ) {
            pal_memcpy( iv, ctx->inter_value.last_blk, ARM_CE_SCA_IV_BYTES );
        }

        /* Execute Dubhe SCA_init command */
        ret = arm_ce_sca_execute_sca_init(
            ctx, ctx->sca_type, mac_type, nc_off, iv, stream_block );
        if ( ret < 0 ) {
            PAL_LOG_ERR( " execute sca init failed, reason: -%x\n", -ret );
            goto clean_up;
        }
        _g_ce_sca_ctx = ctx;
    } else {
        /* check current context conherency */
        if ( !_is_continuous_context(
                 ctx, mode, type, mac_type, nc_off, iv, stream_block ) ) {
            if ( _g_ce_sca_ctx->ctx_finish_flag == ARM_CE_SCA_CTX_NEED_FINISH ) {
                ret = arm_ce_sca_execute_sca_finish(
                    _g_ce_sca_ctx, _g_ce_sca_ctx->inter_value.last_blk );
                CHECK_RETRY( _g_ce_sca_ctx, 1 );
                _g_ce_sca_ctx->retry_count = 0;
            }
            _g_ce_sca_ctx              = NULL;

            ctx->sca_type = type;
            ctx->mode = mode;
            ctx->mac_type = mac_type;

            if ( mac_type != ARM_CE_NONE_MAC ) {
                pal_memcpy( iv, ctx->inter_value.last_blk, ARM_CE_SCA_IV_BYTES);
            }

            /* Execute Dubhe SCA_init command */
            ret = arm_ce_sca_execute_sca_init(
                ctx, ctx->sca_type, mac_type, nc_off, iv, stream_block );
            if ( ret < 0 ) {
                PAL_LOG_ERR( " execute sca init failed, reason: %x\n", ret );
                goto clean_up;
            }
            _g_ce_sca_ctx = ctx;
        }
    }
_RETRY:
    pal_memcpy(&swap_ctx, ctx, sizeof(swap_ctx));
    if ( swap_ctx.ctx_init_flag == ARM_CE_SCA_CTX_NEED_INIT ) {
        ret = arm_ce_sca_execute_sca_init(
            &swap_ctx, swap_ctx.sca_type, mac_type, nc_off, iv, stream_block );

        if ( ret < 0 ) {
            PAL_LOG_ERR( " execute sca init failed, reason: %x\n", ret );
            goto clean_up;
        }
    }
    {
        /* process() consumes iv / stream_block (CTR keeps the running
         * keystream there) while the command is being prepared. Keep the
         * pre-call copies so a BUS_ERR retry restarts this very block. */
        unsigned char iv_save[ARM_CE_SCA_IV_BYTES];
        unsigned char sb_save[ARM_CE_SCA_IV_BYTES];

        if ( iv != NULL )
            pal_memcpy( iv_save, iv, sizeof( iv_save ) );
        if ( stream_block != NULL )
            pal_memcpy( sb_save, stream_block, sizeof( sb_save ) );

        ret = arm_ce_sca_execute_sca_process(
            &swap_ctx, mode, length, iv, stream_block, input, output );
        if ( ret < 0 ) {
            if ( ( DBH_SCA_BUS_ERR == ret ) ) {
                if ( swap_ctx.retry_count++ < RETRY_COUNT ) {
                    /* if mac process failed, should finish ctx first, then
                     * retry init */
                    if ( swap_ctx.mac_type != ARM_CE_NONE_MAC ) {
                        ret = arm_ce_sca_execute_sca_finish(
                            &swap_ctx, swap_ctx.inter_value.last_blk );
                        if (ret) {
                            PAL_LOG_ERR( " execute sca finish failed, reason: %x\n", ret );
                            goto session_invalid;
                        }
                    }
                    if ( iv != NULL )
                        pal_memcpy( iv, iv_save, sizeof( iv_save ) );
                    if ( stream_block != NULL )
                        pal_memcpy( stream_block, sb_save, sizeof( sb_save ) );
                    /* CHECK_RET() has just reset the engine on BUS_ERR, so
                     * the key/IV programmed by SCA_init are gone and the
                     * retry must re-init. _RETRY reloads swap_ctx from ctx,
                     * so the flag has to be set in ctx (writing swap_ctx
                     * alone is overwritten). */
                    ctx->ctx_init_flag = ARM_CE_SCA_CTX_NEED_INIT;
                    /* _RETRY reloads swap_ctx from ctx, which would reset the
                     * counter to its stale value, so keep the incremented one
                     * in ctx; otherwise RETRY_COUNT never takes effect. */
                    ctx->retry_count = swap_ctx.retry_count;
                    PAL_LOG_ERR( "BUS ERROR reinit engine and retry, time(%d)\n",
                                 swap_ctx.retry_count );
                    goto _RETRY;
                }
            } else if ( DBH_SCA_DATA_ALIGN_ERR == ret ) {
                /* handle_input_data() rejected the request before any command
                 * was queued, so the engine session is still intact: return
                 * the parameter error and keep the accumulated MAC/session
                 * state instead of dropping it. */
                goto clean_up;
            }

        session_invalid:
            /* The switch failed (retries exhausted, or the retry could not
             * finish the MAC session). CHECK_RET() has reset the engine on
             * BUS_ERR, so the key/IV programmed by SCA_init are gone and the
             * accumulated state is unknown: drop the session and mark the
             * context unusable. The next arm_ce_sca_crypt() with this context
             * takes the fresh-session path and re-inits the engine, instead of
             * processing on a default engine and returning wrong data. */
            ctx->ctx_init_flag   = ARM_CE_SCA_CTX_NEED_INIT;
            ctx->ctx_finish_flag = 0;
            ctx->retry_count     = 0;
            _g_ce_sca_ctx        = NULL;

            goto clean_up;
        }
    }

    swap_ctx.retry_count = 0;
    pal_memcpy(ctx, &swap_ctx, sizeof(swap_ctx));
    /* Update nc_off */
    if ( type == ARM_CE_AES_CTR || type == ARM_CE_SM4_CTR ) {
        *nc_off = ctx->inter_value.ctr_value.nc_off;
    }
    ret = 0;

clean_up:
    /* Copy the staged output back before the staging buffer is released. */
    if ( stage_out != NULL ) {
        pal_memcpy( output_orig, stage_out, length );
        stage_out = NULL;
    }
    /* CTR plaintext / keystream and the shared temp block would otherwise stay
     * in SRAM after the last command; wipe them while the engine lock is held. */
    dubhe_zeroize( s_sca_extra_stage, sizeof( s_sca_extra_stage ) );
    dubhe_zeroize( s_sca_temp_stage, sizeof( s_sca_temp_stage ) );
#if defined( DUBHE_FOR_RUNTIME )
    dubhe_mutex_unlock( DBH_SCA_MUTEX );
#endif
    if ( stage_out_base != NULL ) {
        dubhe_zeroize( stage_out_base, length );
        os_free( stage_out_base );
        stage_out_base = NULL;
    }
exit:
    if ( stage_in_base != NULL ) {
        dubhe_zeroize( stage_in_base, length );
        os_free( stage_in_base );
    }
    return ( ret );
}

int arm_ce_sca_finish( arm_ce_sca_context_t *ctx, unsigned char *mac )
{
    int ret = 0;

    if ( ctx == NULL )
        return ( DBH_SCA_PARAM_INVAL_ERR );
    if ( ctx->magic != ARM_CE_SCA_MAGIC )
        return ( DBH_SCA_PARAM_INVAL_ERR );
    if ( ctx->mac_type == ARM_CE_NONE_MAC )
        return ( DBH_SCA_PARAM_INVAL_ERR );
    if ( mac == NULL )
        return ( DBH_SCA_PARAM_INVAL_ERR );

#if defined( DUBHE_FOR_RUNTIME )
    dubhe_mutex_lock( DBH_SCA_MUTEX );
#endif
    /* Multi-session context */
    if ( _g_ce_sca_ctx == ctx ) {
        if ( ctx->ctx_finish_flag == ARM_CE_SCA_CTX_NEED_FINISH ) {
            ret = arm_ce_sca_execute_sca_finish( ctx, mac );
            if ( ret ) {
                goto clean_up;
            }
        } else {
            ret = DBH_SCA_MAC_INVAL_ERR;
            goto clean_up;
        }
        pal_memset( ctx->inter_value.last_blk, 0x00, ARM_CE_SCA_BLK_SIZE);
    } else {
        if ( _g_ce_sca_ctx != NULL ) {
            /* Finish old session */
            if ( _g_ce_sca_ctx->ctx_finish_flag == ARM_CE_SCA_CTX_NEED_FINISH ) {
                ret = arm_ce_sca_execute_sca_finish(
                    _g_ce_sca_ctx, _g_ce_sca_ctx->inter_value.last_blk );
                if ( ret ) {
                    goto clean_up;
                }
            }
            _g_ce_sca_ctx = NULL;
        }
        /* Init current session */
        ret = arm_ce_sca_execute_sca_init(
            ctx, ctx->sca_type, ctx->mac_type, NULL, ctx->inter_value.last_blk, NULL );
        if ( ret < 0 ) {
            PAL_LOG_ERR( " execute sca init failed, reason: -%x\n", -ret );
            goto clean_up;
        }
        _g_ce_sca_ctx = ctx;
        /* Finish current session */
        ret = arm_ce_sca_execute_sca_finish( ctx, mac );
        if ( ret ) {
            goto clean_up;
        }
        pal_memset( ctx->inter_value.last_blk, 0x00, ARM_CE_SCA_BLK_SIZE);
    }
    _g_ce_sca_ctx = NULL;

clean_up:
#if defined( DUBHE_FOR_RUNTIME )
    dubhe_mutex_unlock( DBH_SCA_MUTEX );
#endif
    return ( ret );
}

int arm_ce_sca_free( arm_ce_sca_context_t *ctx, unsigned char *mac )
{
    int ret = 0;
    uint8_t tmp_mac[ARM_CE_SCA_BLK_SIZE] = {0};

    if ( ctx == NULL )
        return ( DBH_SCA_PARAM_INVAL_ERR );
    if ( ctx->magic != ARM_CE_SCA_MAGIC )
        return ( DBH_SCA_PARAM_INVAL_ERR );

#if defined( DUBHE_FOR_RUNTIME )
    dubhe_mutex_lock( DBH_SCA_MUTEX );
#endif
RETRY:
    if ( _g_ce_sca_ctx == ctx ) {
        if ( ctx->mac_type != ARM_CE_NONE_MAC ) {
            if ( ctx->ctx_finish_flag == ARM_CE_SCA_CTX_NEED_FINISH ) {
                ret = arm_ce_sca_execute_sca_finish( ctx, tmp_mac );
                CHECK_RETRY( ctx, 1 );
                ctx->retry_count = 0;
                if ( mac != NULL ) {
                    pal_memcpy( mac, tmp_mac, ARM_CE_SCA_BLK_SIZE );
                }
            }
        } else {
            if ( ctx->ctx_finish_flag == ARM_CE_SCA_CTX_NEED_FINISH ) {
                ret = arm_ce_sca_execute_sca_finish( ctx, NULL );
                CHECK_RETRY( ctx, 1 );
                ctx->retry_count = 0;
            }
        }
        /* free operation have to set NULL to _g_ce_sca_ctx to avoid stale pointer */
        _g_ce_sca_ctx = NULL;
    }
    dubhe_zeroize( ctx, sizeof( arm_ce_sca_context_t ) );
clean_up:
#if defined( DUBHE_FOR_RUNTIME )
    dubhe_mutex_unlock( DBH_SCA_MUTEX );
#endif
exit:
    return ( ret );
}

/*************************** The End Of File*****************************/
