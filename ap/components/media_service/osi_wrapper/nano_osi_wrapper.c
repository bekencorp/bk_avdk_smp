#include <components/system.h>
#include <driver/int.h>
#include "nano_osi_wrapper.h"
#include "sys_driver.h"
#include "aspl_lock.h"
#include "spinlock.h"
#if CONFIG_HIGH_PERFORMANCE_DMA
#include <driver/hpdma.h>
#endif

static void *nano_malloc_wrapper(uint32_t size)
{
    return hsram_malloc(size);
}

static void nano_free_wrapper(void *ptr)
{
    os_free(ptr);
}

static void *nano_memset_wrapper(void *s, int c, uint32_t n)
{
    return os_memset(s, c, n);
}

static void *nano_memcpy_wrapper(void *dest, const void *src, uint32_t n)
{
    return os_memcpy(dest, src, n);
}

#if CONFIG_HIGH_PERFORMANCE_DMA
#include <driver/hal/hal_hpdma_types.h>

/*
 * HPDMA single-shot xsize is a 16-bit field (max 65535 bytes), so a >64 KB
 * transfer must be broken into chunks. The previous implementation issued one
 * blocking bk_hpdma_memcpy() per chunk (alloc -> init -> start -> bk_delay_us(1)
 * busy-wait -> free), which:
 *   - serialised 23 channel setups for a 1.38 MB frame,
 *   - spun the CPU on bk_delay_us during every chunk, and
 *   - left the PSRAM bus idle between chunks while the CPU reprogrammed.
 *
 * Instead we build a single HPDMA linked-list (one descriptor per <=60 KB
 * chunk), arm the finish interrupt on the LAST descriptor only, kick the whole
 * chain with one bk_hpdma_link_transfer(), and block the caller on a semaphore
 * that the finish ISR posts. The CPU is suspended (not busy-waiting) for the
 * whole transfer, the chain runs back-to-back in hardware, and link mode uses a
 * 128-bit datapath. Any failure falls back to CPU memcpy so correctness is
 * never at risk.
 */
#define NANO_HPDMA_MAX_CHUNK   0xF000U  /* 61440, safely < 0xFFFF */
#define NANO_HPDMA_MAX_DESCS   48U      /* covers up to ~2.8 MB per transfer */
#define NANO_HPDMA_WAIT_MS     200U

static beken_semaphore_t s_nano_dma_sem;
static bool s_nano_dma_sem_ready;

static void nano_dma_finish_isr(hpdma_id_t hpdma_id, void *user_data)
{
    (void)hpdma_id;
    (void)user_data;
    if (s_nano_dma_sem != NULL) {
        rtos_set_semaphore(&s_nano_dma_sem);
    }
}

/*
 * Linked-list async copy. Returns 0 on success, non-zero so the caller can fall
 * back to CPU memcpy. n must be > 0.
 */
static int nano_dma_memcpy_link(void *dest, const void *src, uint32_t n)
{
    hpdma_link_config_t cfg[NANO_HPDMA_MAX_DESCS];
    uint32_t desc_cnt = (n + NANO_HPDMA_MAX_CHUNK - 1U) / NANO_HPDMA_MAX_CHUNK;
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    void *table = NULL;
    hpdma_id_t chnl = HPDMA_ID_MAX;
    uint32_t remain = n;
    uint32_t i;
    int rc = -1;

    if (desc_cnt > NANO_HPDMA_MAX_DESCS) {
        return -1;  /* too large for one chain; caller uses CPU fallback */
    }

    /* One-shot semaphore creation, reused across all transfers. */
    if (!s_nano_dma_sem_ready) {
        if (rtos_init_semaphore(&s_nano_dma_sem, 1) != BK_OK) {
            return -1;
        }
        s_nano_dma_sem_ready = true;
    }

    table = bk_hpdma_link_init(desc_cnt);
    if (table == NULL) {
        return -1;
    }

    chnl = bk_hpdma_alloc(HPDMA_DEV_DTCM);
    if (chnl >= HPDMA_ID_MAX) {
        bk_hpdma_link_deinit(table);
        return -1;
    }

    os_memset(cfg, 0, sizeof(cfg[0]) * desc_cnt);
    for (i = 0; i < desc_cnt; i++) {
        uint32_t chunk = (remain > NANO_HPDMA_MAX_CHUNK) ? NANO_HPDMA_MAX_CHUNK : remain;

        cfg[i].src_addr = (uint32_t)(uintptr_t)(s + i * NANO_HPDMA_MAX_CHUNK);
        cfg[i].dst_addr = (uint32_t)(uintptr_t)(d + i * NANO_HPDMA_MAX_CHUNK);
        cfg[i].src_xsize = (uint16_t)chunk;
        cfg[i].dst_xsize = (uint16_t)chunk;
        cfg[i].src_ysize = 1U;
        cfg[i].dst_ysize = 1U;
        cfg[i].src_step = 0U;
        cfg[i].dst_step = 0U;
        /* Only the last node raises finish so we wake up exactly once. */
        cfg[i].finish_int_en = (i == desc_cnt - 1U) ? 1U : 0U;
        cfg[i].half_finish_int_en = 0U;
        remain -= chunk;
    }

    if (bk_hpdma_link_set_descs(table, cfg, desc_cnt) != BK_OK) {
        goto out;
    }

    bk_hpdma_register_isr(chnl, NULL, NULL, nano_dma_finish_isr, NULL);
    bk_hpdma_enable_finish_interrupt(chnl);

    if (bk_hpdma_link_transfer(chnl, table) != BK_OK) {
        goto out;
    }

    if (rtos_get_semaphore(&s_nano_dma_sem, NANO_HPDMA_WAIT_MS) != BK_OK) {
        bk_hpdma_stop(chnl);
        goto out;
    }

    rc = 0;

out:
    bk_hpdma_disable_finish_interrupt(chnl);
    bk_hpdma_free(HPDMA_DEV_DTCM, chnl);
    bk_hpdma_link_deinit(table);
    return rc;
}

static int nano_dma_memcpy_wrapper(void *dest, const void *src, uint32_t n)
{
    if (dest == NULL || src == NULL || n == 0U) {
        return -1;
    }
    return nano_dma_memcpy_link(dest, src, n);
}
#endif

static int nano_memcmp_wrapper(const void *s1, const void *s2, size_t n)
{
    return os_memcmp(s1, s2, n);
}

int nano_strcmp_wrapper(const char *s1, const char *s2)
{
    return os_strcmp(s1, s2);
}

static int nano_vsnprintf_wrapper(char *str, uint32_t size, const char *fmt, va_list args)
{
    return vsnprintf(str, size, fmt, args);
}

static void nano_usleep_wrapper(uint32_t usec)
{
    extern void bk_delay_us(UINT32 us);
    bk_delay_us(usec);
}

static void nano_delay_milliseconds_wrapper(uint32_t milliseconds)
{
    rtos_delay_milliseconds(milliseconds);
}

static int nano_mutex_create_wrapper(beken_mutex_t *os_mutex)
{
    return rtos_init_mutex(os_mutex);
}

static int nano_mutex_destroy_wrapper(beken_mutex_t *os_mutex)
{
    return rtos_deinit_mutex(os_mutex);
}

static int nano_mutex_lock_wrapper(beken_mutex_t *os_mutex)
{
    return rtos_lock_mutex(os_mutex);
}

static int nano_mutex_unlock_wrapper(beken_mutex_t *os_mutex)
{
    return rtos_unlock_mutex(os_mutex);
}

static int nano_sem_init_wrapper(beken_semaphore_t *os_sem, uint32_t value)
{
    return rtos_init_semaphore_ex(os_sem, 1, value);
}

static int nano_sem_post_wrapper(beken_semaphore_t *os_sem)
{
    return rtos_set_semaphore(os_sem);
}

static int nano_sem_wait_wrapper(beken_semaphore_t *os_sem, uint32_t ms)
{
    return rtos_get_semaphore(os_sem, ms);
}

static int nano_sem_destroy_wrapper(beken_semaphore_t *os_sem)
{
    return rtos_deinit_semaphore(os_sem);
}

static int nano_init_queue_wrapper(beken_queue_t *os_queue, const char *name, uint32_t size, uint32_t number_of_messages)
{
    return rtos_init_queue(os_queue, name, size, number_of_messages);
}

static int nano_deinit_queue_wrapper(beken_queue_t *os_queue)
{
    return rtos_deinit_queue(os_queue);
}

static int nano_queue_send_wrapper(beken_queue_t *os_queue, void *data, uint32_t timeout)
{
    return rtos_push_to_queue(os_queue, data, timeout);
}

static int nano_queue_recv_wrapper(beken_queue_t *os_queue, void *data, uint32_t timeout)
{
    return rtos_pop_from_queue(os_queue, data, timeout);
}

static int nano_thread_create_wrapper( beken_thread_t* thread, uint8_t priority, const char* name,
    void (*function)(void *), uint32_t stack_size, void *arg)
{
    return rtos_create_hsram_thread(thread, priority, name, (beken_thread_function_t)function, stack_size, arg);
}

static int nano_thread_destroy_wrapper(beken_thread_t *thread)
{
    return rtos_delete_thread(thread);
}

static int isp_int_isr_register_wrapper(uint8_t type, void* isr, void* arg)
{
    return bk_int_isr_register(type, isr, arg);
}

static int isp_int_isr_unregister_wrapper(uint8_t type)
{
    return bk_int_isr_unregister(type);
}

static int isp_int_enable_wrapper(uint32_t int_num, uint32_t int_en)
{
#if CONFIG_SOC_SMP
    return sys_drv_set_int_en(CPU3_CORE_ID, int_num, int_en);
#else
    return sys_drv_set_int_en(rtos_get_core_id(), int_num, int_en);
#endif
}

#if CONFIG_SOC_SMP
static SPINLOCK_SECTION volatile spinlock_t nano_spin_lock = SPIN_LOCK_INIT;
#endif

uint32_t nano_enter_critical( void )
{
    uint32_t flags = rtos_disable_int();
#if CONFIG_SOC_SMP
    spin_lock(&nano_spin_lock);
#endif
    return flags;
}

void nano_exit_critical( uint32_t state )
{
#if CONFIG_SOC_SMP
    spin_unlock(&nano_spin_lock);
#endif
    rtos_enable_int(state);
}

static uint32_t nano_enter_critical_wrapper(void)
{
    return nano_enter_critical();
}

static void nano_exit_critical_wrapper(uint32_t flags)
{
    nano_exit_critical(flags);
}

static uint32_t nano_module_enter_critical_wrapper(bk_nano_module_t module)
{
#if CONFIG_HSPL
    if (module == BK_NANO_MODULE_VENC) {
        return bk_aspl_venc_enter_critical();
    }
    else if (module == BK_NANO_MODULE_VDEC) {
        return bk_aspl_vdec_enter_critical();
    }
    else if (module == BK_NANO_MODULE_ISP) {
        return bk_aspl_isp_enter_critical();
    }
    else {
        return nano_enter_critical();
    }
#else
    return 0;
#endif
}

static void nano_module_exit_critical_wrapper(bk_nano_module_t module, uint32_t flags)
{
#if CONFIG_HSPL
    if (module == BK_NANO_MODULE_VENC) {
        bk_aspl_venc_exit_critical(flags);
    }
    else if (module == BK_NANO_MODULE_VDEC) {
        bk_aspl_vdec_exit_critical(flags);
    }
    else if (module == BK_NANO_MODULE_ISP) {
        bk_aspl_isp_exit_critical(flags);
    }
    else {
        nano_exit_critical(flags);
    }
#else
#endif
}

#ifdef CONFIG_VG_LITE_GPU_BASE_ADDRESS
#define GPU_BASE_ADDR CONFIG_VG_LITE_GPU_BASE_ADDRESS
#else
#define GPU_BASE_ADDR 0
#endif

#ifdef CONFIG_VG_LITE_GPU_CONTIGUOUS_MEM_SZ
#define GPU_VG_LITE_CONTIGUOUS_MEM_SZ CONFIG_VG_LITE_GPU_CONTIGUOUS_MEM_SZ
#else
#define GPU_VG_LITE_CONTIGUOUS_MEM_SZ 0
#endif

#ifdef CONFIG_VG_LITE_GPU_COMMAND_BUFFER_SIZE
#define GPU_VG_LITE_COMMAND_BUFFER_SIZE CONFIG_VG_LITE_GPU_COMMAND_BUFFER_SIZE
#else
#define GPU_VG_LITE_COMMAND_BUFFER_SIZE 0
#endif


static bk_nano_osi_funcs_t s_nano_osi_funcs =
{
    .gpu_base_addr = GPU_BASE_ADDR,
    .gpu_vg_lite_contiguous_mem_sz = GPU_VG_LITE_CONTIGUOUS_MEM_SZ,
    .gpu_vg_lite_command_buffer_size = GPU_VG_LITE_COMMAND_BUFFER_SIZE,

    .malloc      = nano_malloc_wrapper,
    .free        = nano_free_wrapper,
    .memset      = nano_memset_wrapper,
    .memcpy      = nano_memcpy_wrapper,
#if CONFIG_HIGH_PERFORMANCE_DMA
    .dma_memcpy  = nano_dma_memcpy_wrapper,
#else
    .dma_memcpy  = NULL,
#endif
    .memcmp      = nano_memcmp_wrapper,
    .strcmp      = nano_strcmp_wrapper,
    .vsnprintf   = nano_vsnprintf_wrapper,
    .log_printf   = bk_vprintf_ext,
    .usleep      = nano_usleep_wrapper,
    .delay_milliseconds = nano_delay_milliseconds_wrapper,

    .mutex_create  = nano_mutex_create_wrapper,
    .mutex_destroy = nano_mutex_destroy_wrapper,
    .mutex_lock    = nano_mutex_lock_wrapper,
    .mutex_unlock  = nano_mutex_unlock_wrapper,

    .sem_init    = nano_sem_init_wrapper,
    .sem_post    = nano_sem_post_wrapper,
    .sem_wait    = nano_sem_wait_wrapper,
    .sem_destroy = nano_sem_destroy_wrapper,

    .init_queue    = nano_init_queue_wrapper,
    .deinit_queue  = nano_deinit_queue_wrapper,
    .queue_send    = nano_queue_send_wrapper,
    .queue_recv    = nano_queue_recv_wrapper,

    .thread_create  = nano_thread_create_wrapper,
    .thread_destroy = nano_thread_destroy_wrapper,

    .int_isr_register = isp_int_isr_register_wrapper,
    .int_isr_unregister = isp_int_isr_unregister_wrapper,
    .int_enable = isp_int_enable_wrapper,

    .enter_critical = nano_enter_critical_wrapper,
    .exit_critical = nano_exit_critical_wrapper,

    .module_enter_critical = nano_module_enter_critical_wrapper,
    .module_exit_critical = nano_module_exit_critical_wrapper,
};

extern int vsios_sys_adapter_init(void *funcs);

bk_err_t bk_nano_osi_funcs_init(void)
{
    bk_err_t ret = BK_OK;
    if (vsios_sys_adapter_init(&s_nano_osi_funcs) != 0)
    {
        ret = BK_FAIL;
    }
    return ret;
}


