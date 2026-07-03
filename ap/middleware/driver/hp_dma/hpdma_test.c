// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/os.h>
#include "cli.h"
#include "hpdma_hal.h"
#include <driver/hpdma.h>
#include <driver/hal/hal_hpdma_types.h>
#include "hpdma_driver.h"

#if CONFIG_SUPPORT_CACHEABLE_SRAM
#include "cache.h"
#endif

// Forward declarations for internal functions
extern bk_err_t bk_hpdma_memcpy(void *out, const void *in, uint32_t len);
extern bk_err_t hpdma_memcpy_by_chnl(void *out, const void *in, uint32_t len, hpdma_id_t cpy_chnl);

#define HPDMA_TEST_DEFAULT_ALIGN  16
#define HPDMA_CLI_DEFAULT_LEN     16
#define HPDMA_TEST_TIMEOUT_MS     5000
#define HPDMA_TEST_LOG(fmt, ...)  CLI_LOGD("HPDMA_TEST: " fmt, ##__VA_ARGS__)
#define HPDMA_TEST_ERR(fmt, ...)  CLI_LOGE("HPDMA_TEST: " fmt, ##__VA_ARGS__)

static hpdma_config_t s_cli_hpdma_cfg = {
    .mode = HPDMA_WORK_MODE_SINGLE,
    .chan_prio = 2,
    .src = {
        .dev = HPDMA_DEV_DTCM,
        .width = HPDMA_DATA_WIDTH_32BITS,
        .addr_inc_en = HPDMA_ADDR_INC_ENABLE,
        .addr_loop_en = HPDMA_ADDR_LOOP_DISABLE,
        .start_addr = 0,
        // .end_addr = 0,
        .xsize = HPDMA_CLI_DEFAULT_LEN,
        .ysize = 1,
        .step = 0,
    },
    .dst = {
        .dev = HPDMA_DEV_DTCM,
        .width = HPDMA_DATA_WIDTH_32BITS,
        .addr_inc_en = HPDMA_ADDR_INC_ENABLE,
        .addr_loop_en = HPDMA_ADDR_LOOP_DISABLE,
        .start_addr = 0,
        // .end_addr = 0,
        .xsize = HPDMA_CLI_DEFAULT_LEN,
        .ysize = 1,
        .step = 0,
    },
    .trans_type = HPDMA_TRANS_DEFAULT,
};

static uint32_t *s_cli_hpdma_src_buf_p;
static uint32_t *s_cli_hpdma_dst_buf_p;

static void *hpdma_test_aligned_alloc(uint32_t size, uint32_t align);
static void hpdma_test_aligned_free(void *aligned_ptr);

static void cli_hpdma_reset_default_cfg(void)
{
    os_memset(&s_cli_hpdma_cfg, 0, sizeof(s_cli_hpdma_cfg));

    s_cli_hpdma_cfg.mode = HPDMA_WORK_MODE_SINGLE;
    s_cli_hpdma_cfg.chan_prio = 2;
    s_cli_hpdma_cfg.src.dev = HPDMA_DEV_DTCM;
    s_cli_hpdma_cfg.src.width = HPDMA_DATA_WIDTH_32BITS;
    s_cli_hpdma_cfg.src.addr_inc_en = HPDMA_ADDR_INC_ENABLE;
    s_cli_hpdma_cfg.src.addr_loop_en = HPDMA_ADDR_LOOP_DISABLE;
    s_cli_hpdma_cfg.src.xsize = HPDMA_CLI_DEFAULT_LEN;
    s_cli_hpdma_cfg.src.ysize = 1;
    s_cli_hpdma_cfg.dst.dev = HPDMA_DEV_DTCM;
    s_cli_hpdma_cfg.dst.width = HPDMA_DATA_WIDTH_32BITS;
    s_cli_hpdma_cfg.dst.addr_inc_en = HPDMA_ADDR_INC_ENABLE;
    s_cli_hpdma_cfg.dst.addr_loop_en = HPDMA_ADDR_LOOP_DISABLE;
    s_cli_hpdma_cfg.dst.xsize = HPDMA_CLI_DEFAULT_LEN;
    s_cli_hpdma_cfg.dst.ysize = 1;
    s_cli_hpdma_cfg.trans_type = HPDMA_TRANS_DEFAULT;
}

static bk_err_t cli_hpdma_prepare_default_cfg(void)
{
    if (s_cli_hpdma_src_buf_p == NULL) {
        s_cli_hpdma_src_buf_p = (uint32_t *)hpdma_test_aligned_alloc(HPDMA_CLI_DEFAULT_LEN,
                                                                      HPDMA_TEST_DEFAULT_ALIGN);
    }
    if (s_cli_hpdma_dst_buf_p == NULL) {
        s_cli_hpdma_dst_buf_p = (uint32_t *)hpdma_test_aligned_alloc(HPDMA_CLI_DEFAULT_LEN,
                                                                      HPDMA_TEST_DEFAULT_ALIGN);
    }
    if (s_cli_hpdma_src_buf_p == NULL || s_cli_hpdma_dst_buf_p == NULL) {
        if (s_cli_hpdma_src_buf_p) {
            hpdma_test_aligned_free(s_cli_hpdma_src_buf_p);
            s_cli_hpdma_src_buf_p = NULL;
        }
        if (s_cli_hpdma_dst_buf_p) {
            hpdma_test_aligned_free(s_cli_hpdma_dst_buf_p);
            s_cli_hpdma_dst_buf_p = NULL;
        }
        return BK_FAIL;
    }

    cli_hpdma_reset_default_cfg();
    s_cli_hpdma_cfg.src.start_addr = (uint32_t)s_cli_hpdma_src_buf_p;
    s_cli_hpdma_cfg.dst.start_addr = (uint32_t)s_cli_hpdma_dst_buf_p;
    os_memset(s_cli_hpdma_src_buf_p, 0xA5, HPDMA_CLI_DEFAULT_LEN);
    os_memset(s_cli_hpdma_dst_buf_p, 0, HPDMA_CLI_DEFAULT_LEN);
    return BK_OK;
}

static void cli_hpdma_help(void)
{
    CLI_LOGD("HPDMA command help: addr=hex, len/size/count/id=dec\r\n");
    CLI_LOGD("hpdma help\r\n");
    CLI_LOGD("hpdma driver {init|deinit}\r\n");
    CLI_LOGD("hpdma chan {id} {init|deinit|start|stop|get_remain_len}\r\n");
    CLI_LOGD("hpdma int {id} {reg|enable_hf_fini|disable_hf_fini|enable_fini|disable_fini}\r\n");
    CLI_LOGD("hpdma chnl alloc\r\n");
    CLI_LOGD("hpdma chnl_free {free|force} {id}    (force: S0/C - reclaim wedged channel)\r\n");
    CLI_LOGD("hpdma copy {src_hex} {dst_hex} {len_dec}\r\n");
    CLI_LOGD("hpdma link_test_1d {link_cnt_dec} {trans_len_dec}\r\n");
    CLI_LOGD("hpdma link_test_2d {link_cnt_dec} {xsize_dec} {ysize_dec} {step_dec}\r\n");
    CLI_LOGD("hpdma concurrent_test {chan_cnt_dec} {trans_len_dec}\r\n");
    CLI_LOGD("hpdma throughput {size_kb_dec} {iter_dec}\r\n");
    CLI_LOGD("hpdma stress {start|stop|status} [size_kb_dec] [chan_cnt_dec]\r\n");
    CLI_LOGD("hpdma neg_test\r\n");
    CLI_LOGD("hpdma auto [iter_dec]\r\n");
}

static void cli_hpdma_driver_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc < 2) {
        cli_hpdma_help();
        return;
    }

    if (os_strcmp(argv[1], "init") == 0) {
        BK_LOG_ON_ERR(bk_hpdma_driver_init());
        CLI_LOGD("hpdma driver init\n");
    } else if (os_strcmp(argv[1], "deinit") == 0) {
        BK_LOG_ON_ERR(bk_hpdma_driver_deinit());
        CLI_LOGD("hpdma driver deinit\n");
    } else {
        cli_hpdma_help();
        return;
    }
}

static void cli_hpdma_chan_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t id;

    if (argc < 3) {
        cli_hpdma_help();
        return;
    }

    id = os_strtoul(argv[1], NULL, 10);

    if (os_strcmp(argv[2], "init") == 0) {
        bk_err_t ret = cli_hpdma_prepare_default_cfg();
        if (ret != BK_OK) {
            CLI_LOGE("hpdma init, id=%d prepare cfg failed ret=%d\r\n", id, ret);
            return;
        }
        ret = bk_hpdma_init(id, &s_cli_hpdma_cfg);
        if (ret == BK_OK) {
            CLI_LOGD("hpdma init, id=%d src=0x%x dst=0x%x len=%u\r\n",
                     id, s_cli_hpdma_cfg.src.start_addr,
                     s_cli_hpdma_cfg.dst.start_addr,
                     s_cli_hpdma_cfg.src.xsize);
        } else {
            CLI_LOGE("hpdma init, id=%d failed ret=%d\r\n", id, ret);
        }
    } else if (os_strcmp(argv[2], "start") == 0) {
#if (CONFIG_SPE)
        bk_hpdma_set_src_sec_attr(id, HPDMA_ATTR_SEC);
        bk_hpdma_set_dest_sec_attr(id, HPDMA_ATTR_SEC);
#endif
        bk_err_t ret = bk_hpdma_start(id);
        if (ret == BK_OK) {
            CLI_LOGD("hpdma start, id=%d\n", id);
        } else {
            CLI_LOGE("hpdma start, id=%d failed ret=%d\r\n", id, ret);
        }
    } else if (os_strcmp(argv[2], "stop") == 0) {
        bk_err_t ret = bk_hpdma_stop(id);
        if (ret == BK_OK) {
            CLI_LOGD("hpdma stop, id=%d\n", id);
        } else {
            CLI_LOGE("hpdma stop, id=%d failed ret=%d\r\n", id, ret);
        }
    } else if (os_strcmp(argv[2], "deinit") == 0) {
        if(s_cli_hpdma_src_buf_p) {
            hpdma_test_aligned_free(s_cli_hpdma_src_buf_p);
            s_cli_hpdma_src_buf_p = 0;
        }
        if(s_cli_hpdma_dst_buf_p) {
            hpdma_test_aligned_free(s_cli_hpdma_dst_buf_p);
            s_cli_hpdma_dst_buf_p = 0;
        }
        cli_hpdma_reset_default_cfg();

        bk_err_t ret = bk_hpdma_deinit(id);
        if (ret == BK_OK) {
            CLI_LOGD("hpdma deinit, id=%d\n", id);
        } else {
            CLI_LOGE("hpdma deinit, id=%d failed ret=%d\r\n", id, ret);
        }
    } else if (os_strcmp(argv[2], "get_remain_len") == 0) {
        uint32_t remain_len = bk_hpdma_get_remain_len(id);
        CLI_LOGD("hpdma get remain_len, id=%d, len=%x\n", id, remain_len);
    } else {
        cli_hpdma_help();
        return;
    }
}

static void cli_hpdma_half_finish_isr(hpdma_id_t hpdma_id, void *user_data)
{
    CLI_LOGD("hpdma half finish isr(%d)\n", hpdma_id);
}

static void cli_hpdma_finish_isr(hpdma_id_t hpdma_id, void *user_data)
{
    CLI_LOGD("hpdma finish isr(%d)\n", hpdma_id);
}

static void cli_hpdma_int_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t id;

    if (argc < 3) {
        cli_hpdma_help();
        return;
    }

    id = os_strtoul(argv[1], NULL, 10);

    if (os_strcmp(argv[2], "reg") == 0) {
        BK_LOG_ON_ERR(bk_hpdma_register_isr(id, cli_hpdma_half_finish_isr, NULL, cli_hpdma_finish_isr, NULL));
        CLI_LOGD("hpdma id:%d register interrupt isr\n", id);
    } else if (os_strcmp(argv[2], "enable_hf_fini") == 0) {
        BK_LOG_ON_ERR(bk_hpdma_enable_half_finish_interrupt(id));
        CLI_LOGD("hpdma id%d enable half finish interrupt\n", id);
    } else if (os_strcmp(argv[2], "disable_hf_fini") == 0) {
        BK_LOG_ON_ERR(bk_hpdma_disable_half_finish_interrupt(id));
        CLI_LOGD("hpdma id%d disable half finish interrupt\n", id);
    } else if (os_strcmp(argv[2], "enable_fini") == 0) {
        BK_LOG_ON_ERR(bk_hpdma_enable_finish_interrupt(id));
        CLI_LOGD("hpdma id%d enable finish interrupt\n", id);
    } else if (os_strcmp(argv[2], "disable_fini") == 0) {
        BK_LOG_ON_ERR(bk_hpdma_disable_finish_interrupt(id));
        CLI_LOGD("hpdma id%d disable finish interrupt\n", id);
    } else {
        cli_hpdma_help();
        return;
    }
}

static void cli_hpdma_chnl_alloc(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint8_t id;

    if (argc < 2) {
        cli_hpdma_help();
        return;
    }

    if (os_strcmp(argv[1], "alloc") == 0) {
        id = bk_hpdma_alloc(HPDMA_DEV_DTCM);
        CLI_LOGD("hpdma channel id:%x\n", id);
    } else {
        cli_hpdma_help();
        return;
    }
}

static void cli_hpdma_chnl_free(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t id;
    uint32_t ret;

    if (argc < 3) {
        cli_hpdma_help();
        return;
    }

    if (os_strcmp(argv[1], "free") == 0) {
        id = os_strtoul(argv[2], NULL, 10);
        ret = bk_hpdma_free(HPDMA_DEV_DTCM, id);
        /*
         * S0/C (HPDMA review):
         *   bk_hpdma_free now performs stop + wait-to-idle internally
         *   and may return BK_ERR_HPDMA_TIMEOUT when the engine refuses
         *   to halt. Surface the error to the operator and hint at the
         *   force-reclaim escape hatch instead of pretending success.
         */
        if (ret == BK_ERR_HPDMA_TIMEOUT) {
            CLI_LOGE("hpdma channel free id:%u TIMEOUT - engine wedged; "
                     "use 'hpdma chnl_free force {id}' to recover\r\n", id);
        } else if (ret != BK_OK) {
            CLI_LOGE("hpdma channel free id:%u failed ret=%d\r\n", id, ret);
        } else {
            CLI_LOGD("hpdma channel free id:%u OK\r\n", id);
        }
    } else if (os_strcmp(argv[1], "force") == 0) {
        /*
         * S0/C (HPDMA review): operator-only escape hatch matching
         *   bk_hpdma_force_reclaim(); leaves the engine in a clean
         *   per-channel state but may discard in-flight data.
         */
        id = os_strtoul(argv[2], NULL, 10);
        ret = bk_hpdma_force_reclaim(HPDMA_DEV_DTCM, id);
        CLI_LOGD("hpdma channel force reclaim id:%u ret:%d\n", id, ret);
    } else {
        CLI_LOGD("cli_hpdma_chnl_free NOT free\n");
        cli_hpdma_help();
        return;
    }
}

static void cli_hpdma_copy(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc < 4) {
        CLI_LOGD("hpdma copy {src_hex} {dst_hex} {len_dec}\r\n");
        return;
    }

    uint32_t src = os_strtoul(argv[1], NULL, 16);
    uint32_t dst = os_strtoul(argv[2], NULL, 16);
    uint32_t len = os_strtoul(argv[3], NULL, 10);

    bk_err_t ret = bk_hpdma_memcpy((void*)dst, (const void*)src, len);
    if (ret == BK_OK) {
        CLI_LOGD("hpdma copy: src=0x%x dst=0x%x len=%u SUCCESS\r\n", src, dst, len);
    } else {
        CLI_LOGE("hpdma copy: src=0x%x dst=0x%x len=%u FAILED ret=%d\r\n",
                 src, dst, len, ret);
    }
}

static uint8_t cli_hpdma_compare_buffer(uint8_t *pBuffer1, uint8_t *pBuffer2, uint32_t BufferLength)
{
    while (BufferLength--) {
        if (*pBuffer1 != *pBuffer2) {
            return 1;
        }
        pBuffer1++;
        pBuffer2++;
    }
    return 0;
}

static void hpdma_link_transfer_complete_callback(hpdma_id_t hpdma_id, void *user_data)
{
    //CLI_LOGD("hpdma link transfer callback(%d)\n", hpdma_id);
    
    // Release semaphore to notify transfer completion
    // user_data points to the semaphore handle (beken_semaphore_t*)
    if (user_data != NULL) {
        beken_semaphore_t *sem_ptr = (beken_semaphore_t *)user_data;
        rtos_set_semaphore(sem_ptr);
        //CLI_LOGD("Semaphore released in callback\n");
    }
}

static void hpdma_test_release_dma_channel(hpdma_id_t *dma_id)
{
    if (dma_id == NULL || *dma_id >= HPDMA_ID_MAX) {
        return;
    }

    (void)bk_hpdma_disable_finish_interrupt(*dma_id);
    (void)bk_hpdma_disable_half_finish_interrupt(*dma_id);
    (void)bk_hpdma_register_isr(*dma_id, NULL, NULL, NULL, NULL);
    (void)bk_hpdma_free(HPDMA_DEV_DTCM, *dma_id);
    *dma_id = HPDMA_ID_MAX;
}

static void cli_hpdma_link_test_1d(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc < 3) {
        cli_hpdma_help();
        return;
    }

    uint32_t link_cnt = os_strtoul(argv[1], NULL, 10);
    uint32_t trans_len = os_strtoul(argv[2], NULL, 10);

    CLI_LOGD("hpdma_link_test_1d: link_cnt=%d trans_len=%d\r\n", link_cnt, trans_len);

    // Allocate source and destination buffers
    uint32_t total_len = link_cnt * trans_len;
    uint8_t *src_buf = (uint8_t *)os_malloc(total_len);
    uint8_t *dst_buf = (uint8_t *)os_malloc(total_len);

    if (src_buf == NULL || dst_buf == NULL) {
        CLI_LOGE("Failed to allocate buffers\r\n");
        if (src_buf) os_free(src_buf);
        if (dst_buf) os_free(dst_buf);
        return;
    }

    // Initialize source buffer with test pattern
    for (uint32_t i = 0; i < total_len; i++) {
        src_buf[i] = (uint8_t)(i & 0xFF);
    }
    os_memset(dst_buf, 0, total_len);

    // Initialize descriptor table
    void *desc_table = bk_hpdma_link_init(link_cnt);
    if (desc_table == NULL) {
        CLI_LOGE("Failed to initialize descriptor table\r\n");
        os_free(src_buf);
        os_free(dst_buf);
        return;
    }

    // Configure descriptors for 1D transfer
    hpdma_link_config_t configs[link_cnt];
    for (uint32_t i = 0; i < link_cnt; i++) {
        configs[i].src_addr = (uint32_t)(src_buf + i * trans_len);
        configs[i].dst_addr = (uint32_t)(dst_buf + i * trans_len);
        configs[i].src_xsize = trans_len;
        configs[i].src_ysize = 1;  // 1D: 1 row
        configs[i].dst_xsize = trans_len;
        configs[i].dst_ysize = 1;  // 1D: 1 row
        configs[i].src_step = 0;
        configs[i].dst_step = 0;
        configs[i].finish_int_en = (i == link_cnt - 1) ? 1 : 0;  // Enable interrupt on last descriptor
        configs[i].half_finish_int_en = 0;
    }

    // Set all descriptors
    BK_LOG_ON_ERR(bk_hpdma_link_set_descs(desc_table, configs, link_cnt));

    // Allocate DMA channel (application layer allocates)
    hpdma_id_t dma_id = bk_hpdma_alloc(HPDMA_DEV_DTCM);
    if (dma_id >= HPDMA_ID_MAX) {
        CLI_LOGE("Failed to allocate DMA channel\r\n");
        bk_hpdma_link_deinit(desc_table);
        os_free(src_buf);
        os_free(dst_buf);
        return;
    }

    CLI_LOGD("Starting 1D linked list transfer with channel %d...\r\n", dma_id);

    // Create semaphore for synchronization
    beken_semaphore_t transfer_sem = NULL;
    bk_err_t sem_ret = rtos_init_semaphore(&transfer_sem, 1);
    if (sem_ret != BK_OK) {
        CLI_LOGE("Failed to create semaphore\r\n");
        hpdma_test_release_dma_channel(&dma_id);
        bk_hpdma_link_deinit(desc_table);
        os_free(src_buf);
        os_free(dst_buf);
        return;
    }

    // Register ISR with semaphore as user_data
    BK_LOG_ON_ERR(bk_hpdma_register_isr(dma_id, NULL, NULL, hpdma_link_transfer_complete_callback, (void *)&transfer_sem));
    BK_LOG_ON_ERR(bk_hpdma_enable_finish_interrupt(dma_id));

    // Start transfer (asynchronous)
    BK_LOG_ON_ERR(bk_hpdma_link_transfer(dma_id, desc_table));

    CLI_LOGD("Waiting for transfer completion...\r\n");

    // Wait for semaphore (transfer completion signal from callback)
    bk_err_t wait_ret = rtos_get_semaphore(&transfer_sem, BEKEN_WAIT_FOREVER);
    if (wait_ret != BK_OK) {
        CLI_LOGE("Failed to wait for semaphore: %d\r\n", wait_ret);
        hpdma_test_release_dma_channel(&dma_id);
        rtos_deinit_semaphore(&transfer_sem);
        bk_hpdma_link_deinit(desc_table);
        os_free(src_buf);
        os_free(dst_buf);
        return;
    }

    CLI_LOGD("Transfer completed, verifying data...\r\n");

    // Verify data consistency after getting semaphore
    if (cli_hpdma_compare_buffer(src_buf, dst_buf, total_len) == 0) {
        CLI_LOGD("hpdma_link_test_1d: SUCCESS\r\n");
    } else {
        CLI_LOGE("hpdma_link_test_1d: FAILED - Data mismatch\r\n");
        // Find and print first few mismatches for debugging
        uint32_t mismatch_count = 0;
        uint32_t max_print = (total_len < 32 ? total_len : 32);
        for (uint32_t i = 0; i < total_len && mismatch_count < max_print; i++) {
            if (src_buf[i] != dst_buf[i]) {
                CLI_LOGE("Mismatch at offset %d: src=0x%02x dst=0x%02x\r\n", i, src_buf[i], dst_buf[i]);
                mismatch_count++;
            }
        }
        if (mismatch_count == 0) {
            CLI_LOGE("No mismatch found in first %d bytes, but comparison failed\r\n", max_print);
        }
    }

    // Cleanup
    hpdma_test_release_dma_channel(&dma_id);
    rtos_deinit_semaphore(&transfer_sem);
    bk_hpdma_link_deinit(desc_table);
    os_free(src_buf);
    os_free(dst_buf);
}


/* ============================================================
 * HPDMA test infrastructure (stress / automated regression)
 *
 * Buffer allocation strategy: prefer PSRAM heap when
 * CONFIG_PSRAM_AS_SYS_MEMORY=y, fall back to SRAM heap so default
 * builds without PSRAM still work. psram_free is aliased to os_free,
 * so callers always free via hpdma_test_aligned_free().
 * ============================================================ */

static void *hpdma_test_aligned_alloc(uint32_t size, uint32_t align)
{
    if (align < sizeof(void *)) {
        align = sizeof(void *);
    }
    uint32_t total = size + align + (uint32_t)sizeof(void *);
    void *base = NULL;
#if CONFIG_PSRAM_AS_SYS_MEMORY
    base = psram_malloc(total);
#endif
    if (base == NULL) {
        base = os_malloc(total);
    }
    if (base == NULL) {
        return NULL;
    }
    uintptr_t raw = (uintptr_t)base + sizeof(void *);
    uintptr_t aligned = (raw + align - 1) & ~((uintptr_t)align - 1);
    ((void **)aligned)[-1] = base;
    return (void *)aligned;
}

static void hpdma_test_aligned_free(void *aligned_ptr)
{
    if (aligned_ptr == NULL) {
        return;
    }
    void *base = ((void **)aligned_ptr)[-1];
    os_free(base);
}

static void hpdma_test_fill_pattern(uint8_t *buf, uint32_t len, uint32_t seed)
{
    for (uint32_t i = 0; i < len; i++) {
        buf[i] = (uint8_t)((i + seed) ^ ((seed >> 8) & 0xFF));
    }
}

static uint32_t hpdma_test_diff_offset(const uint8_t *a, const uint8_t *b, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        if (a[i] != b[i]) {
            return i;
        }
    }
    return len;
}

/*
 * Run a synchronous 1D linked-list transfer + memcmp.
 * Returns 0 on success, negative on any failure.
 * Used by hpdma_auto / hpdma_concurrent_test internals.
 */
static int hpdma_test_run_link_1d(uint32_t link_cnt, uint32_t trans_len)
{
    int rc = -1;
    void *desc_table = NULL;
    uint8_t *src = NULL;
    uint8_t *dst = NULL;
    hpdma_link_config_t *cfgs = NULL;
    beken_semaphore_t sem = NULL;
    hpdma_id_t dma_id = HPDMA_ID_MAX;
    bk_err_t ret;

    if (link_cnt == 0 || trans_len == 0) {
        return -1;
    }
    uint32_t total_len = link_cnt * trans_len;

    src = (uint8_t *)hpdma_test_aligned_alloc(total_len, HPDMA_TEST_DEFAULT_ALIGN);
    dst = (uint8_t *)hpdma_test_aligned_alloc(total_len, HPDMA_TEST_DEFAULT_ALIGN);
    if (src == NULL || dst == NULL) {
        HPDMA_TEST_ERR("link_1d alloc fail (need %u bytes x2)\r\n", total_len);
        goto out;
    }
    hpdma_test_fill_pattern(src, total_len, 0xA5);
    os_memset(dst, 0, total_len);

    desc_table = bk_hpdma_link_init(link_cnt);
    if (desc_table == NULL) {
        HPDMA_TEST_ERR("link_1d desc_init fail\r\n");
        goto out;
    }

    cfgs = (hpdma_link_config_t *)os_malloc(sizeof(hpdma_link_config_t) * link_cnt);
    if (cfgs == NULL) {
        HPDMA_TEST_ERR("link_1d cfgs alloc fail\r\n");
        goto out;
    }
    for (uint32_t i = 0; i < link_cnt; i++) {
        cfgs[i].src_addr = (uint32_t)(src + i * trans_len);
        cfgs[i].dst_addr = (uint32_t)(dst + i * trans_len);
        cfgs[i].src_xsize = trans_len;
        cfgs[i].src_ysize = 1;
        cfgs[i].dst_xsize = trans_len;
        cfgs[i].dst_ysize = 1;
        cfgs[i].src_step = 0;
        cfgs[i].dst_step = 0;
        cfgs[i].finish_int_en = (i == link_cnt - 1) ? 1 : 0;
        cfgs[i].half_finish_int_en = 0;
    }
    ret = bk_hpdma_link_set_descs(desc_table, cfgs, link_cnt);
    if (ret != BK_OK) {
        HPDMA_TEST_ERR("link_1d set_descs ret=%d\r\n", ret);
        goto out;
    }

    if (rtos_init_semaphore(&sem, 1) != BK_OK) {
        sem = NULL;
        goto out;
    }

    dma_id = bk_hpdma_alloc(HPDMA_DEV_DTCM);
    if (dma_id >= HPDMA_ID_MAX) {
        HPDMA_TEST_ERR("link_1d alloc channel fail\r\n");
        goto out;
    }
    bk_hpdma_register_isr(dma_id, NULL, NULL,
                          hpdma_link_transfer_complete_callback, (void *)&sem);
    bk_hpdma_enable_finish_interrupt(dma_id);

    if (bk_hpdma_link_transfer(dma_id, desc_table) != BK_OK) {
        HPDMA_TEST_ERR("link_1d transfer kick fail\r\n");
        goto out;
    }

    if (rtos_get_semaphore(&sem, HPDMA_TEST_TIMEOUT_MS) != BK_OK) {
        HPDMA_TEST_ERR("link_1d sem timeout\r\n");
        goto out;
    }

    if (hpdma_test_diff_offset(src, dst, total_len) == total_len) {
        rc = 0;
    } else {
        HPDMA_TEST_ERR("link_1d data mismatch\r\n");
    }

out:
    if (cfgs) os_free(cfgs);
    hpdma_test_release_dma_channel(&dma_id);
    if (sem) rtos_deinit_semaphore(&sem);
    if (desc_table) bk_hpdma_link_deinit(desc_table);
    if (src) hpdma_test_aligned_free(src);
    if (dst) hpdma_test_aligned_free(dst);
    return rc;
}

/*
 * Run a synchronous 2D linked-list transfer + per-row memcmp.
 * src is strided (row pitch = step bytes when step >= xsize, else contiguous);
 * dst is contiguous (row pitch == xsize). After transfer, dst row r should
 * equal src row r's first xsize bytes.
 */
static int hpdma_test_run_link_2d(uint32_t link_cnt, uint16_t xsize,
                                  uint16_t ysize, uint16_t step)
{
    int rc = -1;
    void *desc_table = NULL;
    uint8_t *src = NULL;
    uint8_t *dst = NULL;
    hpdma_link_config_t *cfgs = NULL;
    beken_semaphore_t sem = NULL;
    hpdma_id_t dma_id = HPDMA_ID_MAX;
    bk_err_t ret;

    if (link_cnt == 0 || xsize == 0 || ysize == 0) {
        return -1;
    }
    uint32_t src_pitch = step;
    if (src_pitch == 0 || src_pitch < xsize) {
        src_pitch = xsize;
    }
    if (src_pitch > 0xFFFFU) {
        HPDMA_TEST_ERR("link_2d step overflow\r\n");
        return -1;
    }

    uint32_t src_gap = src_pitch - xsize;
    uint32_t src_block_bytes = (uint32_t)(ysize - 1) * src_pitch + xsize;
    uint32_t dst_block_bytes = (uint32_t)xsize * ysize;
    uint32_t src_total = link_cnt * src_block_bytes;
    uint32_t dst_total = link_cnt * dst_block_bytes;

    src = (uint8_t *)hpdma_test_aligned_alloc(src_total, HPDMA_TEST_DEFAULT_ALIGN);
    dst = (uint8_t *)hpdma_test_aligned_alloc(dst_total, HPDMA_TEST_DEFAULT_ALIGN);
    if (src == NULL || dst == NULL) {
        HPDMA_TEST_ERR("link_2d alloc fail (src=%u dst=%u)\r\n", src_total, dst_total);
        goto out;
    }
    hpdma_test_fill_pattern(src, src_total, 0x5A);
    os_memset(dst, 0, dst_total);

    desc_table = bk_hpdma_link_init(link_cnt);
    if (desc_table == NULL) goto out;

    cfgs = (hpdma_link_config_t *)os_malloc(sizeof(hpdma_link_config_t) * link_cnt);
    if (cfgs == NULL) goto out;
    for (uint32_t i = 0; i < link_cnt; i++) {
        cfgs[i].src_addr = (uint32_t)(src + i * src_block_bytes);
        cfgs[i].dst_addr = (uint32_t)(dst + i * dst_block_bytes);
        cfgs[i].src_xsize = xsize;
        cfgs[i].src_ysize = ysize;
        cfgs[i].dst_xsize = xsize;
        cfgs[i].dst_ysize = ysize;
        cfgs[i].src_step = (uint16_t)src_gap;
        cfgs[i].dst_step = 0;
        cfgs[i].finish_int_en = (i == link_cnt - 1) ? 1 : 0;
        cfgs[i].half_finish_int_en = 0;
    }
    ret = bk_hpdma_link_set_descs(desc_table, cfgs, link_cnt);
    if (ret != BK_OK) {
        HPDMA_TEST_ERR("link_2d set_descs ret=%d\r\n", ret);
        goto out;
    }

    if (rtos_init_semaphore(&sem, 1) != BK_OK) { sem = NULL; goto out; }
    dma_id = bk_hpdma_alloc(HPDMA_DEV_DTCM);
    if (dma_id >= HPDMA_ID_MAX) goto out;
    bk_hpdma_register_isr(dma_id, NULL, NULL,
                          hpdma_link_transfer_complete_callback, (void *)&sem);
    bk_hpdma_enable_finish_interrupt(dma_id);
    if (bk_hpdma_link_transfer(dma_id, desc_table) != BK_OK) goto out;
    if (rtos_get_semaphore(&sem, HPDMA_TEST_TIMEOUT_MS) != BK_OK) {
        HPDMA_TEST_ERR("link_2d sem timeout\r\n");
        goto out;
    }

    rc = 0;
    for (uint32_t i = 0; i < link_cnt && rc == 0; i++) {
        for (uint32_t r = 0; r < ysize; r++) {
            uint8_t *src_row = src + i * src_block_bytes + (uint32_t)r * src_pitch;
            uint8_t *dst_row = dst + i * dst_block_bytes + (uint32_t)r * xsize;
            if (hpdma_test_diff_offset(src_row, dst_row, xsize) != xsize) {
                HPDMA_TEST_ERR("link_2d mismatch link=%u row=%u\r\n", i, r);
                rc = -1;
                break;
            }
        }
    }

out:
    if (cfgs) os_free(cfgs);
    hpdma_test_release_dma_channel(&dma_id);
    if (sem) rtos_deinit_semaphore(&sem);
    if (desc_table) bk_hpdma_link_deinit(desc_table);
    if (src) hpdma_test_aligned_free(src);
    if (dst) hpdma_test_aligned_free(dst);
    return rc;
}

static void cli_hpdma_link_test_2d(char *pcWriteBuffer, int xWriteBufferLen,
                                   int argc, char **argv)
{
    if (argc < 5) {
        CLI_LOGD("usage: hpdma link_test_2d {link_cnt_dec} {xsize_dec} {ysize_dec} {step_dec}\r\n");
        return;
    }
    uint32_t link_cnt = os_strtoul(argv[1], NULL, 10);
    uint32_t xsize = os_strtoul(argv[2], NULL, 10);
    uint32_t ysize = os_strtoul(argv[3], NULL, 10);
    uint32_t step  = os_strtoul(argv[4], NULL, 10);

    if (link_cnt == 0 || xsize == 0 || ysize == 0) {
        CLI_LOGE("hpdma_link_test_2d: invalid params\r\n");
        return;
    }
    if (xsize > 0xFFFFu || ysize > 0xFFFFu || step > 0xFFFFu) {
        CLI_LOGE("hpdma_link_test_2d: each dim must <= 0xFFFF\r\n");
        return;
    }

    HPDMA_TEST_LOG("link_test_2d: link_cnt=%u xsize=%u ysize=%u step=%u\r\n",
                   link_cnt, xsize, ysize, step);

    int rc = hpdma_test_run_link_2d(link_cnt, (uint16_t)xsize,
                                    (uint16_t)ysize, (uint16_t)step);
    if (rc == 0) {
        HPDMA_TEST_LOG("link_test_2d: SUCCESS\r\n");
    } else {
        HPDMA_TEST_ERR("link_test_2d: FAILED\r\n");
    }
}

static void cli_hpdma_concurrent_test(char *pcWriteBuffer, int xWriteBufferLen,
                                      int argc, char **argv)
{
    if (argc < 3) {
        CLI_LOGD("usage: hpdma concurrent_test {chan_cnt_dec} {trans_len_dec}\r\n");
        return;
    }
    uint32_t chan_cnt = os_strtoul(argv[1], NULL, 10);
    uint32_t trans_len = os_strtoul(argv[2], NULL, 10);

    if (chan_cnt == 0 || chan_cnt > HPDMA_ID_MAX || trans_len == 0) {
        CLI_LOGE("hpdma_concurrent_test: bad params (max chan=%d)\r\n", HPDMA_ID_MAX);
        return;
    }

    /* Per-channel state arrays. */
    void *desc_table[HPDMA_ID_MAX] = {0};
    uint8_t *src[HPDMA_ID_MAX] = {0};
    uint8_t *dst[HPDMA_ID_MAX] = {0};
    beken_semaphore_t sem[HPDMA_ID_MAX] = {0};
    hpdma_id_t ids[HPDMA_ID_MAX];
    int allocated[HPDMA_ID_MAX] = {0};
    int rc_count = 0;

    for (uint32_t i = 0; i < chan_cnt; i++) {
        ids[i] = HPDMA_ID_MAX;
    }

    HPDMA_TEST_LOG("concurrent_test: chan_cnt=%u trans_len=%u\r\n", chan_cnt, trans_len);

    for (uint32_t i = 0; i < chan_cnt; i++) {
        src[i] = (uint8_t *)hpdma_test_aligned_alloc(trans_len, HPDMA_TEST_DEFAULT_ALIGN);
        dst[i] = (uint8_t *)hpdma_test_aligned_alloc(trans_len, HPDMA_TEST_DEFAULT_ALIGN);
        if (src[i] == NULL || dst[i] == NULL) {
            HPDMA_TEST_ERR("concurrent_test: ch%u alloc fail\r\n", i);
            goto out;
        }
        hpdma_test_fill_pattern(src[i], trans_len, 0x100 + i);
        os_memset(dst[i], 0, trans_len);

        desc_table[i] = bk_hpdma_link_init(1);
        if (desc_table[i] == NULL) goto out;

        hpdma_link_config_t cfg = {0};
        cfg.src_addr = (uint32_t)src[i];
        cfg.dst_addr = (uint32_t)dst[i];
        cfg.src_xsize = trans_len;
        cfg.src_ysize = 1;
        cfg.dst_xsize = trans_len;
        cfg.dst_ysize = 1;
        cfg.finish_int_en = 1;
        if (bk_hpdma_link_set_descs(desc_table[i], &cfg, 1) != BK_OK) goto out;

        if (rtos_init_semaphore(&sem[i], 1) != BK_OK) {
            sem[i] = NULL;
            goto out;
        }
        ids[i] = bk_hpdma_alloc(HPDMA_DEV_DTCM);
        if (ids[i] >= HPDMA_ID_MAX) {
            HPDMA_TEST_ERR("concurrent_test: ch%u alloc dma fail\r\n", i);
            goto out;
        }
        allocated[i] = 1;
        bk_hpdma_register_isr(ids[i], NULL, NULL,
                              hpdma_link_transfer_complete_callback, (void *)&sem[i]);
        bk_hpdma_enable_finish_interrupt(ids[i]);
    }

    /* Kick all channels back-to-back. */
    for (uint32_t i = 0; i < chan_cnt; i++) {
        if (bk_hpdma_link_transfer(ids[i], desc_table[i]) != BK_OK) {
            HPDMA_TEST_ERR("concurrent_test: ch%u kick fail\r\n", i);
            goto out;
        }
    }

    /* Wait for every channel to signal done. */
    for (uint32_t i = 0; i < chan_cnt; i++) {
        if (rtos_get_semaphore(&sem[i], HPDMA_TEST_TIMEOUT_MS) != BK_OK) {
            HPDMA_TEST_ERR("concurrent_test: ch%u sem timeout\r\n", i);
            goto out;
        }
    }

    /* Verify each channel's data independently. */
    for (uint32_t i = 0; i < chan_cnt; i++) {
        if (hpdma_test_diff_offset(src[i], dst[i], trans_len) == trans_len) {
            rc_count++;
        } else {
            HPDMA_TEST_ERR("concurrent_test: ch%u data mismatch\r\n", i);
        }
    }

out:
    for (uint32_t i = 0; i < chan_cnt; i++) {
        if (allocated[i]) hpdma_test_release_dma_channel(&ids[i]);
        if (sem[i]) rtos_deinit_semaphore(&sem[i]);
        if (desc_table[i]) bk_hpdma_link_deinit(desc_table[i]);
        if (src[i]) hpdma_test_aligned_free(src[i]);
        if (dst[i]) hpdma_test_aligned_free(dst[i]);
    }
    if ((uint32_t)rc_count == chan_cnt) {
        HPDMA_TEST_LOG("concurrent_test: SUCCESS (%d/%u)\r\n", rc_count, chan_cnt);
    } else {
        HPDMA_TEST_ERR("concurrent_test: FAILED (%d/%u)\r\n", rc_count, chan_cnt);
    }
}

/* ============================================================
 * hpdma_stress: continuous transfer driven from finish ISR
 *   - mirrors psram_dma_stress.c pattern (S0 review-safe cleanup)
 *   - validates data by comparing src/dst once on stop
 * ============================================================ */

struct hpdma_stress_state {
    uint8_t              running;
    uint8_t              deiniting;
    uint8_t              chan_cnt;
    hpdma_id_t           dma_id[HPDMA_ID_MAX];
    void                *desc_table[HPDMA_ID_MAX];
    uint8_t             *src_aligned[HPDMA_ID_MAX];
    uint8_t             *dst_aligned[HPDMA_ID_MAX];
    uint32_t             buf_size;
    uint32_t             start_ms;
    volatile uint32_t    loop_cnt[HPDMA_ID_MAX];
    volatile uint32_t    err_cnt[HPDMA_ID_MAX];
};

static struct hpdma_stress_state s_hpdma_stress;

static void hpdma_stress_init_channels(struct hpdma_stress_state *st)
{
    for (uint32_t i = 0; i < HPDMA_ID_MAX; i++) {
        st->dma_id[i] = HPDMA_ID_MAX;
        st->desc_table[i] = NULL;
        st->src_aligned[i] = NULL;
        st->dst_aligned[i] = NULL;
        st->loop_cnt[i] = 0;
        st->err_cnt[i] = 0;
    }
}

static void hpdma_stress_complete_cb(hpdma_id_t hpdma_id, void *user_data)
{
    struct hpdma_stress_state *st = (struct hpdma_stress_state *)user_data;
    if (st == NULL) {
        return;
    }
    if (st->deiniting || !st->running) {
        return;
    }

    for (uint32_t i = 0; i < st->chan_cnt; i++) {
        if (st->dma_id[i] != hpdma_id) {
            continue;
        }
        if (st->desc_table[i] == NULL || st->dma_id[i] >= HPDMA_ID_MAX) {
            return;
        }
        st->loop_cnt[i]++;
        bk_err_t ret = bk_hpdma_link_transfer(st->dma_id[i], st->desc_table[i]);
        if (ret != BK_OK) {
            st->err_cnt[i]++;
            st->running = 0;
        }
        return;
    }
}

static void hpdma_stress_cleanup(void)
{
    struct hpdma_stress_state *st = &s_hpdma_stress;
    st->running = 0;

    for (uint32_t i = 0; i < HPDMA_ID_MAX; i++) {
        if (st->dma_id[i] < HPDMA_ID_MAX) {
            bk_hpdma_disable_finish_interrupt(st->dma_id[i]);
            bk_hpdma_register_isr(st->dma_id[i], NULL, NULL, NULL, NULL);
            bk_err_t ret = bk_hpdma_free(HPDMA_DEV_DTCM, st->dma_id[i]);
            if (ret == BK_ERR_HPDMA_TIMEOUT) {
                HPDMA_TEST_ERR("stress: free timeout, force reclaim ch%u\r\n", st->dma_id[i]);
                (void)bk_hpdma_force_reclaim(HPDMA_DEV_DTCM, st->dma_id[i]);
            }
            st->dma_id[i] = HPDMA_ID_MAX;
        }
        if (st->desc_table[i]) {
            bk_hpdma_link_deinit(st->desc_table[i]);
            st->desc_table[i] = NULL;
        }
        if (st->src_aligned[i]) {
            hpdma_test_aligned_free(st->src_aligned[i]);
            st->src_aligned[i] = NULL;
        }
        if (st->dst_aligned[i]) {
            hpdma_test_aligned_free(st->dst_aligned[i]);
            st->dst_aligned[i] = NULL;
        }
    }
    st->buf_size = 0;
    st->chan_cnt = 0;
    st->deiniting = 0;
}

static int hpdma_stress_start_locked(uint32_t size, uint32_t chan_cnt)
{
    struct hpdma_stress_state *st = &s_hpdma_stress;

    if (st->running) {
        HPDMA_TEST_ERR("stress: already running\r\n");
        return -1;
    }
    if (chan_cnt == 0 || chan_cnt > HPDMA_ID_MAX) {
        HPDMA_TEST_ERR("stress: bad chan_cnt=%u max=%u\r\n", chan_cnt, HPDMA_ID_MAX);
        return -1;
    }
    if (size == 0) {
        return -1;
    }
    if (size > 0xFFFFu) {
        HPDMA_TEST_ERR("stress: size > 0xFFFF, use smaller buffer\r\n");
        return -1;
    }

    hpdma_stress_init_channels(st);
    st->chan_cnt = (uint8_t)chan_cnt;
    st->buf_size = size;
    st->start_ms = rtos_get_time();

    for (uint32_t i = 0; i < chan_cnt; i++) {
        hpdma_link_config_t cfg = {0};

        st->src_aligned[i] = (uint8_t *)hpdma_test_aligned_alloc(size, HPDMA_TEST_DEFAULT_ALIGN);
        st->dst_aligned[i] = (uint8_t *)hpdma_test_aligned_alloc(size, HPDMA_TEST_DEFAULT_ALIGN);
        if (st->src_aligned[i] == NULL || st->dst_aligned[i] == NULL) {
            HPDMA_TEST_ERR("stress: ch%u alloc fail (need %u bytes x2)\r\n", i, size);
            goto fail;
        }
        hpdma_test_fill_pattern(st->src_aligned[i], size, 0xC3 + i);
        os_memset(st->dst_aligned[i], 0, size);

        st->desc_table[i] = bk_hpdma_link_init(1);
        if (st->desc_table[i] == NULL) goto fail;

        cfg.src_addr = (uint32_t)st->src_aligned[i];
        cfg.dst_addr = (uint32_t)st->dst_aligned[i];
        cfg.src_xsize = size;
        cfg.src_ysize = 1;
        cfg.dst_xsize = size;
        cfg.dst_ysize = 1;
        cfg.finish_int_en = 1;
        if (bk_hpdma_link_set_descs(st->desc_table[i], &cfg, 1) != BK_OK) goto fail;

        st->dma_id[i] = bk_hpdma_alloc(HPDMA_DEV_DTCM);
        if (st->dma_id[i] >= HPDMA_ID_MAX) goto fail;

        bk_hpdma_register_isr(st->dma_id[i], NULL, NULL,
                              hpdma_stress_complete_cb, (void *)st);
        bk_hpdma_enable_finish_interrupt(st->dma_id[i]);
    }

    st->running = 1;

    for (uint32_t i = 0; i < chan_cnt; i++) {
        if (bk_hpdma_link_transfer(st->dma_id[i], st->desc_table[i]) != BK_OK) {
            HPDMA_TEST_ERR("stress: ch%u kick fail\r\n", i);
            st->running = 0;
            goto fail;
        }
    }
    HPDMA_TEST_LOG("stress: started chan_cnt=%u size=%u\r\n", chan_cnt, size);
    return 0;

fail:
    hpdma_stress_cleanup();
    return -1;
}

static void hpdma_stress_status(void)
{
    struct hpdma_stress_state *st = &s_hpdma_stress;
    if (!st->running && st->chan_cnt == 0) {
        HPDMA_TEST_LOG("stress: idle\r\n");
        return;
    }
    uint32_t now_ms = rtos_get_time();
    uint32_t elapsed = now_ms - st->start_ms;
    if (elapsed == 0) elapsed = 1;
    uint32_t total_loops = 0;
    uint32_t total_errs = 0;
    for (uint32_t i = 0; i < st->chan_cnt; i++) {
        total_loops += st->loop_cnt[i];
        total_errs += st->err_cnt[i];
    }
    uint64_t total_bytes = (uint64_t)total_loops * st->buf_size;
    /* KB/s = bytes / ms; integer math, no FPU. */
    uint32_t kbps = (uint32_t)(total_bytes / elapsed);
    HPDMA_TEST_LOG("stress: running=%u chan_cnt=%u size=%u loops=%u err=%u "
                   "elapsed_ms=%u total_bytes=%llu rate~%uKB/s\r\n",
                   st->running, st->chan_cnt, st->buf_size,
                   total_loops, total_errs, elapsed,
                   (unsigned long long)total_bytes, kbps);
    for (uint32_t i = 0; i < st->chan_cnt; i++) {
        HPDMA_TEST_LOG("stress: ch%u dma_id=%u loops=%u err=%u\r\n",
                       i, st->dma_id[i], st->loop_cnt[i], st->err_cnt[i]);
    }
}

static void hpdma_stress_stop(void)
{
    struct hpdma_stress_state *st = &s_hpdma_stress;
    if (!st->running && st->chan_cnt == 0) {
        HPDMA_TEST_LOG("stress: not running\r\n");
        return;
    }
    st->deiniting = 1;
    st->running = 0;

    /* After running flag drops the next ISR will return without restart;
     * give the engine a tick to drain the in-flight transfer. */
    rtos_delay_milliseconds(2);

    uint8_t chan_cnt = st->chan_cnt;
    int ok_count = 0;
    uint32_t total_loops = 0;
    uint32_t total_errs = 0;
    for (uint32_t i = 0; i < chan_cnt; i++) {
        int data_ok = (st->src_aligned[i] && st->dst_aligned[i] &&
                       hpdma_test_diff_offset(st->src_aligned[i], st->dst_aligned[i],
                                              st->buf_size) == st->buf_size);
        if (data_ok) {
            ok_count++;
        } else {
            HPDMA_TEST_ERR("stress: ch%u data MISMATCH\r\n", i);
        }
        total_loops += st->loop_cnt[i];
        total_errs += st->err_cnt[i];
    }
    uint32_t size = st->buf_size;
    uint32_t elapsed = rtos_get_time() - st->start_ms;
    if (elapsed == 0) elapsed = 1;

    hpdma_stress_cleanup();

    HPDMA_TEST_LOG("stress: stopped RESULT: %s chan_cnt=%u data_ok=%d/%u "
                   "loops=%u err=%u size=%u elapsed_ms=%u\r\n",
                   (ok_count == chan_cnt && total_errs == 0) ? "SUCCESS" : "FAILED",
                   chan_cnt, ok_count, chan_cnt, total_loops, total_errs, size, elapsed);
}

static void cli_hpdma_stress(char *pcWriteBuffer, int xWriteBufferLen,
                             int argc, char **argv)
{
    if (argc < 2) {
        CLI_LOGD("usage: hpdma stress {start|stop|status} [size_kb_dec] [chan_cnt_dec]\r\n");
        return;
    }
    if (os_strcmp(argv[1], "start") == 0) {
        uint32_t size_kb = (argc >= 3) ? os_strtoul(argv[2], NULL, 10) : 4;
        uint32_t chan_cnt = (argc >= 4) ? os_strtoul(argv[3], NULL, 10) : 1;
        if (size_kb == 0) size_kb = 4;
        if (chan_cnt == 0) chan_cnt = 1;
        (void)hpdma_stress_start_locked(size_kb * 1024U, chan_cnt);
    } else if (os_strcmp(argv[1], "stop") == 0) {
        hpdma_stress_stop();
    } else if (os_strcmp(argv[1], "status") == 0) {
        hpdma_stress_status();
    } else {
        CLI_LOGD("usage: hpdma stress {start|stop|status} [size_kb_dec] [chan_cnt_dec]\r\n");
    }
}

/* ============================================================
 * hpdma_throughput: synchronous N-iteration timed transfer.
 *   - 1 channel, 1 descriptor, sem-based wait per iter
 *   - reports total bytes, elapsed ms, KB/s, MB/s
 * ============================================================ */
static void cli_hpdma_throughput(char *pcWriteBuffer, int xWriteBufferLen,
                                 int argc, char **argv)
{
    if (argc < 3) {
        CLI_LOGD("usage: hpdma throughput {size_kb_dec} {iter_dec}\r\n");
        return;
    }
    uint32_t size_kb = os_strtoul(argv[1], NULL, 10);
    uint32_t iter = os_strtoul(argv[2], NULL, 10);
    if (size_kb == 0 || iter == 0 || size_kb > 0x3F) {
        /* Cap per-transfer at <64KB so xsize stays in 16-bit range. */
        CLI_LOGE("hpdma_throughput: 1<=size_kb<=63, iter>0\r\n");
        return;
    }
    uint32_t size = size_kb * 1024U;

    void *desc_table = NULL;
    uint8_t *src = NULL, *dst = NULL;
    beken_semaphore_t sem = NULL;
    hpdma_id_t dma_id = HPDMA_ID_MAX;

    src = (uint8_t *)hpdma_test_aligned_alloc(size, HPDMA_TEST_DEFAULT_ALIGN);
    dst = (uint8_t *)hpdma_test_aligned_alloc(size, HPDMA_TEST_DEFAULT_ALIGN);
    if (src == NULL || dst == NULL) {
        HPDMA_TEST_ERR("throughput: alloc fail (need %u x2)\r\n", size);
        goto out;
    }
    hpdma_test_fill_pattern(src, size, 0x77);

    desc_table = bk_hpdma_link_init(1);
    if (desc_table == NULL) goto out;

    hpdma_link_config_t cfg = {0};
    cfg.src_addr = (uint32_t)src;
    cfg.dst_addr = (uint32_t)dst;
    cfg.src_xsize = size;
    cfg.src_ysize = 1;
    cfg.dst_xsize = size;
    cfg.dst_ysize = 1;
    cfg.finish_int_en = 1;
    if (bk_hpdma_link_set_descs(desc_table, &cfg, 1) != BK_OK) goto out;

    if (rtos_init_semaphore(&sem, 1) != BK_OK) { sem = NULL; goto out; }
    dma_id = bk_hpdma_alloc(HPDMA_DEV_DTCM);
    if (dma_id >= HPDMA_ID_MAX) goto out;
    bk_hpdma_register_isr(dma_id, NULL, NULL,
                          hpdma_link_transfer_complete_callback, (void *)&sem);
    bk_hpdma_enable_finish_interrupt(dma_id);

    uint32_t t0 = rtos_get_time();
    int errs = 0;
    for (uint32_t i = 0; i < iter; i++) {
        if (bk_hpdma_link_transfer(dma_id, desc_table) != BK_OK) {
            errs++;
            break;
        }
        if (rtos_get_semaphore(&sem, HPDMA_TEST_TIMEOUT_MS) != BK_OK) {
            errs++;
            break;
        }
    }
    uint32_t elapsed = rtos_get_time() - t0;
    if (elapsed == 0) elapsed = 1;
    uint64_t total_bytes = (uint64_t)iter * size;
    uint32_t kbps = (uint32_t)(total_bytes / elapsed);
    uint32_t mbps_x100 = (uint32_t)((total_bytes * 100ULL) / elapsed / 1024ULL);

    HPDMA_TEST_LOG("throughput: RESULT: %s size_kb=%u iter=%u errs=%d elapsed_ms=%u "
                   "total_bytes=%llu rate=%u.%02uMB/s (%uKB/s)\r\n",
                   (errs == 0) ? "SUCCESS" : "FAILED",
                   size_kb, iter, errs, elapsed,
                   (unsigned long long)total_bytes,
                   mbps_x100 / 100, mbps_x100 % 100, kbps);

out:
    hpdma_test_release_dma_channel(&dma_id);
    if (sem) rtos_deinit_semaphore(&sem);
    if (desc_table) bk_hpdma_link_deinit(desc_table);
    if (src) hpdma_test_aligned_free(src);
    if (dst) hpdma_test_aligned_free(dst);
}

/* ============================================================
 * hpdma_neg_test: error-injection / parameter validation cases
 *   - asserts the driver REJECTS the bad input (returns != BK_OK)
 *   - prints PASS/FAIL per case + summary
 * ============================================================ */

#define HPDMA_NEG_CASE(name, expr_returns_ok)                              \
    do {                                                                   \
        int _ok = (expr_returns_ok);                                       \
        if (!_ok) {                                                        \
            HPDMA_TEST_LOG("[neg] %-28s PASS\r\n", name);                  \
            pass++;                                                        \
        } else {                                                           \
            HPDMA_TEST_ERR("[neg] %-28s FAIL\r\n", name);                  \
        }                                                                  \
        total++;                                                           \
    } while (0)

static int hpdma_test_run_neg(void)
{
    int pass = 0, total = 0;

    /* Build a known-good config, then mutate per-case. */
    hpdma_config_t good = {0};
    good.mode = HPDMA_WORK_MODE_SINGLE;
    good.chan_prio = 1;
    good.src.dev = HPDMA_DEV_DTCM;
    good.src.width = HPDMA_DATA_WIDTH_32BITS;
    good.src.addr_inc_en = HPDMA_ADDR_INC_ENABLE;
    good.src.start_addr = 0x30000000;
    good.src.xsize = 64;
    good.src.ysize = 1;
    good.dst = good.src;
    good.dst.start_addr = 0x30001000;

    /* 1. Invalid channel id. */
    {
        bk_err_t r = bk_hpdma_init(HPDMA_ID_MAX, &good);
        HPDMA_NEG_CASE("init bad_id", r == BK_OK);
    }
    /* 2. NULL config. */
    {
        bk_err_t r = bk_hpdma_init(HPDMA_ID_0, NULL);
        HPDMA_NEG_CASE("init null_cfg", r == BK_OK);
    }
    /* 3. ysize=0 (regression for the user's first error). */
    {
        hpdma_config_t bad = good;
        bad.src.ysize = 0;
        bk_err_t r = bk_hpdma_init(HPDMA_ID_0, &bad);
        HPDMA_NEG_CASE("init src_ysize_0", r == BK_OK);
        if (r == BK_OK) (void)bk_hpdma_deinit(HPDMA_ID_0);
    }
    {
        hpdma_config_t bad = good;
        bad.dst.ysize = 0;
        bk_err_t r = bk_hpdma_init(HPDMA_ID_0, &bad);
        HPDMA_NEG_CASE("init dst_ysize_0", r == BK_OK);
        if (r == BK_OK) (void)bk_hpdma_deinit(HPDMA_ID_0);
    }
    /* 4. addr_loop_en=1 with ysize!=0 (HAL guard). */
    {
        hpdma_config_t bad = good;
        bad.src.addr_loop_en = HPDMA_ADDR_LOOP_ENABLE;
        bad.src.ysize = 2;
        bk_err_t r = bk_hpdma_init(HPDMA_ID_0, &bad);
        HPDMA_NEG_CASE("init loop_with_ysize", r == BK_OK);
        if (r == BK_OK) (void)bk_hpdma_deinit(HPDMA_ID_0);
    }
    /* 5. Free a never-allocated channel. */
    {
        bk_err_t r = bk_hpdma_free(HPDMA_DEV_DTCM, HPDMA_ID_MAX);
        HPDMA_NEG_CASE("free bad_id", r == BK_OK);
    }
    /* 6. register_isr on bad id. */
    {
        bk_err_t r = bk_hpdma_register_isr(HPDMA_ID_MAX, NULL, NULL, NULL, NULL);
        HPDMA_NEG_CASE("register_isr bad_id", r == BK_OK);
    }
    /* 7. Loop mode still requires 16-byte-aligned loop window start/end. */
    {
        hpdma_config_t bad = good;
        bad.src.addr_loop_en = HPDMA_ADDR_LOOP_ENABLE;
        bad.src.start_addr = 0x30000003;
        bk_err_t r = bk_hpdma_init(HPDMA_ID_0, &bad);
        HPDMA_NEG_CASE("init loop_start_align", r == BK_OK);
        if (r == BK_OK) (void)bk_hpdma_deinit(HPDMA_ID_0);
    }
    {
        hpdma_config_t bad = good;
        bad.src.addr_loop_en = HPDMA_ADDR_LOOP_ENABLE;
        bad.src.xsize = 63;
        bad.dst.xsize = 63;
        bk_err_t r = bk_hpdma_init(HPDMA_ID_0, &bad);
        HPDMA_NEG_CASE("init loop_len_align", r == BK_OK);
        if (r == BK_OK) (void)bk_hpdma_deinit(HPDMA_ID_0);
    }
    /* 8. link_set_descs NULL desc_table. */
    {
        hpdma_link_config_t cfg = {0};
        cfg.src_xsize = 16; cfg.src_ysize = 1; cfg.dst_xsize = 16; cfg.dst_ysize = 1;
        bk_err_t r = bk_hpdma_link_set_descs(NULL, &cfg, 1);
        HPDMA_NEG_CASE("link_set_descs null_table", r == BK_OK);
    }
    /* 9. link_transfer NULL desc_table. */
    {
        bk_err_t r = bk_hpdma_link_transfer(HPDMA_ID_0, NULL);
        HPDMA_NEG_CASE("link_transfer null_table", r == BK_OK);
    }
    /* 10. link_transfer bad id. */
    {
        void *desc = bk_hpdma_link_init(1);
        if (desc) {
            bk_err_t r = bk_hpdma_link_transfer(HPDMA_ID_MAX, desc);
            HPDMA_NEG_CASE("link_transfer bad_id", r == BK_OK);
            bk_hpdma_link_deinit(desc);
        }
    }

    HPDMA_TEST_LOG("neg_test: %d/%d PASS\r\n", pass, total);
    return (pass == total) ? 0 : -1;
}

static void cli_hpdma_neg_test(char *pcWriteBuffer, int xWriteBufferLen,
                               int argc, char **argv)
{
    int rc = hpdma_test_run_neg();
    if (rc == 0) {
        HPDMA_TEST_LOG("neg_test: SUCCESS\r\n");
    } else {
        HPDMA_TEST_ERR("neg_test: FAILED\r\n");
    }
}

/* ============================================================
 * hpdma_auto: serialised test suite, CI-grep friendly output
 *   - prints "[CASE NN] name ... PASS|FAIL" per case
 *   - final line "RESULT: PASS X/Y elapsed=Nms"
 *   - optional [iter] runs the suite N rounds, summing scores
 * ============================================================ */

#define HPDMA_AUTO_CASE(name, expr_returns_zero)                           \
    do {                                                                   \
        int _r = (expr_returns_zero);                                      \
        case_idx++;                                                        \
        if (_r == 0) {                                                     \
            HPDMA_TEST_LOG("[CASE %02d] %-28s PASS\r\n", case_idx, name);  \
            pass++;                                                        \
        } else {                                                           \
            HPDMA_TEST_ERR("[CASE %02d] %-28s FAIL\r\n", case_idx, name);  \
        }                                                                  \
        total++;                                                           \
    } while (0)

static int hpdma_test_alloc_churn(uint32_t rounds)
{
    /* Allocate / free a single channel `rounds` times - exercises the bitmap. */
    for (uint32_t i = 0; i < rounds; i++) {
        hpdma_id_t id = bk_hpdma_alloc(HPDMA_DEV_DTCM);
        if (id >= HPDMA_ID_MAX) {
            HPDMA_TEST_ERR("alloc_churn: alloc fail at i=%u\r\n", i);
            return -1;
        }
        bk_err_t r = bk_hpdma_free(HPDMA_DEV_DTCM, id);
        if (r != BK_OK && r != BK_ERR_HPDMA_TIMEOUT) {
            HPDMA_TEST_ERR("alloc_churn: free fail i=%u ret=%d\r\n", i, r);
            return -1;
        }
        if (r == BK_ERR_HPDMA_TIMEOUT) {
            (void)bk_hpdma_force_reclaim(HPDMA_DEV_DTCM, id);
        }
    }
    return 0;
}

static int hpdma_test_memcpy_check(uint32_t len)
{
    int rc = -1;
    uint8_t *src = (uint8_t *)hpdma_test_aligned_alloc(len, HPDMA_TEST_DEFAULT_ALIGN);
    uint8_t *dst = (uint8_t *)hpdma_test_aligned_alloc(len, HPDMA_TEST_DEFAULT_ALIGN);
    if (src == NULL || dst == NULL) goto out;
    hpdma_test_fill_pattern(src, len, 0xBE);
    os_memset(dst, 0, len);
    if (bk_hpdma_memcpy(dst, src, len) != BK_OK) goto out;
    if (hpdma_test_diff_offset(src, dst, len) == len) {
        rc = 0;
    }
out:
    if (src) hpdma_test_aligned_free(src);
    if (dst) hpdma_test_aligned_free(dst);
    return rc;
}

static void cli_hpdma_auto(char *pcWriteBuffer, int xWriteBufferLen,
                           int argc, char **argv)
{
    uint32_t outer_iter = 1;
    if (argc >= 2) {
        outer_iter = os_strtoul(argv[1], NULL, 10);
        if (outer_iter == 0) outer_iter = 1;
    }

    uint32_t total_pass = 0, total_total = 0;
    uint32_t round_pass[8] = {0};
    uint32_t t0 = rtos_get_time();

    for (uint32_t round = 0; round < outer_iter; round++) {
        int case_idx = 0, pass = 0, total = 0;

        HPDMA_TEST_LOG("==== auto round %u/%u start ====\r\n",
                       round + 1, outer_iter);

        HPDMA_AUTO_CASE("link_1d small",      hpdma_test_run_link_1d(2, 1024));
        HPDMA_AUTO_CASE("link_1d medium",     hpdma_test_run_link_1d(4, 4096));
        HPDMA_AUTO_CASE("link_1d single_big", hpdma_test_run_link_1d(1, 16384));

        HPDMA_AUTO_CASE("link_2d basic",      hpdma_test_run_link_2d(1, 256, 8, 256));
        HPDMA_AUTO_CASE("link_2d strided",    hpdma_test_run_link_2d(1, 128, 8, 256));
        HPDMA_AUTO_CASE("link_2d multilink",  hpdma_test_run_link_2d(2, 64, 4, 64));
        HPDMA_AUTO_CASE("link_2d tiny_multi", hpdma_test_run_link_2d(2, 8, 8, 8));

        HPDMA_AUTO_CASE("memcpy 256B",        hpdma_test_memcpy_check(256));
        HPDMA_AUTO_CASE("memcpy 4KB",         hpdma_test_memcpy_check(4096));
        HPDMA_AUTO_CASE("memcpy 16KB",        hpdma_test_memcpy_check(16384));

        HPDMA_AUTO_CASE("alloc_churn 100x",   hpdma_test_alloc_churn(100));

        HPDMA_AUTO_CASE("neg_test all",       hpdma_test_run_neg());

        HPDMA_TEST_LOG("==== auto round %u: %d/%d PASS ====\r\n",
                       round + 1, pass, total);
        if (round < (uint32_t)(sizeof(round_pass)/sizeof(round_pass[0]))) {
            round_pass[round] = ((uint32_t)pass << 16) | (uint32_t)total;
        }
        total_pass += (uint32_t)pass;
        total_total += (uint32_t)total;
    }
    uint32_t elapsed = rtos_get_time() - t0;
    const char *verdict = (total_pass == total_total) ? "PASS" : "FAIL";
    HPDMA_TEST_LOG("RESULT: %s %u/%u elapsed=%ums rounds=%u\r\n",
                   verdict, total_pass, total_total, elapsed, outer_iter);
}

static void cli_hpdma_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc < 2 || os_strcmp(argv[1], "help") == 0) {
        cli_hpdma_help();
        return;
    }

    if (os_strcmp(argv[1], "driver") == 0) {
        cli_hpdma_driver_cmd(pcWriteBuffer, xWriteBufferLen, argc - 1, argv + 1);
    } else if (os_strcmp(argv[1], "chan") == 0) {
        cli_hpdma_chan_cmd(pcWriteBuffer, xWriteBufferLen, argc - 1, argv + 1);
    } else if (os_strcmp(argv[1], "int") == 0) {
        cli_hpdma_int_cmd(pcWriteBuffer, xWriteBufferLen, argc - 1, argv + 1);
    } else if (os_strcmp(argv[1], "chnl") == 0) {
        cli_hpdma_chnl_alloc(pcWriteBuffer, xWriteBufferLen, argc - 1, argv + 1);
    } else if (os_strcmp(argv[1], "chnl_free") == 0) {
        cli_hpdma_chnl_free(pcWriteBuffer, xWriteBufferLen, argc - 1, argv + 1);
    } else if (os_strcmp(argv[1], "copy") == 0) {
        cli_hpdma_copy(pcWriteBuffer, xWriteBufferLen, argc - 1, argv + 1);
    } else if (os_strcmp(argv[1], "link_test_1d") == 0) {
        cli_hpdma_link_test_1d(pcWriteBuffer, xWriteBufferLen, argc - 1, argv + 1);
    } else if (os_strcmp(argv[1], "link_test_2d") == 0) {
        cli_hpdma_link_test_2d(pcWriteBuffer, xWriteBufferLen, argc - 1, argv + 1);
    } else if (os_strcmp(argv[1], "concurrent_test") == 0) {
        cli_hpdma_concurrent_test(pcWriteBuffer, xWriteBufferLen, argc - 1, argv + 1);
    } else if (os_strcmp(argv[1], "throughput") == 0) {
        cli_hpdma_throughput(pcWriteBuffer, xWriteBufferLen, argc - 1, argv + 1);
    } else if (os_strcmp(argv[1], "stress") == 0) {
        cli_hpdma_stress(pcWriteBuffer, xWriteBufferLen, argc - 1, argv + 1);
    } else if (os_strcmp(argv[1], "neg_test") == 0) {
        cli_hpdma_neg_test(pcWriteBuffer, xWriteBufferLen, argc - 1, argv + 1);
    } else if (os_strcmp(argv[1], "auto") == 0) {
        cli_hpdma_auto(pcWriteBuffer, xWriteBufferLen, argc - 1, argv + 1);
    } else {
        cli_hpdma_help();
    }
}

DRV_CLI_CMD_EXPORT static const struct cli_command s_hpdma_commands[] = {
    {"hpdma", "hpdma help | hpdma {driver|chan|int|chnl|chnl_free|copy|link_test_1d|link_test_2d|concurrent_test|throughput|stress|neg_test|auto} ...", cli_hpdma_cmd},
};

