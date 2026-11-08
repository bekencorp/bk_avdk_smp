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
