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
