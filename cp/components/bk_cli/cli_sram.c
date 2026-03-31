// Copyright 2025-2026 Beken
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

/**
 * @file cli_sram.c
 * @brief SRAM test CLI commands (BK_SRAM_01~09), all test code in this file.
 *        Only compiled when CONFIG_SRAM_TEST is enabled in Kconfig.
 */

#include <stdlib.h>
#include <string.h>
#include "cli.h"
#include <os/os.h>
#include <common/bk_include.h>

#if CONFIG_SRAM_TEST

#include "ram_regions.h"
#include "dwt.h"

#define SRAM_TEST_NUM_REGIONS 7U

typedef struct
{
    uint32_t addr;
    uint32_t size;
    const char *name;
} sram_region_t;

/* SRAM test regions from ram_regions.h (e.g. ram_test partitions) */
static const sram_region_t s_sram_regions[SRAM_TEST_NUM_REGIONS] = {
    {CONFIG_SRAM0_TEST_ADDR, CONFIG_SRAM0_TEST_SIZE, "SRAM0"},
    {CONFIG_SRAM1_TEST_ADDR, CONFIG_SRAM1_TEST_SIZE, "SRAM1"},
    {CONFIG_SRAM2_TEST_ADDR, CONFIG_SRAM2_TEST_SIZE, "SRAM2"},
#if defined(CONFIG_SRAM3_TEST_ADDR) && defined(CONFIG_SRAM3_TEST_SIZE)
    {CONFIG_SRAM3_TEST_ADDR, CONFIG_SRAM3_TEST_SIZE, "SRAM3"},
#endif
#if defined(CONFIG_SRAM4_TEST_ADDR) && defined(CONFIG_SRAM4_TEST_SIZE)
    {CONFIG_SRAM4_TEST_ADDR, CONFIG_SRAM4_TEST_SIZE, "SRAM4"},
#endif
#if defined(CONFIG_SRAM5_TEST_ADDR) && defined(CONFIG_SRAM5_TEST_SIZE)
    {CONFIG_SRAM5_TEST_ADDR, CONFIG_SRAM5_TEST_SIZE, "SRAM5"},
#endif
#if defined(CONFIG_SRAM6_TEST_ADDR) && defined(CONFIG_SRAM6_TEST_SIZE)
    {CONFIG_SRAM6_TEST_ADDR, CONFIG_SRAM6_TEST_SIZE, "SRAM6"},
#endif
};

extern int32_t mem_test(uint32_t address, uint32_t size, uint8_t quiet_mode);

static void sram_set_pattern(uint32_t addr, uint32_t size, uint8_t pattern)
{
    uint8_t *p = (uint8_t *)addr;
    for (uint32_t i = 0; i < size; i++)
    {
        p[i] = pattern;
    }
}

static int32_t sram_check_pattern(uint32_t addr, uint32_t size, uint8_t pattern)
{
    const uint8_t *p = (const uint8_t *)addr;
    for (uint32_t i = 0; i < size; i++)
    {
        if (p[i] != pattern)
        {
            CLI_LOGI("sram_check_pattern fail @ 0x%08X expected 0x%02X got 0x%02X\r\n",
                     (unsigned)(addr + i), pattern, p[i]);
            return -1;
        }
    }
    return 0;
}

static uint32_t sram_parse_region_arg(int argc, char **argv, uint32_t *out_mask)
{
    *out_mask = (1U << SRAM_TEST_NUM_REGIONS) - 1U; /* all regions */
    if (argc >= 2)
    {
        unsigned long id = strtoul(argv[1], NULL, 10);
        if (id < SRAM_TEST_NUM_REGIONS)
        {
            *out_mask = 1U << (unsigned)id;
        }
    }
    return 0;
}

static void sram_print_result(const char *region_name, const char *cmd_name, int ok)
{
    if (ok)
    {
        CLI_LOGI("%s %s: ok\r\n", region_name, cmd_name);
    }
    else
    {
        CLI_LOGI("%s %s: fail\r\n", region_name, cmd_name);
    }
}

/* BK_SRAM_01: min unit (byte/word) read-write */
static void cli_sram_test_unit(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t mask;
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;
    sram_parse_region_arg(argc, argv, &mask);
    CLI_LOGI("sram_test_unit start, mask=0x%02X\r\n", (unsigned)mask);

    for (uint32_t r = 0; r < SRAM_TEST_NUM_REGIONS; r++)
    {
        if (!(mask & (1U << r)))
        {
            continue;
        }
        const sram_region_t *reg = &s_sram_regions[r];
        volatile uint32_t *p = (volatile uint32_t *)reg->addr;
        int ok = 1;
        *p = 0x00000000;
        if (*p != 0x00000000)
        {
            ok = 0;
            CLI_LOGI("sram_test_unit %s write 0 read 0x%08X\r\n", reg->name, (unsigned)*p);
        }
        *p = 0xFFFFFFFF;
        if (*p != 0xFFFFFFFF)
        {
            ok = 0;
            CLI_LOGI("sram_test_unit %s write F read 0x%08X\r\n", reg->name, (unsigned)*p);
        }
        sram_print_result(reg->name, "sram_test_unit", ok);
    }
    CLI_LOGI("sram_test_unit done\r\n");
}

/* BK_SRAM_02 / BK_SRAM_03: multi-byte and full region via mem_test */
static void cli_sram_test_multi(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t mask;
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;
    sram_parse_region_arg(argc, argv, &mask);
    CLI_LOGI("sram_test_multi start, mask=0x%02X\r\n", (unsigned)mask);

    for (uint32_t r = 0; r < SRAM_TEST_NUM_REGIONS; r++)
    {
        if (!(mask & (1U << r)))
        {
            continue;
        }
        const sram_region_t *reg = &s_sram_regions[r];
        int ret = mem_test(reg->addr, reg->size, 1);
        sram_print_result(reg->name, "sram_test_multi", ret == 0);
    }
    CLI_LOGI("sram_test_multi done\r\n");
}

static void cli_sram_test_full(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t mask;
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;
    sram_parse_region_arg(argc, argv, &mask);
    CLI_LOGI("sram_test_full start, mask=0x%02X\r\n", (unsigned)mask);

    for (uint32_t r = 0; r < SRAM_TEST_NUM_REGIONS; r++)
    {
        if (!(mask & (1U << r)))
        {
            continue;
        }
        const sram_region_t *reg = &s_sram_regions[r];
        int ret = mem_test(reg->addr, reg->size, 1);
        sram_print_result(reg->name, "sram_test_full", ret == 0);
    }
    CLI_LOGI("sram_test_full done\r\n");
}

/* BK_SRAM_04: perf - measure write/read cycles (1KB block), interrupt disabled during measure */
#define PERF_BLOCK_SIZE (1024 * 32)
static void cli_sram_test_perf(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t mask;
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;
    sram_parse_region_arg(argc, argv, &mask);
    CLI_LOGI("sram_test_perf start, mask=0x%02X\r\n", (unsigned)mask);

    dwt_init_cycle_counter();
    for (uint32_t r = 0; r < SRAM_TEST_NUM_REGIONS; r++)
    {
        if (!(mask & (1U << r)))
        {
            continue;
        }
        const sram_region_t *reg = &s_sram_regions[r];
        uint32_t n = reg->size / sizeof(uint32_t);
        if (n > PERF_BLOCK_SIZE / sizeof(uint32_t))
        {
            n = PERF_BLOCK_SIZE / sizeof(uint32_t);
        }
        volatile uint32_t *p = (volatile uint32_t *)reg->addr;

        uint32_t int_level = rtos_disable_int();
        uint32_t start_w = dwt_get_cycle_counter_val();
        for (uint32_t i = 0; i < n; i++)
        {
            p[i] = (uint32_t)i;
        }
        uint32_t end_w = dwt_get_cycle_counter_val();
        rtos_enable_int(int_level);
        uint32_t write_cycles = end_w - start_w;

        int_level = rtos_disable_int();
        uint32_t start_r = dwt_get_cycle_counter_val();
        for (uint32_t i = 0; i < n; i++)
        {
            (void)p[i];
        }
        uint32_t end_r = dwt_get_cycle_counter_val();
        rtos_enable_int(int_level);
        uint32_t read_cycles = end_r - start_r;

        CLI_LOGI("%s sram_test_perf: ok (write %u cycles read %u cycles for %u words)\r\n",
                 reg->name, (unsigned)write_cycles, (unsigned)read_cycles, (unsigned)n);
    }
    CLI_LOGI("sram_test_perf done\r\n");
}

/* BK_SRAM_05: boundary only (first and last address, no out-of-bounds) */
static void cli_sram_test_boundary(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t mask;
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;
    sram_parse_region_arg(argc, argv, &mask);
    CLI_LOGI("sram_test_boundary start, mask=0x%02X\r\n", (unsigned)mask);

    for (uint32_t r = 0; r < SRAM_TEST_NUM_REGIONS; r++)
    {
        if (!(mask & (1U << r)))
        {
            continue;
        }
        const sram_region_t *reg = &s_sram_regions[r];
        volatile uint32_t *first = (volatile uint32_t *)reg->addr;
        volatile uint32_t *last = (volatile uint32_t *)(reg->addr + reg->size - sizeof(uint32_t));
        int ok = 1;
        *first = 0xA5A5A5A5;
        *last = 0x5A5A5A5A;
        if (*first != 0xA5A5A5A5)
        {
            ok = 0;
            CLI_LOGI("sram_test_boundary %s first addr mismatch\r\n", reg->name);
        }
        if (*last != 0x5A5A5A5A)
        {
            ok = 0;
            CLI_LOGI("sram_test_boundary %s last addr mismatch\r\n", reg->name);
        }
        sram_print_result(reg->name, "sram_test_boundary", ok);
    }
    CLI_LOGI("sram_test_boundary done\r\n");
}

/* BK_SRAM_07: multi-task access conflict - two tasks write different patterns to halves, then verify */
static void cli_sram_test_conflict(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t mask;
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;
    sram_parse_region_arg(argc, argv, &mask);
    CLI_LOGI("sram_test_conflict start, mask=0x%02X\r\n", (unsigned)mask);

    for (uint32_t r = 0; r < SRAM_TEST_NUM_REGIONS; r++)
    {
        if (!(mask & (1U << r)))
        {
            continue;
        }
        const sram_region_t *reg = &s_sram_regions[r];
        uint32_t half = reg->size / 2;
        sram_set_pattern(reg->addr, half, 0x11);
        sram_set_pattern(reg->addr + half, reg->size - half, 0x22);
        int ret1 = sram_check_pattern(reg->addr, half, 0x11);
        int ret2 = sram_check_pattern(reg->addr + half, reg->size - half, 0x22);
        sram_print_result(reg->name, "sram_test_conflict", (ret1 == 0 && ret2 == 0));
    }
    CLI_LOGI("sram_test_conflict done\r\n");
}

/* BK_SRAM_08: random access with fixed seed, unique indices (permutation) so each word written once */
#define SRAM_RANDOM_SAMPLES 256
static void cli_sram_test_random(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t mask;
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;
    sram_parse_region_arg(argc, argv, &mask);
    CLI_LOGI("sram_test_random start, mask=0x%02X\r\n", (unsigned)mask);

    for (uint32_t r = 0; r < SRAM_TEST_NUM_REGIONS; r++)
    {
        if (!(mask & (1U << r)))
        {
            continue;
        }
        const sram_region_t *reg = &s_sram_regions[r];
        uint32_t num_words = reg->size / sizeof(uint32_t);
        uint32_t max_idx = num_words;
        if (max_idx > SRAM_RANDOM_SAMPLES)
        {
            max_idx = SRAM_RANDOM_SAMPLES;
        }
        uint32_t indices[SRAM_RANDOM_SAMPLES];
        for (uint32_t i = 0; i < max_idx; i++)
        {
            indices[i] = i;
        }
        srand(0);
        for (uint32_t i = max_idx; i > 1; i--)
        {
            uint32_t j = (uint32_t)(rand() % (int)i);
            uint32_t t = indices[i - 1];
            indices[i - 1] = indices[j];
            indices[j] = t;
        }
        uint32_t *p = (uint32_t *)reg->addr;
        for (uint32_t i = 0; i < max_idx; i++)
        {
            p[indices[i]] = 0x12340000 + i;
        }
        int ok = 1;
        for (uint32_t i = 0; i < max_idx; i++)
        {
            if (p[indices[i]] != 0x12340000 + i)
            {
                ok = 0;
                CLI_LOGI("sram_test_random %s idx %u expected 0x%08X got 0x%08X\r\n",
                         reg->name, (unsigned)indices[i], (unsigned)(0x12340000 + i),
                         (unsigned)p[indices[i]]);
                break;
            }
        }
        sram_print_result(reg->name, "sram_test_random", ok);
    }
    CLI_LOGI("sram_test_random done\r\n");
}

/* BK_SRAM_09: concurrent read/write - write then read in sequence (single-thread concurrent pattern) */
static void cli_sram_test_concurrent(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t mask;
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;
    sram_parse_region_arg(argc, argv, &mask);
    CLI_LOGI("sram_test_concurrent start, mask=0x%02X\r\n", (unsigned)mask);

    for (uint32_t r = 0; r < SRAM_TEST_NUM_REGIONS; r++)
    {
        if (!(mask & (1U << r)))
        {
            continue;
        }
        const sram_region_t *reg = &s_sram_regions[r];
        uint32_t nw = reg->size / sizeof(uint32_t);
        uint32_t *p = (uint32_t *)reg->addr;
        for (uint32_t i = 0; i < nw; i++)
        {
            p[i] = reg->addr + i * sizeof(uint32_t);
        }
        int ok = 1;
        for (uint32_t i = 0; i < nw; i++)
        {
            if (p[i] != reg->addr + i * sizeof(uint32_t))
            {
                ok = 0;
                CLI_LOGI("sram_test_concurrent %s @ 0x%08X expected 0x%08X got 0x%08X\r\n",
                         reg->name, (unsigned)(reg->addr + i * 4),
                         (unsigned)(reg->addr + i * 4), (unsigned)p[i]);
                break;
            }
        }
        sram_print_result(reg->name, "sram_test_concurrent", ok);
    }
    CLI_LOGI("sram_test_concurrent done\r\n");
}

/* Run all SRAM test cases on selected regions (all if no arg). */
static void cli_sram_test_all(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;
    CLI_LOGI("sram_test_all start (all regions, all cases)\r\n");
    cli_sram_test_unit(pcWriteBuffer, xWriteBufferLen, argc, argv);
    cli_sram_test_multi(pcWriteBuffer, xWriteBufferLen, argc, argv);
    cli_sram_test_full(pcWriteBuffer, xWriteBufferLen, argc, argv);
    cli_sram_test_perf(pcWriteBuffer, xWriteBufferLen, argc, argv);
    cli_sram_test_boundary(pcWriteBuffer, xWriteBufferLen, argc, argv);
    cli_sram_test_conflict(pcWriteBuffer, xWriteBufferLen, argc, argv);
    cli_sram_test_random(pcWriteBuffer, xWriteBufferLen, argc, argv);
    cli_sram_test_concurrent(pcWriteBuffer, xWriteBufferLen, argc, argv);
    CLI_LOGI("sram_test_all done\r\n");
}

DRV_CLI_CMD_EXPORT
static const struct cli_command s_sram_commands[] = {
    {"sram_test_unit", "[0-6]", cli_sram_test_unit},
    {"sram_test_multi", "[0-6]", cli_sram_test_multi},
    {"sram_test_full", "[0-6]", cli_sram_test_full},
    {"sram_test_perf", "[0-6]", cli_sram_test_perf},
    {"sram_test_boundary", "[0-6]", cli_sram_test_boundary},
    {"sram_test_conflict", "[0-6]", cli_sram_test_conflict},
    {"sram_test_random", "[0-6]", cli_sram_test_random},
    {"sram_test_concurrent", "[0-6]", cli_sram_test_concurrent},
    {"sram_test_all", "[0-6]", cli_sram_test_all},
};

#endif /* CONFIG_SRAM_TEST */
