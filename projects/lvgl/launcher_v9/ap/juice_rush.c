/**
 * @file juice_rush.c
 * @brief Juice Rush: slice-fruit game (LVGL v9, plain C).
 *
 * Fruit is thrown up from the bottom of the screen and sliced by dragging.
 * A missed fruit or a bomb costs one life; the score is settled after 60 s.
 *
 * Software-rendering choices on BK7259:
 *   1. No image assets: everything is drawn with lv_obj primitives.
 *   2. The background and its decorations are static and kept outside the main
 *      fruit flight area, so invalidated areas stay limited to moving objects.
 *   3. Fruit, pieces, particles, popups, slashes and rings are pooled; objects
 *      are never created or destroyed during play, keeping the LVGL heap stable.
 *   4. Physics runs from an lv_timer and uses real tick deltas, so frame-rate
 *      variation does not change the feel.
 *   5. Sliced halves are approximated with same-color circles to avoid the extra
 *      cost of masks and clipping.
 */
#include "lvgl.h"
#include <string.h>
#include "juice_rush.h"
#include "launcher.h"

/* ---------------------------- Tunables ---------------------------- */
#define JR_FRUIT_MAX       8        /* Maximum fruit in flight. */
#define JR_PIECE_MAX       16       /* Sliced halves. */
#define JR_PARTICLE_MAX    28       /* Juice particles. */
#define JR_POPUP_MAX       4        /* Floating score labels. */
#define JR_RING_MAX        4        /* Impact shockwave rings. */
#define JR_SLASH_MAX       4        /* Slash flashes. */
#define JR_TRAIL_MAX       8        /* Blade trail sample points. */

#define JR_TICK_MS         16       /* Physics period. */
#define JR_GAME_MS         (60u * 1000u)
#define JR_GRAVITY         900.0f   /* px/s^2, tuned for ~1.6 s hang time on 480x480. */
#define JR_COMBO_WINDOW_MS 650u     /* Combo window. */
#define JR_TRAIL_OPA       235      /* Maximum blade trail opacity. */

/* Allowed apex position as a percentage of screen height (smaller = higher).
 * The upper bound keeps fruit reachable and the lower bound keeps it out of the HUD. */
#define JR_APEX_TOP_PCT    28
#define JR_APEX_BOTTOM_PCT 62
#define JR_APEX_MIN_RISE   220.0f   /* Minimum rise above the launch point, in pixels. */

#define JR_COLOR_BG        0x0A1220
#define JR_COLOR_CARD      0x152238
#define JR_COLOR_CARD2     0x0C1626
#define JR_COLOR_LINE      0x2B3F5C
#define JR_COLOR_DIM       0x8FA6C0

/* Total top HUD height: status pills plus the time bar and padding.
 * The play field starts at JR_HUD_H so the bar stays visually centered. */
#define JR_HUD_H           82

/* ---------------------------- Data types ---------------------------- */
typedef enum {
    JR_STATE_IDLE = 0,
    JR_STATE_START,
    JR_STATE_PLAY,
    JR_STATE_OVER,
} jr_state_t;

/* Fruit appearance: outer gradient and pulp color. */
typedef struct {
    uint32_t top;
    uint32_t bottom;
    uint32_t pulp;
} jr_kind_t;

typedef struct {
    lv_obj_t *ball;     /* Main circle. */
    lv_obj_t *shine;    /* Highlight, or fuse spark for bombs. */
    bool used;
    bool bomb;
    uint8_t kind;
    float x, y, vx, vy; /* Pixels and seconds. */
    int32_t r;
} jr_fruit_t;

typedef struct {
    lv_obj_t *obj;
    bool used;
    float x, y, vx, vy;
    int32_t r;
    uint32_t ttl;       /* Remaining lifetime in ms. */
    uint32_t ttl_max;
} jr_bit_t;

typedef struct {
    lv_obj_t *lbl;
    bool used;
    float x, y;
    uint32_t ttl;
    uint32_t ttl_max;
} jr_popup_t;

/* Shockwave ring expanding from the cut point. */
typedef struct {
    lv_obj_t *obj;
    bool used;
    float x, y;
    int32_t r_start;
    int32_t r_end;
    uint32_t ttl;
    uint32_t ttl_max;
} jr_ring_t;

/* Slash flash: fixed line segment that only fades out. */
typedef struct {
    lv_obj_t *obj;
    bool used;
    uint32_t ttl;
    uint32_t ttl_max;
} jr_slash_t;

/* ---------------------------- Static data ---------------------------- */
static const jr_kind_t s_kinds[] = {
    {0x5ACB6E, 0x1C7A36, 0xF2546E},   /* Watermelon */
    {0xFFB648, 0xE0701A, 0xFFA24A},   /* Orange */
    {0xFF6B6B, 0xC21F35, 0xFFF0D2},   /* Apple */
    {0xFFE86B, 0xDEA912, 0xFFF6B8},   /* Lemon */
    {0x8B7BFF, 0x4534C7, 0xC9BCFF},   /* Blueberry */
};
#define JR_KIND_NUM ((int)(sizeof(s_kinds) / sizeof(s_kinds[0])))

static lv_obj_t *s_root;        /* Full-screen root. */
static lv_obj_t *s_bg;          /* Static background decorations. */
static lv_obj_t *s_play;        /* Game object layer. */
static lv_obj_t *s_ui;          /* HUD, start page and result page. */
static lv_obj_t *s_touch;       /* Transparent touch layer. */
static lv_obj_t *s_overlay;     /* Current overlay: start or result page. */
static lv_obj_t *s_hud;

static lv_obj_t *s_lbl_score;
static lv_obj_t *s_lbl_time;
static lv_obj_t *s_lbl_combo;
static lv_obj_t *s_bar_time;
static lv_obj_t *s_heart[3];

static lv_obj_t *s_trail_obj;
static lv_point_precise_t s_trail[JR_TRAIL_MAX];
static lv_point_precise_t s_trail_rel[JR_TRAIL_MAX];
static int s_trail_n;
static lv_opa_t s_trail_opa;
static bool s_drag;
static lv_point_t s_prev_pt;

static jr_fruit_t s_fruits[JR_FRUIT_MAX];
static jr_bit_t s_pieces[JR_PIECE_MAX];
static jr_bit_t s_particles[JR_PARTICLE_MAX];
static jr_popup_t s_popups[JR_POPUP_MAX];
static jr_ring_t s_rings[JR_RING_MAX];
static jr_slash_t s_slashes[JR_SLASH_MAX];
/* lv_line keeps a pointer to the point array, so each slash needs its own. */
static lv_point_precise_t s_slash_pts[JR_SLASH_MAX][2];

static lv_timer_t *s_timer;
static int32_t s_w, s_h;
static jr_state_t s_state;
static uint32_t s_last_tick;
static uint32_t s_start_tick;
static uint32_t s_next_spawn_tick;
static uint32_t s_last_slice_tick;
static int32_t s_score, s_best, s_combo, s_lives;
static int32_t s_max_combo;
static bool s_new_record;
static int32_t s_last_score_ui, s_last_sec_ui, s_last_pct_ui, s_last_combo_ui;

/* ---------------------------- Forward declarations ---------------------------- */
static void jr_tick(lv_timer_t *timer);
static void jr_show_start(void);
static void jr_start_game(void);
static void jr_game_over(void);
static void jr_hide_all(void);
static void jr_trail_refresh(lv_opa_t opa);
static void jr_hud_update(bool force);

/* ---------------------------- Basic drawing ---------------------------- */
static lv_color_t jr_hex(uint32_t v)
{
    return lv_color_hex(v);
}

/* Normalize a 2D vector using integer sqrt to avoid pulling in libm. */
static void jr_norm(float *x, float *y)
{
    float len2 = (*x) * (*x) + (*y) * (*y);
    float len;

    if (len2 < 1.0f) {
        *x = 0.0f;
        *y = -1.0f;
        return;
    }
    len = (float)lv_sqrt32((uint32_t)len2);
    if (len < 1.0f) {
        len = 1.0f;
    }
    *x /= len;
    *y /= len;
}

/* Plain circle without theme styles. */
static lv_obj_t *jr_ball_create(lv_obj_t *parent)
{
    lv_obj_t *o = lv_obj_create(parent);

    lv_obj_remove_style_all(o);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(o, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_grad_dir(o, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_width(o, 2, 0);
    /* Let the fuse spark exceed the circle bounds. */
    lv_obj_add_flag(o, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    return o;
}

/* Create the body and highlight objects and store them in f. */
static void jr_ball_pair_create(lv_obj_t *parent, jr_fruit_t *f)
{
    f->ball = jr_ball_create(parent);

    f->shine = lv_obj_create(f->ball);
    lv_obj_remove_style_all(f->shine);
    lv_obj_remove_flag(f->shine, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(f->shine, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(f->shine, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(f->shine, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(f->shine, 110, 0);

    lv_obj_add_flag(f->ball, LV_OBJ_FLAG_HIDDEN);
}

/* Refresh appearance for a fruit kind and size. */
static void jr_fruit_setup(jr_fruit_t *f, uint8_t kind, bool bomb, int32_t r)
{
    int32_t d = r * 2;

    f->r = r;
    f->kind = kind;
    f->bomb = bomb;

    lv_obj_set_size(f->ball, d, d);
    if (bomb) {
        lv_obj_set_style_bg_color(f->ball, jr_hex(0x2B3242), 0);
        lv_obj_set_style_bg_grad_color(f->ball, jr_hex(0x11151F), 0);
        lv_obj_set_style_border_width(f->ball, 4, 0);
        lv_obj_set_style_border_color(f->ball, jr_hex(0xFF5252), 0);
        /* Bombs use the highlight slot as a red fuse spark. */
        lv_obj_set_size(f->shine, d * 20 / 100, d * 20 / 100);
        lv_obj_set_pos(f->shine, d * 40 / 100, -d * 4 / 100);
        lv_obj_set_style_bg_color(f->shine, jr_hex(0xFF7A45), 0);
        lv_obj_set_style_bg_opa(f->shine, LV_OPA_COVER, 0);
    } else {
        lv_obj_set_style_bg_color(f->ball, jr_hex(s_kinds[kind].top), 0);
        lv_obj_set_style_bg_grad_color(f->ball, jr_hex(s_kinds[kind].bottom), 0);
        lv_obj_set_style_border_width(f->ball, 2, 0);
        lv_obj_set_style_border_color(f->ball, jr_hex(s_kinds[kind].bottom), 0);
        lv_obj_set_size(f->shine, d * 28 / 100, d * 28 / 100);
        lv_obj_set_pos(f->shine, d * 17 / 100, d * 15 / 100);
        lv_obj_set_style_bg_color(f->shine, lv_color_white(), 0);
        lv_obj_set_style_bg_opa(f->shine, 105, 0);
    }
}

/* Decorative fruit for the start page; not pooled. */
static void jr_deco_fruit(lv_obj_t *parent, int32_t cx, int32_t cy, int32_t r, uint8_t kind)
{
    jr_fruit_t tmp;

    memset(&tmp, 0, sizeof(tmp));
    jr_ball_pair_create(parent, &tmp);
    jr_fruit_setup(&tmp, kind, false, r);
    lv_obj_set_pos(tmp.ball, cx - r, cy - r);
    lv_obj_remove_flag(tmp.ball, LV_OBJ_FLAG_HIDDEN);
}

/* Flat translucent dot used for glow and stars. */
static lv_obj_t *jr_soft_dot(lv_obj_t *parent, int32_t d, uint32_t color, lv_opa_t opa)
{
    lv_obj_t *o = lv_obj_create(parent);

    lv_obj_remove_style_all(o);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(o, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(o, opa, 0);
    lv_obj_set_style_bg_color(o, jr_hex(color), 0);
    lv_obj_set_size(o, d, d);
    return o;
}

/* Static background decoration, kept away from the main fruit flight area. */
static void jr_build_bg(void)
{
    static const int32_t glow_dia[3] = {430, 320, 220};
    static const uint32_t glow_col[3] = {0x2A477A, 0x35588F, 0x4169A8};
    static const int glow_opa[3] = {34, 38, 44};
    const int32_t glow_cy = 96;
    lv_obj_t *o;
    int i;

    s_bg = lv_obj_create(s_root);
    lv_obj_remove_style_all(s_bg);
    lv_obj_remove_flag(s_bg, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(s_bg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(s_bg, s_w, s_h);
    lv_obj_set_pos(s_bg, 0, 0);

    /* Top glow: three concentric circles fake a radial gradient. Created before
     * the HUD panel so the translucent glow does not wash it out. */
    for (i = 0; i < 3; i++) {
        o = jr_soft_dot(s_bg, glow_dia[i], glow_col[i], (lv_opa_t)glow_opa[i]);
        lv_obj_set_pos(o, s_w / 2 - glow_dia[i] / 2, glow_cy - glow_dia[i] / 2);
    }

    /* Top HUD panel: translucent dark fill plus a bottom highlight line. */
    o = lv_obj_create(s_bg);
    lv_obj_remove_style_all(o);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(o, s_w, JR_HUD_H);
    lv_obj_set_pos(o, 0, 0);
    lv_obj_set_style_bg_opa(o, 150, 0);
    lv_obj_set_style_bg_color(o, jr_hex(0x0E1B2F), 0);
    lv_obj_set_style_bg_grad_color(o, jr_hex(0x15263E), 0);
    lv_obj_set_style_bg_grad_dir(o, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_width(o, 2, 0);
    lv_obj_set_style_border_side(o, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(o, jr_hex(0x2A4A72), 0);

    /* Playfield frame: rounded outline with L-shaped corner marks. */
    {
        const int32_t fx = 10;
        const int32_t fy = JR_HUD_H;
        const int32_t fw = s_w - 20;
        const int32_t fh = s_h - JR_HUD_H - 64;
        const int32_t bl = 26;      /* Corner mark arm length. */
        const int32_t bt = 3;       /* Corner mark thickness. */
        const int32_t bi = 12;      /* Corner mark inset from the frame. */

        o = lv_obj_create(s_bg);
        lv_obj_remove_style_all(o);
        lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(o, fw, fh);
        lv_obj_set_pos(o, fx, fy);
        lv_obj_set_style_bg_opa(o, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(o, 2, 0);
        lv_obj_set_style_border_color(o, jr_hex(0x2A4A72), 0);
        lv_obj_set_style_border_opa(o, 150, 0);
        lv_obj_set_style_radius(o, 14, 0);

        for (i = 0; i < 4; i++) {
            int32_t x = (i & 1) ? (fx + fw - bi - bl) : (fx + bi);
            int32_t y = (i & 2) ? (fy + fh - bi - bl) : (fy + bi);
            int32_t hx = x;
            int32_t hy = (i & 2) ? (y + bl - bt) : y;
            int32_t vx = (i & 1) ? (x + bl - bt) : x;

            /* Horizontal arm. */
            o = lv_obj_create(s_bg);
            lv_obj_remove_style_all(o);
            lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_size(o, bl, bt);
            lv_obj_set_pos(o, hx, hy);
            lv_obj_set_style_bg_opa(o, 220, 0);
            lv_obj_set_style_bg_color(o, jr_hex(0x5E9FE0), 0);
            lv_obj_set_style_radius(o, 2, 0);

            /* Vertical arm. */
            o = lv_obj_create(s_bg);
            lv_obj_remove_style_all(o);
            lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_size(o, bt, bl);
            lv_obj_set_pos(o, vx, y);
            lv_obj_set_style_bg_opa(o, 220, 0);
            lv_obj_set_style_bg_color(o, jr_hex(0x5E9FE0), 0);
            lv_obj_set_style_radius(o, 2, 0);
        }
    }

    /* Ground strip so fruit appears to launch from the floor. */
    o = lv_obj_create(s_bg);
    lv_obj_remove_style_all(o);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(o, s_w, 64);
    lv_obj_set_pos(o, 0, s_h - 64);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(o, jr_hex(0x0B1526), 0);
    lv_obj_set_style_bg_grad_color(o, jr_hex(0x05080E), 0);
    lv_obj_set_style_bg_grad_dir(o, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_width(o, 3, 0);
    lv_obj_set_style_border_side(o, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_color(o, jr_hex(0x21406B), 0);

    /* Three juice puddles on the ground; only the upper half is visible. */
    {
        static const uint32_t dome_col[3] = {0x7A3E8F, 0xC2622A, 0x2E7A57};
        static const int32_t dome_x[3] = {24, 50, 76};   /* Percentage of screen width. */
        static const int32_t dome_r[3] = {46, 36, 42};

        for (i = 0; i < 3; i++) {
            o = jr_soft_dot(s_bg, dome_r[i] * 2, dome_col[i], 60);
            lv_obj_set_pos(o, s_w * dome_x[i] / 100 - dome_r[i], s_h - 18 - dome_r[i]);
        }
    }

    /* Stars in the upper half. */
    for (i = 0; i < 16; i++) {
        int32_t d = (int32_t)lv_rand(3, 6);
        int32_t x = (int32_t)lv_rand(20, s_w - 20);
        int32_t y = (int32_t)lv_rand(100, 340);

        o = jr_soft_dot(s_bg, d, 0x4E79B8, (lv_opa_t)lv_rand(90, 190));
        lv_obj_set_pos(o, x, y);
    }
}

/* Shared rounded button. */
static lv_obj_t *jr_button_create(lv_obj_t *parent, const char *txt, lv_event_cb_t cb,
                                  int32_t w, int32_t h)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_t *lbl;

    lv_obj_set_size(btn, w, h);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn, jr_hex(0x3ED47A), 0);
    lv_obj_set_style_bg_grad_color(btn, jr_hex(0x1B9C52), 0);
    lv_obj_set_style_bg_grad_dir(btn, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_shadow_width(btn, 24, 0);
    lv_obj_set_style_shadow_color(btn, jr_hex(0x0A5A2C), 0);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_60, 0);
    lv_obj_set_style_shadow_offset_y(btn, 8, 0);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);

    lbl = lv_label_create(btn);
    lv_label_set_text(lbl, txt);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_center(lbl);
    return btn;
}

/* ---------------------------- Blade trail ---------------------------- */
static void jr_trail_clear(void)
{
    s_trail_n = 0;
    s_trail_opa = 0;
    if (s_trail_obj) {
        lv_obj_add_flag(s_trail_obj, LV_OBJ_FLAG_HIDDEN);
    }
}

static void jr_trail_push(int32_t x, int32_t y)
{
    if (s_trail_n >= JR_TRAIL_MAX) {
        memmove(&s_trail[0], &s_trail[1], sizeof(s_trail[0]) * (JR_TRAIL_MAX - 1));
        s_trail_n = JR_TRAIL_MAX - 1;
    }
    s_trail[s_trail_n].x = x;
    s_trail[s_trail_n].y = y;
    s_trail_n++;
}

static void jr_trail_refresh(lv_opa_t opa)
{
    int32_t minx = 0x7FFFFFFF, miny = 0x7FFFFFFF;
    int32_t maxx = -0x7FFFFFFF, maxy = -0x7FFFFFFF;
    const int32_t pad = 12;
    int32_t x, y, w, h;
    int i;

    if (s_trail_obj == NULL) {
        return;
    }
    if (s_trail_n < 2 || opa == 0) {
        lv_obj_add_flag(s_trail_obj, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    for (i = 0; i < s_trail_n; i++) {
        if (s_trail[i].x < minx) minx = (int32_t)s_trail[i].x;
        if (s_trail[i].x > maxx) maxx = (int32_t)s_trail[i].x;
        if (s_trail[i].y < miny) miny = (int32_t)s_trail[i].y;
        if (s_trail[i].y > maxy) maxy = (int32_t)s_trail[i].y;
    }

    x = minx - pad;
    y = miny - pad;
    w = maxx - minx + pad * 2;
    h = maxy - miny + pad * 2;

    for (i = 0; i < s_trail_n; i++) {
        s_trail_rel[i].x = s_trail[i].x - x;
        s_trail_rel[i].y = s_trail[i].y - y;
    }

    lv_obj_set_pos(s_trail_obj, x, y);
    lv_obj_set_size(s_trail_obj, w, h);
    lv_line_set_points(s_trail_obj, s_trail_rel, (uint32_t)s_trail_n);
    lv_obj_set_style_line_opa(s_trail_obj, opa, 0);
    lv_obj_remove_flag(s_trail_obj, LV_OBJ_FLAG_HIDDEN);
}

/* ---------------------------- Halves / particles ---------------------------- */
static jr_bit_t *jr_bit_alloc(jr_bit_t *pool, int max)
{
    int i;

    for (i = 0; i < max; i++) {
        if (!pool[i].used) {
            return &pool[i];
        }
    }
    return NULL;
}

static void jr_spawn_piece(float x, float y, float vx, float vy, int32_t r, uint32_t color)
{
    jr_bit_t *p = jr_bit_alloc(s_pieces, JR_PIECE_MAX);

    if (p == NULL) {
        return;
    }
    p->used = true;
    p->x = x;
    p->y = y;
    p->vx = vx;
    p->vy = vy;
    p->r = r;
    p->ttl = 700;
    p->ttl_max = 700;

    lv_obj_set_size(p->obj, r * 2, r * 2);
    lv_obj_set_style_bg_color(p->obj, jr_hex(color), 0);
    lv_obj_set_style_bg_grad_color(p->obj, jr_hex(color), 0);
    lv_obj_set_style_border_color(p->obj, jr_hex(color), 0);
    lv_obj_set_style_opa(p->obj, LV_OPA_COVER, 0);
    lv_obj_set_pos(p->obj, (int32_t)(x - r), (int32_t)(y - r));
    lv_obj_remove_flag(p->obj, LV_OBJ_FLAG_HIDDEN);
}

static void jr_spawn_particles(float x, float y, uint32_t color, int count)
{
    int i;

    for (i = 0; i < count; i++) {
        jr_bit_t *p = jr_bit_alloc(s_particles, JR_PARTICLE_MAX);
        int32_t r;

        if (p == NULL) {
            return;
        }
        r = (int32_t)lv_rand(3, 7);
        p->used = true;
        p->x = x + (float)lv_rand(0, 20) - 10.0f;
        p->y = y + (float)lv_rand(0, 20) - 10.0f;
        p->vx = (float)lv_rand(0, 300) - 150.0f;
        p->vy = (float)lv_rand(0, 230) - 340.0f;
        p->r = r;
        p->ttl = lv_rand(420, 680);
        p->ttl_max = p->ttl;

        lv_obj_set_size(p->obj, r * 2, r * 2);
        lv_obj_set_style_bg_color(p->obj, jr_hex(color), 0);
        lv_obj_set_style_bg_grad_color(p->obj, jr_hex(color), 0);
        lv_obj_set_style_bg_opa(p->obj, LV_OPA_COVER, 0);
        lv_obj_set_style_opa(p->obj, LV_OPA_COVER, 0);
        lv_obj_set_pos(p->obj, (int32_t)(p->x - r), (int32_t)(p->y - r));
        lv_obj_remove_flag(p->obj, LV_OBJ_FLAG_HIDDEN);
    }
}

/* ---------------------------- Cut effects ---------------------------- */
/* Shockwave: a thin ring expands from the pulp and fades out. */
static void jr_spawn_ring(float x, float y, int32_t r_start, int32_t r_end, uint32_t color)
{
    int i;

    for (i = 0; i < JR_RING_MAX; i++) {
        jr_ring_t *p = &s_rings[i];

        if (p->used) {
            continue;
        }
        p->used = true;
        p->x = x;
        p->y = y;
        p->r_start = r_start;
        p->r_end = r_end;
        p->ttl = 260;
        p->ttl_max = 260;

        lv_obj_set_size(p->obj, r_start * 2, r_start * 2);
        lv_obj_set_pos(p->obj, (int32_t)(x - (float)r_start), (int32_t)(y - (float)r_start));
        lv_obj_set_style_border_width(p->obj, 6, 0);
        lv_obj_set_style_border_color(p->obj, jr_hex(color), 0);
        lv_obj_set_style_opa(p->obj, 230, 0);
        lv_obj_remove_flag(p->obj, LV_OBJ_FLAG_HIDDEN);
        return;
    }
}

static void jr_update_rings(uint32_t dtm)
{
    int i;

    for (i = 0; i < JR_RING_MAX; i++) {
        jr_ring_t *p = &s_rings[i];
        uint32_t k;
        int32_t r;

        if (!p->used) {
            continue;
        }
        if (p->ttl <= dtm) {
            p->used = false;
            lv_obj_add_flag(p->obj, LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        p->ttl -= dtm;

        k = 255u - (255u * p->ttl / p->ttl_max);        /* 0 -> 255 */
        r = p->r_start + (p->r_end - p->r_start) * (int32_t)k / 255;
        lv_obj_set_size(p->obj, r * 2, r * 2);
        lv_obj_set_pos(p->obj, (int32_t)(p->x - (float)r), (int32_t)(p->y - (float)r));
        lv_obj_set_style_border_width(p->obj, 6 - (int32_t)(k * 4u / 255u), 0);
        lv_obj_set_style_opa(p->obj, (lv_opa_t)(230u - 230u * k / 255u), 0);
    }
}

/* Slash mark: a short bright line along the drag direction, then fade. */
static void jr_spawn_slash(float cx, float cy, float dx, float dy, int32_t r, uint32_t color)
{
    float len = (float)r * 1.55f;
    float x1, y1, x2, y2;
    int i;

    jr_norm(&dx, &dy);
    x1 = cx - dx * len;
    y1 = cy - dy * len;
    x2 = cx + dx * len;
    y2 = cy + dy * len;

    for (i = 0; i < JR_SLASH_MAX; i++) {
        jr_slash_t *p = &s_slashes[i];
        const int32_t pad = 10;
        int32_t minx, miny, w, h;

        if (p->used) {
            continue;
        }

        minx = (int32_t)(x1 < x2 ? x1 : x2) - pad;
        miny = (int32_t)(y1 < y2 ? y1 : y2) - pad;
        w = (int32_t)(x1 < x2 ? x2 - x1 : x1 - x2) + pad * 2;
        h = (int32_t)(y1 < y2 ? y2 - y1 : y1 - y2) + pad * 2;

        s_slash_pts[i][0].x = (lv_value_precise_t)((int32_t)x1 - minx);
        s_slash_pts[i][0].y = (lv_value_precise_t)((int32_t)y1 - miny);
        s_slash_pts[i][1].x = (lv_value_precise_t)((int32_t)x2 - minx);
        s_slash_pts[i][1].y = (lv_value_precise_t)((int32_t)y2 - miny);

        p->used = true;
        p->ttl = 150;
        p->ttl_max = 150;

        lv_obj_set_pos(p->obj, minx, miny);
        lv_obj_set_size(p->obj, w, h);
        lv_line_set_points(p->obj, s_slash_pts[i], 2);
        lv_obj_set_style_line_color(p->obj, jr_hex(color), 0);
        lv_obj_set_style_line_opa(p->obj, 255, 0);
        lv_obj_remove_flag(p->obj, LV_OBJ_FLAG_HIDDEN);
        return;
    }
}

static void jr_update_slashes(uint32_t dtm)
{
    int i;

    for (i = 0; i < JR_SLASH_MAX; i++) {
        jr_slash_t *p = &s_slashes[i];

        if (!p->used) {
            continue;
        }
        if (p->ttl <= dtm) {
            p->used = false;
            lv_obj_add_flag(p->obj, LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        p->ttl -= dtm;
        lv_obj_set_style_line_opa(p->obj, (lv_opa_t)(255u * p->ttl / p->ttl_max), 0);
    }
}

/* ---------------------------- Floating score ---------------------------- */
static jr_popup_t *jr_popup_alloc(float x, float y, uint32_t color)
{
    jr_popup_t *p = NULL;
    int i;

    for (i = 0; i < JR_POPUP_MAX; i++) {
        if (!s_popups[i].used) {
            p = &s_popups[i];
            break;
        }
    }
    if (p == NULL) {
        return NULL;
    }

    p->used = true;
    p->x = x;
    p->y = y;
    p->ttl = 780;
    p->ttl_max = 780;

    lv_obj_set_style_text_color(p->lbl, jr_hex(color), 0);
    lv_obj_set_pos(p->lbl, (int32_t)(x - 80), (int32_t)y);
    lv_obj_set_style_opa(p->lbl, LV_OPA_COVER, 0);
    lv_obj_remove_flag(p->lbl, LV_OBJ_FLAG_HIDDEN);
    return p;
}

static void jr_popup_score(float x, float y, int32_t gain, int32_t mult)
{
    jr_popup_t *p = jr_popup_alloc(x, y, mult >= 2 ? 0xFFD54A : 0xFFFFFF);

    if (p == NULL) {
        return;
    }
    if (mult >= 2) {
        lv_label_set_text_fmt(p->lbl, "+%d  x%d", (int)gain, (int)mult);
    } else {
        lv_label_set_text_fmt(p->lbl, "+%d", (int)gain);
    }
}

static void jr_popup_text(float x, float y, const char *txt, uint32_t color)
{
    jr_popup_t *p = jr_popup_alloc(x, y, color);

    if (p == NULL) {
        return;
    }
    lv_label_set_text(p->lbl, txt);
}

/* ---------------------------- Fruit logic ---------------------------- */
static void jr_fruit_hide(jr_fruit_t *f)
{
    f->used = false;
    lv_obj_add_flag(f->ball, LV_OBJ_FLAG_HIDDEN);
}

static void jr_hearts_update(void)
{
    int i;

    for (i = 0; i < 3; i++) {
        lv_obj_set_style_bg_color(s_heart[i],
                                  jr_hex(i < s_lives ? 0xFF5B6E : 0x2A3648), 0);
    }
}

static void jr_lose_life(void)
{
    if (s_lives > 0) {
        s_lives--;
    }
    jr_hearts_update();
    if (s_lives <= 0) {
        jr_game_over();
    }
}

static void jr_fruit_miss(jr_fruit_t *f)
{
    bool bomb = f->bomb;

    jr_fruit_hide(f);
    if (!bomb) {
        jr_lose_life();
    }
}

static void jr_slice_fruit(jr_fruit_t *f, float dx, float dy)
{
    float cx = f->x;
    float cy = f->y;
    int32_t r = f->r;
    bool bomb = f->bomb;
    uint32_t pulp = bomb ? 0xFF7043 : s_kinds[f->kind].pulp;

    jr_norm(&dx, &dy);       /* Unit vector along the blade. */
    jr_fruit_hide(f);

    if (bomb) {
        jr_spawn_ring(cx, cy, r * 4 / 10, r * 30 / 10, 0xFF7A45);
        jr_spawn_slash(cx, cy, dx, dy, r, 0xFFD166);
        jr_spawn_particles(cx, cy, 0xFF7043, 12);
        jr_spawn_particles(cx, cy, 0xFFD166, 6);
        jr_popup_text(cx, cy - (float)r, "BOOM!", 0xFF7A45);
        jr_lose_life();
        return;
    }

    /* Push the halves apart perpendicular to the blade so it looks sliced. */
    {
        float px = -dy;
        float py = dx;
        float off = (float)r * 0.30f;
        float kick = 120.0f;

        jr_spawn_piece(cx + px * off, cy + py * off,
                       f->vx + px * kick, f->vy + py * kick - 110.0f, r * 62 / 100, pulp);
        jr_spawn_piece(cx - px * off, cy - py * off,
                       f->vx - px * kick, f->vy - py * kick - 18.0f, r * 62 / 100, pulp);
    }
    jr_spawn_particles(cx, cy, pulp, 9);
    jr_spawn_ring(cx, cy, r * 5 / 10, r * 22 / 10, pulp);
    jr_spawn_slash(cx, cy, dx, dy, r, 0xFFF4D6);

    if (lv_tick_elaps(s_last_slice_tick) < JR_COMBO_WINDOW_MS) {
        s_combo++;
    } else {
        s_combo = 1;
    }
    s_last_slice_tick = lv_tick_get();
    if (s_combo > s_max_combo) {
        s_max_combo = s_combo;
    }

    {
        int32_t mult = s_combo > 5 ? 5 : s_combo;
        int32_t gain = 10 * mult;

        s_score += gain;
        jr_popup_score(cx, cy - (float)r, gain, mult);
    }
}

/* Segment-circle intersection test. */
static bool jr_seg_hits_circle(float x1, float y1, float x2, float y2,
                               float cx, float cy, float r)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    float fx = cx - x1;
    float fy = cy - y1;
    float len2 = dx * dx + dy * dy;
    float t = 0.0f;
    float px, py;

    if (len2 > 0.0001f) {
        t = (fx * dx + fy * dy) / len2;
        if (t < 0.0f) t = 0.0f;
        else if (t > 1.0f) t = 1.0f;
    }
    px = x1 + dx * t - cx;
    py = y1 + dy * t - cy;
    return (px * px + py * py) <= (r * r);
}

static void jr_check_slice(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    int i;

    for (i = 0; i < JR_FRUIT_MAX; i++) {
        jr_fruit_t *f = &s_fruits[i];

        if (!f->used) {
            continue;
        }
        if (jr_seg_hits_circle((float)x1, (float)y1, (float)x2, (float)y2,
                               f->x, f->y, (float)f->r + 10.0f)) {
            jr_slice_fruit(f, (float)(x2 - x1), (float)(y2 - y1));
        }
    }
}

/* ---------------------------- Spawning ---------------------------- */
static void jr_spawn_fruit(bool bomb)
{
    jr_fruit_t *f = NULL;
    int32_t r;
    float flight;
    float target_x;
    int i;

    for (i = 0; i < JR_FRUIT_MAX; i++) {
        if (!s_fruits[i].used) {
            f = &s_fruits[i];
            break;
        }
    }
    if (f == NULL) {
        return;
    }

    r = (int32_t)lv_rand(28, 36);
    jr_fruit_setup(f, (uint8_t)lv_rand(0, JR_KIND_NUM - 1), bomb, r);

    f->x = (float)lv_rand(52, s_w - 52);
    f->y = (float)(s_h + r + 24);

    /* Pick a random apex, then derive the launch speed from it. The lower apex
     * bound keeps at least ~1.7 s of interaction time. */
    {
        int32_t apex_y = (int32_t)lv_rand((uint32_t)(s_h * JR_APEX_TOP_PCT / 100),
                                          (uint32_t)(s_h * JR_APEX_BOTTOM_PCT / 100));
        float rise = f->y - (float)apex_y;

        if (rise < JR_APEX_MIN_RISE) {
            rise = JR_APEX_MIN_RISE;
        }
        f->vy = -(float)lv_sqrt32((uint32_t)(2.0f * JR_GRAVITY * rise));
    }
    target_x = (float)lv_rand(80, s_w - 80);
    flight = -2.0f * f->vy / JR_GRAVITY;      /* Time to fall back to the launch point. */
    f->vx = (target_x - f->x) / flight;
    f->used = true;

    lv_obj_set_pos(f->ball, (int32_t)(f->x - r), (int32_t)(f->y - r));
    lv_obj_remove_flag(f->ball, LV_OBJ_FLAG_HIDDEN);
}

/* ---------------------------- Touch ---------------------------- */
static void jr_touch_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev = lv_indev_active();
    lv_point_t p;

    if (s_state != JR_STATE_PLAY || indev == NULL) {
        return;
    }
    lv_indev_get_point(indev, &p);

    if (code == LV_EVENT_PRESSED) {
        s_drag = true;
        s_prev_pt = p;
        s_trail_n = 0;
        s_trail_opa = JR_TRAIL_OPA;
        jr_trail_push(p.x, p.y);
        return;
    }

    if (code == LV_EVENT_PRESSING) {
        float dx, dy;

        if (!s_drag) {
            return;
        }
        jr_trail_push(p.x, p.y);
        s_trail_opa = JR_TRAIL_OPA;
        jr_trail_refresh(s_trail_opa);

        dx = (float)(p.x - s_prev_pt.x);
        dy = (float)(p.y - s_prev_pt.y);
        /* Require a deliberate drag, otherwise a resting finger would slice fruit. */
        if (dx * dx + dy * dy >= 100.0f) {
            jr_check_slice(s_prev_pt.x, s_prev_pt.y, p.x, p.y);
            s_prev_pt = p;
        }
        return;
    }

    if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        if (s_drag && code == LV_EVENT_RELEASED) {
            float dx = (float)(p.x - s_prev_pt.x);
            float dy = (float)(p.y - s_prev_pt.y);

            /* A fast flick may never emit PRESSING, so also check on release. */
            if (dx * dx + dy * dy >= 100.0f) {
                jr_check_slice(s_prev_pt.x, s_prev_pt.y, p.x, p.y);
            }
        }
        s_drag = false;
        return;
    }
}

/* ---------------------------- Per-frame update ---------------------------- */
static void jr_update_fruits(float dt)
{
    int i;

    for (i = 0; i < JR_FRUIT_MAX; i++) {
        jr_fruit_t *f = &s_fruits[i];

        if (!f->used) {
            continue;
        }

        f->vy += JR_GRAVITY * dt;
        f->x += f->vx * dt;
        f->y += f->vy * dt;

        if (f->x < (float)f->r) {
            f->x = (float)f->r;
            f->vx = -f->vx * 0.55f;
        } else if (f->x > (float)(s_w - f->r)) {
            f->x = (float)(s_w - f->r);
            f->vx = -f->vx * 0.55f;
        }

        if (f->vy > 0.0f && f->y - (float)f->r > (float)s_h) {
            jr_fruit_miss(f);
            if (s_state != JR_STATE_PLAY) {
                return;
            }
            continue;
        }
        lv_obj_set_pos(f->ball, (int32_t)(f->x - (float)f->r),
                       (int32_t)(f->y - (float)f->r));
    }
}

static void jr_update_bits(jr_bit_t *pool, int max, float dt, uint32_t dtm, bool fade)
{
    int i;

    for (i = 0; i < max; i++) {
        jr_bit_t *p = &pool[i];

        if (!p->used) {
            continue;
        }

        if (p->ttl <= dtm) {
            p->used = false;
            lv_obj_add_flag(p->obj, LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        p->ttl -= dtm;

        p->vy += JR_GRAVITY * dt;
        p->x += p->vx * dt;
        p->y += p->vy * dt;
        lv_obj_set_pos(p->obj, (int32_t)(p->x - (float)p->r),
                       (int32_t)(p->y - (float)p->r));

        if (fade) {
            uint32_t o = 255u * p->ttl / p->ttl_max;

            if (o < 12u) {
                o = 12u;
            }
            lv_obj_set_style_opa(p->obj, (lv_opa_t)o, 0);
        }
    }
}

static void jr_update_popups(uint32_t dtm)
{
    int i;

    for (i = 0; i < JR_POPUP_MAX; i++) {
        jr_popup_t *p = &s_popups[i];
        uint32_t k;
        uint32_t o;

        if (!p->used) {
            continue;
        }
        if (p->ttl <= dtm) {
            p->used = false;
            lv_obj_add_flag(p->lbl, LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        p->ttl -= dtm;

        k = 255u - (255u * p->ttl / p->ttl_max);          /* 0 -> 255 */
        lv_obj_set_pos(p->lbl, (int32_t)(p->x - 80.0f),
                       (int32_t)(p->y - (float)k * 0.35f));
        o = 255u * p->ttl / p->ttl_max;
        lv_obj_set_style_opa(p->lbl, (lv_opa_t)o, 0);
    }
}

static void jr_update_trail(uint32_t dtm)
{
    int32_t dec;

    if (s_drag) {
        return;
    }
    if (s_trail_opa == 0) {
        return;
    }

    dec = (int32_t)(dtm * 2);
    if ((int32_t)s_trail_opa <= dec) {
        s_trail_opa = 0;
        jr_trail_clear();
        return;
    }
    s_trail_opa = (lv_opa_t)((int32_t)s_trail_opa - dec);
    jr_trail_refresh(s_trail_opa);
}

static void jr_hud_update(bool force)
{
    uint32_t elapsed;
    uint32_t remain;
    int32_t sec;
    int32_t pct;

    if (s_state != JR_STATE_PLAY) {
        return;
    }

    if (force || s_score != s_last_score_ui) {
        lv_label_set_text_fmt(s_lbl_score, "%d", (int)s_score);
        s_last_score_ui = s_score;
    }

    elapsed = lv_tick_elaps(s_start_tick);
    remain = (elapsed >= JR_GAME_MS) ? 0 : (JR_GAME_MS - elapsed);
    sec = (int32_t)((remain + 999u) / 1000u);
    pct = (int32_t)(remain / (JR_GAME_MS / 100));

    if (force || pct != s_last_pct_ui) {
        lv_bar_set_value(s_bar_time, pct, LV_ANIM_OFF);
        s_last_pct_ui = pct;
    }

    if (force || sec != s_last_sec_ui) {
        lv_label_set_text_fmt(s_lbl_time, "%d:%02d", (int)(sec / 60), (int)(sec % 60));
        if (sec > 20) {
            lv_obj_set_style_bg_color(s_bar_time, jr_hex(0x1B9C52), LV_PART_INDICATOR);
            lv_obj_set_style_bg_grad_color(s_bar_time, jr_hex(0x6EF2A0), LV_PART_INDICATOR);
            lv_obj_set_style_text_color(s_lbl_time, lv_color_white(), 0);
        } else if (sec > 8) {
            lv_obj_set_style_bg_color(s_bar_time, jr_hex(0xE08A12), LV_PART_INDICATOR);
            lv_obj_set_style_bg_grad_color(s_bar_time, jr_hex(0xFFD166), LV_PART_INDICATOR);
            lv_obj_set_style_text_color(s_lbl_time, jr_hex(0xFFC145), 0);
        } else {
            lv_obj_set_style_bg_color(s_bar_time, jr_hex(0xC21F35), LV_PART_INDICATOR);
            lv_obj_set_style_bg_grad_color(s_bar_time, jr_hex(0xFF7A7A), LV_PART_INDICATOR);
            lv_obj_set_style_text_color(s_lbl_time, jr_hex(0xFF5B6E), 0);
        }
        s_last_sec_ui = sec;
    }

    /* Hide the combo hint once its window expires. */
    {
        int32_t combo_show = 0;

        if (s_combo >= 2 && lv_tick_elaps(s_last_slice_tick) < 1500u) {
            combo_show = s_combo;
        }
        if (force || combo_show != s_last_combo_ui) {
            if (combo_show >= 2) {
                lv_label_set_text_fmt(s_lbl_combo, "COMBO  x%d", (int)combo_show);
                lv_obj_set_style_text_color(s_lbl_combo,
                                            jr_hex(combo_show >= 5 ? 0xFFD54A :
                                                   (combo_show >= 3 ? 0x6FE39A : 0x9FC3E8)), 0);
                lv_obj_remove_flag(s_lbl_combo, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(s_lbl_combo, LV_OBJ_FLAG_HIDDEN);
            }
            s_last_combo_ui = combo_show;
        }
    }
}

static void jr_update_play(uint32_t now, float dt)
{
    uint32_t elapsed = lv_tick_elaps(s_start_tick);
    uint32_t sec;
    uint32_t interval;
    int groups, i, bomb_idx;
    int bomb_pct;

    if (elapsed >= JR_GAME_MS) {
        jr_game_over();
        return;
    }

    sec = elapsed / 1000u;
    /* No bombs in the first 5 s; afterwards the chance ramps up quickly. */
    bomb_pct = 18 + (int)sec / 2;
    if (bomb_pct > 48) {
        bomb_pct = 48;
    }
    groups = 1;
    if (sec > 18 && lv_rand(0, 99) < 35) {
        groups = 2;
    }
    if (sec > 38 && lv_rand(0, 99) < 30) {
        groups = 3;
    }

    if (now >= s_next_spawn_tick) {
        bomb_idx = -1;
        if (sec >= 5 && lv_rand(0, 99) < (uint32_t)bomb_pct) {
            bomb_idx = (int)lv_rand(0, (uint32_t)(groups - 1));
        }
        for (i = 0; i < groups; i++) {
            jr_spawn_fruit(i == bomb_idx);
        }
        interval = 1150u - sec * 11u;
        if (interval < 560u) {
            interval = 560u;
        }
        s_next_spawn_tick = now + interval + lv_rand(0, 200);
    }

    jr_update_fruits(dt);
    if (s_state != JR_STATE_PLAY) {
        return;
    }
    jr_hud_update(false);
}

static void jr_tick(lv_timer_t *timer)
{
    uint32_t now = lv_tick_get();
    uint32_t dtm = lv_tick_elaps(s_last_tick);
    float dt;

    (void)timer;
    if (dtm == 0) {
        return;
    }
    if (dtm > 120u) {
        dtm = 120u;      /* Clamp long frames so objects cannot tunnel across the screen. */
    }
    s_last_tick = now;
    dt = (float)dtm / 1000.0f;

    if (s_state == JR_STATE_PLAY) {
        jr_update_play(now, dt);
    }
    jr_update_bits(s_pieces, JR_PIECE_MAX, dt, dtm, true);
    jr_update_bits(s_particles, JR_PARTICLE_MAX, dt, dtm, true);
    jr_update_rings(dtm);
    jr_update_slashes(dtm);
    jr_update_popups(dtm);
    jr_update_trail(dtm);
}

/* ---------------------------- Scenes ---------------------------- */
static void jr_hide_all(void)
{
    int i;

    for (i = 0; i < JR_FRUIT_MAX; i++) {
        if (s_fruits[i].used) {
            jr_fruit_hide(&s_fruits[i]);
        }
    }
    for (i = 0; i < JR_PIECE_MAX; i++) {
        s_pieces[i].used = false;
        lv_obj_add_flag(s_pieces[i].obj, LV_OBJ_FLAG_HIDDEN);
    }
    for (i = 0; i < JR_PARTICLE_MAX; i++) {
        s_particles[i].used = false;
        lv_obj_add_flag(s_particles[i].obj, LV_OBJ_FLAG_HIDDEN);
    }
    for (i = 0; i < JR_POPUP_MAX; i++) {
        s_popups[i].used = false;
        lv_obj_add_flag(s_popups[i].lbl, LV_OBJ_FLAG_HIDDEN);
    }
    for (i = 0; i < JR_RING_MAX; i++) {
        s_rings[i].used = false;
        lv_obj_add_flag(s_rings[i].obj, LV_OBJ_FLAG_HIDDEN);
    }
    for (i = 0; i < JR_SLASH_MAX; i++) {
        s_slashes[i].used = false;
        lv_obj_add_flag(s_slashes[i].obj, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_lbl_combo) {
        lv_obj_add_flag(s_lbl_combo, LV_OBJ_FLAG_HIDDEN);
    }
    s_last_combo_ui = 0;
    jr_trail_clear();
    s_drag = false;
}

static void jr_overlay_clear(void)
{
    if (s_overlay) {
        lv_obj_delete(s_overlay);
        s_overlay = NULL;
    }
}

static lv_obj_t *jr_overlay_new(void)
{
    lv_obj_t *o;

    jr_overlay_clear();
    o = lv_obj_create(s_ui);
    lv_obj_remove_style_all(o);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(o, s_w, s_h);
    lv_obj_set_pos(o, 0, 0);
    s_overlay = o;
    return o;
}

static void jr_btn_start_cb(lv_event_t *e)
{
    (void)e;
    jr_start_game();
}

/* Return to the app carousel, matching the power-on screen. */
static void jr_btn_home_cb(lv_event_t *e)
{
    (void)e;
    juice_rush_stop();
    launcher_start();
}

static void jr_show_start(void)
{
    lv_obj_t *ov;
    lv_obj_t *lbl;
    int i;
    int32_t base_x;

    s_state = JR_STATE_START;
    jr_hide_all();
    lv_obj_add_flag(s_hud, LV_OBJ_FLAG_HIDDEN);

    ov = jr_overlay_new();

    /* Dim the empty playfield behind the start page. */
    {
        lv_obj_t *scrim = lv_obj_create(ov);

        lv_obj_remove_style_all(scrim);
        lv_obj_remove_flag(scrim, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(scrim, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(scrim, s_w, s_h);
        lv_obj_set_pos(scrim, 0, 0);
        lv_obj_set_style_bg_opa(scrim, 90, 0);
        lv_obj_set_style_bg_color(scrim, jr_hex(0x05080E), 0);
    }

    /* Decorative halo behind the title. */
    {
        lv_obj_t *halo = lv_obj_create(ov);

        lv_obj_remove_style_all(halo);
        lv_obj_remove_flag(halo, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(halo, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(halo, 220, 220);
        lv_obj_align(halo, LV_ALIGN_TOP_MID, 0, 20);
        lv_obj_set_style_radius(halo, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(halo, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(halo, 2, 0);
        lv_obj_set_style_border_color(halo, jr_hex(0x2C5388), 0);
        lv_obj_set_style_border_opa(halo, 110, 0);
    }

    /* Mode label. */
    {
        lv_obj_t *chip = lv_obj_create(ov);

        lv_obj_remove_style_all(chip);
        lv_obj_remove_flag(chip, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(chip, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(chip, 170, 32);
        lv_obj_align(chip, LV_ALIGN_TOP_MID, 0, 54);
        lv_obj_set_style_radius(chip, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(chip, 180, 0);
        lv_obj_set_style_bg_color(chip, jr_hex(0x13253D), 0);
        lv_obj_set_style_border_width(chip, 2, 0);
        lv_obj_set_style_border_color(chip, jr_hex(0x2F5A8E), 0);

        lbl = lv_label_create(chip);
        lv_label_set_text(lbl, "CHALLENGE MODE");
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(lbl, jr_hex(0x7FC8FF), 0);
        lv_obj_set_style_text_letter_space(lbl, 1, 0);
        lv_obj_center(lbl);
    }

    lbl = lv_label_create(ov);
    lv_label_set_text(lbl, "JUICE RUSH");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_set_style_text_letter_space(lbl, 6, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 104);

    lbl = lv_label_create(ov);
    lv_label_set_text(lbl, "SLIDE  TO  SLICE");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl, jr_hex(JR_COLOR_DIM), 0);
    lv_obj_set_style_text_letter_space(lbl, 3, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 146);

    /* Gradient divider under the subtitle. */
    {
        lv_obj_t *bar = lv_obj_create(ov);

        lv_obj_remove_style_all(bar);
        lv_obj_remove_flag(bar, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(bar, 200, 3);
        lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 180);
        lv_obj_set_style_radius(bar, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(bar, jr_hex(0x1D3B5E), 0);
        lv_obj_set_style_bg_grad_color(bar, jr_hex(0x3ED47A), 0);
        lv_obj_set_style_bg_grad_dir(bar, LV_GRAD_DIR_HOR, 0);
    }

    /* Decorative fruit row, slightly arched. */
    {
        static const int32_t deco_dy[5] = {-8, -3, 0, -3, -8};

        base_x = s_w / 2 - 96;
        for (i = 0; i < 5; i++) {
            jr_deco_fruit(ov, base_x + i * 48, 214 + deco_dy[i], 18, (uint8_t)i);
        }
    }

    lbl = lv_label_create(ov);
    lv_label_set_text(lbl, "MISS A FRUIT  -1 LIFE      BOMB  -1 LIFE");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl, jr_hex(0x5D728C), 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 254);

    lbl = lv_label_create(ov);
    lv_label_set_text_fmt(lbl, "BEST  %d", (int)s_best);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl, jr_hex(0xFFD54A), 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 278);

    {
        lv_obj_t *btn = jr_button_create(ov, "START", jr_btn_start_cb, 220, 50);

        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 304);
    }

    /* Secondary button: back to the carousel. */
    {
        lv_obj_t *btn = jr_button_create(ov, "HOME", jr_btn_home_cb, 220, 42);

        lv_obj_set_style_bg_color(btn, jr_hex(0x24394F), 0);
        lv_obj_set_style_bg_grad_color(btn, jr_hex(0x16263C), 0);
        lv_obj_set_style_shadow_color(btn, jr_hex(0x0A1420), 0);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 366);
    }
}

static void jr_build_over(void)
{
    lv_obj_t *ov = jr_overlay_new();
    lv_obj_t *card;
    lv_obj_t *lbl;

    /* Dark overlay behind the result dialog. */
    {
        lv_obj_t *scrim = lv_obj_create(ov);

        lv_obj_remove_style_all(scrim);
        lv_obj_remove_flag(scrim, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(scrim, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(scrim, s_w, s_h);
        lv_obj_set_pos(scrim, 0, 0);
        lv_obj_set_style_bg_opa(scrim, 170, 0);
        lv_obj_set_style_bg_color(scrim, jr_hex(0x05080E), 0);
    }

    card = lv_obj_create(ov);

    lv_obj_remove_style_all(card);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(card, 380, 320);
    lv_obj_center(card);
    lv_obj_set_style_radius(card, 24, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(card, jr_hex(JR_COLOR_CARD), 0);
    lv_obj_set_style_bg_grad_color(card, jr_hex(JR_COLOR_CARD2), 0);
    lv_obj_set_style_bg_grad_dir(card, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_border_color(card, jr_hex(JR_COLOR_LINE), 0);

    lbl = lv_label_create(card);
    lv_label_set_text(lbl, "GAME OVER");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_set_style_text_letter_space(lbl, 3, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 18);

    lbl = lv_label_create(card);
    lv_obj_remove_style_all(lbl);
    lv_obj_set_size(lbl, 90, 3);
    lv_obj_set_style_radius(lbl, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(lbl, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(lbl, jr_hex(0x38D07A), 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 52);

    lbl = lv_label_create(card);
    lv_label_set_text(lbl, "SCORE");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl, jr_hex(JR_COLOR_DIM), 0);
    lv_obj_set_style_text_letter_space(lbl, 4, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 70);

    lbl = lv_label_create(card);
    lv_label_set_text_fmt(lbl, "%d", (int)s_score);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 90);

    lbl = lv_label_create(card);
    lv_label_set_text_fmt(lbl, "BEST  %d", (int)s_best);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl, jr_hex(0xFFD54A), 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 132);

    lbl = lv_label_create(card);
    lv_label_set_text_fmt(lbl, "MAX COMBO  x%d", (int)(s_max_combo < 1 ? 1 : s_max_combo));
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl, jr_hex(0x7FB8E8), 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 158);

    /* Highlight badge when a new record is set. */
    if (s_new_record) {
        lv_obj_t *badge = lv_obj_create(card);

        lv_obj_remove_style_all(badge);
        lv_obj_remove_flag(badge, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(badge, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(badge, 170, 32);
        lv_obj_align(badge, LV_ALIGN_TOP_MID, 0, 186);
        lv_obj_set_style_radius(badge, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(badge, 200, 0);
        lv_obj_set_style_bg_color(badge, jr_hex(0x3A2A08), 0);
        lv_obj_set_style_border_width(badge, 2, 0);
        lv_obj_set_style_border_color(badge, jr_hex(0xFFD54A), 0);

        lbl = lv_label_create(badge);
        lv_label_set_text(lbl, "NEW RECORD");
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(lbl, jr_hex(0xFFD54A), 0);
        lv_obj_set_style_text_letter_space(lbl, 2, 0);
        lv_obj_center(lbl);
    }

    {
        lv_obj_t *btn = jr_button_create(card, "PLAY  AGAIN", jr_btn_start_cb, 160, 42);

        lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 18, -16);
    }

    {
        /* Secondary button: dark fill, lower visual weight. */
        lv_obj_t *btn = jr_button_create(card, "HOME", jr_btn_home_cb, 160, 42);

        lv_obj_set_style_bg_color(btn, jr_hex(0x24394F), 0);
        lv_obj_set_style_bg_grad_color(btn, jr_hex(0x16263C), 0);
        lv_obj_set_style_shadow_color(btn, jr_hex(0x0A1420), 0);
        lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, -18, -16);
    }
}

static void jr_game_over(void)
{
    if (s_state != JR_STATE_PLAY) {
        return;
    }
    s_state = JR_STATE_OVER;
    s_new_record = (s_score > s_best);
    if (s_new_record) {
        s_best = s_score;
    }
    jr_hide_all();
    lv_obj_add_flag(s_hud, LV_OBJ_FLAG_HIDDEN);
    jr_build_over();
}

static void jr_start_game(void)
{
    jr_overlay_clear();
    jr_hide_all();

    s_score = 0;
    s_combo = 0;
    s_max_combo = 0;
    s_new_record = false;
    s_lives = 3;
    s_last_score_ui = -1;
    s_last_sec_ui = -1;
    s_last_pct_ui = -1;
    s_last_combo_ui = -1;
    s_start_tick = lv_tick_get();
    s_last_tick = s_start_tick;
    s_next_spawn_tick = s_start_tick + 600;
    s_last_slice_tick = 0;
    s_state = JR_STATE_PLAY;

    jr_hearts_update();
    jr_hud_update(true);
    lv_obj_remove_flag(s_hud, LV_OBJ_FLAG_HIDDEN);
}

/* ---------------------------- Build ---------------------------- */
static void jr_build_hud(void)
{
    lv_obj_t *lbl;
    lv_obj_t *pill;
    int i;

    s_hud = lv_obj_create(s_ui);
    lv_obj_remove_style_all(s_hud);
    lv_obj_remove_flag(s_hud, LV_OBJ_FLAG_SCROLLABLE);
    /* Not clickable, so the top area can still be swiped. */
    lv_obj_remove_flag(s_hud, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(s_hud, s_w, JR_HUD_H);
    lv_obj_set_pos(s_hud, 0, 0);

    /* Score pill background. */
    pill = lv_obj_create(s_hud);
    lv_obj_remove_style_all(pill);
    lv_obj_remove_flag(pill, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(pill, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(pill, 108, 54);
    lv_obj_set_pos(pill, 12, 6);
    lv_obj_set_style_radius(pill, 16, 0);
    lv_obj_set_style_bg_opa(pill, 170, 0);
    lv_obj_set_style_bg_color(pill, jr_hex(0x101E33), 0);
    lv_obj_set_style_bg_grad_color(pill, jr_hex(0x0A1423), 0);
    lv_obj_set_style_bg_grad_dir(pill, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_width(pill, 2, 0);
    lv_obj_set_style_border_color(pill, jr_hex(0x25415F), 0);

    lbl = lv_label_create(pill);
    lv_label_set_text(lbl, "SCORE");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl, jr_hex(JR_COLOR_DIM), 0);
    lv_obj_set_style_text_letter_space(lbl, 2, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 3);

    s_lbl_score = lv_label_create(pill);
    lv_label_set_text(s_lbl_score, "0");
    lv_obj_set_style_text_font(s_lbl_score, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(s_lbl_score, jr_hex(0xFFE08A), 0);
    lv_obj_align(s_lbl_score, LV_ALIGN_BOTTOM_MID, 0, -2);

    /* Lives pill: the width derives from three hearts, gaps and padding, and
     * the hearts use the same formula so they stay centered in the pill. */
    pill = lv_obj_create(s_hud);
    lv_obj_remove_style_all(pill);
    lv_obj_remove_flag(pill, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(pill, LV_OBJ_FLAG_SCROLLABLE);
    {
        const int32_t hd = 18;      /* Heart diameter. */
        const int32_t hg = 8;       /* Heart spacing. */
        const int32_t pad = 14;     /* Horizontal padding. */
        const int32_t pw = 3 * hd + 2 * hg + pad * 2;   /* 98 */
        const int32_t ph = 54;
        const int32_t px = s_w - 12 - pw;
        const int32_t hx0 = px + pad;
        const int32_t hy = 6 + (ph - hd) / 2;

        lv_obj_set_size(pill, pw, ph);
        lv_obj_set_pos(pill, px, 6);
        lv_obj_set_style_radius(pill, 16, 0);
        lv_obj_set_style_bg_opa(pill, 170, 0);
        lv_obj_set_style_bg_color(pill, jr_hex(0x101E33), 0);
        lv_obj_set_style_border_width(pill, 2, 0);
        lv_obj_set_style_border_color(pill, jr_hex(0x25415F), 0);

        for (i = 0; i < 3; i++) {
            s_heart[i] = jr_ball_create(s_hud);
            lv_obj_remove_flag(s_heart[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_size(s_heart[i], hd, hd);
            lv_obj_set_style_border_width(s_heart[i], 0, 0);
            lv_obj_set_pos(s_heart[i], hx0 + i * (hd + hg), hy);
        }
    }
    jr_hearts_update();

    /* Timer pill, centered and aligned with the two side pills. */
    pill = lv_obj_create(s_hud);
    lv_obj_remove_style_all(pill);
    lv_obj_remove_flag(pill, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(pill, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(pill, 100, 54);
    lv_obj_set_pos(pill, (s_w - 100) / 2, 6);
    lv_obj_set_style_radius(pill, 16, 0);
    lv_obj_set_style_bg_opa(pill, 170, 0);
    lv_obj_set_style_bg_color(pill, jr_hex(0x101E33), 0);
    lv_obj_set_style_bg_grad_color(pill, jr_hex(0x0A1423), 0);
    lv_obj_set_style_bg_grad_dir(pill, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_width(pill, 2, 0);
    lv_obj_set_style_border_color(pill, jr_hex(0x25415F), 0);

    lbl = lv_label_create(pill);
    lv_label_set_text(lbl, "TIME");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl, jr_hex(JR_COLOR_DIM), 0);
    lv_obj_set_style_text_letter_space(lbl, 2, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 3);

    s_lbl_time = lv_label_create(pill);
    lv_label_set_text(s_lbl_time, "1:00");
    lv_obj_set_style_text_font(s_lbl_time, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_lbl_time, lv_color_white(), 0);
    lv_obj_align(s_lbl_time, LV_ALIGN_BOTTOM_MID, 0, -4);

    /* Time bar between the pills and the playfield frame. The indicator
     * gradient shifts from green to yellow to red as time runs out. */
    s_bar_time = lv_bar_create(s_hud);
    lv_obj_set_size(s_bar_time, s_w - 24, 6);
    lv_obj_set_pos(s_bar_time, 12, 68);
    lv_obj_set_style_radius(s_bar_time, 3, 0);
    lv_obj_set_style_bg_opa(s_bar_time, 220, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_bar_time, jr_hex(0x16263C), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_bar_time, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_bar_time, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(s_bar_time, 3, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(s_bar_time, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(s_bar_time, jr_hex(0x1B9C52), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_color(s_bar_time, jr_hex(0x6EF2A0), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_dir(s_bar_time, LV_GRAD_DIR_HOR, LV_PART_INDICATOR);
    lv_bar_set_range(s_bar_time, 0, 100);
    lv_bar_set_value(s_bar_time, 100, LV_ANIM_OFF);

    /* Combo hint, shown briefly after a chain. Placed at the top of the
     * playfield because the HUD panel has no vertical room left. */
    s_lbl_combo = lv_label_create(s_ui);
    lv_obj_set_width(s_lbl_combo, s_w);
    lv_obj_set_style_text_align(s_lbl_combo, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(s_lbl_combo, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_lbl_combo, jr_hex(0x9FC3E8), 0);
    lv_obj_set_style_text_letter_space(s_lbl_combo, 2, 0);
    lv_obj_set_pos(s_lbl_combo, 0, 104);
    lv_obj_add_flag(s_lbl_combo, LV_OBJ_FLAG_HIDDEN);

    lv_obj_add_flag(s_hud, LV_OBJ_FLAG_HIDDEN);
}

static void jr_build_pools(void)
{
    int i;

    for (i = 0; i < JR_FRUIT_MAX; i++) {
        memset(&s_fruits[i], 0, sizeof(s_fruits[i]));
        jr_ball_pair_create(s_play, &s_fruits[i]);
    }

    for (i = 0; i < JR_PIECE_MAX; i++) {
        memset(&s_pieces[i], 0, sizeof(s_pieces[i]));
        s_pieces[i].obj = jr_ball_create(s_play);
        lv_obj_set_style_border_width(s_pieces[i].obj, 0, 0);
        lv_obj_add_flag(s_pieces[i].obj, LV_OBJ_FLAG_HIDDEN);
    }

    for (i = 0; i < JR_PARTICLE_MAX; i++) {
        memset(&s_particles[i], 0, sizeof(s_particles[i]));
        s_particles[i].obj = jr_ball_create(s_play);
        lv_obj_set_style_border_width(s_particles[i].obj, 0, 0);
        lv_obj_add_flag(s_particles[i].obj, LV_OBJ_FLAG_HIDDEN);
    }

    for (i = 0; i < JR_POPUP_MAX; i++) {
        memset(&s_popups[i], 0, sizeof(s_popups[i]));
        s_popups[i].lbl = lv_label_create(s_play);
        lv_obj_set_width(s_popups[i].lbl, 160);
        lv_obj_set_style_text_align(s_popups[i].lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(s_popups[i].lbl, &lv_font_montserrat_20, 0);
        lv_obj_add_flag(s_popups[i].lbl, LV_OBJ_FLAG_HIDDEN);
    }

    /* Shockwave ring: transparent circle with only a border. */
    for (i = 0; i < JR_RING_MAX; i++) {
        memset(&s_rings[i], 0, sizeof(s_rings[i]));
        s_rings[i].obj = lv_obj_create(s_play);
        lv_obj_remove_style_all(s_rings[i].obj);
        lv_obj_remove_flag(s_rings[i].obj, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(s_rings[i].obj, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(s_rings[i].obj, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(s_rings[i].obj, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(s_rings[i].obj, 6, 0);
        lv_obj_add_flag(s_rings[i].obj, LV_OBJ_FLAG_HIDDEN);
    }

    /* Slash flash: two-point line with static geometry that only fades. */
    for (i = 0; i < JR_SLASH_MAX; i++) {
        memset(&s_slashes[i], 0, sizeof(s_slashes[i]));
        s_slashes[i].obj = lv_line_create(s_play);
        lv_obj_remove_style_all(s_slashes[i].obj);
        lv_obj_remove_flag(s_slashes[i].obj, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_line_width(s_slashes[i].obj, 9, 0);
        lv_obj_set_style_line_rounded(s_slashes[i].obj, true, 0);
        lv_obj_add_flag(s_slashes[i].obj, LV_OBJ_FLAG_HIDDEN);
    }

    s_trail_obj = lv_line_create(s_play);
    lv_obj_remove_style_all(s_trail_obj);
    lv_obj_remove_flag(s_trail_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_line_color(s_trail_obj, lv_color_white(), 0);
    lv_obj_set_style_line_width(s_trail_obj, 12, 0);
    lv_obj_set_style_line_rounded(s_trail_obj, true, 0);
    lv_obj_set_style_line_opa(s_trail_obj, JR_TRAIL_OPA, 0);
    lv_obj_add_flag(s_trail_obj, LV_OBJ_FLAG_HIDDEN);
}

void juice_rush_reset(void)
{
    s_root = NULL;
    s_bg = NULL;
    s_play = NULL;
    s_ui = NULL;
    s_touch = NULL;
    s_overlay = NULL;
    s_hud = NULL;
    s_lbl_score = NULL;
    s_lbl_time = NULL;
    s_lbl_combo = NULL;
    s_bar_time = NULL;
    s_trail_obj = NULL;
    s_timer = NULL;
    memset(s_heart, 0, sizeof(s_heart));
    memset(s_fruits, 0, sizeof(s_fruits));
    memset(s_pieces, 0, sizeof(s_pieces));
    memset(s_particles, 0, sizeof(s_particles));
    memset(s_popups, 0, sizeof(s_popups));
    memset(s_rings, 0, sizeof(s_rings));
    memset(s_slashes, 0, sizeof(s_slashes));
    memset(s_slash_pts, 0, sizeof(s_slash_pts));
    s_max_combo = 0;
    s_new_record = false;
    s_last_combo_ui = 0;
    s_trail_n = 0;
    s_trail_opa = 0;
    s_drag = false;
    s_state = JR_STATE_IDLE;
}

void juice_rush_stop(void)
{
    if (s_timer != NULL) {
        lv_timer_delete(s_timer);
        s_timer = NULL;
    }
    if (s_root != NULL) {
        lv_obj_delete(s_root);
    }
    juice_rush_reset();
}

void juice_rush_start(void)
{
    lv_display_t *disp;

    juice_rush_reset();

    disp = lv_display_get_default();
    if (disp == NULL) {
        return;
    }
    s_w = lv_display_get_horizontal_resolution(disp);
    s_h = lv_display_get_vertical_resolution(disp);
    lv_rand_set_seed(lv_tick_get());

    /* The screen itself must not scroll, otherwise drags become page scrolls. */
    lv_obj_remove_flag(lv_screen_active(), LV_OBJ_FLAG_SCROLLABLE);

    s_root = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(s_root);
    lv_obj_remove_flag(s_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(s_root, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(s_root, s_w, s_h);
    lv_obj_set_pos(s_root, 0, 0);
    lv_obj_set_style_bg_opa(s_root, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(s_root, jr_hex(0x172644), 0);
    lv_obj_set_style_bg_grad_color(s_root, jr_hex(0x070B14), 0);
    lv_obj_set_style_bg_grad_dir(s_root, LV_GRAD_DIR_VER, 0);

    jr_build_bg();

    s_play = lv_obj_create(s_root);
    lv_obj_remove_style_all(s_play);
    lv_obj_remove_flag(s_play, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(s_play, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(s_play, s_w, s_h);
    lv_obj_set_pos(s_play, 0, 0);

    s_touch = lv_obj_create(s_root);
    lv_obj_remove_style_all(s_touch);
    lv_obj_remove_flag(s_touch, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_touch, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(s_touch, LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_set_size(s_touch, s_w, s_h);
    lv_obj_set_pos(s_touch, 0, 0);
    lv_obj_set_style_bg_opa(s_touch, LV_OPA_TRANSP, 0);
    lv_obj_add_event_cb(s_touch, jr_touch_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(s_touch, jr_touch_cb, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(s_touch, jr_touch_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(s_touch, jr_touch_cb, LV_EVENT_PRESS_LOST, NULL);

    s_ui = lv_obj_create(s_root);
    lv_obj_remove_style_all(s_ui);
    lv_obj_remove_flag(s_ui, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(s_ui, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(s_ui, s_w, s_h);
    lv_obj_set_pos(s_ui, 0, 0);

    jr_build_pools();
    jr_build_hud();

    s_best = 0;
    s_state = JR_STATE_IDLE;
    s_last_tick = lv_tick_get();

    s_timer = lv_timer_create(jr_tick, JR_TICK_MS, NULL);

    jr_show_start();
}
