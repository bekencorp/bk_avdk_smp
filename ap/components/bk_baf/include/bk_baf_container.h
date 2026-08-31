/**
 * @file bk_baf_container.h
 *
 * BAF v1 container parser (see BAF_SPEC_CN.md). Validates a BAF container image
 * held in one memory buffer -- a whole .baf file loaded into RAM, or a compiled-in
 * C byte array (both are the SAME container bytes) -- and aliases its sections
 * in place into a bk_baf_media_t + bk_baf_source_t. No decode, no copy: the
 * produced source feeds the existing bk_baf_open()/poll()/compose() pipeline
 * unchanged. Parser is read-only over @p buf, so a const C array in flash works.
 */

#ifndef BK_BAF_CONTAINER_H
#define BK_BAF_CONTAINER_H

#include "bk_baf.h"          /* bk_baf_source_t / bk_baf_media_t / ops / avdk_err_t */

#ifdef __cplusplus
extern "C" {
#endif

/* ---- On-disk container structs (little-endian, packed). ----
 * Layouts must match BAF_SPEC_CN.md and the packer in baf_tool/tools/to_baf.py. */

#define BAF_MAGIC          "BAFANIM1"   /* 8 bytes, no NUL */
#define BAF_VER_MAJOR      1U
#define BAF_HEADER_SIZE    64U          /* bytes covered before header_crc32 == 60 */

/* FourCC as a little-endian u32 (matches spec appendix A). */
#define BAF_CC(a, b, c, d) \
    ((uint32_t)(uint8_t)(a) | ((uint32_t)(uint8_t)(b) << 8) | \
     ((uint32_t)(uint8_t)(c) << 16) | ((uint32_t)(uint8_t)(d) << 24))
#define BAF_CHUNK_IDX      BAF_CC('I', 'D', 'X', ' ')   /* 0x20584449 */
#define BAF_CHUNK_DUR      BAF_CC('D', 'U', 'R', ' ')   /* 0x20525544 */
#define BAF_CHUNK_DATA     BAF_CC('D', 'A', 'T', 'A')   /* 0x41544144 */

#define BAF_IDX_NONE       0xFFFFFFFFU   /* alpha_idx / alpha_data when no alpha */

typedef struct __attribute__((packed)) {
    char     magic[8];        /* "BAFANIM1" */
    uint16_t ver_major;       /* = 1 */
    uint16_t ver_minor;       /* = 0 */
    uint32_t flags;           /* reserved, 0 */
    uint32_t file_size;       /* total file bytes */
    uint32_t dir_offset;      /* -> ChunkDirectory */
    uint16_t dir_count;
    uint8_t  has_alpha;       /* 0/1 */
    uint8_t  reserved0;
    uint32_t frame_count;
    uint16_t width;
    uint16_t height;
    uint16_t alpha_width;     /* 0 => same as width */
    uint16_t alpha_height;    /* 0 => same as height */
    uint32_t rgb_idx;         /* directory index of RGB 'IDX ' */
    uint32_t rgb_data;        /* directory index of RGB 'DATA' */
    uint32_t alpha_idx;       /* directory index of Alpha 'IDX ' (or BAF_IDX_NONE) */
    uint32_t alpha_data;      /* directory index of Alpha 'DATA' (or BAF_IDX_NONE) */
    uint32_t dur;             /* directory index of 'DUR ' */
    uint32_t header_crc32;    /* CRC32 of the first 60 bytes */
} baf_file_header_t;

typedef struct __attribute__((packed)) {
    uint32_t type;            /* FourCC */
    uint32_t offset;          /* absolute file offset, 64B aligned */
    uint32_t size;            /* chunk byte length */
    uint32_t crc32;           /* 0 = skip */
} baf_chunk_t;

/* IDX entry == bk_baf_au_t layout: { u32 offset; u32 size } */

/* Parse result: media aliased into @p buf, and a ready-to-open source.
 * Zero-initialise before passing to baf_parse_inplace(). */
typedef struct {
    bk_baf_media_t  media;    /* pointers alias into the buffer */
    bk_baf_source_t source;   /* .data=&media, .ops=&bk_baf_decoder_ops */
} baf_view_t;

/* CRC-32 (IEEE 802.3, reflected, poly 0xEDB88320) -- matches zlib/binascii and the
 * packer. Exposed so the FS/LVGL glue can reuse it if needed. */
uint32_t baf_crc32(const uint8_t *data, uint32_t len);

/* Validate the container in @p buf (len bytes) and alias it into @p out.
 * @p buf must stay alive until bk_baf_close() (media points into it). Read-only.
 * Returns AVDK_ERR_OK, or AVDK_ERR_INVAL / AVDK_ERR_UNSUPPORTED on bad input. */
avdk_err_t baf_parse_inplace(const uint8_t *buf, uint32_t len, baf_view_t *out);

#ifdef __cplusplus
}
#endif

#endif /* BK_BAF_CONTAINER_H */
