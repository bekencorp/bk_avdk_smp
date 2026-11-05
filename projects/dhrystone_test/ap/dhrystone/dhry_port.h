/*
 * Dhrystone 2.1 (DMIPS) port configuration - BK7259 AP core
 *
 * The official dhry.h / dhry_1.c / dhry_2.c are kept unmodified in this directory
 * and pulled in by dhry_glue.c through macro adaptation.
 */
#ifndef BK_DHRYSTONE_PORT_H
#define BK_DHRYSTONE_PORT_H

/* Timer0 free-running clock, the same time base as core_mark */
#define DHRY_TIMER_HZ      26000000UL

/* Unit returned by times(): 1 tick = 1 us.
 * The TIMES branch of the official dhry_1.c computes:
 *   Dhrystones_Per_Second = HZ * Number_Of_Runs / User_Time
 *   Microseconds          = User_Time * 1e6 / (HZ * Number_Of_Runs)
 * With HZ = 1000000 and tick = 1 us both formulas yield the real value. */
#define DHRY_HZ            1000000UL

/* DMIPS reference: 1757 Dhrystones/s on a VAX 11/780 is defined as 1 DMIPS */
#define DHRY_DMIPS_BASE    1757.0

/* The official program asks for the run count interactively; provide a default
 * that can be overridden with `dhrystone <runs>`.
 * Note: the official rule requires a measurement window of at least 2 seconds,
 * a smaller count prints "Measured time too small". */
#define DHRY_DEFAULT_RUNS  5000000

/* Code placement, mirroring core_mark (COREMARK_IRAM_CODE / COREMARK_PSRAM_CODE):
 *   DHRY_IRAM_CODE=1  -> internal SRAM (.iram), no Flash XIP stall
 *   DHRY_PSRAM_CODE=1 -> PSRAM code section (.psram.code)
 *   (default)         -> Flash XIP */
#if defined(DHRY_IRAM_CODE) && (DHRY_IRAM_CODE == 1)
#define DHRY_FN_ATTR __attribute__((section(".iram")))
#elif defined(DHRY_PSRAM_CODE) && (DHRY_PSRAM_CODE == 1)
#define DHRY_FN_ATTR __attribute__((section(".psram.code"), noinline))
#else
#define DHRY_FN_ATTR
#endif

/* Placement of the two Rec_Type records the official code allocates with malloc().
 * dhry_malloc() lives in dhry_glue.c and defaults to os_malloc(), i.e. the internal
 * SRAM heap core_mark uses for its data blocks, instead of the PSRAM heap that the
 * newlib malloc() wrapper (__wrap_malloc -> psram_malloc) would hand out. */
void *dhry_malloc(unsigned int size);

void dhrystone_run(int argc, char *argv[]);

#endif /* BK_DHRYSTONE_PORT_H */
