// Copyright 2024-2025 Beken
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

/*
 * Hardware H.264 video decoder for the bk_video_player engine.
 *
 * Pipeline overview:
 *
 *   H.264 bitstream (Annex-B) -> H.264 IP (segment / "flexa" mode) -> 16-line
 *   NV12 macroblock segments in HSRAM -> VG-Lite GPU (reads segments via
 *   Flexa bond, optionally rotates/scales, NV12 -> compressed ARGB8888) ->
 *   compressed ARGB8888 frame in HSRAM when available -> video_player engine -> DPU /
 *   DEC400 -> MIPI panel
 *
 * Why not the old full-frame NV12 in PSRAM path: at 1080x1920 NV12 the recon
 * is 3.1 MB written per frame, the CPU 1088->1080 stride repack is another
 * ~6 MB of PSRAM traffic, and the DPU scan-out reads the whole 3.1 MB at
 * the refresh rate. The combined PSRAM bandwidth caps real-world playback
 * around 2-3 fps on the BK7259 M55. Compressed ARGB8888 cuts the per-frame
 * footprint to ~30% of NV12, and the GPU absorbs the rotation/scale so the
 * CPU repack disappears entirely. The H.264 IP only needs to write tiny
 * 16-line segments into HSRAM (a few tens of KB per buffer), which removes
 * the H.264 -> PSRAM bandwidth bottleneck altogether.
 *
 * Synchronization with the engine:
 *
 *   The engine calls decode(in, out) synchronously: it expects a decoded
 *   frame on return. The Flexa+GPU pipeline is inherently asynchronous --
 *   the GPU's frame_display callback fires once VG-Lite finishes compressing
 *   a frame. We bridge the two by:
 *     1) submitting the AU to the H.264 IP via bk_h264_decode_frame()
 *     2) blocking on a counting semaphore that the GPU frame_display callback
 *        signals
 *     3) handing the GPU output buffer (pointer + size) back to the engine
 *        through out_buffer->data
 *   This keeps the existing AV-sync / pacing / drop logic in the engine
 *   intact and only changes the buffer shape from "NV12 in PSRAM" to
 *   "compressed ARGB8888 in HSRAM/PSRAM".
 *
 *   Buffer ownership:
 *     - GPU malloc cb -> hsram_malloc() first, then PSRAM mem_slab fallback
 *     - GPU frame_display cb -> our queue -> engine -> user's
 *       decode_complete_cb -> bk_display_flush(..., allocator-aware free cb)
 *     - LCD eventually calls the output-frame free callback that matches the
 *       allocator used for that frame
 *     - On decode failure / stop the engine's buffer_free_cb uses the same
 *       allocator-aware release path
 *
 * Memory footprint (1080x1920 portrait, segment_height=1MB, segment_number=3,
 * flexa_buff_cnt=3):
 *   - Flexa pp ring (HSRAM):  mb_w * 16 * seg_h * seg_n * 3/2
 *                             = 1088 * 16 * 1 * 3 * 1.5 = ~76 KB
 *   - GPU output (HSRAM):     one or more compressed ARGB frame buffers,
 *                             ~2 MB each for 1080x1920 DEC400 output
 *                             (falls back to PSRAM if HSRAM is exhausted)
 *   - Annex-B work buffer:    grows on demand, ~max_AU_size + SPS/PPS
 *
 * Source: ap/projects/multimedia/h264d_gpu_display_example/ap/src/
 *         h264d_gpu_display_{demo,gpu,display}.c is the reference
 *         standalone application that drove this pipeline first.
 */

 #include "os/os.h"
 #include "os/mem.h"
 
 #include "components/avdk_utils/avdk_check.h"
 #include "components/bk_decode/bk_h264_decode_ctlr.h"
 #include "components/bk_decode/bk_h264_decode_types.h"
 #include "components/bk_flexa_bond.h"
 #include "components/bk_frame_buffer.h"
 #include "components/bk_gpu.h"
 #include "components/bk_gpu_ctlr.h"
 #include "components/bk_gpu_types.h"
 #include "common/avdk_pixel_types.h"
 #include "components/bk_video_player/bk_video_player_types.h"
 #include "components/bk_video_player/video_decoder/bk_video_player_hw_h264_decoder.h"
 
 #define TAG "vp_h264_dec"
 
 #define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
 #define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
 #define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
 #define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
 #define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)
 
 // ---------------------------------------------------------------------------
 // NAL Unit types and AVCC helpers
 // ---------------------------------------------------------------------------
 
 #define H264_NALU_TYPE_SLICE       1   // non-IDR slice (typically P-frame)
 #define H264_NALU_TYPE_IDR         5   // IDR slice (key frame)
 #define H264_NALU_TYPE_SEI         6
 #define H264_NALU_TYPE_SPS         7
 #define H264_NALU_TYPE_PPS         8
 #define H264_NALU_TYPE_AUD         9
 
 #define H264_NALU_HDR_TYPE(b)      ((uint8_t)((b) & 0x1FU))
 
 // Reasonable upper bound for SPS/PPS payload (a single SPS rarely exceeds 256 bytes).
 #define H264_PARAM_SET_MAX_SIZE    1024U
 
 /* Extra bytes alloced past the requested size of the Annex-B work buffer.
  * The vcdec H.264 IP programs the bitstream DMA with an 8-byte aligned base
  * address (potentially BEFORE the actual NALU start) and reads in wider
  * bursts than the bitstream length suggests. Padding the alloc absorbs any
  * over-read so the mem_slab tail magic word is not clobbered. */
 #define HW_H264_FRAME_BUF_SAFETY_PAD_BYTES   (128U)
 
 // ---------------------------------------------------------------------------
 // Flexa + GPU pipeline tunables
 // ---------------------------------------------------------------------------
 
 /* Segment height in macroblocks. 1 MB = 16 pixel rows. Smaller segments
  * lower latency at the cost of more H264<->GPU handshakes. 1 matches the
  * reference h264d_gpu_display_example and works for all tested resolutions. */
 #define H264_DECODER_SEG_HEIGHT_MB          1U
 
 /* Number of segments in the H264->GPU Flexa ring buffer. Acts as a small
  * pipeline depth that lets the H264 IP run ahead of the GPU by N-1 segments.
  * 3 matches the reference example. */
 #define H264_DECODER_SEG_NUMBER             3U
 
 /* GPU-side ring depth (number of in-flight compressed ARGB frames). At least
  * 2 so the GPU does not block waiting for the LCD to release the previous
  * frame; 3 gives the LCD one frame of slack across DPU VSYNC jitter. */
 #define H264_DECODER_GPU_FLEXA_BUFF_CNT     3U
 
 /* How many lines of NV12 the GPU consumes per Flexa transaction. 16 matches
  * H264_DECODER_SEG_HEIGHT_MB * 16; the GPU and H264 IP have to agree on
  * this value. */
 #define H264_DECODER_GPU_FLEXA_LINES        16U
 
 /* HSRAM Flexa ring buffer alignment: 64-byte cache line. */
 #define H264_DECODER_FLEXA_PP_ALIGN         64U
 
 /* Maximum time to wait for the GPU to finish a single frame. Generous: at
  * 30 fps each frame should complete in ~33 ms; the timeout only triggers on
  * pathological stalls. */
 #define H264_DECODER_FRAME_TIMEOUT_MS       2000U
 
 /* Counting semaphore depth. With H264_DECODER_GPU_FLEXA_BUFF_CNT==3 the GPU
  * can have up to 3 frames ready before the decode thread consumes them, so
  * size the semaphore accordingly. */
 #define H264_DECODER_FRAME_SEM_DEPTH        H264_DECODER_GPU_FLEXA_BUFF_CNT
 
 static volatile uint32_t s_h264_decoder_rotate_degree = 0U;
 
 static bool h264_decoder_ptr_is_hsram(const void *ptr)
 {
 #if defined(CONFIG_AP_HSRAM_HEAP_ADDR) && defined(CONFIG_AP_HSRAM_HEAP_SIZE) && (CONFIG_AP_HSRAM_HEAP_SIZE > 0)
     const uintptr_t addr  = (uintptr_t)ptr;
     const uintptr_t start = (uintptr_t)CONFIG_AP_HSRAM_HEAP_ADDR;
     const uintptr_t end   = start + (uintptr_t)CONFIG_AP_HSRAM_HEAP_SIZE;
 
     return (addr >= start && addr < end);
 #else
     (void)ptr;
     return false;
 #endif
 }
 

 bool bk_video_player_hw_h264_decoder_is_hsram_output_frame(const void *frame)
{
    return h264_decoder_ptr_is_hsram(frame);
}

 avdk_err_t bk_video_player_hw_h264_decoder_free_output_frame(void *frame)
 {
     if (frame == NULL)
     {
         return AVDK_ERR_OK;
     }
 
     if (h264_decoder_ptr_is_hsram(frame))
     {
         hsram_free(frame);
     }
     else
     {
         bk_frame_buffer_free(frame);
     }
 
     return AVDK_ERR_OK;
 }
 
 void bk_video_player_hw_h264_decoder_set_output_rotation(uint32_t rotate_degree)
 {
     if (rotate_degree != 0U && rotate_degree != 90U &&
         rotate_degree != 180U && rotate_degree != 270U)
     {
         LOGW("%s: unsupported rotate_degree=%u, falling back to 0\n",
              __func__, rotate_degree);
         rotate_degree = 0U;
     }
     s_h264_decoder_rotate_degree = rotate_degree;
     LOGI("%s: H264 decoder GPU output rotation set to %u\n", __func__, rotate_degree);
 }
 
 // ---------------------------------------------------------------------------
 // Per-instance context
 // ---------------------------------------------------------------------------
 
 typedef struct hw_h264_decoder_ctx_s
 {
     /* Resolved source / GPU output dimensions */
     uint16_t src_w;
     uint16_t src_h;
     uint16_t mb_w;       /* MB-aligned src width  (16 px aligned) */
     uint16_t mb_h;       /* MB-aligned src height (16 px aligned) */
     uint16_t out_w;      /* Post-rotation visible width  (reported to engine) */
     uint16_t out_h;      /* Post-rotation visible height (reported to engine) */
     uint16_t rotate_degree;
     bool     scale_enable;
 
     /* HW handles */
     bk_h264_decode_ctlr_handle_t h264_handle;
     bk_gpu_ctlr_handle_t         gpu_handle;
     void                        *bond;
 
     /* H.264 Flexa -> GPU segment ring */
     void     *flexa_pp_raw;
     uint8_t  *flexa_pp_buf;
     uint32_t  flexa_pp_size;
 
     /* GPU output frame queue */
     beken_mutex_t     pending_mutex;
     beken_semaphore_t frame_ready_sem;
     void             *pending_frames[H264_DECODER_GPU_FLEXA_BUFF_CNT];
     uint32_t          pending_sizes [H264_DECODER_GPU_FLEXA_BUFF_CNT];
     uint8_t           pending_head;   /* next pop slot */
     uint8_t           pending_tail;   /* next push slot */
     uint32_t          pending_dropped_count;
 
     /* avcC / SPS-PPS / Annex-B work buffer (same machinery as the old
      * frame-mode path; bitstream handling is independent of the recon path). */
     uint8_t  *sps_data;
     uint16_t  sps_size;
     uint8_t  *pps_data;
     uint16_t  pps_size;
     uint8_t   nalu_length_size;     /* 1, 2 or 4. Defaults to 4 when no avcC. */
     bool      need_inject_params;
     uint8_t  *annexb_buf;
     uint32_t  annexb_buf_size;
 
     video_player_video_params_t video_params;
     bool                        is_initialized;
 } hw_h264_decoder_ctx_t;
 
 typedef struct
 {
     video_player_video_decoder_ops_t ops;
     hw_h264_decoder_ctx_t            ctx;
 } hw_h264_decoder_instance_t;
 
 static video_player_video_decoder_ops_t s_ops_template;
 
 static avdk_err_t hw_h264_decoder_deinit(struct video_player_video_decoder_ops_s *ops);
 
 static hw_h264_decoder_ctx_t * volatile s_active_ctx = NULL;
 
 // ---------------------------------------------------------------------------
 // HSRAM aligned alloc helpers (lifted verbatim from h264d_gpu_display_demo)
 // ---------------------------------------------------------------------------
 
 static void *h264_decoder_hsram_aligned_malloc(uint32_t alignment, uint32_t size, void **raw_out)
 {
     void *raw;
     uint32_t total;
     uintptr_t start;
     uintptr_t aligned;
 
     if (alignment < (uint32_t)sizeof(void *))
     {
         alignment = (uint32_t)sizeof(void *);
     }
     if ((alignment & (alignment - 1U)) != 0U)
     {
         return NULL;
     }
 
     total = size + alignment - 1U + (uint32_t)sizeof(void *);
     raw = hsram_malloc(total);
     if (raw == NULL)
     {
         return NULL;
     }
 
     start   = (uintptr_t)raw + sizeof(void *);
     aligned = (start + (alignment - 1U)) & ~((uintptr_t)alignment - 1U);
     ((void **)aligned)[-1] = raw;
 
     if (raw_out != NULL)
     {
         *raw_out = raw;
     }
     return (void *)aligned;
 }
 
 static void h264_decoder_hsram_aligned_free(void *raw)
 {
     if (raw != NULL)
     {
        hsram_free(raw);
     }
 }
 
 // ---------------------------------------------------------------------------
 // avcC parsing (extract SPS/PPS + nalu length size). Unchanged from the
 // previous frame-mode path -- this is independent of the recon pipeline.
 // ---------------------------------------------------------------------------
 
 static void hw_h264_release_param_sets(hw_h264_decoder_ctx_t *ctx)
 {
     if (ctx->sps_data != NULL)
     {
         os_free(ctx->sps_data);
         ctx->sps_data = NULL;
     }
     ctx->sps_size = 0;
 
     if (ctx->pps_data != NULL)
     {
         os_free(ctx->pps_data);
         ctx->pps_data = NULL;
     }
     ctx->pps_size = 0;
 }
 
 static avdk_err_t hw_h264_parse_avcc(hw_h264_decoder_ctx_t *ctx,
                                      const uint8_t *cfg, uint32_t cfg_size)
 {
     AVDK_RETURN_ON_FALSE(ctx && cfg, AVDK_ERR_INVAL, TAG, "invalid avcC args");
     AVDK_RETURN_ON_FALSE(cfg_size >= 7U, AVDK_ERR_INVAL, TAG, "avcC too short: %u", cfg_size);
     AVDK_RETURN_ON_FALSE(cfg[0] == 0x01, AVDK_ERR_INVAL, TAG, "avcC bad version: 0x%02x", cfg[0]);
 
     uint32_t off = 4U;
     const uint8_t length_size = (cfg[off++] & 0x03U) + 1U;
     if (length_size != 1U && length_size != 2U && length_size != 4U)
     {
         LOGE("%s: avcC bad lengthSize=%u\n", __func__, length_size);
         return AVDK_ERR_UNSUPPORTED;
     }
     ctx->nalu_length_size = length_size;
 
     /* SPS */
     const uint8_t num_sps = cfg[off++] & 0x1FU;
     if (num_sps == 0U)
     {
         LOGE("%s: avcC has 0 SPS\n", __func__);
         return AVDK_ERR_INVAL;
     }
     if (num_sps > 1U)
     {
         LOGW("%s: avcC has %u SPS, only the first one will be used\n", __func__, num_sps);
     }
 
     for (uint32_t i = 0; i < num_sps; i++)
     {
         if (off + 2U > cfg_size) return AVDK_ERR_INVAL;
         const uint16_t len = (uint16_t)(((uint16_t)cfg[off] << 8) | (uint16_t)cfg[off + 1U]);
         off += 2U;
         if (len == 0U || len > H264_PARAM_SET_MAX_SIZE || off + len > cfg_size) return AVDK_ERR_INVAL;
         if (i == 0U)
         {
             ctx->sps_data = (uint8_t *)os_malloc(len);
             if (ctx->sps_data == NULL) return AVDK_ERR_NOMEM;
             os_memcpy(ctx->sps_data, &cfg[off], len);
             ctx->sps_size = len;
         }
         off += len;
     }
 
     /* PPS */
     if (off >= cfg_size) return AVDK_ERR_INVAL;
     const uint8_t num_pps = cfg[off++];
     if (num_pps == 0U)
     {
         LOGE("%s: avcC has 0 PPS\n", __func__);
         return AVDK_ERR_INVAL;
     }
     if (num_pps > 1U)
     {
         LOGW("%s: avcC has %u PPS, only the first one will be used\n", __func__, num_pps);
     }
 
     for (uint32_t i = 0; i < num_pps; i++)
     {
         if (off + 2U > cfg_size) return AVDK_ERR_INVAL;
         const uint16_t len = (uint16_t)(((uint16_t)cfg[off] << 8) | (uint16_t)cfg[off + 1U]);
         off += 2U;
         if (len == 0U || len > H264_PARAM_SET_MAX_SIZE || off + len > cfg_size) return AVDK_ERR_INVAL;
         if (i == 0U)
         {
             ctx->pps_data = (uint8_t *)os_malloc(len);
             if (ctx->pps_data == NULL) return AVDK_ERR_NOMEM;
             os_memcpy(ctx->pps_data, &cfg[off], len);
             ctx->pps_size = len;
         }
         off += len;
     }
 
     LOGI("%s: avcC parsed: lengthSize=%u sps_size=%u pps_size=%u\n",
          __func__, ctx->nalu_length_size, ctx->sps_size, ctx->pps_size);
     return AVDK_ERR_OK;
 }
 
 // ---------------------------------------------------------------------------
 // Annex-B / AVCC handling. Unchanged from the previous frame-mode path.
 // ---------------------------------------------------------------------------
 
 static bool hw_h264_buffer_is_annex_b(const uint8_t *buf, uint32_t len)
 {
     const uint32_t scan = (len > 64U) ? 64U : len;
     for (uint32_t i = 0; i + 3U < scan; i++)
     {
         if (buf[i] == 0x00 && buf[i + 1U] == 0x00)
         {
             if (buf[i + 2U] == 0x01) return true;
             if (buf[i + 2U] == 0x00 && buf[i + 3U] == 0x01) return true;
         }
     }
     return false;
 }
 
 static bool hw_h264_au_contains_idr_annexb(const uint8_t *data, uint32_t len)
 {
     if (data == NULL || len < 5U) return false;
     uint32_t i = 0U;
     uint32_t nal_count = 0U;
     while (i + 3U < len)
     {
         if (data[i] == 0x00U && data[i + 1U] == 0x00U)
         {
             uint32_t prefix = 0U;
             if (data[i + 2U] == 0x01U)
             {
                 prefix = 3U;
             }
             else if (i + 3U < len && data[i + 2U] == 0x00U && data[i + 3U] == 0x01U)
             {
                 prefix = 4U;
             }
 
             if (prefix > 0U && (i + prefix) < len)
             {
                 const uint8_t nal_header = data[i + prefix];
                 const uint8_t nal_type   = nal_header & 0x1FU;
                 if (nal_type == 5U) return true;
                 i += prefix + 1U;
                 if (++nal_count >= 32U) return false;
                 continue;
             }
         }
         i++;
     }
     return false;
 }
 
 static bool hw_h264_au_contains_idr_avcc(const uint8_t *data, uint32_t len,
                                          uint8_t length_size)
 {
     if (data == NULL || len < (uint32_t)(length_size + 1U)) return false;
     if (length_size != 1U && length_size != 2U && length_size != 4U) return false;
 
     uint32_t i = 0U;
     uint32_t nal_count = 0U;
     while (i + length_size < len)
     {
         uint32_t nal_len = 0U;
         for (uint8_t k = 0U; k < length_size; k++)
         {
             nal_len = (nal_len << 8) | (uint32_t)data[i + k];
         }
         i += length_size;
         if (nal_len == 0U || i >= len) break;
 
         const uint8_t nal_header = data[i];
         const uint8_t nal_type   = nal_header & 0x1FU;
         if (nal_type == 5U) return true;
 
         if (nal_len > (len - i)) break; /* truncated AU; bail */
         i += nal_len;
 
         if (++nal_count >= 32U) return false;
     }
     return false;
 }
 
 static avdk_err_t hw_h264_ensure_annexb_buf(hw_h264_decoder_ctx_t *ctx, uint32_t need)
{
    if (ctx->annexb_buf != NULL && ctx->annexb_buf_size >= need)
    {
        return AVDK_ERR_OK;
    }

    if (ctx->annexb_buf != NULL)
    {
        bk_frame_buffer_free(ctx->annexb_buf);
        ctx->annexb_buf = NULL;
        ctx->annexb_buf_size = 0;
    }

    const uint32_t alloc_size = need + HW_H264_FRAME_BUF_SAFETY_PAD_BYTES;
    ctx->annexb_buf = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, alloc_size);
    if (ctx->annexb_buf == NULL)
    {
        LOGE("%s: alloc annexb buffer failed, need=%u\n", __func__, alloc_size);
        return AVDK_ERR_NOMEM;
    }
    ctx->annexb_buf_size = need;
    return AVDK_ERR_OK;
}
 
 static inline avdk_err_t hw_h264_append_nalu(hw_h264_decoder_ctx_t *ctx,
                                              uint32_t *out,
                                              const uint8_t *nalu, uint32_t nalu_len)
{
    if (*out + 4U + nalu_len > ctx->annexb_buf_size)
    {
        LOGE("%s: annexb buffer overflow, need=%u have=%u\n",
            __func__, *out + 4U + nalu_len, ctx->annexb_buf_size);
        return AVDK_ERR_NOMEM;
    }
    ctx->annexb_buf[(*out)++] = 0x00;
    ctx->annexb_buf[(*out)++] = 0x00;
    ctx->annexb_buf[(*out)++] = 0x00;
    ctx->annexb_buf[(*out)++] = 0x01;
    os_memcpy(&ctx->annexb_buf[*out], nalu, nalu_len);
    *out += nalu_len;
    return AVDK_ERR_OK;
}
 
static avdk_err_t hw_h264_avcc_to_annexb(hw_h264_decoder_ctx_t *ctx,
                                        const uint8_t *src, uint32_t src_len,
                                        bool inject_param_sets,
                                        uint32_t *out_len)
{
    AVDK_RETURN_ON_FALSE(ctx && src && out_len, AVDK_ERR_INVAL, TAG, "invalid args");

    const uint32_t reserve = src_len
                        + (src_len / 4U)
                        + ctx->sps_size + ctx->pps_size + 32U;
    avdk_err_t ret = hw_h264_ensure_annexb_buf(ctx, reserve);
    if (ret != AVDK_ERR_OK) return ret;

    const uint8_t length_size = ctx->nalu_length_size;
    if (length_size != 1U && length_size != 2U && length_size != 4U)
    {
        LOGE("%s: invalid nalu_length_size=%u\n", __func__, length_size);
        return AVDK_ERR_INVAL;
    }

    bool has_idr = false;
    bool has_inband_sps = false;
    bool has_inband_pps = false;
    {
        uint32_t scan_in = 0;
        while (scan_in + length_size <= src_len)
        {
            uint32_t nalu_len = 0;
            for (uint8_t k = 0; k < length_size; k++)
            {
                nalu_len = (nalu_len << 8) | src[scan_in + k];
            }
            scan_in += length_size;
            if (nalu_len == 0U || nalu_len > (src_len - scan_in)) break;
            const uint8_t type = H264_NALU_HDR_TYPE(src[scan_in]);
            if (type == H264_NALU_TYPE_IDR) has_idr = true;
            else if (type == H264_NALU_TYPE_SPS) has_inband_sps = true;
            else if (type == H264_NALU_TYPE_PPS) has_inband_pps = true;
            scan_in += nalu_len;
        }
    }

    const bool sample_self_contained = (has_inband_sps && has_inband_pps);
    const bool do_inject = (inject_param_sets || has_idr)
                       && (ctx->sps_size > 0U) && (ctx->pps_size > 0U)
                       && !sample_self_contained;

    uint32_t out = 0;
    if (do_inject)
    {
        ret = hw_h264_append_nalu(ctx, &out, ctx->sps_data, ctx->sps_size);
        if (ret != AVDK_ERR_OK) return ret;
        ret = hw_h264_append_nalu(ctx, &out, ctx->pps_data, ctx->pps_size);
        if (ret != AVDK_ERR_OK) return ret;
    }

    uint32_t in = 0;
    while (in + length_size <= src_len)
    {
        uint32_t nalu_len = 0;
        for (uint8_t k = 0; k < length_size; k++)
        {
            nalu_len = (nalu_len << 8) | src[in + k];
        }
        in += length_size;

        if (nalu_len == 0U || nalu_len > (src_len - in))
        {
            LOGE("%s: bad NALU length=%u remaining=%u\n", __func__, nalu_len, src_len - in);
            return AVDK_ERR_INVAL;
        }

        ret = hw_h264_append_nalu(ctx, &out, &src[in], nalu_len);
        if (ret != AVDK_ERR_OK) return ret;
        in += nalu_len;
    }

    if (in != src_len)
    {
        LOGW("%s: trailing %u bytes ignored\n", __func__, src_len - in);
    }

    *out_len = out;

    if (has_idr && ((do_inject) || sample_self_contained))
    {
        ctx->need_inject_params = false;
    }

    return AVDK_ERR_OK;
}
 
 // ---------------------------------------------------------------------------
 // GPU buffer-management glue: callbacks the bk_gpu_ctlr invokes.
 // ---------------------------------------------------------------------------
 
static void *h264_decoder_gpu_frame_malloc(uint32_t size)
{
    /* The GPU produces compressed ARGB frames sized exactly to its tile
    * compressed payload (not strict width*height*4 -- VG-Lite emits ~30%
    * of that). MEM_SLAB_HEAP_UNCODED is the same heap the existing video
    * path uses for decoded frames, so the downstream LCD free path
    * (display_frame_free_cb -> bk_frame_buffer_free) Just Works without
    * any allocator translation. */
    void *ptr = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, size);
    if (ptr == NULL)
    {
        LOGE("%s: alloc gpu output failed, size=%u\n", __func__, (unsigned)size);
    }
    return ptr;
}

static avdk_err_t h264_decoder_gpu_frame_free(void *ptr)
{
    return bk_video_player_hw_h264_decoder_free_output_frame(ptr);
    }

static void h264_decoder_gpu_frame_display(void *frame, uint32_t frame_size, void *args)
{
    /* The GPU controller invokes this once VG-Lite has finished compressing a
    * full frame. Push it into the ring and wake the decode thread. We deliberately
    * do NOT call bk_display_flush() here (unlike the standalone
    * h264d_gpu_display_example): the engine still owns AV-sync and frame pacing,
    * so we forward the frame back to it through the normal decoder ops contract. */
    (void)args;
    hw_h264_decoder_ctx_t *ctx = s_active_ctx;

    if (ctx == NULL || frame == NULL)
    {
        if (frame != NULL)
        {
            (void)h264_decoder_gpu_frame_free(frame);
        }
        return;
    }

    bool dropped = false;
    if (ctx->pending_mutex != NULL && rtos_lock_mutex(&ctx->pending_mutex) == BK_OK)
    {
        const uint8_t next_tail = (uint8_t)((ctx->pending_tail + 1U) % H264_DECODER_GPU_FLEXA_BUFF_CNT);
        if (next_tail == ctx->pending_head)
        {
            /* FIFO full. Shouldn't happen in steady state because the engine
            * pops one frame per decode() and we only have flexa_buff_cnt
            * frames in flight, but defend against producer racing past the
            * consumer (e.g. after a long pacing wait). */
            dropped = true;
            ctx->pending_dropped_count++;
        }
        else
        {
            ctx->pending_frames[ctx->pending_tail] = frame;
            ctx->pending_sizes [ctx->pending_tail] = frame_size;
            ctx->pending_tail = next_tail;
        }
        (void)rtos_unlock_mutex(&ctx->pending_mutex);
    }
    else
    {
        /* No mutex means we are racing teardown; drop the frame. */
        dropped = true;
    }

    if (dropped)
    {
        LOGW("%s: dropping gpu frame (queue full or no mutex)\n", __func__);
        (void)h264_decoder_gpu_frame_free(frame);
        return;
    }

    if (ctx->frame_ready_sem != NULL)
    {
        (void)rtos_set_semaphore(&ctx->frame_ready_sem);
    }
}

static bool h264_decoder_pop_pending(hw_h264_decoder_ctx_t *ctx,
                                    void **frame_out, uint32_t *frame_size_out)
{
    bool got = false;
    if (ctx->pending_mutex == NULL)
    {
        return false;
    }
    if (rtos_lock_mutex(&ctx->pending_mutex) != BK_OK)
    {
        return false;
    }
    if (ctx->pending_head != ctx->pending_tail)
    {
        *frame_out      = ctx->pending_frames[ctx->pending_head];
        *frame_size_out = ctx->pending_sizes [ctx->pending_head];
        ctx->pending_frames[ctx->pending_head] = NULL;
        ctx->pending_sizes [ctx->pending_head] = 0;
        ctx->pending_head = (uint8_t)((ctx->pending_head + 1U) % H264_DECODER_GPU_FLEXA_BUFF_CNT);
        got = true;
    }
    (void)rtos_unlock_mutex(&ctx->pending_mutex);
    return got;
}

static void h264_decoder_drain_pending(hw_h264_decoder_ctx_t *ctx)
{
    void    *frame = NULL;
    uint32_t size  = 0;
    while (h264_decoder_pop_pending(ctx, &frame, &size))
    {
        (void)h264_decoder_gpu_frame_free(frame);
    }
}

// ---------------------------------------------------------------------------
// Pipeline lifecycle: bring up Flexa+GPU+H264 and tear it down.
// ---------------------------------------------------------------------------

static void hw_h264_decoder_dec_frame_done(int status, void *args)
{
    (void)args;
    if (status != BK_OK)
    {
        LOGW("%s: h264 frame done status=%d\n", __func__, status);
    }
}

static void hw_h264_decoder_dec_flexa_done(uint32_t wr_ptr, void *args)
{
    (void)args;
    (void)wr_ptr;
}

static uint32_t hw_h264_calc_flexa_pp_size(uint16_t mb_w)
{
    /* mb_w * 16 (line per MB) * seg_h_mb (MBs per segment row) *
    *   seg_n (number of segments) * 3/2 (NV12 Y + UV) */
    return (uint32_t)mb_w * 16U *
        H264_DECODER_SEG_HEIGHT_MB *
        H264_DECODER_SEG_NUMBER *
        3U / 2U;
}

/* Panel dimensions the GPU output must match exactly so the DEC400 tile grid
* the DPU walks at scan-out lines up with the tile grid the GPU wrote.
* Hard-coded for the current hx8399c_mipi_1080x1920 board; if more boards
* appear later this should become a set/get pair injected by the app layer
* (mirroring bk_video_player_hw_h264_decoder_set_output_rotation). */
#define H264_DECODER_PANEL_WIDTH   1080U
#define H264_DECODER_PANEL_HEIGHT  1920U

static void hw_h264_decoder_resolve_dims(hw_h264_decoder_ctx_t *ctx,
                                        uint16_t width, uint16_t height,
                                        uint32_t rotate_degree)
{
    ctx->src_w = width;
    ctx->src_h = height;
    /* MB-aligned dims = 16-aligned. These are what the H.264 IP actually
    * writes because the recon surface is made of 16x16 macroblocks. */
    ctx->mb_w  = (uint16_t)((width  + 15U) & ~15U);
    ctx->mb_h  = (uint16_t)((height + 15U) & ~15U);

    /* Auto-pick a rotation that makes the post-rotation source orientation
    * match the panel. The GPU's vg_lite + Flexa pipeline cannot reliably do
    * a non-uniform scale (e.g. 1.76x in X with 0.57x in Y) when combined
    * with rotate+compress: with scale_y < 1 the H264 IP fills the Flexa
    * ring much faster than the GPU can drain it and the H264 watchdog fires
    * (irq_status=0x401 / "decode timeout"). Forcing the rotation to match
    * orientations keeps the scale factors close to 1.0 in BOTH axes.
    *
    * The externally-set rotation (set_output_rotation) is treated as a
    * preference: we honor it only when src and panel already share an
    * orientation (so picking 0 vs 180 / 90 vs 270 is purely cosmetic). When
    * orientations differ we override with 90 and warn so the misconfig is
    * visible in the log. */
    const bool src_landscape   = (width  > height);
    const bool panel_landscape =
        (H264_DECODER_PANEL_WIDTH > H264_DECODER_PANEL_HEIGHT);

    uint32_t effective_rotate = rotate_degree;
    if (src_landscape != panel_landscape)
    {
        if (effective_rotate != 90U && effective_rotate != 270U)
        {
            LOGW("%s: src %ux%u (%s) on panel %ux%u (%s) requires 90/270 rotation, "
                "overriding requested rotate_degree=%u with 90\n",
                __func__, width, height, src_landscape ? "landscape" : "portrait",
                H264_DECODER_PANEL_WIDTH, H264_DECODER_PANEL_HEIGHT,
                panel_landscape ? "landscape" : "portrait",
                (unsigned)effective_rotate);
            effective_rotate = 90U;
        }
    }
    else
    {
        if (effective_rotate == 90U || effective_rotate == 270U)
        {
            LOGW("%s: src %ux%u and panel %ux%u share orientation, "
                "overriding requested rotate_degree=%u with 0\n",
                __func__, width, height,
                H264_DECODER_PANEL_WIDTH, H264_DECODER_PANEL_HEIGHT,
                (unsigned)effective_rotate);
            effective_rotate = 0U;
        }
    }
    ctx->rotate_degree = (uint16_t)effective_rotate;

    /* Visible dims reported to the engine are the post-rotation source dims.
    * (The actual GPU output buffer is mb-aligned, see setup_pipeline.) */
    if (ctx->rotate_degree == 90U || ctx->rotate_degree == 270U)
    {
        ctx->out_w = height;
        ctx->out_h = width;
    }
    else
    {
        ctx->out_w = width;
        ctx->out_h = height;
    }

    /* IMPORTANT: must stay false in (compress=true) mode. DEC400 requires
    * the compressed render target to be 16-aligned in BOTH dimensions; any
    * non-16-aligned dst (e.g. 1080) makes vg_lite_set_render_target() fail
    * with "dec align error" and the GPU produces no usable frames.
    *
    * To keep the buffer 16-aligned we leave dst == src_mb (both are 16-px
    * aligned by construction in mb_w/mb_h). The 8-px MB padding on the
    * right side is tolerated by the DPU at scan-out (same as the legacy
    * 1920x1080 -> 1088x1920 path that has always worked on this panel). */
    ctx->scale_enable = false;
}

static avdk_err_t hw_h264_decoder_setup_pipeline(hw_h264_decoder_ctx_t *ctx)
{
    avdk_err_t ret;

    /* HSRAM Flexa pp ring buffer. Zero it so the H264 IP's initial write
    * does not race a stale-data read by the GPU. */
    ctx->flexa_pp_size = hw_h264_calc_flexa_pp_size(ctx->mb_w);
    ctx->flexa_pp_buf  = h264_decoder_hsram_aligned_malloc(
                            H264_DECODER_FLEXA_PP_ALIGN,
                            ctx->flexa_pp_size,
                            &ctx->flexa_pp_raw);
    if (ctx->flexa_pp_buf == NULL)
    {
        LOGE("%s: hsram_malloc(%u) for flexa pp ring failed\n",
            __func__, ctx->flexa_pp_size);
        return AVDK_ERR_NOMEM;
    }
    os_memset(ctx->flexa_pp_buf, 0, ctx->flexa_pp_size);
    LOGI("%s: flexa pp ring=%p size=%u aligned64=%u\n",
        __func__, ctx->flexa_pp_buf, (unsigned)ctx->flexa_pp_size,
        ((uintptr_t)ctx->flexa_pp_buf & 0x3FU) == 0U ? 1U : 0U);

    /* dst must stay 16-aligned for DEC400 compression. We use the MB-aligned
    * source dims directly: scale=false (set in resolve_dims) means the
    * GPU does not actually scale, and the H264 IP recon write stride is
    * already mb_w wide, so passing dst=mb keeps the GPU's Flexa segment
    * stride identical to the H264 IP's segment stride. Any non-16 dst
    * (e.g. dst=1080) would trip vg_lite "dec align error". */
    const uint16_t dst_w = ctx->mb_w;
    const uint16_t dst_h = ctx->mb_h;

    bk_gpu_ctlr_config_t gpu_cfg;
    os_memset(&gpu_cfg, 0, sizeof(gpu_cfg));
    gpu_cfg.rotate_degree     = ctx->rotate_degree;
    gpu_cfg.src_width         = ctx->mb_w;
    gpu_cfg.src_height        = ctx->mb_h;
    gpu_cfg.dst_width         = dst_w;
    gpu_cfg.dst_height        = dst_h;
    gpu_cfg.src_format        = BK_PIXEL_FORMAT_NV12;
    gpu_cfg.dst_format        = BK_PIXEL_FORMAT_ARGB8888;
    gpu_cfg.scale             = ctx->scale_enable;
    gpu_cfg.compress          = true;   /* DEC400 tile compressed for DPU */
    gpu_cfg.src_buffer        = ctx->flexa_pp_buf;
    gpu_cfg.flexa             = true;
    gpu_cfg.flexa_lines       = H264_DECODER_GPU_FLEXA_LINES;
    gpu_cfg.flexa_buff_cnt    = H264_DECODER_GPU_FLEXA_BUFF_CNT;
    gpu_cfg.frame_malloc        = h264_decoder_gpu_frame_malloc;
    gpu_cfg.frame_free          = h264_decoder_gpu_frame_free;
    gpu_cfg.flexa_line_done     = NULL;   /* engine does not consume line ticks */
    gpu_cfg.flexa_line_done_args = NULL;
    gpu_cfg.frame_done          = h264_decoder_gpu_frame_display;
    gpu_cfg.frame_done_args     = NULL;

    LOGI("%s: gpu cfg src=%ux%u dst=%ux%u rotate=%u compress=%u scale=%u flexa_lines=%u flexa_buf_cnt=%u\n",
        __func__,
        (unsigned)gpu_cfg.src_width,  (unsigned)gpu_cfg.src_height,
        (unsigned)gpu_cfg.dst_width,  (unsigned)gpu_cfg.dst_height,
        (unsigned)gpu_cfg.rotate_degree,
        (unsigned)gpu_cfg.compress,   (unsigned)gpu_cfg.scale,
        (unsigned)gpu_cfg.flexa_lines,(unsigned)gpu_cfg.flexa_buff_cnt);

    ret = bk_gpu_ctlr_new(&ctx->gpu_handle, &gpu_cfg);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: bk_gpu_ctlr_new ret=%d\n", __func__, ret);
        goto fail_gpu_new;
    }
    ret = bk_gpu_init(ctx->gpu_handle);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: bk_gpu_init ret=%d\n", __func__, ret);
        goto fail_gpu_init;
    }
    ret = bk_gpu_open(ctx->gpu_handle);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: bk_gpu_open ret=%d\n", __func__, ret);
        goto fail_gpu_open;
    }

    /* H.264 Flexa controller. out_width / out_height are the recon buffer
    * stride the IP writes to memory, NOT the encoded visible resolution
    * (that is carried in the SPS we inject). The IP writes whole 16x16
    * macroblocks, so the stride must be MB-aligned; passing mb_w/mb_h
    * also keeps the Flexa segment stride identical to GPU src_width. */
    bk_h264_decode_flexa_config_t dec_cfg = DEFAULT_H264_DECODE_FLEXA_CONFIG;
    dec_cfg.timeout_ms     = 1000U;
    dec_cfg.out_width      = ctx->mb_w;
    dec_cfg.out_height     = ctx->mb_h;
    dec_cfg.out_format     = BK_PIXEL_FORMAT_NV12;
    dec_cfg.segment_height = H264_DECODER_SEG_HEIGHT_MB;
    dec_cfg.segment_number = H264_DECODER_SEG_NUMBER;
    dec_cfg.frame_done_cb  = hw_h264_decoder_dec_frame_done;
    dec_cfg.frame_done_args= NULL;
    dec_cfg.flexa_done_cb  = hw_h264_decoder_dec_flexa_done;
    dec_cfg.flexa_done_args= NULL;

    ret = bk_h264_decode_flexa_ctlr_new(&ctx->h264_handle, &dec_cfg);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: bk_h264_decode_flexa_ctlr_new ret=%d\n", __func__, ret);
        goto fail_dec_new;
    }
    ret = bk_h264_decode_init(ctx->h264_handle);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: bk_h264_decode_init ret=%d\n", __func__, ret);
        goto fail_dec_init;
    }
    ret = bk_h264_decode_open(ctx->h264_handle);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: bk_h264_decode_open ret=%d\n", __func__, ret);
        goto fail_dec_open;
    }

    /* Bond Flexa output of the H.264 IP to the GPU's Flexa input. After
    * this returns, every segment the H.264 IP writes will be consumed by
    * the GPU automatically -- we only need to keep feeding AUs. */
    ret = bk_flexa_h264d_gpu_bond_start(&ctx->bond, ctx->h264_handle, ctx->gpu_handle);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: bk_flexa_h264d_gpu_bond_start ret=%d\n", __func__, ret);
        goto fail_bond;
    }

    ctx->need_inject_params = true;
    LOGI("%s: pipeline up: src=%ux%u (mb=%ux%u) out=%ux%u rotate=%u\n",
        __func__, ctx->src_w, ctx->src_h, ctx->mb_w, ctx->mb_h,
        ctx->out_w, ctx->out_h, ctx->rotate_degree);
    return AVDK_ERR_OK;

fail_bond:
    (void)bk_h264_decode_close(ctx->h264_handle);
fail_dec_open:
    (void)bk_h264_decode_deinit(ctx->h264_handle);
fail_dec_init:
    (void)bk_h264_decode_delete(ctx->h264_handle);
    ctx->h264_handle = NULL;
fail_dec_new:
    (void)bk_gpu_close(ctx->gpu_handle);
fail_gpu_open:
    (void)bk_gpu_deinit(ctx->gpu_handle);
fail_gpu_init:
    (void)bk_gpu_delete(ctx->gpu_handle);
    ctx->gpu_handle = NULL;
fail_gpu_new:
    if (ctx->flexa_pp_raw != NULL)
    {
        h264_decoder_hsram_aligned_free(ctx->flexa_pp_raw);
        ctx->flexa_pp_raw = NULL;
        ctx->flexa_pp_buf = NULL;
        ctx->flexa_pp_size = 0;
    }
    return ret;
}

static void hw_h264_decoder_teardown_pipeline(hw_h264_decoder_ctx_t *ctx)
{
    if (ctx->bond != NULL)
    {
        bk_flexa_h264d_gpu_bond_stop(ctx->bond);
        ctx->bond = NULL;
    }
    if (ctx->h264_handle != NULL)
    {
        (void)bk_h264_decode_close(ctx->h264_handle);
        (void)bk_h264_decode_deinit(ctx->h264_handle);
        (void)bk_h264_decode_delete(ctx->h264_handle);
        ctx->h264_handle = NULL;
    }
    if (ctx->gpu_handle != NULL)
    {
        (void)bk_gpu_close(ctx->gpu_handle);
        (void)bk_gpu_deinit(ctx->gpu_handle);
        (void)bk_gpu_delete(ctx->gpu_handle);
        ctx->gpu_handle = NULL;
    }

    /* Drain any frames the GPU produced after we stopped accepting them. */
    h264_decoder_drain_pending(ctx);

    if (ctx->flexa_pp_raw != NULL)
    {
        h264_decoder_hsram_aligned_free(ctx->flexa_pp_raw);
        ctx->flexa_pp_raw  = NULL;
        ctx->flexa_pp_buf  = NULL;
        ctx->flexa_pp_size = 0;
    }
}

// ---------------------------------------------------------------------------
// Decoder ops implementation
// ---------------------------------------------------------------------------

static avdk_err_t hw_h264_decoder_get_supported_formats(struct video_player_video_decoder_ops_s *ops,
                                                        const video_player_video_format_t **formats,
                                                        uint32_t *format_count)
{
    (void)ops;
    if (formats == NULL || format_count == NULL) return AVDK_ERR_INVAL;
    static const video_player_video_format_t s_formats[] = {
        VIDEO_PLAYER_VIDEO_FORMAT_H264,
    };
    *formats = s_formats;
    *format_count = (uint32_t)(sizeof(s_formats) / sizeof(s_formats[0]));
    return AVDK_ERR_OK;
}

static avdk_err_t hw_h264_decoder_init(struct video_player_video_decoder_ops_s *ops,
                                    video_player_video_params_t *params)
{
    hw_h264_decoder_instance_t *self = __containerof(ops, hw_h264_decoder_instance_t, ops);
    AVDK_RETURN_ON_FALSE(self,   AVDK_ERR_INVAL, TAG, "instance is NULL");
    AVDK_RETURN_ON_FALSE(params, AVDK_ERR_INVAL, TAG, "params is NULL");
    hw_h264_decoder_ctx_t *ctx = &self->ctx;

    LOGI("%s: width=%u height=%u format=%u fps=%u codec_cfg=%p sz=%u\n",
        __func__, params->width, params->height, params->format, params->fps,
        params->codec_config, params->codec_config_size);

    if (params->format != VIDEO_PLAYER_VIDEO_FORMAT_H264)
    {
        LOGW("%s: HW H264 decoder only supports H264 format (got %u)\n", __func__, params->format);
        return AVDK_ERR_UNSUPPORTED;
    }
    if (params->width == 0 || params->height == 0)
    {
        LOGE("%s: invalid size %ux%u\n", __func__, params->width, params->height);
        return AVDK_ERR_INVAL;
    }
    if ((params->width & 1U) || (params->height & 1U))
    {
        LOGW("%s: GPU pipeline requires even width/height, got %ux%u\n",
            __func__, params->width, params->height);
        return AVDK_ERR_UNSUPPORTED;
    }
    if (s_active_ctx != NULL)
    {
        LOGE("%s: another H264-GPU decoder instance is already active\n", __func__);
        return AVDK_ERR_BUSY;
    }

    if (ctx->is_initialized)
    {
        LOGW("%s: already initialized, deinit first\n", __func__);
        hw_h264_decoder_deinit(ops);
    }

    /* Don't keep the parser's codec_config pointer (lifetime is the
    * container's, not ours); we will own a copy via hw_h264_parse_avcc(). */
    os_memcpy(&ctx->video_params, params, sizeof(video_player_video_params_t));
    ctx->video_params.codec_config = NULL;
    ctx->video_params.codec_config_size = 0;

    ctx->sps_data          = NULL;
    ctx->sps_size          = 0;
    ctx->pps_data          = NULL;
    ctx->pps_size          = 0;
    ctx->nalu_length_size  = 4U;
    ctx->need_inject_params = true;
    ctx->annexb_buf        = NULL;
    ctx->annexb_buf_size   = 0;
    ctx->bond              = NULL;
    ctx->h264_handle       = NULL;
    ctx->gpu_handle        = NULL;
    ctx->flexa_pp_buf      = NULL;
    ctx->flexa_pp_raw      = NULL;
    ctx->flexa_pp_size     = 0;
    ctx->pending_head      = 0;
    ctx->pending_tail      = 0;
    ctx->pending_dropped_count = 0;
    os_memset(ctx->pending_frames, 0, sizeof(ctx->pending_frames));
    os_memset(ctx->pending_sizes,  0, sizeof(ctx->pending_sizes));

    /* Best-effort SPS/PPS extraction. If avcC is missing we assume the
    * container is feeding raw Annex-B. */
    if (params->codec_config != NULL && params->codec_config_size > 0U)
    {
        avdk_err_t pret = hw_h264_parse_avcc(ctx, params->codec_config, params->codec_config_size);
        if (pret != AVDK_ERR_OK)
        {
            LOGW("%s: parse avcC failed, ret=%d (will try without SPS/PPS injection)\n", __func__, pret);
            hw_h264_release_param_sets(ctx);
            ctx->nalu_length_size = 4U;
        }
    }
    else
    {
        LOGW("%s: no codec_config from container; assuming Annex-B input or 4B AVCC\n", __func__);
    }

    avdk_err_t ret;
    if (rtos_init_mutex(&ctx->pending_mutex) != BK_OK)
    {
        LOGE("%s: pending_mutex init failed\n", __func__);
        hw_h264_release_param_sets(ctx);
        return AVDK_ERR_NOMEM;
    }
    if (rtos_init_semaphore(&ctx->frame_ready_sem, H264_DECODER_FRAME_SEM_DEPTH) != BK_OK)
    {
        LOGE("%s: frame_ready_sem init failed\n", __func__);
        (void)rtos_deinit_mutex(&ctx->pending_mutex);
        ctx->pending_mutex = NULL;
        hw_h264_release_param_sets(ctx);
        return AVDK_ERR_NOMEM;
    }

    hw_h264_decoder_resolve_dims(ctx,
                                (uint16_t)params->width,
                                (uint16_t)params->height,
                                s_h264_decoder_rotate_degree);

    s_active_ctx = ctx;

    ret = hw_h264_decoder_setup_pipeline(ctx);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: pipeline setup failed, ret=%d\n", __func__, ret);
        s_active_ctx = NULL;
        (void)rtos_deinit_semaphore(&ctx->frame_ready_sem);
        ctx->frame_ready_sem = NULL;
        (void)rtos_deinit_mutex(&ctx->pending_mutex);
        ctx->pending_mutex = NULL;
        hw_h264_release_param_sets(ctx);
        return ret;
    }

    ctx->is_initialized = true;
    LOGI("%s: H264-GPU decoder initialized (length_size=%u, sps=%uB, pps=%uB)\n",
        __func__, ctx->nalu_length_size, ctx->sps_size, ctx->pps_size);
    return AVDK_ERR_OK;
}

static avdk_err_t hw_h264_decoder_deinit(struct video_player_video_decoder_ops_s *ops)
{
    hw_h264_decoder_instance_t *self = __containerof(ops, hw_h264_decoder_instance_t, ops);
    AVDK_RETURN_ON_FALSE(self, AVDK_ERR_INVAL, TAG, "instance is NULL");
    hw_h264_decoder_ctx_t *ctx = &self->ctx;

    hw_h264_decoder_teardown_pipeline(ctx);

    if (s_active_ctx == ctx)
    {
        s_active_ctx = NULL;
    }

    if (ctx->frame_ready_sem != NULL)
    {
        (void)rtos_deinit_semaphore(&ctx->frame_ready_sem);
        ctx->frame_ready_sem = NULL;
    }
    if (ctx->pending_mutex != NULL)
    {
        (void)rtos_deinit_mutex(&ctx->pending_mutex);
        ctx->pending_mutex = NULL;
    }

    if (ctx->annexb_buf != NULL)
    {
        bk_frame_buffer_free(ctx->annexb_buf);
        ctx->annexb_buf = NULL;
        ctx->annexb_buf_size = 0;
    }

    hw_h264_release_param_sets(ctx);

    ctx->is_initialized     = false;
    ctx->need_inject_params = true;
    return AVDK_ERR_OK;
}

static avdk_err_t hw_h264_decoder_decode(struct video_player_video_decoder_ops_s *ops,
                                        video_player_buffer_t *in_buffer,
                                        video_player_buffer_t *out_buffer,
                                        pixel_format_t out_fmt)
{
    hw_h264_decoder_instance_t *self = __containerof(ops, hw_h264_decoder_instance_t, ops);
    AVDK_RETURN_ON_FALSE(self,                      AVDK_ERR_INVAL,  TAG, "instance is NULL");
    AVDK_RETURN_ON_FALSE(in_buffer,                 AVDK_ERR_INVAL,  TAG, "in_buffer is NULL");
    AVDK_RETURN_ON_FALSE(out_buffer,                AVDK_ERR_INVAL,  TAG, "out_buffer is NULL");
    AVDK_RETURN_ON_FALSE(in_buffer->data,           AVDK_ERR_INVAL,  TAG, "in_buffer->data is NULL");

    hw_h264_decoder_ctx_t *ctx = &self->ctx;
    AVDK_RETURN_ON_FALSE(ctx->is_initialized,    AVDK_ERR_GENERIC, TAG, "decoder not initialized");
    AVDK_RETURN_ON_FALSE(ctx->h264_handle,       AVDK_ERR_GENERIC, TAG, "h264 not initialized");
    AVDK_RETURN_ON_FALSE(ctx->gpu_handle,        AVDK_ERR_GENERIC, TAG, "gpu not initialized");

    (void)out_fmt; /* GPU pipeline always emits compressed ARGB8888; the
                    * engine's requested format is ignored intentionally. */

    const uint32_t t_total_start = rtos_get_time();
    uint32_t t_annexb_done = t_total_start;
    uint32_t t_h264_done = t_total_start;
    uint32_t t_gpu_ready = t_total_start;

    /* Step 1: Annex-B prep (same as the frame-mode path). */
    uint8_t  *bs_data = in_buffer->data;
    uint32_t  bs_len  = in_buffer->length;
    bool      is_annexb_input = hw_h264_buffer_is_annex_b(in_buffer->data, in_buffer->length);

    if (!is_annexb_input)
    {
        uint32_t cvt_len = 0;
        avdk_err_t cvt_ret = hw_h264_avcc_to_annexb(ctx, in_buffer->data, in_buffer->length,
                                                    /*inject_param_sets=*/ctx->need_inject_params,
                                                    &cvt_len);
        if (cvt_ret != AVDK_ERR_OK)
        {
            LOGE("%s: AVCC->Annex-B conversion failed, ret=%d\n", __func__, cvt_ret);
            return cvt_ret;
        }
        bs_data = ctx->annexb_buf;
        bs_len  = cvt_len;
    }
    else if (ctx->need_inject_params && ctx->sps_size > 0U && ctx->pps_size > 0U)
    {
        const uint32_t need = in_buffer->length + ctx->sps_size + ctx->pps_size + 16U;
        avdk_err_t ret = hw_h264_ensure_annexb_buf(ctx, need);
        if (ret != AVDK_ERR_OK) return ret;
        uint32_t out = 0;
        ret = hw_h264_append_nalu(ctx, &out, ctx->sps_data, ctx->sps_size);
        if (ret == AVDK_ERR_OK) ret = hw_h264_append_nalu(ctx, &out, ctx->pps_data, ctx->pps_size);
        if (ret != AVDK_ERR_OK) return ret;
        if (out + in_buffer->length > ctx->annexb_buf_size) return AVDK_ERR_NOMEM;
        os_memcpy(&ctx->annexb_buf[out], in_buffer->data, in_buffer->length);
        bs_data = ctx->annexb_buf;
        bs_len  = out + in_buffer->length;
        ctx->need_inject_params = false;
    }
    t_annexb_done = rtos_get_time();

    /* Step 2: Submit the AU to the H.264 IP. In Flexa mode the call is
    * relatively quick -- the IP streams segments to the GPU asynchronously.
    * The "out_buffer" inside the H264 input is the HSRAM Flexa ring. */
    bk_h264_decode_input_t in = {0};
    in.stream          = bs_data;
    in.stream_len      = bs_len;
    in.out_buffer      = ctx->flexa_pp_buf;
    in.out_buffer_size = ctx->flexa_pp_size;

    avdk_err_t ret = bk_h264_decode_frame(ctx->h264_handle, &in);
    t_h264_done = rtos_get_time();
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: bk_h264_decode_frame failed, ret=%d (bs_len=%u)\n",
            __func__, ret, bs_len);
        ctx->need_inject_params = true; /* re-feed SPS/PPS if playback continues */
        return ret;
    }

    /* Step 3: Wait for the GPU's frame_display callback. */
    int sem_ret = rtos_get_semaphore(&ctx->frame_ready_sem,
                                    H264_DECODER_FRAME_TIMEOUT_MS);
    t_gpu_ready = rtos_get_time();
    if (sem_ret != BK_OK)
    {
        LOGE("%s: gpu frame_ready timeout after %u ms (sem_ret=%d)\n",
            __func__, H264_DECODER_FRAME_TIMEOUT_MS, sem_ret);
        ctx->need_inject_params = true;
        return AVDK_ERR_TIMEOUT;
    }

    void    *gpu_frame      = NULL;
    uint32_t gpu_frame_size = 0;
    if (!h264_decoder_pop_pending(ctx, &gpu_frame, &gpu_frame_size) || gpu_frame == NULL)
    {
        LOGE("%s: sem signaled but no frame in queue\n", __func__);
        return AVDK_ERR_GENERIC;
    }

    if (out_buffer->data != NULL)
    {
        bk_frame_buffer_free(out_buffer->data);
        out_buffer->data = NULL;
    }

    if (out_buffer->frame_buffer != NULL)
    {
        frame_buffer_t *out_frame = (frame_buffer_t *)out_buffer->frame_buffer;
        out_frame->frame     = (uint8_t *)gpu_frame;
        out_frame->size      = gpu_frame_size;
        out_frame->length    = gpu_frame_size;
        out_frame->width     = ctx->out_w;
        out_frame->height    = ctx->out_h;
        /* The DPU consumes this as ARGB8888 compressed; we report the
        * un-compressed format because PIXEL_FMT_ARGB8888 is what the
        * existing display path keys on. */
        out_frame->fmt       = PIXEL_FMT_ARGB8888;
        out_frame->timestamp = (uint32_t)in_buffer->pts;
    }

    out_buffer->data   = (uint8_t *)gpu_frame;
    out_buffer->length = gpu_frame_size;
    out_buffer->pts    = in_buffer->pts;

    LOGI("%s: timing pts=%llu bs=%u annexb=%u h264=%u gpu_wait=%u total=%u\n",
        __func__, (unsigned long long)in_buffer->pts, (unsigned)bs_len,
        (unsigned)(t_annexb_done - t_total_start),
        (unsigned)(t_h264_done - t_annexb_done),
        (unsigned)(t_gpu_ready - t_h264_done),
        (unsigned)(rtos_get_time() - t_total_start));

    LOGV("%s: out gpu frame=%p size=%u dim=%ux%u pts=%llu\n",
        __func__, gpu_frame, (unsigned)gpu_frame_size,
        ctx->out_w, ctx->out_h, (unsigned long long)in_buffer->pts);
    return AVDK_ERR_OK;
}

// ---------------------------------------------------------------------------
// Create / destroy / template
// ---------------------------------------------------------------------------

static video_player_video_decoder_ops_t *hw_h264_decoder_create(void)
{
    hw_h264_decoder_instance_t *inst = os_malloc(sizeof(hw_h264_decoder_instance_t));
    if (inst == NULL)
    {
        LOGE("%s: alloc instance failed\n", __func__);
        return NULL;
    }
    os_memset(inst, 0, sizeof(*inst));
    os_memcpy(&inst->ops, &s_ops_template, sizeof(video_player_video_decoder_ops_t));
    return &inst->ops;
}

static void hw_h264_decoder_destroy(video_player_video_decoder_ops_t *ops)
{
    if (ops == NULL || ops == &s_ops_template) return;
    hw_h264_decoder_instance_t *inst = __containerof(ops, hw_h264_decoder_instance_t, ops);
    os_free(inst);
}

static video_player_video_decoder_ops_t s_ops_template = {
    .create                = hw_h264_decoder_create,
    .destroy               = hw_h264_decoder_destroy,
    .get_supported_formats = hw_h264_decoder_get_supported_formats,
    .init                  = hw_h264_decoder_init,
    .deinit                = hw_h264_decoder_deinit,
    .decode                = hw_h264_decoder_decode,
};

video_player_video_decoder_ops_t *bk_video_player_get_hw_h264_decoder_ops(void)
{
    return &s_ops_template;
}
