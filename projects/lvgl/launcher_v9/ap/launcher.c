/**
 * @file launcher.c
 * @brief Application launcher: horizontal carousel used to pick and open an app.
 *
 * Cards sit in a horizontal scroll container with CENTER snap. The card closest
 * to the screen center drives the title, subtitle and page dots. Tapping the
 * centered card opens the app; tapping a side card only scrolls it to center.
 * All visuals are drawn with plain lv_obj primitives, so no image assets or
 * decoders are needed on this memory-constrained target.
 */
#include "lvgl.h"
#include <stdint.h>
#include <string.h>
#include "launcher.h"
#include "juice_rush.h"
#include "game_2048.h"

/* ---------------------------- Layout ---------------------------- */
#define LC_CARD_W       258         /* Card size for a 480x480 panel. */
#define LC_CARD_H       258
#define LC_CARD_GAP     26
#define LC_CARD_PAD_V   0

#define LC_DOT_SLOT     26          /* Horizontal slot reserved per page dot. */
#define LC_DOT_H        10
#define LC_TOAST_MS     1600        /* "Coming soon" toast duration. */

#define LC_HINT_TEXT    "SWIPE  |  TAP TO OPEN"

#define LC_COLOR_BG_TOP 0x16243C
#define LC_COLOR_BG_BOT 0x060A12
#define LC_COLOR_DIM    0x8FA6C0
#define LC_COLOR_TINY   0x6E88A8

/* ---------------------------- Data ---------------------------- */
typedef struct {
    const char *name;       /* Montserrat only covers ASCII, so names must be English. */
    const char *tag;
    uint32_t color_top;
    uint32_t color_bottom;
    uint32_t accent;        /* Accent color for index, icon and selection border. */
    uint8_t kind;           /* Icon drawing branch. */
} lc_app_t;

/* Card order follows this table; games are kept at the end. */
static const lc_app_t s_apps[] = {
    /* Hues are spread out so no two adjacent cards look alike. */
    {"MUSIC",      "PLAYER",   0x4B3BB0, 0x171132, 0xA897FF, 1},
    {"GALLERY",    "PHOTOS",   0xC26A38, 0x3A1C10, 0xFFC49B, 2},
    {"WEATHER",    "LIVE",     0x2F6FA0, 0x10222F, 0x86DBFF, 3},
    {"SETTINGS",   "SYSTEM",   0x454B57, 0x14161B, 0xC6CEDA, 4},
    {"CALENDAR",   "UTILITY",  0x8E3A34, 0x2A100E, 0xFF8B7A, 5},
    {"VOICE NOTE", "RECORDER", 0xA03A78, 0x33102A, 0xFF9BD2, 6},
    {"WHATSAPP",   "SOCIAL",   0x1F9E5C, 0x0B2C1A, 0x5CF09B, 7},
    {"2048",       "PUZZLE",   0x9A7420, 0x2A1E0C, 0xFFD166, 8},
    {"JUICE RUSH", "ARCADE",   0x1F8B8B, 0x0A2626, 0x7BF0D8, 0},
};
#define LC_APP_NUM  ((int)(sizeof(s_apps) / sizeof(s_apps[0])))
#define LC_APP_2048 (LC_APP_NUM - 2)    /* 2048 is the second-to-last card. */
#define LC_APP_GAME (LC_APP_NUM - 1)    /* Juice Rush is the last card. */

/* ---------------------------- Static state ---------------------------- */
static lv_obj_t *s_root;
static lv_obj_t *s_title;       /* Currently selected app name. */
static lv_obj_t *s_sub;         /* Currently selected category. */
static lv_obj_t *s_cont;        /* Horizontal scroll container. */
static lv_obj_t *s_hint;        /* Bottom hint, also reused by the toast. */
static lv_obj_t *s_cards[LC_APP_NUM];
static lv_obj_t *s_dots[LC_APP_NUM];
static lv_timer_t *s_hint_timer;
static int32_t s_w, s_h;
static int32_t s_dot_x0;
static int s_sel;
/* Last selected card. Not cleared by launcher_reset(), so returning from a
 * game with HOME restores the same card instead of jumping back to the first. */
static int s_keep_sel;
static bool s_entering;

/* ---------------------------- Forward declarations ---------------------------- */
static void lc_update_focus(void);

/* ---------------------------- Basic drawing ---------------------------- */
static lv_color_t lc_hex(uint32_t v)
{
    return lv_color_hex(v);
}

/* Rounded rectangle / circle. Pass LV_OPA_TRANSP as opa for an outline-only shape. */
static lv_obj_t *lc_shape(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h,
                          uint32_t color, lv_opa_t opa, int32_t radius)
{
    lv_obj_t *o = lv_obj_create(parent);

    lv_obj_remove_style_all(o);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(o, radius, 0);
    lv_obj_set_style_bg_opa(o, opa, 0);
    lv_obj_set_style_bg_color(o, lc_hex(color), 0);
    lv_obj_set_size(o, w, h);
    lv_obj_set_pos(o, x, y);
    return o;
}

/* Outline-only circle. */
static lv_obj_t *lc_ring(lv_obj_t *parent, int32_t x, int32_t y, int32_t d,
                         uint32_t color, lv_opa_t opa, int32_t bw)
{
    lv_obj_t *o = lc_shape(parent, x, y, d, d, color, LV_OPA_TRANSP, LV_RADIUS_CIRCLE);

    lv_obj_set_style_border_width(o, bw, 0);
    lv_obj_set_style_border_color(o, lc_hex(color), 0);
    lv_obj_set_style_border_opa(o, opa, 0);
    return o;
}

/* Icons are authored in a 300x300 design space and scaled with the card width,
 * so changing LC_CARD_W/LC_CARD_H scales the whole card without editing every
 * coordinate. LC_ICON_DY lifts the icon to leave room for the accent bar. */
#define LC_ICON_UNIT 300
#define LC_ICON_DY   10
#define LC_ICON_S(v) ((int32_t)((int64_t)(v) * LC_CARD_W / LC_ICON_UNIT))
#define LC_ICON_Y(v) (LC_ICON_S(v) - LC_ICON_DY)

static lv_obj_t *lc_ishape(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h,
                           uint32_t color, lv_opa_t opa, int32_t radius)
{
    return lc_shape(parent, LC_ICON_S(x), LC_ICON_Y(y), LC_ICON_S(w), LC_ICON_S(h),
                    color, opa,
                    (radius == LV_RADIUS_CIRCLE) ? LV_RADIUS_CIRCLE : LC_ICON_S(radius));
}

static lv_obj_t *lc_iring(lv_obj_t *parent, int32_t x, int32_t y, int32_t d,
                          uint32_t color, lv_opa_t opa, int32_t bw)
{
    lv_obj_t *o = lc_ishape(parent, x, y, d, d, color, LV_OPA_TRANSP, LV_RADIUS_CIRCLE);

    lv_obj_set_style_border_width(o, bw, 0);
    lv_obj_set_style_border_color(o, lc_hex(color), 0);
    lv_obj_set_style_border_opa(o, opa, 0);
    return o;
}

static lv_obj_t *lc_label(lv_obj_t *parent, const lv_font_t *font, uint32_t color,
                          int32_t letter_space)
{
    lv_obj_t *l = lv_label_create(parent);

    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, lc_hex(color), 0);
    lv_obj_set_style_text_letter_space(l, letter_space, 0);
    return l;
}

/* ---------------------------- App icons (vector only) ---------------------------- */
static void lc_icon_draw(lv_obj_t *card, uint8_t kind)
{
    switch (kind) {
    case 0: /* Juice Rush: watermelon. */
        lc_ishape(card, 70, 43, 165, 165, 0x3FA45F, LV_OPA_COVER, LV_RADIUS_CIRCLE);
        lc_iring(card, 70, 43, 165, 0x9CF3B6, 90, 4);
        lc_ishape(card, 98, 70, 110, 110, 0xF2546E, LV_OPA_COVER, LV_RADIUS_CIRCLE);
        lc_ishape(card, 138, 93, 9, 9, 0x2A0B12, LV_OPA_COVER, LV_RADIUS_CIRCLE);
        lc_ishape(card, 170, 81, 9, 9, 0x2A0B12, LV_OPA_COVER, LV_RADIUS_CIRCLE);
        lc_ishape(card, 162, 132, 9, 9, 0x2A0B12, LV_OPA_COVER, LV_RADIUS_CIRCLE);
        lc_ishape(card, 129, 129, 9, 9, 0x2A0B12, LV_OPA_COVER, LV_RADIUS_CIRCLE);
        break;

    case 1: /* Music: vinyl record and note. */
        lc_ishape(card, 76, 48, 154, 154, 0x241C4E, LV_OPA_COVER, LV_RADIUS_CIRCLE);
        lc_iring(card, 76, 48, 154, 0xA897FF, 160, 6);
        lc_ishape(card, 134, 106, 39, 39, 0xA897FF, LV_OPA_COVER, LV_RADIUS_CIRCLE);
        lc_ishape(card, 186, 53, 9, 83, 0xFFFFFF, LV_OPA_COVER, 4);
        lc_ishape(card, 159, 121, 36, 36, 0xFFFFFF, LV_OPA_COVER, LV_RADIUS_CIRCLE);
        break;

    case 2: /* Gallery: photo frame, sun and mountains. */
        {
            lv_obj_t *frame = lc_ishape(card, 65, 51, 176, 149, 0xFFB39A,
                                        LV_OPA_TRANSP, 15);
            lv_obj_set_style_border_width(frame, 6, 0);
            lv_obj_set_style_border_color(frame, lc_hex(0xFFB39A), 0);
            lv_obj_set_style_border_opa(frame, 200, 0);
        }
        lc_ishape(card, 87, 73, 35, 35, 0xFFD166, LV_OPA_COVER, LV_RADIUS_CIRCLE);
        lc_ishape(card, 84, 131, 77, 61, 0xFF8A6B, LV_OPA_COVER, 39);
        lc_ishape(card, 142, 142, 88, 50, 0xFFC6A8, LV_OPA_COVER, 25);
        break;

    case 3: /* Weather: sun and cloud. */
        lc_ishape(card, 142, 43, 83, 83, 0xFFD166, 230, LV_RADIUS_CIRCLE);
        lc_ishape(card, 71, 136, 66, 66, 0xFFFFFF, 235, LV_RADIUS_CIRCLE);
        lc_ishape(card, 106, 119, 83, 83, 0xFFFFFF, 235, LV_RADIUS_CIRCLE);
        lc_ishape(card, 159, 139, 61, 61, 0xFFFFFF, 235, LV_RADIUS_CIRCLE);
        lc_ishape(card, 76, 165, 132, 40, 0xFFFFFF, 235, 20);
        break;

    case 4: /* Settings: gear made of a ring plus teeth. */
    {
        static const int32_t tooth[6][2] = {
            {63, 0}, {32, 55}, {-32, 55},
            {-63, 0}, {-32, -55}, {32, -55},
        };
        int i;

        lc_iring(card, 90, 62, 127, 0xC6CEDA, 200, 20);
        for (i = 0; i < 6; i++) {
            lc_ishape(card, 153 + tooth[i][0] - 11, 125 + tooth[i][1] - 11, 22, 22,
                      0xC6CEDA, 210, LV_RADIUS_CIRCLE);
        }
        break;
    }

    case 5: /* Calendar: white body, red header and month/day labels. */
    {
        lv_obj_t *l;

        /* Hangers are drawn first so the card body covers their lower half. */
        lc_ishape(card, 104, 34, 10, 25, 0x8A93A5, LV_OPA_COVER, 5);
        lc_ishape(card, 193, 34, 10, 25, 0x8A93A5, LV_OPA_COVER, 5);

        lc_ishape(card, 70, 48, 165, 165, 0xFFFFFF, LV_OPA_COVER, 29);

        /* Red header: rounded top corners, then a square block fills the bottom. */
        lc_ishape(card, 70, 48, 165, 68, 0xE5484D, LV_OPA_COVER, 29);
        lc_ishape(card, 70, 81, 165, 35, 0xE5484D, LV_OPA_COVER, 0);

        lc_ishape(card, 70, 116, 165, 1, 0xE5E8EF, LV_OPA_COVER, 0);

        l = lc_label(card, &lv_font_montserrat_16, 0xFFFFFF, 3);
        lv_label_set_text(l, "DEC");
        lv_obj_align(l, LV_ALIGN_TOP_MID, 0, LC_ICON_Y(71));

        l = lc_label(card, &lv_font_montserrat_32, 0x1F2430, 1);
        lv_label_set_text(l, "20");
        lv_obj_align(l, LV_ALIGN_TOP_MID, 0, LC_ICON_Y(150));
        break;
    }

    case 6: /* Voice note: magenta card with a white microphone. */
        lc_ishape(card, 70, 48, 165, 165, 0xA03A78, LV_OPA_COVER, 26);
        lc_ishape(card, 134, 64, 39, 83, 0xFFFFFF, LV_OPA_COVER, 19);
        lc_ishape(card, 149, 139, 9, 25, 0xFFFFFF, LV_OPA_COVER, 4);
        lc_ishape(card, 127, 163, 53, 8, 0xFFFFFF, LV_OPA_COVER, 4);
        break;

    case 7: /* WhatsApp: green circle, speech bubble and handset. */
        lc_ishape(card, 70, 48, 165, 165, 0x25D366, LV_OPA_COVER, LV_RADIUS_CIRCLE);
        lc_ishape(card, 105, 82, 97, 97, 0xFFFFFF, LV_OPA_COVER, LV_RADIUS_CIRCLE);
        lc_ishape(card, 85, 152, 25, 25, 0xFFFFFF, LV_OPA_COVER, LV_RADIUS_CIRCLE);

        /* The handset is assembled from upright shapes instead of transform_rotation:
         * software fill ignores the matrix and only takes the refr_obj_matrix() path,
         * whose inverse-transformed clip area is inflated by 1 px and can produce a
         * NULL destination pointer. Avoid transforms here. */
        lc_ishape(card, 143, 101, 20, 59, 0x25D366, LV_OPA_COVER, 10);
        lc_ishape(card, 138, 98, 31, 19, 0x25D366, LV_OPA_COVER, 9);
        lc_ishape(card, 138, 145, 31, 19, 0x25D366, LV_OPA_COVER, 9);
        break;

    case 8: /* 2048: gold tile with the game name. */
    {
        lv_obj_t *box = lc_ishape(card, 73, 51, 160, 160, 0x2A1E0C, LV_OPA_COVER, 24);
        lv_obj_t *l;

        lv_obj_set_style_border_width(box, 3, 0);
        lv_obj_set_style_border_color(box, lc_hex(0xFFD166), 0);

        l = lc_label(box, &lv_font_montserrat_32, 0xFFD166, 2);
        lv_label_set_text(l, "2048");
        lv_obj_center(l);
        break;
    }

    default:
        break;
    }
}

/* ---------------------------- Background / header / footer ---------------------------- */
static void lc_build_bg(void)
{
    static const int32_t glow_d[3] = {520, 360, 226};
    static const uint32_t glow_c[3] = {0x27456F, 0x31588A, 0x3D6BA5};
    static const lv_opa_t glow_o[3] = {30, 34, 40};
    static const int32_t star[8][3] = {
        {46, 104, 4}, {392, 126, 4}, {126, 218, 3}, {324, 252, 4},
        {71, 327, 3}, {370, 378, 3}, {181, 80, 3}, {269, 411, 3},
    };
    int i;

    /* Three-layer top glow. */
    for (i = 0; i < 3; i++) {
        lc_shape(s_root, (s_w - glow_d[i]) / 2, 110 - glow_d[i] / 2, glow_d[i], glow_d[i],
                 glow_c[i], glow_o[i], LV_RADIUS_CIRCLE);
    }

    /* Static star dots placed in the gaps between cards. */
    for (i = 0; i < 8; i++) {
        lc_shape(s_root, star[i][0], star[i][1], star[i][2], star[i][2],
                 0xBFD8FF, 110, LV_RADIUS_CIRCLE);
    }
}

static void lc_build_header(void)
{
    lv_obj_t *l;

    l = lc_label(s_root, &lv_font_montserrat_16, LC_COLOR_TINY, 8);
    lv_label_set_text(l, "APPLICATIONS");
    lv_obj_align(l, LV_ALIGN_TOP_MID, 0, 18);

    /* Selected app name, updated while scrolling. */
    s_title = lc_label(s_root, &lv_font_montserrat_24, 0xFFFFFF, 2);
    lv_label_set_text(s_title, s_apps[0].name);
    lv_obj_align(s_title, LV_ALIGN_TOP_MID, 0, 40);

    s_sub = lc_label(s_root, &lv_font_montserrat_16, LC_COLOR_DIM, 4);
    lv_label_set_text(s_sub, s_apps[0].tag);
    lv_obj_align(s_sub, LV_ALIGN_TOP_MID, 0, 78);
}

static void lc_build_footer(void)
{
    lv_obj_t *panel;
    lv_obj_t *l;

    panel = lc_shape(s_root, 0, 436, s_w, s_h - 436, 0x0B1421, 215, 0);
    lv_obj_set_style_border_width(panel, 2, 0);
    lv_obj_set_style_border_color(panel, lc_hex(0x24334A), 0);
    lv_obj_set_style_border_side(panel, LV_BORDER_SIDE_TOP, 0);

    s_hint = lc_label(s_root, &lv_font_montserrat_12, LC_COLOR_DIM, 2);
    lv_label_set_text(s_hint, LC_HINT_TEXT);
    lv_obj_align(s_hint, LV_ALIGN_TOP_MID, 0, 442);

    l = lc_label(s_root, &lv_font_montserrat_12, 0x4F6785, 2);
    lv_label_set_text(l, "LVGL v9  |  BK7259");
    lv_obj_align(l, LV_ALIGN_TOP_MID, 0, 462);
}

/* ---------------------------- Cards and scroll container ---------------------------- */
static void lc_scroll_cb(lv_event_t *e);
static void lc_card_cb(lv_event_t *e);

static void lc_build_card(int idx)
{
    const lc_app_t *a = &s_apps[idx];
    lv_obj_t *card;
    lv_obj_t *l;

    card = lv_obj_create(s_cont);
    lv_obj_remove_style_all(card);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(card, LC_CARD_W, LC_CARD_H);
    /* Child coordinates are relative to the content area; the container padding
     * already centers the first card. */
    lv_obj_set_pos(card, idx * (LC_CARD_W + LC_CARD_GAP), 0);
    lv_obj_set_style_radius(card, 26, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(card, lc_hex(a->color_top), 0);
    lv_obj_set_style_bg_grad_color(card, lc_hex(a->color_bottom), 0);
    lv_obj_set_style_bg_grad_dir(card, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_border_color(card, lc_hex(0x3A5580), 0);
    lv_obj_add_event_cb(card, lc_card_cb, LV_EVENT_CLICKED, (void *)(intptr_t)idx);
    s_cards[idx] = card;

    /* Top-right index, also used as the page indicator. */
    l = lc_label(card, &lv_font_montserrat_12, a->accent, 2);
    lv_label_set_text_fmt(l, "APP %02d / %02d", idx + 1, LC_APP_NUM);
    lv_obj_align(l, LV_ALIGN_TOP_RIGHT, -14, 10);

    /* App icon. */
    lc_icon_draw(card, a->kind);

    /* Accent bar, name and category, spaced by 10 px and 8 px. */
    {
        lv_obj_t *bar = lc_shape(card, (LC_CARD_W - 66) / 2, 0, 66, 4, a->accent, 220, 2);
        lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, -70);
    }

    l = lc_label(card, &lv_font_montserrat_20, 0xFFFFFF, 1);
    lv_label_set_text(l, a->name);
    lv_obj_align(l, LV_ALIGN_BOTTOM_MID, 0, -36);

    l = lc_label(card, &lv_font_montserrat_12, LC_COLOR_DIM, 3);
    lv_label_set_text(l, a->tag);
    lv_obj_align(l, LV_ALIGN_BOTTOM_MID, 0, -14);
}

static void lc_build_carousel(void)
{
    int32_t pad_x = (s_w - LC_CARD_W) / 2;
    int i;

    s_cont = lv_obj_create(s_root);
    lv_obj_remove_style_all(s_cont);
    lv_obj_add_flag(s_cont, LV_OBJ_FLAG_SCROLLABLE);
    /* No LV_OBJ_FLAG_SCROLL_ONE: momentum scrolling lets a fast swipe pass
     * several cards, then CENTER snap settles on the nearest one. */
    lv_obj_set_size(s_cont, s_w, LC_CARD_H + LC_CARD_PAD_V * 2);
    lv_obj_set_pos(s_cont, 0, 104);
    lv_obj_set_scroll_dir(s_cont, LV_DIR_HOR);
    lv_obj_set_scroll_snap_x(s_cont, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_scrollbar_mode(s_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(s_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_cont, 0, 0);
    lv_obj_set_style_pad_all(s_cont, 0, 0);
    lv_obj_set_style_pad_left(s_cont, pad_x, 0);
    lv_obj_set_style_pad_right(s_cont, pad_x, 0);
    lv_obj_set_style_pad_top(s_cont, LC_CARD_PAD_V, 0);
    lv_obj_set_style_pad_bottom(s_cont, LC_CARD_PAD_V, 0);
    lv_obj_add_event_cb(s_cont, lc_scroll_cb, LV_EVENT_SCROLL, NULL);

    for (i = 0; i < LC_APP_NUM; i++) {
        lc_build_card(i);
    }
}

static void lc_build_dots(void)
{
    int i;

    s_dot_x0 = (s_w - LC_APP_NUM * LC_DOT_SLOT) / 2;
    for (i = 0; i < LC_APP_NUM; i++) {
        lv_obj_t *d = lc_shape(s_root, s_dot_x0 + i * LC_DOT_SLOT + (LC_DOT_SLOT - LC_DOT_H) / 2,
                               386, LC_DOT_H, LC_DOT_H, 0x4A6485, LV_OPA_COVER, LC_DOT_H / 2);
        s_dots[i] = d;
    }
}

/* ---------------------------- Focus state ---------------------------- */
/* Refresh only the affected card and dot. During momentum scrolling s_sel can
 * jump several cards per frame; restyling all nine 300x300 cards would dirty
 * the whole strip and repeatedly allocate style properties. */
static void lc_apply_card(int i, bool on)
{
    int32_t dw = on ? 22 : LC_DOT_H;

    /* Selected card: full opacity, no offset, accent border. */
    lv_obj_set_style_opa(s_cards[i], on ? LV_OPA_COVER : 130, 0);
    lv_obj_set_style_translate_y(s_cards[i], on ? 0 : 10, 0);
    lv_obj_set_style_border_width(s_cards[i], on ? 3 : 2, 0);
    lv_obj_set_style_border_color(s_cards[i],
                                  lc_hex(on ? s_apps[i].accent : 0x3A5580), 0);

    /* Selected dot grows and brightens. */
    lv_obj_set_width(s_dots[i], dw);
    lv_obj_set_x(s_dots[i], s_dot_x0 + i * LC_DOT_SLOT + (LC_DOT_SLOT - dw) / 2);
    lv_obj_set_style_bg_color(s_dots[i], lc_hex(on ? 0xFFFFFF : 0x4A6485), 0);
}

/* Switch selection and repaint only the old and new card. */
static void lc_set_sel(int idx)
{
    int old = s_sel;

    if (idx == old) {
        return;
    }
    s_sel = idx;
    s_keep_sel = idx;
    lc_apply_card(old, false);
    lc_apply_card(idx, true);

    lv_label_set_text(s_title, s_apps[idx].name);
    lv_label_set_text(s_sub, s_apps[idx].tag);
}

/* Full initialization, called once from launcher_start(). */
static void lc_update_focus(void)
{
    int i;

    for (i = 0; i < LC_APP_NUM; i++) {
        lc_apply_card(i, i == s_sel);
    }

    lv_label_set_text(s_title, s_apps[s_sel].name);
    lv_label_set_text(s_sub, s_apps[s_sel].tag);
}

/* Find the card closest to the container center. Cards are evenly spaced, so
 * the index follows directly from scroll_x instead of scanning all nine cards;
 * the SCROLL event fires very often during momentum scrolling. */
static void lc_scroll_cb(lv_event_t *e)
{
    lv_obj_t *cont = lv_event_get_target(e);
    const int32_t step = LC_CARD_W + LC_CARD_GAP;
    int32_t sx = lv_obj_get_scroll_x(cont);
    int idx = (int)((sx + step / 2) / step);

    if (idx < 0) {
        idx = 0;
    } else if (idx >= LC_APP_NUM) {
        idx = LC_APP_NUM - 1;
    }

    if (idx != s_sel) {
        lc_set_sel(idx);
    }
}

/* ---------------------------- Hint and app entry ---------------------------- */
static void lc_hint_timer_cb(lv_timer_t *t)
{
    (void)t;
    lv_label_set_text(s_hint, LC_HINT_TEXT);
    lv_obj_set_style_text_color(s_hint, lc_hex(LC_COLOR_DIM), 0);
    lv_timer_pause(s_hint_timer);
}

static void lc_toast(void)
{
    if (s_hint_timer == NULL) {
        return;
    }
    lv_label_set_text(s_hint, "THIS APP IS COMING SOON");
    lv_obj_set_style_text_color(s_hint, lc_hex(0xFFD54A), 0);
    lv_timer_reset(s_hint_timer);
    lv_timer_resume(s_hint_timer);
}

static void lc_card_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);

    if (s_entering) {
        return;
    }

    /* Tapping a side card only scrolls it to center. */
    if (idx != s_sel) {
        lc_set_sel(idx);
        lv_obj_scroll_to_x(s_cont, idx * (LC_CARD_W + LC_CARD_GAP), LV_ANIM_ON);
        return;
    }

    if (idx == LC_APP_GAME) {
        s_entering = true;
        launcher_stop();
        juice_rush_start();
        return;
    }

    if (idx == LC_APP_2048) {
        s_entering = true;
        launcher_stop();
        game_2048_start();
        return;
    }

    lc_toast();
}

/* ---------------------------- Public API ---------------------------- */
void launcher_reset(void)
{
    s_root = NULL;
    s_title = NULL;
    s_sub = NULL;
    s_cont = NULL;
    s_hint = NULL;
    s_hint_timer = NULL;
    memset(s_cards, 0, sizeof(s_cards));
    memset(s_dots, 0, sizeof(s_dots));
    s_sel = 0;
    s_entering = false;
    s_dot_x0 = 0;
}

void launcher_stop(void)
{
    if (s_hint_timer != NULL) {
        lv_timer_delete(s_hint_timer);
        s_hint_timer = NULL;
    }
    if (s_root != NULL) {
        lv_obj_delete(s_root);
    }
    launcher_reset();
}

void launcher_start(void)
{
    lv_display_t *disp;

    launcher_reset();

    disp = lv_display_get_default();
    if (disp == NULL) {
        return;
    }
    s_w = lv_display_get_horizontal_resolution(disp);
    s_h = lv_display_get_vertical_resolution(disp);

    /* The page manages its own scrolling. */
    lv_obj_remove_flag(lv_screen_active(), LV_OBJ_FLAG_SCROLLABLE);

    s_root = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(s_root);
    lv_obj_remove_flag(s_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(s_root, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(s_root, s_w, s_h);
    lv_obj_set_pos(s_root, 0, 0);
    lv_obj_set_style_bg_opa(s_root, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(s_root, lc_hex(LC_COLOR_BG_TOP), 0);
    lv_obj_set_style_bg_grad_color(s_root, lc_hex(LC_COLOR_BG_BOT), 0);
    lv_obj_set_style_bg_grad_dir(s_root, LV_GRAD_DIR_VER, 0);

    lc_build_bg();
    lc_build_header();
    lc_build_carousel();
    lc_build_dots();
    lc_build_footer();

    /* Restore the last selected card, so HOME does not jump back to the first. */
    s_sel = s_keep_sel;
    if (s_sel < 0) {
        s_sel = 0;
    } else if (s_sel >= LC_APP_NUM) {
        s_sel = LC_APP_NUM - 1;
    }
    lc_update_focus();

    lv_obj_update_layout(s_cont);
    lv_obj_scroll_to_x(s_cont, s_sel * (LC_CARD_W + LC_CARD_GAP), LV_ANIM_OFF);

    s_hint_timer = lv_timer_create(lc_hint_timer_cb, LC_TOAST_MS, NULL);
    if (s_hint_timer != NULL) {
        lv_timer_pause(s_hint_timer);
    }
}
