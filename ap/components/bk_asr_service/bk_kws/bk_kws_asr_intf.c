// Copyright 2023-2024 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0

#include <stdbool.h>
#include <stdint.h>
#include <os/mem.h>
#include <common/bk_include.h>
#include "bk_kws_asr.h"

/* -------- External symbols from kws_model (sdk/ap/properties/.../bk_kws.h) --
 * The bk_kws.h header is owned by an internal-lib component whose include
 * dir is not exposed transitively, so we mirror its declarations here.
 * TODO: promote kws_model include dir into a public REQUIRES so we can just
 *       #include "bk_kws.h" instead of replicating the contract.
 *
 *   bk_kws.cc:320   extern "C" void bk_kws_init();                      (0 args)
 *   bk_kws.cc:360   extern "C" int  bk_tflite_ASR_Recog(short *, int,
 *                                       const char **, float *, int16_t *);
 *   bk_kws.h        uint32_t bk_kws_get_tflm_buf_size(void);
 *                   void     bk_kws_set_tflm_buf(void *buf);
 *                   uint32_t bk_kws_get_npu_scratch_size(void);
 *                   void     bk_kws_set_npu_scratch(void *buf);
 */
extern uint32_t bk_kws_get_tflm_buf_size(void);
extern void     bk_kws_set_tflm_buf(void *buf);
extern uint32_t bk_kws_get_npu_scratch_size(void);
extern void     bk_kws_set_npu_scratch(void *buf);
extern void     bk_kws_init(void);
extern int      bk_tflite_ASR_Recog(short *buf, int buf_len,
                                    const char **text, float *score,
                                    int16_t *result);

/* Alignment requirements specified by bk_kws.h. */
#define KWS_ARENA_ALIGN     32u
#define KWS_SCRATCH_ALIGN   16u
#define ALIGN_UP(p, a)      (((uintptr_t)(p) + ((a)-1u)) & ~((uintptr_t)((a)-1u)))

/* -------- Heap-placement selectors --------------------------------------
 * Bind malloc + free + heap-name as a single set so init() and deinit()
 * stay in lock-step automatically.
 * Priority order (highest first): PSRAM > HSRAM > default (os_malloc).
 * Note: at present os/mem.h aliases psram_free / hsram_free to os_free, but
 * we use the explicit form so this code remains correct if the heaps are
 * ever split into truly separate pools.
 */
#if CONFIG_BEKEN_KWS_ARENA_USE_PSRAM
    #define KWS_ARENA_MALLOC(sz)  psram_malloc(sz)
    #define KWS_ARENA_FREE(p)     psram_free(p)
    #define KWS_ARENA_HEAP_NAME   "PSRAM"
#elif CONFIG_BEKEN_KWS_ARENA_USE_HSRAM
    #define KWS_ARENA_MALLOC(sz)  hsram_malloc(sz)
    #define KWS_ARENA_FREE(p)     hsram_free(p)
    #define KWS_ARENA_HEAP_NAME   "HSRAM"
#else
    #define KWS_ARENA_MALLOC(sz)  os_malloc(sz)
    #define KWS_ARENA_FREE(p)     os_free(p)
    #define KWS_ARENA_HEAP_NAME   "DEFAULT"
#endif

#if CONFIG_BEKEN_KWS_SCRATCH_USE_PSRAM
    #define KWS_SCRATCH_MALLOC(sz)  psram_malloc(sz)
    #define KWS_SCRATCH_FREE(p)     psram_free(p)
    #define KWS_SCRATCH_HEAP_NAME   "PSRAM"
#elif CONFIG_BEKEN_KWS_SCRATCH_USE_HSRAM
    #define KWS_SCRATCH_MALLOC(sz)  hsram_malloc(sz)
    #define KWS_SCRATCH_FREE(p)     hsram_free(p)
    #define KWS_SCRATCH_HEAP_NAME   "HSRAM"
#else
    #define KWS_SCRATCH_MALLOC(sz)  os_malloc(sz)
    #define KWS_SCRATCH_FREE(p)     os_free(p)
    #define KWS_SCRATCH_HEAP_NAME   "DEFAULT"
#endif

/* Caller-owned KWS buffers. Kept at module scope so we can free them later. */
static void *s_kws_tflm_buf_owner    = NULL;   /* raw pointer from psram/os_malloc */
static void *s_kws_npu_scratch_owner = NULL;
static bool  s_kws_initialized       = false;

/* Forward decl for use inside init's error-path. */
void bk_tflite_asr_deinit(void);

int bk_tflite_asr_init(void)
{
    if (s_kws_initialized) {
        BK_LOGW(NULL, "bk_tflite_asr_init: already initialized, skipping\n");
        return 1;
    }

    uint32_t arena_sz   = bk_kws_get_tflm_buf_size();
    uint32_t scratch_sz = bk_kws_get_npu_scratch_size();

    /* Over-allocate by one alignment quantum: the heap allocators do not
     * guarantee the 32/16-byte alignment that the KWS / NPU drivers require. */
    s_kws_tflm_buf_owner    = KWS_ARENA_MALLOC(arena_sz     + KWS_ARENA_ALIGN);
    s_kws_npu_scratch_owner = KWS_SCRATCH_MALLOC(scratch_sz + KWS_SCRATCH_ALIGN);

    if (s_kws_tflm_buf_owner == NULL || s_kws_npu_scratch_owner == NULL) {
        BK_LOGE(NULL, "bk_tflite_asr_init: malloc failed: "
                "arena=%p (%u B from %s), scratch=%p (%u B from %s)\n",
                s_kws_tflm_buf_owner,    (unsigned)arena_sz,   KWS_ARENA_HEAP_NAME,
                s_kws_npu_scratch_owner, (unsigned)scratch_sz, KWS_SCRATCH_HEAP_NAME);
        bk_tflite_asr_deinit();
        return 0;
    }

    void *arena_aligned   = (void *)ALIGN_UP(s_kws_tflm_buf_owner,    KWS_ARENA_ALIGN);
    void *scratch_aligned = (void *)ALIGN_UP(s_kws_npu_scratch_owner, KWS_SCRATCH_ALIGN);

    bk_kws_set_tflm_buf(arena_aligned);
    bk_kws_set_npu_scratch(scratch_aligned);

    bk_kws_init();

    s_kws_initialized = true;
    return 1;
}

/* p1: const char ** receiving keyword text pointer.
 * p2: float       *  receiving recognition score.
 * The void * signature is dictated by the audio engine callback table. */
int bk_tflite_asr_recog(void *read_buf, uint32_t read_size, void *p1, void *p2)
{
    int16_t result = 0;
    int ret = bk_tflite_ASR_Recog((short *)read_buf, (int)read_size,
                                  (const char **)p1, (float *)p2, &result);
    if (ret != 0) {
        /* Upstream signalled an internal error; surface as 0 (no keyword)
         * to keep the legacy "return = keyword index" contract intact. */
        return 0;
    }
    return (int)result;
}

void bk_tflite_asr_deinit(void)
{
    /* Detach pointers in the KWS module first so any stray invoke after
     * deinit hits the NULL guard inside the library instead of dereferencing
     * freed memory. */
    bk_kws_set_tflm_buf(NULL);
    bk_kws_set_npu_scratch(NULL);

    if (s_kws_tflm_buf_owner) {
        KWS_ARENA_FREE(s_kws_tflm_buf_owner);
        s_kws_tflm_buf_owner = NULL;
    }
    if (s_kws_npu_scratch_owner) {
        KWS_SCRATCH_FREE(s_kws_npu_scratch_owner);
        s_kws_npu_scratch_owner = NULL;
    }
    s_kws_initialized = false;
}
