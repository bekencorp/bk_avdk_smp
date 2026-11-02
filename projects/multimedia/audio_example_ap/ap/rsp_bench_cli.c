/*
 * RSP tone benchmark CLI.
 *
 * Feeds a series of sine tones into the speex resampler, runs the scalar and MVE
 * implementations, measures time with the DWT cycle counter and performs a bit-exact
 * comparison, fully bypassing the audio capture/playback path.
 *
 * Usage:
 *   rsp_bench                                             default 16000->48000, 1kHz, 20 frames, Q1, mono
 *   rsp_bench <src_rate> <dst_rate> [tone_hz] [iters] [quality] [ch] [cpu_mhz]
 * Examples:
 *   rsp_bench 44100 48000 1000 20 1 1 480            non-integer ratio, uses interpolation path
 *   rsp_bench 32000 48000 1000 20 1 2 480            common A2DP: 2ch interleaved
 */

#include <stdio.h>
#include <stdlib.h>
#include <common/bk_include.h>
#include <os/os.h>
#include <os/str.h>
#include <modules/pm.h>
#include "bk_private/bk_cli.h"

/* Implemented in aud_rsp_bench.c inside libresample.a */
extern int aud_rsp_bench_run_tone(uint32_t src_rate, uint32_t dst_rate, uint32_t tone_hz, uint32_t iters,
                                  uint32_t quality, uint32_t ch, uint32_t cpu_mhz);

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

static void cli_rsp_bench_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t src_rate = 16000;
    uint32_t dst_rate = 48000;
    uint32_t tone_hz = 1000;
    uint32_t iters = 20;
    uint32_t quality = 1;     /* complexity used by the production speaker path */
    uint32_t ch = 1;
    uint32_t cpu_mhz = 0;     /* 0 = derive automatically from the current PM level */
    int ret;

    if (argc >= 2)
    {
        src_rate = (uint32_t)strtoul(argv[1], NULL, 10);
    }
    if (argc >= 3)
    {
        dst_rate = (uint32_t)strtoul(argv[2], NULL, 10);
    }
    if (argc >= 4)
    {
        tone_hz = (uint32_t)strtoul(argv[3], NULL, 10);
    }
    if (argc >= 5)
    {
        iters = (uint32_t)strtoul(argv[4], NULL, 10);
    }
    if (argc >= 6)
    {
        quality = (uint32_t)strtoul(argv[5], NULL, 10);
    }
    if (argc >= 7)
    {
        ch = (uint32_t)strtoul(argv[6], NULL, 10);
    }
    if (argc >= 8)
    {
        cpu_mhz = (uint32_t)strtoul(argv[7], NULL, 10);
    }
    if (cpu_mhz == 0)
    {
        cpu_mhz = cli_pm_cpu_mhz();
    }

    ret = aud_rsp_bench_run_tone(src_rate, dst_rate, tone_hz, iters, quality, ch, cpu_mhz);

    cmd_printf("rsp_bench %u->%u tone=%uHz iters=%u Q%u %uch cpu=%uMHz(pm=%d) ret=%d\r\n",
               (unsigned)src_rate, (unsigned)dst_rate, (unsigned)tone_hz, (unsigned)iters,
               (unsigned)quality, (unsigned)ch, (unsigned)cpu_mhz,
               (int)bk_pm_current_max_cpu_freq_get(), ret);
}

#define RSP_BENCH_CMD_CNT (sizeof(s_rsp_bench_commands) / sizeof(struct cli_command))
static const struct cli_command s_rsp_bench_commands[] = {
    {"rsp_bench", "rsp_bench [src] [dst] [tone_hz] [iters] [quality] [ch] [cpu_mhz]", cli_rsp_bench_cmd},
};

int cli_rsp_bench_init(void)
{
    return cli_register_commands(s_rsp_bench_commands, RSP_BENCH_CMD_CNT);
}
