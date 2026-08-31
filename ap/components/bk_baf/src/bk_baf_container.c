/**
 * @file bk_baf_container.c
 *
 * BAF v1 container parser: buffer -> bk_baf_source_t (see bk_baf_container.h /
 * BAF_SPEC_CN.md). Pure, no filesystem / LVGL / OS-heavy deps; read-only over the buffer.
 */

#include "bk_baf_container.h"

#include <string.h>

/* ---- CRC-32 (IEEE 802.3, reflected). Bytewise, no table -- containers are small
 * and this runs once per open. Matches Python zlib.crc32 / binascii.crc32. ---- */
uint32_t baf_crc32(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFU;
    for(uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for(int b = 0; b < 8; b++) {
            crc = (crc >> 1) ^ (0xEDB88320U & (uint32_t)(-(int32_t)(crc & 1U)));
        }
    }
    return crc ^ 0xFFFFFFFFU;
}

/* Resolve directory entry @p idx, check type and bounds, optional CRC.
 * On success writes the section base pointer and size. */
static bool chunk_resolve(const uint8_t *buf, uint32_t len,
                          const baf_chunk_t *dir, uint16_t dir_count,
                          uint32_t idx, uint32_t expect_type,
                          const uint8_t **out_ptr, uint32_t *out_size)
{
    if(idx >= dir_count) return false;
    const baf_chunk_t *c = &dir[idx];
    if(c->type != expect_type) return false;
    /* bounds: offset + size within file (guard overflow) */
    if(c->offset > len || c->size > len - c->offset) return false;
    if(c->crc32 != 0U && baf_crc32(buf + c->offset, c->size) != c->crc32) return false;
    *out_ptr = buf + c->offset;
    *out_size = c->size;
    return true;
}

/* Every AU (offset,size) must be non-empty and inside its DATA blob. */
static bool aus_in_bounds(const bk_baf_au_t *aus, uint32_t count, uint32_t data_size)
{
    for(uint32_t i = 0; i < count; i++) {
        if(aus[i].size == 0U) return false;
        if(aus[i].offset > data_size || aus[i].size > data_size - aus[i].offset) return false;
    }
    return true;
}

avdk_err_t baf_parse_inplace(const uint8_t *buf, uint32_t len, baf_view_t *out)
{
    if(buf == NULL || out == NULL || len < BAF_HEADER_SIZE) return AVDK_ERR_INVAL;

    const baf_file_header_t *h = (const baf_file_header_t *)buf;
    if(memcmp(h->magic, BAF_MAGIC, 8) != 0) return AVDK_ERR_INVAL;
    if(h->ver_major != BAF_VER_MAJOR) return AVDK_ERR_UNSUPPORTED;
    if(h->file_size != len) return AVDK_ERR_INVAL;
    if(baf_crc32(buf, BAF_HEADER_SIZE - 4U) != h->header_crc32) return AVDK_ERR_INVAL;

    /* geometry */
    uint32_t fc = h->frame_count;
    if(fc == 0U || h->width == 0U || h->height == 0U ||
       (h->width & 15U) != 0U || (h->height & 15U) != 0U) {
        return AVDK_ERR_INVAL;
    }

    /* directory in bounds */
    if((uint64_t)h->dir_offset + (uint64_t)h->dir_count * sizeof(baf_chunk_t) > (uint64_t)len) {
        return AVDK_ERR_INVAL;
    }
    const baf_chunk_t *dir = (const baf_chunk_t *)(buf + h->dir_offset);

    const uint8_t *rgb_idx_p = NULL, *rgb_data_p = NULL, *dur_p = NULL;
    uint32_t rgb_idx_sz = 0, rgb_data_sz = 0, dur_sz = 0;

    if(!chunk_resolve(buf, len, dir, h->dir_count, h->rgb_idx, BAF_CHUNK_IDX, &rgb_idx_p, &rgb_idx_sz) ||
       !chunk_resolve(buf, len, dir, h->dir_count, h->rgb_data, BAF_CHUNK_DATA, &rgb_data_p, &rgb_data_sz) ||
       !chunk_resolve(buf, len, dir, h->dir_count, h->dur, BAF_CHUNK_DUR, &dur_p, &dur_sz)) {
        return AVDK_ERR_INVAL;
    }
    /* per-frame table sizes must match frame_count */
    if(rgb_idx_sz != fc * (uint32_t)sizeof(bk_baf_au_t) || dur_sz != fc * 4U) {
        return AVDK_ERR_INVAL;
    }
    if(!aus_in_bounds((const bk_baf_au_t *)rgb_idx_p, fc, rgb_data_sz)) {
        return AVDK_ERR_INVAL;
    }

    const uint8_t *alpha_idx_p = NULL, *alpha_data_p = NULL;
    uint32_t alpha_idx_sz = 0, alpha_data_sz = 0;
    if(h->has_alpha) {
        if(!chunk_resolve(buf, len, dir, h->dir_count, h->alpha_idx, BAF_CHUNK_IDX, &alpha_idx_p, &alpha_idx_sz) ||
           !chunk_resolve(buf, len, dir, h->dir_count, h->alpha_data, BAF_CHUNK_DATA, &alpha_data_p, &alpha_data_sz)) {
            return AVDK_ERR_INVAL;
        }
        if(alpha_idx_sz != fc * (uint32_t)sizeof(bk_baf_au_t)) return AVDK_ERR_INVAL;
        if(!aus_in_bounds((const bk_baf_au_t *)alpha_idx_p, fc, alpha_data_sz)) {
            return AVDK_ERR_INVAL;
        }
    }

    /* ---- alias assemble ---- */
    memset(out, 0, sizeof(*out));
    bk_baf_media_t *m = &out->media;
    m->width = h->width;
    m->height = h->height;
    m->alpha_width = h->alpha_width;
    m->alpha_height = h->alpha_height;
    m->frame_count = fc;
    m->rgb.data = rgb_data_p;
    m->rgb.data_size = rgb_data_sz;
    m->rgb.aus = (const bk_baf_au_t *)rgb_idx_p;
    m->rgb.au_count = fc;
    if(h->has_alpha) {
        m->alpha.data = alpha_data_p;
        m->alpha.data_size = alpha_data_sz;
        m->alpha.aus = (const bk_baf_au_t *)alpha_idx_p;
        m->alpha.au_count = fc;
    }
    m->durations_ms = (const uint32_t *)dur_p;

    out->source.magic = BK_BAF_SOURCE_MAGIC;
    out->source.ops = &bk_baf_decoder_ops;
    out->source.data = &out->media;
    return AVDK_ERR_OK;
}
