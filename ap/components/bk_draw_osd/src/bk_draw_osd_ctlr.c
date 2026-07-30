/*
 * bk_draw_osd controller (pipeline compositing model).
 *
 * - Opaque handle + __containerof(handle, private_osd_ctlr_t, ops) lookup.
 * - dynamic_array for runtime display list, mutex-protected add/remove.
 * - Each instance owns an osd_engine bound to external pipeline GPU.
 */
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <sys_types.h>
#include <components/log.h>

#include "bk_draw_osd_ctlr.h"
#include "bk_osd_engine.h"

#define TAG "draw_osd"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

typedef struct {
    bk_draw_osd_ctlr_t   ops;            /* __containerof lookup; position-independent */
    osd_engine_handle_t  engine;
    uint16_t             panel_w;
    uint16_t             panel_h;
    const blend_info_t  *assets;         /* asset table (const, name -> bk_blend_t) */
    uint32_t             assets_size;
    dynamic_array_t      dyn;            /* runtime display list */
    beken_mutex_t        lock;
    uint8_t              next_slot;      /* next free GPU slot for one-shot element/text;
                                          * after array render, points past used clusters; clear resets to 0 */
} private_osd_ctlr_t;

/* ---------------- dynamic_array (ported from fa4592d) ---------------- */

static inline size_t array_length(const blend_info_t *array)
{
    size_t n = 0;
    if (array != NULL) {
        while (array[n].addr != NULL) n++;
    }
    return n;
}

static avdk_err_t dyn_init(dynamic_array_t *dyn, size_t cap)
{
    uint32_t len = cap * sizeof(blend_info_t);
    dyn->entry = os_malloc(len);
    if (dyn->entry == NULL) {
        return AVDK_ERR_NOMEM;
    }
    os_memset((void *)dyn->entry, 0, len);
    dyn->size = 0;
    dyn->capacity = cap;
    return AVDK_ERR_OK;
}

static void dyn_copy_defaults(dynamic_array_t *dyn, const blend_info_t *src)
{
    if (src == NULL) return;
    size_t len = array_length(src);
    if (dyn->size + len + 1 > dyn->capacity) {
        size_t old_cap = dyn->capacity;
        size_t new_cap = dyn->size + len + 1;
        blend_info_t *tmp = os_realloc(dyn->entry, new_cap * sizeof(blend_info_t));
        if (tmp == NULL) {
            LOGE("%s realloc fail\n", __func__);
            return;
        }
        dyn->entry = tmp;
        dyn->capacity = new_cap;
        os_memset(&dyn->entry[old_cap], 0, (new_cap - old_cap) * sizeof(blend_info_t));
    }
    for (size_t i = 0; i < len; i++) {
        dyn->entry[dyn->size++] = src[i];
    }
    dyn->entry[dyn->size].addr = NULL;
}

static const blend_info_t *assets_find_by_name(const blend_info_t *assets, uint32_t size, const char *name)
{
    if (name == NULL || assets == NULL) return NULL;
    for (uint32_t i = 0; i < size; i++) {
        if (os_strcmp((char *)assets[i].name, name) == 0 && assets[i].addr != NULL) {
            return &assets[i];
        }
    }
    return NULL;
}

static const blend_info_t *assets_find_by_content(const blend_info_t *assets, uint32_t size, const char *content)
{
    if (content == NULL || content[0] == '\0' || assets == NULL) return NULL;
    for (uint32_t i = 0; i < size; i++) {
        if (os_strcmp((char *)assets[i].content, content) == 0 && assets[i].addr != NULL) {
            return &assets[i];
        }
    }
    return NULL;
}

static blend_info_t *dyn_find(dynamic_array_t *dyn, const char *name)
{
    if (name == NULL || name[0] == '\0') return NULL;
    for (size_t i = 0; i < dyn->size; i++) {
        if (os_strcmp(dyn->entry[i].name, name) == 0) return &dyn->entry[i];
    }
    return NULL;
}

static avdk_err_t dyn_add_or_update(dynamic_array_t *dyn, const blend_info_t *assets,
                                    uint32_t assets_size, const char *name, const char *content)
{
    if (name == NULL || name[0] == '\0') return AVDK_ERR_INVAL;

    blend_info_t *exist = dyn_find(dyn, name);
    if (exist != NULL) {
        os_strncpy(exist->name, name, sizeof(exist->name) - 1);
        exist->name[sizeof(exist->name) - 1] = '\0';
        if (content != NULL) {
            os_strncpy(exist->content, content, sizeof(exist->content) - 1);
            exist->content[sizeof(exist->content) - 1] = '\0';
            if (content[0] != '\0' && exist->addr->blend_type == BLEND_TYPE_IMAGE) {
                const blend_info_t *img = assets_find_by_content(assets, assets_size, content);
                if (img != NULL) exist->addr = img->addr;
                else             exist->content[0] = '\0';
            }
        }
        return AVDK_ERR_OK;
    }

    const blend_info_t *asset = assets_find_by_name(assets, assets_size, name);
    if (asset == NULL) {
        LOGW("%s: asset '%s' not found\n", __func__, name);
        return AVDK_ERR_INVAL;
    }

    if (dyn->size + 1 >= dyn->capacity) {
        size_t old_cap = dyn->capacity;
        size_t new_cap = (dyn->capacity > 0) ? (dyn->capacity * 2) : 4;
        if (new_cap < dyn->size + 2) new_cap = dyn->size + 2;
        blend_info_t *tmp = os_realloc(dyn->entry, new_cap * sizeof(blend_info_t));
        if (tmp == NULL) {
            LOGE("%s realloc fail\n", __func__);
            return AVDK_ERR_NOMEM;
        }
        dyn->entry = tmp;
        dyn->capacity = new_cap;
        os_memset(&dyn->entry[old_cap], 0, (new_cap - old_cap) * sizeof(blend_info_t));
    }

    dyn->entry[dyn->size] = *asset;
    os_strncpy(dyn->entry[dyn->size].name, name, sizeof(dyn->entry[dyn->size].name) - 1);
    dyn->entry[dyn->size].name[sizeof(dyn->entry[dyn->size].name) - 1] = '\0';
    if (content != NULL) {
        os_strncpy(dyn->entry[dyn->size].content, content, sizeof(dyn->entry[dyn->size].content) - 1);
        dyn->entry[dyn->size].content[sizeof(dyn->entry[dyn->size].content) - 1] = '\0';
        if (content[0] != '\0' && dyn->entry[dyn->size].addr->blend_type == BLEND_TYPE_IMAGE) {
            const blend_info_t *img = assets_find_by_content(assets, assets_size, content);
            if (img != NULL) dyn->entry[dyn->size].addr = img->addr;
            else             dyn->entry[dyn->size].content[0] = '\0';
        }
    }
    dyn->size++;
    dyn->entry[dyn->size].addr = NULL;
    return AVDK_ERR_OK;
}

static void dyn_remove(dynamic_array_t *dyn, const char *name)
{
    if (name == NULL || name[0] == '\0' || dyn->size == 0) return;
    int idx = -1;
    for (size_t i = 0; i < dyn->size; i++) {
        if (os_strcmp(dyn->entry[i].name, name) == 0) { idx = (int)i; break; }
    }
    if (idx < 0) {
        LOGW("remove: '%s' not found\n", name);
        return;
    }
    for (size_t i = idx; i < dyn->size - 1; i++) {
        dyn->entry[i] = dyn->entry[i + 1];
    }
    dyn->size--;
    dyn->entry[dyn->size].addr = NULL;
}

/* ---------------- vtable implementation ---------------- */

static private_osd_ctlr_t *priv_of(bk_draw_osd_ctlr_handle_t handle)
{
    return __containerof(handle, private_osd_ctlr_t, ops);
}

/* One-shot single element (image or font): tight sprite at element xpos/ypos, next free slot.
 * Image copies image.data; font uses bk_font glyphs + .color; text from content or name. */
static avdk_err_t osd_draw_element(bk_draw_osd_ctlr_handle_t handle, const blend_info_t *info)
{
    private_osd_ctlr_t *p = priv_of(handle);
    if (info == NULL || info->addr == NULL) {
        return AVDK_ERR_INVAL;
    }
    const bk_blend_t *b = info->addr;
    uint16_t w = (uint16_t)(b->width  ? b->width  : b->icon_width);
    uint16_t h = (uint16_t)(b->height ? b->height : b->icon_height);
    if (w == 0 || h == 0) {
        return AVDK_ERR_INVAL;
    }

    rtos_lock_mutex(&p->lock);
    if (p->next_slot >= BK_GPU_BLIT_SLOT_MAX) {
        LOGW("draw_element: no free slot (%u used)\n", p->next_slot);
        rtos_unlock_mutex(&p->lock);
        return AVDK_ERR_NOMEM;
    }
    osd_engine_set_slot(p->engine, p->next_slot);
    avdk_err_t ret = osd_engine_begin(p->engine, w, h, b->xpos, b->ypos);
    if (ret != AVDK_ERR_OK) {
        rtos_unlock_mutex(&p->lock);
        return ret;
    }
    if (b->blend_type == BLEND_TYPE_IMAGE) {
        osd_engine_put_icon(p->engine, b, 0, 0);
    } else if (b->font.font_digit_type != NULL) {
        /* Asset fonts are bkfont-only today; extend blend_font_t with kind for LVGL clustering */
        const char *text = (info->content[0] != '\0') ? info->content : b->name;
        osd_engine_put_text(p->engine, OSD_FONT_BKFONT, b->font.font_digit_type,
                            text, 0, 0, b->font.color, 1);
    }
    ret = osd_engine_commit(p->engine);
    if (ret == AVDK_ERR_OK) p->next_slot++;
    rtos_unlock_mutex(&p->lock);
    return ret;
}

/* One-shot raw font text: sprite sized to text, submitted to next free slot. */
static avdk_err_t osd_draw_text(bk_draw_osd_ctlr_handle_t handle, osd_font_kind_t kind,
                                const void *font, const char *utf8,
                                uint16_t x, uint16_t y, uint32_t argb, uint8_t scale)
{
    private_osd_ctlr_t *p = priv_of(handle);
    if (font == NULL || utf8 == NULL) {
        return AVDK_ERR_INVAL;
    }
    if (scale == 0) scale = 1;

    uint16_t w = 0, h = 0;
    osd_engine_text_extent(kind, font, utf8, scale, &w, &h);
    if (w == 0 || h == 0) {
        return AVDK_ERR_INVAL;
    }

    rtos_lock_mutex(&p->lock);
    if (p->next_slot >= BK_GPU_BLIT_SLOT_MAX) {
        LOGW("draw_text: no free slot (%u used)\n", p->next_slot);
        rtos_unlock_mutex(&p->lock);
        return AVDK_ERR_NOMEM;
    }
    osd_engine_set_slot(p->engine, p->next_slot);
    avdk_err_t ret = osd_engine_begin(p->engine, w, h, x, y);
    if (ret != AVDK_ERR_OK) {
        rtos_unlock_mutex(&p->lock);
        return ret;
    }
    osd_engine_put_text(p->engine, kind, font, utf8, 0, 0, argb, scale);
    ret = osd_engine_commit(p->engine);
    if (ret == AVDK_ERR_OK) p->next_slot++;
    rtos_unlock_mutex(&p->lock);
    return ret;
}

/* Array render: auto-cluster elements into GPU slots (recommended path).
 * Nearby elements share one tight sprite/slot; distant elements use separate slots.
 * When element count exceeds slot limit, merge clusters with least added blank area.
 * next_slot advances past used clusters for subsequent element/text calls. */
#define OSD_CLUSTER_MAX_ELEMS 32   /* max elements per render (extras ignored with warning) */

typedef struct { int32_t x0, y0, x1, y1; } osd_rect_t;   /* [x0,x1) x [y0,y1) display coords */

static uint32_t osd_rect_area(const osd_rect_t *r)
{
    int32_t w = r->x1 - r->x0, h = r->y1 - r->y0;
    return (w > 0 && h > 0) ? (uint32_t)w * (uint32_t)h : 0;
}

static void osd_rect_union(osd_rect_t *d, const osd_rect_t *a, const osd_rect_t *b)
{
    d->x0 = a->x0 < b->x0 ? a->x0 : b->x0;
    d->y0 = a->y0 < b->y0 ? a->y0 : b->y0;
    d->x1 = a->x1 > b->x1 ? a->x1 : b->x1;
    d->y1 = a->y1 > b->y1 ? a->y1 : b->y1;
}

static avdk_err_t osd_draw_osd_array(bk_draw_osd_ctlr_handle_t handle, const blend_info_t *list)
{
    private_osd_ctlr_t *p = priv_of(handle);

    rtos_lock_mutex(&p->lock);
    const blend_info_t *arr = (list != NULL) ? list : p->dyn.entry;
    if (arr == NULL || arr[0].addr == NULL) {
        rtos_unlock_mutex(&p->lock);
        return AVDK_ERR_OK;
    }

    /* 1) Collect display rects for visible elements */
    const blend_info_t *elem[OSD_CLUSTER_MAX_ELEMS];
    osd_rect_t rect[OSD_CLUSTER_MAX_ELEMS];
    int n = 0;
    for (const blend_info_t *it = arr; it->addr != NULL; it++) {
        const bk_blend_t *b = it->addr;
        int32_t w = (int32_t)(b->width  ? b->width  : b->icon_width);
        int32_t h = (int32_t)(b->height ? b->height : b->icon_height);
        if (w <= 0 || h <= 0) continue;
        if (n >= OSD_CLUSTER_MAX_ELEMS) {
            LOGW("blend list > %d elems, extra ignored\n", OSD_CLUSTER_MAX_ELEMS);
            break;
        }
        elem[n] = it;
        rect[n].x0 = b->xpos;      rect[n].y0 = b->ypos;
        rect[n].x1 = b->xpos + w;  rect[n].y1 = b->ypos + h;
        n++;
    }
    if (n == 0) {
        rtos_unlock_mutex(&p->lock);
        return AVDK_ERR_INVAL;
    }

    /* 2) Start with one cluster per element */
    int        cid[OSD_CLUSTER_MAX_ELEMS];    /* element -> cluster id (reuses element index) */
    osd_rect_t cbox[OSD_CLUSTER_MAX_ELEMS];   /* cluster bounding box */
    bool       active[OSD_CLUSTER_MAX_ELEMS]; /* cluster alive flag */
    for (int i = 0; i < n; i++) { cid[i] = i; cbox[i] = rect[i]; active[i] = true; }
    int nclusters = n;

    /* 3) Agglomerative merge to <= BK_GPU_BLIT_SLOT_MAX: pick pair with smallest added blank area
     *    (cost = merged bbox area - sum of cluster areas) */
    while (nclusters > BK_GPU_BLIT_SLOT_MAX) {
        int ba = -1, bb = -1;
        int64_t best = 0; bool have = false;
        for (int a = 0; a < n; a++) {
            if (!active[a]) continue;
            for (int c = a + 1; c < n; c++) {
                if (!active[c]) continue;
                osd_rect_t u; osd_rect_union(&u, &cbox[a], &cbox[c]);
                int64_t cost = (int64_t)osd_rect_area(&u)
                             - (int64_t)osd_rect_area(&cbox[a])
                             - (int64_t)osd_rect_area(&cbox[c]);
                if (!have || cost < best) { best = cost; have = true; ba = a; bb = c; }
            }
        }
        if (!have) break;
        osd_rect_union(&cbox[ba], &cbox[ba], &cbox[bb]);
        for (int i = 0; i < n; i++) if (cid[i] == bb) cid[i] = ba;
        active[bb] = false;
        nclusters--;
    }

    /* 4) Per cluster: compact to slots 0..nclusters-1, composite tight sprite, commit */
    avdk_err_t ret = AVDK_ERR_OK;
    uint8_t slot = 0;
    for (int c = 0; c < n; c++) {
        if (!active[c]) continue;
        osd_rect_t *bx = &cbox[c];
        uint16_t sw = (uint16_t)(bx->x1 - bx->x0);
        uint16_t sh = (uint16_t)(bx->y1 - bx->y0);

        osd_engine_set_slot(p->engine, slot);
        ret = osd_engine_begin(p->engine, sw, sh, (uint16_t)bx->x0, (uint16_t)bx->y0);
        if (ret != AVDK_ERR_OK) break;

        for (int i = 0; i < n; i++) {
            if (cid[i] != c) continue;
            const bk_blend_t *b = elem[i]->addr;
            uint16_t x = (uint16_t)(rect[i].x0 - bx->x0);
            uint16_t y = (uint16_t)(rect[i].y0 - bx->y0);
            if (b->blend_type == BLEND_TYPE_IMAGE) {
                osd_engine_put_icon(p->engine, b, x, y);
            } else if (b->font.font_digit_type != NULL) {
                /* Same as osd_draw_element: dispatch by b->font.kind when LVGL assets are supported */
                const char *text = (elem[i]->content[0] != '\0') ? elem[i]->content : b->name;
                osd_engine_put_text(p->engine, OSD_FONT_BKFONT, b->font.font_digit_type,
                                    text, x, y, b->font.color, 1);
            }
        }
        ret = osd_engine_commit(p->engine);
        if (ret != AVDK_ERR_OK) break;
        slot++;
    }

    p->next_slot = slot;   /* cursor past used clusters for follow-up element/text */
    rtos_unlock_mutex(&p->lock);
    LOGI("osd array: %d elems -> %u slots (auto-cluster)\n", n, slot);
    return ret;
}

static avdk_err_t osd_clear(bk_draw_osd_ctlr_handle_t handle)
{
    private_osd_ctlr_t *p = priv_of(handle);
    rtos_lock_mutex(&p->lock);
    p->next_slot = 0;      /* reset cursor; next render starts at slot 0 */
    avdk_err_t ret = osd_engine_clear(p->engine);
    rtos_unlock_mutex(&p->lock);
    return ret;
}

static avdk_err_t osd_add_or_update(bk_draw_osd_ctlr_handle_t handle, const char *name, const char *content)
{
    private_osd_ctlr_t *p = priv_of(handle);
    if (name == NULL) return AVDK_ERR_INVAL;
    rtos_lock_mutex(&p->lock);
    avdk_err_t ret = dyn_add_or_update(&p->dyn, p->assets, p->assets_size, name, content);
    rtos_unlock_mutex(&p->lock);
    return ret;
}

static avdk_err_t osd_remove(bk_draw_osd_ctlr_handle_t handle, const char *name)
{
    private_osd_ctlr_t *p = priv_of(handle);
    if (name == NULL) return AVDK_ERR_INVAL;
    rtos_lock_mutex(&p->lock);
    dyn_remove(&p->dyn, name);
    rtos_unlock_mutex(&p->lock);
    return AVDK_ERR_OK;
}

static avdk_err_t osd_ioctl(bk_draw_osd_ctlr_handle_t handle, uint32_t cmd,
                            uint32_t p1, uint32_t p2, uint32_t p3)
{
    private_osd_ctlr_t *p = priv_of(handle);
    switch (cmd) {
        case OSD_CTLR_CMD_GET_ALL_ASSETS:
            if (p2) *((const blend_info_t **)p2) = p->assets;
            if (p3) *((uint32_t *)p3) = p->assets_size;
            if (p1 && p->assets) {
                uint32_t n = 0;
                for (const blend_info_t *it = p->assets; it->addr != NULL; it++, n++) {
                    LOGI("asset[%u] name=%s type=%s %ux%u\n", n, it->name,
                         it->addr->blend_type == BLEND_TYPE_IMAGE ? "img" : "font",
                         it->addr->width, it->addr->height);
                }
            }
            return AVDK_ERR_OK;
        case OSD_CTLR_CMD_GET_DRAW_INFO:
            rtos_lock_mutex(&p->lock);
            if (p2) *((const blend_info_t **)p2) = p->dyn.entry;
            if (p3) *((uint32_t *)p3) = (uint32_t)p->dyn.size;
            if (p1) {
                LOGI("draw list: %u elements\n", (uint32_t)p->dyn.size);
                for (size_t i = 0; i < p->dyn.size; i++) {
                    LOGI("  [%u] name=%s content=%s\n", (uint32_t)i,
                         p->dyn.entry[i].name, p->dyn.entry[i].content);
                }
            }
            rtos_unlock_mutex(&p->lock);
            return AVDK_ERR_OK;
        default:
            LOGW("ioctl: unsupported cmd %u\n", cmd);
            return AVDK_ERR_UNSUPPORTED;
    }
}

static void osd_ctlr_destroy(private_osd_ctlr_t *p)
{
    if (p == NULL) return;
    if (p->engine) {
        osd_engine_delete(p->engine);
        p->engine = NULL;
    }
    if (p->lock) {
        rtos_deinit_mutex(&p->lock);
        p->lock = NULL;
    }
    if (p->dyn.entry) {
        os_free(p->dyn.entry);
        p->dyn.entry = NULL;
    }
    os_free(p);
}

static avdk_err_t osd_delete(bk_draw_osd_ctlr_handle_t handle)
{
    private_osd_ctlr_t *p = priv_of(handle);
    osd_ctlr_destroy(p);
    LOGI("osd controller deleted\n");
    return AVDK_ERR_OK;
}

/* ---------------- creation ---------------- */

avdk_err_t osd_ctlr_new(bk_draw_osd_ctlr_handle_t *handle, osd_ctlr_config_t *config)
{
    AVDK_RETURN_ON_FALSE(handle && config, AVDK_ERR_INVAL, TAG, "handle/config NULL");
    AVDK_RETURN_ON_FALSE(config->gpu, AVDK_ERR_INVAL, TAG, "config->gpu NULL");

    private_osd_ctlr_t *p = os_malloc(sizeof(private_osd_ctlr_t));
    AVDK_RETURN_ON_FALSE(p, AVDK_ERR_NOMEM, TAG, "malloc failed");
    os_memset(p, 0, sizeof(*p));

    avdk_err_t ret;
    p->panel_w     = config->panel_w;
    p->panel_h     = config->panel_h;
    p->assets      = config->blend_assets;
    p->assets_size = config->blend_assets ? (uint32_t)array_length(config->blend_assets) : 0;

    osd_engine_config_t ecfg = {
        .gpu        = config->gpu,
        .panel_w    = config->panel_w,
        .panel_h    = config->panel_h,
        .src_format = config->src_format,
    };
    ret = osd_engine_new(&p->engine, &ecfg);
    if (ret != AVDK_ERR_OK) {
        LOGE("osd_engine_new failed %d\n", ret);
        goto err;
    }

    ret = dyn_init(&p->dyn, p->assets_size + 1);
    if (ret != AVDK_ERR_OK) {
        LOGE("dyn_init failed\n");
        goto err;
    }
    dyn_copy_defaults(&p->dyn, config->blend_info);

    ret = rtos_init_mutex(&p->lock);
    if (ret != BK_OK) {
        LOGE("mutex init failed\n");
        p->lock = NULL;
        ret = AVDK_ERR_GENERIC;
        goto err;
    }

    p->ops.draw_element   = osd_draw_element;
    p->ops.draw_text      = osd_draw_text;
    p->ops.draw_osd_array = osd_draw_osd_array;
    p->ops.clear          = osd_clear;
    p->ops.add_or_update  = osd_add_or_update;
    p->ops.remove         = osd_remove;
    p->ops.ioctl          = osd_ioctl;
    p->ops.delete         = osd_delete;

    *handle = &p->ops;
    LOGI("osd controller created (panel %ux%u)\n", p->panel_w, p->panel_h);
    return AVDK_ERR_OK;

err:
    osd_ctlr_destroy(p);
    return ret;
}
