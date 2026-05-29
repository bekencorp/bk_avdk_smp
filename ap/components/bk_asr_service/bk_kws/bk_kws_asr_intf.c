
#include <os/mem.h>
#include <common/bk_err.h>
#include <common/bk_include.h>

void bk_kws_init(void *arg);
int  bk_tflite_ASR_Recog(short *buf, int buf_len, const char **text, float *score, int16_t *result);

/*
 * KWS caller-owned buffer provisioning (forwards to kws_model / bk_kws.h).
 *
 * Usage:
 *   uint32_t arena_sz   = bk_kws_get_tflm_buf_size();
 *   uint32_t scratch_sz = bk_kws_get_npu_scratch_size();
 *   bk_kws_set_tflm_buf(arena_ptr);        // caller-owned, PSRAM, >=32B aligned
 *   bk_kws_set_npu_scratch(scratch_ptr);   // caller-owned, PSRAM, >=16B aligned
 *   bk_kws_init(NULL);
 */
uint32_t bk_kws_get_tflm_buf_size(void);
void     bk_kws_set_tflm_buf(void *buf);
uint32_t bk_kws_get_npu_scratch_size(void);
void     bk_kws_set_npu_scratch(void *buf);

/* Caller-owned KWS buffers. Kept at module scope so we can free them later
 * if a deinit path is ever added; KWS today runs for the whole app lifetime. */
static void *s_kws_tflm_buf_owner    = NULL;   /* raw pointer from psram_malloc */
static void *s_kws_npu_scratch_owner = NULL;

int bk_tflite_asr_init(void)
{
    uint32_t arena_sz   = bk_kws_get_tflm_buf_size();
    uint32_t scratch_sz = bk_kws_get_npu_scratch_size();

    /* Over-allocate so we can align the pointer ourselves: psram_malloc does
     * not guarantee 32/16-byte alignment that the algorithm library requires. */
    #if (CONFIG_BEKEN_KWS_ARENA_USE_PSRAM)
        s_kws_tflm_buf_owner    = psram_malloc(arena_sz   + 32);
    #else
        s_kws_tflm_buf_owner    = os_malloc(arena_sz   + 32);
    #endif
    #if (CONFIG_BEKEN_KWS_SCRATCH_USE_PSRAM)
        s_kws_npu_scratch_owner = psram_malloc(scratch_sz + 16);
    #else
        s_kws_npu_scratch_owner = os_malloc(scratch_sz + 16);
    #endif

    if (s_kws_tflm_buf_owner == NULL || s_kws_npu_scratch_owner == NULL) {
        BK_LOGE(NULL, "kws malloc failed: arena=%p (%u B), scratch=%p (%u B)\n",
             s_kws_tflm_buf_owner, (unsigned)arena_sz,
             s_kws_npu_scratch_owner, (unsigned)scratch_sz);
        if (s_kws_tflm_buf_owner)    { os_free(s_kws_tflm_buf_owner);    s_kws_tflm_buf_owner    = NULL; }
        if (s_kws_npu_scratch_owner) { os_free(s_kws_npu_scratch_owner); s_kws_npu_scratch_owner = NULL; }
        return 0;
    }

    void *arena_aligned   = (void *)(((uintptr_t)s_kws_tflm_buf_owner    + 31u) & ~(uintptr_t)31u);
    void *scratch_aligned = (void *)(((uintptr_t)s_kws_npu_scratch_owner + 15u) & ~(uintptr_t)15u);

    bk_kws_set_tflm_buf(arena_aligned);
    bk_kws_set_npu_scratch(scratch_aligned);

    bk_kws_init(NULL);
    return 1;
}

int bk_tflite_asr_recog(void *read_buf, uint32_t read_size, void *p1, void *p2)
{
    int16_t result = 0;

    bk_tflite_ASR_Recog((short *)read_buf, read_size, p1, p2, &result);

    return result;
}

void bk_tflite_asr_deinit(void)
{
    if (s_kws_tflm_buf_owner)
    {
        os_free(s_kws_tflm_buf_owner);
        s_kws_tflm_buf_owner = NULL;
    }
    if (s_kws_npu_scratch_owner)
    {
        os_free(s_kws_npu_scratch_owner);
        s_kws_npu_scratch_owner = NULL;
    }
}
