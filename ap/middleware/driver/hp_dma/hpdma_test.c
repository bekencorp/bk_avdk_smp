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
        .xsize = 0,
        .ysize = 0,
        .step = 0,
    },
    .dst = {
        .dev = HPDMA_DEV_DTCM,
        .width = HPDMA_DATA_WIDTH_32BITS,
        .addr_inc_en = HPDMA_ADDR_INC_ENABLE,
        .addr_loop_en = HPDMA_ADDR_LOOP_DISABLE,
        .start_addr = 0,
        // .end_addr = 0,
        .xsize = 0,
        .ysize = 0,
        .step = 0,
    },
    .trans_type = HPDMA_TRANS_DEFAULT,
};

static uint32_t *s_cli_hpdma_src_buf_p;
static uint32_t *s_cli_hpdma_dst_buf_p;

static void cli_hpdma_help(void)
{
    CLI_LOGD("hpdma_driver {init|deinit}\n");
    CLI_LOGD("hpdma {id} {init|deinit|start|stop}\n");
    CLI_LOGD("hpdma_int {id} {reg|enable|disable}\n");
    CLI_LOGD("hpdma_chnl alloc \n");
    CLI_LOGD("hpdma_chnl_free free {id} \n");
    CLI_LOGD("hpdma_memcopy_test {copy} {count|in_number1|in_number2|out_number1|out_number2} (numbers in hex)\r\n");
    CLI_LOGD("hpdma_link_test_1d {link_cnt} {trans_len}\r\n");
    CLI_LOGD("hpdma_link_test_2d {link_cnt} {xsize} {ysize} {step}\r\n");
    CLI_LOGD("hpdma_config {mode|priority|src|dst}{mode value/priority value/dev,width,increase_en,loop_en,start_addr,end_addr,xsize,ysize,step}\r\n");
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

static void cli_hpdma_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t id;

    if (argc < 2) {
        cli_hpdma_help();
        return;
    }

    id = os_strtoul(argv[1], NULL, 10);

    if (os_strcmp(argv[2], "init") == 0) {
        BK_LOG_ON_ERR(bk_hpdma_init(id, &s_cli_hpdma_cfg));
        CLI_LOGD("hpdma init, id=%d\n", id);
    } else if (os_strcmp(argv[2], "start") == 0) {
#if (CONFIG_SPE)
        bk_hpdma_set_src_sec_attr(id, HPDMA_ATTR_SEC);
        bk_hpdma_set_dest_sec_attr(id, HPDMA_ATTR_SEC);
#endif
        BK_LOG_ON_ERR(bk_hpdma_start(id));
        CLI_LOGD("hpdma start, id=%d\n", id);
    } else if (os_strcmp(argv[2], "stop") == 0) {
        BK_LOG_ON_ERR(bk_hpdma_stop(id));
        CLI_LOGD("hpdma stop, id=%d\n", id);
    } else if (os_strcmp(argv[2], "deinit") == 0) {
        if(s_cli_hpdma_src_buf_p) {
            os_free(s_cli_hpdma_src_buf_p);
            s_cli_hpdma_src_buf_p = 0;
        }
        if(s_cli_hpdma_dst_buf_p) {
            os_free(s_cli_hpdma_dst_buf_p);
            s_cli_hpdma_dst_buf_p = 0;
        }
        os_memset(&s_cli_hpdma_cfg, 0, sizeof(s_cli_hpdma_cfg));

        BK_LOG_ON_ERR(bk_hpdma_deinit(id));
        CLI_LOGD("hpdma deinit, id=%d\n", id);
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

    if (argc < 2) {
        cli_hpdma_help();
        return;
    }

    if (os_strcmp(argv[1], "free") == 0) {
        id = os_strtoul(argv[2], NULL, 10);
        ret = bk_hpdma_free(HPDMA_DEV_DTCM, id);
        CLI_LOGD("hpdma channel free id:%d ret:%d\n", id, ret);
    } else {
        CLI_LOGD("cli_hpdma_chnl_free NOT free\n");
        cli_hpdma_help();
        return;
    }
}

static void cli_hpdma_copy(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc < 4) {
        CLI_LOGD("hpdma_copy {src} {dst} {len}\r\n");
        return;
    }

    uint32_t src = os_strtoul(argv[1], NULL, 16);
    uint32_t dst = os_strtoul(argv[2], NULL, 16);
    uint32_t len = os_strtoul(argv[3], NULL, 10);
    
    // Ensure 128-bit (16-byte) alignment
    src = (src + 15) & ~15;
    dst = (dst + 15) & ~15;
    len = (len + 15) & ~15;  // Align length to 16 bytes
    
    bk_hpdma_memcpy((void*)dst, (const void*)src, len);
    CLI_LOGD("hpdma_copy: src=0x%x dst=0x%x len=%d\r\n", src, dst, len);
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
    CLI_LOGD("hpdma link transfer callback(%d)\n", hpdma_id);
    
    // Release semaphore to notify transfer completion
    // user_data points to the semaphore handle (beken_semaphore_t*)
    if (user_data != NULL) {
        beken_semaphore_t *sem_ptr = (beken_semaphore_t *)user_data;
        rtos_set_semaphore(sem_ptr);
        CLI_LOGD("Semaphore released in callback\n");
    }
}

static void cli_hpdma_link_test_1d(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc < 3) {
        cli_hpdma_help();
        return;
    }

    uint32_t link_cnt = os_strtoul(argv[1], NULL, 10);
    uint32_t trans_len = os_strtoul(argv[2], NULL, 10);
    
    // Ensure 128-bit alignment
    trans_len = (trans_len + 15) & ~15;
    
    CLI_LOGD("hpdma_link_test_1d: link_cnt=%d trans_len=%d\r\n", link_cnt, trans_len);
    
    // Allocate source and destination buffers
    uint32_t total_len = link_cnt * trans_len;
    uint8_t *src_buf = (uint8_t *)os_malloc(total_len + 16);
    uint8_t *dst_buf = (uint8_t *)os_malloc(total_len + 16);
    
    if (src_buf == NULL || dst_buf == NULL) {
        CLI_LOGE("Failed to allocate buffers\r\n");
        return;
    }
    
    // Align buffers to 16-byte boundary
    uint8_t *src_aligned = (uint8_t *)(((uintptr_t)src_buf + 15) & ~15);
    uint8_t *dst_aligned = (uint8_t *)(((uintptr_t)dst_buf + 15) & ~15);
    
    // Initialize source buffer with test pattern
    for (uint32_t i = 0; i < total_len; i++) {
        src_aligned[i] = (uint8_t)(i & 0xFF);
    }
    os_memset(dst_aligned, 0, total_len);
    
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
        configs[i].src_addr = (uint32_t)(src_aligned + i * trans_len);
        configs[i].dst_addr = (uint32_t)(dst_aligned + i * trans_len);
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
        bk_hpdma_free(HPDMA_DEV_DTCM, dma_id);
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
        rtos_deinit_semaphore(&transfer_sem);
        bk_hpdma_free(HPDMA_DEV_DTCM, dma_id);
        bk_hpdma_link_deinit(desc_table);
        os_free(src_buf);
        os_free(dst_buf);
        return;
    }
    
    CLI_LOGD("Transfer completed, verifying data...\r\n");
    
    // Verify data consistency after getting semaphore
    if (cli_hpdma_compare_buffer(src_aligned, dst_aligned, total_len) == 0) {
        CLI_LOGD("hpdma_link_test_1d: SUCCESS\r\n");
    } else {
        CLI_LOGE("hpdma_link_test_1d: FAILED - Data mismatch\r\n");
        // Find and print first few mismatches for debugging
        uint32_t mismatch_count = 0;
        uint32_t max_print = (total_len < 32 ? total_len : 32);
        for (uint32_t i = 0; i < total_len && mismatch_count < max_print; i++) {
            if (src_aligned[i] != dst_aligned[i]) {
                CLI_LOGE("Mismatch at offset %d: src=0x%02x dst=0x%02x\r\n", i, src_aligned[i], dst_aligned[i]);
                mismatch_count++;
            }
        }
        if (mismatch_count == 0) {
            CLI_LOGE("No mismatch found in first %d bytes, but comparison failed\r\n", max_print);
        }
    }
    
    // Cleanup
    rtos_deinit_semaphore(&transfer_sem);
    bk_hpdma_free(HPDMA_DEV_DTCM, dma_id);  // Free DMA channel allocated by application
    bk_hpdma_link_deinit(desc_table);
    os_free(src_buf);
    os_free(dst_buf);
}


#define HPDMA_CMD_CNT (sizeof(s_hpdma_commands) / sizeof(struct cli_command))
DRV_CLI_CMD_EXPORT static const struct cli_command s_hpdma_commands[] = {
    {"hpdma_driver", "hpdma_driver {init|deinit}", cli_hpdma_driver_cmd},
    {"hpdma", "hpdma {id} {init|deinit|start|stop|get_remain_len}", cli_hpdma_cmd},
    {"hpdma_int", "hpdma_int {id} {reg|enable_hf_fini|disable_hf_fini|enable_fini|disable_fini}", cli_hpdma_int_cmd},
    {"hpdma_chnl", "hpdma_chnl alloc", cli_hpdma_chnl_alloc},
    {"hpdma_chnl_free", "hpdma_chnl_free {id}", cli_hpdma_chnl_free},
    {"hpdma_link_test_1d", "hpdma_link_test_1d {link_cnt} {trans_len}", cli_hpdma_link_test_1d},
    {"hpdma_copy", "copy {src} {dst} {len}", cli_hpdma_copy},
};

int bk_hpdma_register_cli_test_feature(void)
{
    BK_LOG_ON_ERR(bk_hpdma_driver_init());
    return cli_register_commands(s_hpdma_commands, HPDMA_CMD_CNT);
}

