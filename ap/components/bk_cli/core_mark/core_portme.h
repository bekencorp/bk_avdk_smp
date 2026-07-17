#ifndef FESDK_CORE_PORTME_H
#define FESDK_CORE_PORTME_H

#include <stdint.h>
#include <stddef.h>

#if defined(COREMARK_CONTEXTS) && (COREMARK_CONTEXTS > 1)
#include <os/os.h>
#endif

#define HAS_FLOAT 1
#define HAS_STDIO 1
#define HAS_PRINTF 1
#define SEED_METHOD SEED_VOLATILE
#define CORE_TICKS uint64_t

typedef signed short ee_s16;
typedef unsigned short ee_u16;
typedef signed int ee_s32;
typedef double ee_f32;
typedef unsigned char ee_u8;
typedef signed int ee_u32;
typedef unsigned long long ee_u64;
typedef ee_u32 ee_ptr_int;
typedef size_t ee_size_t;

#ifndef COREMARK_COMPILER_FLAGS
#define COREMARK_COMPILER_FLAGS " option_xxx "
#endif
#define COMPILER_FLAGS COREMARK_COMPILER_FLAGS

/* Code placement options (mutually exclusive, IRAM takes priority):
 *   COREMARK_IRAM_CODE=1  → internal SRAM (.iram), eliminates Flash XIP stall
 *   COREMARK_PSRAM_CODE=1 → PSRAM code section (.psram.code)
 *   (default)             → Flash XIP */
#if defined(COREMARK_IRAM_CODE) && (COREMARK_IRAM_CODE == 1)
/* No noinline — allow full inlining so the compiler optimizes across call sites.
 * The section attribute only matters for outlined (non-inlined) functions;
 * inlined code lands wherever the caller lands, which is also .iram via
 * the linker script rule for libbk_cli.a(cli_rpc.c.obj). */
#define COREMARK_FUNC_ATTR __attribute__((section(".iram")))
#define COREMARK_PLACEMENT "IRAM"
#elif defined(COREMARK_PSRAM_CODE) && (COREMARK_PSRAM_CODE == 1)
#define COREMARK_FUNC_ATTR __attribute__((section(".psram.code"), noinline))
#define COREMARK_PLACEMENT "PSRAM"
#else
#define COREMARK_FUNC_ATTR
#define COREMARK_PLACEMENT "FLASH-XIP"
#endif

#define align_mem(x) (void *)(((ee_ptr_int)(x) + sizeof(ee_u32) - 1) & -sizeof(ee_u32))

#ifdef __GNUC__
 #ifdef __clang__
  # define COMPILER_VERSION __VERSION__
 #else
  # define COMPILER_VERSION "GCC"__VERSION__
 #endif
#else
# error
#endif

#if defined(COREMARK_CONTEXTS) && (COREMARK_CONTEXTS > 1)
#define MEM_METHOD MEM_MALLOC
#define MEM_LOCATION "HEAP"
#else
#define MEM_METHOD MEM_STATIC
#define MEM_LOCATION "STATIC"
#endif

#ifndef COREMARK_CONTEXTS
#define COREMARK_CONTEXTS 1
#endif

#define MULTITHREAD COREMARK_CONTEXTS

#if (MULTITHREAD > 1)
#define PARALLEL_METHOD "FreeRTOS"
extern ee_u32 default_num_contexts;

typedef struct {
    beken_thread_t thread;
    beken_semaphore_t done;
    uint32_t core_id;
} core_portable;

static void portable_init(core_portable *p, int *argc, char *argv[]);
static void portable_fini(core_portable *p);
#else
#define default_num_contexts MULTITHREAD

typedef int core_portable;
static void portable_init(core_portable *p, int *argc, char *argv[]) {}
static void portable_fini(core_portable *p) {}
#endif

#if !defined(PROFILE_RUN) && !defined(PERFORMANCE_RUN) && !defined(VALIDATION_RUN)
#if (TOTAL_DATA_SIZE==1200)
#define PROFILE_RUN 1
#elif (TOTAL_DATA_SIZE==2000)
#define PERFORMANCE_RUN 1
#else
#define VALIDATION_RUN 1
#endif
#endif

#endif
