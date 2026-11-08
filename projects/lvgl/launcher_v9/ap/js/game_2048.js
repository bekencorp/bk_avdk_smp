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
