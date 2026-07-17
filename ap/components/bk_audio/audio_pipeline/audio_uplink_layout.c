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

/*
 * Non-inline helpers for the uplink lane-layout SSOT (audio_uplink_layout.h).
 *
 * Only the string/log helpers live here: they use snprintf and are used from just
 * a couple of call sites, so keeping them out of the header avoids inlining
 * snprintf into every translation unit that includes the SSOT. The geometry
 * derivation functions stay header-only inline (they are tiny and pure).
 */

#include <stdio.h>
#include <components/bk_audio/audio_pipeline/audio_uplink_layout.h>

const char *aud_uplink_layout_fmt(const aud_uplink_layout_t *l, char *buf, int buf_len)
{
    int n = 0;
    uint8_t i;
    if (!buf || buf_len <= 0) {
        return "";
    }
    n += snprintf(buf + n, (size_t)(buf_len - n), "mic[");
    for (i = 0; i < l->mic_cnt && i < AUD_UPLINK_MAX_LANES && n < buf_len; i++) {
        n += snprintf(buf + n, (size_t)(buf_len - n), "%s%d",
                      i ? "," : "", (int)l->mic_index[i]);
    }
    n += snprintf(buf + n, (size_t)(buf_len - n), "] ref@%d", (int)l->ref_index);
    if (l->discard_cnt) {
        n += snprintf(buf + n, (size_t)(buf_len - n), " discard[");
        for (i = 0; i < l->discard_cnt && i < AUD_UPLINK_MAX_LANES && n < buf_len; i++) {
            n += snprintf(buf + n, (size_t)(buf_len - n), "%s%d",
                          i ? "," : "", (int)l->discard_index[i]);
        }
        n += snprintf(buf + n, (size_t)(buf_len - n), "]");
    }
    (void)n;
    return buf;
}

const char *aud_uplink_check_str(aud_uplink_check_t r)
{
    switch (r) {
    case AUD_UPLINK_OK:                 return "OK";
    case AUD_UPLINK_ERR_NO_MIC:         return "NO_MIC(ch_bitmap empty)";
    case AUD_UPLINK_ERR_LANE_MISMATCH:  return "LANE_MISMATCH(capture!=aec)";
    case AUD_UPLINK_ERR_REF_MISSING:    return "REF_MISSING(loop=1 append needs aec_en)";
    case AUD_UPLINK_ERR_REF_UNEXPECTED: return "REF_UNEXPECTED(sw mode has aec_en)";
    case AUD_UPLINK_ERR_MIC_MISMATCH:   return "MIC_MISMATCH(sw mic count)";
    default:                            return "UNKNOWN";
    }
}
