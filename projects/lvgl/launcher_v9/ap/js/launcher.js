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
