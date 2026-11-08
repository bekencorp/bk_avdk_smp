/**
 * @file game_2048.c
 * @brief 2048 puzzle game (LVGL v9, plain C).
 *
 * Swipe up/down/left/right to slide all tiles; equal neighbours merge and add
 * to the score. Every effective move spawns a 2 (10% chance of a 4). The game
 * ends when the board is full and no merge is possible.
 *
 * Performance notes:
 *   1. The 16 tiles and labels are created once and only moved/restyled, so the
 *      LVGL heap is not fragmented during play.
 *   2. The board and background are static; only moved tiles are invalidated.
 *   3. No images or frame animations, which keeps software rendering cheap.
 *   4. Swipe detection sits on a full-screen transparent layer below the buttons.
 */
#include "lvgl.h"
#include <stdint.h>
#include <string.h>
#include "game_2048.h"
#include "launcher.h"

/* ---------------------------- Layout ---------------------------- */
#define G_COLS      4
#define G_CELL      64
#define G_GAP       8
#define G_BOARD_W   (G_COLS * G_CELL + (G_COLS - 1) * G_GAP)   /* 280 */
#define G_BOARD_X   ((480 - G_BOARD_W) / 2)                    /* 100 */
#define G_BOARD_Y   72

#define G_SWIPE_MIN 60      /* Minimum travel in pixels to count as a swipe. */

/* ---------------------------- Data types ---------------------------- */
typedef struct {
    lv_obj_t *obj;
    lv_obj_t *lbl;
} g_tile_t;

/* ---------------------------- Static state ---------------------------- */
static lv_obj_t *s_root;
static lv_obj_t *s_ui;
static lv_obj_t *s_board;
static lv_obj_t *s_touch;
static lv_obj_t *s_lbl_score;
static lv_obj_t *s_lbl_best;
static lv_obj_t *s_lbl_hint;
static lv_obj_t *s_overlay;

static g_tile_t s_tiles[G_COLS * G_COLS];

static uint8_t s_b[G_COLS][G_COLS];   /* 0 = empty, otherwise log2 of the value (1 means 2). */
static int32_t s_score, s_best;
static bool s_over, s_won;
static int32_t s_w, s_h;
static int32_t s_press_x, s_press_y;

/* ---------------------------- Forward declarations ---------------------------- */
static void g_move(int dir);
static void g_btn_home_cb(lv_event_t *e);
static void g_btn_restart_cb(lv_event_t *e);

/* ---------------------------- Basic drawing ---------------------------- */
static lv_color_t g_hex(uint32_t v)
{
    return lv_color_hex(v);
}

static lv_obj_t *g_shape(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h,
                         uint32_t color, lv_opa_t opa, int32_t radius)
{
    lv_obj_t *o = lv_obj_create(parent);

    lv_obj_remove_style_all(o);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(o, radius, 0);
    lv_obj_set_style_bg_opa(o, opa, 0);
    lv_obj_set_style_bg_color(o, g_hex(color), 0);
    lv_obj_set_size(o, w, h);
    lv_obj_set_pos(o, x, y);
    return o;
}

static lv_obj_t *g_label(lv_obj_t *parent, const lv_font_t *font, uint32_t color,
                         int32_t letter_space)
{
    lv_obj_t *l = lv_label_create(parent);

    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, g_hex(color), 0);
    lv_obj_set_style_text_letter_space(l, letter_space, 0);
    return l;
}

/* Tile color by exponent. */
static uint32_t g_tile_bg(uint8_t exp)
{
    static const uint32_t color[] = {
        0x000000,
        0xEEE4DA, 0xEDE0C8, 0xF2B179, 0xF59563, 0xF67C5F,
        0xF65E3B, 0xEDCF72, 0xEDCC61, 0xEDC850, 0xEDC53F,
        0xEDC22E, 0x3C3A32, 0x3C3A32, 0x3C3A32, 0x3C3A32,
    };

    if (exp >= sizeof(color) / sizeof(color[0])) {
        exp = (uint8_t)(sizeof(color) / sizeof(color[0]) - 1);
    }
    return color[exp];
}

/* ---------------------------- Board logic ---------------------------- */
static uint8_t g_get(int dir, int line, int pos)
{
    switch (dir) {
    case 0:  return s_b[line][pos];        /* Left */
    case 1:  return s_b[line][G_COLS - 1 - pos]; /* Right */
    case 2:  return s_b[pos][line];        /* Up */
    default: return s_b[G_COLS - 1 - pos][line]; /* Down */
    }
}

static void g_set(int dir, int line, int pos, uint8_t v)
{
    switch (dir) {
    case 0:  s_b[line][pos] = v; break;
    case 1:  s_b[line][G_COLS - 1 - pos] = v; break;
    case 2:  s_b[pos][line] = v; break;
    default: s_b[G_COLS - 1 - pos][line] = v; break;
    }
}

/* Compress and merge one line toward index 0; returns whether anything moved. */
static bool g_slide_line(uint8_t *v, int32_t *gain)
{
    uint8_t tmp[G_COLS] = {0};
    uint8_t res[G_COLS] = {0};
    int i, n = 0, r = 0;

    for (i = 0; i < G_COLS; i++) {
        if (v[i]) {
            tmp[n++] = v[i];
        }
    }

    for (i = 0; i < n; i++) {
        if (i + 1 < n && tmp[i] == tmp[i + 1]) {
            res[r] = (uint8_t)(tmp[i] + 1);
            *gain += (int32_t)(1u << res[r]);
            r++;
            i++;    /* Skip the merged tile. */
        } else {
            res[r++] = tmp[i];
        }
    }

    if (memcmp(v, res, G_COLS) == 0) {
        return false;
    }
    memcpy(v, res, G_COLS);
    return true;
}

static bool g_can_move(void)
{
    int r, c;

    for (r = 0; r < G_COLS; r++) {
        for (c = 0; c < G_COLS; c++) {
            if (s_b[r][c] == 0) {
                return true;
            }
            if (c + 1 < G_COLS && s_b[r][c] == s_b[r][c + 1]) {
                return true;
            }
            if (r + 1 < G_COLS && s_b[r][c] == s_b[r + 1][c]) {
                return true;
            }
        }
    }
    return false;
}

static void g_spawn(void)
{
    int empty[G_COLS * G_COLS];
    int n = 0, r, c, pick;

    for (r = 0; r < G_COLS; r++) {
        for (c = 0; c < G_COLS; c++) {
            if (s_b[r][c] == 0) {
                empty[n++] = r * G_COLS + c;
            }
        }
    }
    if (n == 0) {
        return;
    }

    pick = empty[lv_rand(0, n - 1)];
    /* 90% chance of 2, 10% chance of 4. */
    s_b[pick / G_COLS][pick % G_COLS] = (lv_rand(0, 9) == 0) ? 2 : 1;
}

/* Tile font shrinks with digit count so four digits still fit the cell. */
static const lv_font_t *g_tile_font(uint8_t exp)
{
    if (exp <= 6) {
        return &lv_font_montserrat_40;   /* 2 ~ 64 */
    }
    if (exp <= 9) {
        return &lv_font_montserrat_32;   /* 128 ~ 512 */
    }
    return &lv_font_montserrat_24;       /* 1024 and above */
}

/* ---------------------------- Rendering ---------------------------- */
static void g_render(void)
{
    int r, c, i = 0;

    for (r = 0; r < G_COLS; r++) {
        for (c = 0; c < G_COLS; c++) {
            uint8_t v = s_b[r][c];
            g_tile_t *t = &s_tiles[i];
            const lv_font_t *font;

            i++;
            if (v == 0) {
                lv_obj_add_flag(t->obj, LV_OBJ_FLAG_HIDDEN);
                continue;
            }

            lv_obj_remove_flag(t->obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(t->obj, G_BOARD_X + c * (G_CELL + G_GAP),
                           G_BOARD_Y + r * (G_CELL + G_GAP));
            lv_obj_set_style_bg_color(t->obj, g_hex(g_tile_bg(v)), 0);
            /* Classic palette: dark brown text on 2/4, off-white on the rest. */
            lv_obj_set_style_text_color(t->lbl, g_hex(v <= 2 ? 0x776E65 : 0xF9F6F2), 0);

            font = g_tile_font(v);
            if (lv_obj_get_style_text_font(t->lbl, 0) != font) {
                lv_obj_set_style_text_font(t->lbl, font, 0);
            }
            lv_label_set_text_fmt(t->lbl, "%d", (int)(1u << v));
        }
    }

    lv_label_set_text_fmt(s_lbl_score, "%d", (int)s_score);
    lv_label_set_text_fmt(s_lbl_best, "%d", (int)s_best);
}

/* ---------------------------- Game over / restart ---------------------------- */
static void g_show_over(void)
{
    lv_obj_t *card;
    lv_obj_t *l;
    lv_obj_t *btn;

    s_over = true;
    if (s_overlay != NULL) {
        return;
    }

    /* Semi-transparent mask that also blocks the swipe layer below. */
    s_overlay = lv_obj_create(s_ui);
    lv_obj_remove_style_all(s_overlay);
    lv_obj_remove_flag(s_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(s_overlay, s_w, s_h);
    lv_obj_set_pos(s_overlay, 0, 0);
    lv_obj_set_style_bg_opa(s_overlay, 235, 0);
    lv_obj_set_style_bg_color(s_overlay, g_hex(0xFAF8EF), 0);

    card = g_shape(s_overlay, 40, 90, 400, 300, 0xEEE4DA, LV_OPA_COVER, 20);
    lv_obj_set_style_border_width(card, 3, 0);
    lv_obj_set_style_border_color(card, g_hex(0xBBADA0), 0);

    l = g_label(card, &lv_font_montserrat_32, 0x776E65, 2);
    lv_label_set_text(l, "GAME OVER");
    lv_obj_align(l, LV_ALIGN_TOP_MID, 0, 26);

    l = g_label(card, &lv_font_montserrat_16, 0x8F7A66, 2);
    lv_label_set_text_fmt(l, "SCORE   %d", (int)s_score);
    lv_obj_align(l, LV_ALIGN_TOP_MID, 0, 92);

    l = g_label(card, &lv_font_montserrat_16, 0x8F7A66, 2);
    lv_label_set_text_fmt(l, "BEST    %d", (int)s_best);
    lv_obj_align(l, LV_ALIGN_TOP_MID, 0, 122);

    btn = lv_button_create(card);
    lv_obj_set_size(btn, 150, 44);
    lv_obj_set_style_radius(btn, 12, 0);
    lv_obj_set_style_bg_color(btn, g_hex(0x8F7A66), 0);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 24, -32);
    lv_obj_add_event_cb(btn, g_btn_restart_cb, LV_EVENT_CLICKED, NULL);
    l = g_label(btn, &lv_font_montserrat_16, 0xF9F6F2, 2);
    lv_label_set_text(l, "RETRY");
    lv_obj_center(l);

    btn = lv_button_create(card);
    lv_obj_set_size(btn, 150, 44);
    lv_obj_set_style_radius(btn, 12, 0);
    lv_obj_set_style_bg_color(btn, g_hex(0xBBADA0), 0);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, -24, -32);
    lv_obj_add_event_cb(btn, g_btn_home_cb, LV_EVENT_CLICKED, NULL);
    l = g_label(btn, &lv_font_montserrat_16, 0xF9F6F2, 2);
    lv_label_set_text(l, "HOME");
    lv_obj_center(l);
}

static void g_new_game(void)
{
    memset(s_b, 0, sizeof(s_b));
    s_score = 0;
    s_over = false;
    s_won = false;

    if (s_overlay != NULL) {
        lv_obj_delete(s_overlay);
        s_overlay = NULL;
    }
    lv_label_set_text(s_lbl_hint, "SLIDE UP / DOWN / LEFT / RIGHT  -  EQUAL TILES MERGE");

    g_spawn();
    g_spawn();
    g_render();
}

static void g_btn_restart_cb(lv_event_t *e)
{
    (void)e;
    g_new_game();
}

static void g_btn_home_cb(lv_event_t *e)
{
    (void)e;
    game_2048_stop();
    launcher_start();
}

/* Move one step toward dir: 0 left, 1 right, 2 up, 3 down. */
static void g_move(int dir)
{
    int i, j;
    bool changed = false;
    int32_t gain = 0;

    if (s_over || s_root == NULL) {
        return;
    }

    for (i = 0; i < G_COLS; i++) {
        uint8_t v[G_COLS];

        for (j = 0; j < G_COLS; j++) {
            v[j] = g_get(dir, i, j);
        }
        if (g_slide_line(v, &gain)) {
            changed = true;
        }
        for (j = 0; j < G_COLS; j++) {
            g_set(dir, i, j, v[j]);
        }
    }

    if (!changed) {
        return;
    }

    s_score += gain;
    if (s_score > s_best) {
        s_best = s_score;
    }

    g_spawn();
    g_render();

    /* Show a hint the first time 2048 is reached, then keep playing. */
    if (!s_won) {
        for (i = 0; i < G_COLS && !s_won; i++) {
            for (j = 0; j < G_COLS; j++) {
                if (s_b[i][j] >= 12) {
                    s_won = true;
                    lv_label_set_text(s_lbl_hint, "2048 REACHED!  KEEP GOING");
                    break;
                }
            }
        }
    }

    if (!g_can_move()) {
        g_show_over();
    }
}

/* ---------------------------- Swipe input ---------------------------- */
static void g_touch_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev = lv_indev_active();
    lv_point_t p;

    if (indev == NULL) {
        return;
    }
    lv_indev_get_point(indev, &p);

    if (code == LV_EVENT_PRESSED) {
        s_press_x = p.x;
        s_press_y = p.y;
        return;
    }

    if (code != LV_EVENT_RELEASED) {
        return;
    }

    {
        int32_t dx = p.x - s_press_x;
        int32_t dy = p.y - s_press_y;
        int32_t adx = dx < 0 ? -dx : dx;
        int32_t ady = dy < 0 ? -dy : dy;

        if (adx < G_SWIPE_MIN && ady < G_SWIPE_MIN) {
            return;
        }

        if (adx > ady) {
            g_move(dx > 0 ? 1 : 0);
        } else {
            g_move(dy > 0 ? 3 : 2);
        }
    }
}

/* ---------------------------- Scene construction ---------------------------- */
/* Classic 2048 palette: off-white background, brown board and warm tiles. */

static void g_build_header(void)
{
    lv_obj_t *l;

    l = g_label(s_ui, &lv_font_montserrat_32, 0x776E65, 1);
    lv_label_set_text(l, "2048");
    lv_obj_set_pos(l, 16, 8);

    l = g_label(s_ui, &lv_font_montserrat_12, 0x8F7A66, 1);
    lv_label_set_text(l, "JOIN THE TILES TO REACH 2048!");
    lv_obj_set_pos(l, 16, 42);

    /* Score box. */
    {
        lv_obj_t *box = g_shape(s_ui, 316, 8, 74, 44, 0xBBADA0, LV_OPA_COVER, 10);

        l = g_label(box, &lv_font_montserrat_12, 0xEEE4DA, 2);
        lv_label_set_text(l, "SCORE");
        lv_obj_align(l, LV_ALIGN_TOP_MID, 0, 4);
        s_lbl_score = g_label(box, &lv_font_montserrat_20, 0xFFFFFF, 1);
        lv_label_set_text(s_lbl_score, "0");
        lv_obj_align(s_lbl_score, LV_ALIGN_BOTTOM_MID, 0, -4);
    }

    {
        lv_obj_t *box = g_shape(s_ui, 398, 8, 74, 44, 0xBBADA0, LV_OPA_COVER, 10);

        l = g_label(box, &lv_font_montserrat_12, 0xEEE4DA, 2);
        lv_label_set_text(l, "BEST");
        lv_obj_align(l, LV_ALIGN_TOP_MID, 0, 4);
        s_lbl_best = g_label(box, &lv_font_montserrat_20, 0xFFFFFF, 1);
        lv_label_set_text(s_lbl_best, "0");
        lv_obj_align(s_lbl_best, LV_ALIGN_BOTTOM_MID, 0, -4);
    }
}

static void g_build_board(void)
{
    int i;

    /* Board background. */
    s_board = g_shape(s_root, G_BOARD_X - 8, G_BOARD_Y - 8, G_BOARD_W + 16, G_BOARD_W + 16,
                      0xBBADA0, LV_OPA_COVER, 16);

    /* Empty slots stay visible when a tile moves away. */
    for (i = 0; i < G_COLS * G_COLS; i++) {
        g_shape(s_root, G_BOARD_X + (i % G_COLS) * (G_CELL + G_GAP),
                G_BOARD_Y + (i / G_COLS) * (G_CELL + G_GAP),
                G_CELL, G_CELL, 0xCDC1B4, LV_OPA_COVER, 8);
    }

    for (i = 0; i < G_COLS * G_COLS; i++) {
        s_tiles[i].obj = g_shape(s_root, 0, 0, G_CELL, G_CELL, 0xEEE4DA, LV_OPA_COVER, 8);
        s_tiles[i].lbl = g_label(s_tiles[i].obj, &lv_font_montserrat_40, 0x776E65, 1);
        lv_label_set_text(s_tiles[i].lbl, "2");
        lv_obj_center(s_tiles[i].lbl);
        lv_obj_add_flag(s_tiles[i].obj, LV_OBJ_FLAG_HIDDEN);
    }
}

static void g_build_footer(void)
{
    lv_obj_t *btn;
    lv_obj_t *l;

    s_lbl_hint = g_label(s_ui, &lv_font_montserrat_12, 0x8F7A66, 1);
    lv_label_set_text(s_lbl_hint, "SLIDE UP / DOWN / LEFT / RIGHT - EQUAL TILES MERGE");
    lv_obj_align(s_lbl_hint, LV_ALIGN_TOP_MID, 0, 384);

    btn = lv_button_create(s_ui);
    lv_obj_set_size(btn, 160, 34);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_set_style_bg_color(btn, g_hex(0x8F7A66), 0);
    lv_obj_set_pos(btn, 64, 406);
    lv_obj_add_event_cb(btn, g_btn_restart_cb, LV_EVENT_CLICKED, NULL);
    l = g_label(btn, &lv_font_montserrat_16, 0xF9F6F2, 1);
    lv_label_set_text(l, "NEW GAME");
    lv_obj_center(l);

    btn = lv_button_create(s_ui);
    lv_obj_set_size(btn, 160, 34);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_set_style_bg_color(btn, g_hex(0xBBADA0), 0);
    lv_obj_set_pos(btn, 256, 406);
    lv_obj_add_event_cb(btn, g_btn_home_cb, LV_EVENT_CLICKED, NULL);
    l = g_label(btn, &lv_font_montserrat_16, 0xF9F6F2, 1);
    lv_label_set_text(l, "HOME");
    lv_obj_center(l);
}

/* ---------------------------- Public API ---------------------------- */
void game_2048_reset(void)
{
    s_root = NULL;
    s_ui = NULL;
    s_board = NULL;
    s_touch = NULL;
    s_lbl_score = NULL;
    s_lbl_best = NULL;
    s_lbl_hint = NULL;
    s_overlay = NULL;
    memset(s_tiles, 0, sizeof(s_tiles));
    memset(s_b, 0, sizeof(s_b));
    s_score = 0;
    s_over = false;
    s_won = false;
}

void game_2048_stop(void)
{
    if (s_root != NULL) {
        lv_obj_delete(s_root);
    }
    game_2048_reset();
}

void game_2048_start(void)
{
    lv_display_t *disp;

    game_2048_reset();

    disp = lv_display_get_default();
    if (disp == NULL) {
        return;
    }
    s_w = lv_display_get_horizontal_resolution(disp);
    s_h = lv_display_get_vertical_resolution(disp);
    lv_rand_set_seed(lv_tick_get());

    lv_obj_remove_flag(lv_screen_active(), LV_OBJ_FLAG_SCROLLABLE);

    s_root = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(s_root);
    lv_obj_remove_flag(s_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(s_root, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(s_root, s_w, s_h);
    lv_obj_set_pos(s_root, 0, 0);
    lv_obj_set_style_bg_opa(s_root, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(s_root, g_hex(0xFAF8EF), 0);

    g_build_board();

    /* Swipe layer: above the tiles, below the buttons. */
    s_touch = lv_obj_create(s_root);
    lv_obj_remove_style_all(s_touch);
    lv_obj_remove_flag(s_touch, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_touch, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(s_touch, LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_set_size(s_touch, s_w, s_h);
    lv_obj_set_pos(s_touch, 0, 0);
    lv_obj_set_style_bg_opa(s_touch, LV_OPA_TRANSP, 0);
    lv_obj_add_event_cb(s_touch, g_touch_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(s_touch, g_touch_cb, LV_EVENT_RELEASED, NULL);

    /* UI layer: title, score, buttons and game-over overlay. */
    s_ui = lv_obj_create(s_root);
    lv_obj_remove_style_all(s_ui);
    lv_obj_remove_flag(s_ui, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(s_ui, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(s_ui, s_w, s_h);
    lv_obj_set_pos(s_ui, 0, 0);

    g_build_header();
    g_build_footer();

    g_new_game();
}
