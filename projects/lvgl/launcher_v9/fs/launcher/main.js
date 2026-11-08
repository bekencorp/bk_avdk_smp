/*
 * Shared drawing helpers for the launcher and the two games.
 *
 * Everything is built from plain lv_obj primitives, matching the original C
 * implementation: no image assets and no decoders are needed on this target.
 */

var W = 480;
var H = 480;

var OPA_TRANSP = 0;
var OPA_COVER = 255;
var RADIUS_CIRCLE = 0x7FFF;

/* Plain rounded rectangle without any theme style. */
function shape(parent, x, y, w, h, color, opa, radius) {
  var o = lv.new('obj', parent, { x: x, y: y, w: w, h: h });
  o.call('lv_obj_remove_style_all');
  o.flag('clickable', false);
  o.flag('scrollable', false);
  o.setStyle('radius', radius);
  o.setStyle('bg_opa', opa);
  o.setStyle('bg_color', color);
  return o;
}

function circle(parent, x, y, d, color, opa) {
  return shape(parent, x, y, d, d, color, opa, RADIUS_CIRCLE);
}

function ring(parent, x, y, d, color, opa, bw) {
  var o = shape(parent, x, y, d, d, color, OPA_TRANSP, RADIUS_CIRCLE);
  o.setStyle('border_width', bw);
  o.setStyle('border_color', color);
  o.setStyle('border_opa', opa);
  return o;
}

function label(parent, text, font, color, letter) {
  var l = lv.new('label', parent, { text: text, font: font });
  l.setStyle('text_color', color);
  l.setStyle('text_letter_space', letter || 0);
  return l;
}

function show(o) {
  if (o) { o.flag('hidden', false); }
}

function hide(o) {
  if (o) { o.flag('hidden', true); }
}

/* Uniform random integer in [lo, hi] using the LVGL generator, so the same
 * sequence as the C version is produced for the same seed. */
function rnd(lo, hi) {
  return lv.call('lv_rand', lo, hi);
}

function norm(v) {
  return v < 0 ? -v : v;
}
/*
 * Application launcher: horizontal carousel used to pick and open an app.
 *
 * Cards sit in a horizontal scroll container with CENTER snap. The card closest
 * to the screen center drives the title, subtitle and page dots. Tapping the
 * centered card opens the app; tapping a side card only scrolls it to center.
 */

var LC_CARD_W = 258;
var LC_CARD_H = 258;
var LC_CARD_GAP = 26;
var LC_DOT_SLOT = 26;
var LC_DOT_H = 10;
var LC_TOAST_MS = 1600;

var LC_HINT_TEXT = 'SWIPE  |  TAP TO OPEN';

var LC_COLOR_BG_TOP = 0x16243C;
var LC_COLOR_BG_BOT = 0x060A12;
var LC_COLOR_DIM = 0x8FA6C0;
var LC_COLOR_TINY = 0x6E88A8;

var LC_APPS = [
  { name: 'MUSIC',      tag: 'PLAYER',   top: 0x4B3BB0, bot: 0x171132, accent: 0xA897FF, kind: 1 },
  { name: 'GALLERY',    tag: 'PHOTOS',   top: 0xC26A38, bot: 0x3A1C10, accent: 0xFFC49B, kind: 2 },
  { name: 'WEATHER',    tag: 'LIVE',     top: 0x2F6FA0, bot: 0x10222F, accent: 0x86DBFF, kind: 3 },
  { name: 'SETTINGS',   tag: 'SYSTEM',   top: 0x454B57, bot: 0x14161B, accent: 0xC6CEDA, kind: 4 },
  { name: 'CALENDAR',   tag: 'UTILITY',  top: 0x8E3A34, bot: 0x2A100E, accent: 0xFF8B7A, kind: 5 },
  { name: 'VOICE NOTE', tag: 'RECORDER', top: 0xA03A78, bot: 0x33102A, accent: 0xFF9BD2, kind: 6 },
  { name: 'WHATSAPP',   tag: 'SOCIAL',   top: 0x1F9E5C, bot: 0x0B2C1A, accent: 0x5CF09B, kind: 7 },
  { name: '2048',       tag: 'PUZZLE',   top: 0x9A7420, bot: 0x2A1E0C, accent: 0xFFD166, kind: 8 },
  { name: 'JUICE RUSH', tag: 'ARCADE',   top: 0x1F8B8B, bot: 0x0A2626, accent: 0x7BF0D8, kind: 0 }
];

var LC_APP_NUM = 9;
var LC_APP_2048 = LC_APP_NUM - 2;
var LC_APP_GAME = LC_APP_NUM - 1;

var LC_ICON_DY = 10;

var L = {
  root: null,
  title: null,
  sub: null,
  cont: null,
  hint: null,
  cards: [],
  dots: [],
  sel: 0,
  keep: 0,
  dot_x0: 0,
  entering: false,
  toast_timer: 0
};

/* Icons are authored in a 300x300 design space and scaled with the card width,
 * so changing LC_CARD_W scales the whole card without editing coordinates. */
function IU(v) {
  return Math.floor(v * LC_CARD_W / 300);
}

function IY(v) {
  return IU(v) - LC_ICON_DY;
}

function ishape(parent, x, y, w, h, color, opa, radius) {
  return shape(parent, IU(x), IY(y), IU(w), IU(h), color, opa,
               radius < 0 ? RADIUS_CIRCLE : IU(radius));
}

function iring(parent, x, y, d, color, opa, bw) {
  var o = ishape(parent, x, y, d, d, color, OPA_TRANSP, -1);
  o.setStyle('border_width', bw);
  o.setStyle('border_color', color);
  o.setStyle('border_opa', opa);
  return o;
}

function lc_icon_draw(card, kind) {
  var i;
  var box;

  switch (kind) {
  case 0: /* Juice Rush: watermelon. */
    ishape(card, 70, 43, 165, 165, 0x3FA45F, OPA_COVER, -1);
    iring(card, 70, 43, 165, 0x9CF3B6, 90, 4);
    ishape(card, 98, 70, 110, 110, 0xF2546E, OPA_COVER, -1);
    ishape(card, 138, 93, 9, 9, 0x2A0B12, OPA_COVER, -1);
    ishape(card, 170, 81, 9, 9, 0x2A0B12, OPA_COVER, -1);
    ishape(card, 162, 132, 9, 9, 0x2A0B12, OPA_COVER, -1);
    ishape(card, 129, 129, 9, 9, 0x2A0B12, OPA_COVER, -1);
    break;

  case 1: /* Music: vinyl record and note. */
    ishape(card, 76, 48, 154, 154, 0x241C4E, OPA_COVER, -1);
    iring(card, 76, 48, 154, 0xA897FF, 160, 6);
    ishape(card, 134, 106, 39, 39, 0xA897FF, OPA_COVER, -1);
    ishape(card, 186, 53, 9, 83, 0xFFFFFF, OPA_COVER, 4);
    ishape(card, 159, 121, 36, 36, 0xFFFFFF, OPA_COVER, -1);
    break;

  case 2: /* Gallery: photo frame, sun and mountains. */
    box = ishape(card, 65, 51, 176, 149, 0xFFB39A, OPA_TRANSP, 15);
    box.setStyle('border_width', 6);
    box.setStyle('border_color', 0xFFB39A);
    box.setStyle('border_opa', 200);
    ishape(card, 87, 73, 35, 35, 0xFFD166, OPA_COVER, -1);
    ishape(card, 84, 131, 77, 61, 0xFF8A6B, OPA_COVER, 39);
    ishape(card, 142, 142, 88, 50, 0xFFC6A8, OPA_COVER, 25);
    break;

  case 3: /* Weather: sun and cloud. */
    ishape(card, 142, 43, 83, 83, 0xFFD166, 230, -1);
    ishape(card, 71, 136, 66, 66, 0xFFFFFF, 235, -1);
    ishape(card, 106, 119, 83, 83, 0xFFFFFF, 235, -1);
    ishape(card, 159, 139, 61, 61, 0xFFFFFF, 235, -1);
    ishape(card, 76, 165, 132, 40, 0xFFFFFF, 235, 20);
    break;

  case 4: /* Settings: gear made of a ring plus teeth. */
    var tooth = [[63, 0], [32, 55], [-32, 55], [-63, 0], [-32, -55], [32, -55]];
    iring(card, 90, 62, 127, 0xC6CEDA, 200, 20);
    for (i = 0; i < 6; i++) {
      ishape(card, 153 + tooth[i][0] - 11, 125 + tooth[i][1] - 11, 22, 22,
             0xC6CEDA, 210, -1);
    }
    break;

  case 5: /* Calendar: white body, red header and month/day labels. */
    /* Hangers are drawn first so the card body covers their lower half. */
    ishape(card, 104, 34, 10, 25, 0x8A93A5, OPA_COVER, 5);
    ishape(card, 193, 34, 10, 25, 0x8A93A5, OPA_COVER, 5);

    ishape(card, 70, 48, 165, 165, 0xFFFFFF, OPA_COVER, 29);

    /* Red header: rounded top corners, then a square block fills the bottom. */
    ishape(card, 70, 48, 165, 68, 0xE5484D, OPA_COVER, 29);
    ishape(card, 70, 81, 165, 35, 0xE5484D, OPA_COVER, 0);

    ishape(card, 70, 116, 165, 1, 0xE5E8EF, OPA_COVER, 0);

    var m = label(card, 'DEC', 16, 0xFFFFFF, 3);
    m.align('top_mid', 0, IY(71));

    var d = label(card, '20', 32, 0x1F2430, 1);
    d.align('top_mid', 0, IY(150));
    break;

  case 6: /* Voice note: magenta card with a white microphone. */
    ishape(card, 70, 48, 165, 165, 0xA03A78, OPA_COVER, 26);
    ishape(card, 134, 64, 39, 83, 0xFFFFFF, OPA_COVER, 19);
    ishape(card, 149, 139, 9, 25, 0xFFFFFF, OPA_COVER, 4);
    ishape(card, 127, 163, 53, 8, 0xFFFFFF, OPA_COVER, 4);
    break;

  case 7: /* WhatsApp: green circle, speech bubble and handset. */
    ishape(card, 70, 48, 165, 165, 0x25D366, OPA_COVER, -1);
    ishape(card, 105, 82, 97, 97, 0xFFFFFF, OPA_COVER, -1);
    ishape(card, 85, 152, 25, 25, 0xFFFFFF, OPA_COVER, -1);

    /* The handset is assembled from upright shapes instead of
     * transform_rotation: software fill ignores the matrix and only takes the
     * refr_obj_matrix() path, whose inflated clip area can produce a NULL
     * destination pointer. Avoid transforms here. */
    ishape(card, 143, 101, 20, 59, 0x25D366, OPA_COVER, 10);
    ishape(card, 138, 98, 31, 19, 0x25D366, OPA_COVER, 9);
    ishape(card, 138, 145, 31, 19, 0x25D366, OPA_COVER, 9);
    break;

  case 8: /* 2048: gold tile with the game name. */
    box = ishape(card, 73, 51, 160, 160, 0x2A1E0C, OPA_COVER, 24);
    box.setStyle('border_width', 3);
    box.setStyle('border_color', 0xFFD166);
    var t = label(box, '2048', 32, 0xFFD166, 2);
    t.center();
    break;
  }
}

function lc_build_bg() {
  var glow_d = [520, 360, 226];
  var glow_c = [0x27456F, 0x31588A, 0x3D6BA5];
  var glow_o = [30, 34, 40];
  var star = [[46, 104, 4], [392, 126, 4], [126, 218, 3], [324, 252, 4],
              [71, 327, 3], [370, 378, 3], [181, 80, 3], [269, 411, 3]];
  var i;

  /* Three-layer top glow. */
  for (i = 0; i < 3; i++) {
    circle(L.root, (W - glow_d[i]) / 2, 110 - glow_d[i] / 2, glow_d[i],
           glow_c[i], glow_o[i]);
  }

  /* Static star dots placed in the gaps between cards. */
  for (i = 0; i < 8; i++) {
    circle(L.root, star[i][0], star[i][1], star[i][2], 0xBFD8FF, 110);
  }
}

function lc_build_header() {
  var l = label(L.root, 'APPLICATIONS', 16, LC_COLOR_TINY, 8);
  l.align('top_mid', 0, 18);

  L.title = label(L.root, LC_APPS[0].name, 24, 0xFFFFFF, 2);
  L.title.align('top_mid', 0, 40);

  L.sub = label(L.root, LC_APPS[0].tag, 16, LC_COLOR_DIM, 4);
  L.sub.align('top_mid', 0, 78);
}

function lc_build_footer() {
  var panel = shape(L.root, 0, 436, W, H - 436, 0x0B1421, 215, 0);
  panel.setStyle('border_width', 2);
  panel.setStyle('border_color', 0x24334A);
  panel.setStyle('border_side', 2);   /* LV_BORDER_SIDE_TOP */

  L.hint = label(L.root, LC_HINT_TEXT, 12, LC_COLOR_DIM, 2);
  L.hint.align('top_mid', 0, 442);

  var l = label(L.root, 'LVGL v9  |  BK7259', 12, 0x4F6785, 2);
  l.align('top_mid', 0, 462);
}

function lc_apply_card(i, on) {
  var dw = on ? 22 : LC_DOT_H;
  var card = L.cards[i];
  var dot = L.dots[i];

  /* Selected card: full opacity, no offset, accent border. */
  card.setStyle('opa', on ? OPA_COVER : 130);
  card.setStyle('translate_y', on ? 0 : 10);
  card.setStyle('border_width', on ? 3 : 2);
  card.setStyle('border_color', on ? LC_APPS[i].accent : 0x3A5580);

  /* Selected dot grows and brightens. */
  dot.setSize(dw, LC_DOT_H);
  dot.setPos(L.dot_x0 + i * LC_DOT_SLOT + (LC_DOT_SLOT - dw) / 2, 386);
  dot.setStyle('bg_color', on ? 0xFFFFFF : 0x4A6485);
}

function lc_set_sel(idx) {
  var old = L.sel;

  if (idx === old) {
    return;
  }
  L.sel = idx;
  L.keep = idx;
  lc_apply_card(old, false);
  lc_apply_card(idx, true);

  L.title.setText(LC_APPS[idx].name);
  L.sub.setText(LC_APPS[idx].tag);
}

function lc_on_scroll() {
  var step = LC_CARD_W + LC_CARD_GAP;
  var sx = L.cont.call('lv_obj_get_scroll_x');
  var idx = Math.floor((sx + step / 2) / step);

  if (idx < 0) {
    idx = 0;
  } else if (idx >= LC_APP_NUM) {
    idx = LC_APP_NUM - 1;
  }

  if (idx !== L.sel) {
    lc_set_sel(idx);
  }
}

function lc_toast() {
  L.hint.setText('THIS APP IS COMING SOON');
  L.hint.setStyle('text_color', 0xFFD54A);
  if (L.toast_timer) {
    timer.clear(L.toast_timer);
  }
  L.toast_timer = timer.setTimeout(function () {
    L.toast_timer = 0;
    L.hint.setText(LC_HINT_TEXT);
    L.hint.setStyle('text_color', LC_COLOR_DIM);
  }, LC_TOAST_MS);
}

function lc_open(idx) {
  if (L.entering) {
    return;
  }

  /* Tapping a side card only scrolls it to center. */
  if (idx !== L.sel) {
    lc_set_sel(idx);
    L.cont.call('lv_obj_scroll_to_x', idx * (LC_CARD_W + LC_CARD_GAP), 1);
    return;
  }

  if (idx === LC_APP_GAME) {
    L.entering = true;
    showJuice();
    return;
  }

  if (idx === LC_APP_2048) {
    L.entering = true;
    show2048();
    return;
  }

  lc_toast();
}

function lc_build_card(idx) {
  var a = LC_APPS[idx];
  var card = lv.new('obj', L.cont, {
    x: idx * (LC_CARD_W + LC_CARD_GAP), y: 0,
    w: LC_CARD_W, h: LC_CARD_H
  });

  card.call('lv_obj_remove_style_all');
  card.flag('scrollable', false);
  card.flag('clickable', true);
  card.setStyle('radius', 26);
  card.setStyle('bg_opa', OPA_COVER);
  card.setStyle('bg_color', a.top);
  card.setStyle('bg_grad_color', a.bot);
  card.setStyle('bg_grad_dir', 1);    /* LV_GRAD_DIR_VER */
  card.setStyle('border_width', 2);
  card.setStyle('border_color', 0x3A5580);
  card.on('clicked', function () {
    lc_open(idx);
  });
  L.cards[idx] = card;

  /* Top-right index, also used as the page indicator. */
  var no = idx + 1 < 10 ? '0' + (idx + 1) : '' + (idx + 1);
  var total = LC_APP_NUM < 10 ? '0' + LC_APP_NUM : '' + LC_APP_NUM;
  var l = label(card, 'APP ' + no + ' / ' + total, 12, a.accent, 2);
  l.align('top_right', -14, 10);

  lc_icon_draw(card, a.kind);

  /* Accent bar, name and category. */
  var bar = shape(card, (LC_CARD_W - 66) / 2, 0, 66, 4, a.accent, 220, 2);
  bar.align('bottom_mid', 0, -70);

  l = label(card, a.name, 20, 0xFFFFFF, 1);
  l.align('bottom_mid', 0, -36);

  l = label(card, a.tag, 12, LC_COLOR_DIM, 3);
  l.align('bottom_mid', 0, -14);
}

function lc_build_carousel() {
  var pad_x = (W - LC_CARD_W) / 2;
  var i;

  L.cont = lv.new('obj', L.root, { x: 0, y: 104, w: W, h: LC_CARD_H });
  L.cont.call('lv_obj_remove_style_all');
  L.cont.flag('scrollable', true);
  /* No SCROLL_ONE: momentum scrolling lets a fast swipe pass several cards,
   * then CENTER snap settles on the nearest one. */
  L.cont.call('lv_obj_set_scroll_dir', 3);          /* LV_DIR_HOR */
  L.cont.call('lv_obj_set_scroll_snap_x', 3);       /* LV_SCROLL_SNAP_CENTER */
  L.cont.call('lv_obj_set_scrollbar_mode', 0);      /* LV_SCROLLBAR_MODE_OFF */
  L.cont.setStyle('bg_opa', OPA_TRANSP);
  L.cont.setStyle('border_width', 0);
  L.cont.setStyle('radius', 0);
  L.cont.setStyle('pad_left', pad_x);
  L.cont.setStyle('pad_right', pad_x);
  L.cont.setStyle('pad_top', 0);
  L.cont.setStyle('pad_bottom', 0);
  L.cont.on('scroll', lc_on_scroll);

  for (i = 0; i < LC_APP_NUM; i++) {
    lc_build_card(i);
  }
}

function lc_build_dots() {
  var i;

  L.dot_x0 = (W - LC_APP_NUM * LC_DOT_SLOT) / 2;
  for (i = 0; i < LC_APP_NUM; i++) {
    L.dots[i] = circle(L.root,
                       L.dot_x0 + i * LC_DOT_SLOT + (LC_DOT_SLOT - LC_DOT_H) / 2,
                       386, LC_DOT_H, 0x4A6485, OPA_COVER);
  }
}

function launcher_stop() {
  if (L.toast_timer) {
    timer.clear(L.toast_timer);
    L.toast_timer = 0;
  }
  if (L.root) {
    L.root.delete();
  }
  L.root = null;
  L.title = null;
  L.sub = null;
  L.cont = null;
  L.hint = null;
  L.cards = [];
  L.dots = [];
  L.sel = 0;
  L.entering = false;
}

function launcher_start() {
  var i;

  launcher_stop();

  lv.screen().flag('scrollable', false);

  L.root = lv.new('obj', lv.screen(), { x: 0, y: 0, w: W, h: H });
  L.root.call('lv_obj_remove_style_all');
  L.root.flag('scrollable', false);
  L.root.flag('clickable', false);
  L.root.setStyle('bg_opa', OPA_COVER);
  L.root.setStyle('bg_color', LC_COLOR_BG_TOP);
  L.root.setStyle('bg_grad_color', LC_COLOR_BG_BOT);
  L.root.setStyle('bg_grad_dir', 1);

  lc_build_bg();
  lc_build_header();
  lc_build_carousel();
  lc_build_dots();
  lc_build_footer();

  /* Restore the last selected card, so HOME does not jump back to the first. */
  L.sel = L.keep;
  if (L.sel < 0) {
    L.sel = 0;
  } else if (L.sel >= LC_APP_NUM) {
    L.sel = LC_APP_NUM - 1;
  }

  for (i = 0; i < LC_APP_NUM; i++) {
    lc_apply_card(i, i === L.sel);
  }
  L.title.setText(LC_APPS[L.sel].name);
  L.sub.setText(LC_APPS[L.sel].tag);

  L.cont.call('lv_obj_update_layout');
  L.cont.call('lv_obj_scroll_to_x', L.sel * (LC_CARD_W + LC_CARD_GAP), 0);
}
/*
 * 2048 puzzle game.
 *
 * Swipe up/down/left/right to slide all tiles; equal neighbours merge and add
 * to the score. Every effective move spawns a 2 (10% chance of a 4). The game
 * ends when the board is full and no merge is possible.
 *
 * The 16 tiles are created once and only moved/restyled, so the object count
 * and the JS handle count stay constant during play.
 */

var G_COLS = 4;
var G_CELL = 64;
var G_GAP = 8;
var G_BOARD_W = G_COLS * G_CELL + (G_COLS - 1) * G_GAP;   /* 280 */
var G_BOARD_X = (W - G_BOARD_W) / 2;                      /* 100 */
var G_BOARD_Y = 72;
var G_SWIPE_MIN = 60;

var G_INK = 0x776E65;
var G_INK_LIGHT = 0xF9F6F2;
var G_BOARD = 0xBBADA0;
var G_SLOT = 0xCDC1B4;
var G_PAPER = 0xFAF8EF;

var G = {
  root: null,
  ui: null,
  touch: null,
  board: null,
  lbl_score: null,
  lbl_best: null,
  lbl_hint: null,
  overlay: null,
  tiles: [],
  font: [],
  b: [],
  score: 0,
  best: 0,
  over: false,
  won: false,
  press_x: 0,
  press_y: 0
};

function g_tile_bg(exp) {
  var color = [0x000000,
               0xEEE4DA, 0xEDE0C8, 0xF2B179, 0xF59563, 0xF67C5F,
               0xF65E3B, 0xEDCF72, 0xEDCC61, 0xEDC850, 0xEDC53F,
               0xEDC22E, 0x3C3A32, 0x3C3A32, 0x3C3A32, 0x3C3A32];

  if (exp >= color.length) {
    exp = color.length - 1;
  }
  return color[exp];
}

function g_tile_font(exp) {
  if (exp <= 6) {
    return 40;
  }
  if (exp <= 9) {
    return 32;
  }
  return 24;
}

function g_get(dir, line, pos) {
  switch (dir) {
  case 0:  return G.b[line][pos];
  case 1:  return G.b[line][G_COLS - 1 - pos];
  case 2:  return G.b[pos][line];
  default: return G.b[G_COLS - 1 - pos][line];
  }
}

function g_set(dir, line, pos, v) {
  switch (dir) {
  case 0:  G.b[line][pos] = v; break;
  case 1:  G.b[line][G_COLS - 1 - pos] = v; break;
  case 2:  G.b[pos][line] = v; break;
  default: G.b[G_COLS - 1 - pos][line] = v; break;
  }
}

/* Compress and merge one line toward index 0. Returns the merge gain, or -1
 * when nothing moved. */
function g_slide_line(v) {
  var tmp = [0, 0, 0, 0];
  var res = [0, 0, 0, 0];
  var i;
  var n = 0;
  var r = 0;
  var gain = 0;
  var changed = false;

  for (i = 0; i < G_COLS; i++) {
    if (v[i]) {
      tmp[n++] = v[i];
    }
  }

  for (i = 0; i < n; i++) {
    if (i + 1 < n && tmp[i] === tmp[i + 1]) {
      res[r] = tmp[i] + 1;
      gain += 1 << res[r];
      r++;
      i++;      /* Skip the merged tile. */
    } else {
      res[r++] = tmp[i];
    }
  }

  for (i = 0; i < G_COLS; i++) {
    if (v[i] !== res[i]) {
      changed = true;
      v[i] = res[i];
    }
  }

  return changed ? gain : -1;
}

function g_can_move() {
  var r, c;

  for (r = 0; r < G_COLS; r++) {
    for (c = 0; c < G_COLS; c++) {
      if (G.b[r][c] === 0) {
        return true;
      }
      if (c + 1 < G_COLS && G.b[r][c] === G.b[r][c + 1]) {
        return true;
      }
      if (r + 1 < G_COLS && G.b[r][c] === G.b[r + 1][c]) {
        return true;
      }
    }
  }
  return false;
}

function g_spawn() {
  var empty = [];
  var r, c, pick;

  for (r = 0; r < G_COLS; r++) {
    for (c = 0; c < G_COLS; c++) {
      if (G.b[r][c] === 0) {
        empty.push(r * G_COLS + c);
      }
    }
  }
  if (empty.length === 0) {
    return;
  }

  pick = empty[rnd(0, empty.length - 1)];
  /* 90% chance of 2, 10% chance of 4. */
  G.b[Math.floor(pick / G_COLS)][pick % G_COLS] = (rnd(0, 9) === 0) ? 2 : 1;
}

function g_render() {
  var r, c, i = 0;

  for (r = 0; r < G_COLS; r++) {
    for (c = 0; c < G_COLS; c++) {
      var v = G.b[r][c];
      var t = G.tiles[i];
      var f;

      i++;
      if (v === 0) {
        hide(t.obj);
        continue;
      }

      show(t.obj);
      t.obj.setPos(G_BOARD_X + c * (G_CELL + G_GAP),
                   G_BOARD_Y + r * (G_CELL + G_GAP));
      t.obj.setStyle('bg_color', g_tile_bg(v));
      /* Classic palette: dark brown text on 2/4, off-white on the rest. */
      t.lbl.setStyle('text_color', v <= 2 ? G_INK : G_INK_LIGHT);

      f = g_tile_font(v);
      if (G.font[i - 1] !== f) {
        G.font[i - 1] = f;
        t.lbl.setFont(f);
      }
      t.lbl.setText('' + (1 << v));
    }
  }

  G.lbl_score.setText('' + G.score);
  G.lbl_best.setText('' + G.best);
}

function g_btn_restart() {
  g_new_game();
}

function g_btn_home() {
  showLauncher();
}

function g_new_game() {
  var i;

  G.b = [];
  for (i = 0; i < G_COLS; i++) {
    G.b.push([0, 0, 0, 0]);
  }
  G.score = 0;
  G.over = false;
  G.won = false;

  if (G.overlay) {
    G.overlay.delete();
    G.overlay = null;
  }
  G.lbl_hint.setText('SLIDE UP / DOWN / LEFT / RIGHT  -  EQUAL TILES MERGE');
  G.lbl_hint.setStyle('text_color', 0x8F7A66);

  g_spawn();
  g_spawn();
  g_render();
}

function g_button(parent, x, y, w, h, text, color, radius, letter_space) {
  var btn = lv.new('button', parent, { x: x, y: y, w: w, h: h, text: text });

  btn.setStyle('radius', radius);
  btn.setStyle('bg_color', color);
  btn.setStyle('shadow_width', 0);
  btn.setStyle('text_letter_space', letter_space);
  btn.setFont(16);
  btn.setColor(G_INK_LIGHT);
  return btn;
}

function g_show_over() {
  var l;

  G.over = true;
  if (G.overlay) {
    return;
  }

  /* Semi-transparent mask that also blocks the swipe layer below. */
  G.overlay = lv.new('obj', G.ui, { x: 0, y: 0, w: W, h: H });
  G.overlay.call('lv_obj_remove_style_all');
  G.overlay.flag('scrollable', false);
  G.overlay.flag('clickable', true);
  G.overlay.setStyle('bg_opa', 235);
  G.overlay.setStyle('bg_color', G_PAPER);

  var card = shape(G.overlay, 40, 90, 400, 300, 0xEEE4DA, OPA_COVER, 20);
  card.setStyle('border_width', 3);
  card.setStyle('border_color', G_BOARD);

  l = label(card, 'GAME OVER', 32, G_INK, 2);
  l.align('top_mid', 0, 26);

  l = label(card, 'SCORE   ' + G.score, 16, 0x8F7A66, 2);
  l.align('top_mid', 0, 92);

  l = label(card, 'BEST    ' + G.best, 16, 0x8F7A66, 2);
  l.align('top_mid', 0, 122);

  var b1 = g_button(card, 0, 0, 150, 44, 'RETRY', 0x8F7A66, 12, 2);
  b1.align('bottom_left', 24, -32);
  b1.on('clicked', g_btn_restart);

  var b2 = g_button(card, 0, 0, 150, 44, 'HOME', G_BOARD, 12, 2);
  b2.align('bottom_right', -24, -32);
  b2.on('clicked', g_btn_home);
}

/* Move one step toward dir: 0 left, 1 right, 2 up, 3 down. */
function g_move(dir) {
  var i, j;
  var changed = false;
  var gain = 0;

  if (G.over || G.root === null) {
    return;
  }

  for (i = 0; i < G_COLS; i++) {
    var v = [0, 0, 0, 0];
    var g;

    for (j = 0; j < G_COLS; j++) {
      v[j] = g_get(dir, i, j);
    }
    g = g_slide_line(v);
    if (g >= 0) {
      gain += g;
      changed = true;
    }
    for (j = 0; j < G_COLS; j++) {
      g_set(dir, i, j, v[j]);
    }
  }

  if (!changed) {
    return;
  }

  G.score += gain;
  if (G.score > G.best) {
    G.best = G.score;
  }

  g_spawn();
  g_render();

  /* Show a hint the first time 2048 is reached, then keep playing. */
  if (!G.won) {
    for (i = 0; i < G_COLS && !G.won; i++) {
      for (j = 0; j < G_COLS; j++) {
        if (G.b[i][j] >= 12) {
          G.won = true;
          G.lbl_hint.setText('2048 REACHED!  KEEP GOING');
          break;
        }
      }
    }
  }

  if (!g_can_move()) {
    g_show_over();
  }
}

function g_touch_pressed() {
  var p = lv.touch();

  if (p === null) {
    return;
  }
  G.press_x = p.x;
  G.press_y = p.y;
}

function g_touch_released() {
  var p = lv.touch();
  var dx, dy, adx, ady;

  if (p === null) {
    return;
  }

  dx = p.x - G.press_x;
  dy = p.y - G.press_y;
  adx = Math.abs(dx);
  ady = Math.abs(dy);

  if (adx < G_SWIPE_MIN && ady < G_SWIPE_MIN) {
    return;
  }

  if (adx > ady) {
    g_move(dx > 0 ? 1 : 0);
  } else {
    g_move(dy > 0 ? 3 : 2);
  }
}

function g_build_header() {
  var l = label(G.ui, '2048', 32, G_INK, 1);
  l.setPos(16, 8);

  l = label(G.ui, 'JOIN THE TILES TO REACH 2048!', 12, 0x8F7A66, 1);
  l.setPos(16, 42);

  var box = shape(G.ui, 316, 8, 74, 44, G_BOARD, OPA_COVER, 10);
  l = label(box, 'SCORE', 12, 0xEEE4DA, 2);
  l.align('top_mid', 0, 4);
  G.lbl_score = label(box, '0', 20, 0xFFFFFF, 1);
  G.lbl_score.align('bottom_mid', 0, -4);

  box = shape(G.ui, 398, 8, 74, 44, G_BOARD, OPA_COVER, 10);
  l = label(box, 'BEST', 12, 0xEEE4DA, 2);
  l.align('top_mid', 0, 4);
  G.lbl_best = label(box, '0', 20, 0xFFFFFF, 1);
  G.lbl_best.align('bottom_mid', 0, -4);
}

function g_build_board() {
  var i;

  G.board = shape(G.root, G_BOARD_X - 8, G_BOARD_Y - 8,
                  G_BOARD_W + 16, G_BOARD_W + 16, G_BOARD, OPA_COVER, 16);

  /* Empty slots stay visible when a tile moves away. */
  for (i = 0; i < G_COLS * G_COLS; i++) {
    shape(G.root, G_BOARD_X + (i % G_COLS) * (G_CELL + G_GAP),
          G_BOARD_Y + Math.floor(i / G_COLS) * (G_CELL + G_GAP),
          G_CELL, G_CELL, G_SLOT, OPA_COVER, 8);
  }

  for (i = 0; i < G_COLS * G_COLS; i++) {
    var obj = shape(G.root, 0, 0, G_CELL, G_CELL, 0xEEE4DA, OPA_COVER, 8);
    var lbl = label(obj, '2', 40, G_INK, 1);

    lbl.center();
    hide(obj);
    G.tiles.push({ obj: obj, lbl: lbl });
    G.font.push(40);
  }
}

function g_build_footer() {
  G.lbl_hint = label(G.ui, 'SLIDE UP / DOWN / LEFT / RIGHT - EQUAL TILES MERGE',
                     12, 0x8F7A66, 1);
  G.lbl_hint.align('top_mid', 0, 384);

  var b1 = g_button(G.ui, 64, 406, 160, 34, 'NEW GAME', 0x8F7A66, 10, 1);
  b1.on('clicked', g_btn_restart);

  var b2 = g_button(G.ui, 256, 406, 160, 34, 'HOME', G_BOARD, 10, 1);
  b2.on('clicked', g_btn_home);
}

function game2048_stop() {
  if (G.root) {
    G.root.delete();
  }
  G.root = null;
  G.ui = null;
  G.touch = null;
  G.board = null;
  G.lbl_score = null;
  G.lbl_best = null;
  G.lbl_hint = null;
  G.overlay = null;
  G.tiles = [];
  G.font = [];
  G.b = [];
  G.score = 0;
  G.over = false;
  G.won = false;
}

function game2048_start() {
  game2048_stop();
  lv.call('lv_rand_set_seed', timer.now());

  lv.screen().flag('scrollable', false);

  G.root = lv.new('obj', lv.screen(), { x: 0, y: 0, w: W, h: H });
  G.root.call('lv_obj_remove_style_all');
  G.root.flag('scrollable', false);
  G.root.flag('clickable', false);
  G.root.setStyle('bg_opa', OPA_COVER);
  G.root.setStyle('bg_color', G_PAPER);

  g_build_board();

  /* Swipe layer: above the tiles, below the buttons. */
  G.touch = lv.new('obj', G.root, { x: 0, y: 0, w: W, h: H });
  G.touch.call('lv_obj_remove_style_all');
  G.touch.flag('scrollable', false);
  G.touch.flag('clickable', true);
  G.touch.flag('press_lock', true);
  G.touch.setStyle('bg_opa', OPA_TRANSP);
  G.touch.on('pressed', g_touch_pressed);
  G.touch.on('released', g_touch_released);

  /* UI layer: title, score, buttons and game-over overlay. */
  G.ui = lv.new('obj', G.root, { x: 0, y: 0, w: W, h: H });
  G.ui.call('lv_obj_remove_style_all');
  G.ui.flag('scrollable', false);
  G.ui.flag('clickable', false);

  g_build_header();
  g_build_footer();

  g_new_game();
}
/*
 * Juice Rush: slice-fruit game.
 *
 * Port of the C implementation. Fruit is thrown up from the bottom and sliced
 * by dragging; a missed fruit or a bomb costs one life and the score is
 * settled after 60 s.
 *
 * Software-rendering choices kept from the C version:
 *   1. No image assets, everything is drawn with plain objects.
 *   2. Background decorations are static and stay outside the flight area.
 *   3. Fruit, pieces, particles, popups, slashes and rings are pooled, so the
 *      object and handle count stays constant during play.
 *   4. Sliced halves are same-colour circles to avoid mask/clip costs.
 */

var JR_FRUIT_MAX = 8;
var JR_PIECE_MAX = 16;
var JR_PARTICLE_MAX = 28;
var JR_POPUP_MAX = 4;
var JR_RING_MAX = 4;
var JR_SLASH_MAX = 4;
var JR_TRAIL_MAX = 8;

var JR_TICK_MS = 16;
var JR_GAME_MS = 60000;
var JR_GRAVITY = 900;
var JR_COMBO_WINDOW_MS = 650;
var JR_TRAIL_OPA = 235;
/* A dropped release/press_lost must not leave the blade trail on screen. */
var JR_DRAG_TIMEOUT_MS = 300;

var JR_APEX_TOP_PCT = 28;
var JR_APEX_BOTTOM_PCT = 62;
var JR_APEX_MIN_RISE = 220;

var JR_COLOR_CARD = 0x152238;
var JR_COLOR_CARD2 = 0x0C1626;
var JR_COLOR_LINE = 0x2B3F5C;
var JR_COLOR_DIM = 0x8FA6C0;

var JR_HUD_H = 82;

var JR_STATE_IDLE = 0;
var JR_STATE_START = 1;
var JR_STATE_PLAY = 2;
var JR_STATE_OVER = 3;

/* Outer gradient and pulp colour per fruit kind. */
var JR_KINDS = [
  { top: 0x5ACB6E, bottom: 0x1C7A36, pulp: 0xF2546E },   /* Watermelon */
  { top: 0xFFB648, bottom: 0xE0701A, pulp: 0xFFA24A },   /* Orange */
  { top: 0xFF6B6B, bottom: 0xC21F35, pulp: 0xFFF0D2 },   /* Apple */
  { top: 0xFFE86B, bottom: 0xDEA912, pulp: 0xFFF6B8 },   /* Lemon */
  { top: 0x8B7BFF, bottom: 0x4534C7, pulp: 0xC9BCFF }    /* Blueberry */
];
var JR_KIND_NUM = 5;

var J = {
  root: null,
  bg: null,
  play: null,
  ui: null,
  touch: null,
  overlay: null,
  hud: null,
  lbl_score: null,
  lbl_time: null,
  lbl_combo: null,
  bar_time: null,
  hearts: [],

  trail_obj: null,
  trail: [],
  trail_opa: 0,
  drag: false,
  prev_x: 0,
  prev_y: 0,
  last_touch_tick: 0,

  fruits: [],
  pieces: [],
  particles: [],
  popups: [],
  rings: [],
  slashes: [],

  timer: 0,
  state: JR_STATE_IDLE,
  last_tick: 0,
  start_tick: 0,
  next_spawn_tick: 0,
  last_slice_tick: 0,

  score: 0,
  best: 0,
  combo: 0,
  lives: 0,
  max_combo: 0,
  new_record: false,

  last_score_ui: -1,
  last_sec_ui: -1,
  last_pct_ui: -1,
  last_combo_ui: -1
};

/* ---------------------------- Basic drawing ---------------------------- */

/* Plain circle without theme styles; the fuse spark may exceed the bounds. */
function jr_ball(parent) {
  var o = lv.new('obj', parent, { x: 0, y: 0, w: 10, h: 10 });

  o.call('lv_obj_remove_style_all');
  o.flag('clickable', false);
  o.flag('scrollable', false);
  o.flag('overflow_visible', true);
  o.setStyle('radius', RADIUS_CIRCLE);
  o.setStyle('bg_opa', OPA_COVER);
  o.setStyle('bg_grad_dir', 1);        /* LV_GRAD_DIR_VER */
  o.setStyle('border_width', 2);
  return o;
}

function jr_ball_pair(parent) {
  var ball = jr_ball(parent);
  var shine = lv.new('obj', ball, { x: 0, y: 0, w: 10, h: 10 });

  shine.call('lv_obj_remove_style_all');
  shine.flag('clickable', false);
  shine.flag('scrollable', false);
  shine.setStyle('radius', RADIUS_CIRCLE);
  shine.setStyle('bg_color', 0xFFFFFF);
  shine.setStyle('bg_opa', 110);

  hide(ball);
  return { ball: ball, shine: shine };
}

function jr_fruit_setup(f, kind, bomb, r) {
  var d = r * 2;

  f.r = r;
  f.kind = kind;
  f.bomb = bomb;
  f.ball.setSize(d, d);

  if (bomb) {
    f.ball.setStyle('bg_color', 0x2B3242);
    f.ball.setStyle('bg_grad_color', 0x11151F);
    f.ball.setStyle('border_width', 4);
    f.ball.setStyle('border_color', 0xFF5252);
    /* The highlight slot becomes the red fuse spark. */
    f.shine.setSize(Math.floor(d * 20 / 100), Math.floor(d * 20 / 100));
    f.shine.setPos(Math.floor(d * 40 / 100), -Math.floor(d * 4 / 100));
    f.shine.setStyle('bg_color', 0xFF7A45);
    f.shine.setStyle('bg_opa', OPA_COVER);
  } else {
    var k = JR_KINDS[kind];

    f.ball.setStyle('bg_color', k.top);
    f.ball.setStyle('bg_grad_color', k.bottom);
    f.ball.setStyle('border_width', 2);
    f.ball.setStyle('border_color', k.bottom);
    f.shine.setSize(Math.floor(d * 28 / 100), Math.floor(d * 28 / 100));
    f.shine.setPos(Math.floor(d * 17 / 100), Math.floor(d * 15 / 100));
    f.shine.setStyle('bg_color', 0xFFFFFF);
    f.shine.setStyle('bg_opa', 105);
  }
}

/* Decorative fruit for the start page; not pooled. */
function jr_deco_fruit(parent, cx, cy, r, kind) {
  var pair = jr_ball_pair(parent);
  var f = { ball: pair.ball, shine: pair.shine };

  jr_fruit_setup(f, kind, false, r);
  pair.ball.setPos(cx - r, cy - r);
  show(pair.ball);
}

function jr_soft_dot(parent, d, color, opa) {
  return circle(parent, 0, 0, d, color, opa);
}

/* Static background decoration, kept away from the main flight area. */
function jr_build_bg() {
  var glow_d = [430, 320, 220];
  var glow_c = [0x2A477A, 0x35588F, 0x4169A8];
  var glow_o = [34, 38, 44];
  var glow_cy = 96;
  var i;

  J.bg = lv.new('obj', J.root, { x: 0, y: 0, w: W, h: H });
  J.bg.call('lv_obj_remove_style_all');
  J.bg.flag('clickable', false);
  J.bg.flag('scrollable', false);

  /* Top glow: concentric circles fake a radial gradient. Created before the
   * HUD panel so the translucent glow does not wash it out. */
  for (i = 0; i < 3; i++) {
    var g = jr_soft_dot(J.bg, glow_d[i], glow_c[i], glow_o[i]);
    g.setPos(W / 2 - glow_d[i] / 2, glow_cy - glow_d[i] / 2);
  }

  /* Top HUD panel: translucent dark fill plus a bottom highlight line. */
  var panel = shape(J.bg, 0, 0, W, JR_HUD_H, 0x0E1B2F, 150, 0);
  panel.setStyle('bg_grad_color', 0x15263E);
  panel.setStyle('bg_grad_dir', 1);
  panel.setStyle('border_width', 2);
  panel.setStyle('border_side', 1);      /* LV_BORDER_SIDE_BOTTOM */
  panel.setStyle('border_color', 0x2A4A72);

  /* Playfield frame: rounded outline with L-shaped corner marks. */
  var fx = 10;
  var fy = JR_HUD_H;
  var fw = W - 20;
  var fh = H - JR_HUD_H - 64;
  var bl = 26;
  var bt = 3;
  var bi = 12;
  var frame = shape(J.bg, fx, fy, fw, fh, 0x000000, OPA_TRANSP, 14);

  frame.setStyle('border_width', 2);
  frame.setStyle('border_color', 0x2A4A72);
  frame.setStyle('border_opa', 150);

  for (i = 0; i < 4; i++) {
    var x = (i & 1) ? (fx + fw - bi - bl) : (fx + bi);
    var y = (i & 2) ? (fy + fh - bi - bl) : (fy + bi);
    var hx = x;
    var hy = (i & 2) ? (y + bl - bt) : y;
    var vx = (i & 1) ? (x + bl - bt) : x;

    var ha = shape(J.bg, hx, hy, bl, bt, 0x5E9FE0, 220, 2);
    var va = shape(J.bg, vx, y, bt, bl, 0x5E9FE0, 220, 2);

    ha.setStyle('radius', 2);
    va.setStyle('radius', 2);
  }

  /* Ground strip so fruit appears to launch from the floor. */
  var ground = shape(J.bg, 0, H - 64, W, 64, 0x0B1526, OPA_COVER, 0);

  ground.setStyle('bg_grad_color', 0x05080E);
  ground.setStyle('bg_grad_dir', 1);
  ground.setStyle('border_width', 3);
  ground.setStyle('border_side', 2);      /* LV_BORDER_SIDE_TOP */
  ground.setStyle('border_color', 0x21406B);

  /* Three juice puddles on the ground; only the upper half is visible. */
  var dome_c = [0x7A3E8F, 0xC2622A, 0x2E7A57];
  var dome_x = [24, 50, 76];
  var dome_r = [46, 36, 42];

  for (i = 0; i < 3; i++) {
    var dome = jr_soft_dot(J.bg, dome_r[i] * 2, dome_c[i], 60);
    dome.setPos(W * dome_x[i] / 100 - dome_r[i], H - 18 - dome_r[i]);
  }

  /* Stars in the upper half. */
  for (i = 0; i < 16; i++) {
    var d = rnd(3, 6);
    var sx = rnd(20, W - 20);
    var sy = rnd(100, 340);
    var st = jr_soft_dot(J.bg, d, 0x4E79B8, rnd(90, 190));

    st.setPos(sx, sy);
  }
}

/* Shared rounded button. Labels inside buttons are centred by the binding. */
function jr_button(parent, txt, w, h) {
  var btn = lv.new('button', parent, { x: 0, y: 0, w: w, h: h, text: txt });

  btn.setStyle('radius', RADIUS_CIRCLE);
  btn.setStyle('bg_color', 0x3ED47A);
  btn.setStyle('bg_grad_color', 0x1B9C52);
  btn.setStyle('bg_grad_dir', 1);
  btn.setStyle('shadow_width', 24);
  btn.setStyle('shadow_color', 0x0A5A2C);
  btn.setStyle('shadow_opa', 153);        /* LV_OPA_60 */
  btn.setStyle('shadow_offset_y', 8);
  btn.setFont(20);
  btn.setColor(0xFFFFFF);
  return btn;
}

function jr_button_secondary(btn) {
  btn.setStyle('bg_color', 0x24394F);
  btn.setStyle('bg_grad_color', 0x16263C);
  btn.setStyle('shadow_color', 0x0A1420);
}

/* ---------------------------- Blade trail ---------------------------- */

function jr_trail_clear() {
  J.trail = [];
  J.trail_opa = 0;
  hide(J.trail_obj);
}

function jr_trail_push(x, y) {
  if (J.trail.length >= JR_TRAIL_MAX) {
    J.trail.shift();
  }
  J.trail.push({ x: x, y: y });
}

function jr_trail_refresh(opa) {
  var minx = 1e9, miny = 1e9, maxx = -1e9, maxy = -1e9;
  var pad = 12;
  var i;

  if (!J.trail_obj) {
    return;
  }
  if (J.trail.length < 2 || opa === 0) {
    hide(J.trail_obj);
    return;
  }

  for (i = 0; i < J.trail.length; i++) {
    if (J.trail[i].x < minx) { minx = J.trail[i].x; }
    if (J.trail[i].x > maxx) { maxx = J.trail[i].x; }
    if (J.trail[i].y < miny) { miny = J.trail[i].y; }
    if (J.trail[i].y > maxy) { maxy = J.trail[i].y; }
  }

  var x = minx - pad;
  var y = miny - pad;
  var w = maxx - minx + pad * 2;
  var h = maxy - miny + pad * 2;
  var pts = [];

  for (i = 0; i < J.trail.length; i++) {
    pts.push(J.trail[i].x - x, J.trail[i].y - y);
  }

  J.trail_obj.setPos(x, y);
  J.trail_obj.setSize(w, h);
  lv.setLinePoints(J.trail_obj, pts);
  J.trail_obj.setStyle('line_opa', opa);
  show(J.trail_obj);
}

/* ---------------------------- Halves / particles ---------------------------- */

function jr_bit_alloc(pool) {
  var i;

  for (i = 0; i < pool.length; i++) {
    if (!pool[i].used) {
      return pool[i];
    }
  }
  return null;
}

function jr_spawn_piece(x, y, vx, vy, r, color) {
  var p = jr_bit_alloc(J.pieces);

  if (p === null) {
    return;
  }
  p.used = true;
  p.x = x;
  p.y = y;
  p.vx = vx;
  p.vy = vy;
  p.r = r;
  p.ttl = 700;
  p.ttl_max = 700;

  p.obj.setSize(r * 2, r * 2);
  p.obj.setStyle('bg_color', color);
  p.obj.setStyle('bg_grad_color', color);
  p.obj.setStyle('border_color', color);
  p.obj.setStyle('opa', OPA_COVER);
  p.obj.setPos(Math.floor(x - r), Math.floor(y - r));
  show(p.obj);
}

function jr_spawn_particles(x, y, color, count) {
  var i;

  for (i = 0; i < count; i++) {
    var p = jr_bit_alloc(J.particles);
    var r;

    if (p === null) {
      return;
    }
    r = rnd(3, 7);
    p.used = true;
    p.x = x + rnd(0, 20) - 10;
    p.y = y + rnd(0, 20) - 10;
    p.vx = rnd(0, 300) - 150;
    p.vy = rnd(0, 230) - 340;
    p.r = r;
    p.ttl = rnd(420, 680);
    p.ttl_max = p.ttl;

    p.obj.setSize(r * 2, r * 2);
    p.obj.setStyle('bg_color', color);
    p.obj.setStyle('bg_grad_color', color);
    p.obj.setStyle('bg_opa', OPA_COVER);
    p.obj.setStyle('opa', OPA_COVER);
    p.obj.setPos(Math.floor(p.x - r), Math.floor(p.y - r));
    show(p.obj);
  }
}

/* ---------------------------- Cut effects ---------------------------- */

function jr_spawn_ring(x, y, r_start, r_end, color) {
  var i;

  for (i = 0; i < JR_RING_MAX; i++) {
    var p = J.rings[i];

    if (p.used) {
      continue;
    }
    p.used = true;
    p.x = x;
    p.y = y;
    p.r_start = r_start;
    p.r_end = r_end;
    p.ttl = 260;
    p.ttl_max = 260;

    p.obj.setSize(r_start * 2, r_start * 2);
    p.obj.setPos(Math.floor(x - r_start), Math.floor(y - r_start));
    p.obj.setStyle('border_width', 6);
    p.obj.setStyle('border_color', color);
    p.obj.setStyle('opa', 230);
    show(p.obj);
    return;
  }
}

function jr_update_rings(dtm) {
  var i;

  for (i = 0; i < JR_RING_MAX; i++) {
    var p = J.rings[i];
    var k, r;

    if (!p.used) {
      continue;
    }
    if (p.ttl <= dtm) {
      p.used = false;
      hide(p.obj);
      continue;
    }
    p.ttl -= dtm;

    k = 255 - Math.floor(255 * p.ttl / p.ttl_max);
    r = p.r_start + Math.floor((p.r_end - p.r_start) * k / 255);
    p.obj.setSize(r * 2, r * 2);
    p.obj.setPos(Math.floor(p.x - r), Math.floor(p.y - r));
    p.obj.setStyle('border_width', 6 - Math.floor(k * 4 / 255));
    p.obj.setStyle('opa', Math.floor(230 - 230 * k / 255));
  }
}

function jr_norm2(dx, dy) {
  var len2 = dx * dx + dy * dy;
  var len;

  if (len2 < 1) {
    return [0, -1];
  }
  len = Math.sqrt(len2);
  if (len < 1) {
    len = 1;
  }
  return [dx / len, dy / len];
}

/* Slash mark: a short bright line along the drag direction, then fade. */
function jr_spawn_slash(cx, cy, dx, dy, r, color) {
  var n = jr_norm2(dx, dy);
  var len = r * 1.55;
  var x1 = cx - n[0] * len;
  var y1 = cy - n[1] * len;
  var x2 = cx + n[0] * len;
  var y2 = cy + n[1] * len;
  var i;

  for (i = 0; i < JR_SLASH_MAX; i++) {
    var p = J.slashes[i];
    var pad = 10;
    var minx, miny, w, h;

    if (p.used) {
      continue;
    }

    minx = Math.floor(x1 < x2 ? x1 : x2) - pad;
    miny = Math.floor(y1 < y2 ? y1 : y2) - pad;
    w = Math.floor(x1 < x2 ? x2 - x1 : x1 - x2) + pad * 2;
    h = Math.floor(y1 < y2 ? y2 - y1 : y1 - y2) + pad * 2;

    p.used = true;
    p.ttl = 150;
    p.ttl_max = 150;

    p.obj.setPos(minx, miny);
    p.obj.setSize(w, h);
    lv.setLinePoints(p.obj, [Math.floor(x1) - minx, Math.floor(y1) - miny,
                             Math.floor(x2) - minx, Math.floor(y2) - miny]);
    p.obj.setStyle('line_color', color);
    p.obj.setStyle('line_opa', 255);
    show(p.obj);
    return;
  }
}

function jr_update_slashes(dtm) {
  var i;

  for (i = 0; i < JR_SLASH_MAX; i++) {
    var p = J.slashes[i];

    if (!p.used) {
      continue;
    }
    if (p.ttl <= dtm) {
      p.used = false;
      hide(p.obj);
      continue;
    }
    p.ttl -= dtm;
    p.obj.setStyle('line_opa', Math.floor(255 * p.ttl / p.ttl_max));
  }
}

/* ---------------------------- Floating score ---------------------------- */

function jr_popup_alloc(x, y, color) {
  var p = null;
  var i;

  for (i = 0; i < JR_POPUP_MAX; i++) {
    if (!J.popups[i].used) {
      p = J.popups[i];
      break;
    }
  }
  if (p === null) {
    return null;
  }

  p.used = true;
  p.x = x;
  p.y = y;
  p.ttl = 780;
  p.ttl_max = 780;

  p.lbl.setStyle('text_color', color);
  p.lbl.setPos(Math.floor(x - 80), Math.floor(y));
  p.lbl.setStyle('opa', OPA_COVER);
  show(p.lbl);
  return p;
}

function jr_popup_score(x, y, gain, mult) {
  var p = jr_popup_alloc(x, y, mult >= 2 ? 0xFFD54A : 0xFFFFFF);

  if (p === null) {
    return;
  }
  if (mult >= 2) {
    p.lbl.setText('+' + gain + '  x' + mult);
  } else {
    p.lbl.setText('+' + gain);
  }
}

function jr_popup_text(x, y, txt, color) {
  var p = jr_popup_alloc(x, y, color);

  if (p === null) {
    return;
  }
  p.lbl.setText(txt);
}

/* ---------------------------- Fruit logic ---------------------------- */

function jr_fruit_hide(f) {
  f.used = false;
  hide(f.ball);
}

function jr_hearts_update() {
  var i;

  for (i = 0; i < 3; i++) {
    J.hearts[i].setStyle('bg_color', i < J.lives ? 0xFF5B6E : 0x2A3648);
  }
}

function jr_lose_life() {
  if (J.lives > 0) {
    J.lives--;
  }
  jr_hearts_update();
  if (J.lives <= 0) {
    jr_game_over();
  }
}

function jr_fruit_miss(f) {
  var bomb = f.bomb;

  jr_fruit_hide(f);
  if (!bomb) {
    jr_lose_life();
  }
}

function jr_slice_fruit(f, dx, dy) {
  var cx = f.x;
  var cy = f.y;
  var r = f.r;
  var bomb = f.bomb;
  var pulp = bomb ? 0xFF7043 : JR_KINDS[f.kind].pulp;
  var n = jr_norm2(dx, dy);

  jr_fruit_hide(f);

  if (bomb) {
    jr_spawn_ring(cx, cy, Math.floor(r * 4 / 10), Math.floor(r * 30 / 10), 0xFF7A45);
    jr_spawn_slash(cx, cy, n[0], n[1], r, 0xFFD166);
    jr_spawn_particles(cx, cy, 0xFF7043, 12);
    jr_spawn_particles(cx, cy, 0xFFD166, 6);
    jr_popup_text(cx, cy - r, 'BOOM!', 0xFF7A45);
    jr_lose_life();
    return;
  }

  /* Push the halves apart perpendicular to the blade so it looks sliced. */
  var px = -n[1];
  var py = n[0];
  var off = r * 0.30;
  var kick = 120;

  jr_spawn_piece(cx + px * off, cy + py * off,
                 f.vx + px * kick, f.vy + py * kick - 110, Math.floor(r * 62 / 100), pulp);
  jr_spawn_piece(cx - px * off, cy - py * off,
                 f.vx - px * kick, f.vy - py * kick - 18, Math.floor(r * 62 / 100), pulp);
  jr_spawn_particles(cx, cy, pulp, 9);
  jr_spawn_ring(cx, cy, Math.floor(r * 5 / 10), Math.floor(r * 22 / 10), pulp);
  jr_spawn_slash(cx, cy, n[0], n[1], r, 0xFFF4D6);

  if (lv.call('lv_tick_elaps', J.last_slice_tick) < JR_COMBO_WINDOW_MS) {
    J.combo++;
  } else {
    J.combo = 1;
  }
  J.last_slice_tick = lv.call('lv_tick_get');
  if (J.combo > J.max_combo) {
    J.max_combo = J.combo;
  }

  var mult = J.combo > 5 ? 5 : J.combo;
  var gain = 10 * mult;

  J.score += gain;
  jr_popup_score(cx, cy - r, gain, mult);
}

/* Segment-circle intersection test. */
function jr_seg_hits_circle(x1, y1, x2, y2, cx, cy, r) {
  var dx = x2 - x1;
  var dy = y2 - y1;
  var fx = cx - x1;
  var fy = cy - y1;
  var len2 = dx * dx + dy * dy;
  var t = 0;
  var px, py;

  if (len2 > 0.0001) {
    t = (fx * dx + fy * dy) / len2;
    if (t < 0) { t = 0; }
    else if (t > 1) { t = 1; }
  }
  px = x1 + dx * t - cx;
  py = y1 + dy * t - cy;
  return (px * px + py * py) <= (r * r);
}

function jr_check_slice(x1, y1, x2, y2) {
  var i;

  for (i = 0; i < JR_FRUIT_MAX; i++) {
    var f = J.fruits[i];

    if (!f.used) {
      continue;
    }
    if (jr_seg_hits_circle(x1, y1, x2, y2, f.x, f.y, f.r + 10)) {
      jr_slice_fruit(f, x2 - x1, y2 - y1);
    }
  }
}

/* ---------------------------- Spawning ---------------------------- */

function jr_spawn_fruit(bomb) {
  var f = null;
  var r;
  var i;

  for (i = 0; i < JR_FRUIT_MAX; i++) {
    if (!J.fruits[i].used) {
      f = J.fruits[i];
      break;
    }
  }
  if (f === null) {
    return;
  }

  r = rnd(28, 36);
  jr_fruit_setup(f, rnd(0, JR_KIND_NUM - 1), bomb, r);

  f.x = rnd(52, W - 52);
  f.y = H + r + 24;

  /* Pick a random apex, then derive the launch speed from it. */
  var apex_y = rnd(Math.floor(H * JR_APEX_TOP_PCT / 100),
                   Math.floor(H * JR_APEX_BOTTOM_PCT / 100));
  var rise = f.y - apex_y;

  if (rise < JR_APEX_MIN_RISE) {
    rise = JR_APEX_MIN_RISE;
  }
  f.vy = -Math.sqrt(2 * JR_GRAVITY * rise);

  var target_x = rnd(80, W - 80);
  var flight = -2 * f.vy / JR_GRAVITY;
  f.vx = (target_x - f.x) / flight;
  f.used = true;

  f.ball.setPos(Math.floor(f.x - r), Math.floor(f.y - r));
  show(f.ball);
}

/* ---------------------------- Touch ---------------------------- */

function jr_touch_cb(code) {
  var p, dx, dy;

  if (J.state !== JR_STATE_PLAY) {
    return;
  }
  J.last_touch_tick = lv.call('lv_tick_get');
  p = lv.touch();
  if (p === null) {
    return;
  }

  if (code === 0) {                         /* pressed */
    J.drag = true;
    J.prev_x = p.x;
    J.prev_y = p.y;
    J.trail = [];
    J.trail_opa = JR_TRAIL_OPA;
    jr_trail_push(p.x, p.y);
    return;
  }

  if (code === 1) {                         /* pressing */
    if (!J.drag) {
      return;
    }
    jr_trail_push(p.x, p.y);
    J.trail_opa = JR_TRAIL_OPA;
    jr_trail_refresh(J.trail_opa);

    dx = p.x - J.prev_x;
    dy = p.y - J.prev_y;
    /* Require a deliberate drag so a resting finger does not slice fruit. */
    if (dx * dx + dy * dy >= 100) {
      jr_check_slice(J.prev_x, J.prev_y, p.x, p.y);
      J.prev_x = p.x;
      J.prev_y = p.y;
    }
    return;
  }

  /* released (2) or press lost (3) */
  if (J.drag && code === 2) {
    dx = p.x - J.prev_x;
    dy = p.y - J.prev_y;
    /* A fast flick may never emit PRESSING, so also check on release. */
    if (dx * dx + dy * dy >= 100) {
      jr_check_slice(J.prev_x, J.prev_y, p.x, p.y);
    }
  }
  J.drag = false;
}

/* ---------------------------- Per-frame update ---------------------------- */

function jr_update_fruits(dt) {
  var i;

  for (i = 0; i < JR_FRUIT_MAX; i++) {
    var f = J.fruits[i];

    if (!f.used) {
      continue;
    }

    f.vy += JR_GRAVITY * dt;
    f.x += f.vx * dt;
    f.y += f.vy * dt;

    if (f.x < f.r) {
      f.x = f.r;
      f.vx = -f.vx * 0.55;
    } else if (f.x > W - f.r) {
      f.x = W - f.r;
      f.vx = -f.vx * 0.55;
    }

    if (f.vy > 0 && f.y - f.r > H) {
      jr_fruit_miss(f);
      if (J.state !== JR_STATE_PLAY) {
        return;
      }
      continue;
    }
    f.ball.setPos(Math.floor(f.x - f.r), Math.floor(f.y - f.r));
  }
}

function jr_update_bits(pool, dt, dtm, fade) {
  var i;

  for (i = 0; i < pool.length; i++) {
    var p = pool[i];
    var o;

    if (!p.used) {
      continue;
    }

    if (p.ttl <= dtm) {
      p.used = false;
      hide(p.obj);
      continue;
    }
    p.ttl -= dtm;

    p.vy += JR_GRAVITY * dt;
    p.x += p.vx * dt;
    p.y += p.vy * dt;
    p.obj.setPos(Math.floor(p.x - p.r), Math.floor(p.y - p.r));

    if (fade) {
      o = Math.floor(255 * p.ttl / p.ttl_max);
      if (o < 12) {
        o = 12;
      }
      p.obj.setStyle('opa', o);
    }
  }
}

function jr_update_popups(dtm) {
  var i;

  for (i = 0; i < JR_POPUP_MAX; i++) {
    var p = J.popups[i];
    var k;

    if (!p.used) {
      continue;
    }
    if (p.ttl <= dtm) {
      p.used = false;
      hide(p.lbl);
      continue;
    }
    p.ttl -= dtm;

    k = 255 - Math.floor(255 * p.ttl / p.ttl_max);
    p.lbl.setPos(Math.floor(p.x - 80), Math.floor(p.y - k * 0.35));
    p.lbl.setStyle('opa', Math.floor(255 * p.ttl / p.ttl_max));
  }
}

function jr_update_trail(dtm) {
  var dec;

  if (J.drag) {
    return;
  }
  if (J.trail_opa === 0) {
    return;
  }

  dec = dtm * 2;
  if (J.trail_opa <= dec) {
    J.trail_opa = 0;
    jr_trail_clear();
    return;
  }
  J.trail_opa -= dec;
  jr_trail_refresh(J.trail_opa);
}

function jr_pad2(v) {
  return v < 10 ? '0' + v : '' + v;
}

function jr_hud_update(force) {
  var elapsed, remain, sec, pct, combo_show;

  if (J.state !== JR_STATE_PLAY) {
    return;
  }

  if (force || J.score !== J.last_score_ui) {
    J.lbl_score.setText('' + J.score);
    J.last_score_ui = J.score;
  }

  elapsed = lv.call('lv_tick_elaps', J.start_tick);
  remain = (elapsed >= JR_GAME_MS) ? 0 : (JR_GAME_MS - elapsed);
  sec = Math.floor((remain + 999) / 1000);
  pct = Math.floor(remain / (JR_GAME_MS / 100));

  if (force || pct !== J.last_pct_ui) {
    lv.call('lv_bar_set_value', J.bar_time, pct, 0);   /* LV_ANIM_OFF */
    J.last_pct_ui = pct;
  }

  if (force || sec !== J.last_sec_ui) {
    J.lbl_time.setText(Math.floor(sec / 60) + ':' + jr_pad2(sec % 60));
    if (sec > 20) {
      J.bar_time.setStyle('bg_color', 0x1B9C52, 'indicator');
      J.bar_time.setStyle('bg_grad_color', 0x6EF2A0, 'indicator');
      J.lbl_time.setStyle('text_color', 0xFFFFFF);
    } else if (sec > 8) {
      J.bar_time.setStyle('bg_color', 0xE08A12, 'indicator');
      J.bar_time.setStyle('bg_grad_color', 0xFFD166, 'indicator');
      J.lbl_time.setStyle('text_color', 0xFFC145);
    } else {
      J.bar_time.setStyle('bg_color', 0xC21F35, 'indicator');
      J.bar_time.setStyle('bg_grad_color', 0xFF7A7A, 'indicator');
      J.lbl_time.setStyle('text_color', 0xFF5B6E);
    }
    J.last_sec_ui = sec;
  }

  /* Hide the combo hint once its window expires. */
  combo_show = 0;
  if (J.combo >= 2 && lv.call('lv_tick_elaps', J.last_slice_tick) < 1500) {
    combo_show = J.combo;
  }
  if (force || combo_show !== J.last_combo_ui) {
    if (combo_show >= 2) {
      J.lbl_combo.setText('COMBO  x' + combo_show);
      J.lbl_combo.setStyle('text_color', combo_show >= 5 ? 0xFFD54A :
                           (combo_show >= 3 ? 0x6FE39A : 0x9FC3E8));
      show(J.lbl_combo);
    } else {
      hide(J.lbl_combo);
    }
    J.last_combo_ui = combo_show;
  }
}

function jr_update_play(now, dt) {
  var elapsed = lv.call('lv_tick_elaps', J.start_tick);
  var sec, interval, groups, i, bomb_idx, bomb_pct;

  if (elapsed >= JR_GAME_MS) {
    jr_game_over();
    return;
  }

  sec = Math.floor(elapsed / 1000);
  /* No bombs in the first 5 s; afterwards the chance ramps up quickly. */
  bomb_pct = 18 + Math.floor(sec / 2);
  if (bomb_pct > 48) {
    bomb_pct = 48;
  }
  groups = 1;
  if (sec > 18 && rnd(0, 99) < 35) {
    groups = 2;
  }
  if (sec > 38 && rnd(0, 99) < 30) {
    groups = 3;
  }

  if (now >= J.next_spawn_tick) {
    bomb_idx = -1;
    if (sec >= 5 && rnd(0, 99) < bomb_pct) {
      bomb_idx = rnd(0, groups - 1);
    }
    for (i = 0; i < groups; i++) {
      jr_spawn_fruit(i === bomb_idx);
    }
    interval = 1150 - sec * 11;
    if (interval < 560) {
      interval = 560;
    }
    J.next_spawn_tick = now + interval + rnd(0, 200);
  }

  jr_update_fruits(dt);
  if (J.state !== JR_STATE_PLAY) {
    return;
  }
  jr_hud_update(false);
}

function jr_tick() {
  var now = lv.call('lv_tick_get');
  var dtm = lv.call('lv_tick_elaps', J.last_tick);
  var dt;

  if (dtm === 0) {
    return;
  }
  if (dtm > 120) {
    dtm = 120;      /* Clamp long frames so objects cannot tunnel. */
  }
  J.last_tick = now;
  dt = dtm / 1000;

  if (J.drag && lv.call('lv_tick_elaps', J.last_touch_tick) > JR_DRAG_TIMEOUT_MS) {
    J.drag = false;
    jr_trail_clear();
  }

  if (J.state === JR_STATE_PLAY) {
    jr_update_play(now, dt);
  }
  jr_update_bits(J.pieces, dt, dtm, true);
  jr_update_bits(J.particles, dt, dtm, true);
  jr_update_rings(dtm);
  jr_update_slashes(dtm);
  jr_update_popups(dtm);
  jr_update_trail(dtm);
}

/* ---------------------------- Scenes ---------------------------- */

function jr_hide_all() {
  var i;

  for (i = 0; i < JR_FRUIT_MAX; i++) {
    if (J.fruits[i].used) {
      jr_fruit_hide(J.fruits[i]);
    }
  }
  for (i = 0; i < JR_PIECE_MAX; i++) {
    J.pieces[i].used = false;
    hide(J.pieces[i].obj);
  }
  for (i = 0; i < JR_PARTICLE_MAX; i++) {
    J.particles[i].used = false;
    hide(J.particles[i].obj);
  }
  for (i = 0; i < JR_POPUP_MAX; i++) {
    J.popups[i].used = false;
    hide(J.popups[i].lbl);
  }
  for (i = 0; i < JR_RING_MAX; i++) {
    J.rings[i].used = false;
    hide(J.rings[i].obj);
  }
  for (i = 0; i < JR_SLASH_MAX; i++) {
    J.slashes[i].used = false;
    hide(J.slashes[i].obj);
  }
  if (J.lbl_combo) {
    hide(J.lbl_combo);
  }
  J.last_combo_ui = 0;
  jr_trail_clear();
  J.drag = false;
}

function jr_overlay_clear() {
  if (J.overlay) {
    J.overlay.delete();
    J.overlay = null;
  }
}

function jr_overlay_new() {
  jr_overlay_clear();

  var o = lv.new('obj', J.ui, { x: 0, y: 0, w: W, h: H });

  o.call('lv_obj_remove_style_all');
  o.flag('scrollable', false);
  J.overlay = o;
  return o;
}

function jr_show_start() {
  var i;

  J.state = JR_STATE_START;
  jr_hide_all();
  hide(J.hud);

  var ov = jr_overlay_new();

  /* Dim the empty playfield behind the start page. */
  shape(ov, 0, 0, W, H, 0x05080E, 90, 0);

  /* Decorative halo behind the title. */
  var halo = ring(ov, 0, 0, 220, 0x2C5388, 110, 2);

  halo.align('top_mid', 0, 20);

  /* Mode chip. */
  var chip = shape(ov, 0, 0, 170, 32, 0x13253D, 180, RADIUS_CIRCLE);

  chip.align('top_mid', 0, 54);
  chip.setStyle('border_width', 2);
  chip.setStyle('border_color', 0x2F5A8E);

  var lbl = label(chip, 'CHALLENGE MODE', 12, 0x7FC8FF, 1);
  lbl.center();

  lbl = label(ov, 'JUICE RUSH', 32, 0xFFFFFF, 6);
  lbl.align('top_mid', 0, 104);

  lbl = label(ov, 'SLIDE  TO  SLICE', 16, JR_COLOR_DIM, 3);
  lbl.align('top_mid', 0, 146);

  /* Gradient divider under the subtitle. */
  var bar = shape(ov, 0, 0, 200, 3, 0x1D3B5E, OPA_COVER, RADIUS_CIRCLE);

  bar.align('top_mid', 0, 180);
  bar.setStyle('bg_grad_color', 0x3ED47A);
  bar.setStyle('bg_grad_dir', 2);       /* LV_GRAD_DIR_HOR */

  /* Decorative fruit row, slightly arched. */
  var deco_dy = [-8, -3, 0, -3, -8];
  var base_x = W / 2 - 96;

  for (i = 0; i < 5; i++) {
    jr_deco_fruit(ov, base_x + i * 48, 214 + deco_dy[i], 18, i);
  }

  lbl = label(ov, 'MISS A FRUIT  -1 LIFE      BOMB  -1 LIFE', 12, 0x5D728C, 0);
  lbl.align('top_mid', 0, 254);

  lbl = label(ov, 'BEST  ' + J.best, 16, 0xFFD54A, 0);
  lbl.align('top_mid', 0, 278);

  var start = jr_button(ov, 'START', 220, 50);

  start.align('top_mid', 0, 304);
  start.on('clicked', jr_start_game);

  /* Secondary button: back to the carousel. */
  var home = jr_button(ov, 'HOME', 220, 42);

  jr_button_secondary(home);
  home.align('top_mid', 0, 366);
  home.on('clicked', jr_go_home);
}

function jr_build_over() {
  var ov = jr_overlay_new();

  /* Dark overlay behind the result dialog. */
  shape(ov, 0, 0, W, H, 0x05080E, 170, 0);

  var card = lv.new('obj', ov, { x: 0, y: 0, w: 380, h: 320 });

  card.call('lv_obj_remove_style_all');
  card.flag('scrollable', false);
  card.setStyle('radius', 24);
  card.setStyle('bg_opa', OPA_COVER);
  card.setStyle('bg_color', JR_COLOR_CARD);
  card.setStyle('bg_grad_color', JR_COLOR_CARD2);
  card.setStyle('bg_grad_dir', 1);
  card.setStyle('border_width', 2);
  card.setStyle('border_color', JR_COLOR_LINE);
  card.center();

  var lbl = label(card, 'GAME OVER', 24, 0xFFFFFF, 3);
  lbl.align('top_mid', 0, 18);

  var divider = shape(card, 0, 0, 90, 3, 0x38D07A, OPA_COVER, RADIUS_CIRCLE);
  divider.align('top_mid', 0, 52);

  lbl = label(card, 'SCORE', 12, JR_COLOR_DIM, 4);
  lbl.align('top_mid', 0, 70);

  lbl = label(card, '' + J.score, 32, 0xFFFFFF, 0);
  lbl.align('top_mid', 0, 90);

  lbl = label(card, 'BEST  ' + J.best, 16, 0xFFD54A, 0);
  lbl.align('top_mid', 0, 132);

  lbl = label(card, 'MAX COMBO  x' + (J.max_combo < 1 ? 1 : J.max_combo), 16, 0x7FB8E8, 0);
  lbl.align('top_mid', 0, 158);

  /* Highlight badge when a new record is set. */
  if (J.new_record) {
    var badge = shape(card, 0, 0, 170, 32, 0x3A2A08, 200, RADIUS_CIRCLE);

    badge.align('top_mid', 0, 186);
    badge.setStyle('border_width', 2);
    badge.setStyle('border_color', 0xFFD54A);

    lbl = label(badge, 'NEW RECORD', 16, 0xFFD54A, 2);
    lbl.center();
  }

  var again = jr_button(card, 'PLAY  AGAIN', 160, 42);
  again.align('bottom_left', 18, -16);
  again.on('clicked', jr_start_game);

  /* Secondary button: dark fill, lower visual weight. */
  var home = jr_button(card, 'HOME', 160, 42);
  jr_button_secondary(home);
  home.align('bottom_right', -18, -16);
  home.on('clicked', jr_go_home);
}

function jr_game_over() {
  if (J.state !== JR_STATE_PLAY) {
    return;
  }
  J.state = JR_STATE_OVER;
  J.new_record = (J.score > J.best);
  if (J.new_record) {
    J.best = J.score;
  }
  jr_hide_all();
  hide(J.hud);
  jr_build_over();
}

function jr_start_game() {
  jr_overlay_clear();
  jr_hide_all();

  J.score = 0;
  J.combo = 0;
  J.max_combo = 0;
  J.new_record = false;
  J.lives = 3;
  J.last_score_ui = -1;
  J.last_sec_ui = -1;
  J.last_pct_ui = -1;
  J.last_combo_ui = -1;
  J.start_tick = lv.call('lv_tick_get');
  J.last_tick = J.start_tick;
  J.next_spawn_tick = J.start_tick + 600;
  J.last_slice_tick = 0;
  J.state = JR_STATE_PLAY;

  jr_hearts_update();
  jr_hud_update(true);
  show(J.hud);
}

/* ---------------------------- Build ---------------------------- */

function jr_build_hud() {
  var lbl, pill, i;

  J.hud = lv.new('obj', J.ui, { x: 0, y: 0, w: W, h: JR_HUD_H });
  J.hud.call('lv_obj_remove_style_all');
  /* Not clickable, so the top area can still be swiped. */
  J.hud.flag('clickable', false);
  J.hud.flag('scrollable', false);

  /* Score pill background. */
  pill = shape(J.hud, 12, 6, 108, 54, 0x101E33, 170, 16);
  pill.setStyle('bg_grad_color', 0x0A1423);
  pill.setStyle('bg_grad_dir', 1);
  pill.setStyle('border_width', 2);
  pill.setStyle('border_color', 0x25415F);

  lbl = label(pill, 'SCORE', 12, JR_COLOR_DIM, 2);
  lbl.align('top_mid', 0, 3);

  J.lbl_score = label(pill, '0', 24, 0xFFE08A, 0);
  J.lbl_score.align('bottom_mid', 0, -2);

  /* Lives pill: the width derives from three hearts, gaps and padding. */
  var hd = 18;
  var hg = 8;
  var pad = 14;
  var pw = 3 * hd + 2 * hg + pad * 2;   /* 98 */
  var ph = 54;
  var px = W - 12 - pw;
  var hx0 = px + pad;
  var hy = 6 + Math.floor((ph - hd) / 2);

  pill = shape(J.hud, px, 6, pw, ph, 0x101E33, 170, 16);
  pill.setStyle('border_width', 2);
  pill.setStyle('border_color', 0x25415F);

  for (i = 0; i < 3; i++) {
    var heart = jr_ball(J.hud);

    show(heart);
    heart.setSize(hd, hd);
    heart.setStyle('border_width', 0);
    heart.setPos(hx0 + i * (hd + hg), hy);
    J.hearts[i] = heart;
  }
  jr_hearts_update();

  /* Timer pill, centered and aligned with the two side pills. */
  pill = shape(J.hud, (W - 100) / 2, 6, 100, 54, 0x101E33, 170, 16);
  pill.setStyle('bg_grad_color', 0x0A1423);
  pill.setStyle('bg_grad_dir', 1);
  pill.setStyle('border_width', 2);
  pill.setStyle('border_color', 0x25415F);

  lbl = label(pill, 'TIME', 12, JR_COLOR_DIM, 2);
  lbl.align('top_mid', 0, 3);

  J.lbl_time = label(pill, '1:00', 20, 0xFFFFFF, 0);
  J.lbl_time.align('bottom_mid', 0, -4);

  /* Time bar; the indicator gradient shifts green -> yellow -> red. */
  J.bar_time = lv.new('bar', J.hud, { x: 12, y: 68, w: W - 24, h: 6 });
  J.bar_time.setStyle('radius', 3, 'main');
  J.bar_time.setStyle('bg_opa', 220, 'main');
  J.bar_time.setStyle('bg_color', 0x16263C, 'main');
  J.bar_time.setStyle('border_width', 0, 'main');
  J.bar_time.setStyle('pad_top', 0, 'main');
  J.bar_time.setStyle('pad_bottom', 0, 'main');
  J.bar_time.setStyle('pad_left', 0, 'main');
  J.bar_time.setStyle('pad_right', 0, 'main');
  J.bar_time.setStyle('radius', 3, 'indicator');
  J.bar_time.setStyle('bg_opa', OPA_COVER, 'indicator');
  J.bar_time.setStyle('bg_color', 0x1B9C52, 'indicator');
  J.bar_time.setStyle('bg_grad_color', 0x6EF2A0, 'indicator');
  J.bar_time.setStyle('bg_grad_dir', 2, 'indicator');   /* LV_GRAD_DIR_HOR */
  lv.call('lv_bar_set_range', J.bar_time, 0, 100);
  lv.call('lv_bar_set_value', J.bar_time, 100, 0);

  /* Combo hint, shown briefly after a chain. */
  J.lbl_combo = label(J.ui, '', 20, 0x9FC3E8, 2);
  J.lbl_combo.call('lv_obj_set_width', W);
  J.lbl_combo.setStyle('text_align', 1);   /* LV_TEXT_ALIGN_CENTER */
  J.lbl_combo.setPos(0, 104);
  hide(J.lbl_combo);

  hide(J.hud);
}

function jr_build_pools() {
  var i;

  for (i = 0; i < JR_FRUIT_MAX; i++) {
    var pair = jr_ball_pair(J.play);

    J.fruits.push({
      ball: pair.ball, shine: pair.shine, used: false, bomb: false, kind: 0,
      x: 0, y: 0, vx: 0, vy: 0, r: 28
    });
  }

  for (i = 0; i < JR_PIECE_MAX; i++) {
    var piece = jr_ball(J.play);

    piece.setStyle('border_width', 0);
    hide(piece);
    J.pieces.push({ obj: piece, used: false, x: 0, y: 0, vx: 0, vy: 0, r: 8, ttl: 0, ttl_max: 1 });
  }

  for (i = 0; i < JR_PARTICLE_MAX; i++) {
    var part = jr_ball(J.play);

    part.setStyle('border_width', 0);
    hide(part);
    J.particles.push({ obj: part, used: false, x: 0, y: 0, vx: 0, vy: 0, r: 3, ttl: 0, ttl_max: 1 });
  }

  for (i = 0; i < JR_POPUP_MAX; i++) {
    var pl = label(J.play, '', 20, 0xFFFFFF, 0);

    pl.call('lv_obj_set_width', 160);
    pl.setStyle('text_align', 1);
    hide(pl);
    J.popups.push({ lbl: pl, used: false, x: 0, y: 0, ttl: 0, ttl_max: 1 });
  }

  /* Shockwave ring: transparent circle with only a border. */
  for (i = 0; i < JR_RING_MAX; i++) {
    var ro = ring(J.play, 0, 0, 20, 0xFFFFFF, 0, 6);

    hide(ro);
    J.rings.push({ obj: ro, used: false, x: 0, y: 0, r_start: 0, r_end: 0, ttl: 0, ttl_max: 1 });
  }

  /* Slash flash: two-point line with static geometry that only fades. */
  for (i = 0; i < JR_SLASH_MAX; i++) {
    var line = lv.new('line', J.play, { x: 0, y: 0, w: 10, h: 10 });

    line.call('lv_obj_remove_style_all');
    line.flag('clickable', false);
    line.setStyle('line_width', 9);
    line.setStyle('line_rounded', 1);
    hide(line);
    J.slashes.push({ obj: line, used: false, ttl: 0, ttl_max: 1 });
  }

  J.trail_obj = lv.new('line', J.play, { x: 0, y: 0, w: 10, h: 10 });
  J.trail_obj.call('lv_obj_remove_style_all');
  J.trail_obj.flag('clickable', false);
  J.trail_obj.setStyle('line_color', 0xFFFFFF);
  J.trail_obj.setStyle('line_width', 12);
  J.trail_obj.setStyle('line_rounded', 1);
  J.trail_obj.setStyle('line_opa', JR_TRAIL_OPA);
  hide(J.trail_obj);
}

/* ---------------------------- Entry points ---------------------------- */

function juice_rush_reset() {
  J.root = null;
  J.bg = null;
  J.play = null;
  J.ui = null;
  J.touch = null;
  J.overlay = null;
  J.hud = null;
  J.lbl_score = null;
  J.lbl_time = null;
  J.lbl_combo = null;
  J.bar_time = null;
  J.trail_obj = null;
  J.hearts = [];
  J.fruits = [];
  J.pieces = [];
  J.particles = [];
  J.popups = [];
  J.rings = [];
  J.slashes = [];
  J.trail = [];
  J.trail_opa = 0;
  J.drag = false;
  J.timer = 0;
  J.max_combo = 0;
  J.new_record = false;
  J.last_combo_ui = 0;
  J.state = JR_STATE_IDLE;
}

function juice_rush_stop() {
  if (J.timer) {
    timer.clear(J.timer);
    J.timer = 0;
  }
  if (J.root) {
    J.root.delete();
  }
  juice_rush_reset();
}

function jr_go_home() {
  showLauncher();
}

function juice_rush_start() {
  juice_rush_reset();
  lv.call('lv_rand_set_seed', lv.call('lv_tick_get'));

  /* The screen itself must not scroll, otherwise drags become page scrolls. */
  lv.screen().flag('scrollable', false);

  J.root = lv.new('obj', lv.screen(), { x: 0, y: 0, w: W, h: H });
  J.root.call('lv_obj_remove_style_all');
  J.root.flag('scrollable', false);
  J.root.flag('clickable', false);
  J.root.setStyle('bg_opa', OPA_COVER);
  J.root.setStyle('bg_color', 0x172644);
  J.root.setStyle('bg_grad_color', 0x070B14);
  J.root.setStyle('bg_grad_dir', 1);

  jr_build_bg();

  J.play = lv.new('obj', J.root, { x: 0, y: 0, w: W, h: H });
  J.play.call('lv_obj_remove_style_all');
  J.play.flag('scrollable', false);
  J.play.flag('clickable', false);

  J.touch = lv.new('obj', J.root, { x: 0, y: 0, w: W, h: H });
  J.touch.call('lv_obj_remove_style_all');
  J.touch.flag('scrollable', false);
  J.touch.flag('clickable', true);
  J.touch.flag('press_lock', true);
  J.touch.setStyle('bg_opa', OPA_TRANSP);
  J.touch.on('pressed', function () { jr_touch_cb(0); });
  J.touch.on('pressing', function () { jr_touch_cb(1); });
  J.touch.on('released', function () { jr_touch_cb(2); });
  J.touch.on('press_lost', function () { jr_touch_cb(3); });

  J.ui = lv.new('obj', J.root, { x: 0, y: 0, w: W, h: H });
  J.ui.call('lv_obj_remove_style_all');
  J.ui.flag('scrollable', false);
  J.ui.flag('clickable', false);

  jr_build_pools();
  jr_build_hud();

  J.best = 0;
  J.state = JR_STATE_IDLE;
  J.last_tick = lv.call('lv_tick_get');

  J.timer = timer.setInterval(jr_tick, JR_TICK_MS);

  jr_show_start();
}
/*
 * Entry point and page router.
 *
 * The bundle is evaluated once at start-up; every page is a plain function so
 * switching pages only deletes the previous root object and builds the next
 * one, exactly like launcher_start()/juice_rush_start()/game2048_start() do in
 * the C version.
 */

function showLauncher() {
  launcher_stop();
  game2048_stop();
  juice_rush_stop();
  launcher_start();
}

function show2048() {
  launcher_stop();
  juice_rush_stop();
  game2048_start();
}

function showJuice() {
  launcher_stop();
  game2048_stop();
  juice_rush_start();
}

/* Boot into the carousel, matching the power-on screen. */
launcher_start();
