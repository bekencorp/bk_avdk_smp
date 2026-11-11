/*
 * Opus CELT inner-product benchmark CLI.
 *
 * Decodes the embedded opus vectors (SILK / hybrid / CELT, mono and stereo,
 * 2.5ms to 60ms frames) and encodes 20ms voice frames, running the scalar and
 * MVE CELT kernels over the same stream, measuring with the DWT cycle counter
 * and checking bit-exactness of the PCM / bitstream output. It bypasses the
 * capture/playback path completely.
 *
 * Usage:
 *   opus_bench                                    all cases, 10 iterations, PM clock
 *   opus_bench [iters] [case] [cpu_mhz]
 * Examples:
 *   opus_bench 10 0 480                             all 13 decode + 4 encode cases at 480MHz
 *   opus_bench 20 10 0                              decode vector D10 (fullband CELT), PM clock
 *   opus_bench 20 101 480                           encode case E01 (32kbps VBR mediumband)
 */

#include <stdio.h>
#include <stdlib.h>
#include <common/bk_include.h>
#include <os/os.h>
#include <os/str.h>
#include <modules/pm.h>
#include "bk_private/bk_cli.h"

/* Implemented in aud_opus_bench.c inside libopus.a */
extern int aud_opus_bench_run_case(uint32_t case_idx, uint32_t iters, uint32_t cpu_mhz);

/* Read the CPU frequency currently in effect on the AP and convert it to MHz.
 * Note that CONFIG_PM_CPU_FRQ_DEFAULT may keep the AP at a lower level such as
 * 80M/160M; converting time at 480MHz would then be wrong, so use the runtime level. */
static uint32_t cli_pm_cpu_mhz(void)
{
    switch (bk_pm_current_max_cpu_freq_get())
    {
        case PM_CPU_FRQ_80M:
            return 80;
        case PM_CPU_FRQ_120M:
            return 120;
        case PM_CPU_FRQ_160M:
            return 160;
        case PM_CPU_FRQ_240M:
            return 240;
        case PM_CPU_FRQ_320M:
            return 320;
        case PM_CPU_FRQ_480M:
            return 480;
        default:
            return 480;   /* XTAL/HIGHEST/DEFAULT cannot be converted; fall back to the highest level */
    }
}

/* opus_decode() / opus_encode() allocate their scratch buffers on the caller's
 * stack (VAR_ARRAYS), so the demand scales with the packet duration and channel
 * count: opus accepts packets up to 120ms (48 x 2.5ms), i.e. 5760 samples per
 * channel. Sizing this by guesswork is pointless - the "stack peak" line printed
 * by a `opus_bench 0` and `opus_bench 200/201` run measures it, and the value
 * below is that measured peak plus margin. */
#define OPUS_BENCH_TASK_STACK_SIZE (65536)

static beken_thread_t s_bench_thread = NULL;
static beken_semaphore_t s_bench_done = NULL;
static uint32_t s_bench_iters;
static uint32_t s_bench_case;
static uint32_t s_bench_cpu_mhz;
static int s_bench_ret;

static void opus_bench_worker(beken_thread_arg_t arg)
{
    (void)arg;

    s_bench_ret = aud_opus_bench_run_case(s_bench_case, s_bench_iters, s_bench_cpu_mhz);

    /* case 0 runs every vector inside this single task, so the reported
     * "peak_used" of the opus_bench thread is the worst-case stack demand and
     * is what the stack size above must be sized against. 200/201 are the
     * 120ms probes and are the true worst case. */
    if ((s_bench_case == 0) || (s_bench_case == 200) || (s_bench_case == 201))
    {
        extern void rtos_dump_stack_memory_usage(void);
        rtos_dump_stack_memory_usage();
    }

    rtos_set_semaphore(&s_bench_done);
    rtos_delete_thread(NULL);
}

static void cli_opus_bench_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t iters = 10;
    uint32_t case_idx = 0;   /* 0 = all cases */
    uint32_t cpu_mhz = 0;    /* 0 = derive automatically from the current PM level */
    bk_err_t err;
    int ret;

    if (argc >= 2)
    {
        iters = (uint32_t)strtoul(argv[1], NULL, 10);
    }
    if (argc >= 3)
    {
        case_idx = (uint32_t)strtoul(argv[2], NULL, 10);
    }
    if (argc >= 4)
    {
        cpu_mhz = (uint32_t)strtoul(argv[3], NULL, 10);
    }
    if (cpu_mhz == 0)
    {
        cpu_mhz = cli_pm_cpu_mhz();
    }

    if (s_bench_thread != NULL)
    {
        cmd_printf("opus_bench busy: a run is already in progress\r\n");
        return;
    }
    if (s_bench_done == NULL)
    {
        err = rtos_init_semaphore(&s_bench_done, 1);
        if (err != BK_OK)
        {
            cmd_printf("opus_bench init semaphore failed: %d\r\n", (int)err);
            return;
        }
    }

    s_bench_iters = iters;
    s_bench_case = case_idx;
    s_bench_cpu_mhz = cpu_mhz;
    s_bench_ret = -1;

    err = rtos_create_thread(&s_bench_thread, BEKEN_DEFAULT_WORKER_PRIORITY - 1, "opus_bench",
                             opus_bench_worker, OPUS_BENCH_TASK_STACK_SIZE, NULL);
    if (err != BK_OK)
    {
        s_bench_thread = NULL;
        cmd_printf("opus_bench create task failed: %d\r\n", (int)err);
        return;
    }

    rtos_get_semaphore(&s_bench_done, BEKEN_WAIT_FOREVER);
    s_bench_thread = NULL;
    ret = s_bench_ret;

    cmd_printf("opus_bench iters=%u case=%u cpu=%uMHz(pm=%d) ret=%d\r\n",
               (unsigned)iters, (unsigned)case_idx, (unsigned)cpu_mhz,
               (int)bk_pm_current_max_cpu_freq_get(), ret);
}

/* The four kernel switches are defined unconditionally in the *_mve.c files of the
 * internal Opus library, so the command links in every MVE=1 build. It only has a
 * visible effect in dual-run images, where both branches are compiled in; in a
 * single-run image the scalar branch does not exist and the flag is unused. */
extern int aud_opus_mve_enabled;
extern int aud_opus_nsq_mve_enabled;
extern int aud_opus_lpc_mve_enabled;
extern int aud_opus_sum_sqr_mve_enabled;

static void cli_opus_mve_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    int on = 1;

    (void)pcWriteBuffer;
    (void)xWriteBufferLen;

    if (argc >= 2)
    {
        if (os_strcmp(argv[1], "off") == 0)
        {
            on = 0;
        }
        else if (os_strcmp(argv[1], "on") == 0)
        {
            on = 1;
        }
        else
        {
            on = (strtoul(argv[1], NULL, 10) != 0) ? 1 : 0;
        }
    }

    aud_opus_mve_enabled = on;
    aud_opus_nsq_mve_enabled = on;
    aud_opus_lpc_mve_enabled = on;
    aud_opus_sum_sqr_mve_enabled = on;

    cmd_printf("opus_mve %s (celt=%d nsq=%d lpc=%d sum_sqr=%d)\r\n",
               on ? "on" : "off", aud_opus_mve_enabled, aud_opus_nsq_mve_enabled,
               aud_opus_lpc_mve_enabled, aud_opus_sum_sqr_mve_enabled);
}

#define OPUS_BENCH_CMD_CNT (sizeof(s_opus_bench_commands) / sizeof(struct cli_command))
static const struct cli_command s_opus_bench_commands[] = {
    {"opus_bench", "opus_bench [iters] [case] [cpu_mhz] (case 0=all, 1..13=decode, 101..104=encode, 300/301=prod encode/roundtrip bitexact)",
     cli_opus_bench_cmd},
    {"opus_mve", "opus_mve [on|off] (switch MVE/scalar kernels at run time, dual-run images only)",
     cli_opus_mve_cmd},
};

int cli_opus_bench_init(void)
{
    return cli_register_commands(s_opus_bench_commands, OPUS_BENCH_CMD_CNT);
}
