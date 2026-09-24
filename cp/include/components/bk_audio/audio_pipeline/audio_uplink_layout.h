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

#ifndef _AUDIO_UPLINK_LAYOUT_H_
#define _AUDIO_UPLINK_LAYOUT_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Uplink (ADC/mic + AEC) interleaved lane-layout: Single Source of Truth.
 *
 * The lane layout of the ADC capture stream is currently encoded independently
 * in three places that MUST agree, or the AEC de-interleave silently emits
 * corrupted audio:
 *   1) onboard_mic_stream : DMA lane count = popcount(ch_bitmap) + (aec_en?1:0)
 *   2) ADC hw register    : adc_en bitmap + aec_en digital loopback lane
 *   3) aec_v3_algorithm   : hard-coded /4,/6,/8 de-interleave + fixed ref index
 *
 * This header centralises that truth so the capture side and the consume side
 * can be described, logged and cross-checked from one place. It is purely
 * additive: it changes no runtime behaviour, it only lets callers describe and
 * validate the layout they already use.
 *
 * The tables below reproduce the SHIPPED behaviour verbatim, quirks included.
 * Confirmed with the hardware/driver owner: loop=1 and loop=0 are TWO DIFFERENT
 * hardware-reference mechanisms, distinguished by the ADC aec_en register
 * (aud_adc_driver.c: audio_reg_hal_set_adc_cfg_aec_en):
 *
 *   - loop=1 ("aec loop"): adc_cfg.aec_en is SET. The hardware APPENDS a dedicated
 *     reference lane at the END of the ADC stream. Capture lane count therefore
 *     gains +1 (see aud_adc_get_active_ch_num). Mic ADC channels are untouched.
 *   - loop=0 ("plain hw loopback"): adc_cfg.aec_en is NOT set. No lane is appended;
 *     the reference is instead carried by a real ADC channel (current convention:
 *     the FIRST channel, ch0). Capture lane count = popcount(ch_bitmap), no +1.
 *
 * Resulting de-interleave, verbatim as shipped:
 *       single mic     : [mic, ref]                     -> /4  (loop flag is a
 *                                                          no-op; ref ends up last
 *                                                          either way)
 *       dual  loop=0   : [ref, mic, mic]                -> /6  (ref = ch0, no append)
 *       dual  loop=1   : [mic0(unused), mic, mic, ref]  -> /8  (ref appended last)
 *
 *   - The "/8" first lane (g_mic3 / ch0) is NOT a real mic: during bring-up ch0
 *     was enabled although it was not needed, so the stream carries a lane that
 *     the AEC simply discards. This is now FROZEN DEAD legacy: both shipped dual
 *     mic projects (ai, doorbell) use the clean ch1|ch2 dual-dmic capture and pass
 *     adc_ch_num=2, so they de-interleave the exact 3 lanes (mic[0,1] ref@2). The
 *     /8 discard path is kept only for adc_ch_num==0 back-compat and is surfaced
 *     via discard_index[] so nothing silently corrupts if an old config resurfaces.
 *   - Recommended layout for NEW customers: pin the hardware loopback to ADC_CH0
 *     and only enable the mics actually used. The loopback lane position should
 *     ultimately be a config field (ref_index) rather than a hard-coded quirk;
 *     the defaults below keep every shipped project bit-for-bit unchanged.
 *
 * NB: the loop <-> aec_en correspondence is strict only for DUAL mic. For single
 * mic loop is a no-op, so aec_en=1 (append) with loop=0 is still valid: the
 * appended ref simply lands at index 1, which is exactly what /4 reads as ref.
 * The reliable cross-check is therefore capture.lane_num == consume.lane_num.
 */

/* Physical ceiling on this SoC: 3 ADC channels (AUD_ADC_CHL_MAX) + at most one
 * appended aec_en reference lane = 4 interleaved lanes. */
#define AUD_UPLINK_MAX_LANES   (5)

typedef struct {
    uint8_t lane_num;                          /* total int16 lanes interleaved on the ADC stream */
    uint8_t mic_cnt;                           /* mic lanes actually fed to the AEC */
    int8_t  ref_index;                         /* lane index of the reference; -1 = none (software ref) */
    int8_t  mic_index[AUD_UPLINK_MAX_LANES];   /* lane index of each mic consumed by the AEC */
    uint8_t discard_cnt;                       /* captured-but-unused lanes (e.g. dual+loop extra mic) */
    int8_t  discard_index[AUD_UPLINK_MAX_LANES];
} aud_uplink_layout_t;

typedef enum {
    AUD_UPLINK_OK = 0,
    AUD_UPLINK_ERR_NO_MIC,          /* ch_bitmap has no active channel */
    AUD_UPLINK_ERR_LANE_MISMATCH,   /* capture lane count != AEC expected lane count */
    AUD_UPLINK_ERR_REF_MISSING,     /* loop=1 (append mode) but aec_en=0: no appended ref lane */
    AUD_UPLINK_ERR_REF_UNEXPECTED,  /* software mode but aec_en=1 (stray ref lane) */
    AUD_UPLINK_ERR_MIC_MISMATCH,    /* software mode: popcount(ch_bitmap) != AEC mic count */
} aud_uplink_check_t;

static inline uint8_t aud_uplink_popcount(uint32_t x)
{
    uint8_t n = 0;
    while (x) { n += (uint8_t)(x & 1u); x >>= 1; }
    return n;
}

/* Capture side: what onboard_mic_stream physically interleaves onto the ADC
 * stream. Mirrors aud_adc_get_active_ch_num(): mics first, and when aec_en is set
 * (loop=1 append mode) a dedicated ref lane is appended last. When aec_en=0
 * (plain hw loopback / software) no lane is appended and ref_index stays -1 here;
 * in that case the reference, if any, is carried inside one of the mic channels
 * and its semantics are resolved on the consume side. */
static inline aud_uplink_layout_t
aud_uplink_capture_layout(uint32_t ch_bitmap, bool aec_en)
{
    aud_uplink_layout_t l;
    uint8_t mics = aud_uplink_popcount(ch_bitmap);
    uint8_t i;

    for (i = 0; i < AUD_UPLINK_MAX_LANES; i++) {
        l.mic_index[i]     = -1;
        l.discard_index[i] = -1;
    }
    for (i = 0; i < mics && i < AUD_UPLINK_MAX_LANES; i++) {
        l.mic_index[i] = (int8_t)i;
    }
    l.mic_cnt     = mics;
    l.ref_index   = aec_en ? (int8_t)mics : (int8_t)-1;
    l.lane_num    = (uint8_t)(mics + (aec_en ? 1 : 0));
    l.discard_cnt = 0;
    return l;
}

/* Pass this as ref_ch to keep the shipped/default reference position. */
#define AUD_UPLINK_REF_CH_DEFAULT   (-1)

/* Consume side (configurable): describe how the AEC should de-interleave the
 * stream. The de-interleave is fully driven by the returned (lane_num, ref_index,
 * mic_index[]) so callers no longer hard-code /4,/6,/8.
 *
 *   dual_ch: mic-count selector (the field keeps its historical name):
 *     0 = single mic, 1 = dual mic, 2 = triple mic. Triple is RESERVED: the layout
 *     is described so the plumbing is ready, but the AEC algorithm/de-interleave do
 *     not consume 3 mics yet (the integrator guards against it).
 *
 *   ref_ch:
 *     - AUD_UPLINK_REF_CH_DEFAULT (or out of range): keep the shipped position
 *       (in-channel -> ch0/first; append -> hardware appends last; single -> last).
 *       This reproduces the legacy /4,/6,/8 exactly.
 *     - 0..lane_num-1: only meaningful for the in-channel mechanism (aec_loop=0):
 *       pins the reference to that ADC lane; the remaining lanes become the mics in
 *       ascending order. Ignored for append (hardware fixes ref last) and single.
 *
 *   adc_ch_num: number of ADC channels actually enabled on the mic side (popcount of
 *     the mic ch_bitmap, EXCLUDING the appended aec_en ref lane).
 *     - 0 = UNKNOWN: reproduce the legacy hard-coded /4,/6,/8 exactly (back-compat
 *       for configs that never set it, e.g. ai's over-enabled ch0 /8 quirk).
 *     - >0 = authoritative: derive the clean, quirk-free layout from the real capture
 *       count. For append, any ADC lanes beyond mic_num are treated as leading discard
 *       lanes (matches the legacy convention that the ref/extra rides the FIRST lanes),
 *       so a correctly-sized config (adc_ch_num == mic_num) yields NO discard.
 */
static inline aud_uplink_layout_t
aud_uplink_aec_consume_layout_ex(bool hw_mode, uint8_t dual_ch, bool aec_loop,
                                 int ref_ch, uint8_t adc_ch_num)
{
    aud_uplink_layout_t l;
    uint8_t i, m;
    uint8_t mic_num = (uint8_t)(dual_ch + 1);   /* 0->1, 1->2, 2->3 mics */

    for (i = 0; i < AUD_UPLINK_MAX_LANES; i++) {
        l.mic_index[i]     = -1;
        l.discard_index[i] = -1;
    }
    l.mic_cnt = 0;
    l.ref_index = -1;
    l.lane_num = 0;
    l.discard_cnt = 0;

    if (mic_num < 1) { mic_num = 1; }
    if (mic_num > 3) { mic_num = 3; }   /* only 1..3 are defined */

    /* -----------------------------------------------------------------
     * adc_ch_num > 0: authoritative capture count -> clean layout.
     * Drives the de-interleave off the REAL number of ADC lanes so the mic
     * side and the AEC side can never silently disagree on lane_num. ----- */
    if (adc_ch_num > 0) {
        if (adc_ch_num > (AUD_UPLINK_MAX_LANES - 1)) {
            adc_ch_num = (uint8_t)(AUD_UPLINK_MAX_LANES - 1);
        }
        if (!hw_mode) {
            for (m = 0; m < adc_ch_num && m < AUD_UPLINK_MAX_LANES; m++) {
                l.mic_index[m] = (int8_t)m;
            }
            l.mic_cnt = adc_ch_num; l.lane_num = adc_ch_num;   /* ref via multi_input */
            return l;
        }
        if (aec_loop) {
            /* append: hw adds the ref as one extra lane at the very end. Any ADC
             * lane in excess of mic_num is a leading discard lane (legacy ch0). */
            uint8_t discard = (adc_ch_num > mic_num) ? (uint8_t)(adc_ch_num - mic_num) : 0;
            for (i = 0; i < discard && i < AUD_UPLINK_MAX_LANES; i++) {
                l.discard_index[i] = (int8_t)i;
            }
            l.discard_cnt = discard;
            m = 0;
            for (i = discard; i < adc_ch_num && m < AUD_UPLINK_MAX_LANES; i++) {
                l.mic_index[m++] = (int8_t)i;
            }
            l.mic_cnt   = m;
            l.ref_index = (int8_t)adc_ch_num;                  /* appended last */
            l.lane_num  = (uint8_t)(adc_ch_num + 1);
        } else {
            /* in-channel: the ref rides one of the real ADC lanes (default ch0). */
            int r = (ref_ch < 0 || ref_ch >= (int)adc_ch_num) ? 0 : ref_ch;
            l.ref_index = (int8_t)r;
            m = 0;
            for (i = 0; i < adc_ch_num && m < AUD_UPLINK_MAX_LANES; i++) {
                if ((int)i != r) { l.mic_index[m++] = (int8_t)i; }
            }
            l.mic_cnt  = m;
            l.lane_num = adc_ch_num;
        }
        return l;
    }

    if (!hw_mode) {
        /* software: mics ride the ADC stream, ref arrives via multi_input */
        for (m = 0; m < mic_num; m++) { l.mic_index[m] = (int8_t)m; }
        l.mic_cnt = mic_num; l.lane_num = mic_num;   /* ref_index stays -1 */
        return l;
    }

    if (mic_num == 1) {
        /* [mic, ref] -> /4 ; ref is always last for single mic (loop is a no-op) */
        l.lane_num = 2; l.mic_cnt = 1; l.mic_index[0] = 0; l.ref_index = 1;
    } else if (aec_loop && mic_num == 2) {
        /* legacy append (/8): ai over-enabled ch0 -> first lane discarded, ref last */
        l.lane_num = 4; l.mic_cnt = 2; l.ref_index = 3; l.mic_index[0] = 1; l.mic_index[1] = 2;
        l.discard_cnt = 1; l.discard_index[0] = 0;
    } else if (aec_loop) {
        /* append (general, incl. reserved 3-mic): mics first, ref appended last */
        for (m = 0; m < mic_num; m++) { l.mic_index[m] = (int8_t)m; }
        l.mic_cnt = mic_num; l.ref_index = (int8_t)mic_num;
        l.lane_num = (uint8_t)(mic_num + 1);
    } else {
        /* in-channel: ref occupies one real ADC lane (default ch0), rest are mics */
        uint8_t lanes = (uint8_t)(mic_num + 1);
        int r = (ref_ch < 0 || ref_ch >= (int)lanes) ? 0 : ref_ch;
        l.lane_num = lanes; l.mic_cnt = mic_num; l.ref_index = (int8_t)r;
        m = 0;
        for (i = 0; i < lanes; i++) {
            if ((int)i != r) { l.mic_index[m++] = (int8_t)i; }
        }
    }
    return l;
}

/* Consume side: how aec_v3_algorithm de-interleaves the stream today. Mirrors
 * the shipped /4,/6,/8 paths exactly. aec_loop == CONFIG_AUD_AEC_LOOP. */
static inline aud_uplink_layout_t
aud_uplink_aec_consume_layout(bool hw_mode, bool dual_ch, bool aec_loop)
{
    return aud_uplink_aec_consume_layout_ex(hw_mode, dual_ch, aec_loop,
                                            AUD_UPLINK_REF_CH_DEFAULT, 0);
}

/* Cross-check the capture side against the AEC consume side. Returns
 * AUD_UPLINK_OK when the two agree; otherwise a reason code. */
static inline aud_uplink_check_t
aud_uplink_check(bool hw_mode, uint32_t ch_bitmap, bool aec_en, bool dual_ch, bool aec_loop)
{
    aud_uplink_layout_t cap = aud_uplink_capture_layout(ch_bitmap, aec_en);
    aud_uplink_layout_t con = aud_uplink_aec_consume_layout(hw_mode, dual_ch, aec_loop);

    if (cap.mic_cnt == 0) {
        return AUD_UPLINK_ERR_NO_MIC;
    }
    if (hw_mode) {
        /* loop=1 is the append mechanism: it REQUIRES aec_en to add the tail lane.
         * loop=0 is plain hw loopback: aec_en=0 is valid, the ref rides a mic
         * channel. The reliable invariant either way is lane_num equality. */
        if (aec_loop && !aec_en) {
            return AUD_UPLINK_ERR_REF_MISSING;
        }
        if (cap.lane_num != con.lane_num) {
            return AUD_UPLINK_ERR_LANE_MISMATCH;
        }
    } else {
        if (aec_en) {
            return AUD_UPLINK_ERR_REF_UNEXPECTED;
        }
        if (cap.mic_cnt != con.mic_cnt) {
            return AUD_UPLINK_ERR_MIC_MISMATCH;
        }
    }
    return AUD_UPLINK_OK;
}

/* Render a layout as "mic[1,2] ref@3 discard[0]" into caller buffer for logging.
 * Returns buf. Purely for self-describe logs; no behavioural effect.
 * Implemented in audio_uplink_layout.c (kept out of this header-only SSOT so the
 * snprintf-based formatting is not inlined into every including translation unit). */
const char *aud_uplink_layout_fmt(const aud_uplink_layout_t *l, char *buf, int buf_len);

/* Human-readable string for an aud_uplink_check_t code. Implemented in
 * audio_uplink_layout.c (see aud_uplink_layout_fmt). */
const char *aud_uplink_check_str(aud_uplink_check_t r);

/* =========================================================================
 * Uplink resolver: high-level intent  ->  low-level mic + AEC settings.
 *
 * This is the forward "single source of truth" the whole discussion aims at.
 * The integrator states WHAT it wants (how many real mics, how the reference
 * is obtained) and the resolver derives, in one place and always consistent:
 *   - the mic/ADC side: how many ADC channels to enable + the aec_en register;
 *   - the AEC side: dual_ch, aec_loop, ref_ch;
 *   - the shared interleaved lane layout used to de-interleave and to size the
 *     AEC input buffer (lane_num == the capture lane count BY CONSTRUCTION).
 *
 * It intentionally produces the CLEAN layout (no bring-up quirks such as the
 * ai /8 discard lane). Legacy projects that rely on those quirks keep their
 * hand-written config and the *_ex() legacy layout; the resolver is what new
 * or corrected configs should be driven from.
 *
 * ch_bitmap is deliberately NOT emitted here: which *physical* ADC channels a
 * board wires its mics/ref to is board-specific. The resolver emits the COUNT
 * of ADC channels to enable (adc_ch_enable_num) and aec_en; the integrator maps
 * that onto its board's ch_bitmap, and popcount(ch_bitmap) must equal it.
 * ========================================================================= */

typedef enum {
    /* Hardware appends a dedicated ref lane at the tail (ADC aec_en register SET).
     * Enable only the real mics on the ADC; hw adds ref as the last lane. */
    AUD_UPLINK_REF_APPEND = 0,
    /* Plain hw loopback: the ref rides a real ADC channel (aec_en register NOT set).
     * Enable mics + 1 extra ADC channel that carries the ref (position = ref_ch). */
    AUD_UPLINK_REF_IN_CHANNEL,
    /* Reference delivered out-of-band (software ring / multi_input); ADC carries
     * mics only. */
    AUD_UPLINK_REF_SOFTWARE,
} aud_uplink_ref_kind_t;

/* Sentinel for intent.ref_ch meaning "let the resolver pick the default (ch0)". */
#define AUD_UPLINK_REF_CH_AUTO   (0xFFu)

typedef struct {
    uint8_t               mic_num;   /* real mics wanted: 1..3 (3 reserved) */
    aud_uplink_ref_kind_t ref;       /* how the AEC reference is obtained */
    uint8_t               ref_ch;    /* IN_CHANNEL only: lane index of the ref,
                                        or AUD_UPLINK_REF_CH_AUTO for default */
} aud_uplink_intent_t;

typedef struct {
    /* --- AEC-side settings --- */
    bool     hw_mode;           /* false only for AUD_UPLINK_REF_SOFTWARE */
    uint8_t  dual_ch;           /* mic-count selector: mic_num - 1 */
    uint8_t  aec_loop;          /* 1 = append mechanism, 0 = in-channel/software */
    uint8_t  ref_ch;            /* resolved ref lane index (in-channel); else 0xFF */
    /* --- mic/ADC-side settings --- */
    uint8_t  aec_en;            /* ADC aec_en register (1 only for APPEND) */
    uint8_t  adc_ch_enable_num; /* how many ADC channels the mic must enable;
                                   popcount(ch_bitmap) must equal this */
    /* --- shared truth --- */
    aud_uplink_layout_t layout; /* lane_num == capture lanes; ref/mic indices */
} aud_uplink_resolved_t;

/* =========================================================================
 * Uplink CAPS: the compact capture-side truth propagated producer -> consumer.
 *
 * The mic/capture element is the natural owner of the interleaved-stream
 * geometry (it is the one that assembled the ADC stream), so it publishes these
 * few bytes and the pipeline forwards them downstream to the AEC. The AEC then
 * derives the full lane layout with aud_uplink_layout_from_caps() - no dual_ch /
 * aec_loop / adc_ch_num / ref_ch needed on the AEC's own config.
 *
 * This carries ONLY what the AEC cannot know on its own; the reference PATH
 * (ADC stream vs software ringbuffer) stays an AEC concern (aec mode), because
 * software ref does not ride the ADC stream at all.
 * ========================================================================= */
#define AUD_UPLINK_CAPS_VERSION   (0)

typedef struct {
    uint8_t  valid;      /* 1 = producer published real capture geometry */
    uint8_t  aec_en;     /* ADC aec_en register: hw appends a ref lane at the tail */
    uint8_t  ref_ch;     /* in-channel ref lane index (0..lane-1); AUD_UPLINK_REF_CH_AUTO
                            = no in-channel ref (append when aec_en, else mics only) */
    uint8_t  version;
    uint32_t ch_bitmap;  /* enabled ADC channels (popcount = mic ADC lanes) */
    uint32_t reserved[4];
} aud_uplink_caps_t;

/* Derive the interleaved lane layout the AEC must de-interleave, straight from
 * the capture caps. This is the capture interleave itself (mics in ascending ADC
 * order, plus the reference wherever the hardware placed it), so the consume side
 * matches the capture side BY CONSTRUCTION:
 *   - aec_en=1            : append -> lane_num = popcount+1, ref at the tail;
 *   - aec_en=0, ref_ch<pc : in-channel -> lane_num = popcount, ref at ref_ch;
 *   - aec_en=0, no ref_ch : mics only -> lane_num = popcount, ref_index = -1
 *                           (software-ref or no ref on the ADC stream).
 */
static inline aud_uplink_layout_t
aud_uplink_layout_from_caps(const aud_uplink_caps_t *c)
{
    aud_uplink_layout_t l;
    uint8_t pc = c ? aud_uplink_popcount(c->ch_bitmap) : 0;
    uint8_t i, m;

    for (i = 0; i < AUD_UPLINK_MAX_LANES; i++) {
        l.mic_index[i]     = -1;
        l.discard_index[i] = -1;
    }
    l.mic_cnt = 0;
    l.ref_index = -1;
    l.lane_num = 0;
    l.discard_cnt = 0;

    if (!c || pc == 0) {
        return l;
    }
    if (pc > (AUD_UPLINK_MAX_LANES - 1)) {
        pc = (uint8_t)(AUD_UPLINK_MAX_LANES - 1);
    }

    if (c->aec_en) {
        /* append: all enabled ADC channels are mics, hw adds ref last */
        for (m = 0; m < pc; m++) { l.mic_index[m] = (int8_t)m; }
        l.mic_cnt   = pc;
        l.ref_index = (int8_t)pc;
        l.lane_num  = (uint8_t)(pc + 1);
    } else if (c->ref_ch < pc) {
        /* in-channel: ref rides a real ADC lane, the rest are mics */
        l.ref_index = (int8_t)c->ref_ch;
        m = 0;
        for (i = 0; i < pc; i++) {
            if (i != c->ref_ch) { l.mic_index[m++] = (int8_t)i; }
        }
        l.mic_cnt  = m;
        l.lane_num = pc;
    } else {
        /* mics only on the ADC stream (software ref, or no ref) */
        for (m = 0; m < pc; m++) { l.mic_index[m] = (int8_t)m; }
        l.mic_cnt  = pc;
        l.lane_num = pc;
    }
    return l;
}

static inline aud_uplink_resolved_t
aud_uplink_resolve(const aud_uplink_intent_t *in)
{
    aud_uplink_resolved_t r;
    uint8_t mic_num = in ? in->mic_num : 1;
    aud_uplink_ref_kind_t ref = in ? in->ref : AUD_UPLINK_REF_APPEND;
    uint8_t i, m;

    if (mic_num < 1) { mic_num = 1; }
    if (mic_num > 3) { mic_num = 3; }   /* 3 reserved; algorithm not ready yet */

    for (i = 0; i < AUD_UPLINK_MAX_LANES; i++) {
        r.layout.mic_index[i]     = -1;
        r.layout.discard_index[i] = -1;
    }
    r.layout.mic_cnt     = mic_num;
    r.layout.ref_index   = -1;
    r.layout.lane_num    = 0;
    r.layout.discard_cnt = 0;      /* resolver never emits discard lanes */

    r.dual_ch = (uint8_t)(mic_num - 1);
    r.ref_ch  = 0xFFu;

    if (ref == AUD_UPLINK_REF_SOFTWARE) {
        r.hw_mode = false;
        r.aec_en  = 0;
        r.aec_loop = 0;
        r.adc_ch_enable_num = mic_num;               /* mics only on ADC */
        for (m = 0; m < mic_num; m++) { r.layout.mic_index[m] = (int8_t)m; }
        r.layout.lane_num = mic_num;                 /* ref not on ADC stream */
        return r;
    }

    r.hw_mode = true;

    if (ref == AUD_UPLINK_REF_APPEND) {
        r.aec_en  = 1;
        r.aec_loop = 1;
        r.adc_ch_enable_num = mic_num;               /* hw appends the ref lane */
        for (m = 0; m < mic_num; m++) { r.layout.mic_index[m] = (int8_t)m; }
        r.layout.ref_index = (int8_t)mic_num;        /* appended last */
        r.layout.lane_num  = (uint8_t)(mic_num + 1);
    } else { /* AUD_UPLINK_REF_IN_CHANNEL */
        uint8_t lanes = (uint8_t)(mic_num + 1);      /* ref is a real ADC lane */
        uint8_t rc = (in && in->ref_ch < lanes) ? in->ref_ch : 0;
        r.aec_en  = 0;
        r.aec_loop = 0;
        r.ref_ch  = rc;
        r.adc_ch_enable_num = lanes;                 /* mics + ref all real ADC */
        r.layout.ref_index = (int8_t)rc;
        m = 0;
        for (i = 0; i < lanes; i++) {
            if (i != rc) { r.layout.mic_index[m++] = (int8_t)i; }
        }
        r.layout.lane_num = lanes;
    }
    return r;
}

#ifdef __cplusplus
}
#endif

#endif /* _AUDIO_UPLINK_LAYOUT_H_ */
