/*
Copyright 2018 Embedded Microprocessor Benchmark Consortium (EEMBC)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Original Author: Shay Gal-on
*/

/* Topic: Description
        This file contains  declarations of the various benchmark functions.
*/

/* Configuration: TOTAL_DATA_SIZE
        Define total size for data algorithms will operate on
*/
#ifndef FESDK_CORE_MARK_H
#define FESDK_CORE_MARK_H

#ifndef TOTAL_DATA_SIZE
#define TOTAL_DATA_SIZE 2 * 1000
#endif

#define SEED_ARG      0
#define SEED_FUNC     1
#define SEED_VOLATILE 2

#define MEM_STATIC 0
#define MEM_MALLOC 1
#define MEM_STACK  2

#include "core_portme.h"

#if HAS_STDIO
#include <stdio.h>
#endif
#if HAS_PRINTF
#define ee_printf(...) BK_LOGD(NULL, ##__VA_ARGS__)
#endif

/* Actual benchmark execution in iterate */
void *iterate(void *pres);

/* Typedef: secs_ret
        For machines that have floating point support, get number of seconds as
   a double. Otherwise an unsigned int.
*/
#if HAS_FLOAT
typedef double secs_ret;
#else
typedef ee_u32 secs_ret;
#endif


void       start_time(void);
void       stop_time(void);
CORE_TICKS get_time(void);
secs_ret   time_in_secs(CORE_TICKS ticks);

typedef enum coremark_seg_id_e
{
    COREMARK_SEG_LIST = 0,
    COREMARK_SEG_MATRIX,
    COREMARK_SEG_STATE,
    COREMARK_SEG_CRC,
    COREMARK_SEG_LIST_FIND_REV,
    COREMARK_SEG_LIST_FIND_ONLY,
    COREMARK_SEG_LIST_REVERSE,
    COREMARK_SEG_LIST_SORT_COMPLEX,
    COREMARK_SEG_LIST_REMOVE_CRC,
    COREMARK_SEG_LIST_SORT_IDX,
    COREMARK_SEG_LIST_FINAL_CRC,
    COREMARK_SEG_COUNT
} coremark_seg_id;

#ifndef COREMARK_SEGMENT_ENABLE
#define COREMARK_SEGMENT_ENABLE 0
#endif

#if COREMARK_SEGMENT_ENABLE
void coremark_seg_reset(void);
void coremark_seg_enter(coremark_seg_id seg);
void coremark_seg_exit(coremark_seg_id seg);
void coremark_seg_report(ee_u32 iterations, ee_u32 contexts);
void coremark_find_nodes_enable(ee_u32 enable);
void coremark_find_nodes_add(ee_u32 nodes);
#else
#define coremark_seg_reset()                  ((void)0)
#define coremark_seg_enter(seg)               ((void)0)
#define coremark_seg_exit(seg)                ((void)0)
#define coremark_seg_report(iterations, ctxs) ((void)0)
#define coremark_find_nodes_enable(enable)    ((void)0)
#define coremark_find_nodes_add(nodes)        ((void)0)
#endif

/* Misc useful functions */
ee_u16 crcu8(ee_u8 data, ee_u16 crc);
ee_u16 crc16(ee_s16 newval, ee_u16 crc);
ee_u16 crcu16(ee_u16 newval, ee_u16 crc);
ee_u16 crcu32(ee_u32 newval, ee_u16 crc);
ee_u8  check_data_types(void);
void * portable_malloc(ee_size_t size);
void   portable_free(void *p);
ee_s32 parseval(char *valstring);

/* Algorithm IDS */
#define ID_LIST             (1 << 0)
#define ID_MATRIX           (1 << 1)
#define ID_STATE            (1 << 2)
#define ALL_ALGORITHMS_MASK (ID_LIST | ID_MATRIX | ID_STATE)
#define NUM_ALGORITHMS      3

/* list data structures */
typedef struct list_data_s
{
    ee_s16 data16;
    ee_s16 idx;
} list_data;

typedef struct list_head_s
{
    struct list_head_s *next;
    struct list_data_s *info;
} list_head;

/*matrix benchmark related stuff */
#define MATDAT_INT 1
#if MATDAT_INT
typedef ee_s16 MATDAT;
typedef ee_s32 MATRES;
#else
typedef ee_f16 MATDAT;
typedef ee_f32 MATRES;
#endif

typedef struct MAT_PARAMS_S
{
    int     N;
    MATDAT *A;
    MATDAT *B;
    MATRES *C;
} mat_params;

/* state machine related stuff */
/* List of all the possible states for the FSM */
typedef enum CORE_STATE
{
    CORE_START = 0,
    CORE_INVALID,
    CORE_S1,
    CORE_S2,
    CORE_INT,
    CORE_FLOAT,
    CORE_EXPONENT,
    CORE_SCIENTIFIC,
    NUM_CORE_STATES
} core_state_e;

/* Helper structure to hold results */
typedef struct RESULTS_S
{
    /* inputs */
    ee_s16              seed1;       /* Initializing seed */
    ee_s16              seed2;       /* Initializing seed */
    ee_s16              seed3;       /* Initializing seed */
    void *              memblock[4]; /* Pointer to safe memory location */
    ee_u32              size;        /* Size of the data */
    ee_u32              iterations;  /* Number of iterations to execute */
    ee_u32              execs;       /* Bitmask of operations to execute */
    struct list_head_s *list;
    mat_params          mat;
    /* outputs */
    ee_u16 crc;
    ee_u16 crclist;
    ee_u16 crcmatrix;
    ee_u16 crcstate;
    ee_s16 err;
    /* ultithread specific */
    core_portable port;
} core_results;

/* Multicore execution handling */
#if (MULTITHREAD > 1)
ee_u8 core_start_parallel(core_results *res);
ee_u8 core_stop_parallel(core_results *res);
#endif

/* list benchmark functions */
list_head *core_list_init(ee_u32 blksize, list_head *memblock, ee_s16 seed);
ee_u16     core_bench_list(core_results *res, ee_s16 finder_idx);

/* state benchmark functions */
void   core_init_state(ee_u32 size, ee_s16 seed, ee_u8 *p);
ee_u16 core_bench_state(ee_u32 blksize,
                        ee_u8 *memblock,
                        ee_s16 seed1,
                        ee_s16 seed2,
                        ee_s16 step,
                        ee_u16 crc);

/* matrix benchmark functions */
ee_u32 core_init_matrix(ee_u32      blksize,
                        void *      memblk,
                        ee_s32      seed,
                        mat_params *p);
ee_u16 core_bench_matrix(mat_params *p, ee_s16 seed, ee_u16 crc);

#endif  /* FESDK_CORE_MARK_H */

