// Copyright 2026 Beken
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
 * @file cli_sram_bus_bench.c
 * @brief Isolated CPU SRAM latency/throughput microbenchmark.
 *
 * This benchmark deliberately does not share implementation with CoreMark or
 * cli_sram.c. It uses only SRAM regions reserved by ram_regions.csv.
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "cli.h"
#include <os/os.h>
#include <common/bk_include.h>

#if CONFIG_SRAM_TEST

#include "ram_regions.h"
#include "dwt.h"
#include "sys_driver.h"
#include <driver/hpdma.h>
#include <modules/pm.h>
#include <soc/soc.h>

#define SRAM_BUS_BENCH_DEFAULT_BYTES       (64U * 1024U)
#define SRAM_BUS_BENCH_MIN_BYTES           1024U
#define SRAM_BUS_BENCH_DEFAULT_REPEATS     16U
#define SRAM_BUS_BENCH_MAX_REPEATS         128U
#define SRAM_BUS_BENCH_CPU_MHZ             480U
#define SRAM_BUS_BENCH_LCG_A               1664525U
#define SRAM_BUS_BENCH_LCG_C               1013904223U
#define SRAM_BUS_BENCH_HPDMA_MAX_BYTES     0xF000U

typedef struct {
    uint32_t addr;
    uint32_t size;
    const char *name;
} sram_bus_region_t;

typedef struct sram_bus_node {
    struct sram_bus_node *next;
    uint32_t value;
} sram_bus_node_t;

static volatile uintptr_t s_sram_bus_sink;

/*
 * CONFIG_* addresses use the peripheral-visible 0x28 alias. The AP CPU must
 * access SRAM through SOC_SRAM_CPU_ADDR(), which maps it to the 0x2c alias
 * when CONFIG_SRAM_DIRECT_ADDR is enabled.
 */
static const sram_bus_region_t s_sram_bus_regions[] = {
#if defined(CONFIG_SRAM3_TEST_ADDR) && defined(CONFIG_SRAM3_TEST_SIZE)
    {CONFIG_SRAM3_TEST_ADDR, CONFIG_SRAM3_TEST_SIZE, "SRAM3"},
#endif
#if defined(CONFIG_SRAM4_TEST_ADDR) && defined(CONFIG_SRAM4_TEST_SIZE)
    {CONFIG_SRAM4_TEST_ADDR, CONFIG_SRAM4_TEST_SIZE, "SRAM4"},
#endif
#if defined(CONFIG_SRAM5_TEST_ADDR) && defined(CONFIG_SRAM5_TEST_SIZE)
    {CONFIG_SRAM5_TEST_ADDR, CONFIG_SRAM5_TEST_SIZE, "SRAM5_LOW"},
#endif
};

#define SRAM_BUS_BENCH_REGION_COUNT \
    ((uint32_t)(sizeof(s_sram_bus_regions) / sizeof(s_sram_bus_regions[0])))

static inline void sram_bus_barrier(void)
{
    __asm volatile("dsb sy" ::: "memory");
}

static uint32_t sram_bus_power_of_two_floor(uint32_t value)
{
    uint32_t result = 1U;

    while ((result <= (UINT32_MAX >> 1)) && ((result << 1) <= value)) {
        result <<= 1;
    }

    return result;
}

static int sram_bus_parse_u32(const char *text, uint32_t min_value,
                              uint32_t max_value, uint32_t *value)
{
    char *end = NULL;
    unsigned long parsed;

    if ((text == NULL) || (value == NULL)) {
        return -1;
    }

    parsed = strtoul(text, &end, 0);
    if ((end == text) || (*end != '\0') ||
        (parsed < min_value) || (parsed > max_value)) {
        return -1;
    }

    *value = (uint32_t)parsed;
    return 0;
}

static void sram_bus_print_usage(void)
{
    CLI_LOGI("usage: sram_bus_bench [all|region_id] [bytes] [repeats]\r\n");
    CLI_LOGI("  bytes: %u..%u, rounded down to power-of-two\r\n",
             (unsigned)SRAM_BUS_BENCH_MIN_BYTES,
             (unsigned)SRAM_BUS_BENCH_DEFAULT_BYTES);
    CLI_LOGI("  repeats: 1..%u, default %u\r\n",
             (unsigned)SRAM_BUS_BENCH_MAX_REPEATS,
             (unsigned)SRAM_BUS_BENCH_DEFAULT_REPEATS);
    for (uint32_t i = 0; i < SRAM_BUS_BENCH_REGION_COUNT; ++i) {
        CLI_LOGI("  %u: %s peripheral=0x%08x cpu=0x%08x size=%u\r\n",
                 (unsigned)i, s_sram_bus_regions[i].name,
                 (unsigned)s_sram_bus_regions[i].addr,
                 (unsigned)SOC_SRAM_CPU_ADDR(s_sram_bus_regions[i].addr),
                 (unsigned)s_sram_bus_regions[i].size);
    }
}

static uint32_t sram_bus_measure_overhead(void)
{
    uint32_t start;
    uint32_t end;

    sram_bus_barrier();
    start = dwt_get_cycle_counter_val();
    sram_bus_barrier();
    end = dwt_get_cycle_counter_val();

    return end - start;
}

__attribute__((noinline))
static uint32_t sram_bus_seq_write(volatile uint32_t *buffer,
                                   uint32_t words, uint32_t repeats)
{
    volatile uint32_t *dst;
    uint32_t remaining;
    uint32_t value;
    uint32_t repeat = 0U;
    uint32_t start;
    uint32_t end;

    sram_bus_barrier();
    start = dwt_get_cycle_counter_val();
    __asm volatile(
        "cmp %[words], #0\n"
        "beq 3f\n"
        "cmp %[repeats], #0\n"
        "beq 3f\n"
        "1:\n"
        "mov %[dst], %[base]\n"
        "mov %[remaining], %[words]\n"
        "mov %[value], %[repeat]\n"
        "2:\n"
        "str %[value], [%[dst], #0]\n"
        "adds %[value], %[value], #1\n"
        "str %[value], [%[dst], #4]\n"
        "adds %[value], %[value], #1\n"
        "str %[value], [%[dst], #8]\n"
        "adds %[value], %[value], #1\n"
        "str %[value], [%[dst], #12]\n"
        "adds %[value], %[value], #1\n"
        "str %[value], [%[dst], #16]\n"
        "adds %[value], %[value], #1\n"
        "str %[value], [%[dst], #20]\n"
        "adds %[value], %[value], #1\n"
        "str %[value], [%[dst], #24]\n"
        "adds %[value], %[value], #1\n"
        "str %[value], [%[dst], #28]\n"
        "adds %[value], %[value], #1\n"
        "adds %[dst], %[dst], #32\n"
        "subs %[remaining], %[remaining], #8\n"
        "bne 2b\n"
        "adds %[repeat], %[repeat], #1\n"
        "cmp %[repeat], %[repeats]\n"
        "bne 1b\n"
        "3:\n"
        : [dst] "=&r"(dst),
          [remaining] "=&r"(remaining),
          [value] "=&r"(value),
          [repeat] "+&r"(repeat)
        : [base] "r"(buffer),
          [words] "r"(words),
          [repeats] "r"(repeats)
        : "cc", "memory");
    sram_bus_barrier();
    end = dwt_get_cycle_counter_val();

    return end - start;
}

__attribute__((noinline))
static uint32_t sram_bus_seq_write_sync(volatile uint32_t *buffer,
                                        uint32_t words, uint32_t repeats)
{
    volatile uint32_t *dst;
    uint32_t remaining;
    uint32_t value;
    uint32_t repeat = 0U;
    uint32_t start;
    uint32_t end;

    sram_bus_barrier();
    start = dwt_get_cycle_counter_val();
    __asm volatile(
        "cmp %[words], #0\n"
        "beq 3f\n"
        "cmp %[repeats], #0\n"
        "beq 3f\n"
        "1:\n"
        "mov %[dst], %[base]\n"
        "mov %[remaining], %[words]\n"
        "mov %[value], %[repeat]\n"
        "2:\n"
        "str %[value], [%[dst], #0]\n"
        "dsb sy\n"
        "adds %[value], %[value], #1\n"
        "str %[value], [%[dst], #4]\n"
        "dsb sy\n"
        "adds %[value], %[value], #1\n"
        "str %[value], [%[dst], #8]\n"
        "dsb sy\n"
        "adds %[value], %[value], #1\n"
        "str %[value], [%[dst], #12]\n"
        "dsb sy\n"
        "adds %[value], %[value], #1\n"
        "str %[value], [%[dst], #16]\n"
        "dsb sy\n"
        "adds %[value], %[value], #1\n"
        "str %[value], [%[dst], #20]\n"
        "dsb sy\n"
        "adds %[value], %[value], #1\n"
        "str %[value], [%[dst], #24]\n"
        "dsb sy\n"
        "adds %[value], %[value], #1\n"
        "str %[value], [%[dst], #28]\n"
        "dsb sy\n"
        "adds %[value], %[value], #1\n"
        "adds %[dst], %[dst], #32\n"
        "subs %[remaining], %[remaining], #8\n"
        "bne 2b\n"
        "adds %[repeat], %[repeat], #1\n"
        "cmp %[repeat], %[repeats]\n"
        "bne 1b\n"
        "3:\n"
        : [dst] "=&r"(dst),
          [remaining] "=&r"(remaining),
          [value] "=&r"(value),
          [repeat] "+&r"(repeat)
        : [base] "r"(buffer),
          [words] "r"(words),
          [repeats] "r"(repeats)
        : "cc", "memory");
    end = dwt_get_cycle_counter_val();

    return end - start;
}

__attribute__((noinline))
static uint32_t sram_bus_seq_read(volatile const uint32_t *buffer,
                                  uint32_t words, uint32_t repeats)
{
    uint32_t sum0 = 0U;
    uint32_t sum1 = 0U;
    uint32_t start;
    uint32_t end;

    sram_bus_barrier();
    start = dwt_get_cycle_counter_val();
    for (uint32_t r = 0; r < repeats; ++r) {
        for (uint32_t i = 0; i < words; i += 8U) {
            sum0 += buffer[i + 0U];
            sum1 += buffer[i + 1U];
            sum0 += buffer[i + 2U];
            sum1 += buffer[i + 3U];
            sum0 += buffer[i + 4U];
            sum1 += buffer[i + 5U];
            sum0 += buffer[i + 6U];
            sum1 += buffer[i + 7U];
        }
    }
    sram_bus_barrier();
    end = dwt_get_cycle_counter_val();
    s_sram_bus_sink = (uintptr_t)(sum0 ^ sum1);

    return end - start;
}

static sram_bus_node_t *sram_bus_build_random_cycle(uint32_t cpu_addr,
                                                    uint32_t node_count)
{
    sram_bus_node_t *nodes = (sram_bus_node_t *)(uintptr_t)cpu_addr;
    const uint32_t mask = node_count - 1U;

    /*
     * The LCG has one full-period cycle for a power-of-two modulus because
     * C is odd and A mod 4 is 1. It gives every node exactly one successor
     * without allocating a permutation table.
     */
    for (uint32_t i = 0; i < node_count; ++i) {
        uint32_t next = (SRAM_BUS_BENCH_LCG_A * i +
                         SRAM_BUS_BENCH_LCG_C) & mask;
        nodes[i].next = &nodes[next];
        nodes[i].value = i ^ 0x5A5A5A5AU;
    }
    sram_bus_barrier();

    return &nodes[0];
}

__attribute__((noinline))
static uint32_t sram_bus_random_read(sram_bus_node_t *first,
                                     uint32_t accesses)
{
    sram_bus_node_t *node = first;
    uint32_t start;
    uint32_t end;

    sram_bus_barrier();
    start = dwt_get_cycle_counter_val();
    for (uint32_t i = 0; i < accesses; ++i) {
        node = node->next;
    }
    sram_bus_barrier();
    end = dwt_get_cycle_counter_val();
    s_sram_bus_sink = (uintptr_t)node;

    return end - start;
}

__attribute__((noinline))
static uint32_t sram_bus_random_rmw(sram_bus_node_t *first,
                                    uint32_t accesses)
{
    sram_bus_node_t *node = first;
    uint32_t start;
    uint32_t end;

    sram_bus_barrier();
    start = dwt_get_cycle_counter_val();
    for (uint32_t i = 0; i < accesses; ++i) {
        node->value += i;
        node = node->next;
    }
    sram_bus_barrier();
    end = dwt_get_cycle_counter_val();
    s_sram_bus_sink = (uintptr_t)node;

    return end - start;
}

__attribute__((noinline))
static uint32_t sram_bus_random_write(volatile uint32_t *buffer,
                                      uint32_t words, uint32_t accesses)
{
    const uint32_t mask = words - 1U;
    uint32_t index = 0U;
    uint32_t start;
    uint32_t end;

    sram_bus_barrier();
    start = dwt_get_cycle_counter_val();
    for (uint32_t i = 0; i < accesses; ++i) {
        index = (SRAM_BUS_BENCH_LCG_A * index +
                 SRAM_BUS_BENCH_LCG_C) & mask;
        buffer[index] = i ^ 0xA5A55A5AU;
    }
    sram_bus_barrier();
    end = dwt_get_cycle_counter_val();
    s_sram_bus_sink = index;

    return end - start;
}

__attribute__((noinline))
static uint32_t sram_bus_random_write_sync(volatile uint32_t *buffer,
                                           uint32_t words,
                                           uint32_t accesses)
{
    const uint32_t mask = words - 1U;
    uint32_t index = 0U;
    uint32_t start;
    uint32_t end;

    sram_bus_barrier();
    start = dwt_get_cycle_counter_val();
    for (uint32_t i = 0; i < accesses; ++i) {
        index = (SRAM_BUS_BENCH_LCG_A * index +
                 SRAM_BUS_BENCH_LCG_C) & mask;
        buffer[index] = i ^ 0x5A5AA5A5U;
        sram_bus_barrier();
    }
    end = dwt_get_cycle_counter_val();
    s_sram_bus_sink = index;

    return end - start;
}

static void sram_bus_print_metric(const char *name, uint32_t cycles,
                                  uint32_t overhead, uint32_t accesses,
                                  uint32_t bytes_per_access)
{
    uint32_t adjusted_cycles = (cycles > overhead) ? cycles - overhead : cycles;
    uint64_t total_bytes = (uint64_t)accesses * bytes_per_access;
    if (adjusted_cycles == 0U) {
        adjusted_cycles = 1U;
    }
    uint64_t cycles_per_access_x100 =
        ((uint64_t)adjusted_cycles * 100U + accesses / 2U) / accesses;
    uint64_t mbps_x100 =
        (total_bytes * SRAM_BUS_BENCH_CPU_MHZ * 100U +
         adjusted_cycles / 2U) / adjusted_cycles;

    CLI_LOGI("  %-10s cycles=%u accesses=%u cycles/access=%u.%02u "
             "throughput=%u.%02u MB/s\r\n",
             name, (unsigned)adjusted_cycles, (unsigned)accesses,
             (unsigned)(cycles_per_access_x100 / 100U),
             (unsigned)(cycles_per_access_x100 % 100U),
             (unsigned)(mbps_x100 / 100U),
             (unsigned)(mbps_x100 % 100U));
}

static void sram_bus_print_transfer_metric(const char *name, uint32_t cycles,
                                           uint64_t total_bytes)
{
    uint64_t cycles_per_byte_x100;
    uint64_t mbps_x100;

    if (cycles == 0U) {
        cycles = 1U;
    }
    cycles_per_byte_x100 =
        ((uint64_t)cycles * 100U + total_bytes / 2U) / total_bytes;
    mbps_x100 =
        (total_bytes * SRAM_BUS_BENCH_CPU_MHZ * 100U + cycles / 2U) / cycles;

    CLI_LOGI("  %-10s cycles=%u bytes=%llu cycles/byte=%u.%02u "
             "throughput=%u.%02u MB/s\r\n",
             name, (unsigned)cycles, (unsigned long long)total_bytes,
             (unsigned)(cycles_per_byte_x100 / 100U),
             (unsigned)(cycles_per_byte_x100 % 100U),
             (unsigned)(mbps_x100 / 100U),
             (unsigned)(mbps_x100 % 100U));
}

__attribute__((noinline))
static uint32_t sram_bus_memcpy(void *dst, const void *src,
                                uint32_t bytes, uint32_t repeats)
{
    uint32_t start;
    uint32_t end;

    sram_bus_barrier();
    start = dwt_get_cycle_counter_val();
    for (uint32_t i = 0; i < repeats; ++i) {
        memcpy(dst, src, bytes);
    }
    sram_bus_barrier();
    end = dwt_get_cycle_counter_val();
    s_sram_bus_sink = (uintptr_t)((const uint8_t *)dst)[bytes - 1U];

    return end - start;
}

/*
 * Copy 32 bytes per loop with one LDMIA and one STMIA instruction.
 *
 * This is a CPU load/store-multiple test, not DMA. Cortex-M55 can translate
 * load/store multiples to AXI INCR transactions for Normal memory, but the
 * actual AxLEN is implementation dependent and must be confirmed by a bus
 * monitor.
 */
__attribute__((noinline))
static void sram_bus_ldmstm_copy_once(void *dst, const void *src,
                                      uint32_t bytes)
{
    uint32_t *dst_words = (uint32_t *)dst;
    const uint32_t *src_words = (const uint32_t *)src;
    uint32_t blocks = bytes / 32U;

    __asm volatile(
        "cmp %[blocks], #0\n"
        "beq 2f\n"
        "1:\n"
        "ldmia %[src]!, {r4-r11}\n"
        "stmia %[dst]!, {r4-r11}\n"
        "subs %[blocks], %[blocks], #1\n"
        "bne 1b\n"
        "2:\n"
        : [dst] "+r"(dst_words),
          [src] "+r"(src_words),
          [blocks] "+r"(blocks)
        :
        : "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11",
          "cc", "memory");
}

__attribute__((noinline))
static uint32_t sram_bus_cpu_ldmstm(void *dst, const void *src,
                                    uint32_t bytes, uint32_t repeats)
{
    uint32_t start;
    uint32_t end;

    sram_bus_barrier();
    start = dwt_get_cycle_counter_val();
    for (uint32_t i = 0; i < repeats; ++i) {
        sram_bus_ldmstm_copy_once(dst, src, bytes);
    }
    sram_bus_barrier();
    end = dwt_get_cycle_counter_val();
    s_sram_bus_sink = (uintptr_t)((const uint8_t *)dst)[bytes - 1U];

    return end - start;
}

static void sram_bus_run_region(const sram_bus_region_t *region,
                                uint32_t requested_bytes, uint32_t repeats)
{
    uint32_t bytes = requested_bytes;
    uint32_t cpu_addr;
    uint32_t words;
    uint32_t node_count;
    uint32_t seq_accesses;
    uint32_t random_accesses;
    uint32_t overhead;
    uint32_t int_level;
    uint32_t write_cycles;
    uint32_t write_sync_cycles;
    uint32_t read_cycles;
    uint32_t random_read_cycles;
    uint32_t random_rmw_cycles;
    uint32_t random_write_cycles;
    uint32_t random_write_sync_cycles;
    uint32_t memcpy_bytes;
    uint32_t memcpy_cycles;
    uint32_t ldmstm_cycles;
    sram_bus_node_t *first;

    if (bytes > region->size) {
        bytes = region->size;
    }
    if (bytes > SRAM_BUS_BENCH_DEFAULT_BYTES) {
        bytes = SRAM_BUS_BENCH_DEFAULT_BYTES;
    }
    bytes = sram_bus_power_of_two_floor(bytes);
    if (bytes < SRAM_BUS_BENCH_MIN_BYTES) {
        CLI_LOGI("%s skipped: reserved region too small (%u bytes)\r\n",
                 region->name, (unsigned)region->size);
        return;
    }

    cpu_addr = SOC_SRAM_CPU_ADDR(region->addr);
    words = bytes / sizeof(uint32_t);
    words &= ~7U;
    node_count = bytes / sizeof(sram_bus_node_t);
    node_count = sram_bus_power_of_two_floor(node_count);
    seq_accesses = words * repeats;
    random_accesses = node_count * repeats;

    CLI_LOGI("\r\n[%s] peripheral=0x%08x cpu=0x%08x bytes=%u repeats=%u\r\n",
             region->name, (unsigned)region->addr, (unsigned)cpu_addr,
             (unsigned)bytes, (unsigned)repeats);

    /* Warm instruction paths before the measured samples. */
    (void)sram_bus_seq_write((volatile uint32_t *)(uintptr_t)cpu_addr, words, 1U);
    (void)sram_bus_seq_write_sync(
        (volatile uint32_t *)(uintptr_t)cpu_addr, words, 1U);
    (void)sram_bus_seq_read((volatile const uint32_t *)(uintptr_t)cpu_addr,
                            words, 1U);

    int_level = rtos_disable_int();
    overhead = sram_bus_measure_overhead();
    write_cycles = sram_bus_seq_write(
        (volatile uint32_t *)(uintptr_t)cpu_addr, words, repeats);
    write_sync_cycles = sram_bus_seq_write_sync(
        (volatile uint32_t *)(uintptr_t)cpu_addr, words, repeats);
    read_cycles = sram_bus_seq_read(
        (volatile const uint32_t *)(uintptr_t)cpu_addr, words, repeats);
    rtos_enable_int(int_level);

    first = sram_bus_build_random_cycle(cpu_addr, node_count);
    (void)sram_bus_random_read(first, node_count);
    (void)sram_bus_random_rmw(first, node_count);

    int_level = rtos_disable_int();
    random_read_cycles = sram_bus_random_read(first, random_accesses);
    random_rmw_cycles = sram_bus_random_rmw(first, random_accesses);
    rtos_enable_int(int_level);

    (void)sram_bus_random_write(
        (volatile uint32_t *)(uintptr_t)cpu_addr, words, node_count);
    (void)sram_bus_random_write_sync(
        (volatile uint32_t *)(uintptr_t)cpu_addr, words, node_count);

    int_level = rtos_disable_int();
    random_write_cycles = sram_bus_random_write(
        (volatile uint32_t *)(uintptr_t)cpu_addr, words, random_accesses);
    random_write_sync_cycles = sram_bus_random_write_sync(
        (volatile uint32_t *)(uintptr_t)cpu_addr, words, random_accesses);
    rtos_enable_int(int_level);

    memcpy_bytes = bytes / 2U;
    (void)sram_bus_memcpy(
        (void *)(uintptr_t)(cpu_addr + memcpy_bytes),
        (const void *)(uintptr_t)cpu_addr, memcpy_bytes, 1U);
    int_level = rtos_disable_int();
    memcpy_cycles = sram_bus_memcpy(
        (void *)(uintptr_t)(cpu_addr + memcpy_bytes),
        (const void *)(uintptr_t)cpu_addr, memcpy_bytes, repeats);
    rtos_enable_int(int_level);

    (void)sram_bus_cpu_ldmstm(
        (void *)(uintptr_t)(cpu_addr + memcpy_bytes),
        (const void *)(uintptr_t)cpu_addr, memcpy_bytes, 1U);
    int_level = rtos_disable_int();
    ldmstm_cycles = sram_bus_cpu_ldmstm(
        (void *)(uintptr_t)(cpu_addr + memcpy_bytes),
        (const void *)(uintptr_t)cpu_addr, memcpy_bytes, repeats);
    rtos_enable_int(int_level);

    sram_bus_print_metric("seq_write", write_cycles, overhead,
                          seq_accesses, sizeof(uint32_t));
    sram_bus_print_metric("seq_wr_sync", write_sync_cycles, overhead,
                          seq_accesses, sizeof(uint32_t));
    sram_bus_print_metric("seq_read", read_cycles, overhead,
                          seq_accesses, sizeof(uint32_t));
    sram_bus_print_metric("random_rd", random_read_cycles, overhead,
                          random_accesses, sizeof(void *));
    sram_bus_print_metric("random_rmw", random_rmw_cycles, overhead,
                          random_accesses,
                          sizeof(void *) + 2U * sizeof(uint32_t));
    sram_bus_print_metric("random_wr", random_write_cycles, overhead,
                          random_accesses, sizeof(uint32_t));
    sram_bus_print_metric("random_wr_sync", random_write_sync_cycles, overhead,
                          random_accesses, sizeof(uint32_t));
    sram_bus_print_transfer_metric("memcpy", memcpy_cycles,
                                   (uint64_t)memcpy_bytes * repeats);
    sram_bus_print_transfer_metric("cpu_ldm8", ldmstm_cycles,
                                   (uint64_t)memcpy_bytes * repeats);
}

#if CONFIG_HIGH_PERFORMANCE_DMA && CONFIG_SPE
static const hpdma_burst_len_t s_sram_bus_burst_values[] = {
    HPDMA_BURST_LEN_SINGLE,
    HPDMA_BURST_LEN_INC4,
    HPDMA_BURST_LEN_INC8,
    HPDMA_BURST_LEN_INC16,
};

static const char *const s_sram_bus_burst_names[] = {
    "SINGLE", "INCR4", "INCR8", "INCR16",
};

static void sram_bus_fill_copy_source(uint32_t src_addr, uint32_t dst_addr,
                                      uint32_t bytes)
{
    volatile uint32_t *src = (volatile uint32_t *)(uintptr_t)src_addr;
    volatile uint32_t *dst = (volatile uint32_t *)(uintptr_t)dst_addr;
    uint32_t words = bytes / sizeof(uint32_t);

    for (uint32_t i = 0; i < words; ++i) {
        src[i] = 0xA5000000U ^ (i * 0x1021U);
        dst[i] = 0U;
    }
    sram_bus_barrier();
}

static bk_err_t sram_bus_hpdma_case(uint32_t src_addr, uint32_t dst_addr,
                                    uint32_t bytes, uint32_t repeats,
                                    hpdma_burst_len_t burst,
                                    uint32_t *cycles, uint32_t *actual_burst)
{
    hpdma_id_t dma_id = HPDMA_ID_MAX;
    hpdma_config_t config = {0};
    bk_err_t ret;
    uint32_t start;
    uint32_t end;

    if ((cycles == NULL) || (actual_burst == NULL)) {
        return BK_ERR_PARAM;
    }

    dma_id = bk_hpdma_alloc(HPDMA_DEV_DTCM);
    if (dma_id >= HPDMA_ID_MAX) {
        return BK_FAIL;
    }

    config.mode = HPDMA_WORK_MODE_SINGLE;
    config.chan_prio = 2;
    config.src.dev = HPDMA_DEV_DTCM;
    config.src.width = HPDMA_DATA_WIDTH_32BITS;
    config.src.addr_inc_en = HPDMA_ADDR_INC_ENABLE;
    config.src.addr_loop_en = HPDMA_ADDR_LOOP_DISABLE;
    config.src.start_addr = src_addr;
    config.src.xsize = (uint16_t)bytes;
    config.src.ysize = 1U;
    config.dst.dev = HPDMA_DEV_DTCM;
    config.dst.width = HPDMA_DATA_WIDTH_32BITS;
    config.dst.addr_inc_en = HPDMA_ADDR_INC_ENABLE;
    config.dst.addr_loop_en = HPDMA_ADDR_LOOP_DISABLE;
    config.dst.start_addr = dst_addr;
    config.dst.xsize = (uint16_t)bytes;
    config.dst.ysize = 1U;
    config.trans_type = HPDMA_TRANS_DEFAULT;

    ret = bk_hpdma_init(dma_id, &config);
    if (ret != BK_OK) {
        goto cleanup;
    }
    ret = bk_hpdma_set_src_burst_len(dma_id, burst);
    if (ret != BK_OK) {
        goto cleanup;
    }
    ret = bk_hpdma_set_dest_burst_len(dma_id, burst);
    if (ret != BK_OK) {
        goto cleanup;
    }
    ret = bk_hpdma_set_src_sec_attr(dma_id, HPDMA_ATTR_SEC);
    if (ret != BK_OK) {
        goto cleanup;
    }
    ret = bk_hpdma_set_dest_sec_attr(dma_id, HPDMA_ATTR_SEC);
    if (ret != BK_OK) {
        goto cleanup;
    }

    /* Warm the driver path and apply the hardware SMEM burst policy. */
    ret = bk_hpdma_start(dma_id);
    if (ret != BK_OK) {
        goto cleanup;
    }
    ret = bk_hpdma_wait_to_idle(dma_id);
    if (ret != BK_OK) {
        goto cleanup;
    }
    *actual_burst = bk_hpdma_get_src_burst_len(dma_id);

    sram_bus_barrier();
    start = dwt_get_cycle_counter_val();
    for (uint32_t i = 0; i < repeats; ++i) {
        ret = bk_hpdma_start(dma_id);
        if (ret != BK_OK) {
            break;
        }
        ret = bk_hpdma_wait_to_idle(dma_id);
        if (ret != BK_OK) {
            break;
        }
    }
    sram_bus_barrier();
    end = dwt_get_cycle_counter_val();
    *cycles = end - start;

cleanup:
    if (dma_id < HPDMA_ID_MAX) {
        bk_err_t free_ret = bk_hpdma_free(HPDMA_DEV_DTCM, dma_id);
        if ((ret == BK_OK) && (free_ret != BK_OK)) {
            ret = free_ret;
        }
    }
    return ret;
}

static void sram_bus_run_hpdma_pair(const sram_bus_region_t *src_region,
                                    const sram_bus_region_t *dst_region,
                                    uint32_t requested_bytes,
                                    uint32_t repeats)
{
    uint32_t bytes = requested_bytes;
    uint32_t src_addr = SOC_SRAM_CPU_ADDR(src_region->addr);
    uint32_t dst_addr = SOC_SRAM_CPU_ADDR(dst_region->addr);

    if (bytes > SRAM_BUS_BENCH_HPDMA_MAX_BYTES) {
        bytes = SRAM_BUS_BENCH_HPDMA_MAX_BYTES;
    }
    if (bytes > src_region->size) {
        bytes = src_region->size;
    }
    if (bytes > dst_region->size) {
        bytes = dst_region->size;
    }
    bytes &= ~63U;
    if (bytes < SRAM_BUS_BENCH_MIN_BYTES) {
        return;
    }

    CLI_LOGI("\r\n[HPDMA %s -> %s] src=0x%08x dst=0x%08x "
             "bytes=%u repeats=%u\r\n",
             src_region->name, dst_region->name,
             (unsigned)src_addr, (unsigned)dst_addr,
             (unsigned)bytes, (unsigned)repeats);

    sram_bus_fill_copy_source(src_addr, dst_addr, bytes);
    for (uint32_t i = 0;
         i < (uint32_t)(sizeof(s_sram_bus_burst_values) /
                        sizeof(s_sram_bus_burst_values[0]));
         ++i) {
        uint32_t cycles = 0U;
        uint32_t actual_burst = 0U;
        bk_err_t ret = sram_bus_hpdma_case(
            src_addr, dst_addr, bytes, repeats,
            s_sram_bus_burst_values[i], &cycles, &actual_burst);

        if (ret != BK_OK) {
            CLI_LOGI("  HPDMA %-6s failed: ret=%d\r\n",
                     s_sram_bus_burst_names[i], (int)ret);
            continue;
        }
        if (memcmp((const void *)(uintptr_t)src_addr,
                   (const void *)(uintptr_t)dst_addr, bytes) != 0) {
            CLI_LOGI("  HPDMA %-6s failed: data mismatch\r\n",
                     s_sram_bus_burst_names[i]);
            continue;
        }

        CLI_LOGI("  requested=%-6s actual=%s\r\n",
                 s_sram_bus_burst_names[i],
                 (actual_burst < (uint32_t)(sizeof(s_sram_bus_burst_names) /
                                             sizeof(s_sram_bus_burst_names[0])))
                     ? s_sram_bus_burst_names[actual_burst] : "UNKNOWN");
        sram_bus_print_transfer_metric(
            "hpdma", cycles, (uint64_t)bytes * repeats);
    }
}

static void sram_bus_run_hpdma_matrix(uint32_t bytes, uint32_t repeats)
{
    if (SRAM_BUS_BENCH_REGION_COUNT < 3U) {
        CLI_LOGI("HPDMA burst matrix skipped: SRAM3/4/5 regions required\r\n");
        return;
    }

    /*
     * Cross-bank transfers preserve the requested INC16 setting. Same-bank
     * SMEM transfers are deliberately limited to INC8 by the HPDMA driver.
     */
    sram_bus_run_hpdma_pair(&s_sram_bus_regions[0],
                            &s_sram_bus_regions[1], bytes, repeats);
    sram_bus_run_hpdma_pair(&s_sram_bus_regions[0],
                            &s_sram_bus_regions[2], bytes, repeats);
    sram_bus_run_hpdma_pair(&s_sram_bus_regions[2],
                            &s_sram_bus_regions[0], bytes, repeats);
}
#endif /* CONFIG_HIGH_PERFORMANCE_DMA && CONFIG_SPE */

static void cli_sram_bus_bench(char *pcWriteBuffer, int xWriteBufferLen,
                               int argc, char **argv)
{
    uint32_t first_region = 0U;
    uint32_t last_region = SRAM_BUS_BENCH_REGION_COUNT;
    uint32_t bytes = SRAM_BUS_BENCH_DEFAULT_BYTES;
    uint32_t repeats = SRAM_BUS_BENCH_DEFAULT_REPEATS;
    uint32_t region_id;

    (void)pcWriteBuffer;
    (void)xWriteBufferLen;

    if ((argc > 4) || ((argc >= 2) && (strcmp(argv[1], "all") != 0) &&
        (sram_bus_parse_u32(argv[1], 0U, SRAM_BUS_BENCH_REGION_COUNT - 1U,
                            &region_id) != 0))) {
        sram_bus_print_usage();
        return;
    }

    if ((argc >= 2) && (strcmp(argv[1], "all") != 0)) {
        first_region = region_id;
        last_region = region_id + 1U;
    }
    if ((argc >= 3) &&
        (sram_bus_parse_u32(argv[2], SRAM_BUS_BENCH_MIN_BYTES,
                            SRAM_BUS_BENCH_DEFAULT_BYTES, &bytes) != 0)) {
        sram_bus_print_usage();
        return;
    }
    if ((argc >= 4) &&
        (sram_bus_parse_u32(argv[3], 1U, SRAM_BUS_BENCH_MAX_REPEATS,
                            &repeats) != 0)) {
        sram_bus_print_usage();
        return;
    }

    if (sys_drv_switch_cpu_bus_freq(PM_CPU_FRQ_480M) != BK_OK) {
        CLI_LOGI("sram_bus_bench failed to switch CPU/bus to %u MHz\r\n",
                 (unsigned)SRAM_BUS_BENCH_CPU_MHZ);
        return;
    }

    dwt_init_cycle_counter();
    CLI_LOGI("sram_bus_bench start: CPU/bus switched to %u MHz\r\n",
             (unsigned)SRAM_BUS_BENCH_CPU_MHZ);
    CLI_LOGI("random_rd is a serialized pointer chase; seq_* measures CPU "
             "streaming, not guaranteed AXI burst\r\n");
    CLI_LOGI("seq_write uses the same inline assembly on both chips; "
             "seq_wr_sync executes DSB after every store\r\n");
    CLI_LOGI("random_wr drains posted stores once; random_wr_sync uses DSB "
             "after every store to measure completion latency\r\n");
    CLI_LOGI("cpu_ldm8 uses CPU LDMIA/STMIA (8 words/instruction); "
             "actual AXI AxLEN requires bus-monitor confirmation\r\n");
    CLI_LOGI("memcpy measures scalar CPU LDR/STR; HPDMA explicitly requests "
             "SINGLE/INCR4/INCR8/INCR16\r\n");

    for (uint32_t i = first_region; i < last_region; ++i) {
        sram_bus_run_region(&s_sram_bus_regions[i], bytes, repeats);
    }

#if CONFIG_HIGH_PERFORMANCE_DMA && CONFIG_SPE
    if ((first_region == 0U) &&
        (last_region == SRAM_BUS_BENCH_REGION_COUNT)) {
        sram_bus_run_hpdma_matrix(bytes, repeats);
    }
#else
    CLI_LOGI("HPDMA burst matrix unavailable: HPDMA/SPE not enabled\r\n");
#endif

    CLI_LOGI("\r\nsram_bus_bench done, sink=0x%08x\r\n",
             (unsigned)s_sram_bus_sink);
}

DRV_CLI_CMD_EXPORT
static const struct cli_command s_sram_bus_bench_commands[] = {
    {"sram_bus_bench", "[all|region_id] [bytes] [repeats]",
     cli_sram_bus_bench},
};

#endif /* CONFIG_SRAM_TEST */
