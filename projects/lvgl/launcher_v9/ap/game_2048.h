/**
 * @file game_2048.h
 * @brief 2048 tile-merging game public interface (LVGL v9, plain C).
 */
#ifndef GAME_2048_H
#define GAME_2048_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start 2048: build the scene and begin a round.
 *
 * Call only after LVGL is initialized and while holding the display lock
 * (lv_vendor_disp_lock()).
 */
void game_2048_start(void);

/**
 * @brief Stop 2048 and delete every object of the scene.
 */
void game_2048_stop(void);

/**
 * @brief Clear internal pointers only (after LVGL deinit; touches no LVGL object).
 */
void game_2048_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* GAME_2048_H */
