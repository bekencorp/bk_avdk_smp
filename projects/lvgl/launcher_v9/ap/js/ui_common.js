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
